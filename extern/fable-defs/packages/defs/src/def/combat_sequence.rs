use crate::DefStruct;
use crate::enums::{
    ActionRegisteredType, CombatSequenceInterruptionType, CombatSequenceIsValidType,
    CombatSequenceOnStartModuleType, CombatSequenceOnStopModuleType, CombatSequenceType,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatSequenceDef {
    #[def("ValidCombatZones")]
    pub valid_combat_zones: i32,
    #[def("Type")]
    pub type_: CombatSequenceType,
    #[def("InterruptionType")]
    pub interruption_type: CombatSequenceInterruptionType,
    #[def("IsValidType")]
    pub is_valid_type: CombatSequenceIsValidType,
    #[def("OnStartType")]
    pub on_start_type: CombatSequenceOnStartModuleType,
    #[def("OnStopType")]
    pub on_stop_type: CombatSequenceOnStopModuleType,
    #[def("ActionList")]
    pub action_list: Vec<ActionRegisteredType>,
    #[def("SecondsToPredictTargetPosition")]
    pub seconds_to_predict_target_position: f32,
    #[def("PreferIfValid", default = true)]
    pub prefer_if_valid: bool,
}
