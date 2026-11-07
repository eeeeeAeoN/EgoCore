//! Scratch: compare the NameRef `ClassIndex` word between two builds, matched by name.
//!
//!   probe_counter_diff <ours> <retail>   (BIN=game|frontend|script)
use defs::binary::DefBinary;
use defs::names::Names;
use std::collections::BTreeMap;
use std::path::Path;

fn map(dir: &str, bin: &str) -> BTreeMap<String, u32> {
    let names = Names::load(&Path::new(dir).join("names.bin")).unwrap();
    let b = DefBinary::load_with_names(&Path::new(dir).join(format!("{bin}.bin")), &names).unwrap();
    let mut m = BTreeMap::new();
    for e in b.entries(&names) {
        if let Some(f) = e.file_name {
            m.insert(f.to_string(), b.name_refs[e.global_index].counter);
        }
    }
    m
}

fn main() {
    let a = std::env::args().nth(1).unwrap();
    let r = std::env::args().nth(2).unwrap();
    let bin = std::env::var("BIN").unwrap_or_else(|_| "game".into());
    let ma = map(&a, &bin);
    let mr = map(&r, &bin);

    let (mut same, mut diff_nulldef, mut diff_named) = (0usize, Vec::new(), Vec::new());
    for (name, ca) in &ma {
        let Some(cr) = mr.get(name) else { continue };
        if ca == cr {
            same += 1;
        } else if name.starts_with("NULLDEF_") {
            diff_nulldef.push((name.clone(), *ca, *cr));
        } else {
            diff_named.push((name.clone(), *ca, *cr));
        }
    }
    println!("== {bin}.bin ClassIndex vs retail ==");
    println!("  matching        : {same}");
    println!("  NULLDEF wrong   : {}", diff_nulldef.len());
    println!("  named wrong     : {}", diff_named.len());
    for (n, a, r) in diff_nulldef.iter().take(5) {
        println!("    {n}: ours={a} retail={r}");
    }
    for (n, a, r) in diff_named.iter().take(10) {
        println!("    {n}: ours={a} retail={r}");
    }
}
