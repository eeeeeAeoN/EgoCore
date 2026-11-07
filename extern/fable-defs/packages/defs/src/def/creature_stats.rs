use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureStatsDef {
    #[def("MoralityForKill", default = -100)]
    pub morality_for_kill: i32,
    #[def("MoralityForPunch", default = -1)]
    pub morality_for_punch: i32,
    #[def("MoralityForWeaponHit", default = -5)]
    pub morality_for_weapon_hit: i32,
    #[def("ExperienceWorth")]
    pub experience_worth: i32,
    #[def("RenownWorth")]
    pub renown_worth: i32,
    #[def("Speed", default = 0.5)]
    pub speed: f32,
    #[def("Dexterity", default = 0.5)]
    pub dexterity: f32,
    #[def("StrengthDamageMultiplier", default = 1.0)]
    pub strength_damage_multiplier: f32,
}
