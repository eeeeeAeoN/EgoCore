use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesTurncoatSpellDef {
    #[def("StaminaCostPerSec")]
    pub stamina_cost_per_sec: Vec<f32>,
    #[def("ResistanceDamagePerSec")]
    pub resistance_damage_per_sec: Vec<f32>,
    #[def("TurncoatTime")]
    pub turncoat_time: Vec<f32>,
    #[def("TurncoatRange")]
    pub turncoat_range: Vec<f32>,
    #[def("TurncoatXPUpEveryNSecs")]
    pub turncoat_xp_up_every_n_secs: Vec<f32>,
}
