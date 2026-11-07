//! From-scratch def binary builder.
//!
//! Assembles `game.bin` / `frontend.bin` / `script.bin` (+ the shared
//! `names.bin`) with **our own** global-index allocation for the named and
//! sub-def regions. No retail binary is consulted.
//!
//! The pipeline has three phases:
//! 1. **[`parse_corpus`]** — parse every `.def`/`.tpl` file once, loading all
//!    header symbols, registering every source file, and producing the shared
//!    [`ParsedCorpus`].
//! 2. **[`build_one_bin`]** — for each binary, filter the corpus by manifest
//!    membership, allocate indices, lower, and serialize. Driven by a
//!    [`BinConfig`].
//! 3. **Finalize** — write `names.bin` from the shared [`NamesBuilder`].
//!
//! Diagnostics are **collected, not rendered**: everything the build has to say
//! about the corpus lands in [`BuildReport::diagnostics`] (or
//! [`BuildError::diagnostics`]) with the source spans intact, so the caller
//! owns presentation. [`BuildReport::sources`] carries the source text the
//! spans point into.

use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::{Duration, Instant};

use defs::binary::{
    Chunk, ChunkIndex, ChunkIndexEntry, ChunkIndexHeader, DefBinary, DefBinaryHeader, DefBody,
    EntryPreamble, EntryRecord, NameRef, SubDefRecord, def_name_has_subdef_table,
};
use defs::crc32;
use defs::names::NamesBuilder;
use defs::text::{
    DefParseError, Definition, Expr, FileId, SourceAst, Span, Spanned, Statement, SymbolTable,
    TextParseErrorKind, parse_source,
    symbols::{Redefinition, SymbolEvalError},
};

use crate::lower::{LowerError, chain_runs, lower_def, specialization_chain};
use crate::manifest;
use crate::reader::Body;
use crate::reader::{DefReaderError, EvalError};
use crate::walk_def_files;

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

/// One source file the build read, kept so diagnostic spans can be resolved
/// back to text by whoever renders them.
#[derive(Debug, Clone)]
pub struct SourceFile {
    /// Path as given on disk, with separators normalized to `/`.
    pub path: String,
    pub text: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Severity {
    Warning,
    Error,
}

/// An annotated span, in a named source file.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DiagnosticLabel {
    /// Which file the span is in — an index into `sources`.
    ///
    /// Per-label rather than per-diagnostic because one diagnostic can point
    /// into several files at once. Specialization is the reason: a bad
    /// statement in a template in `objects.tpl` is inherited by definitions in
    /// a dozen other files, and the error wants to show both the template and
    /// the definitions affected.
    ///
    /// It is also a correctness fix. When the file lived on the diagnostic, the
    /// lowering path set it from the *child* definition's file while the span
    /// came from the *template*, so a cross-file inherited error drew its caret
    /// at that byte offset in the wrong file — pointing confidently at
    /// unrelated source.
    pub source: usize,
    /// `true` for the span the diagnostic is *about*; `false` for supporting
    /// context (e.g. "in this definition").
    pub primary: bool,
    pub span: Span,
    pub message: Option<String>,
}

/// One thing the build has to say about the corpus.
///
/// A diagnostic with no labels has no source location — an I/O failure, or a
/// statement about the corpus rather than about a byte in it. Otherwise each
/// label names its own file, so one diagnostic can point into several.
#[derive(Debug, Clone)]
pub struct BuildDiagnostic {
    pub severity: Severity,
    pub message: String,
    pub labels: Vec<DiagnosticLabel>,
    /// Trailing explanations, rendered after the source excerpt.
    ///
    /// Keeps context out of the headline. `unknown constant` and
    /// `unknown definition` are the same shape of mistake in two different
    /// namespaces, and a note is where that distinction belongs — the headline
    /// should stay short enough to scan.
    pub notes: Vec<String>,
}

impl BuildDiagnostic {
    /// A diagnostic with no source location — an I/O failure, or a statement
    /// about the corpus as a whole rather than about a byte in it.
    fn bare(severity: Severity, message: String) -> Self {
        Self {
            severity,
            message,
            labels: Vec::new(),
            notes: Vec::new(),
        }
    }

    /// The location this diagnostic is *about*, used for ordering.
    fn primary_location(&self) -> Option<(usize, usize)> {
        self.labels
            .iter()
            .find(|l| l.primary)
            .map(|l| (l.source, l.span.start))
    }
}

/// Per-binary outcome.
#[derive(Debug, Clone)]
pub struct BinSummary {
    pub label: &'static str,
    pub file_name: &'static str,
    /// Named definitions successfully lowered.
    pub lowered: usize,
    /// Total entries written (NULLDEF + named + anonymous sub-defs).
    pub entries: u32,
    pub has_sub_defs: bool,
    pub sub_defs_lowered: usize,
    /// Distinct anonymous sub-def entries after `(tag, bytes)` dedup.
    pub sub_defs_unique: usize,
}

/// A successful build.
#[derive(Debug)]
pub struct BuildReport {
    pub sources: Vec<SourceFile>,
    /// Warnings only — any error fails the build (see [`BuildError`]).
    pub diagnostics: Vec<BuildDiagnostic>,
    pub bins: Vec<BinSummary>,
    pub elapsed: Duration,
}

impl BuildReport {
    pub fn warnings(&self) -> impl Iterator<Item = &BuildDiagnostic> {
        self.diagnostics
            .iter()
            .filter(|d| d.severity == Severity::Warning)
    }
}

/// A failed build. Carries the diagnostics collected before the failure so the
/// caller can report *why* it failed, not just that it did.
#[derive(Debug)]
pub struct BuildError {
    pub message: String,
    pub sources: Vec<SourceFile>,
    pub diagnostics: Vec<BuildDiagnostic>,
}

impl BuildError {
    fn bare(message: String) -> Self {
        Self {
            message,
            sources: Vec::new(),
            diagnostics: Vec::new(),
        }
    }

    pub fn errors(&self) -> impl Iterator<Item = &BuildDiagnostic> {
        self.diagnostics
            .iter()
            .filter(|d| d.severity == Severity::Error)
    }
}

impl std::fmt::Display for BuildError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.message)
    }
}

impl std::error::Error for BuildError {}

/// Progress events, emitted as the build proceeds so long builds can report
/// liveness. Purely informational — everything durable is in [`BuildReport`].
#[derive(Debug)]
pub enum Progress<'a> {
    FileParsed { path: &'a str, definitions: usize },
    CompileStarted,
    Lowering { label: &'static str, named: usize },
    BinFinished(&'a BinSummary),
}

/// Compile the text def corpus under `input` into the four binaries in
/// `output`, creating `output` if needed.
///
/// `input` is the `Defs/` directory: `.def`/`.tpl` sources plus the `.h` header
/// files they draw symbols from, scanned recursively.  `output` receives
/// `game.bin`, `frontend.bin`, `script.bin`, and `names.bin`.
pub fn build(input: &Path, output: &Path) -> Result<BuildReport, BuildError> {
    build_with_progress(input, output, &mut |_| {})
}

/// [`build`], plus a callback invoked as the build proceeds.
pub fn build_with_progress(
    input: &Path,
    output: &Path,
    on_progress: &mut dyn FnMut(Progress),
) -> Result<BuildReport, BuildError> {
    // Without this check a wrong input path is not an error: the directory walks
    // yield nothing, and the build cheerfully emits NULLDEF-only binaries that
    // look valid and load into a broken game.
    if !input.is_dir() {
        return Err(BuildError::bare(format!(
            "input is not a directory: {}",
            input.display()
        )));
    }
    std::fs::create_dir_all(output)
        .map_err(|e| BuildError::bare(format!("create out dir: {e}")))?;

    let started = Instant::now();
    let diagnostics = Diagnostics::default();

    let corpus = match parse_corpus(input, &diagnostics, on_progress) {
        Ok(c) => c,
        Err(message) => {
            return Err(BuildError {
                message,
                sources: Vec::new(),
                diagnostics: diagnostics.take(),
            });
        }
    };

    // From here on any failure can still point at source, so hand the caller
    // the sources and everything collected so far.
    let finish =
        |message: String, sources: Vec<SourceFile>, diagnostics: &Diagnostics| BuildError {
            message,
            sources,
            diagnostics: diagnostics.take(),
        };

    // A file that fails to load is fatal: a bad `.def` drops every def in it,
    // and a bad `.h` drops the symbols every def referencing them needs.
    // Continuing would silently emit binaries missing defs, or bury the real
    // cause under a pile of "unknown symbol" errors at distant use sites —
    // exactly the failures the diagnostics exist to prevent.
    let load_failures = diagnostics.count(Severity::Error);
    if load_failures > 0 {
        return Err(finish(
            format!(
                "{load_failures} source file{} failed to load",
                if load_failures == 1 { "" } else { "s" }
            ),
            corpus.sources,
            &diagnostics,
        ));
    }

    on_progress(Progress::CompileStarted);
    let names_cell = RefCell::new(NamesBuilder::new());
    let mut bins = Vec::with_capacity(3);
    for config in [&GAME_CONFIG, &FRONTEND_CONFIG, &SCRIPT_CONFIG] {
        match build_one_bin(
            &corpus,
            config,
            &diagnostics,
            &names_cell,
            output,
            on_progress,
        ) {
            Ok(summary) => {
                on_progress(Progress::BinFinished(&summary));
                bins.push(summary);
            }
            Err(message) => return Err(finish(message, corpus.sources, &diagnostics)),
        }
    }

    let names = names_cell
        .into_inner()
        .finalize(manifest::NAMES_HEADER_BYTES);
    if let Err(e) = std::fs::write(output.join("names.bin"), names.to_bytes()) {
        return Err(finish(
            format!("write names.bin: {e}"),
            corpus.sources,
            &diagnostics,
        ));
    }

    Ok(BuildReport {
        sources: corpus.sources,
        diagnostics: diagnostics.take(),
        bins,
        elapsed: started.elapsed(),
    })
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Internal plumbing
// ═══════════════════════════════════════════════════════════════════════════════

/// Interior-mutable diagnostic sink, so the collection points can sit behind
/// the shared `&BuildCtx` borrows the pipeline is built on.
#[derive(Default)]
struct Diagnostics(RefCell<Vec<BuildDiagnostic>>);

impl Diagnostics {
    fn push(&self, diag: BuildDiagnostic) {
        self.0.borrow_mut().push(diag);
    }

    /// Drain, merging repeats of one mistake and ordering by source position.
    ///
    /// Specialization fan-out is why this exists. A template's statements are
    /// read by every descendant, so one bad
    /// statement in a widely-inherited template is lowered once per descendant
    /// and reported once per descendant — every report with its caret on the
    /// same byte of the same template, because that is the only place the
    /// mistake is written. A stock-shaped template can have well over a
    /// thousand descendants.
    ///
    /// The descendants are not labelled, only counted: an inheriting definition
    /// contains no trace of the problem, so sending the reader there wastes the
    /// trip. What it does need to know is the blast radius, which is the note.
    ///
    /// Genuinely distinct mistakes are untouched: two typos of the same name on
    /// two different lines have different primary spans and stay separate, and
    /// both are reported.
    ///
    /// Ordering is by (file, offset) so a build reads top-to-bottom per file
    /// rather than in pipeline order, which interleaved header diagnostics,
    /// parse errors, and three separate per-binary lowering passes.
    fn take(&self) -> Vec<BuildDiagnostic> {
        /// One mistake: the same message, at the same place, at the same
        /// severity.
        type MergeKey = (Severity, Option<(usize, usize, usize)>, String);

        let raw = self.0.borrow_mut().split_off(0);
        let mut out: Vec<BuildDiagnostic> = Vec::with_capacity(raw.len());
        let mut seen: HashMap<MergeKey, usize> = HashMap::new();
        let mut repeats: Vec<usize> = Vec::new();
        for diag in raw {
            let primary = diag
                .labels
                .iter()
                .find(|l| l.primary)
                .map(|l| (l.source, l.span.start, l.span.end));
            let key = (diag.severity, primary, diag.message.clone());
            match seen.get(&key) {
                Some(&i) => repeats[i] += 1,
                None => {
                    seen.insert(key, out.len());
                    repeats.push(0);
                    out.push(diag);
                }
            }
        }
        for (diag, repeats) in out.iter_mut().zip(repeats) {
            if repeats > 0 {
                diag.notes.push(format!(
                    "{} other definition(s) inherit this and fail for the same reason",
                    repeats
                ));
            }
        }
        out.sort_by_key(|d| d.primary_location());
        out
    }

    fn count(&self, severity: Severity) -> usize {
        self.0
            .borrow()
            .iter()
            .filter(|d| d.severity == severity)
            .count()
    }
}

/// Normalize `\` to `/` so path matching and reported paths are the same on
/// every platform. The scoping rules below match on `/`-separated fragments.
fn normalize_path(path: &Path) -> String {
    path.to_string_lossy().replace('\\', "/")
}

/// Recursively walk all statements/expressions in every definition's body
/// (including specialization-chain parents) and collect every bare symbol and
/// quoted string name. These names are candidate def references; template defs
/// whose name never appears in this set are unused and should not become binary
/// entries.
fn collect_body_references(
    files: &[&ParsedFile],
    defs_by_name: &HashMap<&str, &Definition>,
) -> HashSet<String> {
    fn walk_expr(expr: &Expr, out: &mut HashSet<String>) {
        match expr {
            Expr::String(s) | Expr::Symbol(s) => {
                out.insert(s.clone());
            }
            Expr::Constructor(call) => {
                for arg in &call.arguments {
                    walk_expr(&arg.value, out);
                }
            }
            Expr::BitOr(terms) | Expr::Add(terms) => {
                for t in terms {
                    walk_expr(&t.value, out);
                }
            }
            _ => {}
        }
    }
    fn walk_stmt(stmt: &Statement, out: &mut HashSet<String>) {
        match stmt {
            Statement::Field(f) => walk_expr(&f.expr.value, out),
            Statement::MethodCall(mc) => {
                for arg in &mc.call.arguments {
                    walk_expr(&arg.value, out);
                }
            }
            Statement::TaggedBlock(tb) => {
                for s in &tb.body {
                    walk_stmt(&s.value, out);
                }
            }
        }
    }
    fn walk_specialization_chain<'a>(
        def: &'a Definition,
        defs_by_name: &'a HashMap<&str, &Definition>,
        out: &mut HashSet<String>,
        visited: &mut HashSet<&'a str>,
    ) {
        if !visited.insert(def.name.as_str()) {
            return;
        }
        for stmt in &def.body {
            walk_stmt(&stmt.value, out);
        }
        if let Some(parent_name) = &def.specializes
            && let Some(parent) = defs_by_name.get(parent_name.as_str())
        {
            walk_specialization_chain(parent, defs_by_name, out, visited);
        }
    }
    let mut refs = HashSet::new();
    let mut visited = HashSet::new();
    for pf in files {
        for d in pf.def_file.definitions() {
            walk_specialization_chain(&d.value, defs_by_name, &mut refs, &mut visited);
        }
    }
    refs
}

/// Shared context built once per binary and threaded through the lowering
/// and emission pipeline.
struct BuildCtx<'a> {
    symbols: &'a SymbolTable,
    def_indices: &'a HashMap<String, u32>,
    names: &'a RefCell<NamesBuilder>,
    sources: &'a [SourceFile],
    def_to_source: &'a HashMap<String, usize>,
    def_spans: &'a HashMap<String, Span>,
    defs_by_name: &'a HashMap<&'a str, &'a Definition>,
    nulldefs: &'a HashMap<String, DefBody>,
    diagnostics: &'a Diagnostics,
}

struct ParsedCorpus {
    /// Parsed def files with their disk paths (for per-binary scoping).
    files: Vec<ParsedFile>,
    symbols: SymbolTable,
    sources: Vec<SourceFile>,
    def_to_source: HashMap<String, usize>,
    def_spans: HashMap<String, Span>,
}

struct ParsedFile {
    /// Normalized (`/`-separated) path; also the `sources` entry's path.
    path: String,
    def_file: SourceAst,
}

/// Per-binary configuration: everything that distinguishes game.bin from
/// frontend.bin from script.bin. All membership data comes from the manifest.
struct BinConfig {
    label: &'static str,
    nulldef_entries: &'static [&'static str],
    binary_header: DefBinaryHeader,
    out_filename: &'static str,
    has_subdefs: bool,
    /// Exclude template defs that aren't referenced by any other def's body.
    /// Safe to enable for large binaries (game.bin); disable for small ones
    /// (frontend/script) where the engine may reference templates by name.
    filter_templates: bool,
    file_scope: fn(corpus: &[ParsedFile]) -> Vec<&ParsedFile>,
}

struct Built {
    def_name_off: u32,
    file_name_off: u32,
    counter: u32,
    preamble: EntryPreamble,
    sub_defs: Option<Vec<SubDefRecord>>,
    body: DefBody,
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Per-binary file scoping
// ═══════════════════════════════════════════════════════════════════════════════

fn game_file_scope(corpus: &[ParsedFile]) -> Vec<&ParsedFile> {
    corpus
        .iter()
        .filter(|pf| {
            let p = pf.path.replace('\\', "/");
            !p.contains("/FrontEndDefs/") && !p.contains("/ScriptDefs/")
        })
        .collect()
}

/// Retail's explicit frontend file list first, in its own order, then any other
/// file under `FrontEndDefs/` in walk order. The tail is what lets defs added
/// by an editor (which writes to its own file in that directory) reach
/// frontend.bin; the retail corpus has no such files, so ordering there is
/// unchanged.
fn frontend_file_scope(corpus: &[ParsedFile]) -> Vec<&ParsedFile> {
    let mut scoped: Vec<&ParsedFile> = manifest::FRONTEND_DEF_FILES
        .iter()
        .filter_map(|rel| corpus.iter().find(|pf| path_ends_with(&pf.path, rel)))
        .collect();
    for pf in corpus {
        if pf.path.contains("/FrontEndDefs/") && !scoped.iter().any(|s| std::ptr::eq(*s, pf)) {
            scoped.push(pf);
        }
    }
    scoped
}

fn script_file_scope(corpus: &[ParsedFile]) -> Vec<&ParsedFile> {
    corpus
        .iter()
        .filter(|pf| pf.path.contains("ScriptDefs/"))
        .collect()
}

/// Whether a normalized path ends with a normalized relative path, **at a path
/// boundary**.
///
/// The boundary matters: the corpus has both `Defs/controls.def` and
/// `Defs/pc_controls.def`, and both `Defs/engine.def` and
/// `Defs/FrontEndDefs/engine.def`. A bare `str::ends_with` matches the wrong one
/// of each pair, and only picks the right file because the sorted walk order
/// happens to reach it first.
fn path_ends_with(path: &str, rel: &str) -> bool {
    match path.strip_suffix(rel) {
        Some("") => true,
        Some(prefix) => prefix.ends_with('/'),
        None => false,
    }
}

static GAME_CONFIG: BinConfig = BinConfig {
    label: "game",
    nulldef_entries: manifest::GAME_NULLDEF_ENTRIES,
    binary_header: manifest::GAME_HEADER,
    out_filename: "game.bin",
    has_subdefs: true,
    filter_templates: false,
    file_scope: game_file_scope,
};

static FRONTEND_CONFIG: BinConfig = BinConfig {
    label: "frontend",
    nulldef_entries: manifest::FRONTEND_NULLDEF_ENTRIES,
    binary_header: manifest::FRONTEND_HEADER,
    out_filename: "frontend.bin",
    has_subdefs: false,
    filter_templates: false,
    file_scope: frontend_file_scope,
};

static SCRIPT_CONFIG: BinConfig = BinConfig {
    label: "script",
    nulldef_entries: manifest::SCRIPT_NULLDEF_ENTRIES,
    binary_header: manifest::SCRIPT_HEADER,
    out_filename: "script.bin",
    has_subdefs: false,
    filter_templates: false,
    file_scope: script_file_scope,
};

// ═══════════════════════════════════════════════════════════════════════════════
//  Phase 1: Parse the entire corpus once
// ═══════════════════════════════════════════════════════════════════════════════

/// Directories under the corpus root that each hold a **complete variant of
/// the same header set**, most-preferred first.
///
/// `Data/Defs` does not ship one flat set of headers. `RetailHeaders/` and
/// `DevHeaders/` are two builds of the same ~15 files (`meshdata.h`, `text.h`,
/// `pc/textures.h`, …), so scanning both unions two copies of every symbol
/// into one namespace. Exactly one set must win.
///
/// `RetailHeaders` is preferred because it is the richer of the two — the
/// `DevHeaders` lipsync headers are empty stubs — and because it is the set the
/// modding tools write to. The two agree on every shared name/value pair, so
/// the choice does not change the compiled output of a stock corpus; it decides
/// only which set a mod's edits are read from. A corpus with just one of them
/// (the Anniversary debug build ships only `DevHeaders/`) is unaffected.
const HEADER_SET_ROOTS: &[&str] = &["RetailHeaders", "DevHeaders"];

/// Platform variants *within* a header set. Same problem one level down:
/// `pc/textures.h` and `xbox/textures.h` define the same names.
const IGNORED_PLATFORM_DIRS: &[&str] = &["/xbox/"];

/// Pick the header set to read, and report the ones being skipped.
///
/// The skipped sets are named because a modder who patched the losing set
/// would otherwise see their symbols silently vanish — the exact failure this
/// resolution exists to prevent.
fn select_header_set(source: &Path, diagnostics: &Diagnostics) -> Option<&'static str> {
    let present: Vec<&'static str> = HEADER_SET_ROOTS
        .iter()
        .copied()
        .filter(|root| source.join(root).is_dir())
        .collect();
    let (&winner, skipped) = present.split_first()?;
    if !skipped.is_empty() {
        let list = skipped
            .iter()
            .map(|r| format!("{r}/"))
            .collect::<Vec<_>>()
            .join(", ");
        diagnostics.push(BuildDiagnostic::bare(
            Severity::Warning,
            format!(
                "using header set {winner}/; ignoring {list} \
                 (same headers, one set must win — edits to the ignored set are not read)"
            ),
        ));
    }
    Some(winner)
}

/// Collect the `.h` files to read.
///
/// All variant matching is done on the path **relative to the corpus root**.
/// Matching the absolute path would make the compiler's behaviour depend on
/// where the corpus happens to live — a directory called `xbox` or
/// `DevHeaders` anywhere above the root would silently skip every header.
fn collect_h_files(dir: &Path, root: &Path, selected_set: Option<&str>, out: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    let root_prefix = normalize_path(root);
    for entry in entries.filter_map(|e| e.ok()) {
        let path = entry.path();
        if path.is_dir() {
            collect_h_files(&path, root, selected_set, out);
        } else if matches!(path.extension().and_then(|e| e.to_str()), Some("h")) {
            let full = normalize_path(&path);
            // Leading `/` so a segment match is always `/segment/`, including
            // for a variant root sitting directly at the corpus root.
            let rel = match full.strip_prefix(&root_prefix) {
                Some(r) => format!("/{}", r.trim_start_matches('/')),
                None => full.clone(),
            };
            if IGNORED_PLATFORM_DIRS.iter().any(|d| rel.contains(d))
                || rel.contains("scriptdialoguesnds2")
            {
                continue;
            }
            // Headers outside every variant root (the ~47 at the corpus root)
            // are shared and always read.
            let losing_set = HEADER_SET_ROOTS
                .iter()
                .filter(|r| Some(**r) != selected_set)
                .any(|r| rel.contains(&format!("/{r}/")));
            if losing_set {
                continue;
            }
            out.push(path);
        }
    }
}

/// Read every header in the selected set into one symbol table.
///
/// A header that cannot be read, parsed, or evaluated is an **error**: it
/// contributes no symbols (or, for an evaluate failure, only those before the
/// failure), so letting the build continue turns one broken header into a
/// distant pile of "unknown symbol" errors at the use sites.
///
/// Every header read is registered in `sources`, so its diagnostics render with
/// a line number, a source excerpt, and a caret — the same treatment `.def`
/// files get. Headers are ~3.8 MB against the ~9.1 MB of `.def`/`.tpl` text
/// already retained; keeping them all (rather than only the failing ones) is
/// what lets redefinition warnings point at a location too.
fn load_symbols(
    source: &Path,
    diagnostics: &Diagnostics,
    sources: &mut Vec<SourceFile>,
) -> SymbolTable {
    let mut symbols = SymbolTable::new();
    let selected_set = select_header_set(source, diagnostics);
    let mut h_files = Vec::new();
    collect_h_files(source, source, selected_set, &mut h_files);
    h_files.sort();

    // Read, then assign ids, then parse, then evaluate. Reading and parsing are
    // pure and run in parallel; **evaluation must stay sequential and in file
    // order**, because `SymbolTable` is last-definition-wins (§4.1).
    //
    // Ids are assigned only to files that read successfully, matching the
    // previous behaviour where an unreadable header consumed no id and shifted
    // every later one down.
    let reads: Vec<std::io::Result<String>> = parallel_map(
        &h_files,
        |p| p.metadata().map(|m| m.len() as usize).unwrap_or(0),
        |_, path| std::fs::read_to_string(path),
    );

    let mut to_parse: Vec<usize> = Vec::with_capacity(h_files.len());
    for (path, text) in h_files.iter().zip(reads) {
        let display = normalize_path(path);
        match text {
            Ok(text) => {
                to_parse.push(sources.len());
                sources.push(SourceFile {
                    path: display,
                    text,
                });
            }
            // No text to point at, so this one stays location-less.
            Err(e) => diagnostics.push(BuildDiagnostic::bare(
                Severity::Error,
                format!("cannot read header {display}: {e}"),
            )),
        }
    }

    let parsed = parallel_map(
        &to_parse,
        |&sid| sources[sid].text.len(),
        |_, &sid| parse_source(&sources[sid].text, FileId(sid as u32)),
    );

    for (&sid, ast) in to_parse.iter().zip(parsed) {
        match ast {
            Ok(ast) => match symbols.evaluate_items(&ast.items) {
                Ok(redefined) => report_redefinitions(sid, &redefined, diagnostics),
                Err(e) => diagnostics.push(symbol_eval_diagnostic(sid, &e)),
            },
            Err(e) => diagnostics.push(parse_error_diagnostic(sid, &e)),
        }
    }
    symbols
}

/// Map `f` over `items` across threads, returning results **in input order**.
///
/// Work is handed out **largest first** — `cost` orders it — because file sizes
/// span ~146× and the makespan is set by when the biggest file starts. For a
/// fixed set of independent items whose costs are known up front, that is all
/// the scheduling this needs: measured on the corpus it beats `rayon`'s
/// work-stealing (16.5 ms vs 17.9 ms on 12 cores, against 91 ms sequential),
/// with no dependency and no thread pool outliving the call.
///
/// The last point is the one that matters beyond this function: `defc_build` is
/// a C ABI entry point in a cdylib that EgoCore loads, so threads must not
/// survive the call that spawned them. `std::thread::scope` guarantees that.
fn parallel_map<T, R, F>(items: &[T], cost: impl Fn(&T) -> usize, f: F) -> Vec<R>
where
    T: Sync,
    R: Send,
    F: Fn(usize, &T) -> R + Sync,
{
    let threads = std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(1)
        .min(items.len());
    if threads <= 1 {
        return items.iter().enumerate().map(|(i, x)| f(i, x)).collect();
    }

    let mut order: Vec<usize> = (0..items.len()).collect();
    order.sort_by_key(|&i| std::cmp::Reverse(cost(&items[i])));

    let next = AtomicUsize::new(0);
    let (f, order, next) = (&f, &order, &next);
    // Each worker keeps its own results and they are placed by index at the
    // end, so nothing is shared mutably and the output cannot depend on which
    // thread finished first.
    let mut parts: Vec<Vec<(usize, R)>> = std::thread::scope(|scope| {
        let handles: Vec<_> = (0..threads)
            .map(|_| {
                scope.spawn(move || {
                    let mut out = Vec::new();
                    loop {
                        let slot = next.fetch_add(1, Ordering::Relaxed);
                        let Some(&i) = order.get(slot) else { break };
                        out.push((i, f(i, &items[i])));
                    }
                    out
                })
            })
            .collect();
        handles.into_iter().map(|h| h.join().unwrap()).collect()
    });

    let mut slots: Vec<Option<R>> = (0..items.len()).map(|_| None).collect();
    for (i, r) in parts.drain(..).flatten() {
        slots[i] = Some(r);
    }
    slots
        .into_iter()
        .map(|r| r.expect("every index filled"))
        .collect()
}

/// A declaration that would not evaluate, pointed at the offending expression.
fn symbol_eval_diagnostic(source: usize, error: &SymbolEvalError) -> BuildDiagnostic {
    BuildDiagnostic {
        severity: Severity::Error,
        message: format!("{error}"),
        labels: vec![DiagnosticLabel {
            source,
            primary: true,
            span: error.span(),
            message: None,
        }],
        notes: Vec::new(),
    }
}

/// How many redefinitions to name individually before collapsing to a count.
/// A regenerated header can redefine thousands of symbols at once, and a log
/// that long buries every other diagnostic.
const MAX_REPORTED_REDEFINITIONS: usize = 10;

fn report_redefinitions(source: usize, redefined: &[Redefinition], diagnostics: &Diagnostics) {
    for r in redefined.iter().take(MAX_REPORTED_REDEFINITIONS) {
        diagnostics.push(BuildDiagnostic {
            severity: Severity::Warning,
            message: format!(
                "symbol {} redefined: {} -> {} (later definition wins)",
                r.name, r.previous, r.value
            ),
            labels: vec![DiagnosticLabel {
                source,
                primary: true,
                span: r.span,
                message: Some(format!("was {}, now {}", r.previous, r.value)),
            }],
            notes: Vec::new(),
        });
    }
    if let Some(rest) = redefined.len().checked_sub(MAX_REPORTED_REDEFINITIONS)
        && rest > 0
    {
        diagnostics.push(BuildDiagnostic {
            severity: Severity::Warning,
            message: format!("… and {rest} more symbol(s) redefined in this file"),
            labels: Vec::new(),
            notes: Vec::new(),
        });
    }
}

/// Engine enums the def-script parser registers in C++ (ECompositeBlendType,
/// _core/L4.hpp) that appear in no text header. The def-script uses the short
/// `BLEND_*` names (COMPOSITE_ prefix stripped). `BLEND_ALPHA = 2` is confirmed
/// against retail (every CHeroMorphDef TextureMorph blend); without it,
/// `TextureMorphs.Add(..., BLEND_ALPHA)` would silently lower the blend to 0.
///
/// These are injected *after* the headers are read, so with last-definition-wins
/// they would override a corpus that does define them. They are fallbacks for
/// names the text is missing, not overrides, so a header definition keeps
/// priority. No corpus currently defines any of them.
fn inject_engine_enums(symbols: &mut SymbolTable) {
    let mut fallback = |name: &str, value: i64| {
        if symbols.lookup(name).is_none() {
            symbols.insert(name, value);
        }
    };
    fallback("WATER_BUMP_PC", 0);
    for (name, value) in [
        ("BLEND_NULL", 0),
        ("BLEND_ADDITIVE", 1),
        ("BLEND_ALPHA", 2),
        ("BLEND_SOLID", 3),
        ("BLEND_MULTIPLY", 4),
    ] {
        fallback(name, value);
    }
}

/// Parse every `.def` and `.tpl` file under `source`, loading all header
/// symbols.  Returns a [`ParsedCorpus`] that all three binary builders share.
fn parse_corpus(
    source: &Path,
    diagnostics: &Diagnostics,
    on_progress: &mut dyn FnMut(Progress),
) -> Result<ParsedCorpus, String> {
    // Headers are registered first, so header diagnostics can reference them by
    // source id; `.def`/`.tpl` files append after.
    let mut sources: Vec<SourceFile> = Vec::new();
    let mut symbols = load_symbols(source, diagnostics, &mut sources);
    inject_engine_enums(&mut symbols);

    let mut def_to_source: HashMap<String, usize> = HashMap::new();
    let mut def_spans: HashMap<String, Span> = HashMap::new();
    let mut parsed_files: Vec<ParsedFile> = Vec::new();
    let mut source_ids: Vec<usize> = Vec::new();

    let def_files: Vec<PathBuf> = walk_def_files(source)
        .into_iter()
        .filter(|p| {
            !p.file_name()
                .and_then(|n| n.to_str())
                .is_some_and(|n| n.contains("_deprecated."))
        })
        .collect();

    // Read and parse in parallel, then fold the results in file order. A read
    // failure here is fatal, unlike a header's — a missing `.def` silently
    // drops its definitions from the output.
    let reads: Vec<std::io::Result<Vec<u8>>> = parallel_map(
        &def_files,
        |p| p.metadata().map(|m| m.len() as usize).unwrap_or(0),
        |_, p| std::fs::read(p),
    );
    let mut texts: Vec<String> = Vec::with_capacity(def_files.len());
    for (p, raw) in def_files.iter().zip(reads) {
        let raw = raw.map_err(|e| format!("read {p:?}: {e}"))?;
        texts.push(String::from_utf8_lossy(&raw).into_owned());
    }

    // Ids have to be known before parsing, since every span a parse produces is
    // stamped with one. Every `.def` gets an id whether or not it parses, so
    // they are just consecutive from where the headers left off.
    let base = sources.len();
    let parsed = parallel_map(
        &texts,
        |t| t.len(),
        |i, text| parse_source(text, FileId((base + i) as u32)),
    );

    for ((p, text), ast) in def_files.iter().zip(texts).zip(parsed) {
        let path = normalize_path(p);
        let sid = sources.len();
        match ast {
            Ok(f) => {
                let def_count = f.definitions().count();
                sources.push(SourceFile {
                    path: path.clone(),
                    text,
                });
                source_ids.push(sid);
                for d in f.definitions() {
                    def_to_source.insert(d.value.name.clone(), sid);
                    def_spans.insert(d.value.name.clone(), d.span);
                }
                on_progress(Progress::FileParsed {
                    path: &path,
                    definitions: def_count,
                });
                parsed_files.push(ParsedFile { path, def_file: f });
            }
            Err(e) => {
                sources.push(SourceFile { path, text });
                diagnostics.push(parse_error_diagnostic(sid, &e));
            }
        }
    }

    // An input directory that exists but holds no def sources would otherwise
    // produce NULLDEF-only binaries with no indication anything was wrong.
    //
    // Test `sources`, not `parsed_files`: a file that fails to parse is still
    // registered as a source, so this stays "found nothing to compile" and does
    // not swallow the "everything failed to parse" case, whose diagnostics are
    // far more useful to the caller.
    if sources.is_empty() {
        return Err(format!(
            "no .def or .tpl files found under {}",
            source.display()
        ));
    }

    for (&sid, pf) in source_ids.iter().zip(parsed_files.iter()) {
        match symbols.evaluate_items(&pf.def_file.items) {
            Ok(redefined) => report_redefinitions(sid, &redefined, diagnostics),
            Err(e) => diagnostics.push(symbol_eval_diagnostic(sid, &e)),
        }
    }

    Ok(ParsedCorpus {
        files: parsed_files,
        symbols,
        sources,
        def_to_source,
        def_spans,
    })
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Diagnostic construction
// ═══════════════════════════════════════════════════════════════════════════════

fn def_header_end(text: &str, start: usize) -> usize {
    text[start..].find('\n').map_or(text.len(), |n| start + n)
}

/// A parse error is strict and fatal for its file, so the whole file's defs
/// vanish — it must always be surfaced.
///
/// The primary label underlines the offending token; the secondary names the
/// construct the error happened inside ("in enum `ESoundNames`"), which the
/// parser attaches as it unwinds. Both grammars produce the same shape.
fn parse_error_diagnostic(source: usize, error: &DefParseError) -> BuildDiagnostic {
    let msg = format!("{error}");
    // No message on the primary label: it would repeat the headline verbatim
    // directly under it. The caret alone carries the location.
    let mut labels = vec![DiagnosticLabel {
        source,
        primary: true,
        span: error.span,
        message: None,
    }];
    if let Some(context) = &error.context {
        // For a missing `#end_definition` the useful statement is about the
        // *construct*, not the token that revealed it.
        let note = match error.inner {
            TextParseErrorKind::MissingEndDefinition => {
                format!("this {} is never closed", context.what)
            }
            _ => context.label(),
        };
        labels.push(DiagnosticLabel {
            source,
            primary: false,
            span: context.span,
            message: Some(note),
        });
    }
    BuildDiagnostic {
        severity: Severity::Error,
        message: msg,
        labels,
        notes: Vec::new(),
    }
}

fn lowering_error_diagnostic(
    def_name: &str,
    def_type: &str,
    error: &LowerError,
    ctx: &BuildCtx,
) -> BuildDiagnostic {
    let source = ctx.def_to_source.get(def_name).copied();
    let Some(sid) = source else {
        return BuildDiagnostic::bare(Severity::Error, format!("{def_type} {def_name}: {error}"));
    };
    let mut labels = Vec::new();
    let primary = error.primary_span();
    // The span names its own file. That matters here because the statement may
    // have been inherited: a template's statements are read by every child, so
    // the span can belong to a different file than the definition being
    // compiled.
    if let Some(span) = primary {
        labels.push(DiagnosticLabel {
            source: span.file.0 as usize,
            primary: true,
            span,
            message: None,
        });
    }

    // "in this definition" is only worth saying when the offending text is
    // *in* this definition.
    //
    // If the statement was inherited, this definition does not mention the
    // problem anywhere — it just specialises something that does. Labelling it
    // sends the reader to a file where there is nothing to see and nothing to
    // fix, and a template near the root of the hierarchy would drag in
    // hundreds of such labels. The count is the useful part, and the merge in
    // `Diagnostics::take` supplies that.
    //
    // Containment is exact now that spans know their file; before, comparing
    // bare offsets across files was a coin flip.
    let own_statement = primary.is_some_and(|span| {
        ctx.def_spans
            .get(def_name)
            .is_some_and(|d| d.contains(span))
    });
    if own_statement && let Some(dspan) = ctx.def_spans.get(def_name) {
        // Point at the name in the header line, not the whole line, so the
        // caret lands on what identifies the def.
        let text = &ctx.sources[sid].text;
        let header_line = &text[dspan.start..def_header_end(text, dspan.start)];
        if let Some(name_pos) = header_line.find(def_name) {
            let start = dspan.start + name_pos;
            labels.push(DiagnosticLabel {
                source: sid,
                primary: false,
                span: Span::new(dspan.file, start, start + def_name.len()),
                message: Some("in this definition".to_string()),
            });
        }
    }
    // Names live in two namespaces that look identical at a use site: constants
    // come from `enum`/`#define` declarations, definition names from
    // `#definition`. Saying which one was searched is the difference between
    // "typo" and "wrong kind of thing entirely".
    let notes = match error {
        LowerError::Reader(DefReaderError::Eval(EvalError::UnknownSymbol(_), _)) => {
            vec!["constants come from `enum` / `#define` declarations".to_string()]
        }
        LowerError::UnresolvedReference(..) => {
            vec![format!(
                "no `#definition` with this name is in scope for this binary"
            )]
        }
        _ => Vec::new(),
    };
    BuildDiagnostic {
        severity: Severity::Error,
        message: format!("{error}"),
        labels,
        notes,
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Shared helper functions
// ═══════════════════════════════════════════════════════════════════════════════

/// Assign our own global indices to the named region: the first named entry
/// follows the `nulldef_count` NULLDEF entries, then one index per distinct
/// named def in first-seen corpus order (only defs the manifest lists as named
/// for this binary; the first occurrence of a duplicate name wins its slot).
fn collect_named(
    files: &[&ParsedFile],
    allowed_def_types: &HashSet<&str>,
    body_refs: Option<&HashSet<String>>,
    nulldef_count: u32,
) -> (Vec<String>, HashMap<String, u32>) {
    let mut named_order: Vec<String> = Vec::new();
    let mut named_indices: HashMap<String, u32> = HashMap::new();
    for pf in files {
        for d in pf.def_file.definitions() {
            if named_indices.contains_key(d.value.name.as_str()) {
                continue;
            }
            if !allowed_def_types.contains(d.value.def_type.as_str()) {
                continue;
            }
            if d.value.is_template
                && !body_refs
                    .as_ref()
                    .map(|r| r.contains(d.value.name.as_str()))
                    .unwrap_or(true)
            {
                continue;
            }
            let name = d.value.name.as_str();
            named_indices.insert(name.to_string(), nulldef_count + named_order.len() as u32);
            named_order.push(name.to_string());
        }
    }
    (named_order, named_indices)
}

fn build_nulldefs(
    classes: &[&str],
    symbols: &SymbolTable,
    def_indices: &HashMap<String, u32>,
    names: &RefCell<NamesBuilder>,
) -> Result<HashMap<String, DefBody>, String> {
    let mut map: HashMap<String, DefBody> = HashMap::new();
    for &dn in classes {
        if map.contains_key(dn) {
            continue;
        }
        // A NULLDEF body is `def_default()` with no statements applied, so this
        // can only fail if the type is missing from the schema entirely.
        let body = lower_def(dn, None, Body::EMPTY, symbols, def_indices, names)
            .map_err(|e| format!("NULLDEF lowering failed for {dn}: {e}"))?;
        map.insert(dn.to_string(), body);
    }
    Ok(map)
}

/// Allocate the next `ClassIndex` for `class_name` — the third word of the
/// entry's `NameRef`.
///
/// This is **one 0-based counter per class, spanning every emission pass** in
/// global-index order: the NULLDEF first, then named entries, then anonymous
/// sub-def entries. Retail writes it from `CInstantiatedDefInfo::ClassIndex`
/// (`lib_definition_manager.cpp:1581`) and reads it back through
/// `CDefinitionManager::GetDefClassIndexFromGlobalIndex`.
///
/// **`0` is the engine's "no such def" sentinel.** An unset `DefIndex` is `0`,
/// which resolves to whichever NULLDEF sits at global index 0; call sites test
/// the resulting class index against `0` to decide "unset". So the NULLDEF of
/// every class *must* be 0 and named entries *must* start at 1 — otherwise an
/// unset reference reads as class index 1 and silently resolves to the first
/// real def of that class. (This was the local-detail bug: every theme with no
/// `LocalDetailGeneratorDef` resolved to class index 1 —
/// `LOCAL_DETAIL_BRIGHTWOOD_BIRCH_BRACKEN` — and the world editor scattered
/// birch and bracken across every untextured surface.)
fn next_class_index(counters: &mut HashMap<String, u32>, class_name: &str) -> u32 {
    let slot = counters.entry(class_name.to_string()).or_insert(0);
    let index = *slot;
    *slot += 1;
    index
}

/// One named def's tagged blocks, merged across its specialization chain and
/// ordered by tag CRC — the input [`build_subdefs`] lowers.
///
/// Collected during the named pass, off the specialization chain that pass has
/// already resolved: re-deriving them in `build_subdefs` meant walking *and
/// flattening* the chain a second time for every definition (345 ms). The
/// statements are **borrowed** from the parsed ASTs — a merged block is a list
/// of slices, concatenated only when it is about to be lowered, so nothing is
/// retained for the whole build.
type SubDefBlocks<'a> = Vec<(u32, &'a str, MergedBlock<'a>)>;

/// One tag's bodies, in parent-first order, still borrowed from the ASTs.
type MergedBlock<'a> = Vec<&'a [Spanned<Statement>]>;

/// Merge same-tag tagged blocks across a specialization chain, parent-first,
/// ordered by tag CRC.
fn collect_sub_def_blocks<'a>(chain: &[&'a Definition]) -> SubDefBlocks<'a> {
    let mut blocks: HashMap<u32, (&'a str, MergedBlock<'a>)> = HashMap::new();
    for def in chain {
        for st in &def.body {
            if let Statement::TaggedBlock(tb) = &st.value {
                blocks
                    .entry(crc32::crc(tb.tag.as_bytes()))
                    .or_insert_with(|| (tb.tag.as_str(), Vec::new()))
                    .1
                    .push(tb.body.as_slice());
            }
        }
    }
    let mut out: SubDefBlocks<'a> = blocks
        .into_iter()
        .map(|(crc, (tag, bodies))| (crc, tag, bodies))
        .collect();
    out.sort_by_key(|(crc, _, _)| *crc);
    out
}

fn emit_nulldef_and_named<'a>(
    entries: &mut Vec<Built>,
    nulldef_entries: &[&str],
    named_order: &[String],
    ctx: &BuildCtx<'a>,
    class_index: &mut HashMap<String, u32>,
    sub_blocks: &mut Vec<SubDefBlocks<'a>>,
) -> Result<usize, String> {
    for &class_name in nulldef_entries {
        let fnm = format!("NULLDEF_{class_name}");
        let body = ctx
            .nulldefs
            .get(class_name)
            .ok_or_else(|| format!("NULLDEF body missing for {class_name}"))?
            .clone();
        let counter = next_class_index(class_index, class_name);
        let def_name_off = ctx.names.borrow_mut().intern(class_name);
        let file_name_off = ctx.names.borrow_mut().intern(&fnm);
        entries.push(Built {
            def_name_off,
            file_name_off,
            counter,
            preamble: EntryPreamble {
                is_real: false,
                is_template: false,
                unknown_0: 0,
            },
            sub_defs: if def_name_has_subdef_table(class_name) {
                Some(Vec::new())
            } else {
                None
            },
            body,
        });
    }

    let mut n_ok = 0;
    let mut error_count = 0;
    for name in named_order {
        let Some(def) = ctx.defs_by_name.get(name.as_str()) else {
            ctx.diagnostics.push(BuildDiagnostic::bare(
                Severity::Error,
                format!("definition {name} not found in parsed corpus"),
            ));
            error_count += 1;
            continue;
        };
        let def_type = def.def_type.clone();
        let def_name_off = ctx.names.borrow_mut().intern(&def_type);
        let file_name_off = ctx.names.borrow_mut().intern(name);

        let chain = match specialization_chain(def, ctx.defs_by_name) {
            Ok(c) => c,
            Err(e) => {
                ctx.diagnostics
                    .push(lowering_error_diagnostic(name, &def_type, &e, ctx));
                error_count += 1;
                continue;
            }
        };
        // Read the chain in place. Concatenating it produced 7× the corpus's
        // own statements in clones, all of it discarded after lowering.
        let runs = chain_runs(&chain);

        let lowered = match lower_def(
            &def_type,
            ctx.nulldefs.get(def_type.as_str()).as_ref().copied(),
            Body::Chain(&runs),
            ctx.symbols,
            ctx.def_indices,
            ctx.names,
        ) {
            Ok(b) => {
                n_ok += 1;
                b
            }
            Err(e) => {
                ctx.diagnostics
                    .push(lowering_error_diagnostic(name, &def_type, &e, ctx));
                error_count += 1;
                continue;
            }
        };
        let counter = next_class_index(class_index, &def_type);
        let has_sub_defs = def_name_has_subdef_table(&def_type);
        // `build_subdefs` has to run after every named entry exists — sub-def
        // entries follow them in the index space and in the `ClassIndex`
        // counters — so hand it the merged blocks off this chain rather than
        // making it re-resolve and re-flatten every definition.
        sub_blocks.push(if has_sub_defs {
            collect_sub_def_blocks(&chain)
        } else {
            SubDefBlocks::new()
        });
        entries.push(Built {
            def_name_off,
            file_name_off,
            counter,
            preamble: EntryPreamble {
                is_real: true,
                is_template: false,
                unknown_0: 1,
            },
            sub_defs: if has_sub_defs { Some(Vec::new()) } else { None },
            body: lowered,
        });
    }
    if error_count == 0 {
        Ok(n_ok)
    } else {
        Err(format!(
            "{error_count} definition{} failed to compile",
            if error_count == 1 { "" } else { "s" }
        ))
    }
}

fn assemble_and_write(
    entries: Vec<Built>,
    header: &DefBinaryHeader,
    out_path: &Path,
    label: &str,
) -> Result<u32, String> {
    let entry_count = entries.len() as u32;
    let name_refs: Vec<NameRef> = entries
        .iter()
        .map(|e| NameRef {
            def_name_offset: e.def_name_off,
            file_name_offset: e.file_name_off,
            counter: e.counter,
        })
        .collect();
    let records: Vec<EntryRecord> = entries
        .into_iter()
        .map(|e| EntryRecord {
            preamble: e.preamble,
            sub_defs: e.sub_defs,
            chunk_start: 0,
            chunk_end: 0,
            body: e.body,
            raw_bytes: Vec::new(),
        })
        .collect();
    const TARGET: usize = 16384;
    // Find the chunk boundaries first, then consume `records` in one forward
    // pass. Draining the front of the `Vec` per chunk instead is quadratic in a
    // 5 KB `EntryRecord` — it memmoves the whole tail once per chunk, which for
    // game.bin (~13k entries, ~430 chunks) cost 1.8 s, roughly half the build.
    let sizes: Vec<usize> = records.iter().map(|e| e.byte_size()).collect();
    let mut bounds: Vec<usize> = Vec::new();
    let mut sz = 0usize;
    for (i, &size) in sizes.iter().enumerate() {
        // `sz > 0` keeps an entry that alone exceeds TARGET in a chunk of its
        // own rather than looping forever on an impossible split.
        if sz > 0 && sz + size > TARGET {
            bounds.push(i);
            sz = 0;
        }
        sz += size;
    }
    bounds.push(sizes.len());

    let mut chunks = Vec::with_capacity(bounds.len());
    let mut records = records.into_iter();
    let mut entry_base = 0u32;
    for bound in bounds {
        let count = bound - entry_base as usize;
        chunks.push(Chunk::from_entries(
            entry_base,
            records.by_ref().take(count).collect(),
        ));
        entry_base = bound as u32;
    }
    let hdr = DefBinaryHeader {
        use_names_bin: header.use_names_bin,
        file_indicator: header.file_indicator,
        platform_indicator: header.platform_indicator,
        entry_count,
    };
    let binary = DefBinary {
        header: hdr,
        name_refs,
        chunk_index: ChunkIndex {
            header: ChunkIndexHeader {
                chunk_count: chunks.len() as u32 + 1,
                reserved: 0,
            },
            entries: chunks
                .iter()
                .scan(0u32, |cum, c| {
                    *cum += c.entry_count;
                    Some(ChunkIndexEntry {
                        compressed_offset: 0,
                        cumulative_entry_count: *cum,
                    })
                })
                .collect(),
        },
        chunks,
    };
    std::fs::write(out_path, binary.to_bytes()).map_err(|e| format!("write {label}: {e}"))?;
    Ok(entry_count)
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Phase 2: Build one binary
// ═══════════════════════════════════════════════════════════════════════════════

/// Identity of a merged tagged block: its tag plus the identity of each source
/// slice. Two definitions that inherit the same block from the same template
/// borrow the *same* statements, so this catches the duplication that matters —
/// 33,525 sub-defs across the corpus come from ~5,700 distinct inputs — without
/// needing to hash the statements themselves.
type SubDefInput = (u32, Vec<(usize, usize)>);

fn sub_def_input(tag_crc: u32, bodies: &MergedBlock<'_>) -> SubDefInput {
    (
        tag_crc,
        bodies
            .iter()
            .map(|b| (b.as_ptr() as usize, b.len()))
            .collect(),
    )
}

/// Build the anonymous sub-def region (game.bin only).  Lowers each merged
/// tagged block (see [`collect_sub_def_blocks`]), deduplicates anonymous
/// entries by (class-tag, bytes), and appends them to `entries`.
fn build_subdefs(
    named_order: &[String],
    named_base: usize,
    ctx: &BuildCtx,
    entries: &mut Vec<Built>,
    class_index: &mut HashMap<String, u32>,
    sub_blocks: &[SubDefBlocks],
) -> Result<(usize, usize), String> {
    let mut sub_dedup: HashMap<(String, Vec<u8>), u32> = HashMap::new();
    // Identical inputs lower to identical bytes and intern the same (already
    // interned) strings, so reusing the earlier result is indistinguishable
    // from redoing the work — including for `names.bin`, since interning is
    // idempotent.
    let mut input_memo: HashMap<SubDefInput, u32> = HashMap::new();
    let mut sub_entries: Vec<Built> = Vec::new();
    let (mut sub_ok, mut sub_fail) = (0, 0);
    for (oi, name) in named_order.iter().enumerate() {
        let owner_index = (named_base + oi) as u32;
        let blocks = &sub_blocks[oi];
        if blocks.is_empty() {
            continue;
        }
        let mut table: Vec<SubDefRecord> = Vec::new();
        for (k, tag, bodies) in blocks {
            let k = *k;
            let tag = *tag;
            if let Some(&sub_idx) = input_memo.get(&sub_def_input(k, bodies)) {
                sub_ok += 1;
                table.push(SubDefRecord {
                    name_crc: k,
                    def_index: sub_idx,
                    owner_index,
                });
                continue;
            }
            let lowered = match lower_def(
                tag,
                ctx.nulldefs.get(tag).as_ref().copied(),
                Body::Chain(bodies),
                ctx.symbols,
                ctx.def_indices,
                ctx.names,
            ) {
                Ok(b) => {
                    sub_ok += 1;
                    b
                }
                Err(e) => {
                    // As on the named path, the span carries its own file: a
                    // sub-def block can be inherited from a template elsewhere.
                    let labels = e
                        .primary_span()
                        .map(|span| {
                            vec![DiagnosticLabel {
                                source: span.file.0 as usize,
                                primary: true,
                                span,
                                message: None,
                            }]
                        })
                        .unwrap_or_default();
                    ctx.diagnostics.push(BuildDiagnostic {
                        severity: Severity::Error,
                        message: format!("in <{tag}> of {name}: {e}"),
                        labels,
                        notes: Vec::new(),
                    });
                    sub_fail += 1;
                    continue;
                }
            };
            let mut bytes = vec![0u8; lowered.byte_size()];
            {
                let mut o = &mut bytes[..];
                if lowered.serialize(&mut o).is_err() {
                    continue;
                }
            }
            let sub_idx = *sub_dedup
                .entry((tag.to_string(), bytes.clone()))
                .or_insert_with(|| {
                    let idx = sub_entries.len() as u32;
                    let counter = next_class_index(class_index, tag);
                    sub_entries.push(Built {
                        def_name_off: ctx.names.borrow_mut().intern(tag),
                        file_name_off: u32::MAX,
                        counter,
                        preamble: EntryPreamble {
                            is_real: true,
                            is_template: false,
                            unknown_0: 1,
                        },
                        sub_defs: if def_name_has_subdef_table(tag) {
                            Some(Vec::new())
                        } else {
                            None
                        },
                        body: lowered,
                    });
                    idx
                });
            input_memo.insert(sub_def_input(k, bodies), sub_idx);
            table.push(SubDefRecord {
                name_crc: k,
                def_index: sub_idx,
                owner_index,
            });
        }
        entries[named_base + oi].sub_defs = Some(table);
    }
    if sub_fail > 0 {
        return Err(format!(
            "{sub_fail} sub-definition{} failed to compile",
            if sub_fail == 1 { "" } else { "s" }
        ));
    }
    let sub_base = entries.len() as u32;
    for e in &mut entries[named_base..] {
        if let Some(table) = &mut e.sub_defs {
            for rec in table {
                rec.def_index += sub_base;
            }
        }
    }
    let unique = sub_entries.len();
    entries.extend(sub_entries);
    Ok((sub_ok, unique))
}

fn build_one_bin(
    corpus: &ParsedCorpus,
    config: &BinConfig,
    diagnostics: &Diagnostics,
    names: &RefCell<NamesBuilder>,
    out_dir: &Path,
    on_progress: &mut dyn FnMut(Progress),
) -> Result<BinSummary, String> {
    // Scope the corpus to this binary's file set, preserving the original
    // file-processing order (the game walk order for game.bin, the explicit
    // file-list order for frontend.bin, sorted-directory order for script.bin).
    let scoped = (config.file_scope)(&corpus.files);

    // Build the name→definition map from scoped files (last-wins by file order).
    let mut defs_by_name: HashMap<&str, &Definition> = HashMap::new();
    for pf in &scoped {
        for d in pf.def_file.definitions() {
            defs_by_name.insert(d.value.name.as_str(), &d.value);
        }
    }

    let allowed_def_types: HashSet<&str> = config.nulldef_entries.iter().copied().collect();
    let body_refs = if config.filter_templates {
        Some(collect_body_references(&scoped, &defs_by_name))
    } else {
        None
    };
    let nulldef_count = config.nulldef_entries.len() as u32;
    let (named_order, named_indices) = collect_named(
        &scoped,
        &allowed_def_types,
        body_refs.as_ref(),
        nulldef_count,
    );

    let nulldefs = build_nulldefs(
        config.nulldef_entries,
        &corpus.symbols,
        &named_indices,
        names,
    )?;
    let ctx = BuildCtx {
        symbols: &corpus.symbols,
        def_indices: &named_indices,
        names,
        sources: &corpus.sources,
        def_to_source: &corpus.def_to_source,
        def_spans: &corpus.def_spans,
        defs_by_name: &defs_by_name,
        nulldefs: &nulldefs,
        diagnostics,
    };

    on_progress(Progress::Lowering {
        label: config.label,
        named: named_order.len(),
    });
    let mut entries: Vec<Built> = Vec::new();
    // One 0-based `ClassIndex` counter per class, shared across all three
    // emission passes so it runs continuously in global-index order. See
    // `next_class_index`.
    let mut class_index: HashMap<String, u32> = HashMap::new();
    // Filled by the named pass, consumed by `build_subdefs` — parallel to
    // `named_order`, which holds because a failure in the named pass aborts the
    // build before `build_subdefs` runs.
    let mut sub_blocks: Vec<SubDefBlocks> = Vec::with_capacity(named_order.len());
    let lowered = emit_nulldef_and_named(
        &mut entries,
        config.nulldef_entries,
        &named_order,
        &ctx,
        &mut class_index,
        &mut sub_blocks,
    )?;

    let named_base = nulldef_count as usize;
    let (sub_defs_lowered, sub_defs_unique) = if config.has_subdefs {
        build_subdefs(
            &named_order,
            named_base,
            &ctx,
            &mut entries,
            &mut class_index,
            &sub_blocks,
        )?
    } else {
        (0, 0)
    };

    let entries = assemble_and_write(
        entries,
        &config.binary_header,
        &out_dir.join(config.out_filename),
        config.label,
    )?;
    Ok(BinSummary {
        label: config.label,
        file_name: config.out_filename,
        lowered,
        entries,
        has_sub_defs: config.has_subdefs,
        sub_defs_lowered,
        sub_defs_unique,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    /// The engine treats `ClassIndex 0` as "no such def": an unset `DefIndex` is
    /// `0`, resolves to whichever NULLDEF sits at global index 0, and call sites
    /// test the resulting class index against `0`. So each class's NULLDEF must
    /// be `0` and its first real entry must be `1`.
    ///
    /// The bug this pins: three independent per-pass counters, each starting at
    /// `1`, gave the NULLDEF `1` *and* the first named entry `1`. Themes with no
    /// `LocalDetailGeneratorDef` then resolved to class index 1 —
    /// `LOCAL_DETAIL_BRIGHTWOOD_BIRCH_BRACKEN` — instead of "none".
    #[test]
    fn class_index_is_zero_based_and_continuous_across_passes() {
        let mut counters = HashMap::new();
        // NULLDEF pass, then the named pass, then anonymous sub-defs — one
        // continuous run per class, in global-index order.
        assert_eq!(next_class_index(&mut counters, "LOCAL_DETAIL_GENERATOR"), 0);
        assert_eq!(next_class_index(&mut counters, "LOCAL_DETAIL_GENERATOR"), 1);
        assert_eq!(next_class_index(&mut counters, "LOCAL_DETAIL_GENERATOR"), 2);
        // Counters are per class, not global.
        assert_eq!(next_class_index(&mut counters, "ARMOUR"), 0);
        assert_eq!(next_class_index(&mut counters, "LOCAL_DETAIL_GENERATOR"), 3);

        // `CONTROL_SCHEME` has *two* NULLDEF entries in frontend.bin, so they
        // take 0 and 1 and named entries start at 2 — as retail emits them.
        let mut cs = HashMap::new();
        assert_eq!(next_class_index(&mut cs, "CONTROL_SCHEME"), 0);
        assert_eq!(next_class_index(&mut cs, "CONTROL_SCHEME"), 1);
        assert_eq!(next_class_index(&mut cs, "CONTROL_SCHEME"), 2);
    }

    #[test]
    fn path_matching_respects_path_boundaries() {
        // The two real ambiguities in the corpus.
        assert!(path_ends_with("x/Defs/controls.def", "controls.def"));
        assert!(!path_ends_with("x/Defs/pc_controls.def", "controls.def"));
        assert!(path_ends_with(
            "x/Defs/FrontEndDefs/engine.def",
            "FrontEndDefs/engine.def"
        ));
        assert!(!path_ends_with(
            "x/Defs/engine.def",
            "FrontEndDefs/engine.def"
        ));
        // A bare relative path with no leading directory still matches at root.
        assert!(path_ends_with("ui_dialogs.def", "ui_dialogs.def"));
    }

    #[test]
    fn windows_separators_normalize_to_forward_slashes() {
        let path = normalize_path(Path::new(r"C:\Fable\Data\Defs\FrontEndDefs\engine.def"));
        assert_eq!(path, "C:/Fable/Data/Defs/FrontEndDefs/engine.def");
        // The scoping rules are written against `/`, so they now work on paths
        // that came from a Windows filesystem.
        assert!(path_ends_with(&path, "FrontEndDefs/engine.def"));
        assert!(normalize_path(Path::new(r"a\Defs\ScriptDefs\quest.def")).contains("ScriptDefs/"));
        assert!(normalize_path(Path::new(r"a\DevHeaders\xbox\gfx.h")).contains("/xbox/"));
    }

    // ── header-set resolution ────────────────────────────────────────────────

    /// Build a corpus skeleton under a fresh temp dir: each entry is a path
    /// relative to the root, created as an empty file.
    fn corpus(name: &str, files: &[&str]) -> PathBuf {
        let root =
            std::env::temp_dir().join(format!("defc-hdr-test-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        for rel in files {
            let path = root.join(rel);
            std::fs::create_dir_all(path.parent().unwrap()).unwrap();
            std::fs::write(&path, "").unwrap();
        }
        root
    }

    fn selected_headers(root: &Path) -> Vec<String> {
        let diagnostics = Diagnostics::default();
        let set = select_header_set(root, &diagnostics);
        let mut out = Vec::new();
        collect_h_files(root, root, set, &mut out);
        let root_str = normalize_path(root);
        let mut rel: Vec<String> = out
            .iter()
            .map(|p| {
                normalize_path(p)
                    .strip_prefix(&root_str)
                    .unwrap()
                    .trim_start_matches('/')
                    .to_string()
            })
            .collect();
        rel.sort();
        rel
    }

    /// The bug: `Data/Defs` ships two complete variants of the same header set,
    /// and reading both unions two copies of every symbol into one namespace.
    #[test]
    fn retail_header_set_wins_over_dev() {
        let root = corpus(
            "both",
            &[
                "meshdata_shared.h",
                "DevHeaders/meshdata.h",
                "DevHeaders/pc/textures.h",
                "RetailHeaders/meshdata.h",
                "RetailHeaders/pc/textures.h",
            ],
        );
        assert_eq!(
            selected_headers(&root),
            vec![
                "RetailHeaders/meshdata.h",
                "RetailHeaders/pc/textures.h",
                // Headers outside every variant root are shared, never skipped.
                "meshdata_shared.h",
            ]
        );
        let _ = std::fs::remove_dir_all(&root);
    }

    /// The Anniversary debug corpus ships only `DevHeaders/`; selection must be
    /// a no-op there or the golden build changes.
    #[test]
    fn lone_dev_header_set_is_used() {
        let root = corpus("devonly", &["action_def.h", "DevHeaders/meshdata.h"]);
        assert_eq!(
            selected_headers(&root),
            vec!["DevHeaders/meshdata.h", "action_def.h"]
        );
        let _ = std::fs::remove_dir_all(&root);
    }

    /// Platform variants are the same problem one level down.
    #[test]
    fn xbox_platform_variant_is_skipped() {
        let root = corpus(
            "xbox",
            &["DevHeaders/pc/textures.h", "DevHeaders/xbox/textures.h"],
        );
        assert_eq!(selected_headers(&root), vec!["DevHeaders/pc/textures.h"]);
        let _ = std::fs::remove_dir_all(&root);
    }

    /// A corpus with no variant roots at all keeps the plain recursive scan.
    #[test]
    fn corpus_without_variant_roots_reads_everything() {
        let root = corpus("flat", &["a.h", "sub/b.h"]);
        assert_eq!(selected_headers(&root), vec!["a.h", "sub/b.h"]);
        let _ = std::fs::remove_dir_all(&root);
    }

    /// Variant matching is relative to the corpus root. Matching the absolute
    /// path would make the result depend on where the corpus lives: a corpus
    /// stored under a directory called `xbox` or `DevHeaders` would have every
    /// header skipped and produce symbol-less binaries.
    #[test]
    fn variant_matching_ignores_directories_above_the_corpus_root() {
        let outer = corpus("nested", &["xbox/DevHeaders/Defs/action_def.h"]);
        let inner = outer.join("xbox/DevHeaders/Defs");
        assert_eq!(selected_headers(&inner), vec!["action_def.h"]);
        let _ = std::fs::remove_dir_all(&outer);
    }

    /// Skipping a set silently is what made the original bug so hard to see:
    /// a mod patched the ignored set and its symbols vanished with no signal.
    #[test]
    fn ignored_header_set_is_reported() {
        let root = corpus(
            "warn",
            &["DevHeaders/meshdata.h", "RetailHeaders/meshdata.h"],
        );
        let diagnostics = Diagnostics::default();
        select_header_set(&root, &diagnostics);
        let diags = diagnostics.take();
        assert_eq!(diags.len(), 1);
        assert_eq!(diags[0].severity, Severity::Warning);
        assert!(diags[0].message.contains("DevHeaders/"), "{:?}", diags[0]);
        let _ = std::fs::remove_dir_all(&root);
    }

    /// One set present means nothing is being ignored, so nothing to report.
    #[test]
    fn single_header_set_reports_nothing() {
        let root = corpus("quiet", &["DevHeaders/meshdata.h"]);
        let diagnostics = Diagnostics::default();
        select_header_set(&root, &diagnostics);
        assert!(diagnostics.take().is_empty());
        let _ = std::fs::remove_dir_all(&root);
    }

    /// A diagnostic's `source` is an index into `BuildReport::sources`, and a
    /// span's `FileId` is that same index — so a file's id must depend only on
    /// its position in the sorted walk, never on which thread parsed it first.
    ///
    /// Reading and parsing run in parallel (`parallel_map`), so this is the
    /// property that would break silently: the build would still succeed, and
    /// the error would just point at a different file's text. Golden cannot see
    /// it, because a corpus that compiles produces no diagnostics at all.
    #[test]
    fn parse_error_points_at_the_file_it_came_from() {
        let root = std::env::temp_dir().join(format!("defc-sid-test-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        std::fs::create_dir_all(&root).unwrap();
        // Padding either side so the broken file is neither first nor last, and
        // so it is not the largest — largest-first scheduling parses it early.
        for name in ["a_pad.def", "m_broken.def", "z_pad.def"] {
            let body = if name == "m_broken.def" {
                "#definition OBJECT BROKEN\n  Health ((;\n#end_definition\n".to_string()
            } else {
                format!("{}\n", "// filler comment\n".repeat(500))
            };
            std::fs::write(root.join(name), body).unwrap();
        }

        let diagnostics = Diagnostics::default();
        let corpus = parse_corpus(&root, &diagnostics, &mut |_| {}).expect("corpus parses");
        let diags = diagnostics.take();

        let broken = diags
            .iter()
            .find(|d| d.severity == Severity::Error)
            .expect("the malformed definition is reported");
        let sid = broken.labels.first().expect("error is located").source;
        assert!(
            corpus.sources[sid].path.ends_with("m_broken.def"),
            "diagnostic points at {}, not m_broken.def",
            corpus.sources[sid].path
        );
        // The span's own file id has to agree with the label's source index.
        assert_eq!(broken.labels[0].span.file.0 as usize, sid);
        let _ = std::fs::remove_dir_all(&root);
    }
}
