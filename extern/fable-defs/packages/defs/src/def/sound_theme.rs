use crate::DefStruct;
use crate::def::{values::RGBColour, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SoundThemeDef {
    #[def("AtmosIndex")]
    pub atmos_index: i32,
    #[def("MusicSet")]
    pub music_set: i32,
    #[def("EditColour")]
    pub edit_colour: RGBColour,
    #[def("Gain")]
    pub gain: f32,
    #[def("Range")]
    pub range: f32,
    #[def("Granularity")]
    pub granularity: f32,
    #[def("AtmosGroupMap")]
    pub atmos_group_map: VecMap<String, i32>,
    #[def("PlayOnlyWhenInside")]
    pub play_only_when_inside: bool,
    #[def("PlayOnlyWhenOutside")]
    pub play_only_when_outside: bool,
    #[def("PlayQuietlyWhenInside")]
    pub play_quietly_when_inside: bool,
    #[def("PlayQuietlyWhenOutside")]
    pub play_quietly_when_outside: bool,
}
