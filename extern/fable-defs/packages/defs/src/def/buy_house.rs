use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BuyHouseDef {
    #[def("Main", default = true)]
    pub main: bool,
}
