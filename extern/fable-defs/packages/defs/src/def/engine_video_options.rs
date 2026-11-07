use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EngineVideoOptionsDef {
    #[def("HiresTextureMemory")]
    pub hires_texture_memory: i32,
    #[def("LODErrorTolerance")]
    pub lod_error_tolerance: f32,
    #[def("CharacterLODErrorTolerance")]
    pub character_lod_error_tolerance: f32,
    #[def("DrawDistanceMultiplier")]
    pub draw_distance_multiplier: f32,
    #[def("DrawDistanceMinimum")]
    pub draw_distance_minimum: f32,
    #[def("DrawDistanceMaximum")]
    pub draw_distance_maximum: f32,
    #[def("RepeatedMeshDrawDistanceFactor")]
    pub repeated_mesh_draw_distance_factor: f32,
    #[def("MinimumZSpriteAsMeshDistance")]
    pub minimum_z_sprite_as_mesh_distance: f32,
    #[def("MaximumZSpriteAsMeshDistance")]
    pub maximum_z_sprite_as_mesh_distance: f32,
    #[def("ZSpriteDrawDistanceMultiplier")]
    pub z_sprite_draw_distance_multiplier: f32,
    #[def("ShadowBufferSize", default = 1024)]
    pub shadow_buffer_size: i32,
    #[def("ShadowDistanceScale", default = 1.0)]
    pub shadow_distance_scale: f32,
    #[def("Enable2DDisplacement")]
    pub enable2_d_displacement: bool,
    #[def("Enable3DDisplacement")]
    pub enable3_d_displacement: bool,
    #[def("EnableGlow")]
    pub enable_glow: bool,
    #[def("EnableRadialBlur")]
    pub enable_radial_blur: bool,
    #[def("EnableWaterReflection")]
    pub enable_water_reflection: bool,
    #[def("EnableWeatherEffects")]
    pub enable_weather_effects: bool,
    #[def("EnableColourFilter")]
    pub enable_colour_filter: bool,
    #[def("WeatherDensity", default = 1.0)]
    pub weather_density: f32,
    #[def("EnableRepeatedMeshes", default = true)]
    pub enable_repeated_meshes: bool,
}
