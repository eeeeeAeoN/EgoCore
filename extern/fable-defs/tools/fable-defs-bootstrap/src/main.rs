//! CLI: generate (or check) the cleaned C++ def header.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitCode};

use fable_defs_bootstrap::generate_from_tree;

fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("fable-defs-bootstrap: {message}");
            ExitCode::FAILURE
        }
    }
}

fn run() -> Result<(), String> {
    let mut args = env::args().skip(1);
    let mut tree_dir = None;
    let mut decomp_cli = None;
    let mut store = None;
    let mut output = None;
    let mut check = false;
    let mut stdout = false;

    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--tree" | "-t" => {
                tree_dir = Some(PathBuf::from(
                    args.next().ok_or("--tree needs a directory")?,
                ));
            }
            "--decomp-cli" => {
                decomp_cli = Some(PathBuf::from(
                    args.next().ok_or("--decomp-cli needs a path")?,
                ));
            }
            "--store" => {
                store = Some(PathBuf::from(args.next().ok_or("--store needs a path")?));
            }
            "--output" | "-o" => {
                output = Some(PathBuf::from(args.next().ok_or("--output needs a file")?));
            }
            "--check" => check = true,
            "--stdout" => stdout = true,
            "--help" | "-h" => {
                println!("{USAGE}");
                return Ok(());
            }
            other => return Err(format!("unknown argument `{other}`\n\n{USAGE}")),
        }
    }

    let package = Path::new(env!("CARGO_MANIFEST_DIR"));
    let output = output.unwrap_or_else(|| package.join("out/fable_defs.hpp"));

    let (tree_dir, temporary) = match tree_dir {
        Some(dir) => (dir, false),
        None => match env::var_os("FABLE_DECOMP_TREE").map(PathBuf::from) {
            Some(dir) => (dir, false),
            None => {
                let cli = decomp_cli
                    .or_else(|| env::var_os("FABLE_DECOMP_CLI").map(PathBuf::from))
                    .ok_or(
                        "no decompiled tree: pass --tree DIR (a `decomp-cli tree` output), \
                         set FABLE_DECOMP_TREE, or pass --decomp-cli PATH",
                    )?;
                let dir =
                    env::temp_dir().join(format!("fable-defs-bootstrap-tree-{}", std::process::id()));
                fs::create_dir_all(&dir).map_err(|e| format!("{}: {e}", dir.display()))?;
                let mut command = Command::new(&cli);
                command.args(["tree", "--out"]).arg(&dir);
                if let Some(store) = &store {
                    command.arg("--store").arg(store);
                }
                let status = command
                    .status()
                    .map_err(|e| format!("{}: {e}", cli.display()))?;
                if !status.success() {
                    return Err(format!("{} tree failed ({status})", cli.display()));
                }
                (dir, true)
            }
        },
    };

    let (header, stats) = generate_from_tree(&tree_dir).map_err(|e| e.to_string())?;
    if temporary {
        let _ = fs::remove_dir_all(&tree_dir);
    }

    if stdout {
        print!("{header}");
        return Ok(());
    }

    let summary = format!(
        "{} def classes, {} emitted classes ({} nested), {} enums, {} data members",
        stats.def_classes, stats.classes, stats.nested, stats.enums, stats.members
    );

    if check {
        let current =
            fs::read_to_string(&output).map_err(|e| format!("{}: {e}", output.display()))?;
        if current == header {
            eprintln!("{} is up to date ({summary})", output.display());
            return Ok(());
        }
        return Err(format!(
            "{} is stale; run `cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml`",
            output.display()
        ));
    }

    if let Some(parent) = output.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("{}: {e}", parent.display()))?;
    }
    fs::write(&output, header).map_err(|e| format!("{}: {e}", output.display()))?;
    eprintln!("wrote {} ({summary})", output.display());
    Ok(())
}

const USAGE: &str = "\
Usage: fable-defs-bootstrap [OPTIONS]

Generate a cleaned C++ header from Fable's decompiled def class declarations.

Options:
  -t, --tree <DIR>        A `decomp-cli tree` output directory
      --decomp-cli <PATH> Run this decomp-cli to produce a tree when --tree is absent
      --store <PATH>      Pass a store to `decomp-cli tree`
  -o, --output <FILE>     Output header (default: out/fable_defs.hpp)
      --check             Verify the output is current; write nothing
      --stdout            Print the header instead of writing a file
  -h, --help              Show this help

Environment:
  FABLE_DECOMP_TREE       Default for --tree
  FABLE_DECOMP_CLI        Default for --decomp-cli

Regenerate the checked-in header with:
  cargo run --manifest-path tools/fable-defs-bootstrap/Cargo.toml -- --decomp-cli ~/git/fable-decomp-engine/target/release/decomp-cli
";
