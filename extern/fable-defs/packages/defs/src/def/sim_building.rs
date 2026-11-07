use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SimBuildingDef {
    #[def("Property")]
    pub property: i32,
    #[def("WorkSpaces")]
    pub work_spaces: i32,
    #[def("LiveSpaces")]
    pub live_spaces: i32,
}
