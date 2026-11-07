use crate::DefStruct;
use crate::def::values::TextureReplacementEntry;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TextureReplacementDef {
    #[def("Entries")]
    pub entries: Vec<TextureReplacementEntry>,
}
