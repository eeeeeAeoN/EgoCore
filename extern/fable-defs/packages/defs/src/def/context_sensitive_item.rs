use crate::DefStruct;
use crate::enums::ContextSensitiveType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ContextSensitiveItemDef {
    #[def("Type")]
    pub type_: ContextSensitiveType,
    #[def("Priority")]
    pub priority: i32,
    #[def("SlotIndex")]
    pub slot_index: i32,
    #[def("ExtraData1")]
    pub extra_data1: i32,
    #[def("ExtraData2")]
    pub extra_data2: i32,
}
