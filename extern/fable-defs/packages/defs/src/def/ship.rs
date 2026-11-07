use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ShipDef {
    #[def("SightingFrequency", default = 1)]
    pub sighting_frequency: i32,
    #[def("SightingOffset")]
    pub sighting_offset: i32,
    #[def("DaysVisible", default = 1)]
    pub days_visible: i32,
    #[def("Swell")]
    pub swell: f32,
    #[def("Pitch")]
    pub pitch: f32,
    #[def("Roll")]
    pub roll: f32,
    #[def("WaveLength", default = 1.0)]
    pub wave_length: f32,
}
