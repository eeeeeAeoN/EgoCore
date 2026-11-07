use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FireballSpellLevelDef {
    #[def("Level")]
    pub level: i32,
}
