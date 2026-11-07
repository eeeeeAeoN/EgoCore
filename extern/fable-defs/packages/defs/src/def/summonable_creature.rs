use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SummonableCreatureDef {
    #[def("Rank")]
    pub rank: f32,
    #[def("SummonedBrain")]
    pub summoned_brain: DefString,
}
