use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesGhostSwordDef {
    #[def("TimeOfSummonLevel0")]
    pub time_of_summon_level0: f32,
    #[def("TimeOfSummonLevel1")]
    pub time_of_summon_level1: f32,
    #[def("TimeOfSummonLevel2")]
    pub time_of_summon_level2: f32,
    #[def("TimeOfSummonLevel3")]
    pub time_of_summon_level3: f32,
    #[def("FadeInTime")]
    pub fade_in_time: f32,
    #[def("FadeOutTime")]
    pub fade_out_time: f32,
    #[def("CreationDistance")]
    pub creation_distance: f32,
}
