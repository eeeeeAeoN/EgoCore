use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernDef {
    #[def("BedCost")]
    pub bed_cost: DefIndex,
}
