use crate::DefStruct;
use crate::values::CombatAbilityData;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatAbilityStrafeDef {
    #[def("RotationSpeed", default = 20.0)]
    pub rotation_speed: f32,
    #[def("CombatAbility")]
    pub combat_ability: CombatAbilityData,
}
