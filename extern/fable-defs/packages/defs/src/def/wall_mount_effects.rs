use crate::DefStruct;
use crate::def::{enums::WallMountEffects, wire::DefIndex};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WallMountEffectsDef {
    #[def("Effect")]
    pub effect: WallMountEffects,
    #[def("HealingAmount")]
    pub healing_amount: f32,
    #[def("SecondsBetweenEffectUsesAllowed")]
    pub seconds_between_effect_uses_allowed: DefIndex,
}
