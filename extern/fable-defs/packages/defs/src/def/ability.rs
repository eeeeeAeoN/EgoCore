use crate::DefStruct;
use crate::enums::HeroAbility;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AbilityDef {
    #[def("Ability")]
    pub ability: HeroAbility,
}
