use crate::DefStruct;
use crate::def::{enums::OpinionDeedType, values::OpinionPersonalityTraitsPtr, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionPersonalityDef {
    #[def("PersonalityTraits")]
    pub personality_traits: OpinionPersonalityTraitsPtr,
    #[def("AttitudeEnabledDefaultForNotNone", default = true)]
    pub attitude_enabled_default_for_not_none: bool,
    #[def("AttitudeEnabled")]
    pub attitude_enabled: VecMap<OpinionDeedType, bool>,
    #[def("ToleranceToBeingHit")]
    pub tolerance_to_being_hit: f32,
}
