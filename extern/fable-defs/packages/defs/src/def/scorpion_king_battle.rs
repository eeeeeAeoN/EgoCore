use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ScorpionKingBattleDef {
    #[def("NumChargesBeforeTailStrike")]
    pub num_charges_before_tail_strike: i32,
    #[def("UnderGroundTime")]
    pub under_ground_time: f32,
    #[def("TailPlungeTime")]
    pub tail_plunge_time: f32,
    #[def("SpikeTime")]
    pub spike_time: f32,
    #[def("SpikeDecisionTime")]
    pub spike_decision_time: f32,
    #[def("TailStrikePrepareTime")]
    pub tail_strike_prepare_time: f32,
    #[def("DamageToCauseTailStrike")]
    pub damage_to_cause_tail_strike: f32,
    #[def("ChargeDamage", default = 10.0)]
    pub charge_damage: f32,
    #[def("ClawObstructionRadius", default = 2.0)]
    pub claw_obstruction_radius: f32,
}
