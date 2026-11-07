use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesThunderLightningStormDef {
    #[def("NumBeams", default = 5)]
    pub num_beams: i32,
    #[def("StartRadius", default = 1.0)]
    pub start_radius: f32,
    #[def("EndRadius", default = 10.0)]
    pub end_radius: f32,
    #[def("SkyBeamStartTime", default = 1.0)]
    pub sky_beam_start_time: f32,
    #[def("MainBeamsStartTime", default = 3.0)]
    pub main_beams_start_time: f32,
    #[def("BeamRotationStartSpeed", default = 0.5)]
    pub beam_rotation_start_speed: f32,
    #[def("BeamRotationEndSpeed", default = 0.5)]
    pub beam_rotation_end_speed: f32,
    #[def("TotalLifetime", default = 6.0)]
    pub total_lifetime: f32,
    #[def("Damage", default = 5.0)]
    pub damage: f32,
    #[def("InitSound")]
    pub init_sound: DefString,
    #[def("LoopingSound")]
    pub looping_sound: DefString,
}
