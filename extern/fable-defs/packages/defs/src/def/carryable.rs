use crate::DefStruct;
use crate::values::Vector3D;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CarryableDef {
    #[def("ActiveCarrySlot")]
    pub active_carry_slot: DefIndex,
    #[def("SecondaryActiveCarrySlot")]
    pub secondary_active_carry_slot: DefIndex,
    #[def("PassiveCarrySlot")]
    pub passive_carry_slot: DefIndex,
    #[def("IncludeInThingScansWhenCarried")]
    pub include_in_thing_scans_when_carried: bool,
    #[def("OnKillFX")]
    pub on_kill_fx: i32,
    #[def("OffsetCoordRelativeToAttachToDummy")]
    pub offset_coord_relative_to_attach_to_dummy: Vector3D,
    #[def("OffsetAmountForThingsOnTopOfMe")]
    pub offset_amount_for_things_on_top_of_me: f32,
    #[def("PassiveCarrySlotScale", default = 1.0)]
    pub passive_carry_slot_scale: f32,
}
