use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PlayerInventoryDef {
    #[def("Slot")]
    pub slot: Vec<i32>,
    #[def("MaxInSlot")]
    pub max_in_slot: Vec<i32>,
    #[def("InitialAmount")]
    pub initial_amount: Vec<i32>,
    #[def("IsUsable")]
    pub is_usable: Vec<bool>,
    #[def("InitialSelectedSlot")]
    pub initial_selected_slot: i32,
}
