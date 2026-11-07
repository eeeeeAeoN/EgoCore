use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MeleeCombatKnockdownEffects {
    #[def("BlendInFrames")]
    pub blend_in_frames: i32,
    #[def("BlendOutFrames")]
    pub blend_out_frames: i32,
    #[def("AttackerLightingChannel", default = -1)]
    pub attacker_lighting_channel: i32,
    #[def("TargetLightingChannel", default = -1)]
    pub target_lighting_channel: i32,
    #[def("EnvironmentTheme", default = -1)]
    pub environment_theme: i32,
    #[def("CameraFOVChange")]
    pub camera_fov_change: bool,
    #[def("CameraPan")]
    pub camera_pan: bool,
    #[def("AttackerAndTargetSpeedMultiplier", default = -1.0)]
    pub attacker_and_target_speed_multiplier: f32,
    #[def("PauseEntities")]
    pub pause_entities: bool,
    #[def("NonAttackingEntities")]
    pub non_attacking_entities: bool,
    #[def("AttackerInvulnerable")]
    pub attacker_invulnerable: bool,
}
