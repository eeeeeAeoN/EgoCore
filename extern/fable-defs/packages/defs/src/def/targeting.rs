use crate::DefStruct;
use crate::def::{enums::Opinion, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TargetingDef {
    #[def("Type")]
    pub type_: u32,
    #[def("UseGlowTargeting", default = true)]
    pub use_glow_targeting: bool,
    #[def("TargetArc", default = 90.0)]
    pub target_arc: f32,
    #[def("TargetArcPCThirdPersonAimingMode")]
    pub target_arc_pc_third_person_aiming_mode: f32,
    #[def("TargetingBasePosFromCamera")]
    pub targeting_base_pos_from_camera: bool,
    #[def("TargetingFacingDirFromCamera")]
    pub targeting_facing_dir_from_camera: bool,
    #[def("TargetingPCFacingDirFromCamera")]
    pub targeting_pc_facing_dir_from_camera: bool,
    #[def("RejectTargetsBehindIfRunning")]
    pub reject_targets_behind_if_running: bool,
    #[def("AcceptOnlyEnemiesBehind")]
    pub accept_only_enemies_behind: bool,
    #[def("EnemyBehindTargetingRange")]
    pub enemy_behind_targeting_range: f32,
    #[def("FirstPersonTargetingAngleFalloff")]
    pub first_person_targeting_angle_falloff: f32,
    #[def("FirstPersonTargetingDistanceFalloff")]
    pub first_person_targeting_distance_falloff: f32,
    #[def("ThirdPersonTargetingAngleFalloff")]
    pub third_person_targeting_angle_falloff: f32,
    #[def("ThirdPersonTargetingDistanceFalloff")]
    pub third_person_targeting_distance_falloff: f32,
    #[def("ZLockTargetSelectAngleFalloff")]
    pub z_lock_target_select_angle_falloff: f32,
    #[def("ZLockTargetSelectDistanceFalloff")]
    pub z_lock_target_select_distance_falloff: f32,
    #[def("FirstPersonTargetingOverrideShortRangeFactor")]
    pub first_person_targeting_override_short_range_factor: f32,
    #[def("FirstPersonTargetingOverrideShortRangeFalloff")]
    pub first_person_targeting_override_short_range_falloff: f32,
    #[def("ThirdPersonTargetingOverrideShortRangeFactor")]
    pub third_person_targeting_override_short_range_factor: f32,
    #[def("ThirdPersonTargetingOverrideShortRangeFalloff")]
    pub third_person_targeting_override_short_range_falloff: f32,
    #[def("FirstPersonTargetingEnemyPreferenceFactor")]
    pub first_person_targeting_enemy_preference_factor: f32,
    #[def("ThirdPersonTargetingEnemyPreferenceFactor")]
    pub third_person_targeting_enemy_preference_factor: f32,
    #[def("TargetingRanges")]
    pub targeting_ranges: VecMap<Opinion, f32>,
    #[def("PreferPlayerWeight", default = 1.25)]
    pub prefer_player_weight: f32,
    #[def("PreferNonCombatantsWeight", default = 2.0)]
    pub prefer_non_combatants_weight: f32,
    #[def("PreferLastAttackerWeight", default = 1.0)]
    pub prefer_last_attacker_weight: f32,
    #[def("PreferCurrentBestEnemyWeight", default = 1.0)]
    pub prefer_current_best_enemy_weight: f32,
}
