use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PlayerMovementDef {
    #[def("JoystickMovementStationaryBand")]
    pub joystick_movement_stationary_band: f32,
    #[def("JoystickMovementSlowBand")]
    pub joystick_movement_slow_band: f32,
    #[def("JoystickMovementWalkingBand")]
    pub joystick_movement_walking_band: f32,
    #[def("JoystickMovementJoggingBand")]
    pub joystick_movement_jogging_band: f32,
    #[def("MinRequiredAngleFor180")]
    pub min_required_angle_for180: f32,
    #[def("SecondsToStoreLastMovementFor")]
    pub seconds_to_store_last_movement_for: f32,
    #[def("NoMovementFramesToCheckFor180")]
    pub no_movement_frames_to_check_for180: i32,
    #[def("NoMovementFramesToCheckFor180PriorStraightLine")]
    pub no_movement_frames_to_check_for180_prior_straight_line: i32,
    #[def("MinNoRunSecondsForSkidHaltMove")]
    pub min_no_run_seconds_for_skid_halt_move: f32,
    #[def("NoRunningFramesToPreventSkidHalt")]
    pub no_running_frames_to_prevent_skid_halt: i32,
    #[def("MaxAllowedAngleChangeForSkidHalt")]
    pub max_allowed_angle_change_for_skid_halt: f32,
    #[def("MaxTurnSpeed")]
    pub max_turn_speed: f32,
    #[def("MaxTurnSpeedHeavyWeapon")]
    pub max_turn_speed_heavy_weapon: f32,
    #[def("LeanAngleTurnFactor")]
    pub lean_angle_turn_factor: f32,
    #[def("LeanAngleLimitAngle")]
    pub lean_angle_limit_angle: f32,
    #[def("LeanAngleStepPerFrame")]
    pub lean_angle_step_per_frame: f32,
}
