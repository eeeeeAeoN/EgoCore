use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TurncoatDef {
    #[def("Turncoatable")]
    pub turncoatable: bool,
    #[def("TurncoatedEffect")]
    pub turncoated_effect: DefIndex,
    #[def("TurncoatActivateEffect")]
    pub turncoat_activate_effect: DefIndex,
    #[def("TurncoatDeactivateEffect")]
    pub turncoat_deactivate_effect: DefIndex,
    #[def("MinimumResistance")]
    pub minimum_resistance: f32,
    #[def("TimeDelayBeforeResistanceRecoverySecs")]
    pub time_delay_before_resistance_recovery_secs: f32,
    #[def("ResistanceRecoveryPerSec")]
    pub resistance_recovery_per_sec: f32,
    #[def("TurncoatStageEffects")]
    pub turncoat_stage_effects: Vec<i32>,
    #[def("PercentageOfHealthAsResistance")]
    pub percentage_of_health_as_resistance: f32,
    #[def("ExtraResistance")]
    pub extra_resistance: f32,
}
