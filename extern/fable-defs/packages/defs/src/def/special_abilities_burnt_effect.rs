use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesBurntEffectDef {
    #[def("FadeInTimeSecs")]
    pub fade_in_time_secs: f32,
    #[def("FadeOutTimeStartSecs")]
    pub fade_out_time_start_secs: f32,
    #[def("FadeOutTimeSecs")]
    pub fade_out_time_secs: f32,
    #[def("Color")]
    pub color: Vec<i32>,
}
