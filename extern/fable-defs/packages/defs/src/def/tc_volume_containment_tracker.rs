use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TCVolumeContainmentTrackerDef {
    #[def("UpdateEveryFrame")]
    pub update_every_frame: bool,
    #[def("CalculateBlendedLightingChannel")]
    pub calculate_blended_lighting_channel: bool,
    #[def("LightingChannelSphereModelRadius")]
    pub lighting_channel_sphere_model_radius: f32,
    #[def("LightingChannelSphereModelZOffset")]
    pub lighting_channel_sphere_model_z_offset: f32,
}
