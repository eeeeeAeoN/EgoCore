use std::path::PathBuf;

use codespan_reporting::diagnostic::{Diagnostic, Label};
use codespan_reporting::files::SimpleFiles;
use codespan_reporting::term::{self, Styles, StylesWriter, termcolor::StandardStream};
use def_compiler::{BuildDiagnostic, Progress, Severity, SourceFile};

/// Fable def compiler
#[derive(argh::FromArgs)]
struct Args {
    /// input directory of .def, .tpl, and .h files
    #[argh(option, short = 'i')]
    input: Option<PathBuf>,

    /// output directory for .bin files
    #[argh(option, short = 'o')]
    output: Option<PathBuf>,

    /// print version
    #[argh(switch)]
    version: bool,
}

/// Mirror the library's source list into a `codespan-reporting` file store so
/// diagnostic spans render with source excerpts.
fn file_store(sources: &[SourceFile]) -> SimpleFiles<&str, &str> {
    let mut files = SimpleFiles::new();
    for source in sources {
        files.add(source.path.as_str(), source.text.as_str());
    }
    files
}

fn render(files: &SimpleFiles<&str, &str>, diagnostics: &[BuildDiagnostic]) {
    let writer = StandardStream::stderr(termcolor::ColorChoice::Auto);
    let config = term::Config::default();
    let styles = Styles::default();
    for diag in diagnostics {
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
        let _ = term::emit_to_write_style(
            &mut StylesWriter::new(writer.lock(), &styles),
            &config,
            files,
            &rendered,
        );
    }
}

fn main() {
    let args: Args = argh::from_env();

    if args.version {
        println!("defc {}", env!("DEFC_VERSION"));
        return;
    }

    let mut missing = Vec::new();
    if args.input.is_none() {
        missing.push("--input / -i");
    }
    if args.output.is_none() {
        missing.push("--output / -o");
    }
    if !missing.is_empty() {
        eprintln!("Required options not provided:");
        for m in &missing {
            eprintln!("    {m}");
        }
        eprintln!("\nRun defc --help for more information.");
        std::process::exit(1);
    }
    let input = args.input.unwrap();
    let output = args.output.unwrap();

    let mut on_progress = |event: Progress| match event {
        Progress::FileParsed { path, definitions } => {
            eprintln!("    {path} ({definitions} definitions)");
        }
        Progress::CompileStarted => eprintln!("  compiling..."),
        Progress::Lowering { label: _, named } => {
            eprintln!("    lowering {named} named definitions...");
        }
        Progress::BinFinished(bin) => {
            if bin.has_sub_defs {
                eprintln!(
                    "  {}: {} lowered, sub-defs: {} ok/{} unique, {} entries",
                    bin.label, bin.lowered, bin.sub_defs_lowered, bin.sub_defs_unique, bin.entries,
                );
            } else {
                eprintln!(
                    "  {}: {} lowered, {} entries",
                    bin.label, bin.lowered, bin.entries
                );
            }
        }
    };

    match def_compiler::build_with_progress(&input, &output, &mut on_progress) {
        Ok(report) => {
            render(&file_store(&report.sources), &report.diagnostics);
            let summary: Vec<String> = report
                .bins
                .iter()
                .map(|b| format!("{}: {} entries", b.file_name, b.entries))
                .collect();
            eprintln!(
                "  finished in {:.1}s — {}",
                report.elapsed.as_secs_f64(),
                summary.join(", "),
            );
        }
        Err(e) => {
            render(&file_store(&e.sources), &e.diagnostics);
            eprintln!("error: {e}");
            std::process::exit(1);
        }
    }
}
