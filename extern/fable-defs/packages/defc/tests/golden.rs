//! Golden-output determinism gate.
//!
//! The from-scratch build is a pure function of the text corpus + manifest, so
//! its output must be **byte-identical** across runs and across refactors that
//! are meant to preserve behavior. This test rebuilds all four binaries and
//! asserts each one's hash matches the committed manifest — the reliable
//! regression gate for the improvement/simplification phase (a pure refactor
//! that changes a single output byte fails here immediately).
//!
//! Requires the text corpus; set `OA_TEXT_DIR` to the Defs/ directory. When it
//! is unset the test skips, so CI without the proprietary assets stays green.
//!
//!   OA_TEXT_DIR=~/doc/Fable_Anniversary-2013-02-25/Fable/Data/Defs cargo test -p defc --test golden
//!
//! To intentionally re-bless the manifest after an accepted output change:
//!   OA_BLESS=1 OA_TEXT_DIR=... cargo test -p defc --test golden

use std::path::Path;
use std::process::Command;

const FILES: [&str; 4] = ["game.bin", "frontend.bin", "script.bin", "names.bin"];
const MANIFEST: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/tests/golden_manifest.txt");

/// FNV-1a 64-bit — deterministic, dependency-free; any byte change flips it.
fn fnv1a(bytes: &[u8]) -> u64 {
    let mut h: u64 = 0xcbf29ce484222325;
    for &b in bytes {
        h ^= b as u64;
        h = h.wrapping_mul(0x100000001b3);
    }
    h
}

fn read_manifest() -> std::collections::HashMap<String, u64> {
    std::fs::read_to_string(MANIFEST)
        .unwrap_or_default()
        .lines()
        .filter_map(|l| {
            let (name, hash) = l.split_once(char::is_whitespace)?;
            Some((name.to_string(), u64::from_str_radix(hash.trim(), 16).ok()?))
        })
        .collect()
}

#[test]
fn golden_output_is_stable() {
    let Ok(text_dir) = std::env::var("OA_TEXT_DIR") else {
        eprintln!("SKIP golden_output_is_stable: set OA_TEXT_DIR to run");
        return;
    };
    let out = std::env::temp_dir().join("oa_golden_test");
    let _ = std::fs::remove_dir_all(&out);

    let status = Command::new(env!("CARGO_BIN_EXE_defc"))
        .args(["-i", text_dir.as_str(), "-o", out.to_str().unwrap()])
        .status()
        .expect("run defc");
    assert!(status.success(), "defc build failed");

    let actual: Vec<(String, u64)> = FILES
        .iter()
        .map(|f| {
            let bytes = std::fs::read(out.join(f)).unwrap_or_else(|_| panic!("read {f}"));
            (f.to_string(), fnv1a(&bytes))
        })
        .collect();

    if std::env::var("OA_BLESS").is_ok() {
        let body: String = actual
            .iter()
            .map(|(f, h)| format!("{f} {h:016x}\n"))
            .collect();
        std::fs::write(MANIFEST, body).expect("write manifest");
        eprintln!(
            "BLESSED golden manifest at {}",
            Path::new(MANIFEST).display()
        );
        return;
    }

    let expected = read_manifest();
    assert!(
        !expected.is_empty(),
        "golden manifest missing; run once with OA_BLESS=1"
    );
    let mut failures = Vec::new();
    for (f, h) in &actual {
        match expected.get(f) {
            Some(e) if e == h => {}
            Some(e) => failures.push(format!("{f}: got {h:016x}, expected {e:016x}")),
            None => failures.push(format!("{f}: absent from manifest")),
        }
    }
    assert!(
        failures.is_empty(),
        "output diverged from golden:\n  {}",
        failures.join("\n  ")
    );
}
