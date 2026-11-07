//! Scratch: dump the NameRef third word (the engine's `ClassIndex`) per entry.
//!
//!   probe_counter <dir> [DEF_TYPE]   (BIN=game|frontend|script)
use defs::binary::DefBinary;
use defs::names::Names;
use std::path::Path;

fn main() {
    let dir = std::env::args().nth(1).unwrap();
    let want = std::env::args().nth(2);
    let bin = std::env::var("BIN").unwrap_or_else(|_| "game".into());
    let names = Names::load(&Path::new(&dir).join("names.bin")).unwrap();
    let b =
        DefBinary::load_with_names(&Path::new(&dir).join(format!("{bin}.bin")), &names).unwrap();

    let mut rows: Vec<(usize, u32, String, String)> = Vec::new();
    for e in b.entries(&names) {
        let nr = &b.name_refs[e.global_index];
        rows.push((
            e.global_index,
            nr.counter,
            e.def_name.unwrap_or("?").to_string(),
            e.file_name.unwrap_or("<anon>").to_string(),
        ));
    }
    rows.sort();

    match want {
        Some(t) => {
            println!("== ClassIndex for {t} in {dir}/{bin}.bin ==");
            for (gi, c, _ty, fname) in rows.iter().filter(|r| r.2 == t) {
                println!("  idx {gi:6}  ClassIndex {c:5}  {fname}");
            }
        }
        None => {
            let min = rows.iter().map(|r| r.1).min().unwrap();
            let max = rows.iter().map(|r| r.1).max().unwrap();
            println!("== {dir}/{bin}.bin — ClassIndex range {min}..={max} ==");
            for (gi, c, ty, fname) in rows.iter().take(6) {
                println!("  idx {gi:6}  ClassIndex {c:5}  {ty} / {fname}");
            }
        }
    }
}
