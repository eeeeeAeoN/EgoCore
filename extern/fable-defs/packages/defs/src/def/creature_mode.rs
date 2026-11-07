use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureModeDef {
    #[def("DefaultCreatureMode", default = 1)]
    pub default_creature_mode: i32,
    #[def("InitialCreatureMode")]
    pub initial_creature_mode: i32,
    #[def("DefaultWeaponCreatureMode", default = 4)]
    pub default_weapon_creature_mode: i32,
    #[def("AvailableIdleModes")]
    pub available_idle_modes: Vec<i32>,
}
