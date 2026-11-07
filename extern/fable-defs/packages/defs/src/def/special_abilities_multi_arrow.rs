use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesMultiArrowDef {
    #[def("NumberOfShotsLevel0")]
    pub number_of_shots_level0: i32,
    #[def("NumberOfShotsLevel1")]
    pub number_of_shots_level1: i32,
    #[def("NumberOfShotsLevel2")]
    pub number_of_shots_level2: i32,
    #[def("NumberOfShotsLevel3")]
    pub number_of_shots_level3: i32,
    #[def("NumberInVolleyLevel0")]
    pub number_in_volley_level0: i32,
    #[def("NumberInVolleyLevel1")]
    pub number_in_volley_level1: i32,
    #[def("NumberInVolleyLevel2")]
    pub number_in_volley_level2: i32,
    #[def("NumberInVolleyLevel3")]
    pub number_in_volley_level3: i32,
    #[def("AmmoEffectCreationDelaySecs")]
    pub ammo_effect_creation_delay_secs: f32,
    #[def("BowHighlightWidth")]
    pub bow_highlight_width: f32,
    #[def("BowHighlightColor")]
    pub bow_highlight_color: Vec<i32>,
    #[def("MultiArrowPerArrowDamageMultiplier")]
    pub multi_arrow_per_arrow_damage_multiplier: Vec<f32>,
    #[def("CastSound")]
    pub cast_sound: DefString,
}
