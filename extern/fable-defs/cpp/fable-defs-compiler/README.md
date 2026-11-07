# fable-defs-compiler

A C++ port of the Rust def compiler: text layer, model lowering, wire encoding
and the binary build pipeline.

The C++ data model lives in [`../fable-defs`](../fable-defs). This project
contains the compiler and CLI.

## Status

The **text front end is ported and validates against the real corpus**:

```
files:        244 (177 def/tpl, 67 h)
definitions:  10522
enums:        184
defines:      70
namespaces:   6
conditionals: 46
ignored runs: 4
symbols:      66510
redefinitions: 5322
parse errors: 0
symbol errors: 0
```

The 10,522 definition count matches the Rust compiler's per-file sum exactly.
The header count is higher because this parser scans every `.h`; the Rust
builder applies its header-set variant resolution first.

Implemented:

- unified lexer for `.def` / `.tpl` / `.h`
- AST for definitions, statements, tagged blocks, method calls, property paths,
  expressions, enums, defines, namespaces and conditionals
- parser with source spans and strict errors
- symbol table with last-definition-wins and redefinition reporting
- generated field reflection for the C++ data model (`visit_fields`)
- generated Transfer metadata from lifted `Transfer` bodies: exact wire names,
  member paths, transfer kinds and explicit defaults
- automatic text def-type → model class mapping (`with_class_for_type`)
- manifest-driven named index space (`249` NULLDEFs + `9,264` game named defs)
- `CDefString` name interning and `names.bin` emission
- `NameRef.ClassIndex` counters, entry preambles, and chunked binary containers
- tagged-block / anonymous sub-def collection, lowering and byte dedup
- `build` command emitting `game.bin`, `frontend.bin`, `script.bin` and
  `names.bin`
- generic lowering into the model: integers, floats, bools, enums,
  `CDefString`, `CDefIndex`, strings, UTF-16 strings, bit-fields and inherited
  fields
- sub-path fields (`Graphic.Type`, `States[...]`)
- container lowering: vectors, maps, arrays, and simple `.clear()` / `.Add()`
  methods
- `specialises` chains, read ancestor-first and flattened into one body
- manifest-driven def-name reference resolution (`GO_ITEMS`, `MATERIAL_WOOD`, …)
- explicit Transfer defaults applied before lowering
- wire serialization: field control ids (`crc32(name)`) + values for scalars,
  strings, enums, containers, compound classes and arrays
- CLI: `parse`, `symbols`, `lower [--dump]`, `build`

Current parity against the Rust compiler's output on the stock corpus:

- **frontend.bin: byte-identical** after decompression (all entries, NameRefs,
  counters and body bytes).
- **script.bin: one names.bin offset differs**; bodies and entry metadata match.
- **game.bin: structurally correct** at 12,932 entries (9,264 named plus
  3,419 distinct anonymous sub-defs), against Rust's 13,239 / 3,726. The
  remaining gap is mostly the animation/morph/creature bespoke lowering that
  distinguishes more sub-def bodies and adds their strings.
- `names.bin: 11,398 entries`, against Rust's 13,570; the missing strings are
  predominantly from those unlowered sub-def fields.

Chunks currently use a valid stored-DEFLATE stream rather than level-1
compression. That is deliberate while the container is being brought up; it
keeps the port dependency-free but makes the files much larger than retail.

Not yet ported:

- exact bespoke lowering for `CAppearanceDef`/`CAnimatingObjectDef` animations,
  `CCreatureDef` expressions/wounds, `CHeroMorphDef`, and the remaining arms
  (`CSpecialEffectsDef` and `CEntitySoundDef` are now close/exact on sub-def
  counts)
- the remaining WireStruct field-order/type differences exposed by game bodies
- retail-level deflate compression and final golden parity

## Build

The C++ projects use CMake:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Or from the `cpp/` directory with a plain CMake invocation:

```bash
cmake -S cpp -B build
cmake --build build
ctest --test-dir build
```

Run against a `Defs/` directory:

```bash
build/cpp-debug/bin/fable-defc parse ~/fable-ref/Data/Defs
build/cpp-debug/bin/fable-defc symbols ~/fable-ref/Data/Defs | head
build/cpp-debug/bin/fable-defc lower ~/fable-ref/Data/Defs \
    NUISystem::CUIDef UI_TABLE_TITLE_WHOLE
build/cpp-debug/bin/fable-defc lower ~/fable-ref/Data/Defs \
    auto CREATURE_BASE_TEMPLATE --binary game --dump
build/cpp-debug/bin/fable-defc build ~/fable-ref/Data/Defs /tmp/fable-out
```

The `lower` command lowers a definition into the model class. Pass `auto` to
select the class from the text def type:

```text
$ fable-defc lower ~/fable-ref/Data/Defs auto CREATURE_BASE_TEMPLATE
class:        CThingCreatureDef
```

The exact wire names and explicit defaults come from the decompiler's
`Transfer` bodies. Harvest and regenerate them with:

```bash
# Harvest every class with a Transfer declaration (about 5 s each).
python3 tools/lift_transfers.py \
    --tree /tmp/fable-all-tree \
    --decomp-cli ~/git/fable-decomp-engine/target/release/decomp-cli \
    --out /tmp/fable-transfer-lifts

python3 tools/gen_transfer.py --lifts /tmp/fable-transfer-lifts \
    --out cpp/fable-defs/include/fable/defs/transfer.hpp
python3 tools/gen_type_registry.py
```

`transfer.hpp` is committed; the raw lifts are not.

The `lower` command reports how many fields were applied, absent, or unsupported.
For example:

```text
$ fable-defc lower ~/fable-ref/Data/Defs CThingObjectDef OBJECT_STICK
definition:   OBJECT_STICK
chain:        OBJECT_BASE -> ... -> OBJECT_STICK
class:        CThingObjectDef
applied:      15
absent:       30
unsupported:  1
errors:       0
fields: ... GroupDef ... Material ... Health ... Type ... BankIndex ...
```

Sub-paths such as `Graphic.Type` and indexed fields such as `States[...]` lower.
The `build` command produces all four retail-named files; frontend parity is
exact, script parity is one names-table offset away, and game parity awaits the
remaining bespoke arms.

## Layout

```
include/fable/defs/compiler/
  diagnostic.hpp   FileId, Span, ParseError, render_error
  lexer.hpp        TokenKind, Token, lex
  ast.hpp          Item, Definition, Statement, Expr, declarations
  parser.hpp       parse_source
  symbols.hpp      SymbolTable, Redefinition
  lower.hpp        generic `lower_definition<T>` and expression evaluation
  wire.hpp         field controls, crc32 and value serialization
  type_traits.hpp  shared container/pointer traits
  model_registry.hpp  generated class-name dispatch
  type_registry.hpp   generated text def-type -> class dispatch
  build_index.hpp  manifest-driven index allocation
  build_pipeline.hpp  binary build pipeline and file emitter
  names.hpp        names.bin interning
  manifest.hpp     generated NULLDEF / header constants
src/
  diagnostic.cpp
  lexer.cpp
  parser.cpp
  symbols.cpp
  definitions.cpp
  build_index.cpp
  build_pipeline.cpp
  lower.cpp
  names.cpp
  wire.cpp
  defc_main.cpp
tests/parser_test.cpp
tests/lower_test.cpp
tests/wire_test.cpp
tests/corpus_test.cpp
```

## Roadmap

1. ~~**Model metadata** — generated `visit_fields` reflection.~~ Done.
2. ~~**Generic lowering** — scalar/string/bit-field/enum lowering with last-wins
   field statements.~~ Done.
3. ~~**References** — manifest-driven index space (249 NULLDEFs + 9,264 named
   game defs).~~ Done.
4. ~~**Specialization** — read `specialises` chains in place.~~ Done; tagged
   sub-defs remain.
5. ~~**Containers and sub-paths** — vectors, maps, arrays, `States[...]`,
   `Graphic.Type`.~~ Done; compound constructors remain.
6. ~~**Wire value encoding** — field controls, crc32, values.~~ Done for the
   supported types; `DefString` interning and `DefIndex` resolution still need
   the real index/names tables.
7. **Defaults and type fidelity** — Rust schema / retail NULLDEF defaults and
   the reference/string field types the PDB erased.
8. ~~**Binary container** — `game.bin` / `frontend.bin` / `script.bin` /
   `names.bin`, chunking and class counters.~~ Done; compression is stored
   DEFLATE for now.
9. ~~**Build pipeline** — manifest membership, NULLDEFs, sub-def dedup.~~ Done;
   frontend is byte-identical to the Rust compiler.
10. **Golden parity** — port the bespoke lowering arms until game/script and
    names.bin are byte-identical, then re-enable level-1 deflate.

The C++ model is already a link dependency of this library, so phase 1 plugs
into the existing declarations rather than re-declaring the schema.
