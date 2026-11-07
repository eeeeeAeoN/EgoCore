use crate::DefStruct;
use crate::enums::WeaponClass;
use crate::values::CombatAbilityData;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatAbilityBlockDefBase {
    #[def("BlockAngle", default = 120.0)]
    pub block_angle: f32,
    #[def("BlockCanBeBroken", default = true)]
    pub block_can_be_broken: bool,
    #[def("ForceRecoilWhenBlocking", default = true)]
    pub force_recoil_when_blocking: bool,
    #[def("CombatAbility")]
    pub combat_ability: CombatAbilityData,
    #[def("ValidBlockWeaponTypes")]
    pub valid_block_weapon_types: Vec<WeaponClass>,
}
