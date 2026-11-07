use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WaspQueenBattleDef {
    #[def("NumFramesPauseBetweenAttacks")]
    pub num_frames_pause_between_attacks: i32,
}
