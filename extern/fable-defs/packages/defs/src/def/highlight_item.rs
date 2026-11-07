use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HighlightItemDef {
    #[def("StartAsRunning")]
    pub start_as_running: bool,
    #[def("ParticleEmitter")]
    pub particle_emitter: i32,
    #[def("PickupParticleEmitter")]
    pub pickup_particle_emitter: i32,
    #[def("RotationSpeed", default = 3.0)]
    pub rotation_speed: f32,
    #[def("DistanceFromGround", default = 1.0)]
    pub distance_from_ground: f32,
    #[def("BobTime", default = 1.0)]
    pub bob_time: f32,
    #[def("BobDistance", default = 0.5)]
    pub bob_distance: f32,
    #[def("StartAsRunningDelay", default = 3.0)]
    pub start_as_running_delay: f32,
}
