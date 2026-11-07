use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EnvironmentThemeDef {
    #[def("TimeOfDay")]
    pub time_of_day: f32,
    #[def("MoonLit")]
    pub moon_lit: bool,
    #[def("FogStartZ")]
    pub fog_start_z: f32,
    #[def("FogEndZ")]
    pub fog_end_z: f32,
    #[def("SkyTexture0")]
    pub sky_texture0: i32,
    #[def("SkyTexture1")]
    pub sky_texture1: i32,
    #[def("SkyTexture1Blend")]
    pub sky_texture1_blend: f32,
    #[def("CloudLowerLayerTexture0")]
    pub cloud_lower_layer_texture0: i32,
    #[def("CloudLowerLayerTexture1")]
    pub cloud_lower_layer_texture1: i32,
    #[def("CloudLowerLayerTexture1Blend")]
    pub cloud_lower_layer_texture1_blend: f32,
    #[def("CloudLowerLayerSpeedMultiplier")]
    pub cloud_lower_layer_speed_multiplier: f32,
    #[def("CloudUpperLayerTexture0")]
    pub cloud_upper_layer_texture0: i32,
    #[def("CloudUpperLayerTexture1")]
    pub cloud_upper_layer_texture1: i32,
    #[def("CloudUpperLayerTexture1Blend")]
    pub cloud_upper_layer_texture1_blend: f32,
    #[def("CloudUpperLayerSpeedMultiplier")]
    pub cloud_upper_layer_speed_multiplier: f32,
    #[def("WaterColourToReflectionBlend")]
    pub water_colour_to_reflection_blend: f32,
    #[def("WaterAlphaFactor")]
    pub water_alpha_factor: f32,
    #[def("WaterSpecularHightlightFactor")]
    pub water_specular_hightlight_factor: f32,
    #[def("RainStrength")]
    pub rain_strength: f32,
    #[def("SnowStrength")]
    pub snow_strength: f32,
    #[def("MistAlpha")]
    pub mist_alpha: f32,
    #[def("LightningFrequency")]
    pub lightning_frequency: f32,
    #[def("GlowThresholdScale")]
    pub glow_threshold_scale: f32,
    #[def("GlowBloomScale")]
    pub glow_bloom_scale: f32,
    #[def("GlowMotionBlur")]
    pub glow_motion_blur: f32,
    #[def("ShadowFactor")]
    pub shadow_factor: f32,
    #[def("FadedShadowFactor")]
    pub faded_shadow_factor: f32,
    #[def("WaterRefractionBlendStart")]
    pub water_refraction_blend_start: f32,
    #[def("WaterRefractionBlendEnd", default = 15.0)]
    pub water_refraction_blend_end: f32,
    #[def("WaterFlowSpeedFactor", default = 0.25)]
    pub water_flow_speed_factor: f32,
    #[def("WaterOscilationSpeed", default = 0.05)]
    pub water_oscilation_speed: f32,
    #[def("WaterSwellFactor", default = 0.0625)]
    pub water_swell_factor: f32,
    #[def("WaterShoreSwellFactor", default = 0.041666668)]
    pub water_shore_swell_factor: f32,
    #[def("WaterReflectionOffset", default = 0.01)]
    pub water_reflection_offset: f32,
}
