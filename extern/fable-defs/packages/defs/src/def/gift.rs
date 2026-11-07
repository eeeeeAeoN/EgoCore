use crate::DefStruct;
use crate::enums::GiftType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct GiftDef {
    #[def("GiftType")]
    pub gift_type: GiftType,
    #[def("IsWeddingRing")]
    pub is_wedding_ring: bool,
}
