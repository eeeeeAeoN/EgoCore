use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ArenaCreatureDef {
    #[def("CreatureType")]
    pub creature_type: String,
    // NumCreatures/DeathScore are `Transfer<long>` (decomp script_def.cpp) —
    // plain i32 counts/scores, not def references.
    #[def("NumCreatures")]
    pub num_creatures: i32,
    #[def("HUDType")]
    pub hud_type: String,
    #[def("DeathScore")]
    pub death_score: i32,
}
