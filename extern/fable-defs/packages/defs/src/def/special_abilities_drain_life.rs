use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesDrainLifeDef {
    #[def("AmountOfHealthSucked")]
    pub amount_of_health_sucked: Vec<f32>,
    #[def("NumEffects")]
    pub num_effects: Vec<i32>,
    #[def("SpellRange")]
    pub spell_range: Vec<f32>,
    #[def("ShotName")]
    pub shot_name: Vec<String>,
    #[def("SpellDelaySecs")]
    pub spell_delay_secs: f32,
}
