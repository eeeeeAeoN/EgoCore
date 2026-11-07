use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AreaOfEffectAttackDef {
    #[def("TrapIndex")]
    pub trap_index: DefIndex,
    #[def("ExplosionIndex")]
    pub explosion_index: DefIndex,
    #[def("ObstructionRadius")]
    pub obstruction_radius: f32,
    #[def("ObstructionLifeTime")]
    pub obstruction_life_time: f32,
}
