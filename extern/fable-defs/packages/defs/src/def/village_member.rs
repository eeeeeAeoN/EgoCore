use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct VillageMemberDef {
    #[def("NeedsVillageToFunctionCorrectly")]
    pub needs_village_to_function_correctly: bool,
}
