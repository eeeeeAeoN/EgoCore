use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CardDef {
    #[def("CardName")]
    pub card_name: DefIndex,
    #[def("CardVal")]
    pub card_val: DefIndex,
}
