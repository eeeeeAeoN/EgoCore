use crate::DefStruct;
use crate::wire::{DefIndex, VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroSuitDef {
    // Each part maps a CLOTHING_SUIT_* slot id to an OBJECT_* def reference
    // (`SuitParts[CLOTHING_SUIT_HEAD] OBJECT_HERO_NO_HAT;`), so the value is a
    // `DefIndex`, not an enum — byte-identical (both 4-byte LE), but the differ
    // now resolves it by name across the two index spaces.
    #[def("SuitParts")]
    pub suit_parts: VecMap<i32, DefIndex>,
}
