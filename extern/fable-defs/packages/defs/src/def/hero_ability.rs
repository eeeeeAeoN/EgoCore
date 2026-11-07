use crate::DefStruct;
use crate::def::{
    enums::{HeroAbility, HeroExperienceStatCategory},
    values::Vector2D,
    wire::DefIndex,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroAbilityDef {
    #[def("Ability")]
    pub ability: HeroAbility,
    #[def("Stat")]
    pub stat: HeroExperienceStatCategory,
    #[def("IconGraphicIndex")]
    pub icon_graphic_index: i32,
    #[def("IconEffectIndex")]
    pub icon_effect_index: i32,
    #[def("IconActiveEffectIndex")]
    pub icon_active_effect_index: i32,
    #[def("IconEffectOffset")]
    pub icon_effect_offset: Vector2D,
    #[def("Name")]
    pub name: i32,
    #[def("Description")]
    pub description: i32,
    #[def("StaminaCost")]
    pub stamina_cost: Vec<i32>,
    #[def("StatExperienceGainedOnUse")]
    pub stat_experience_gained_on_use: i32,
    #[def("ExperienceCostsToUpgrade")]
    pub experience_costs_to_upgrade: Vec<i32>,
    #[def("LevDescription")]
    pub lev_description: Vec<i32>,
    #[def("ABXYPriority")]
    pub abxy_priority: i32,
    #[def("MoralityCostFactor")]
    pub morality_cost_factor: f32,
    #[def("Aggressive", default = true)]
    pub aggressive: bool,
    #[def("MaxedOutDescription")]
    pub maxed_out_description: i32,
    #[def("DummyObject")]
    pub dummy_object: DefIndex,
}
