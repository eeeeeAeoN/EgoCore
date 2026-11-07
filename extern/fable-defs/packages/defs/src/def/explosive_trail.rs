use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExplosiveTrailDef {
    #[def("TrapDefIndex")]
    pub trap_def_index: DefIndex,
    #[def("SecondsBetweenTraps")]
    pub seconds_between_traps: f32,
    #[def("DistanceBetweenTraps")]
    pub distance_between_traps: f32,
    #[def("SecondsForTrapPersistance")]
    pub seconds_for_trap_persistance: f32,
    #[def("SecondsTrailLifetime")]
    pub seconds_trail_lifetime: f32,
    #[def("DistanceForLateralDeviation")]
    pub distance_for_lateral_deviation: f32,
    #[def("DegreesMaxTurnTowardsTargetPerTrap")]
    pub degrees_max_turn_towards_target_per_trap: f32,
    #[def("FractionOfLifetimeToTurnTowardsTarget")]
    pub fraction_of_lifetime_to_turn_towards_target: f32,
    #[def("SecondsIcicleAttackTargetPositionPredictionTime")]
    pub seconds_icicle_attack_target_position_prediction_time: f32,
}
