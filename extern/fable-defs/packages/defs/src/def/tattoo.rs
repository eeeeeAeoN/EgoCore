use crate::DefStruct;
use crate::def::enums::CompositeBlendType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TattooDef {
    #[def("TattooName")]
    pub tattoo_name: String,
    #[def("TextureLayer")]
    pub texture_layer: i32,
    #[def("ReplacingTextureIndex")]
    pub replacing_texture_index: i32,
    #[def("BankIndex")]
    pub bank_index: i32,
    #[def("Blend", default = CompositeBlendType::ALPHA)]
    pub blend: CompositeBlendType,
    #[def("RemovesAllTattoos")]
    pub removes_all_tattoos: bool,
    #[def("Permanent")]
    pub permanent: bool,
    #[def("CoversBodyAreaFlags")]
    pub covers_body_area_flags: i32,
    #[def("Scariness")]
    pub scariness: f32,
    #[def("Attractiveness")]
    pub attractiveness: f32,
    #[def("Goodstrength")]
    pub goodstrength: f32,
    #[def("VisibilityMultiplier", default = 1.0)]
    pub visibility_multiplier: f32,
    #[def("SpecificCoversBodyAreaFlags")]
    pub specific_covers_body_area_flags: i32,
    #[def("CustomFileName")]
    pub custom_file_name: String,
    #[def("CustomFileNameBase")]
    pub custom_file_name_base: String,
}
