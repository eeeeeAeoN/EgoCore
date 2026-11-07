use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ResurrectionItemDef {
    // A particles.h bank id (`POTION_RESURRECTION_01 = 756`), not a def ref — `i32`.
    #[def("OnUseParticleEffect")]
    pub on_use_particle_effect: i32,
}
