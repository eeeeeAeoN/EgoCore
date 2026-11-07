use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TimeAppearanceFadeDef {
    #[def("FadeInStart")]
    pub fade_in_start: f32,
    #[def("FadeInEnd")]
    pub fade_in_end: f32,
    #[def("FadeOutStart")]
    pub fade_out_start: f32,
    #[def("FadeOutEnd")]
    pub fade_out_end: f32,
}
