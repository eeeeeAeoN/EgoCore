use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EngineDef {
    #[def("LODErrorTolerance")]
    pub lod_error_tolerance: f32,
    #[def("CharacterLODErrorTolerance")]
    pub character_lod_error_tolerance: f32,
    #[def("LODErrorFactor")]
    pub lod_error_factor: f32,
    #[def("SeaHeight")]
    pub sea_height: f32,
    #[def("LocalDetailBooleanAlphaDefaultAlphaRef")]
    pub local_detail_boolean_alpha_default_alpha_ref: i32,
    #[def("DefaultPrimitiveAlphaRef")]
    pub default_primitive_alpha_ref: i32,
    #[def("GamePrimitiveDefaultFadeStart")]
    pub game_primitive_default_fade_start: f32,
    #[def("GamePrimitiveDefaultFadeRangeRatio")]
    pub game_primitive_default_fade_range_ratio: f32,
    #[def("LocalDetailDefaultFadeStart")]
    pub local_detail_default_fade_start: f32,
    #[def("LocalDetailDefaultFadeRangeRatio")]
    pub local_detail_default_fade_range_ratio: f32,
    #[def("TestStaticMesh")]
    pub test_static_mesh: i32,
    #[def("TestAnimatedMesh")]
    pub test_animated_mesh: i32,
    #[def("TestAnim")]
    pub test_anim: i32,
    #[def("TestGraphic")]
    pub test_graphic: i32,
    #[def("FOV_2D")]
    pub fov_2_d: f32,
    #[def("InvalidTextureStandin")]
    pub invalid_texture_standin: i32,
    #[def("InvalidThemeStandin")]
    pub invalid_theme_standin: DefIndex,
}
