use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesBattleChargeDef {
    #[def("RunSpeed")]
    pub run_speed: f32,
    #[def("DashDistance")]
    pub dash_distance: Vec<f32>,
    #[def("RadiusOfEffect")]
    pub radius_of_effect: Vec<f32>,
    #[def("BattleChargeDamage")]
    pub battle_charge_damage: Vec<f32>,
    #[def("BattleChargeCombatMultiplierFactor")]
    pub battle_charge_combat_multiplier_factor: f32,
    #[def("BattleChargeRadialStartSeconds")]
    pub battle_charge_radial_start_seconds: f32,
    #[def("BattleChargeRadialEndSeconds")]
    pub battle_charge_radial_end_seconds: f32,
    #[def("BattleChargeRadialThreshold")]
    pub battle_charge_radial_threshold: f32,
    #[def("BattleChargeRadialLength")]
    pub battle_charge_radial_length: f32,
    #[def("BattleChargeRadialIntensity")]
    pub battle_charge_radial_intensity: f32,
    #[def("BattleChargeRadialFade")]
    pub battle_charge_radial_fade: f32,
    #[def("BattleChargeRadialRange")]
    pub battle_charge_radial_range: f32,
    #[def("BattleChargeRadialTwist")]
    pub battle_charge_radial_twist: f32,
    #[def("PoseDelaySecs")]
    pub pose_delay_secs: f32,
    #[def("ChargeDelay")]
    pub charge_delay: f32,
    #[def("NumPeopleHit")]
    pub num_people_hit: Vec<i32>,
    #[def("DragonExtraGumphRadiusMultiplier")]
    pub dragon_extra_gumph_radius_multiplier: f32,
    #[def("DragonExtraGumphRadius")]
    pub dragon_extra_gumph_radius: f32,
}
