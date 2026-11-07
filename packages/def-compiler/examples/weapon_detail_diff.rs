use defs::def::binary::{DefBinary, DefBinaryEntry, DefBody};
use defs::names::Names;
use std::path::Path;

fn load_bin(dir: &str) -> (&'static Names, Vec<DefBinaryEntry<'static>>) {
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
    let (_names_a, entries_a) = load_bin("COMPARE_FOLDER/MODDED_COMPILED_DEFS");
    let (_names_c, entries_c) = load_bin("COMPARE_FOLDER/OUR_COMPILED_DEFS");

    let ea = entries_a.iter().find(|e| e.file_name == Some("OBJECT_IRON_BASTARD_SWORD_CWeaponDef")).unwrap();
    let ec = &entries_c[13338]; // the weapon subdef in C

    let mut b_a = vec![0u8; ea.record.byte_size()];
    let mut cur = &mut b_a[..]; ea.record.serialize(&mut cur).unwrap();
    let mut b_c = vec![0u8; ec.record.byte_size()];
    let mut cur = &mut b_c[..]; ec.record.serialize(&mut cur).unwrap();

    println!("b_a len={}, b_c len={}", b_a.len(), b_c.len());
    for i in 0..b_a.len().min(b_c.len()) {
        if b_a[i] != b_c[i] {
            println!("Diff at byte offset {:#x} ({}): A={:#x} ({}), C={:#x} ({})", i, i, b_a[i], b_a[i], b_c[i], b_c[i]);
        }
    }

    if let (DefBody::WeaponDef(wa), DefBody::WeaponDef(wc)) = (&ea.record.body, &ec.record.body) {
        println!("\n=== WeaponDef Field Comparisons ===");
        println!("Damage: A={}, C={}", wa.damage, wc.damage);
        println!("Type: A={:?}, C={:?}", wa.type_, wc.type_);
        println!("Class: A={:?}, C={:?}", wa.class, wc.class);
        println!("WeaponTrails: A has {}, C has {}", wa.weapon_trails.0.len(), wc.weapon_trails.0.len());
        for (k, v) in &wa.weapon_trails.0 {
            println!("  A trail: key={:?}, val={:?}", k, v);
        }
        for (k, v) in &wc.weapon_trails.0 {
            println!("  C trail: key={:?}, val={:?}", k, v);
        }
    }
}
