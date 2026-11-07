use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThunderBattleDef {
    #[def("NumGetHitsBeforeStormAttack")]
    pub num_get_hits_before_storm_attack: i32,
    #[def("NumMissesBeforeStormAttack")]
    pub num_misses_before_storm_attack: i32,
    #[def("LightningDamage", default = 5.0)]
    pub lightning_damage: f32,
    #[def("BattleChargeLevel")]
    pub battle_charge_level: i32,
}
