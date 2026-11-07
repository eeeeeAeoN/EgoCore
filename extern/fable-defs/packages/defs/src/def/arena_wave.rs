use crate::DefStruct;
use crate::def::ArenaCreatureDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ArenaWaveDef {
    // `Transfer<long>` (decomp script_def.cpp) — a plain count, not a def ref
    // (matches the hero-souls twin HeroSoulsWaveDef.num_wave_creatures).
    #[def("NumWaveCreatures")]
    pub num_wave_creatures: i32,
    #[def("Creatures")]
    pub creatures: Vec<ArenaCreatureDef>,
    #[def("ShortWave")]
    pub short_wave: bool,
}
