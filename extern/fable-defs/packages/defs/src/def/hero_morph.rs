use crate::DefStruct;
use crate::values::{ParticleMorphs, SkeletalMorphs, TextureMorphs};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroMorphDef {
    #[def("TextureMorphs")]
    pub texture_morphs: TextureMorphs,
    #[def("SkeletalMorphs")]
    pub skeletal_morphs: SkeletalMorphs,
    #[def("ParticleMorphs")]
    pub particle_morphs: ParticleMorphs,
    #[def("IdleParticleMorphs")]
    pub idle_particle_morphs: ParticleMorphs,
}
