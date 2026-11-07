use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct KrakenDef {
    #[def("HeightOffset", default = 10.0)]
    pub height_offset: f32,
    #[def("PoolRadius", default = 9.5)]
    pub pool_radius: f32,
    #[def("HeadRiseDelay", default = 5.0)]
    pub head_rise_delay: f32,
    #[def("HeadIdleTime", default = 3.0)]
    pub head_idle_time: f32,
    #[def("LowStrikesBeforeHigh", default = 3)]
    pub low_strikes_before_high: i32,
    #[def("LowStrikeChargeLoops", default = 1)]
    pub low_strike_charge_loops: i32,
    #[def("LowStrikeAttackLoops", default = 3)]
    pub low_strike_attack_loops: i32,
    #[def("HighStrikeAttackLoops", default = 8)]
    pub high_strike_attack_loops: i32,
    #[def("RoarLoops", default = 1)]
    pub roar_loops: i32,
    #[def("DelayBeforeLowBeam")]
    pub delay_before_low_beam: f32,
    #[def("DelayBeforeHighBeam")]
    pub delay_before_high_beam: f32,
    #[def("BeamExplosionRadius", default = 15.0)]
    pub beam_explosion_radius: f32,
    #[def("BeamTrackSpeed", default = 1.0)]
    pub beam_track_speed: f32,
    #[def("TargetingTrackSpeed", default = 5.0)]
    pub targeting_track_speed: f32,
    #[def("BeamMouthDistLow", default = 2.0)]
    pub beam_mouth_dist_low: f32,
    #[def("BeamMouthDistHigh", default = 6.0)]
    pub beam_mouth_dist_high: f32,
    #[def("TicksBetweenBeamExplosions", default = 2)]
    pub ticks_between_beam_explosions: i32,
    #[def("TentacleDefIndex")]
    pub tentacle_def_index: Vec<DefIndex>,
    #[def("TentacleMaxRiseDelay", default = 1.0)]
    pub tentacle_max_rise_delay: f32,
}
