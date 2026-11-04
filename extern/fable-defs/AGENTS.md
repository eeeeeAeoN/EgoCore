# fable-defs — Def Compiler: Architecture & Handoff Guide

fable-defs is a **Rust** workspace providing a **from-scratch def compiler** (`defc`) that
compiles the text `.def` sources from Fable: The Lost Chapters' debug build into the retail
binary format: `game.bin`, `frontend.bin`, `script.bin`, and the shared `names.bin`.

**Where it stands:** all four binaries compile entirely from text — no retail binary is read
at build time — and the retail engine loads and runs the output (save-load verified). Golden
byte-determinism holds, the workspace is warning-clean, and the semantic ledger (§6) surfaces
only verified artifacts. Divergence closure is done; the text layer has one unified grammar;
diagnostics point at real source with real spans.

This document is the architecture reference *and* the operational handoff. Keep it current.
It deliberately records **decisions and their reasons**, not project history — if you want to
know what changed when, read the git log.

---

## Contents

1. [Orientation](#1-orientation)
2. [Pipeline](#2-pipeline)
3. [Central design fact: typed structs are the IR](#3-central-design-fact-typed-structs-are-the-ir)
4. [Layer guide](#4-layer-guide)
5. [Diagnostics](#5-diagnostics)
6. [Verification & the bug-fixing loop](#6-verification--the-bug-fixing-loop)
7. [The manifest](#7-the-manifest)
8. [Settled decisions](#8-settled-decisions)
9. [Roadmap](#9-roadmap)
10. [Operational reference](#10-operational-reference)

---

## 1. Orientation

### Workspace crates

| Crate | Type | Role |
|---|---|---|
| `defs` | lib | Format library: text parsing, the typed def model, binary I/O, the `SemVal` differ |
| `defs-derive` | proc-macro | The five schema derives (`DefStruct`/`WireStruct`/`DefVariant`/`DefEnum`/`DefFlags`) |
| `def-compiler` | lib | Lowering (text → typed structs) and the whole build pipeline |
| `defc` | bin | CLI front end: argument parsing, progress output, diagnostic rendering, exit code |
| `def-compiler-sys` | cdylib | C ABI for downstream consumers (EgoCore); renders diagnostics to a plain-text log |

### Key external resources

| Resource | Path |
|---|---|
| Fable decompilation | `~/git/fable-decomp` — the `Transfer<T>` type/order oracle (§6). **Absent at time of writing** |
| Retail binaries (ground truth) | `~/Fable/data/CompiledDefs/backup-retail-verified/` |
| Anniversary debug build (text defs) | `~/doc/Fable_Anniversary-2013-02-25/Fable/Data/` |
| Live game install (ships **both** header sets, §4.1) | `~/Fable/data/Defs/` |
| EgoCore (downstream; links `def-compiler-sys`) | `~/git/EgoCore` — its C++ `FableDefCompiler/` is a behavioural oracle |

```bash
REF=~/Fable/data/CompiledDefs/backup-retail-verified
TEXT=~/doc/Fable_Anniversary-2013-02-25/Fable/Data
cargo run -q -p defc -- $TEXT/Defs <out_dir>
```

### Scale (measured, release build, Anniversary corpus)

| | |
|---|---|
| Source files parsed | 177 `.def`/`.tpl` + 61 `.h` |
| Definitions | 10,454 |
| Output entries | game 13,239 · frontend 851 · script 611 |
| game.bin composition | NULLDEFs + 9,264 named + 3,726 distinct anonymous sub-defs (33,525 lowered, deduped) |
| **Total build** | **~0.56 s** (was 3.8 s before §9 Track A) |

**Per-phase, on the game.bin path** (~85% of the build; frontend + script are ~0.06 s). Measured
by replaying the pipeline through the public API — see the recipe in §10.

| Phase | before | now |
|---|---|---|
| parse 61 headers + evaluate symbols | 49 | 44 |
| parse 177 `.def`/`.tpl` | 138 | 133 |
| index space (`collect_body_references` + `collect_named`) | 25 | 5 |
| lower 249 NULLDEF bodies | 3 | 2 |
| flatten the specialization chain, named pass | 390 | **4** |
| flatten again for the sub-def pass — a duplicate call | 345 | 0 |
| lower ~9,264 named bodies | 291 | **82** |
| tagged-block merge → 33,525 sub-def inputs | 46 | 14 |
| lower 33,525 sub-def bodies (→ 3,726 distinct) | 269 | **~25** (126 un-memoized) |
| move bodies into `EntryRecord` | 102 | 102 |
| serialize ~12.5k entries (6.8 MB) | 15 | 15 |
| **chunk split** | **1,804** | **32** |
| zlib, 434 chunks | 35 | 35 |

> **The old headline "lower + emit game.bin = 88%" was an artefact of the measurement**, which
> timed the gap between two progress lines and attributed all of it to lowering. Lowering was
> 15%, and nearly half the build was an accidental quadratic in the chunk split. Everything
> removed was **redundant work, not necessary work** — the output stayed byte-identical at every
> step. Re-measure before optimising anything; these numbers drive §9.

Reading and parsing now run in parallel (§9 A.3 step 1), which is why the total is below the sum
of the sequential phases above. Parsing was 152 ms of a 650 ms build; largest-first scheduling
across 12 cores took it to ~17 ms. What is left is **allocation-bound, not algorithm-bound**:
~1.27 M allocations for 1.12 M tokens, about one per token.

---

## 2. Pipeline

```
 INPUTS                     defs (format layer)             def-compiler
┌──────────────┐   ┌──────────────────────────────────┐   ┌────────────────────────┐
│ Defs/*.def    │──▶│ parse_source → SourceAst {        │──▶│ specialization_chain   │
│ Defs/*.tpl    │   │   items: Vec<Item> }              │   │  (read parent-first)   │
│ Defs/**/*.h   │──▶│   Item = Definition | Enum |      │   │ lower_def dispatch:    │
│               │   │     Define | Namespace |          │   │  • ~35 bespoke arms    │
│ manifest.rs   │   │     Conditional | PragmaOnce      │   │  • lower_generic via   │
│ (membership)  │   │ SymbolTable::evaluate_items       │   │    VisitFields         │
└──────────────┘   ├──────────────────────────────────┤   └────────────────────────┘
                    │ ~273 DefStruct + 86 WireStruct +  │                │
                    │ 4 DefVariant + 62 DefEnum +       │◀───────────────┘
                    │ 13 DefFlags = THE TYPED IR        │
                    │ binary.rs: container + SemVal     │   ┌────────────────────────┐
                    └──────────────────────────────────┘   │ build.rs: per binary   │
                                                            │  • collect_named       │
 VERIFICATION: golden.rs (byte determinism — THE gate)      │  • NULLDEFs            │
              · verify.rs semantic ledger vs retail         │  • sub-def tables      │
              · in-game load                                │  • NamesBuilder        │
                                                            │  • 16 KB zlib chunks   │
                                                            └────────────────────────┘
```

**Stage 0 — inputs.** Text corpus (`.def` = concrete `#definition`s, `.tpl` = abstract
`#definition_template`s; both must load so `specialises` chains resolve), C-like header files,
and the static manifest (per-binary membership + NULLDEF lists, extracted once from retail).

**Stage 1 — parsing** (`defs::text`). One grammar, one parser, one AST for every file kind
(§4.1). A definition body is three statement forms:

```text
Health 100;                            → Statement::Field   (path, expr)
Controls.Add(CActionInputControl(…));  → Statement::MethodCall (object path, call)
<CTavernGameDef> … <\CTavernGameDef>   → Statement::TaggedBlock (tag, body)
```

**Stage 2 — specialization** (`lower::specialization_chain`). The `specialises`
chain is read most-distant-ancestor-first as one statement sequence (`Body::Chain`, never
materialized — §9). With downstream last-wins semantics this reproduces the game compiler's
copy-parent-then-apply. Same-tag tagged blocks **merge** (parent first), never replace.

**Stage 3 — lowering** (`def-compiler::lower`). `lower_def` dispatches on def type name.
~35 bespoke arms handle C++-specific logic (§4.5); the remaining ~210 flow through
`lower_generic::<T>`.

**Stage 4 — assembly** (`def-compiler::build`). Per binary: allocate a global index space
(NULLDEF region first, then named entries in first-seen corpus order, filtered by manifest),
lower every body, generate sub-def tables + deduplicated anonymous entries (game.bin only),
intern all strings into one shared `NamesBuilder`, pack into ≤16 KB zlib chunks, serialize.

**Stage 5 — serialization** (`defs::binary` + `wire`). Every typed struct serializes itself;
the container layer adds headers, name-ref tables, the chunk index, and compression.

---

## 3. Central design fact: typed structs are the IR

**The typed def structs are the compiler's IR**; the pipeline is `text AST → typed structs →
bytes`. Each `#[derive(DefStruct)]` struct is simultaneously (1) the binary layout spec,
(2) the parse target when reading retail bins, (3) the lowering target when compiling text,
(4) the reflection subject for generic lowering and the differ. **Layout is declared exactly
once**, so parse/serialize/size can never disagree, and golden pins the whole chain.

Proportions: ~4.5k lines of logic over ~22k lines of declarations. Most of the repo is schema,
not machinery — the right shape for a format compiler, and the reason most of the backlog is
about moving *more* knowledge from machinery into schema.

**Rejected alternatives** (don't relitigate): a mid-level "resolved statement" IR between text
AST and typed structs (adds a layer for the ~210 types that don't need one, changes nothing
for the ~35 bespoke arms whose logic is irreducible retail-compiler knowledge); a
dynamic/schema-interpreted def model (loses compile-time layout checking, makes every bespoke
arm stringly-typed).

---

## 4. Layer guide

### 4.1 Text layer (`defs/src/text/`)

| File | Contents |
|---|---|
| `lexer.rs` | `lex(input, file)` → `Vec<Token>`; `Cursor`; `TokenKind`; `TextParseErrorKind` |
| `mod.rs` | **The whole grammar**: `Item`, `SourceAst`, the AST types, and every production |
| `symbols.rs` | `SymbolTable::evaluate_items` — declarations → flat `name → i64` |
| `base.rs` | `FileId`, `Span`, `Spanned<T>`, `LineIndex`, `ParseContext`, `ParseError<T>` |

**One grammar for every file under `Defs/`.** There is no separate header parser. A file —
`.def`, `.tpl`, or `.h` — is a sequence of `Item`s: definitions *and* declarations
(`enum`, `#define`, `namespace`, `#ifdef`/`#ifndef`, `#pragma once`).

> **Why:** the split was conventional, not structural. `engine_local_detail.def` declares two
> `enum`s whose 31 symbols appear in no header and are referenced ~1,800 times, so a `.def`
> legitimately carries declarations. Worse, the two parsers had different top-level item sets
> and opposite strictness, so **the same text meant different things depending on extension**:
> `#ifdef` was a real conditional in a `.h` and silently-skipped junk in a `.def`, which meant
> a guarded `#define` in a `.def` leaked its symbol unconditionally.

File *discovery* still keys off the extension — header-set variant resolution applies to `.h`,
and only `.def`/`.tpl` contribute definitions. Only the grammar is unified.

Load-bearing properties:

- **A span identifies its own file** (today: `Span { file: FileId, start, end }`).
  Non-negotiable: a template's statements are read by every inheriting definition, spans
  included, so a span routinely outlives the file it was read from. A span that cannot say which
  file it belongs to makes a diagnostic interpret the offset against the wrong text and point
  confidently at unrelated code.

  > This is the *property*, not the representation. §9 A.3 step 2 plans to keep the property
  > while dropping the per-span `FileId`, by giving every file a disjoint range in one global
  > byte-offset space — offsets then identify the file on their own. Any redesign must preserve
  > the property; storing the file id 1.1 M times is not required to.

  There is **deliberately no `Span::join`** — combining spans is only meaningful within one file
  and nothing can guarantee that; productions needing a multi-token range use
  `Span::new(cursor.file(), start, end)`, taking the file from the cursor. That rule survives a
  global offset space unchanged.
- **AST nodes carry spans** (`Spanned<Statement>`, `Spanned<Expr>`, `PathSegment::Index`,
  `Item::Definition(Spanned<Definition>)`, and all declaration names). `Spanned<T>: PartialEq`
  ignores the span. Corpus files are CRLF; `\r` is trivia.
- **Statement order is preserved** through parsing and flattening — load-bearing, because
  method-call DSLs (`Animation.StartGroup` … `Add` … `EndGroup`) and `Field.clear()` have
  positional semantics.
- **A valueless `#define NAME` binds no symbol.** 46 of them in the corpus (every include
  guard) against 25 with values. `Define.value` is `Option<i64>`; in C, using a valueless
  define as a number is an error too, and this keeps guard names out of the table entirely.
- **Include guards are ordinary conditionals** and evaluate correctly on their own, so there
  is no prologue/epilogue special-casing. `#endif __GUARD__` (the pre-C99 idiom) is accepted.
- **`SymbolTable` duplicates: last definition wins**, never fatal. Matches the C preprocessor
  and the retail tooling's `m_SymbolMap[name] = value`. `evaluate_items` returns
  `Vec<Redefinition>` that the builder reports as located warnings.
- **Text outside any item is skipped, recorded, and not reported.** `SourceAst.ignored` holds
  the coalesced byte ranges. The canonical corpus has four such runs (leftover bodies of two
  commented-out definitions, a stray identifier, a duplicate `#end_definition`) — see §5 for
  why nothing warns about them today.
- **Decorative banners are lexer trivia.** An all-separator line (`/*****…` with no closer
  anywhere after it) is a section divider, not a comment. This *cannot* be replaced by
  "ignore text outside definitions": it is a lexer concern and "outside a definition" is a
  parser notion. Measured: 65 firings, all outside definitions, none in headers. A `/*` with
  real content and no closer is still `UnterminatedBlockComment`.
- **Greedy comment pairing is faithful, not a bug.** A `/*` pairs with the first `*/` anywhere
  after it (the C rule). The old char parser did the same and the corpus never triggers the
  pathological case.
- **`Expr::Number(String)` keeps literals raw.** Interpretation is type-specific per field and
  lives in `reader.rs::Evaluator`: a float-shaped literal in an int context truncates, `f32`
  strips a trailing `f`. All asymmetries are corpus-driven and golden-verified.

#### Header-set variant resolution

`Data/Defs` does **not** ship one flat set of headers. It ships *complete variants of the same
set*, along two axes:

| Axis | Variants | Resolution |
|---|---|---|
| Build | `RetailHeaders/` vs `DevHeaders/` | `HEADER_SET_ROOTS`, most-preferred first; only the winner is scanned |
| Platform | `pc/` vs `xbox/` within a set | `IGNORED_PLATFORM_DIRS` skips `xbox` |

`RetailHeaders` wins: it is the richer set (the `DevHeaders` lipsync headers are empty stubs)
and it is what the modding tools write to. The two agree on every shared name/value pair, so
the choice is byte-neutral for a stock corpus — verified three ways. It decides only *which
set a mod's edits are read from*. The Anniversary corpus ships only `DevHeaders/`. When a set
is skipped the build warns, naming it.

> **The bug this prevents:** both sets were once scanned; `DevHeaders/*` sorted first and
> claimed every symbol; each `RetailHeaders/*` file then hit a duplicate on its *first* enum
> variant and — because a duplicate aborted the rest of the file — was discarded whole.
> ~44,900 symbol definitions dropped at warning severity. A mod that patched `RetailHeaders/`
> saw its five new symbols surface as one `unknown constant` error 5,279 lines away. Two
> independent defects: fatal duplicates, and unioned variant sets. Both fixed; regression
> tests in `build.rs::tests` and `symbols.rs::tests`.

### 4.2 Schema layer — proc-macro derives (`defs-derive`)

| Derive | Instances | Generates |
|---|---|---|
| `DefStruct` | ~273 | `Default`/`DefDefault` + control-level `parse`/`serialize`/`byte_size` + `visit_fields` + `Wire` + `StructSlot` (`visit_named=true`) + `AsField` |
| `WireStruct` | 86 | compound value (members, **no** control ids) |
| `DefVariant` | 4 | tagged union (`u32` tag + case fields) |
| `DefEnum` | 62 | closed i32 enum with C++ symbol names; out-of-table parse is an **error** |
| `DefFlags` | 13 | bit-set newtype (for "enums" the game ORs together) |

```rust
#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureDef {
    #[def("PhonemeAnim")]    pub phoneme_anim: VecMap<String, i32>,
    #[def("AllowedInProtectedTowns", default = true)] pub allowed_in_protected_towns: bool,
    //   wire name (crc32 id)          ctor default        rust field        wire type
}
```

`#[def("WireName")]` = the field's def-script name (its `crc32` id); `default = <expr>`
overrides `DefDefault` for the never-parsed NULLDEF body. `tag = "sibling_field"` marks a
**polymorphic field** whose in-memory variant is chosen by an earlier-declared sibling's value
(`UiDef.mesh_index`/`MeshRef`).

**Wire encodings** (byte-verified against every entry of all three retail bins): `f32`/`i32`/
`u32`/`u16`/`u8` LE; `bool` 1 byte; `String` NUL-term UTF-8; `WStr` NUL-term UTF-16LE;
`PString` u32 len + bytes; `DefString` i32 names.bin offset (default −1); `DefIndex` i32 global
index (default 0); `Vec<T>` u32 count + elems; `[T;N]` N elems no count; `BTreeMap` u32 count +
key-ordered pairs (`std::map`); `VecMap` same shape, stored order preserved (`CVectorMap`);
`DefEnum` 4 bytes must be in table; `DefVariant` u32 tag + case fields.

A `DefStruct` body is a sequence of **field controls**: `u32 crc32(wire name)` + wire value, in
declaration order. `crc32` ids are **validated, not dispatched**. The `DefBody` enum and its
dispatch are generated by a local `def_body!` macro in `binary.rs` from an inlined table of
~247 rows. **Proc macros can't enumerate a crate's types, so this central list is irreducible.**

### 4.3 Reflection layer (`defs/src/visit.rs`)

`FieldRef<'a>` is a typed mutable handle to one field (one variant per wire kind, plus
`Vec`/`Map`/`Struct`/`Variant`/`Array` slot traits and a `Complex` escape hatch).
`visit_fields` pushes each field with its **wire name**. Consumers: generic lowering (mutating
walk) and the semantic differ (reading walk). Known limits, all from the `&mut`-only design:
`MapSlot::for_each_pair` clones keys/values to read them; three overlapping surfaces.

### 4.4 Lowering engine (`def-compiler`)

Three primitives in `reader.rs`:

- **`Evaluator`** — single source of truth for evaluating one `Expr` against the symbol table.
  Handles `NULL` (→0), `TRUE/FALSE/BTRUE/BFALSE`, `|` and `+` folds. Deliberate asymmetries
  (e.g. `f32` doesn't accept `NULL`) reflect the corpus — extend only with corpus evidence.
- **`Args`** — positional reader over ctor/method-call argument lists.
- **`DefReader`** — scans statements by name at a path depth; on duplicates **last wins**
  (specialization concatenates parent+child, so last-wins realizes child-overrides-parent).
  Combinators: `group`, `indexed_sparse`, `keyed`, `calls`, `any_*`, `finish`.

**Pull model.** The obvious "iterate statements, look up field by name" is blocked by the
borrow checker (can't hold a `name → FieldRef` map — aliasing `&mut`). So `lower_generic`
clones the NULLDEF base and an `Applier` (a `FieldVisitor`) walks fields via reflection, each
field *pulling* its matching statements. Order-sensitive semantics that don't survive inversion
are handled by a pre-pass (`strip_superseded_by_clear`) or by interception before the generic
walk (method-call DSLs).

**Unconsumed statements are silently dropped.** This matches C++ `Transfer` skipping unknown
field names, which cross-type specialization needs. There is no warning: see §9 for why the
previous attempt was removed and what reviving it would require.

`DefReader::finish()` is different and **is** live — it validates a *group* sub-reader (a Vec
element, map value, or indexed sub-struct), which really does own every statement handed to it.

### 4.5 Bespoke lowering catalog

Everything the generic walk can't do lives in ~35 arms in `lower_def`. The decomp is the
authority.

**A. Method-call DSL interception** (statement-order replay of a C++ builder API):
`CAnimatingObjectDef`/`CAppearanceDef` (`Animation.Add/StartGroup/…`, key=crc(name),
stable-sorted); `CCreatureDef` (`Expressions.Add`, `WoundMorphs.*`, 40-byte packets);
`CEntitySoundDef` (`SoundMap.*`, crc-keyed, sorted after each add); `CFlammableDef`
(`std::map<CDefString,…>` ordered by TablePos not strcmp); `CHeroMorphDef`;
`OPINION_DEED_EFFECTS` (seconds→frames ×15, **x87 single-rounding** via f64 intermediates);
`OPINION_PERSONALITY` (5×36-byte blob, args 6/7 swap into slots 7/6);
`OPINION_REACTION_MANAGER` (ctor families precompute inverse radii / per-frame rates,
hurried-set at idx+79); `CSpecialEffectsDef` (crc-keyed, dup insert = no-op **first** wins,
then sort); `CDegradableDef`; `CReplaceableMeshDef`; `CAppearanceModifierDef` (24-byte packet,
args reordered); THING family ×10 (`Components.Add/Remove`, one `lower_thing!` macro,
DriverType from the static 13-entry registry, universal `trailing_u32=28_011_726`).

**B. Container-semantics fixups** (post-walk): `CCombatAbilityBlock*` (`ValidBlockWeaponTypes`
is a `std::set` → sort+dedup); `PLAYER_GUI` (`VecMap` sorted by crc32(key));
`OPINION_REACTION_MANAGER` (eight `VecMap` fields sorted by key).

**C. Quirks:** `CAMERA_MANAGER` (consume `CameraList.clear()`); `CWeaponDef` (`WeaponTrails`
stored swapped); `CWillResponseDef` (strip Anniversary-only `ForceLightningable`);
`OPINION_SOURCE` (derive 79 bools from flag defaults+maps); `CLookDef.CombatMaxTurnSpeed`
(naive default propagation regresses ~503 entries — 1-entry divergence accepted, NOT
implemented).

**D. Compound ctor arg permutations** (in `apply_struct_from_expr`): `BlendedParticleEffectSet`,
`ObjectAugmentationParticleSet`, `ExplosionRing` (0/1 swap), `ParticleAttachmentInfo` (0/1
swap), `AttackHistoryCombo` (multiplier-first vs vec-first).

**E. Hand-written frontend lowerings:** `UI`, `CONTROL_SCHEME` (`CActionInputControl` slot
dispatch by controller type), `UI_MISC_THINGS_DEF`, `FRONT_END`. Other frontend types
(`ENGINE`, `ENVIRONMENT`, `ENVIRONMENT_THEME_DAY`, `ENGINE_VIDEO_OPTIONS`,
`CONFIG_OPTIONS_DEFAULTS_DEF`, `UI_ICONS_DEF`) flow through generic.

**~39 `.ok()` / `unwrap_or` sites remain** in these arms, where a bad constructor argument
silently becomes `0`/`-1`/`false` instead of erroring. See §9.

### 4.6 Assembly layer (`def-compiler/src/build.rs`)

One pipeline, three instantiations (game/frontend/script differ only in file collection,
manifest slices, and the game-only sub-def region):

1. **Index allocation** (`collect_named`): global index = position. NULLDEF region first, then
   one index per distinct named def in first-seen corpus order, filtered by `*_NAMED`. File
   order = sorted path list. ~800 duplicate instance names exist and **the whole later body
   replaces the earlier one** (last-processed-file wins), matching retail's
   `CompileDefinition`. The *first* occurrence keeps the index slot; the *last* supplies the
   body. No diagnostic (§9).
2. **NULLDEF bodies**: `lower_def(class, empty)` = the type's `def_default()`. Preamble
   `(false,false,0)`.
3. **Named bodies**: flatten → lower. Preamble `(true,false,1)` — the `1` is
   `AreDefaultValsApplied`, load-bearing (0 ⇒ save-load crash). **No fallback**: a def that
   fails to lower is not emitted.
4. **Sub-defs (game.bin only)**: merge same-tag tagged blocks across the specialization chain
   (CRC of tag, parent-first concat), lower each, serialize, dedup anonymous entries by
   **(class-tag, body bytes)** — byte-only dedup type-confuses combat sub-defs and crashes
   save-load. Anonymous `file_name_offset = 0xFFFFFFFF`.
5. **names.bin**: one shared `NamesBuilder` across all three; header words at +8/+12 are
   content-derived.
6. **Chunking**: greedy split at 16 KB decompressed, zlib level 1 (`78 01`).

> **Load-bearing ordering in this pass.** `build_subdefs` must run after *every* named entry
> exists: sub-def entries follow the named ones in the global index space and in the continuous
> `ClassIndex` counters, and their tags are interned into `names.bin` after every named def's
> strings. That is why the named pass resolves each specialization chain and hands the merged
> tagged blocks forward (`SubDefBlocks`, borrowed from the ASTs) instead of `build_subdefs`
> re-deriving them — and why sub-def *lowering* cannot simply be interleaved into the named loop
> without changing `names.bin`. Phase 2's symbolic strings (§9) are what would lift that
> constraint.

### 4.7 Binary container layer (`defs/src/binary.rs`)

```text
DefBinary   := header(13B) · NameRef[n](12B) · ChunkIndexHeader(8B)
             · ChunkIndexEntry[chunks](8B) · sentinel ChunkIndexEntry (REQUIRED —
               chunk offsets are relative to the region after it) · zlib chunks
NameRef     := def_name_offset(u32) · file_name_offset(u32) · ClassIndex(u32)
EntryRecord := preamble(3B: is_real, is_template, AreDefaultValsApplied)
             · [sub-def table: u16 count · 12B records]  — iff the type derives the sub-def bases
             · body: field controls (u32 crc32(name) · wire value)…
names.bin   := 20B header (off8=StringCount, off12=StreamLength) · (u32 crc · NUL utf8)…
```

- **`NameRef.ClassIndex` is load-bearing and the differ cannot see it.** It is
  `CInstantiatedDefInfo::ClassIndex`: **one 0-based counter per class**, running continuously
  in global-index order across all three emission passes — NULLDEF first, then named, then
  anonymous sub-defs. `CONTROL_SCHEME` has two NULLDEFs in frontend.bin, so they take 0 and 1
  and its named entries start at 2.

  > **`0` is the engine's "no such def" sentinel.** An unset `DefIndex` is `0`, resolving to
  > whichever NULLDEF sits at global index 0; consumers test the resulting class index against
  > `0` to mean "unset". Emitting 1-based indices makes every unset reference resolve to the
  > *first real def of that class*. That was the local-detail bug: 260 NULLDEFs were off by
  > one, so every theme without a `LocalDetailGeneratorDef` resolved to
  > `LOCAL_DETAIL_BRIGHTWOOD_BIRCH_BRACKEN`, and the world editor scattered birch and bracken
  > over bare ground or crashed on the collision.
  >
  > **Why the divergence hunt missed it:** `verify.rs` decodes entry *bodies* only. Nothing in
  > the ledger ever compared the `NameRef` table. A systematic 260-entry defect survived a
  > "divergence closure complete" sign-off because of that blind spot. `probe_counter` /
  > `probe_counter_diff` exist to check it; a ledger arm for `NameRef` would be better.

- **Sub-def table presence is a per-wire-name property** (`def_name_has_subdef_table`,
  generated from `sub_def_names!` — 106 wire names deriving `CSubDefClassBase`/
  `CParentDefClassBase`). Present ⇒ the u16 count is always serialized, even when 0.
- **Parse is total with typed fallback**: unknown type → `DefBody::Unknown{raw}`; a known type
  whose bytes don't match propagates an error and the entry keeps raw bytes intact.
- **Two error styles coexist**: the wire layer uses composable context wrapping
  (`ParseWireError::Member{name}/Item{index}` — the good pattern); the container layer uses
  one-enum-per-function. Unification candidate.

### 4.8 Reference semantics (cross-cutting)

- **External refs are by NAME; in-body refs are global indices.** The engine looks up
  Thing→def by instantiation name, which is why own-order index assignment is safe.
- **A field's *type* declares whether it is a reference.** The ≈90 once-mistyped reference
  fields are `DefIndex` (wire = `i32` LE); plain `i32`/`u32` fields evaluate strictly, so a
  typo in a numeric field errors instead of silently becoming a def index. Corpus-measured:
  94% of integer fields are purely numeric, ~5.5% purely references, ~0 genuinely both.

---

## 5. Diagnostics

**Two tiers, one policy each. No configurable failure policies.**

- **Parse errors: strict, one per file, fail-fast, no recovery.** The first error aborts that
  file (its defs drop), is rendered, and the build moves on. A file that fails to parse **must
  always be surfaced** — its defs silently vanish otherwise.
- **Compile diagnostics: collected, many per def.** **No fallback default def** — a def with
  ≥1 error is not emitted and the build exits non-zero after reporting everything.

Everything is **collected, not rendered**: `BuildReport::diagnostics` / `BuildError::diagnostics`
carry spans, and `BuildReport::sources` carries the text they point into. `defc` and
`def-compiler-sys` each render; the library never prints.

### The diagnostic model

```rust
struct BuildDiagnostic { severity, message, labels: Vec<DiagnosticLabel>, notes: Vec<String> }
struct DiagnosticLabel { source: usize, primary: bool, span: Span, message: Option<String> }
```

- **Labels carry their own file**, so one diagnostic can point into several. Required for
  correctness, not just convenience — see §4.1 on spans outliving their file.
- **Notes carry explanation**, keeping the headline scannable. `unknown constant` and
  `unknown definition` are the same shape of mistake in two namespaces; a note says which was
  searched.
- **No message on a primary label when it would repeat the headline.** The caret carries the
  location; the headline carries the words.
- **Secondary labels are for context worth visiting.** "in this definition" is attached only
  when the offending statement is in that definition's *own* body (an exact test —
  `Span::contains` compares the file). A definition that merely inherits a broken template
  contains no trace of the problem, so labelling it wastes the reader's trip.

### Merging

`Diagnostics::take()` merges repeats of one mistake and orders everything by (file, offset).

Specialization fan-out is why: a template's statements are read by
every descendant, so one bad statement is lowered and reported once per descendant, every
report pointing at the same byte of the same template. `OBJECT_BASE` has 3,147 transitive
descendants. Merged output is one diagnostic plus a count:

```
error: unknown constant `TEMPLATE_TYPO_XYZ`
    ┌─ objects.tpl:520:16
520 │      Health              TEMPLATE_TYPO_XYZ;
    │                          ^^^^^^^^^^^^^^^^^
    = constants come from `enum` / `#define` declarations
    = 1315 other definition(s) inherit this and fail for the same reason
```

**Genuinely distinct mistakes are never merged** — two typos of the same name on two lines have
different primary spans and both are reported in full.

### Parse-error context

`ParseError` carries a `Span` (so carets cover the offending token) and an optional
`ParseContext { what, name, span }` naming the construct the error happened *inside*. Attached
as productions unwind, innermost-first — the innermost production is the first to run on the
way out, and outer frames never overwrite it.

```
error: expected `=`, `,` or `}`, found identifier
     ┌─ Defs/gamesnds.h:1890:21
   8 │ enum    SND
     │         --- in enum `SND`
     ·
1890 │     SND_TROLL_LAUGH_01 REVERB = 1881,
     │                        ^^^^^^ expected `=`, `,` or `}`, found identifier
```

**Expected-token sets must be accurate.** `UnexpectedToken { expected, found }` builds the
"found …" half once in `Display`, so no site formats its own. The `expected` half must list
*every* token valid at that byte — an enum variant with no value yet can still take `=`, so
the set is conditional on how far the variant got. A confidently wrong expectation is worse
than a vague one; where the parser can't cheaply prove the full set, prefer a plain message
with a good span.

### The canonical corpus must compile silently

**0 warnings, 0 errors on the stock Anniversary corpus.** This is a hard invariant. The corpus
is the ground state; a build that is never quiet trains everyone to ignore the output.

Consequence: anything the stock corpus does — text outside definitions, ~800 duplicate
definition names, cross-type specialization leftovers — cannot be a warning as things stand.
`SourceAst.ignored` records the spans anyway (free, since the parser skips those tokens
regardless), waiting on a verbose mode that presents them as **notes** (§9).

---

## 6. Verification & the bug-fixing loop

Three layers, each proving something different:

| Layer | Proves | Does NOT prove |
|---|---|---|
| **`defc/tests/golden.rs`** (FNV hash of all 4 outputs; needs `OA_TEXT_DIR`; re-bless `OA_BLESS=1`) | Build is deterministic and byte-identical to the *blessed* build — **the regression gate for every refactor** | That the blessed build is correct vs retail (it can bake in bugs) |
| **`def-compiler/examples/verify.rs`** — decodes both sides to reference-resolved `SemVal`, classifies each entry `Reproduced`/`AcceptSort`/`OpaqueOnly`/`Bug`/`Missing` | **The divergence-from-retail oracle.** Where real bugs surface | Anything about *modified* defs — only in-game does |
| **In-game load** | The only proof that a *modified* or *fixed* def works | — |

`SemVal` decodes bodies through reflection, resolving `DefIndex→name` and `DefString→string`
per side, so two independent index spaces compare by *meaning*. `DiffPolicy::unordered()`
compares containers as multisets to separate MSVC sort-tie-break noise from real diffs.
Anonymous sub-defs are matched by **(parent instance name, tag)** — the one identity that
survives across index spaces.

> **The ledger is the primary bug-finding oracle, not noise.** It was once described as
> "informational only — `Bug`s are mostly noise". That was wrong and hid two large systematic
> default-value bugs (UI `Font` defaulting to null instead of `ENG_ARIAL_16`; THING
> `PersistenceFlags` defaulting to 0 instead of `EPF_STATIC`) affecting ~4,900 entries, masked
> as "index noise". Golden cannot see these because it compares to our own baseline.

### Sources of authority (when inputs disagree)

Ranked; higher wins.

1. **The retail binary itself** (`$REF`), read back through our own parser. Parsing is
   **positional** (each field control is `crc32(name)` + value in declaration order,
   crc-validated), so loading retail under our schema is a **structural oracle**: a
   `WrongId { expected, found }` pinpoints a field count/order/name mismatch at the exact byte.
2. **Retail NULLDEF entries = authoritative for field DEFAULTS.** A NULLDEF body is the
   default-constructed state serialized with no text applied.
3. **The decomp `Transfer<T>` functions** — authoritative for wire types and field order.
   `Transfer<long>` ⇒ `i32`, `Transfer<float>` ⇒ `f32`, `Transfer<CDefString>` ⇒ `DefString`.
   The order of `Transfer<…>` calls *is* the binary field order.
4. **Text source defs** — useful for what a field *means*, **not** for layout. A field the text
   writes as a string can legitimately carry `0` = null string, so "string-typed" and
   "sometimes 0" are compatible.

**NOT authoritative:** `defs-spec.json` (an early extraction that *produced* several
mistypings — it wrongly said `Font`=long, `PersistenceFlags`=defindex); the C++ copy-operator /
`TransferIn` functions (noisy — `TemplePrayerFactorHighest` is declared **twice** on purpose
because retail's `CScriptDef::Transfer` emits two consecutive controls with that crc; "fixing"
the duplicate broke positional parse of retail's script.bin).

### The bug-fixing loop (the "onion")

The ledger reports one representative field per entry, so fixing the first-diverging field
**reveals the next one underneath**. Expect to peel.

1. **Find** — run the ledger; read the `BUG diff paths` histogram and samples.
2. **Classify** — *value divergence* (real bug) vs *index-space noise* (a reference field
   showing different raw indices that resolve to the same name; only fixable by re-typing to
   `DefIndex`). `probe_field --vs $REF` compares by resolved meaning.
3. **Diagnose** — establish the correct type/default from the authority ranking.
4. **Fix** in the schema (`#[def(..., default = …)]`; change the field type; bespoke-lower
   seeding when a default needs interning — see `lower_ui`'s `ENG_ARIAL_16`).
5. **Verify** — re-run; the fixed field drops to 0; confirm Bug monotonically down.

### Byte-identical vs byte-changing fixes

- A **type fix that doesn't change encoding** (`DefIndex`↔`i32`↔`DefFlags` — all 4-byte LE) is
  byte-identical: golden stays green, no re-bless. Do these freely.
- A **default or structural fix changes bytes**: golden **fails by design**. Verify against
  `$REF`, **deploy + in-game test**, then re-bless with `OA_BLESS=1`. Do **not** re-bless a
  byte-changing fix without the in-game load test.

---

## 7. The manifest

`def-compiler/src/manifest.rs` (~10.6k generated lines) is the **only retail-derived input**.
It encodes exactly two things text can't provide: **membership** (which text defs retail
shipped per binary) and **NULLDEF lists**
(which classes get NULLDEF entries, order, and the duplication quirk — `CONTROL_SCHEME` appears
twice in frontend.bin). Deriving membership from text would change the contract from
*retail-equivalent* to *superset*, raising the in-game verification burden for no gain.
**Keep it** — small, static, honest.

**There is no regeneration tool.** The `dump_manifest` example older notes pointed at lived in
the pre-extraction OpenAlbion monorepo and did not come across; the reference survived in this
document and in `manifest.rs`'s own header for some time without anyone noticing it was dead.
Regenerating means writing it again: read the three retail binaries and, for each, emit the
`NameRef` def-name list (membership) and the leading NULLDEF run (classes, in order,
duplicates preserved — `CONTROL_SCHEME` twice). Only needed if the retail reference changes,
which it does not.

---

## 8. Settled decisions

Don't relitigate:

- The typed-struct IR and single-declaration layout principle (§3).
- Proc-macro derives as the declaration mechanism. Serde rejected (no in-place mutation);
  spec-codegen rejected (the Rust declarations have absorbed byte-verified corrections
  *against* the spec).
- The `DefReader` pull model — a legitimate answer to `&mut` aliasing, verified against every
  entry. Don't rewrite for elegance.
- One grammar for all file types (§4.1).
- Spans carry their file; no `Span::join` (§4.1).
- The manifest's existence (§7).
- The 16 KB chunk envelope. Output is ~1.17 MB larger than retail but content is
  byte-identical — the size gap is the TARGET, not a correctness gap.
- The canonical corpus compiles silently (§5).
- **No def cache** (§9 A.2). The build is 0.56 s; a cache buys ~1.4× and risks silent
  wrong output. `Body` reading chains in place (§9 A.1) is the shape lowering keeps.

Facet reflection was evaluated as the long-term home for `visit.rs` (its `Shape`/`Peek`/`Poke`
map onto the hand-rolled reflection) but deferred — it's 0.1.x/experimental.

---

## 9. Roadmap

### Track A — Build performance

This track began as "incremental compilation: a `redb`-backed cache so an edit-compile cycle
costs time proportional to what changed". **It ended somewhere else.** Measuring per phase
showed the premise was wrong (A.0), removing the redundancy that measurement exposed took the
build from 3.8 s to 0.56 s with no cache at all (A.1, A.3), and that left the cache buying ~1.4×
for a large architectural change and a silent-wrong-output risk — so it is **not being built**
(A.2). What remains is ordinary optimisation, led by parsing (A.3).

**The transferable lesson: measure per phase, and optimise before caching.** A cache would have
hidden every one of Phase 1's wins behind a warm path and left the cold build at 3.8 s.

#### A.0 Where the time actually goes

The old claim was that lowering is 88% of the build. It is 15%. The build has **three
independent algorithmic problems, none of which a cache fixes**:

| Problem | Cost |
|---|---|
| Chunk split is O(n²) over a 5,152-byte record | **1,804 ms** (measured; 32 ms fixed) |
| `lower_generic` is O(fields × statements) | **~180 ms** of the 272 ms named lowering |
| `flatten_specialization` clones 681k statements — and runs twice | **735 ms** |

Finer decomposition of the game.bin path, with text pre-read so parse timings exclude I/O:

| | ms |
|---|---|
| lex, all 238 files (12.8 MB) | 52 (248 MB/s) |
| AST construction on top of lex | 95 |
| evaluate header symbols | 18 |
| index space | 25 |
| flatten, one pass | 361 |
| lower 9,264 named bodies | 272 |
| lower 33,525 sub-def bodies | 184 |
| serialize 12.5k entries (6.8 MB) | 15 |
| chunk split | 1,804 |
| zlib, 434 chunks | 35 |

**Lowering's cost is not in the bespoke logic.** Grouped by def type, 94% of the 272 ms lands in
types that have bespoke arms — but every one of those arms calls `lower_generic` internally, so
that says the expensive types have big bodies, not that the bespoke code is slow:

| type | `#[def]` fields | stmts/def | µs/def |
|---|---|---|---|
| `CREATURE` | 89 | 132 | 275 |
| `OBJECT` | 44 | 31 | 20 |
| `UI` | 110 | 15 | 9 |
| `PLAYER_GUI` | — | 1,625 | 3,865 |

**Direct test of the cause** — pad a def's body with statements whose field name matches nothing
in the schema (pure no-op work) and time it:

```
OBJECT     28 stmts → 14.1 µs      CREATURE   41 stmts → 36.7 µs
          164 stmts → 50.9 µs                113 stmts → 55.4 µs
```

Linear, at **~0.27 µs per statement that matches nothing**. `DefReader::find_opt_leaf`
(`reader.rs:495`) scans every unconsumed entry on every field lookup and cannot early-exit
because the semantics are last-wins — so a def costs `fields × statements` string comparisons.
Over 681,479 flattened statements that is **roughly 180 ms of the 272 ms**, and a proportional
share of the sub-def 184 ms.

Sub-def lowering is additionally **5.6× redundant at the input**: 33,525 lowerings from ~5,700
distinct `(tag, statement-list)` inputs (they dedup to 3,726 distinct *outputs*).

#### A.1 Phase 1 — the algorithmic fixes (no cache, byte-identical, golden-gated)

**Status: complete. Build is 3.8 s → 0.65 s (~5.8×), output byte-identical throughout.**

Every one of these turned out to be *the same bug in five places*: work proportional to the
whole corpus being redone or re-copied per definition. Not one was a wrong algorithm in the
sense of a wrong output — the four binaries were byte-for-byte unchanged at every step.

| # | Fix | Result |
|---|---|---|
| 1 ✅ | **Chunk split.** `assemble_and_write` looped `remaining.drain(..split)`, memmoving the tail of a `Vec<EntryRecord>` (5,152 B each) once per chunk. Precompute sizes, find boundaries, consume in one forward pass. | **1,804 → 32 ms** (3.8 → 1.6 s) |
| 3 ✅ | **`flatten_specialization` ran twice per def.** The named pass resolves the chain once and collects the merged tagged blocks off it, **borrowed** from the ASTs. | 1.6 → 1.43 s |
| 2 ✅ | **`strip_superseded_by_clear` deep-cloned every body** — it returned `body.to_vec()` when there was nothing to strip, which is nearly always. | lowering **275 → 96 ms** (1.43 → 1.15 s) |
| 5 ✅ | **Sub-def lowering memoized on its input.** Borrowed blocks make `(tag, [(ptr, len)])` an exact key; inheriting the same block from a template is the whole redundancy. | 1.15 → 0.95 s |
| 4 ✅ | **Chains are read in place, never flattened.** `Body` (below) replaced the concatenation entirely. | **0.95 → 0.65 s** |

> **Fix 2 is not the fix that was planned, and the planned one was wrong.** The prediction was
> that `DefReader`'s by-name accessors — which scan the whole body per lookup, several times per
> field — were the ~180 ms. A name index was built and measured: it does remove the slope
> (0.134 → 0.036 µs per statement) but its `HashMap` build cost cancels the gain at this
> corpus's body sizes, showing **no difference at any threshold (16/64/128/off)**. It was
> reverted — `DefReader` is the semantic core and does not get complexity that does not pay.
> The `O(fields × statements)` scan is real; its constant is just small. The 0.27 µs/statement
> the padding probe measured was mostly the *clone*, not the scan. Revisit the index only if a
> corpus with much larger bodies appears.

##### `Body`: a def body need not be contiguous

The observation that made fix 4 mechanical rather than a rewrite: **all 33 functions in
`lower.rs` that took `body: &[Spanned<Statement>]` only ever call `body.iter()`.** Nothing
indexes or slices a body. So the contiguity that flattening provided was never needed —
`DefReader::new` stores `&Spanned<Statement>` and never the values.

```rust
#[derive(Clone, Copy)]
pub enum Body<'a> {
    Flat(&'a [Spanned<Statement>]),          // one run — a definition's own body
    Chain(&'a [&'a [Spanned<Statement>]]),   // runs read back to back
    Refs(&'a [&'a Spanned<Statement>]),      // an explicit selection
}
```

- **`Chain` *is* the flattening.** Reading a specialization chain's bodies most-distant-first,
  with the reader's last-wins overrides, reproduces copy-parent-then-apply exactly. There is
  deliberately **no function that concatenates a chain into a `Vec`** — `flatten_specialization`
  and `flatten_chain` were removed rather than left public, because a helper that materializes
  7× the corpus sitting next to the reason it was removed is a trap. Use
  `specialization_chain` + `chain_runs` + `Body::Chain`.
- **`Refs` is what filtering produces.** `filter_out_field(s)`, `partition_method_calls`,
  `partition_field_calls`, `strip_superseded_by_clear` and the five hand-written filter loops in
  §4.5's arms collect `Vec<&Spanned<Statement>>` — pointers, not statements.
  `strip_superseded_by_clear` keeps a nothing-to-strip early-out, so the common path allocates
  nothing at all.

Both callers ended up *simpler*: the named pass hands `Body::Chain` the chain it already
resolved, and `build_subdefs` hands it the merged block's slices directly instead of
concatenating them.

> Considered and rejected: instead of filtering, pre-mark the filtered-out statements as
> `consumed` in the `DefReader` — "filtered out" and "already consumed" really are the same
> thing, and it would allocate nothing at all. It requires `lower_generic` to take a built
> `DefReader` rather than a body, which inverts ~25 call sites and moves the filtering
> *semantics* of the bespoke arms into the reader. `Refs` got the same allocation win for a
> fraction of the disturbance. Revisit only if the pointer vectors show up in a profile.

Cross-cutting, still open: `size_of::<DefBody>() = 5,080 B` is why moving bodies into
`EntryRecord` costs 102 ms and why fix 1 was so dramatic. Boxing the large variants shrinks the
constant everywhere; Phase 2 removes most of the need by keeping the link phase on byte buffers.

##### Executing Phase 1

**Every step is byte-identical by construction, so golden is an exact gate, not an
approximation.** This is the whole reason to do Phase 1 before Phase 2: no output changes, so a
red golden means the step is wrong — never re-bless during Phase 1, and never batch two steps
into one commit. Run after each:

```bash
OA_TEXT_DIR=$TEXT/Defs cargo test -p defc --test golden
time ./target/release/defc -i $TEXT/Defs -o /tmp/out    # after cargo build --release -p defc
```

**`cargo test -p defc --test golden` is not enough on its own** — it hashes the four outputs
against a blessed baseline, so it catches a change but does not tell you *which* file moved.
Keep a known-good output directory and `cmp` all four; that is how each of these was verified.

Two things this pass taught that generalise:

- **Suspect copying before suspecting algorithms.** Three of the four wins were a `Vec` or
  statement-list being duplicated per definition, not a loop with bad complexity. The one that
  *was* a genuine complexity bug (the chunk split) was invisible in profile-by-inspection
  because the expensive operation is a `memmove` inside `Vec::drain`, attributed to nothing.
- **Verify the diagnosis before building the fix.** The planned fix 2 was designed off a
  micro-benchmark showing 0.27 µs per no-op statement, which was read as "the by-name scan".
  Most of it was the clone. Padding-probe slopes tell you cost *scales* with body size; they do
  not tell you *what* scales.

Still worth doing when someone touches `DefReader` for other reasons: a name index makes
"which statements did nobody claim" cheap, which is what Track B's unconsumed-statement warnings
need — though the build-scoped consumption ledger that work actually requires is separate.

#### A.2 Decision: **no def cache.** The prize no longer justifies it

Phase 1 changed the arithmetic that motivated Track A. Against the 0.65 s build it left:

| | ms |
|---|---|
| A cache could skip: parse 152 · symbol eval 18 · lower named 82 · lower sub-defs 25 · chains + block collection 18 | **~295** |
| It cannot skip: file I/O 17 · index space 5 · `EntryRecord` moves 102 · serialize 15 · chunk 32 · zlib 35 · frontend + script 60 · cache read ~10 · **plus a new link pass** | **~275** |

So a warm rebuild lands around **0.37 s against 0.65 s — under 2×**. A.3 step 1 has since taken
parsing off the skippable side almost entirely (152 ms → ~17 ms), so the real figure today is
**~0.40 s against 0.56 s, about 1.4×**. The case has only got weaker.

**That is not worth what it costs.** The bill is a relocation table threaded through the
lowering core, an on-disk format with versioning and a schema fingerprint, a shift test, an API
change to `def-compiler-sys`, and a class of bug this project should least tolerate:
**under-invalidation is a silent wrong-output bug, and §6's own gate cannot see it** (a
from-scratch build stays internally consistent, so corruption only appears on a later cache
hit). Trading that risk for ~250 ms is a bad trade for a compiler whose correctness story is its
main asset.

**Do not build the cache.** The design below is kept because it is sound and because the
compile/link split has *standalone* value — see A.4 — not because the cache is queued.

#### A.3 Parsing — the plan

Parsing *was* the largest remaining item — 152 ms of a 650 ms build, plus 18 ms evaluating
symbols. Step 1 has landed; the measurements below are the ones that shaped the rest.
Measured with a counting allocator over the whole 238-file, 12.8 MB corpus:

| | time | allocations | allocated | retained |
|---|---|---|---|---|
| read all 238 files | 17 ms | 468 | 28 MB | 12.8 MB (kept for diagnostics) |
| lex → 1,120,975 tokens | 63 ms | 2,015 | 149 MB | — (transient per file) |
| build the AST | 89 ms | ~1,272,000 | ~147 MB | **81.4 MB** |

Two facts decide the ordering, and both cut against the obvious instinct:

- **Lexing is compute-bound, not write-bound.** 12.8 MB in 63 ms is 203 MB/s — slow for a lexer.
  The 54 MB token stream is only ~5 ms of writes at memory bandwidth, so **shrinking `Token` is
  mostly a memory win, not a time win.** (The ~95 MB of doubling churn *above* that 54 MB is
  real time — roughly 10 ms of `memcpy` — and is fixed by reserving capacity, not by shrinking.)
- **AST construction is allocation-bound.** ~1.27 M allocations for 1.12 M tokens — about one
  per token. ~430 k are identifier/number `String`s (the ≤8/≤16/≤32 B buckets); the rest are the
  `Vec`s behind property paths, argument lists and bodies.

##### Step 1 ✅ — read and parse files in parallel

**Done. Build 0.65 s → 0.56 s**, output byte-identical and verified deterministic over six
consecutive runs.

`parse_source(&text, FileId) -> Result<SourceAst, _>` was already pure — a `&str` and a file id
in, a tree out, nothing shared. Only the scheduling around it was sequential. Both the header set
and the `.def`/`.tpl` corpus are now read and parsed across threads; **evaluation stays
sequential and in file order**, which is required because `SymbolTable` is last-definition-wins
(§4.1). That is the one real constraint in the whole step.

**`parallel_map` hands work out largest-first**, and that ordering is the entire scheduling
strategy: file sizes span 146× (median 10 KB, max 1.5 MB), so the makespan is set by when the
biggest file *starts*. Measured over the corpus on 12 cores against 91 ms sequential:

| | |
|---|---|
| static chunks by file count | 25.9 ms |
| atomic queue, input order | 23.1 ms |
| `rayon`, global pool | 17.9 ms |
| **atomic queue, largest first** | **16.5 ms** |

> **`rayon` was measured and rejected.** It loses to a 25-line largest-first queue here, because
> work-stealing's adaptivity buys nothing when there are 238 coarse independent items whose costs
> are known exactly (file lengths) before starting. It also costs six transitive crates in a
> deliberately lean tree.
>
> The deciding factor is not the 1.4 ms. **`rayon`'s global pool spawns threads that live for the
> whole process**, and `defc_build` is a C ABI entry point in a cdylib EgoCore loads — threads
> outliving the call complicate unload, take cores the host may want, and cannot be configured by
> it. `std::thread::scope` joins every thread before returning. Reach for `rayon` only if
> something here ever becomes recursive or has unpredictable per-item cost.

The property parallelism could break silently is **source-id assignment**: a file's id must
depend only on its position in the sorted walk, never on which thread finished first. Golden
cannot catch that — a corpus that compiles produces no diagnostics at all — so
`parse_error_points_at_the_file_it_came_from` in `build.rs::tests` pins it directly.

**Now re-measure before starting step 2.** Parsing is no longer the bottleneck, and steps 2–5
each shave single-digit milliseconds off a number that no longer dominates. They may not be
worth doing at all on time grounds; steps 2–4 remain justified on *memory* and *simplicity*
grounds, which is how they should be argued from here.

##### Step 2 — `Span` as a global byte offset (24 bytes → 8)

The redundancy is real: `lex(input, file)` stamps one `FileId` into all 1,120,975 token spans and
the parser copies it into every AST node. That is ~4.5 MB of duplicated file id in the token
stream and ~1.3 MB in the retained AST.

But the file id cannot simply be dropped to a per-tree field. §4.1's rule is load-bearing: **a
span outlives the context it was read in.** Lowering reads a template's statements while
compiling a definition in a *different* file (that is what `Body::Chain` does), so a bare
`{start, end}` would be ambiguous exactly when a diagnostic needs it most.

**The fix that gets both: one global byte-offset space across the whole corpus** (the model
rustc uses for `BytePos`). Each source file is assigned a disjoint range; a span is
`{ lo: u32, hi: u32 }`; the file is recovered by binary-searching a table of file bases. The span
stays self-identifying, and the id stops being stored 1.1 M times.

- The corpus is 12.8 MB against `u32`'s 4 GB — ~300× headroom.
- File lookup happens **only when rendering a diagnostic**, so the binary search is paid exactly
  where cost does not matter.
- `Span::contains` becomes plain range containment and the file check disappears — the comment
  explaining why the check exists goes with it.

Measured layouts (mock types, same alignment rules):

| | `Span` | `Spanned<Statement>` | `Spanned<Expr>` | `Token` |
|---|---|---|---|---|
| today | 24 | 120 | 72 | 48 |
| keep `FileId`, `u32` offsets | 12 | 112 | 64 | 32 |
| **global `u32` offsets** | **8** | **104** | **56** | **32** |

Note what alignment does here: **for `Token` the two options are identical** (both land on 32,
because the `&str` forces 8-byte alignment), so the extra 4 bytes only pay off in the AST and
in step 3. Across the ~323,747 `Spanned` nodes the corpus retains, keeping `FileId` saves
2.59 MB and the global space saves 5.18 MB — of an 81.4 MB AST. **Call it a 6% AST reduction and
a 33% token-stream reduction; do not expect a large time win on its own.** Its better
justification is that it is a simplification, and that every later pass walks smaller nodes —
lowering reads 681,479 statements through `Body::Chain`, so node size is cache behaviour.

Change surface, which is small and mostly mechanical:

| Where | What |
|---|---|
| `text/base.rs` | `Span { lo: u32, hi: u32 }`, `SYNTHETIC` becomes a reserved value (`lo == hi == u32::MAX`), `contains` loses the file check |
| `text/lexer.rs` | `Lexer`/`lex` take a `base: u32` instead of `FileId`; ~6 `Span::new` sites |
| `text/mod.rs` | `parse_source(input, base)`; ~12 `Span::new(cursor.file(), …)` sites |
| `build.rs` | assign bases from file lengths before parsing (composes with step 1, which reads all files first); 2 sites turning `span.file.0` into `DiagnosticLabel.source` become a `SourceMap` lookup; 1 `Span::new(dspan.file, …)`; 1 `contains` |
| `defc/main.rs`, `def-compiler-sys` | both render through `codespan-reporting` with a **per-file** range, so subtract the base: `span.lo - base(label.source)` |

`SourceFile` gains a `base: u32`, and `BuildReport`/`BuildError` expose the table. `FileId`
survives only as the index into that table — which is what `DiagnosticLabel.source` already is,
so the two stop being redundant representations of the same thing.

Anonymous parses (`parse_expr_str`, unit tests, `FileId::ANONYMOUS`) get a reserved high base
whose lookup returns "no file", which renders location-less — the shape `BuildDiagnostic::bare`
already has.

##### Step 3 — shrink `Token` to 12 bytes (depends on step 2)

`Token { kind: TokenKind, span: Span, source: &'a str }` is 48 bytes, of which `source` is 16 and
is **redundant**: it is exactly `&input[span.lo - base .. span.hi - base]`. Drop it and have
`Cursor` hold the input, and with step 2's 8-byte span a token is `1 + 8` → **12 bytes**, a
quarter of today's. The token stream goes 54 MB → 13.5 MB.

Pair it with **reserving the token `Vec`** from an estimate of `input.len()`, which removes the
~95 MB of doubling churn — that one is worth ~10 ms and is a two-line change that can land
independently of everything else.

##### Step 4 — box `PathSegment::Index` (independent of 1–3)

`PathSegment` is **72 bytes** because one of its two variants is a `Spanned<Expr>`; the common
variant is `Field(String)` at 24. Every property path is a `Vec<PathSegment>` and most are one
or two segments, so the corpus pays ~48 bytes of padding per segment. Boxing the `Index` variant
takes `PathSegment` to 24 — **a larger AST saving than step 2**, in a much smaller change.
Indexed segments are rare, so the extra indirection is not on the hot path.

##### Step 5 — intern identifiers (conditional, last)

517,988 identifier/number tokens across the corpus are only **108,349 distinct (4.8×
repetition)**, 7.2 MB of text against 2.99 MB of distinct text. Interning removes ~430 k of the
~1.27 M AST allocations.

Two shapes were considered:

- **`Symbol(u32)` into a table.** 4 bytes per name, and every name comparison becomes an integer
  compare — which would speed up *lowering* too, since `DefReader` matches path segments against
  `&'static str` field names on every lookup. Deepest change: AST field types, plus interning the
  schema's names once at startup.
- **Borrowing `&'a str` from the source text.** Zero allocations and zero interning, but it puts
  a lifetime on `SourceAst`/`Definition`/`Statement`/`Expr`, which then infects `defs_by_name`,
  `Body`, and every signature in `lower.rs`.

**Neither is worth starting before step 1 is measured.** `&'static str` by leaking an interner
arena is *not* an option here: `def-compiler-sys` is a cdylib that EgoCore calls repeatedly in a
long-lived process, and a leak that grows per distinct corpus is a real defect there.

##### Ordering, and what gates each step

1. ~~Parallel parsing~~ ✅ — done, 0.65 s → 0.56 s.
2. **Reserve the token `Vec`** — two lines, ~10 ms, independent of everything.
3. **`Span` → global offsets** — mechanical but wide; the compiler finds every site. Justified on
   memory and simplification, not on time.
4. **Drop `Token.source`** — only after 3.
5. **Box `PathSegment::Index`** — independent; best AST-memory-per-line-changed of the lot.
6. **Interning** — only if a profile after 1–5 still shows AST construction on top.

Every step here is byte-identical by construction, so **golden is an exact gate throughout**, as
it was for Phase 1 (§A.1). Keep a known-good output directory and `cmp` all four binaries — the
golden hash tells you *that* something moved, not *which* file.

#### A.4 The compile/link split, if it is ever built

**Its real value is parallelism and memory, not caching.** The shared
`&RefCell<NamesBuilder>` is the only reason lowering must be sequential; per-def relocation
tables remove it. Together with a link phase that works on byte buffers, that would take
~107 ms of lowering plus the 102 ms of `EntryRecord` moves and make both parallel or unnecessary
— **~180 ms, no cache involved.**

It is also *far* safer to build for that purpose than for a cache: with lowering and linking in
the same build, a missed relocation site changes the output immediately and **golden catches it
exactly**. The shift test below exists only because a cache breaks that property.

A lowered body is not position-independent. It carries two kinds of corpus-global state:

1. **`DefIndex` = a global entry index**, assigned by position in `collect_named`. Adding or
   removing any earlier def shifts every later index.
2. **`DefString` = a byte offset into the shared `names.bin`**, which depends on interning order
   across the entire build.

So a cached body must keep references **symbolic** and resolve them at link time.

> **You cannot recover the symbolic form after the fact.** The tempting shortcut — lower as
> today, then reverse-map index→name and offset→string — is unsound. `resolve_ref` /
> `resolve_ref_i32` (`lower.rs:413`/`441`) fall back to the `Evaluator`, so a `DefIndex` slot
> legitimately holds a plain evaluated number: a header symbol's value, or one of the hashed
> text ids in the mis-typed reference fields (§10). A literal `1881` is indistinguishable from
> "entry 1881", and relocating it corrupts the def. Same on the string side — `lower_ui` stores
> `DefString(eval.eval_i32(expr)?)` for the polymorphic `Font`. **References must be recorded
> where they are resolved.**

##### The design: table-id slots

Every `DefIndex`/`DefString` slot in a lowered body holds **an index into that def's own
relocation table**, never a final value:

```rust
enum Reloc { Def(String), Str(String), Lit(i32) }

struct Lowered {
    body:  Vec<u8>,          // serialized; every reference slot holds a table id
    slots: Vec<(u32, u32)>,  // (byte offset in `body`, table id)
    table: Vec<Reloc>,       // ids assigned in first-encounter order
}
```

Three properties make this work where the obvious "list of `(field crc, symbolic value)` pairs
consumed in order" does not:

- **The id travels in the slot**, so there is no ordering invariant between the relocation list
  and container elements. Last-wins overwrites, `Field.clear()`, and the ~35 bespoke arms that
  build vectors and maps by hand are all automatically correct. An ordering invariant across
  those arms is the thing most likely to break silently; this removes it entirely.
- **Literals get table entries too** (`Lit`), so a slot value is *always* a table id — no tag
  bit, no reserved sentinel range, no chance of a literal colliding with an id.
- **A missed recording site is detectable.** Valid ids are dense `0..table.len()` (tens of
  entries), so a raw resolved index left in a slot is out of range and trips an assertion.

Mechanics — this is a change to *what the two existing arguments are*, not a new parameter
threaded through 35 arms:

- `names: &RefCell<NamesBuilder>` → a per-def interner with the same `intern(&str) -> u32`
  shape, returning a table id. The **11 `intern()` call sites in `lower.rs` are untouched.**
- `def_indices: &HashMap<String, u32>` → a resolver that still consults the real name set (so
  `UnresolvedReference` diagnostics are unchanged) but returns a table id. **5 lookup sites**,
  all inside `resolve_ref`, `resolve_ref_i32`, and `apply_ui_state`.
- Cloning the NULLDEF base seeds the child's table with a copy of the base's table, so ids in
  the cloned body stay valid. NULLDEF bodies *do* carry relocations — `lower_ui` seeds
  `ENG_ARIAL_16`.
- Byte offsets come from a generated `visit_reloc_offsets` in `defs-derive` — the same
  traversal `wire_size` already performs, emitting the running offset at each `DefIndex` /
  `DefString`. Generated from the same field list, so it cannot drift from `serialize`. Cost is
  `byte_size`-shaped: 0.8 ms for 12,486 entries.

Link, per entry in global index order:

```
intern def_type and file name                (unchanged)
resolve table entries in id order            ← replays today's intern order exactly
out = body.clone();  for (off, id) in slots: out[off..off+4] = resolved[id]
```

**Why this stays byte-identical.** Table ids are assigned in first-encounter order, which *is*
the order lowering calls `intern()` today; link replays them in id order, per def, in global
index order — the same sequence of `NamesBuilder::intern` calls the current build makes. Golden
is the check, and it is exact.

Three consequences worth having on their own:

- **Every relocation is 4 bytes, so a cached body's `byte_size` is constant.** Chunk splitting
  needs no `DefBody` at all; the whole link phase runs on byte buffers, and the 5 KB `DefBody`
  never has to be moved into an `EntryRecord`.
- **There is one lowering path.** From-scratch builds go lower → symbolic → link exactly like
  cache hits, so golden covers the link phase and there is no second path to drift. Do *not*
  add a "cache miss ⇒ lower directly and emit" shortcut.
- **Lowering becomes parallelisable.** The shared `&RefCell<NamesBuilder>` is the only thing
  making it sequential today; per-def relocation tables remove it. Measured feasibility (each
  thread lowering a disjoint slice with its own builder): 285 ms → 154 ms, only **1.85× on 12
  cores** — the static chunking puts the whole `CREATURE` cluster on one thread. Work-stealing
  would do better, but note that after Phase 1 fix 2 lowering is small enough that this is a
  minor lever. Do not reach for threads before fixing the O(F×S) scan.

**Sub-def dedup must stay on *linked* bytes.** Today's key is `(tag, resolved bytes)`, so a
`Lit(5)` sub-def and a `Def(X)`-resolving-to-5 sub-def dedup together. Deduping on symbolic
form would split them, add an entry, and shift every later index. Order stays: link sub-defs →
dedup → assign indices.

##### The gate: the shift test

**A missed relocation site is invisible to golden.** A from-scratch build is internally
consistent, so a slot that kept a raw resolved value still serializes correct bytes; it only
corrupts on a cache hit after the index space moves. So Phase 2's acceptance test is:

> Build corpus `C`. Build corpus `C′` — `C` plus one definition inserted at the front of the
> first file (shifting every index) and one new string. Link `C′` from `C`'s cached bodies and
> assert the result is byte-identical to a from-scratch build of `C′`.

This test is what makes the approach trustworthy, and **it must exist before any cache does.**

#### A.5 The cache design — **not being built** (A.2), kept so it need not be re-derived

Cache key, Merkle-chained rather than cascade-propagated:

```
key(def) = H(schema_fingerprint, symbols_hash, def_type, is_template,
             ast_hash(own body), key(parent))
```

Folding the parent's key in makes **cascade invalidation automatic** — editing `OBJECT_BASE`
changes the key of all 3,147 descendants with no reverse-edge graph and no way to
under-invalidate. Memoize over the chain (max depth 8, median 1) in topological order.

- **`ast_hash` must exclude spans.** Spans move whenever anything above them in the file moves.
  `Spanned<T>: PartialEq` already ignores them; `Hash` must match. Test: insert a comment,
  assert the hash is unchanged.
- **`schema_fingerprint` = hash of all 249 NULLDEF bodies plus their relocation tables**,
  computed at runtime (`build_nulldefs`, 2.9 ms). Any field-order, type, or default change moves
  it automatically. Prefer this to a hand-bumped `schema_version`, which only works if nobody
  ever forgets.
- **`symbols_hash` is global**: any header edit invalidates everything. Correct and cheap to
  start with; per-def symbol-read sets are the refinement, not the starting point.
- Value: the def's `Lowered`, plus its sub-def blocks in the same form. Because the value is
  fully symbolic, **one cache entry serves all three binaries** — a def's lowering no longer
  depends on which index space it lands in.
- **Duplicate definition names** (§4.6, ~800 of them): first occurrence wins the index, last
  wins the body. The key must hash the *resolved* definition (`defs_by_name`, last-wins), not
  the file-local one.

The invalidation graph is favourable: 9,775 of 10,454 definitions (93.5%) are leaves, so the
typical edit invalidates exactly one def. 23 defs have ≥100 transitive descendants
(`OBJECT_BASE` 3,147 · `OBJECT_VILLAGE_SOLID_FURNITURE_BASE_TEMPLATE` 1,716 ·
`CREATURE_BASE_TEMPLATE` 523); a root-template edit invalidating 30% of the corpus is a
correctness requirement, not a performance problem. **Under-invalidation is a silent
wrong-output bug; over-invalidation only costs time.**

##### Store: redb, and why the overhead does not matter

The access pattern is unusual for a database: **the hot path reads every entry**, because link
has to touch every entry whether or not it changed. That is a file-load workload, not a
point-lookup workload, so it was worth checking that a B-tree store does not tax it. Benchmarked
on 14,700 entries / 9.5 MB payload with a size distribution modelled on the measured bodies:

| | redb 4.1 | flat single-file image |
|---|---|---|
| open | 0.8 ms | — |
| **read every entry** | **7–8 ms** | 1.3 ms read + 1.0 ms index |
| cold populate + commit | 89 ms (63 ms at `Durability::None`) | 17 ms |
| write back 1 changed entry | 6 ms | 17 ms (full rewrite) |
| write back 3,147 (an `OBJECT_BASE` edit) | 18 ms | 17 ms (full rewrite) |
| on-disk size | 16.85 MB | 9.62 MB |

**redb costs ~7 ms more per hot build and saves the full-image rewrite on every write.** Against
a hot build in the hundreds of milliseconds that is noise, so pick on properties, not speed:
redb gives crash-safe incremental writes and a single file; the flat image is smaller and has no
dependency but rewrites everything on any change. **Go with redb.** Use `Durability::None` for
the populate path — it is a cache, and the correct response to a torn write is a cold rebuild.

The API shape is settled: **`defc_build` takes a cache-file path.** Absent or unopenable ⇒ cold
build, populate it. Present and valid ⇒ hot build. This keeps `defc_build` one-shot, needs no
compiler handle in `def-compiler-sys`, and works identically for the `defc` CLI. A stale,
corrupt, or foreign cache must only ever cost time, never correctness — the Merkle key plus the
schema fingerprint make a mismatch a miss, and nothing else.

##### What a warm rebuild would still cost

See A.2 for the arithmetic that closed this out: ~0.37 s warm against a 0.65 s cold build,
under 2×, shrinking toward 1.5× as A.3's parsing work lands. If the cache is ever revisited,
**re-derive those two columns first** — they are what the decision turns on, and Phase 1 moved
them by 6×.

Note also that a *parse* cache is a different and much smaller proposition than a def cache: one
key per file, no relocation table, no index space to reconcile, and a miss costs a re-parse
rather than a wrong def. If any caching is ever wanted, that is the one to build.

#### A.6 Settled for this track

- **No def cache** (A.2). Revisit only with fresh numbers, never on the old framing.
- **Optimise before caching.** Every Phase 1 win was removable redundancy, and finding it
  required per-phase measurement. A cache would have hidden all of it behind a warm path and
  left the cold build at 3.8 s.
- If the compile/link split is ever built (A.4): **relocation table, not `LowerEnv`** — the seam
  that matters is what the two existing arguments *return*, not their type; **slot-carried table
  ids, not `(field crc, ordinal)` pairs**, so there is no ordering invariant across §4.5's arms;
  **literals get table entries**, so no tag bit and no sentinel range; **dedup sub-defs after
  linking**, on resolved bytes, as today.
- If any cache is ever built: **a miss must only ever cost time** — no cache state may change
  output bytes — and **`redb` with a cache-file path argument**, measured at ~7 ms/build over a
  flat image in exchange for delta writes.

#### A.7 Not on the critical path

Measured and set aside, so nobody re-derives them:

- **Parallel lowering** — 1.85× on 12 cores with static chunking (against parsing's 5.5×), and
  it needs the per-def `NamesBuilder` from A.4 to be correct at all. The `CREATURE` cluster
  lands on one thread; work-stealing would do better.
- **Boxing `DefBody`'s large variants** — 5,080 B per value, and **102 ms of the build is just
  moving bodies into `EntryRecord`**. This is now the largest item after parsing and lowering,
  and unlike A.4 it is a local change. Worth doing on its own.
- **`size_of::<Span>() = 24`** — now A.3 step 2, where it belongs: it is not only a parsing
  cost, since `Span` sits on every AST node the whole build holds.
- **The retained AST is 81.4 MB** for a 12.8 MB corpus — 6.4× the source text. A.3 steps 2 and 4
  take ~12 MB of that; the rest is `Statement` at 96 bytes and `Expr` at 48. Nothing has needed
  it yet, but it is the number to check if memory ever becomes the complaint.
- **`LineIndex` (`text/base.rs`) is dead code.** Both renderers resolve line/column through
  `codespan-reporting` instead. Delete it, or use it — but it should not sit there implying
  span→line resolution lives in `defs`.

### Track B — Diagnostics, remaining

- **Verbose mode** presenting `SourceAst.ignored`, cross-file duplicate definition names
  (786 in the corpus vs 18 within a single file — the latter are near-certainly bugs), and
  other corpus oddities as **notes**, so the default build stays silent (§5).
- **Unconsumed-statement warnings.** Removed, not deferred. `remaining_statements()` answered
  "did *this reader* consume it?", but consumption is spread across several readers and
  pipeline stages: tagged blocks are consumed by the sub-def pass, `Components.Add` by
  `build_thing_components`, and the bespoke UI/CONTROL_SCHEME arms run a second full-body
  reader over statements the generic pass already took. Measured against the stock corpus it
  produced **103,467 "unconsumed" statements, of which 10 were genuine**. Reviving it needs a
  **build-scoped consumption ledger keyed on statement identity** and checked once at the end —
  not a `Vec` returned per reader. `Span` now carrying a `FileId` makes that identity possible.
- **Bespoke-arm error propagation** — ~39 `.ok()`/`unwrap_or` sites (§4.5). Highest-risk first:
  `build_animation_anims` (a typo'd animation key silently gives bank 0), then the `OPINION_*`
  ctor closures. Warn before promoting to error, and gate each step on golden *and* on the
  corpus staying silent.
- **"Did you mean?"** on `unknown constant` / `unknown definition` / unknown field / unknown
  parent. Bounded edit distance over the candidate set, which each site already has in hand.

### Track C — Quality

- **Unit tests for `lower.rs`** — 3,400 lines, **zero** tests, the largest untested file in the
  workspace (`binary.rs` at 1,300 is second). The bespoke arms (ctor arg remapping, DSL replay,
  sort/dedup fixups) rely entirely on the golden end-to-end gate. Prerequisite for Track B's
  error-propagation work.
- **Bundle lowering's three context arguments.** `symbols: &SymbolTable`,
  `def_indices: &HashMap<String, u32>` and `names: &RefCell<NamesBuilder>` are threaded together
  through **29 signatures and 52 call sites** and always travel as a unit. A plain
  `LowerCtx<'a>` struct of three references would remove ~80 sites of repetition at zero runtime
  cost.

  > This is **not** the rejected `LowerEnv` (§9 A.6). That was a *trait* abstracting how
  > references resolve; this only groups three parameters. Worth doing on its own merits, and it
  > happens to be exactly the seam §9 A.4 would change if the compile/link split is ever built —
  > which is a reason to keep it a plain struct, not to make it clever.
- **A `NameRef` arm for the semantic ledger** — the blind spot that hid the 260-entry
  ClassIndex bug (§4.7).
- **Enum typing** — a few fields remain `i32`/`u32` where a `DefEnum`/`DefFlags` exists. All
  byte-identical. `PersistenceFlags` for the Thing family needs decomp extraction first.
- **Schema-ize bespoke lowering** — `#[def(container = …)]`, declarative ctor-arg maps, to move
  §4.5's B and D categories out of code.
- **Container-layer error unification** — adopt the wire layer's composable context wrapping
  (§4.7).

**Deliberate non-fixes**, so they are not "cleaned up" later:

- **The two renderers duplicate ~40 lines.** `defc/main.rs` and `def-compiler-sys` each convert
  `BuildDiagnostic` into a `codespan-reporting` `Diagnostic` and build a `SimpleFiles` store.
  Sharing that would put `codespan-reporting` in `def-compiler`, which is exactly the coupling §5
  avoids: the library collects diagnostics and never renders, so consumers stay free to render
  however they like. Two small copies is the cheaper side of that trade.
- **`&mut &mut [u8]` in the wire layer** (31 sites, flagged by `clippy::pedantic`) is the
  serialize cursor, not an oversight.
- **`clippy::pedantic` is not adopted wholesale.** Run it to *find* things — it surfaced the
  dead code and the triplicated statement-path match — but most of its output here is
  `# Errors` doc sections, `#[must_use]`, and "more than 3 bools in a struct", which is the wire
  format describing itself. Cherry-pick; do not add a lint block to satisfy it.

---

## 10. Operational reference

### Commands

```bash
REF=~/Fable/data/CompiledDefs/backup-retail-verified
TEXT=~/doc/Fable_Anniversary-2013-02-25/Fable/Data

# From-scratch build (all four binaries)
cargo run -q -p defc -- $TEXT/Defs <out_dir>

# Golden byte-determinism gate (THE regression gate)
OA_TEXT_DIR=$TEXT/Defs cargo test -p defc --test golden
# ...re-bless after an INTENDED output change (see §6 first):
OA_BLESS=1 OA_TEXT_DIR=$TEXT/Defs cargo test -p defc --test golden

# Semantic ledger vs retail (build first, then verify the output)
cargo run -q -p def-compiler --example verify -- <out_dir> $REF
cargo run -q -p def-compiler --example verify -- <out_dir> $REF --dump-subdef <Parent> [<Tag>]

# Probe/compare a single field vs retail (the divergence workhorse)
cargo run -q -p def-compiler --example probe_field -- <dir> <game|frontend|script> <Field> --vs $REF
cargo run -q -p def-compiler --example probe_field -- <dir> game --entry NULLDEF_UI --vs $REF

# NameRef / ClassIndex checks (the ledger's blind spot)
cargo run -q -p def-compiler --example probe_counter -- <dir>
cargo run -q -p def-compiler --example probe_counter_diff -- <dir> $REF

# NOTE: there is no manifest-regeneration tool in this repo. The `dump_manifest`
# example older notes referred to lived in the pre-extraction OpenAlbion monorepo
# and did not come across. See §7.

# Total build time
cargo build -q --release -p defc
time ./target/release/defc -i $TEXT/Defs -o /tmp/out

# Per-phase timing (§1). There is no phase instrumentation in the library, and the
# progress lines are too coarse to attribute cost — timing the gap between
# "compiling" and "game:" lumps flatten, lower, serialize, chunk and zlib together,
# which is exactly how "lowering is 88% of the build" got into this document when
# lowering is 15%. Measure by replaying the pipeline through the public API in a
# throwaway `packages/def-compiler/tests/` harness instead: `lower_def`,
# `specialization_chain`, `chain_runs`, `Body`, `manifest::*`, `parse_source`,
# `SymbolTable`, `NamesBuilder`, `Chunk`/`DefBinary` are all public, so the game.bin
# path can be rebuilt phase by phase with an `Instant` between each. Delete it
# afterwards.

# In-game test — deploy all four; ROLLBACK from $REF if it won't load
cp <out_dir>/{game,frontend,script,names}.bin ~/Fable/data/CompiledDefs/
cp $REF/{game,frontend,script,names}.bin ~/Fable/data/CompiledDefs/   # rollback
```

### Key files

| File | Role |
|---|---|
| `packages/defs/src/text/mod.rs` | **The grammar**: `Item`, `SourceAst`, AST types, all productions |
| `packages/defs/src/text/lexer.rs` | `lex`/`Cursor`/`TokenKind`/`TextParseErrorKind` |
| `packages/defs/src/text/base.rs` | `FileId`, `Span`, `Spanned`, `ParseContext`, `ParseError` |
| `packages/defs/src/text/symbols.rs` | `SymbolTable`, `SymbolEvalError`, `Redefinition` |
| `packages/defs/src/binary.rs` | Container + generated `DefBody`/`sub_def_names!` + `SemVal` |
| `packages/defs/src/wire.rs` | Wire model + runtime helpers |
| `packages/defs/src/enums.rs` | `DefEnum`/`DefFlags` + all enum tables |
| `packages/defs/src/visit.rs` | `FieldRef` + slot traits (reflection) |
| `packages/defs/src/def/*.rs` | ~273 def struct declarations, one module per type |
| `packages/defs-derive/src/lib.rs` | The five derives + `#[def]`/`#[flags]` attr parsing |
| `packages/def-compiler/src/reader.rs` | `Evaluator`/`Args`/`DefReader` |
| `packages/def-compiler/src/lower.rs` | `specialization_chain`; `Body`; `Applier`; ~35 bespoke arms; `lower_def`/`lower_generic` |
| `packages/def-compiler/src/build.rs` | Corpus parsing, assembly, **all diagnostic construction** |
| `packages/def-compiler/src/manifest.rs` | Generated retail membership + NULLDEF lists |
| `packages/defc/src/main.rs` | CLI + diagnostic rendering |
| `packages/defc/tests/golden.rs` | **Byte-determinism gate** |
| `packages/def-compiler/examples/verify.rs` | Semantic ledger — the divergence oracle |

Test counts: `defs` 129, `def-compiler` 41, `defc` 1 (golden), plus doctests.

### Load-bearing gotchas

- **CRC32** = standard zlib (poly `0xEDB88320`, init 0, no final xor) = `CCharString::GetCRC`.
- **`EntryPreamble.unknown_0 = 1`** for ALL non-NULLDEF entries (`AreDefaultValsApplied`;
  `0` ⇒ save-load crash). NULLDEF preambles are `(false, false, 0)`.
- **`NameRef.ClassIndex` is 0-based and continuous across all three emission passes** (§4.7).
- **Tagged blocks MERGE** across specialization (parent-first concat), never replace.
- **Anonymous entry `file_name_offset` = `0xFFFFFFFF`**; dedup keys on **(class-tag, bytes)**
  (byte-only type-confuses combat sub-defs → crash).
- **`names.bin` is ONE shared table** across all three; header off8/off12 content-derived.
- **External def refs are by NAME; in-body refs are global indices.**
- **`DefString::def_default()` = −1; `DefIndex::def_default()` = 0.**
- **Text-tag fields mis-typed as `DefIndex`** (`CShopDef.Name`, `CTavernGameDef.Banter`…) hold
  hashed text ids — "out of range" but load-safe.
- **The chunk-index sentinel entry is required**; chunks compress at zlib level 1.
- **CONTROL_SCHEME appears twice** in frontend.bin NULLDEFs — iterate `NULLDEF_ENTRIES`
  (non-deduped) for output, build bodies from `NULLDEF_CLASSES` (deduped).
- **`_deprecated.` files are skipped** by the corpus walk.

### Decomp reference map

| What | Where |
|---|---|
| CRC32 | `bbblibrary/lib_crc.cpp` |
| Def / sub-def compile | `fablelib/defs/definition_manager.cpp`, `bbblibrary/lib_definition_manager.cpp` (`CompileSubDefinition`) |
| Def lookup | `lib_definition_manager.hpp` (`GetPDefFrom{GlobalIndex,InstantiationName}`) |
| `ClassIndex` write / read | `lib_definition_manager.cpp:1581` / `GetDefClassIndexFromGlobalIndex` |
| names.bin format | `bbblibrary/lib_definition_string.cpp` (`CDefStringTable`) |
| Thing def ref = name | `fablelib/thing.cpp`; anon entry sentinel `thing_base_def.cpp` |
| Animation / opinion / sound / controls | `animation_set.cpp`; `tc_opinion_of_hero.cpp`, `opinion_reaction_manager.cpp`; `lib_sound_map.cpp`; `fablelib/defs/controls_def.cpp` |
| Thing components | `CThingComponentSet::Add` (driver types via `GetPTCInfo`) |
| Script def inheritance | `fablescripting/defs/{script_def.hpp:258,cutscene_def.hpp,regionscriptdef.hpp}` |
| **Wire types & field order (AUTHORITATIVE)** | `<Class>::Transfer` in `~/git/fable-decomp/out/source/**/*.cpp` |
| Layout spec (NOT authoritative) | `fable-decomp/defs-spec.json` — superseded wherever it disagrees |
