//! Def compilation: text [`Definition`]s → typed binary def structs → the four
//! compiled-def binaries.
//!
//! Depends on `defs` for the lower-level text parsing/symbol evaluation
//! and the binary def structs. This crate owns the [`DefReader`] (which scans a
//! definition body by field name), the lowering layer, and the assembly
//! pipeline.
//!
//! The top-level entry point is [`build`]:
//!
//! ```no_run
//! let report = def_compiler::build(
//!     std::path::Path::new("Fable/Data/Defs"),
//!     std::path::Path::new("Fable/Data/CompiledDefs"),
//! )?;
//! for warning in report.warnings() {
//!     eprintln!("{}", warning.message);
//! }
//! # Ok::<_, def_compiler::BuildError>(())
//! ```
//!
//! Diagnostics are returned, not printed — see [`BuildReport`]. Rendering them
//! (with source excerpts, colour, line numbers, or however the caller likes) is
//! the caller's job; `defc` is the reference renderer.
//!
//! [`Definition`]: defs::text::Definition

pub mod build;
pub mod lower;
pub mod manifest;
pub mod reader;

pub use self::build::{
    BinSummary, BuildDiagnostic, BuildError, BuildReport, DiagnosticLabel, Progress, Severity,
    SourceFile, build, build_with_progress,
};

pub use self::reader::{Args, Body, DefReader, DefReaderError, EvalError, Evaluator};

pub use self::lower::{LowerError, chain_runs, lower_def, specialization_chain};

use std::path::{Path, PathBuf};

pub fn walk_def_files(dir: &Path) -> Vec<PathBuf> {
    let mut out = Vec::new();
    collect(dir, &mut out);
    out.sort();
    out
}

fn collect(dir: &Path, out: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.filter_map(|e| e.ok()) {
        let path = entry.path();
        if path.is_dir() {
            collect(&path, out);
        } else if matches!(
            path.extension().and_then(|e| e.to_str()),
            Some("def") | Some("tpl")
        ) {
            out.push(path);
        }
    }
}
