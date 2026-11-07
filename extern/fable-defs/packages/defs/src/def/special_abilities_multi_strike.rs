use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesMultiStrikeDef {
    #[def("SoundLevel0")]
    pub sound_level0: DefString,
    #[def("SoundLevel1")]
    pub sound_level1: DefString,
    #[def("SoundLevel2")]
    pub sound_level2: DefString,
    #[def("SoundLevel3")]
    pub sound_level3: DefString,
    #[def("AnimationLevel0")]
    pub animation_level0: DefString,
    #[def("AnimationLevel1")]
    pub animation_level1: DefString,
    #[def("AnimationLevel2")]
    pub animation_level2: DefString,
    #[def("AnimationLevel3")]
    pub animation_level3: DefString,
    #[def("HighlightWidth")]
    pub highlight_width: f32,
    #[def("HighlightColor")]
    pub highlight_color: Vec<i32>,
    #[def("DelayTimeSecs")]
    pub delay_time_secs: Vec<f32>,
    #[def("NumHits")]
    pub num_hits: Vec<i32>,
    #[def("PerHitDamagerMultiplier")]
    pub per_hit_damager_multiplier: Vec<f32>,
    #[def("AttachmentPoint")]
    pub attachment_point: Vec<String>,
}
