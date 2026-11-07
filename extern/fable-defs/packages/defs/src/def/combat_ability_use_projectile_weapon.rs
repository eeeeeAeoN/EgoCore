use crate::DefStruct;
use crate::values::CombatAbilityData;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatAbilityUseProjectileWeaponDef {
    #[def("AbleToStrafeAndShoot")]
    pub able_to_strafe_and_shoot: bool,
    #[def("ShotAccuracyPercentage")]
    pub shot_accuracy_percentage: i32,
    #[def("CombatAbility")]
    pub combat_ability: CombatAbilityData,
}
