use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WhisperBattleDef {
    #[def("NumGetHitsBeforeSommersault")]
    pub num_get_hits_before_sommersault: i32,
    #[def("NumPhase2Potions")]
    pub num_phase2_potions: i32,
}
