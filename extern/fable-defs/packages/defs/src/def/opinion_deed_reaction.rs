use crate::DefStruct;
use crate::def::{enums::OpinionAttitudeType, wire::DefString};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionDeedReactionDef {
    #[def("Attitudes")]
    pub attitudes: Vec<OpinionAttitudeType>,
    #[def("Animation")]
    pub animation: DefString,
    #[def("DialogueTag")]
    pub dialogue_tag: u32,
}
