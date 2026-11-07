use defs::def::binary::{DefBinary, DefBody};
use defs::names::Names;
use std::collections::HashSet;
use std::path::Path;

fn main() {
    let mod_names = Names::load(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/names.bin")).unwrap();
    let mod_bin = defs::wire::with_lenient_control_ids(|| {
        DefBinary::load_with_names(Path::new("COMPARE_FOLDER/MODDED_COMPILED_DEFS/game.bin"), &mod_names)
    }).unwrap();
    let mod_entries: Vec<_> = mod_bin.entries(&mod_names).collect();

    let mut affected_owners = HashSet::new();
    for (idx, e) in mod_entries.iter().enumerate() {
        let wt_opt = match &e.record.body {
            DefBody::WeaponDef(w) => Some(&w.weapon_trails),
            _ => None,
        };
        if let Some(wt) = wt_opt {
            let has_null_trail = wt.0.iter().any(|(k, v)| {
                k.attack == 0 && k.knockdown == 10 && v.0 == 12
            });
            if has_null_trail {
                // Find owner if this is a subdef
                let name = e.file_name.or(e.def_name).unwrap_or("<unknown>");
                // Check which named def points to this subdef
                let mut found_owner = false;
                for parent in &mod_entries {
                    if let Some(subs) = &parent.record.sub_defs {
                        if subs.iter().any(|s| s.def_index as usize == idx) {
                            if let Some(pname) = parent.file_name {
                                affected_owners.insert(pname.to_string());
                                found_owner = true;
                            }
                        }
                    }
                }
                if !found_owner {
                    affected_owners.insert(name.to_string());
                }
            }
        }
    }

    println!("Total definitions that have WeaponTrails[AUGMENTATION_NULL]: {}", affected_owners.len());
    let mut list: Vec<_> = affected_owners.into_iter().collect();
    list.sort();
    for o in list.iter().take(20) {
        println!("  {}", o);
    }
}
