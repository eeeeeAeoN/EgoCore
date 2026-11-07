use crate::DefStruct;
use crate::def::{values::PhysicalPrimitiveInit, wire::DefIndex};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HitLocationDef {
    #[def("PhysicalPrimitive")]
    pub physical_primitive: PhysicalPrimitiveInit,
    #[def("Armour")]
    pub armour: DefIndex,
    #[def("HitDamageEffect")]
    pub hit_damage_effect: i32,
    #[def("HitBlockedEffect")]
    pub hit_blocked_effect: i32,
    #[def("VulnerableEffect")]
    pub vulnerable_effect: i32,
    #[def("HitResponseAction")]
    pub hit_response_action: i32,
    #[def("HitResponseToEpicSpellsAction")]
    pub hit_response_to_epic_spells_action: i32,
    #[def("HitKnockdownAction")]
    pub hit_knockdown_action: i32,
    #[def("HitKnockdownDeathAction")]
    pub hit_knockdown_death_action: i32,
    #[def("BlockResponseAction")]
    pub block_response_action: i32,
    #[def("Flags")]
    pub flags: i32,
    #[def("DefendableAgainstShots", default = true)]
    pub defendable_against_shots: bool,
    #[def("DefendableAgainstMelee", default = true)]
    pub defendable_against_melee: bool,
    #[def("Priority")]
    pub priority: i32,
    #[def("Default")]
    pub default: bool,
    #[def("DecapitateOnHit")]
    pub decapitate_on_hit: bool,
    #[def("PlayGetHitResponse", default = true)]
    pub play_get_hit_response: bool,
    #[def("EnableOnCreate", default = true)]
    pub enable_on_create: bool,
    #[def("HitLocationCodelet")]
    pub hit_location_codelet: i32,
}
