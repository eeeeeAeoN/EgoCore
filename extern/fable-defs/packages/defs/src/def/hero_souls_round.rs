use crate::DefStruct;
use crate::def::HeroSoulsWaveDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroSoulsRoundDef {
    #[def("NumWaves")]
    pub num_waves: i32,
    #[def("Waves")]
    pub waves: Vec<HeroSoulsWaveDef>,
}
