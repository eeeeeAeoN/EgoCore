use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesSummonSpellDef {
    #[def("TimeOfSummon")]
    pub time_of_summon: Vec<f32>,
    #[def("BaseSummonCreatureName")]
    pub base_summon_creature_name: DefString,
    #[def("FadeInTime")]
    pub fade_in_time: f32,
    #[def("FadeOutTime")]
    pub fade_out_time: f32,
    #[def("ResetCreatureManaCost")]
    pub reset_creature_mana_cost: i32,
    #[def("SummonTintColour")]
    pub summon_tint_colour: Vec<i32>,
    #[def("EyeLeftDummyPointName")]
    pub eye_left_dummy_point_name: String,
    #[def("EyeRightDummyPointName")]
    pub eye_right_dummy_point_name: String,
}
