use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CarriedReadableDef {
    #[def("TitleTextTag")]
    pub title_text_tag: DefString,
    #[def("BodyTextTag")]
    pub body_text_tag: DefString,
}
