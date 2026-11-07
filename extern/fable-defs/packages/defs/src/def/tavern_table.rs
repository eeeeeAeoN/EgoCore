use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernTableDef {
    #[def("AddTankards")]
    pub add_tankards: bool,
    #[def("HighQualityTankards")]
    pub high_quality_tankards: bool,
}
