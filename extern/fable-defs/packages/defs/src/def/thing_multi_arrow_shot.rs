use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThingMultiArrowShotDef {
    #[def("AccelerationScalar")]
    pub acceleration_scalar: f32,
    #[def("MaxAngle")]
    pub max_angle: f32,
    #[def("MaxDistance")]
    pub max_distance: f32,
    #[def("TimeToReachTarget")]
    pub time_to_reach_target: f32,
    #[def("MaxAngleToTurnDegs")]
    pub max_angle_to_turn_degs: f32,
    #[def("MultiArrowWeaponTrail")]
    pub multi_arrow_weapon_trail: i32,
    #[def("MultiArrowHitEffect")]
    pub multi_arrow_hit_effect: i32,
    #[def("MultiArrowTrailEffect")]
    pub multi_arrow_trail_effect: i32,
    #[def("MultiArrowSpeed")]
    pub multi_arrow_speed: f32,
    #[def("MultiArrowDeflectionByMelee")]
    pub multi_arrow_deflection_by_melee: bool,
    #[def("MultiArrowHitEffectPercentage")]
    pub multi_arrow_hit_effect_percentage: f32,
}
