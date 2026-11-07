use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesElectrocutedEffectDef {
    #[def("FadeInTimeSecs")]
    pub fade_in_time_secs: f32,
    #[def("FadeOutTimeStartSecs")]
    pub fade_out_time_start_secs: f32,
    #[def("FadeOutTimeSecs")]
    pub fade_out_time_secs: f32,
    #[def("Color")]
    pub color: Vec<i32>,
    #[def("HighlightColor")]
    pub highlight_color: Vec<i32>,
    #[def("HighlightWidth")]
    pub highlight_width: f32,
    #[def("HeightSmall")]
    pub height_small: f32,
    #[def("HeightMedium")]
    pub height_medium: f32,
}
