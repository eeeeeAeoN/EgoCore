use crate::DefStruct;
use crate::values::SoundMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EntitySoundDef {
    #[def("SoundMap")]
    pub sound_map: SoundMap,
    #[def("SoundIdentifier")]
    pub sound_identifier: String,
    #[def("VoicePitchOverride", default = 1.0)]
    pub voice_pitch_override: f32,
    #[def("AnimCriteriaClipDistance", default = 22)]
    pub anim_criteria_clip_distance: i32,
}
