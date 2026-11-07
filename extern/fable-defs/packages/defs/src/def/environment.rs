use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EnvironmentDef {
    #[def("ColourLookupTexture")]
    pub colour_lookup_texture: String,
    #[def("DiffuseLookupRow")]
    pub diffuse_lookup_row: i32,
    #[def("AmbientLookupRow")]
    pub ambient_lookup_row: i32,
    #[def("BacklightLookupRow")]
    pub backlight_lookup_row: i32,
    #[def("ReflectionLookupRow")]
    pub reflection_lookup_row: i32,
    #[def("MistEffectColourLookupRow")]
    pub mist_effect_colour_lookup_row: i32,
    #[def("FogColourLookupRow")]
    pub fog_colour_lookup_row: i32,
    #[def("FogAlphaLookupRow")]
    pub fog_alpha_lookup_row: i32,
    #[def("SunColourLookupRow")]
    pub sun_colour_lookup_row: i32,
    #[def("CloudColourLookupRow")]
    pub cloud_colour_lookup_row: i32,
    #[def("MoonColourLookupRow")]
    pub moon_colour_lookup_row: i32,
    #[def("StarsColourLookupRow")]
    pub stars_colour_lookup_row: i32,
    #[def("SunFlareColourLookupRow")]
    pub sun_flare_colour_lookup_row: i32,
    #[def("LensFlareColourLookupRow")]
    pub lens_flare_colour_lookup_row: i32,
    #[def("SkyGradientTopLookupRow")]
    pub sky_gradient_top_lookup_row: i32,
    #[def("SkyGradientTopAlphaLookupRow")]
    pub sky_gradient_top_alpha_lookup_row: i32,
    #[def("SkyGradientBottomLookupRow")]
    pub sky_gradient_bottom_lookup_row: i32,
    #[def("SkyGradientBottomAlphaLookupRow")]
    pub sky_gradient_bottom_alpha_lookup_row: i32,
    #[def("SunlightAttenuatorColourLookupRow")]
    pub sunlight_attenuator_colour_lookup_row: i32,
    #[def("DiffuseClampAngle")]
    pub diffuse_clamp_angle: f32,
    #[def("SunlightAttenuatorAngleFadeStart")]
    pub sunlight_attenuator_angle_fade_start: f32,
    #[def("SunlightAttenuatorAngleFadeEnd")]
    pub sunlight_attenuator_angle_fade_end: f32,
    #[def("WaterColourLookupRow")]
    pub water_colour_lookup_row: i32,
    #[def("SeaColourLookupRow")]
    pub sea_colour_lookup_row: i32,
    #[def("GlowThresholdColourLookupRow")]
    pub glow_threshold_colour_lookup_row: i32,
    #[def("GlowBloomColourLookupRow")]
    pub glow_bloom_colour_lookup_row: i32,
    #[def("SeaTexture")]
    pub sea_texture: i32,
    #[def("SeaRadius")]
    pub sea_radius: i32,
    #[def("SeaFlatSectionStart")]
    pub sea_flat_section_start: i32,
    #[def("SeaFlatSectionEnd")]
    pub sea_flat_section_end: i32,
    #[def("DayStartTime")]
    pub day_start_time: f32,
    #[def("DaySpeed")]
    pub day_speed: f32,
    #[def("MistAlphaGraphic")]
    pub mist_alpha_graphic: i32,
    #[def("MistAlphaGraphicPC")]
    pub mist_alpha_graphic_pc: i32,
    #[def("IceBumpMap")]
    pub ice_bump_map: i32,
    #[def("IceBumpMapPC")]
    pub ice_bump_map_pc: i32,
    #[def("IceTexture")]
    pub ice_texture: i32,
    #[def("WaterEdgeAlphaMap")]
    pub water_edge_alpha_map: i32,
    #[def("WaterSurfMap")]
    pub water_surf_map: i32,
    #[def("WaterBumpMapPC")]
    pub water_bump_map_pc: i32,
    #[def("WaterBumpMap")]
    pub water_bump_map: i32,
    #[def("WaterBumpMap2")]
    pub water_bump_map2: i32,
    #[def("SeaBumpMapPC")]
    pub sea_bump_map_pc: i32,
    #[def("SeaBumpMap")]
    pub sea_bump_map: i32,
    #[def("SeaBumpMap2")]
    pub sea_bump_map2: i32,
    #[def("WaterEnvMapOverlayTexture")]
    pub water_env_map_overlay_texture: i32,
    #[def("RainTexture")]
    pub rain_texture: i32,
    #[def("RainTexturePC")]
    pub rain_texture_pc: i32,
    #[def("SnowTexture")]
    pub snow_texture: i32,
    #[def("RainSplashParticle")]
    pub rain_splash_particle: i32,
    #[def("LightningFadeInDuration")]
    pub lightning_fade_in_duration: f32,
    #[def("LightningFadeOutDuration")]
    pub lightning_fade_out_duration: f32,
    #[def("LightningFlashDuration")]
    pub lightning_flash_duration: f32,
    #[def("LightningRainThreshold")]
    pub lightning_rain_threshold: f32,
    #[def("LightningTheme")]
    pub lightning_theme: DefIndex,
    #[def("CloudSpeedMultiplier")]
    pub cloud_speed_multiplier: f32,
    #[def("CloudMaxSpeed")]
    pub cloud_max_speed: f32,
    #[def("CloudTextureCoordMultiplier")]
    pub cloud_texture_coord_multiplier: f32,
    #[def("CloudTextureCoordOffset")]
    pub cloud_texture_coord_offset: f32,
    #[def("CloudHeightOffset")]
    pub cloud_height_offset: f32,
    #[def("WaterLakeMinimumFlowSpeed")]
    pub water_lake_minimum_flow_speed: f32,
    #[def("WaterLakeMaximumFlowSpeed")]
    pub water_lake_maximum_flow_speed: f32,
}
