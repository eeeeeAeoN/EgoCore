use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SnowTrollDef {
    #[def("IcicleAttackIndex")]
    pub icicle_attack_index: DefIndex,
    #[def("SecondsIcicleAttackTargetPositionPredictionTime")]
    pub seconds_icicle_attack_target_position_prediction_time: f32,
    #[def("DistancePoundGroundRange")]
    pub distance_pound_ground_range: f32,
    #[def("SecondsForIcicleAssault")]
    pub seconds_for_icicle_assault: f32,
    #[def("SecondsLeadIntoIcicles")]
    pub seconds_lead_into_icicles: f32,
    #[def("SecondsBetweenIcicles")]
    pub seconds_between_icicles: f32,
    #[def("SecondsBetweenWarningFXAndTrigger")]
    pub seconds_between_warning_fx_and_trigger: f32,
    #[def("SecondsForTrapToRemain")]
    pub seconds_for_trap_to_remain: f32,
    #[def("DistanceRange")]
    pub distance_range: f32,
    #[def("DistanceAroundTarget")]
    pub distance_around_target: f32,
    #[def("DistanceExclusionAroundIcicles")]
    pub distance_exclusion_around_icicles: f32,
    #[def("IcicleAssaultWarningFXIndex")]
    pub icicle_assault_warning_fx_index: i32,
    #[def("IcicleAssaultTrapIndex")]
    pub icicle_assault_trap_index: DefIndex,
    #[def("IceTrailDistanceLimit", default = -1.0)]
    pub ice_trail_distance_limit: f32,
}
