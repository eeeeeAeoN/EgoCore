use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HasNameDef {
    #[def("DefaultNameTag")]
    pub default_name_tag: u32,
    #[def("Home")]
    pub home: u32,
    #[def("Occupation")]
    pub occupation: u32,
}
