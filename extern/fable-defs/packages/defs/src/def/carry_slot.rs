use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CarrySlotDef {
    #[def("DummyPosName")]
    pub dummy_pos_name: DefString,
    #[def("DummyPosIndex")]
    pub dummy_pos_index: i32,
    #[def("PrimarySlot")]
    pub primary_slot: bool,
    #[def("SecondaryDummyPosName")]
    pub secondary_dummy_pos_name: DefString,
}
