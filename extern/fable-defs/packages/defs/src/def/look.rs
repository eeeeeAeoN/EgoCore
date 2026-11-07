use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct LookDef {
    #[def("EyesTurnRangeXY", default = 40.0)]
    pub eyes_turn_range_xy: f32,
    #[def("EyesTurnRangeYZ", default = 20.0)]
    pub eyes_turn_range_yz: f32,
    #[def("HeadTurnRangeXY", default = 180.0)]
    pub head_turn_range_xy: f32,
    #[def("HeadTurnRangeYZUp", default = 90.0)]
    pub head_turn_range_yz_up: f32,
    #[def("HeadTurnRangeYZDown", default = 90.0)]
    pub head_turn_range_yz_down: f32,
    #[def("MaxTurnSpeed", default = 360.0)]
    pub max_turn_speed: f32,
    #[def("AllowTurningViaAnimation", default = true)]
    pub allow_turning_via_animation: bool,
    #[def("AnimationTurnSpeedMultiplier", default = 1.0)]
    pub animation_turn_speed_multiplier: f32,
    #[def("CombatMaxTurnSpeed", default = 360.0)]
    pub combat_max_turn_speed: f32,
    #[def("CombatAllowTurningViaAnimation", default = true)]
    pub combat_allow_turning_via_animation: bool,
    #[def("AllowOversteer", default = true)]
    pub allow_oversteer: bool,
    #[def("AllowUndersteer", default = true)]
    pub allow_understeer: bool,
    #[def("DrawEyePrimitives", default = true)]
    pub draw_eye_primitives: bool,
    #[def("CutOffNoLooking")]
    pub cut_off_no_looking: f32,
    #[def("AlwaysUpdateHeadPos")]
    pub always_update_head_pos: bool,
}
