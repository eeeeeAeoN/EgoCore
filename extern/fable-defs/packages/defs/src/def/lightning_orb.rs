use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct LightningOrbDef {
    #[def("LightningDefIndex")]
    pub lightning_def_index: DefIndex,
    #[def("LifeTime")]
    pub life_time: f32,
    #[def("Speed")]
    pub speed: f32,
    #[def("ExplosionRange")]
    pub explosion_range: f32,
    #[def("ExplosionDefIndex")]
    pub explosion_def_index: DefIndex,
}
