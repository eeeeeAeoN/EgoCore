use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SmashableDef {
    #[def("Smashable")]
    pub smashable: bool,
    #[def("ReplacementObject")]
    pub replacement_object: DefIndex,
    // A particle-emitter bank id (e.g. BREAKING_WINDOW_01 = 11 in particles.h),
    // not a def reference — matches CDegradableDef.smash_particle_emitter (i32).
    #[def("SmashParticleEmitter")]
    pub smash_particle_emitter: i32,
}
