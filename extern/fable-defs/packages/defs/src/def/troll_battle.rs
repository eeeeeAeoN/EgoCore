use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TrollBattleDef {
    #[def("GroundPoundSpeedMultiplierStart", default = 1.0)]
    pub ground_pound_speed_multiplier_start: f32,
    #[def("GroundPoundSpeedMultiplierEnd", default = 1.0)]
    pub ground_pound_speed_multiplier_end: f32,
    #[def("AttackPosPredictiveness")]
    pub attack_pos_predictiveness: f32,
    #[def("PercentageChanceToUseSecondaryShotType")]
    pub percentage_chance_to_use_secondary_shot_type: f32,
    #[def("SecondaryShotType")]
    pub secondary_shot_type: DefIndex,
    #[def("MinLaughDistance", default = 8.0)]
    pub min_laugh_distance: f32,
    #[def("MaxTimeBetweenKnockdownAndBoast", default = 10.5)]
    pub max_time_between_knockdown_and_boast: f32,
    #[def("FallDownExplosionIndex")]
    pub fall_down_explosion_index: DefIndex,
    #[def("DeathExplosionIndex")]
    pub death_explosion_index: DefIndex,
}
