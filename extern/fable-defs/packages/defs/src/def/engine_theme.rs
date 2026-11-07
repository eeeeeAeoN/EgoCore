use crate::DefStruct;
use crate::enums::{MinimapThemeType, WaterType};
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EngineThemeDef {
    #[def("BaseTexture")]
    pub base_texture: i32,
    #[def("BackgroundTexture")]
    pub background_texture: i32,
    #[def("CliffBaseTexture")]
    pub cliff_base_texture: i32,
    #[def("CliffBackgroundTexture")]
    pub cliff_background_texture: i32,
    #[def("BaseBumpMap")]
    pub base_bump_map: i32,
    #[def("CliffBumpMap")]
    pub cliff_bump_map: i32,
    #[def("BaseTextureMaxSize")]
    pub base_texture_max_size: i32,
    #[def("CliffTextureMaxSize")]
    pub cliff_texture_max_size: i32,
    #[def("BaseBumpMapMaxSize")]
    pub base_bump_map_max_size: i32,
    #[def("CliffBumpMapMaxSize")]
    pub cliff_bump_map_max_size: i32,
    #[def("BaseTextureSelfIllumination")]
    pub base_texture_self_illumination: f32,
    #[def("CliffTextureSelfIllumination")]
    pub cliff_texture_self_illumination: f32,
    #[def("GroupDef")]
    pub group_def: DefIndex,
    #[def("Height")]
    pub height: f32,
    #[def("MinCameraHeight")]
    pub min_camera_height: f32,
    #[def("CoverValue")]
    pub cover_value: f32,
    #[def("MaterialDef")]
    pub material_def: DefIndex,
    #[def("MinimapTheme")]
    pub minimap_theme: MinimapThemeType,
    #[def("LocalDetailGeneratorDef")]
    pub local_detail_generator_def: DefIndex,
    #[def("DestructionEffectDef")]
    pub destruction_effect_def: DefIndex,
    #[def("DestructionThemeDef")]
    pub destruction_theme_def: DefIndex,
    #[def("Friction", default = 1.0)]
    pub friction: f32,
    #[def("WaterHeight")]
    pub water_height: f32,
    #[def("WaterType")]
    pub water_type: WaterType,
    #[def("NoWaterThemeDef")]
    pub no_water_theme_def: DefIndex,
    #[def("Passable", default = true)]
    pub passable: bool,
}
