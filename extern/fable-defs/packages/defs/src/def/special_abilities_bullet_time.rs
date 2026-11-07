use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesBulletTimeDef {
    #[def("Level0CamDist")]
    pub level0_cam_dist: f32,
    #[def("Level1CamDist")]
    pub level1_cam_dist: f32,
    #[def("Level2CamDist")]
    pub level2_cam_dist: f32,
    #[def("Level3CamDist")]
    pub level3_cam_dist: f32,
    #[def("Level0CamHeight")]
    pub level0_cam_height: f32,
    #[def("Level1CamHeight")]
    pub level1_cam_height: f32,
    #[def("Level2CamHeight")]
    pub level2_cam_height: f32,
    #[def("Level3CamHeight")]
    pub level3_cam_height: f32,
    #[def("Level0CamFOV")]
    pub level0_cam_fov: f32,
    #[def("Level1CamFOV")]
    pub level1_cam_fov: f32,
    #[def("Level2CamFOV")]
    pub level2_cam_fov: f32,
    #[def("Level3CamFOV")]
    pub level3_cam_fov: f32,
    #[def("Level0CamSwitchTime")]
    pub level0_cam_switch_time: f32,
    #[def("Level1CamSwitchTime")]
    pub level1_cam_switch_time: f32,
    #[def("Level2CamSwitchTime")]
    pub level2_cam_switch_time: f32,
    #[def("Level3CamSwitchTime")]
    pub level3_cam_switch_time: f32,
    #[def("SlowTimeDurationLevel0")]
    pub slow_time_duration_level0: f32,
    #[def("SlowTimeDurationLevel1")]
    pub slow_time_duration_level1: f32,
    #[def("SlowTimeDurationLevel2")]
    pub slow_time_duration_level2: f32,
    #[def("SlowTimeDurationLevel3")]
    pub slow_time_duration_level3: f32,
    #[def("BulletTimeSlowdownMultiplierLevel0")]
    pub bullet_time_slowdown_multiplier_level0: i32,
    #[def("BulletTimeSlowdownMultiplierLevel1")]
    pub bullet_time_slowdown_multiplier_level1: i32,
    #[def("BulletTimeSlowdownMultiplierLevel2")]
    pub bullet_time_slowdown_multiplier_level2: i32,
    #[def("BulletTimeSlowdownMultiplierLevel3")]
    pub bullet_time_slowdown_multiplier_level3: i32,
    #[def("LevelUpTimings0")]
    pub level_up_timings0: f32,
    #[def("LevelUpTimings1")]
    pub level_up_timings1: f32,
    #[def("LevelUpTimings2")]
    pub level_up_timings2: f32,
    #[def("CastCamDist")]
    pub cast_cam_dist: f32,
    #[def("CastCamHeight")]
    pub cast_cam_height: f32,
    #[def("CastCamFOV")]
    pub cast_cam_fov: f32,
    #[def("CastCamSwitchTime")]
    pub cast_cam_switch_time: f32,
    #[def("ChargeSound")]
    pub charge_sound: DefString,
    #[def("PowerUp1Sound")]
    pub power_up1_sound: DefString,
    #[def("PowerUp2Sound")]
    pub power_up2_sound: DefString,
    #[def("PowerUp3Sound")]
    pub power_up3_sound: DefString,
    #[def("InitSound")]
    pub init_sound: DefString,
    #[def("LoopSound")]
    pub loop_sound: DefString,
    #[def("EndSound")]
    pub end_sound: DefString,
    #[def("RadialBlurThresholdColR")]
    pub radial_blur_threshold_col_r: f32,
    #[def("RadialBlurThresholdColG")]
    pub radial_blur_threshold_col_g: f32,
    #[def("RadialBlurThresholdColB")]
    pub radial_blur_threshold_col_b: f32,
    #[def("RadialBlurLength")]
    pub radial_blur_length: f32,
    #[def("RadialBlurIntensityColR")]
    pub radial_blur_intensity_col_r: f32,
    #[def("RadialBlurIntensityColG")]
    pub radial_blur_intensity_col_g: f32,
    #[def("RadialBlurIntensityColB")]
    pub radial_blur_intensity_col_b: f32,
    #[def("RadialBlurFade")]
    pub radial_blur_fade: f32,
    #[def("RadialBlurRange")]
    pub radial_blur_range: f32,
    #[def("RadialBlurTwist")]
    pub radial_blur_twist: f32,
    #[def("RadialBlurFadeTimeSecs")]
    pub radial_blur_fade_time_secs: f32,
    #[def("DamageRadialBlurThresholdColR")]
    pub damage_radial_blur_threshold_col_r: f32,
    #[def("DamageRadialBlurThresholdColG")]
    pub damage_radial_blur_threshold_col_g: f32,
    #[def("DamageRadialBlurThresholdColB")]
    pub damage_radial_blur_threshold_col_b: f32,
    #[def("DamageRadialBlurLength")]
    pub damage_radial_blur_length: f32,
    #[def("DamageRadialBlurIntensityColR")]
    pub damage_radial_blur_intensity_col_r: f32,
    #[def("DamageRadialBlurIntensityColG")]
    pub damage_radial_blur_intensity_col_g: f32,
    #[def("DamageRadialBlurIntensityColB")]
    pub damage_radial_blur_intensity_col_b: f32,
    #[def("DamageRadialBlurFade")]
    pub damage_radial_blur_fade: f32,
    #[def("DamageRadialBlurRange")]
    pub damage_radial_blur_range: f32,
    #[def("DamageRadialBlurTwist")]
    pub damage_radial_blur_twist: f32,
    #[def("DamageRadialBlurFadeTimeSecs")]
    pub damage_radial_blur_fade_time_secs: f32,
    #[def("HeroCombatSlowness")]
    pub hero_combat_slowness: f32,
    #[def("ScreenFilterAlpha")]
    pub screen_filter_alpha: Vec<f32>,
    #[def("ScreenFilterSaturation")]
    pub screen_filter_saturation: Vec<f32>,
    #[def("ScreenFilterContrast")]
    pub screen_filter_contrast: Vec<f32>,
    #[def("ScreenFilterBrightness")]
    pub screen_filter_brightness: Vec<f32>,
    #[def("ScreenFilterTintR")]
    pub screen_filter_tint_r: Vec<f32>,
    #[def("ScreenFilterTintG")]
    pub screen_filter_tint_g: Vec<f32>,
    #[def("ScreenFilterTintB")]
    pub screen_filter_tint_b: Vec<f32>,
    #[def("ScreenFilterFadeOutSecs")]
    pub screen_filter_fade_out_secs: f32,
    #[def("ScreenFilterFadeInSecs")]
    pub screen_filter_fade_in_secs: f32,
}
