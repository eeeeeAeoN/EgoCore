use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CrateStackDef {
    #[def("StackType")]
    pub stack_type: i32,
    #[def("Priority")]
    pub priority: i32,
}
