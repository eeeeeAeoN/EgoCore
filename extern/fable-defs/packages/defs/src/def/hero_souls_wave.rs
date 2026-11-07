use crate::DefStruct;
use crate::def::HeroSoulsCreatureDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroSoulsWaveDef {
    #[def("NumWaveCreatures")]
    pub num_wave_creatures: i32,
    #[def("Creatures")]
    pub creatures: Vec<HeroSoulsCreatureDef>,
}
