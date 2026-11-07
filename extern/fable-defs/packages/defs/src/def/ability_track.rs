use crate::DefStruct;
use crate::def::AbilityTrackLevelDef;
use crate::enums::HeroExperienceStatCategory;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AbilityTrackDef {
    #[def("ExperienceStat")]
    pub experience_stat: HeroExperienceStatCategory,
    #[def("Abilities")]
    pub abilities: Vec<AbilityTrackLevelDef>,
    #[def("MaxedOutDescription")]
    pub maxed_out_description: i32,
}
