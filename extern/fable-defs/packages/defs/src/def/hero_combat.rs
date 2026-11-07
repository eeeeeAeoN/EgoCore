use crate::DefStruct;
use crate::def::{
    enums::{IdleStateGroup, Opinion},
    values::FloatRange,
    wire::DefIndex,
};
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroCombatDef {
    #[def("RangedAccuracySecondsForMaximumWithSkill")]
    pub ranged_accuracy_seconds_for_maximum_with_skill: FloatRange,
    #[def("RangedAccuracyPercentFractionWithBuildup")]
    pub ranged_accuracy_percent_fraction_with_buildup: FloatRange,
    #[def("RangedAccuracyPercentWithStats")]
    pub ranged_accuracy_percent_with_stats: FloatRange,
    #[def("RangedAccuracyWeighting")]
    pub ranged_accuracy_weighting: BTreeMap<Opinion, f32>,
    #[def("RangedAccuracyShakeyCursor")]
    pub ranged_accuracy_shakey_cursor: FloatRange,
    #[def("RangedAccuracyShakeyCursorWeighting")]
    pub ranged_accuracy_shakey_cursor_weighting: BTreeMap<Opinion, f32>,
    #[def("RangedAccuracyShakeFractionOfRadius")]
    pub ranged_accuracy_shake_fraction_of_radius: f32,
    #[def("RangedDamageMultiplierWeighting")]
    pub ranged_damage_multiplier_weighting: BTreeMap<Opinion, f32>,
    #[def("RangedDamageMultiplierSpeedWithStatParameter")]
    pub ranged_damage_multiplier_speed_with_stat_parameter: f32,
    #[def("RangedDamageMultiplierSpeedWithDistance")]
    pub ranged_damage_multiplier_speed_with_distance: f32,
    #[def("RangedDamageMultiplierSpeedNominalBuildup")]
    pub ranged_damage_multiplier_speed_nominal_buildup: f32,
    #[def("RangedDamageMultiplierScaleNominalBuildup")]
    pub ranged_damage_multiplier_scale_nominal_buildup: f32,
    #[def("RangedDamageMultiplierSpeedSuperBuildup")]
    pub ranged_damage_multiplier_speed_super_buildup: f32,
    #[def("RangedDamageMultiplierScaleSuperBuildup")]
    pub ranged_damage_multiplier_scale_super_buildup: f32,
    #[def("RangedWeaponReloadSpeedWeighting")]
    pub ranged_weapon_reload_speed_weighting: BTreeMap<Opinion, f32>,
    #[def("MeleeDamageMultiplierWithStats")]
    pub melee_damage_multiplier_with_stats: FloatRange,
    #[def("MeleeDamageWeighting")]
    pub melee_damage_weighting: BTreeMap<Opinion, f32>,
    #[def("HitDamageMultiplierWithStats")]
    pub hit_damage_multiplier_with_stats: FloatRange,
    #[def("HitDamageWeighting")]
    pub hit_damage_weighting: BTreeMap<Opinion, f32>,
    #[def("MeleeFlourishValidOnMultiplierLevelIncrease")]
    pub melee_flourish_valid_on_multiplier_level_increase: bool,
    #[def("MeleeFlourishNumMultiplierLevelsPerFlourish")]
    pub melee_flourish_num_multiplier_levels_per_flourish: i32,
    #[def("MeleeFlourishAllowBreakingOfRotation")]
    pub melee_flourish_allow_breaking_of_rotation: bool,
    #[def("MeleeFlourishSecondsForAvailibility")]
    pub melee_flourish_seconds_for_availibility: f32,
    #[def("MeleeFlourishOneShot")]
    pub melee_flourish_one_shot: bool,
    #[def("MeleeFlourishUsable")]
    pub melee_flourish_usable: bool,
    #[def("MeleeCombatPlayStrikeSounds")]
    pub melee_combat_play_strike_sounds: bool,
    #[def("TargetingPressureFractionThresholdForToggle")]
    pub targeting_pressure_fraction_threshold_for_toggle: f32,
    #[def("MaxAngleToTurnToShootAtTargetInThirdPersonBowMode")]
    pub max_angle_to_turn_to_shoot_at_target_in_third_person_bow_mode: f32,
    #[def("MeleeStrikeNumFramesPauseOnHeroStrike")]
    pub melee_strike_num_frames_pause_on_hero_strike: i32,
    #[def("MeleeStrikeAnimationSpeedFractionWithStats")]
    pub melee_strike_animation_speed_fraction_with_stats: FloatRange,
    #[def("MeleeStrikeAnimationSpeedWeighting")]
    pub melee_strike_animation_speed_weighting: BTreeMap<Opinion, f32>,
    #[def("AutoBlockSkillThreshold")]
    pub auto_block_skill_threshold: f32,
    #[def("AutoBlockStrengthThreshold")]
    pub auto_block_strength_threshold: f32,
    #[def("MeleeRecoilMaintainHandednessSkillThreshold")]
    pub melee_recoil_maintain_handedness_skill_threshold: f32,
    #[def("MeleeRecoilMaintainHandednessStrengthThreshold")]
    pub melee_recoil_maintain_handedness_strength_threshold: f32,
    #[def("MeleeRecoilMaintainComboChainSkillThreshold")]
    pub melee_recoil_maintain_combo_chain_skill_threshold: f32,
    #[def("MeleeRecoilMaintainComboChainStrengthThreshold")]
    pub melee_recoil_maintain_combo_chain_strength_threshold: f32,
    #[def("DefaultCombatAbilities")]
    pub default_combat_abilities: Vec<DefIndex>,
    #[def("HeavyWeapons")]
    pub heavy_weapons: BTreeMap<i32, IdleStateGroup>,
    #[def("ProjectileWeaponFirstPersonModeMaxFOVValues")]
    pub projectile_weapon_first_person_mode_max_fov_values: Vec<f32>,
    #[def("ProjectileTargetingAcquireTargetTag")]
    pub projectile_targeting_acquire_target_tag: bool,
    #[def("ProjectileTargetingAcquireTargetsAuto")]
    pub projectile_targeting_acquire_targets_auto: bool,
    #[def("ProjectileTargetingUseAnalogueZoom")]
    pub projectile_targeting_use_analogue_zoom: bool,
    #[def("ProjectileWeaponAnalogueZoomControlWeight")]
    pub projectile_weapon_analogue_zoom_control_weight: f32,
}
