use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct KrakenTentacleDef {
    #[def("TentacleSmackDownTime", default = 8.0)]
    pub tentacle_smack_down_time: f32,
    #[def("TentacleTurnSpeed", default = 3.0)]
    pub tentacle_turn_speed: f32,
    #[def("TentacleMoveSpeed", default = 0.1)]
    pub tentacle_move_speed: f32,
    #[def("TentacleStrikeAngleTolerence", default = 5.0)]
    pub tentacle_strike_angle_tolerence: f32,
    #[def("TentacleStrikeDistTolerence", default = 0.5)]
    pub tentacle_strike_dist_tolerence: f32,
    #[def("TentacleHeightOffset", default = 0.3)]
    pub tentacle_height_offset: f32,
    #[def("TentacleTimeBeforeExplosion", default = 1.0)]
    pub tentacle_time_before_explosion: f32,
    #[def("TentacleTimeBetweenAttacks", default = 10.0)]
    pub tentacle_time_between_attacks: f32,
    #[def("TentacleTimeBeforeObstructionRemoval", default = 0.5)]
    pub tentacle_time_before_obstruction_removal: f32,
    #[def("TentacleObstructionRadius", default = 1.5)]
    pub tentacle_obstruction_radius: f32,
    #[def("TentacleExplosionBodgeAngle", default = 5.0)]
    pub tentacle_explosion_bodge_angle: f32,
    #[def("TentacleExplosionRadius", default = 10.0)]
    pub tentacle_explosion_radius: f32,
    #[def("TentacleObstructionRadiusMin", default = 5.0)]
    pub tentacle_obstruction_radius_min: f32,
    #[def("TentacleObstructionRadiusMax", default = 15.0)]
    pub tentacle_obstruction_radius_max: f32,
}
