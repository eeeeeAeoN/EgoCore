use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DialogueLayerDef {
    #[def("ResponseTimeSecsAttack")]
    pub response_time_secs_attack: f32,
    #[def("ResponseTimeSecsRelease")]
    pub response_time_secs_release: f32,
    #[def("GainLevelHigh")]
    pub gain_level_high: f32,
    #[def("GainLevelMid")]
    pub gain_level_mid: f32,
    #[def("GainLevelLow")]
    pub gain_level_low: f32,
    #[def("FocusShiftDistanceThreshold")]
    pub focus_shift_distance_threshold: f32,
}
