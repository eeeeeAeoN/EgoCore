use defs::def::binary::{DefBinary, DefBody};
use defs::names::Names;
use std::collections::{HashMap, HashSet};
use std::path::Path;

fn load_bin(dir: &str) -> (&'static Names, Vec<defs::def::binary::DefBinaryEntry<'static>>) {
    let names = Names::load(Path::new(&format!("{}/names.bin", dir))).unwrap();
    let names_ref: &'static Names = Box::leak(Box::new(names));
    let bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new(&format!("{}/game.bin", dir)), names_ref)
    }).unwrap();
    let bin_ref: &'static DefBinary = Box::leak(Box::new(bin));
    let entries: Vec<_> = bin_ref.entries(names_ref).collect();
    (names_ref, entries)
}

fn main() {
    println!("Loading A: MODDED_COMPILED_DEFS...");
    let (names_a, entries_a) = load_bin("COMPARE_FOLDER/MODDED_COMPILED_DEFS");
    println!("Loading B: ENGINE_COMPILED_DEFS (doesn't crash)...");
    let (names_b, entries_b) = load_bin("COMPARE_FOLDER/ENGINE_COMPILED_DEFS");
    println!("Loading C: OUR_COMPILED_DEFS (crashes)...");
    let (names_c, entries_c) = load_bin("COMPARE_FOLDER/OUR_COMPILED_DEFS");

    println!("\n=== TOTAL ENTRY COUNTS ===");
    println!("A (Modded Original): {}", entries_a.len());
    println!("B (Fresh Engine):    {}", entries_b.len());
    println!("C (Our defc):        {}", entries_c.len());

    let names_in = |entries: &[defs::def::binary::DefBinaryEntry<'_>]| -> Vec<String> {
        entries.iter().filter_map(|e| e.file_name.map(|s| s.to_string())).collect()
    };

    let set_a: HashSet<_> = names_in(&entries_a).into_iter().collect();
    let set_b: HashSet<_> = names_in(&entries_b).into_iter().collect();
    let set_c: HashSet<_> = names_in(&entries_c).into_iter().collect();

    println!("\n=== NAMED DEF COUNTS ===");
    println!("A (Modded Original): {}", set_a.len());
    println!("B (Fresh Engine):    {}", set_b.len());
    println!("C (Our defc):        {}", set_c.len());

    let in_b_not_c: Vec<_> = set_b.difference(&set_c).collect();
    let in_c_not_b: Vec<_> = set_c.difference(&set_b).collect();
    let in_a_not_b: Vec<_> = set_a.difference(&set_b).collect();
    let in_c_not_a: Vec<_> = set_c.difference(&set_a).collect();

    println!("In B but NOT C (count={}): {:?}", in_b_not_c.len(), &in_b_not_c[..in_b_not_c.len().min(10)]);
    println!("In C but NOT B (count={}): {:?}", in_c_not_b.len(), &in_c_not_b[..in_c_not_b.len().min(10)]);
    println!("In A but NOT B (count={}): {:?}", in_a_not_b.len(), &in_a_not_b[..in_a_not_b.len().min(10)]);
    println!("In C but NOT A (count={}): {:?}", in_c_not_a.len(), &in_c_not_a[..in_c_not_a.len().min(10)]);

    // Check sentinels in B vs C
    println!("\n=== SENTINEL / TRAILING CHECKS IN B (ENGINE) ===");
    let mut b_sentinels_zero = 0;
    let mut c_sentinels_zero = 0;
    for e in &entries_b {
        if let DefBody::CreatureDef(c) = &e.record.body {
            if c.expressions.trailing_u32 == 0 { b_sentinels_zero += 1; }
            if c.wound_morphs.trailing_u32 == 0 { b_sentinels_zero += 1; }
        }
    }
    for e in &entries_c {
        if let DefBody::CreatureDef(c) = &e.record.body {
            if c.expressions.trailing_u32 == 0 { c_sentinels_zero += 1; }
            if c.wound_morphs.trailing_u32 == 0 { c_sentinels_zero += 1; }
        }
    }
    println!("Sentinels == 0 in B: {}", b_sentinels_zero);
    println!("Sentinels == 0 in C: {}", c_sentinels_zero);

    // Let's inspect sub-defs of common named defs in B vs C
    let common_bc: Vec<_> = set_b.intersection(&set_c).cloned().collect();
    let mut subdef_count_diffs = 0;
    let mut subdef_tag_diffs = 0;
    for name in &common_bc {
        let eb = entries_b.iter().find(|e| e.file_name == Some(name.as_str())).unwrap();
        let ec = entries_c.iter().find(|e| e.file_name == Some(name.as_str())).unwrap();
        let subs_b = eb.record.sub_defs.as_deref().unwrap_or(&[]);
        let subs_c = ec.record.sub_defs.as_deref().unwrap_or(&[]);
        if subs_b.len() != subs_c.len() {
            if subdef_count_diffs < 10 {
                println!("Subdef count diff in {}: B has {}, C has {}", name, subs_b.len(), subs_c.len());
            }
            subdef_count_diffs += 1;
        } else {
            for (sb, sc) in subs_b.iter().zip(subs_c.iter()) {
                if sb.name_crc != sc.name_crc {
                    if subdef_tag_diffs < 10 {
                        println!("Subdef tag mismatch in {}: B has {:#x}, C has {:#x}", name, sb.name_crc, sc.name_crc);
                    }
                    subdef_tag_diffs += 1;
                }
            }
        }
    }
    println!("Total named defs with subdef count diff (B vs C): {}", subdef_count_diffs);
    println!("Total subdef tag mismatches (B vs C): {}", subdef_tag_diffs);

    // Let's check Armour defs in B vs A!
    println!("\n=== ARMOUR DEFS INSPECTION (B vs A) ===");
    for e in &entries_a {
        if let Some(name) = e.file_name {
            if name.starts_with("OBJECT_ARMOUR") || name.starts_with("OBJECT_HERO_ARMOUR") || name.contains("BOOTS") || name.contains("SHIRT") || name.contains("TROUSERS") || name.contains("GLOVES") {
                let eb = entries_b.iter().find(|b| b.file_name == Some(name));
                if let Some(eb) = eb {
                    let mut b_a = vec![0u8; e.record.byte_size()];
                    let mut cur = &mut b_a[..]; e.record.serialize(&mut cur).unwrap();
                    let mut b_b = vec![0u8; eb.record.byte_size()];
                    let mut cur = &mut b_b[..]; eb.record.serialize(&mut cur).unwrap();
                    if b_a != b_b {
                        println!("Armour {} diff in A vs B: size A={}, size B={}", name, b_a.len(), b_b.len());
                        // check subdefs
                        let sa = e.record.sub_defs.as_deref().unwrap_or(&[]);
                        let sb = eb.record.sub_defs.as_deref().unwrap_or(&[]);
                        println!("  subdefs: A has {}, B has {}", sa.len(), sb.len());
                        break; // sample just first
                    }
                } else {
                    println!("Armour {} missing in B!", name);
                    break;
                }
            }
        }
    }
}
