use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FireHeartPatternDef {
    #[def("PadActive")]
    pub pad_active: Vec<bool>,
}
