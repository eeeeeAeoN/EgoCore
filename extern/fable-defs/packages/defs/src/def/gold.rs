use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct GoldDef {
    #[def("Gold")]
    pub gold: i32,
}
