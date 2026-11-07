use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ConfigOptionsDefaultsDef {
    #[def("Antialiasing")]
    pub antialiasing: u32,
    #[def("ResolutionWidth", default = 1024)]
    pub resolution_width: i32,
    #[def("ResolutionHeight", default = 768)]
    pub resolution_height: i32,
    #[def("BitDepth", default = 16)]
    pub bit_depth: i32,
    #[def("TextureDetail", default = 1.0)]
    pub texture_detail: f32,
    #[def("MaxTextureDetail", default = 3.0)]
    pub max_texture_detail: f32,
    #[def("ShadowDetail", default = 1.0)]
    pub shadow_detail: f32,
    #[def("MaxShadowDetail", default = 3.0)]
    pub max_shadow_detail: f32,
    #[def("MeshDetail", default = 1.0)]
    pub mesh_detail: f32,
    #[def("MaxMeshDetail", default = 3.0)]
    pub max_mesh_detail: f32,
    #[def("EffectsDetail", default = 1.0)]
    pub effects_detail: f32,
    #[def("MaxEffectsDetail", default = 3.0)]
    pub max_effects_detail: f32,
    #[def("MinResolutionWidth", default = 1024)]
    pub min_resolution_width: i32,
    #[def("MinResolutionHeight", default = 768)]
    pub min_resolution_height: i32,
}
