use crate::DefStruct;
use crate::def::ArenaWaveDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ArenaRoundDef {
    #[def("NumWaves")]
    pub num_waves: i32,
    #[def("Waves")]
    pub waves: Vec<ArenaWaveDef>,
}
