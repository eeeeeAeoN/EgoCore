use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SmokeGeneratorDef {
    #[def("GenerationDistance")]
    pub generation_distance: f32,
    #[def("ParticleEmitter")]
    pub particle_emitter: DefIndex,
}
