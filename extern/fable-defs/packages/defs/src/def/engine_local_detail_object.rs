use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EngineLocalDetailObjectDef {
    #[def("Mesh")]
    pub mesh: i32,
    #[def("ShadowMesh")]
    pub shadow_mesh: i32,
    #[def("Probability", default = 1.0)]
    pub probability: f32,
    #[def("ThemeBlendThreshold")]
    pub theme_blend_threshold: f32,
    #[def("Scale")]
    pub scale: f32,
    #[def("ScaleRandomElement")]
    pub scale_random_element: f32,
    #[def("FadeStart")]
    pub fade_start: f32,
    #[def("FadeEnd")]
    pub fade_end: f32,
    #[def("ZSpriteMesh")]
    pub z_sprite_mesh: i32,
    #[def("ZSpriteFadeStart")]
    pub z_sprite_fade_start: f32,
    #[def("ZSpriteFadeEnd")]
    pub z_sprite_fade_end: f32,
    #[def("SlopeFadeStart")]
    pub slope_fade_start: f32,
    #[def("SlopeFadeEnd")]
    pub slope_fade_end: f32,
    #[def("AlphaRef", default = -1)]
    pub alpha_ref: i32,
    #[def("AlphaMipBias")]
    pub alpha_mip_bias: f32,
    #[def("WindSkewConstantFactor")]
    pub wind_skew_constant_factor: f32,
    #[def("WindSkewRandomFactor")]
    pub wind_skew_random_factor: f32,
    #[def("WindSkewSpeedFactor")]
    pub wind_skew_speed_factor: f32,
    #[def("CastShadows")]
    pub cast_shadows: bool,
    #[def("ReceiveShadows", default = true)]
    pub receive_shadows: bool,
    #[def("IsRepeatedMesh")]
    pub is_repeated_mesh: bool,
    #[def("IsZSprite")]
    pub is_z_sprite: bool,
    #[def("AlphaIsBoolean")]
    pub alpha_is_boolean: bool,
    #[def("HasLandscapeNormalLighting")]
    pub has_landscape_normal_lighting: bool,
    #[def("TiltToSlope")]
    pub tilt_to_slope: bool,
    #[def("HasWindSkew")]
    pub has_wind_skew: bool,
}
