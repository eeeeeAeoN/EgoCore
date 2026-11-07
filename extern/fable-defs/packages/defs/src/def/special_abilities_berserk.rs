use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesBerserkDef {
    #[def("SpeedMultiplier")]
    pub speed_multiplier: f32,
    #[def("FramesToFadeOut")]
    pub frames_to_fade_out: i32,
    #[def("LowerBerserkPercent")]
    pub lower_berserk_percent: f32,
    #[def("TimeForThemeTransition")]
    pub time_for_theme_transition: f32,
    #[def("BlurCenterFromFloor")]
    pub blur_center_from_floor: f32,
    #[def("BerserkThreshold")]
    pub berserk_threshold: f32,
    #[def("BerserkLength")]
    pub berserk_length: f32,
    #[def("BerserkIntensity")]
    pub berserk_intensity: f32,
    #[def("BerserkFade")]
    pub berserk_fade: f32,
    #[def("BerserkRange")]
    pub berserk_range: f32,
    #[def("BerserkTwist")]
    pub berserk_twist: f32,
    #[def("TimeForRadialBlurToFadeOut")]
    pub time_for_radial_blur_to_fade_out: f32,
    #[def("CameraStartDist")]
    pub camera_start_dist: f32,
    #[def("CameraEndDist")]
    pub camera_end_dist: f32,
    #[def("CameraStartDeltaHeight")]
    pub camera_start_delta_height: f32,
    #[def("CameraEndDeltaHeight")]
    pub camera_end_delta_height: f32,
    #[def("LevelUpTimings")]
    pub level_up_timings: Vec<f32>,
    #[def("BerserkDamageToEnemyMultiplier")]
    pub berserk_damage_to_enemy_multiplier: Vec<f32>,
    #[def("BerserkDamageToEnemyMultiplierAdditionPerHit")]
    pub berserk_damage_to_enemy_multiplier_addition_per_hit: f32,
    #[def("BerserkDamageToMeMultiplier")]
    pub berserk_damage_to_me_multiplier: Vec<f32>,
    #[def("BerserkDuration")]
    pub berserk_duration: Vec<f32>,
    #[def("MaxIncreaseInStrength")]
    pub max_increase_in_strength: Vec<f32>,
    #[def("BerserkScale")]
    pub berserk_scale: Vec<f32>,
    #[def("BerserkBlastRadius")]
    pub berserk_blast_radius: f32,
    #[def("CamDist")]
    pub cam_dist: Vec<f32>,
    #[def("CamHeight")]
    pub cam_height: Vec<f32>,
    #[def("CamFOV")]
    pub cam_fov: Vec<f32>,
    #[def("CamSwitchTime")]
    pub cam_switch_time: Vec<f32>,
    #[def("RumbleLevel")]
    pub rumble_level: Vec<f32>,
    #[def("RumbleSmoothness")]
    pub rumble_smoothness: Vec<f32>,
    #[def("RumbleAttack")]
    pub rumble_attack: Vec<f32>,
    #[def("RumbleDecay")]
    pub rumble_decay: Vec<f32>,
    #[def("RumbleReleaseLevelDelta")]
    pub rumble_release_level_delta: f32,
    #[def("RumbleReleaseSmoothnessDelta")]
    pub rumble_release_smoothness_delta: f32,
    #[def("RumbleReleaseAttackDelta")]
    pub rumble_release_attack_delta: f32,
    #[def("RumbleReleaseDecayDelta")]
    pub rumble_release_decay_delta: f32,
    #[def("DecapitateMaxProb")]
    pub decapitate_max_prob: Vec<f32>,
    #[def("CrazyActionRadius")]
    pub crazy_action_radius: f32,
    #[def("CrazyActionDamage")]
    pub crazy_action_damage: f32,
    #[def("TargetNearestEnemyDistance")]
    pub target_nearest_enemy_distance: f32,
    #[def("WeaponTrailAttack")]
    pub weapon_trail_attack: i32,
    #[def("WeaponTrailKnockDown")]
    pub weapon_trail_knock_down: i32,
    #[def("TintColorR")]
    pub tint_color_r: Vec<i32>,
    #[def("TintColorG")]
    pub tint_color_g: Vec<i32>,
    #[def("TintColorB")]
    pub tint_color_b: Vec<i32>,
    #[def("CanBlockWhileBerserked")]
    pub can_block_while_berserked: bool,
    #[def("CrazyIdleIntervalSecs")]
    pub crazy_idle_interval_secs: f32,
}
