//! Probe/compare a single top-level field across compiled def binaries.
//!
//! Decodes every named entry of a binary, pulls one field by its wire name via
//! reflection, and prints a value histogram + per-entry samples. With `--vs
//! <retail_dir>` it matches entries by name across two builds and reports raw
//! value divergences — the raw ints the semantic ledger hides behind
//! `DefIndex`/`DefString` resolution.
//!
//! Usage:
//!   cargo run -p def-compiler --example probe_field -- \
//!       <dir> <bin> <FieldName> [--vs <retail_dir>]
//!
//!   <bin> is one of: game frontend script
//!
//! Examples:
//!   probe_field $OUT game Font --vs $REF
//!   probe_field $REF game PersistenceFlags
use defs::def::binary::DefBinary;
use defs::def::visit::{FieldRef, FieldVisitor, VisitFields};
use defs::names::Names;
use std::collections::BTreeMap;
use std::path::Path;

/// Grabs the raw value of the first top-level field whose wire name matches.
struct Grab<'n> {
    want: &'n str,
    got: Option<String>,
    names: &'n Names,
    idx2name: &'n BTreeMap<i32, String>,
}

impl FieldVisitor for Grab<'_> {
    fn field(&mut self, name: &'static str, field: FieldRef<'_>) {
        if name != self.want || self.got.is_some() {
            return;
        }
        self.got = Some(render_deep(&field, self.names, self.idx2name));
    }
}

/// Global-index -> named-entry name, for resolving references across index spaces.
fn index_names(dir: &str, bin: &str) -> BTreeMap<i32, String> {
    let (b, names) = load(dir, bin);
    let mut m = BTreeMap::new();
    for e in b.entries(&names) {
        if let Some(f) = e.file_name
            && !f.starts_with("NULLDEF_")
        {
            m.insert(e.global_index as i32, f.to_string());
        }
    }
    m
}

/// Render, resolving DefIndex/u32 references to names (index-space-independent) when the
/// value looks like a valid global index; otherwise fall back to the raw render.
fn render_deep(field: &FieldRef<'_>, names: &Names, idx: &BTreeMap<i32, String>) -> String {
    match field {
        FieldRef::DefIndex(v) => idx
            .get(&v.0)
            .map(|n| format!("ref {n}"))
            .unwrap_or_else(|| format!("idx {}", v.0)),
        // Only DefIndex is resolved: it is the type that *declares* "this is a reference."
        // Raw i32/u32 are left alone — resolving them by index collides with numeric fields
        // (Type=4, ViewAreaBRX=640 are not refs). Re-type a ref field to DefIndex to make
        // the differ (and this tool) resolve it correctly.
        _ => render(field, names),
    }
}

/// Render a scalar-ish field to `raw (resolved)`. Extend as needed.
fn render(field: &FieldRef<'_>, names: &Names) -> String {
    let resolve = |off: i32| -> String {
        if off < 0 {
            format!("<{off}>")
        } else {
            names
                .map
                .get(&(off as u32))
                .map(|e| format!("{:?}", e.string))
                .unwrap_or_else(|| format!("<no-str@{off}>"))
        }
    };
    match field {
        FieldRef::I32(v) => format!("{}", **v),
        FieldRef::U32(v) => format!("{}", **v),
        FieldRef::U16(v) => format!("{}", **v),
        FieldRef::U8(v) => format!("{}", **v),
        FieldRef::I16(v) => format!("{}", **v),
        FieldRef::I8(v) => format!("{}", **v),
        FieldRef::Bool(v) => format!("{}", **v),
        FieldRef::F32(v) => format!("{}", **v),
        FieldRef::DefIndex(v) => format!("idx {}", v.0),
        FieldRef::DefString(v) => format!("{} -> {}", v.0, resolve(v.0)),
        FieldRef::Str(v) => format!("{:?}", v),
        FieldRef::WStr(v) => format!("{:?}", v.0),
        FieldRef::Flags(_) => "<flags>".into(),
        FieldRef::Enum(_) => "<enum>".into(),
        _ => "<unsupported-kind>".into(),
    }
}

fn load(dir: &str, bin: &str) -> (DefBinary, Names) {
    let names = Names::load(&Path::new(dir).join("names.bin")).unwrap();
    let file = format!("{bin}.bin");
    let b = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(&Path::new(dir).join(&file), &names)
    }).unwrap();
    (b, names)
}

/// name -> field-value string, over named (non-NULLDEF-only) entries. Keyed by
/// file_name when present, else def_name, so both sides match by identity.
fn collect(dir: &str, bin: &str, field: &str) -> BTreeMap<String, String> {
    let (b, names) = load(dir, bin);
    let idx2name = index_names(dir, bin);
    let mut out = BTreeMap::new();
    for e in b.entries(&names) {
        let key = match (e.file_name, e.def_name) {
            (Some(f), _) => f.to_string(),
            (None, Some(d)) => format!("<{d}>"),
            _ => continue,
        };
        let mut body = e.record.body.clone();
        let mut g = Grab {
            want: field,
            got: None,
            names: &names,
            idx2name: &idx2name,
        };
        body.visit_fields(&mut g);
        if let Some(v) = g.got {
            out.insert(key, v);
        }
    }
    out
}

/// Records EVERY top-level scalar field of one entry (for `--entry` mode).
struct DumpAll<'n> {
    fields: Vec<(String, String)>,
    names: &'n Names,
    idx2name: &'n BTreeMap<i32, String>,
}
impl FieldVisitor for DumpAll<'_> {
    fn field(&mut self, name: &'static str, field: FieldRef<'_>) {
        self.fields.push((
            name.to_string(),
            render_deep(&field, self.names, self.idx2name),
        ));
    }
}

/// Dump all fields of one named entry from both dirs and diff (default-gap finder).
fn entry_diff(dir: &str, retail: &str, bin: &str, entry: &str) {
    let dump = |d: &str| -> Vec<(String, String)> {
        let (b, names) = load(d, bin);
        let idx2name = index_names(d, bin);
        for e in b.entries(&names) {
            if e.file_name == Some(entry) || e.def_name == Some(entry) {
                let mut body = e.record.body.clone();
                let mut v = DumpAll {
                    fields: vec![],
                    names: &names,
                    idx2name: &idx2name,
                };
                body.visit_fields(&mut v);
                return v.fields;
            }
        }
        vec![]
    };
    let ours = dump(dir);
    let theirs = dump(retail);
    let tmap: BTreeMap<_, _> = theirs.iter().cloned().collect();
    println!("== {entry} field diff (ours vs retail); showing only divergences ==");
    let meaning = |s: &str| {
        s.split_once(" -> ")
            .map(|(_, m)| m.to_string())
            .unwrap_or_else(|| s.to_string())
    };
    let mut n = 0;
    for (name, ov) in &ours {
        if let Some(tv) = tmap.get(name)
            && meaning(ov) != meaning(tv)
        {
            println!("  {name:<28} ours={ov:<22} theirs={tv}");
            n += 1;
        }
    }
    println!("{n} field(s) diverge");
}

fn main() {
    let a: Vec<String> = std::env::args().collect();
    if a.len() < 4 {
        eprintln!(
            "usage: probe_field <dir> <game|frontend|script> <FieldName|--entry NAME> [--vs <retail_dir>]"
        );
        std::process::exit(2);
    }
    let (dir, bin) = (&a[1], &a[2]);
    let vs = a.iter().position(|x| x == "--vs").map(|i| a[i + 1].clone());
    if a[3] == "--entry" {
        entry_diff(dir, vs.as_deref().unwrap_or(dir), bin, &a[4]);
        return;
    }
    let field = &a[3];

    let ours = collect(dir, bin, field);
    println!(
        "== {field} in {bin}.bin ({dir}) — {} entries carry it ==",
        ours.len()
    );

    let mut hist: BTreeMap<&str, usize> = BTreeMap::new();
    for v in ours.values() {
        *hist.entry(v.as_str()).or_default() += 1;
    }
    println!("value histogram:");
    for (v, c) in &hist {
        println!("  x{c:<5} {v}");
    }
    println!("NULLDEF entries (per-type defaults):");
    for (k, v) in &ours {
        if k.contains("NULLDEF") {
            println!("  {k:<32} {v}");
        }
    }

    if let Some(retail_dir) = vs {
        let theirs = collect(&retail_dir, bin, field);
        let mut diffs = 0usize;
        let mut shown = 0usize;
        println!("\n== divergences vs {retail_dir} (raw) ==");
        // For DefString fields the raw offset differs by names.bin layout; the
        // meaning is the part after "-> ". Compare that when present.
        let meaning = |s: &str| -> String {
            s.split_once(" -> ")
                .map(|(_, m)| m.to_string())
                .unwrap_or_else(|| s.to_string())
        };
        for (k, ov) in &ours {
            if let Some(tv) = theirs.get(k)
                && meaning(ov) != meaning(tv)
            {
                diffs += 1;
                if shown < 40 {
                    println!("  {k:<40} ours={ov:<24} theirs={tv}");
                    shown += 1;
                }
            }
        }
        println!("\n{diffs} entries diverge (of {} matched)", ours.len());
    }
}
