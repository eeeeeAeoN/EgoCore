use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OccupiableDef {
    #[def("TypeFlags")]
    pub type_flags: u32,
}
