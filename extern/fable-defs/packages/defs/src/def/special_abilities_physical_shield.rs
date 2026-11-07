use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesPhysicalShieldDef {
    #[def("InitSound")]
    pub init_sound: DefString,
    #[def("PowerUp1Sound")]
    pub power_up1_sound: DefString,
    #[def("PowerUp2Sound")]
    pub power_up2_sound: DefString,
    #[def("PowerUp3Sound")]
    pub power_up3_sound: DefString,
    #[def("CastSound")]
    pub cast_sound: DefString,
    #[def("LoopingSoundLevel0")]
    pub looping_sound_level0: DefString,
    #[def("LoopingSoundLevel1")]
    pub looping_sound_level1: DefString,
    #[def("LoopingSoundLevel2")]
    pub looping_sound_level2: DefString,
    #[def("LoopingSoundLevel3")]
    pub looping_sound_level3: DefString,
    #[def("EndSound")]
    pub end_sound: DefString,
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
    #[def("Level0Health")]
    pub level0_health: f32,
    #[def("Level1Health")]
    pub level1_health: f32,
    #[def("Level2Health")]
    pub level2_health: f32,
    #[def("Level3Health")]
    pub level3_health: f32,
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
    #[def("MinWidth")]
    pub min_width: f32,
    #[def("MaxWidth")]
    pub max_width: Vec<f32>,
    #[def("ColorStartLevel1")]
    pub color_start_level1: Vec<f32>,
    #[def("ColorStartLevel2")]
    pub color_start_level2: Vec<f32>,
    #[def("ColorStartLevel3")]
    pub color_start_level3: Vec<f32>,
    #[def("ColorStartLevel4")]
    pub color_start_level4: Vec<f32>,
    #[def("ColorEnd")]
    pub color_end: Vec<f32>,
    #[def("ColorHit")]
    pub color_hit: Vec<f32>,
    #[def("WobbleRate")]
    pub wobble_rate: f32,
    #[def("FadeOutPhysicalShieldTimeSecs")]
    pub fade_out_physical_shield_time_secs: f32,
    #[def("BeenHitExtraWidth")]
    pub been_hit_extra_width: f32,
    #[def("BeenHitFadeOffTimeSecs")]
    pub been_hit_fade_off_time_secs: f32,
    #[def("ManaDamage")]
    pub mana_damage: Vec<f32>,
    #[def("ShieldCoversCarriedItems")]
    pub shield_covers_carried_items: bool,
}
