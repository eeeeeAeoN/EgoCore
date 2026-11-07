use std::path::PathBuf;
use std::process::ExitCode;

use argh::FromArgs;

#[derive(FromArgs)]
/// Fable def decompiler and binary differ.
struct CliArgs {
    /// path to directory containing original retail binaries (names.bin, game.bin, etc.)
    #[argh(positional)]
    orig_dir: PathBuf,

    /// path to directory containing modded binaries (names.bin, game.bin, etc.)
    #[argh(positional)]
    mod_dir: PathBuf,

    /// path to directory containing base source def files (Fable/Data/Defs)
    #[argh(positional)]
    target_defs_dir: PathBuf,

    /// output .def file path (defaults to target directory routing: <target_defs_dir>/[ScriptDefs/]z_mod_decompiled.def)
    #[argh(option, short = 'o')]
    out: Option<PathBuf>,
}

fn main() -> ExitCode {
    let args: CliArgs = argh::from_env();

    eprintln!(
        "Diffing mod binaries at '{}' against original at '{}'...",
        args.mod_dir.display(),
        args.orig_dir.display()
    );

    match def_decompiler::run_diff(&args.orig_dir, &args.mod_dir, &args.target_defs_dir) {
        Ok(output) => {
            let total_modified: usize = output
                .results
                .iter()
                .map(|r| r.modified_definitions.len())
                .sum();
            let total_added: usize = output
                .results
                .iter()
                .map(|r| r.added_definitions.len())
                .sum();
            for res in &output.results {
                eprintln!(
                    "[{}] modified: {} (patched in-place), descendants (skipped): {}, newly added: {} (in z_mod_decompiled.def)",
                    res.binary_name,
                    res.modified_definitions.len(),
                    res.descendant_count,
                    res.added_definitions.len(),
                );
            }
            if let Some(out_path) = args.out {
                if let Err(e) = output.write_to_file(&out_path) {
                    eprintln!("Error writing output file '{}': {e}", out_path.display());
                    return ExitCode::FAILURE;
                }
                eprintln!(
                    "Successfully wrote {} definitions to '{}'",
                    total_modified + total_added,
                    out_path.display()
                );
            } else {
                if let Err(e) = output.write_to_target_dir(&args.target_defs_dir) {
                    eprintln!(
                        "Error writing output to target dir '{}': {e}",
                        args.target_defs_dir.display()
                    );
                    return ExitCode::FAILURE;
                }
                eprintln!(
                    "Successfully patched {total_modified} modified definitions and wrote {total_added} added definitions to '{}'",
                    args.target_defs_dir.display()
                );
            }
            ExitCode::SUCCESS
        }
        Err(e) => {
            eprintln!("Diff error: {e}");
            ExitCode::FAILURE
        }
    }
}
