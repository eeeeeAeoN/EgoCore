use crate::DefStruct;
use crate::enums::ClockHandType;
use crate::wire::VecMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ClockDef {
    #[def("Sound")]
    pub sound: VecMap<String, i32>,
    #[def("HandType")]
    pub hand_type: ClockHandType,
}
