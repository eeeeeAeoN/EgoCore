use defs::def::binary::{DefBinary, DefBody};
use defs::def::CreatureDef;
use defs::names::Names;
use std::collections::BTreeMap;
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

fn has_real_morph(c: &CreatureDef) -> bool {
    !c.random_appearance_morph.body_parts0.meshes.is_empty()
        || !c.random_appearance_morph.body_parts1.meshes.is_empty()
        || !c.random_appearance_morph.body_parts2.meshes.is_empty()
}

fn tex_morph_eq(c1: &CreatureDef, c2: &CreatureDef) -> bool {
    let m1 = &c1.random_appearance_morph;
    let m2 = &c2.random_appearance_morph;
    m1.body_parts0 == m2.body_parts0
        && m1.body_parts1 == m2.body_parts1
        && m1.body_parts2 == m2.body_parts2
        && m1.final_trailing == m2.final_trailing
}

fn main() {
    let orig_names = Names::load(Path::new("COMPARE_FOLDER/ORIGINAL_COMPILED_DEFS/names.bin")).unwrap();
    let orig_bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new("COMPARE_FOLDER/ORIGINAL_COMPILED_DEFS/game.bin"), &orig_names)
    }).unwrap();
    let orig_entries: Vec<_> = orig_bin.entries(&orig_names).collect();

    let mut orig_creatures: BTreeMap<String, &CreatureDef> = BTreeMap::new();
    for e in &orig_entries {
        if let Some(f) = e.file_name {
            if let Some(c) = extract_creature_def(e, &orig_entries) {
                if has_real_morph(c) {
                    orig_creatures.insert(f.to_string(), c);
                }
            }
        }
    }

    if let Some(&m) = orig_creatures.get("CREATURE_BANDIT_GRUNT") {
        orig_creatures.insert("CREATURE_BANDIT_FOOTSOLDIER_BASE".to_string(), m);
    }
    if let Some(&m) = orig_creatures.get("CREATURE_BANDIT_GRUNT_LEVEL2") {
        orig_creatures.insert("CREATURE_BANDIT_FOOTSOLDIER_TBG_BASE".to_string(), m);
    }
    if let Some(&m) = orig_creatures.get("CREATURE_BANDIT_GRUNT_LEVEL3") {
        orig_creatures.insert("CREATURE_BANDIT_FOOTSOLDIER_BLG_BASE".to_string(), m);
    }

    let authored_85 = [
        "CREATURE_ARENA_SHOPKEEPER", "CREATURE_BANDIT_CAMP_VILLAGER_GAME_MASTER", "CREATURE_BANDIT_FOOTSOLDIER_BASE",
        "CREATURE_BANDIT_FOOTSOLDIER_BLG_BASE", "CREATURE_BANDIT_FOOTSOLDIER_TBG_BASE", "CREATURE_BANDIT_LEADER",
        "CREATURE_BANDIT_LIEUTENANT", "CREATURE_BODY_GUARD", "CREATURE_BODY_GUARD_BASE", "CREATURE_BODY_GUARD_GOOD",
        "CREATURE_BODY_GUARD_GOOD_WEAK", "CREATURE_BODY_GUARD_WEAK", "CREATURE_BOWERSTONE_POSH_VILLAGER_FEMALE_NORMAL_MESH",
        "CREATURE_BOWERSTONE_POSH_VILLAGER_MALE_NORMAL_MESH", "CREATURE_BOWERSTONE_SLUMS_VILLAGER_FEMALE_NORMAL_MESH",
        "CREATURE_BOWERSTONE_SLUMS_VILLAGER_MALE_NORMAL_MESH", "CREATURE_BS_FISTICUFFS_MEMBER", "CREATURE_BS_GUARD_BLACK",
        "CREATURE_BS_GUARD_RED", "CREATURE_BS_VILLAGER_BOY", "CREATURE_BS_VILLAGER_BULLY_MALE_ADULT", "CREATURE_BS_VILLAGER_FEMALE",
        "CREATURE_BS_VILLAGER_GIRL", "CREATURE_BS_VILLAGER_MALE", "CREATURE_BUTTERFLY_RANDOM", "CREATURE_FEMALE_APPRENTICE_FIGHTER",
        "CREATURE_GHOST_VILLAGER_FEMALE", "CREATURE_GHOST_VILLAGER_MALE", "CREATURE_GUARD", "CREATURE_GUILD_APPRENTICE_FEMALE",
        "CREATURE_GUILD_APPRENTICE_MALE", "CREATURE_GUILD_EVIL_APPRENTICE_FEMALE", "CREATURE_GUILD_EVIL_APPRENTICE_MALE",
        "CREATURE_GUILD_GOOD_APPRENTICE_FEMALE", "CREATURE_GUILD_GOOD_APPRENTICE_MALE", "CREATURE_GUILD_VILLAGER_INNKEEPER",
        "CREATURE_GUILD_VILLAGER_SHOPKEEPER", "CREATURE_HERO_RIVAL_EVIL_01", "CREATURE_HOBBE_BASIC", "CREATURE_HOBBE_GRUNT",
        "CREATURE_HOBBE_LIEUTENANT", "CREATURE_HOBBE_SPELLCASTER_BASE", "CREATURE_HOOK_COAST_GAMES_MASTER",
        "CREATURE_HOOK_COAST_VILLAGER_FEMALE_NORMAL_MESH", "CREATURE_HOOK_COAST_VILLAGER_FEMALE_TAILOR",
        "CREATURE_HOOK_COAST_VILLAGER_MALE_BARMAN", "CREATURE_HOOK_COAST_VILLAGER_MALE_BLACKSMITH",
        "CREATURE_HOOK_COAST_VILLAGER_MALE_NORMAL_MESH", "CREATURE_KHG_FISTICUFFS_MEMBER",
        "CREATURE_KNOTHOLE_GLADE_VILLAGER_FEMALE_NORMAL_MESH", "CREATURE_KNOTHOLE_GLADE_VILLAGER_GAME_MASTER",
        "CREATURE_KNOTHOLE_GLADE_VILLAGER_MALE_BARMAN", "CREATURE_KNOTHOLE_GLADE_VILLAGER_MALE_BLACKSMITH",
        "CREATURE_KNOTHOLE_GLADE_VILLAGER_MALE_NORMAL_MESH", "CREATURE_KNOTHOLE_GLADE_VILLAGER_MALE_SHOPKEEPER",
        "CREATURE_KN_VILLAGER_FEMALE", "CREATURE_KN_VILLAGER_MALE", "CREATURE_OAKVALE_VILLAGER_FEMALE_NORMAL_MESH",
        "CREATURE_OAKVALE_VILLAGER_GAME_MASTER", "CREATURE_OAKVALE_VILLAGER_MALE_BARMAN", "CREATURE_OAKVALE_VILLAGER_MALE_BLACKSMITH",
        "CREATURE_OAKVALE_VILLAGER_MALE_FEARLESS", "CREATURE_OAKVALE_VILLAGER_MALE_NORMAL_MESH",
        "CREATURE_OAKVALE_VILLAGER_MALE_SHOPKEEPER", "CREATURE_OK_UNDEAD_GENERAL_NO_FX", "CREATURE_OK_UNDEAD_LIEUTENANT_NO_FX",
        "CREATURE_OK_UNDEAD_SOLDIER_SWORD_NO_FX", "CREATURE_OV_FISTICUFFS_MEMBER", "CREATURE_PRISON_GUARD_SMALL",
        "CREATURE_PRISON_GUARD_TORTURER", "CREATURE_PROPHET_01", "CREATURE_PROPHET_02", "CREATURE_PROPHET_03",
        "CREATURE_PROPHET_04", "CREATURE_PROPHET_05", "CREATURE_RIVAL_HERO_THUNDER", "CREATURE_RIVAL_HERO_THUNDER_TLC",
        "CREATURE_RIVAL_HERO_WHISPER", "CREATURE_RIVAL_HERO_WHISPER_HOODED", "CREATURE_SS_ORACLE_GUARD", "CREATURE_SS_TRADER",
        "CREATURE_SS_VILLAGER_FEMALE", "CREATURE_SS_VILLAGER_MALE", "CREATURE_TRADER_01", "CREATURE_TRADER_INFECTED",
    ];

    let mut authored_morphs: BTreeMap<String, &CreatureDef> = BTreeMap::new();
    for name in &authored_85 {
        if let Some(&m) = orig_creatures.get(*name) {
            authored_morphs.insert(name.to_string(), m);
        }
    }

    let mod_names = Names::load(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/names.bin")).unwrap();
    let mod_bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/game.bin"), &mod_names)
    }).unwrap();
    let mod_entries: Vec<_> = mod_bin.entries(&mod_names).collect();

    let mut matches = BTreeMap::new();
    for e in &mod_entries {
        if let Some(f) = e.file_name {
            if let Some(c_mod) = extract_creature_def(e, &mod_entries) {
                if has_real_morph(c_mod) {
                    if let Some(&auth_c) = authored_morphs.get(f) {
                        if tex_morph_eq(auth_c, c_mod) {
                            matches.insert(f.to_string(), f.to_string());
                            continue;
                        }
                    }
                    if let Some((auth_name, _)) = authored_morphs.iter().find(|(_, auth_c)| tex_morph_eq(auth_c, c_mod)) {
                        matches.insert(f.to_string(), auth_name.clone());
                    }
                }
            }
        }
    }

    matches.insert("CREATURE_BANDIT_FOOTSOLDIER_BASE".to_string(), "CREATURE_BANDIT_FOOTSOLDIER_BASE".to_string());
    matches.insert("CREATURE_BANDIT_FOOTSOLDIER_TBG_BASE".to_string(), "CREATURE_BANDIT_FOOTSOLDIER_TBG_BASE".to_string());
    matches.insert("CREATURE_BANDIT_FOOTSOLDIER_BLG_BASE".to_string(), "CREATURE_BANDIT_FOOTSOLDIER_BLG_BASE".to_string());

    println!("Total matched creatures: {}", matches.len());

    let mut out = String::from("{\n");
    let count = matches.len();
    for (i, (k, v)) in matches.iter().enumerate() {
        let comma = if i + 1 < count { "," } else { "" };
        out.push_str(&format!("  \"{}\": \"{}\"{}\n", k, v, comma));
    }
    out.push_str("}\n");
    std::fs::write("morph_matches.json", out).unwrap();
    println!("Wrote morph_matches.json");
}
