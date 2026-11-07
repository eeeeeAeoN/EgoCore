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

fn dump_bp<M>(name: &str, meshes: &[M], get_mesh: impl Fn(&M) -> (i32, i32, usize, Box<dyn Iterator<Item = (u32, i32, i32)> + '_>)) {
    if !meshes.is_empty() {
        println!("  {} (len={}):", name, meshes.len());
        for m in meshes {
            let (mesh_id, priority, entries_len, entries) = get_mesh(m);
            println!("    mesh={} priority={} entries={}", mesh_id, priority, entries_len);
            for (key, orig, repl) in entries {
                println!("      key={} orig={} repl={}", key, orig, repl);
            }
        }
    }
}

fn main() {
    let mod_names = Names::load(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/names.bin")).unwrap();
    let mod_bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/game.bin"), &mod_names)
    }).unwrap();
    let mod_entries: Vec<_> = mod_bin.entries(&mod_names).collect();

    let list = [
        "CREATURE_BOWERSTONE_POSH_VILLAGER_FEMALE_HELPFUL",
        "CREATURE_HOBBE_GIANTBOSS", "CREATURE_UNCLOTHED_DEMON_DOOR_WOMAN",
        "CREATURE_CREATURE_UNCLOTHED_DEMON_DOOR_MAN",
        "CREATURE_CREATURE_GOLDENCHESTBANDIT",
        "CREATURE_TRADER_STOCKED_BANK", "CREATURE_EMILY_ADULT", "CREATURE_TAHLIA",
        "CREATURE_MILLICENT"
    ];

    for name in &list {
        if let Some(e) = mod_entries.iter().find(|e| e.file_name == Some(*name)) {
            if let Some(c) = extract_creature_def(e, &mod_entries) {
                let m = &c.random_appearance_morph;
                println!("=== {} final_trailing={} ===", name, m.final_trailing);
                dump_bp("body_parts0", &m.body_parts0.meshes, |m| (m.mesh_id, m.texture_id, m.texture_morphs.entries.len(), Box::new(m.texture_morphs.entries.iter().map(|e| (e.key, i32::from_le_bytes(e.morph[0..4].try_into().unwrap()), i32::from_le_bytes(e.morph[4..8].try_into().unwrap()))))));
                dump_bp("body_parts1", &m.body_parts1.meshes, |m| (m.mesh_id, m.texture_id, m.texture_morphs.entries.len(), Box::new(m.texture_morphs.entries.iter().map(|e| (e.key, i32::from_le_bytes(e.morph[0..4].try_into().unwrap()), i32::from_le_bytes(e.morph[4..8].try_into().unwrap()))))));
                dump_bp("body_parts2", &m.body_parts2.meshes, |m| (m.mesh_id, m.texture_id, m.texture_morphs.entries.len(), Box::new(m.texture_morphs.entries.iter().map(|e| (e.key, i32::from_le_bytes(e.morph[0..4].try_into().unwrap()), i32::from_le_bytes(e.morph[4..8].try_into().unwrap()))))));
            }
        }
    }
}
