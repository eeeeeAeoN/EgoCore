//! C ABI for [`def_compiler::build`] — one call in, a status code and a log
//! string out.
//!
//! The whole surface is [`defc_build`]; see `include/def_compiler.h` for the
//! header this crate is contracted to.
//!
//! Design notes:
//!
//! - **Diagnostics are rendered to plain text here.** The Rust API returns
//!   structured diagnostics with byte spans, but a C consumer that just wants to
//!   show the user what went wrong would have to re-implement span→line
//!   resolution to use them. So this layer renders `path:line:col: severity:
//!   message` lines and hands over one string. Structured access is a
//!   deliberate non-goal of this ABI.
//! - **Panics are caught.** Unwinding out of an `extern "C"` function is
//!   undefined behaviour, so every entry point wraps its body in
//!   [`catch_unwind`] and reports [`DEFC_ERROR_PANIC`] instead.
//! - **Paths are UTF-8.** `const char*` inputs are decoded as UTF-8, which is
//!   correct for ASCII paths on every platform but *not* for a non-ASCII path
//!   that reached the caller as a Windows ANSI string. Such a path is rejected
//!   with [`DEFC_ERROR_ARGS`] rather than silently mis-resolved.

use std::ffi::{CStr, c_char};
use std::fmt::Write as _;
use std::panic::catch_unwind;
use std::path::Path;

use codespan_reporting::diagnostic::{Diagnostic, Label};
use codespan_reporting::files::SimpleFiles;
use codespan_reporting::term::termcolor::NoColor;
use codespan_reporting::term::{self, Styles, StylesWriter};
use def_compiler::{BuildDiagnostic, Severity, SourceFile};

/// The build succeeded. The log holds any warnings, then the summary.
pub const DEFC_OK: i32 = 0;
/// The build failed. The log holds the errors and the reason.
pub const DEFC_ERROR_BUILD: i32 = 1;
/// A pointer argument was null, or a path was not valid UTF-8.
pub const DEFC_ERROR_ARGS: i32 = 2;
/// The compiler panicked — a bug. The log holds the panic message.
pub const DEFC_ERROR_PANIC: i32 = 3;

/// Compile the text def corpus in `input_dir` into the four binaries in
/// `output_dir`.
///
/// `input_dir` is the `Defs/` directory; `output_dir` receives `game.bin`,
/// `frontend.bin`, `script.bin`, and `names.bin`, and is created if missing.
///
/// The human-readable log is written NUL-terminated into `log_buf`, truncated at
/// a UTF-8 boundary to fit `log_cap`. `log_len` (when non-null) receives the
/// log's **full** length in bytes excluding the NUL, so a caller can size a
/// buffer exactly and call again. Passing a null `log_buf` or a `log_cap` of 0
/// still runs the build and still reports `log_len`.
///
/// # Safety
///
/// `input_dir` and `output_dir` must be valid NUL-terminated C strings.
/// `log_buf`, when non-null, must point to at least `log_cap` writable bytes,
/// and `log_len`, when non-null, must point to a writable `size_t`. None of the
/// pointers may alias.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn defc_build(
    input_dir: *const c_char,
    output_dir: *const c_char,
    log_buf: *mut c_char,
    log_cap: usize,
    log_len: *mut usize,
) -> i32 {
    let mut log = String::new();

    let status = match catch_unwind(|| {
        let input = unsafe { cstr_to_path(input_dir) }?;
        let output = unsafe { cstr_to_path(output_dir) }?;
        Ok(compile(Path::new(input), Path::new(output)))
    }) {
        Ok(Ok((status, rendered))) => {
            log = rendered;
            status
        }
        Ok(Err(())) => {
            log.push_str("error: input and output paths must be non-null UTF-8\n");
            DEFC_ERROR_ARGS
        }
        Err(payload) => {
            let _ = writeln!(
                log,
                "error: def compiler panicked: {}",
                panic_message(&payload)
            );
            DEFC_ERROR_PANIC
        }
    };

    unsafe { write_log(&log, log_buf, log_cap, log_len) };
    status
}

/// Version of the compiler this library was built from, as a static
/// NUL-terminated string.
#[unsafe(no_mangle)]
pub extern "C" fn defc_version() -> *const c_char {
    concat!(env!("CARGO_PKG_VERSION"), "\0").as_ptr() as *const c_char
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Implementation
// ═══════════════════════════════════════════════════════════════════════════════

/// Cap on rendered diagnostics. A corpus-wide mistake can produce thousands;
/// past this point the log stops being something a human reads and starts being
/// a memory problem for the caller's buffer.
const MAX_RENDERED: usize = 100;

/// Run the build and render its outcome. Split out from the `extern "C"` shell
/// so it is ordinary safe Rust and directly testable.
fn compile(input: &Path, output: &Path) -> (i32, String) {
    let mut log = String::new();
    match def_compiler::build(input, output) {
        Ok(report) => {
            render_diagnostics(&mut log, &report.sources, &report.diagnostics, input);
            let _ = writeln!(
                log,
                "compiled {} in {:.1}s",
                report
                    .bins
                    .iter()
                    .map(|b| format!("{} ({} entries)", b.file_name, b.entries))
                    .collect::<Vec<_>>()
                    .join(", "),
                report.elapsed.as_secs_f64(),
            );
            (DEFC_OK, log)
        }
        Err(error) => {
            render_diagnostics(&mut log, &error.sources, &error.diagnostics, input);
            let _ = writeln!(log, "error: {}", error.message);
            (DEFC_ERROR_BUILD, log)
        }
    }
}

/// Render diagnostics the way `defc` does — message first, then
/// `--> path:line:col`, then the offending source line with a caret under the
/// span. Keeping the path on its own line matters for a GUI: a long absolute
/// path no longer pushes the message out of view.
fn render_diagnostics(
    log: &mut String,
    sources: &[SourceFile],
    diagnostics: &[BuildDiagnostic],
    input: &Path,
) {
    if diagnostics.is_empty() {
        return;
    }

    // Show paths relative to the corpus root; the absolute prefix is noise the
    // reader already knows.
    let root = normalize_separators(&input.to_string_lossy());
    let mut files: SimpleFiles<&str, &str> = SimpleFiles::new();
    for source in sources {
        files.add(strip_root(&source.path, &root), source.text.as_str());
    }

    let config = term::Config::default();
    let styles = Styles::default();
    let mut buffer: Vec<u8> = Vec::new();

    for diag in diagnostics.iter().take(MAX_RENDERED) {
        let mut rendered = match diag.severity {
            Severity::Warning => Diagnostic::warning(),
            Severity::Error => Diagnostic::error(),
        }
        .with_message(&diag.message);
        if !diag.notes.is_empty() {
            rendered = rendered.with_notes(diag.notes.clone());
        }

        // Each label names its own file, so one diagnostic can span several —
        // a template and the definitions that inherit from it.
        rendered = rendered.with_labels(
            diag.labels
                .iter()
                .map(|l| {
                    let range = l.span.start..l.span.end;
                    let label = if l.primary {
                        Label::primary(l.source, range)
                    } else {
                        Label::secondary(l.source, range)
                    };
                    match &l.message {
                        Some(m) => label.with_message(m),
                        None => label,
                    }
                })
                .collect(),
        );

        // NoColor: the caller is a GUI text box, not a terminal.
        let _ = term::emit_to_write_style(
            &mut StylesWriter::new(NoColor::new(&mut buffer), &styles),
            &config,
            &files,
            &rendered,
        );
    }

    log.push_str(&String::from_utf8_lossy(&buffer));

    if let Some(extra) = diagnostics
        .len()
        .checked_sub(MAX_RENDERED)
        .filter(|n| *n > 0)
    {
        let _ = writeln!(log, "... and {extra} more diagnostic(s) not shown");
    }
}

/// `\` → `/`, and drop any trailing slash, so prefixes compare cleanly.
fn normalize_separators(path: &str) -> String {
    let normalized = path.replace('\\', "/");
    normalized.trim_end_matches('/').to_string()
}

/// Present `<root>/FrontEndDefs/x.def` as `FrontEndDefs/x.def`. Falls back to
/// the full path when it lies outside the corpus root.
fn strip_root<'a>(path: &'a str, root: &str) -> &'a str {
    if root.is_empty() {
        return path;
    }
    path.strip_prefix(root)
        .map(|rest| rest.trim_start_matches('/'))
        .filter(|rest| !rest.is_empty())
        .unwrap_or(path)
}

/// Decode a C string as a UTF-8 path, or `Err(())` if null / not UTF-8.
///
/// # Safety
/// `ptr`, when non-null, must be a valid NUL-terminated C string.
unsafe fn cstr_to_path(ptr: *const c_char) -> Result<&'static str, ()> {
    if ptr.is_null() {
        return Err(());
    }
    unsafe { CStr::from_ptr(ptr) }.to_str().map_err(|_| ())
}

/// Copy `log` into the caller's buffer, NUL-terminated and truncated at a UTF-8
/// character boundary, and report the full untruncated byte length.
///
/// # Safety
/// `buf`, when non-null, must point to `cap` writable bytes; `len`, when
/// non-null, must point to a writable `usize`.
unsafe fn write_log(log: &str, buf: *mut c_char, cap: usize, len: *mut usize) {
    if !len.is_null() {
        unsafe { *len = log.len() };
    }
    if buf.is_null() || cap == 0 {
        return;
    }
    // Reserve one byte for the terminator, then back off to a char boundary so
    // the C string is never cut through the middle of a multi-byte character.
    let mut keep = log.len().min(cap - 1);
    while keep > 0 && !log.is_char_boundary(keep) {
        keep -= 1;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(log.as_ptr(), buf as *mut u8, keep);
        *buf.add(keep) = 0;
    }
}

fn panic_message(payload: &Box<dyn std::any::Any + Send>) -> String {
    if let Some(s) = payload.downcast_ref::<&str>() {
        (*s).to_string()
    } else if let Some(s) = payload.downcast_ref::<String>() {
        s.clone()
    } else {
        "unknown panic".to_string()
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Tests
// ═══════════════════════════════════════════════════════════════════════════════

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    /// Call `defc_build` twice the way a C caller sizing its buffer would:
    /// once to learn the length, once to fill it.
    fn call(input: &str, output: &str, cap: usize) -> (i32, String, usize) {
        let input = CString::new(input).unwrap();
        let output = CString::new(output).unwrap();
        let mut buf = vec![0u8; cap];
        let mut len = 0usize;
        let status = unsafe {
            defc_build(
                input.as_ptr(),
                output.as_ptr(),
                if cap == 0 {
                    std::ptr::null_mut()
                } else {
                    buf.as_mut_ptr() as *mut c_char
                },
                cap,
                &mut len,
            )
        };
        let text = CStr::from_bytes_until_nul(&buf)
            .map(|c| c.to_string_lossy().into_owned())
            .unwrap_or_default();
        (status, text, len)
    }

    #[test]
    fn null_paths_are_rejected_not_dereferenced() {
        let mut len = 0usize;
        let status = unsafe {
            defc_build(
                std::ptr::null(),
                std::ptr::null(),
                std::ptr::null_mut(),
                0,
                &mut len,
            )
        };
        assert_eq!(status, DEFC_ERROR_ARGS);
        assert!(len > 0, "a rejected call still reports a log length");
    }

    #[test]
    fn missing_input_directory_fails_with_a_log() {
        let out = std::env::temp_dir().join("defc_sys_missing_input");
        let (status, log, len) = call("/nonexistent/Defs", out.to_str().unwrap(), 4096);
        assert_eq!(status, DEFC_ERROR_BUILD);
        assert!(len > 0 && log.contains("error:"), "log was {log:?}");
        let _ = std::fs::remove_dir_all(&out);
    }

    #[test]
    fn log_truncates_at_a_char_boundary_and_reports_full_length() {
        // A short buffer must still produce a valid NUL-terminated C string.
        let out = std::env::temp_dir().join("defc_sys_truncate");
        let (_, log, len) = call("/nonexistent/Defs", out.to_str().unwrap(), 8);
        assert!(log.len() <= 7, "truncated to fit: {log:?}");
        assert!(len > log.len(), "full length {len} exceeds truncated");
        let _ = std::fs::remove_dir_all(&out);
    }

    /// The GUI shows this log verbatim, so its shape is part of the contract:
    /// message first, then a short relative path, then a source excerpt with a
    /// caret. A long absolute path on the message line was the original problem.
    #[test]
    fn a_bad_def_renders_message_first_with_a_relative_path_and_caret() {
        let dir = std::env::temp_dir().join("defc_sys_diag_format");
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(dir.join("FrontEndDefs")).unwrap();
        std::fs::write(
            dir.join("FrontEndDefs/broken.def"),
            "#definition UI SOME_UI\n    Material 12 34;\n#end_definition\n",
        )
        .unwrap();

        let out = std::env::temp_dir().join("defc_sys_diag_format_out");
        let (status, log, _) = call(dir.to_str().unwrap(), out.to_str().unwrap(), 8192);

        assert_eq!(
            status, DEFC_ERROR_BUILD,
            "a parse error must fail the build"
        );
        // The message leads, so it is visible without scrolling.
        assert!(log.starts_with("error: "), "log was:\n{log}");
        // The path is relative to the corpus root, on its own line.
        assert!(
            log.contains("FrontEndDefs/broken.def:"),
            "expected a root-relative path, log was:\n{log}"
        );
        assert!(
            !log.contains(dir.to_str().unwrap()),
            "the absolute root should not appear, log was:\n{log}"
        );
        // The source excerpt and caret are what make it readable.
        assert!(log.contains("Material 12 34;"), "log was:\n{log}");
        assert!(log.contains('^'), "log was:\n{log}");

        let _ = std::fs::remove_dir_all(&dir);
        let _ = std::fs::remove_dir_all(&out);
    }

    #[test]
    fn zero_capacity_reports_length_without_writing() {
        let out = std::env::temp_dir().join("defc_sys_zero_cap");
        let (status, _, len) = call("/nonexistent/Defs", out.to_str().unwrap(), 0);
        assert_eq!(status, DEFC_ERROR_BUILD);
        assert!(len > 0, "length is reported even with no buffer");
        let _ = std::fs::remove_dir_all(&out);
    }
}
