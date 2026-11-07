use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DecapitationDef {
    #[def("BodyParticleEffect")]
    pub body_particle_effect: i32,
    #[def("Head")]
    pub head: DefIndex,
}
