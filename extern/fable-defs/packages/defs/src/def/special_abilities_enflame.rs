use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesEnflameDef {
    #[def("LevelUpTimings0")]
    pub level_up_timings0: f32,
    #[def("LevelUpTimings1")]
    pub level_up_timings1: f32,
    #[def("LevelUpTimings2")]
    pub level_up_timings2: f32,
    #[def("Level0Damage")]
    pub level0_damage: f32,
    #[def("Level1Damage")]
    pub level1_damage: f32,
    #[def("Level2Damage")]
    pub level2_damage: f32,
    #[def("Level3Damage")]
    pub level3_damage: f32,
    #[def("RadiusOfEffect")]
    pub radius_of_effect: Vec<f32>,
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
    #[def("EnflameCombatMultiplierFactor")]
    pub enflame_combat_multiplier_factor: f32,
}
