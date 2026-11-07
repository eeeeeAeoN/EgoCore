use defs::def::binary::{DefBinary, DefBody};
use defs::def::CreatureDef;
use defs::names::Names;
use std::collections::HashMap;
use std::path::Path;

fn extract_creature_def<'a>(e: &'a defs::def::binary::DefBinaryEntry<'a>, all: &'a [defs::def::binary::DefBinaryEntry<'a>]) -> Option<&'a CreatureDef> {
    if let Some(subs) = &e.record.sub_defs {
        for s in subs {
            let sub = &all[s.def_index as usize];
            if let DefBody::CreatureDef(c) = &sub.record.body {
                return Some(c);
            }
        }
    }
    None
}

fn main() {
    let orig_names = Names::load(Path::new("COMPARE_FOLDER/ORIGINAL_COMPILED_DEFS/names.bin")).unwrap();
    let orig_bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new("COMPARE_FOLDER/ORIGINAL_COMPILED_DEFS/game.bin"), &orig_names)
    }).unwrap();
    let orig_entries: Vec<_> = orig_bin.entries(&orig_names).collect();

    // Map each named def to its morph
    let mut named_morphs = HashMap::new();
    for e in &orig_entries {
        if let Some(f) = e.file_name {
            if let Some(c) = extract_creature_def(e, &orig_entries) {
                if c.random_appearance_morph.num_body_parts > 0 || !c.random_appearance_morph.body_parts0.meshes.is_empty() {
                    named_morphs.insert(f.to_string(), c.random_appearance_morph.clone());
                }
            }
        }
    }

    println!("Total named defs in ORIGINAL with morph: {}", named_morphs.len());

    // Map morph -> canonical source def name among the 85
    // Let's see how many distinct morphs exist among all creatures in ORIGINAL!
    let mut distinct_morphs: Vec<(String, _)> = Vec::new();
    for (name, m) in &named_morphs {
        if !distinct_morphs.iter().any(|(_, existing)| existing == m) {
            distinct_morphs.push((name.clone(), m.clone()));
        }
    }
    println!("Total distinct morph configurations in ORIGINAL: {}", distinct_morphs.len());
    for (name, _) in &distinct_morphs {
        println!("  Canonical: {}", name);
    }
}
