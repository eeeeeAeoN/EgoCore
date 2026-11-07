use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct StealthDef {
    #[def("DistanceMovementCanBeHeardFrom")]
    pub distance_movement_can_be_heard_from: f32,
    #[def("RunningNoiseMultiplier", default = 1.0)]
    pub running_noise_multiplier: f32,
    #[def("JoggingNoiseMultiplier", default = 1.0)]
    pub jogging_noise_multiplier: f32,
    #[def("WalkingNoiseMultiplier", default = 1.0)]
    pub walking_noise_multiplier: f32,
    #[def("DayTimeLampVisibilityMultiplier", default = 1.0)]
    pub day_time_lamp_visibility_multiplier: f32,
    #[def("NightTimeLampVisibilityMultiplier", default = 1.0)]
    pub night_time_lamp_visibility_multiplier: f32,
    #[def("DangerFeedbackHangtimeSeconds", default = 1.0)]
    pub danger_feedback_hangtime_seconds: f32,
    #[def("RunningAwayFromCombatHangtimeSeconds", default = 1.0)]
    pub running_away_from_combat_hangtime_seconds: f32,
}
