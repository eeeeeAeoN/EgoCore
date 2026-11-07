use crate::DefStruct;
use crate::def::wire::VecMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct RumbleDef {
    #[def("QuakeIntensities")]
    pub quake_intensities: VecMap<i32, f32>,
    #[def("QuakeDurations")]
    pub quake_durations: VecMap<i32, f32>,
}
