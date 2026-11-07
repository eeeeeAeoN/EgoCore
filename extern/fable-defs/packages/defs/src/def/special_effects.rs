use crate::DefStruct;
use crate::def::values::SpecialEffectsStringMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialEffectsDef {
    #[def("SpecialEffects")]
    pub special_effects: SpecialEffectsStringMap,
}
