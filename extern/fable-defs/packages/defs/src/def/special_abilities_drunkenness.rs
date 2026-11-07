use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesDrunkennessDef {
    #[def("DrunkenGraphic")]
    pub drunken_graphic: i32,
    #[def("MaxAlpha")]
    pub max_alpha: i32,
    #[def("USpeed")]
    pub u_speed: f32,
    #[def("VSpeed")]
    pub v_speed: f32,
    #[def("RotSpeed")]
    pub rot_speed: f32,
}
