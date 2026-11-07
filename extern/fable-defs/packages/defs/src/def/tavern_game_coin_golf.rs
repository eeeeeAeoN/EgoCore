use crate::DefStruct;
use crate::def::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernGameCoinGolfDef {
    #[def("TargetDistanceScale")]
    pub target_distance_scale: f32,
    #[def("JoystickScaleXboxX")]
    pub joystick_scale_xbox_x: f32,
    #[def("JoystickScaleXboxY")]
    pub joystick_scale_xbox_y: f32,
    #[def("JoystickScaleX")]
    pub joystick_scale_x: f32,
    #[def("JoystickScaleY")]
    pub joystick_scale_y: f32,
    #[def("JoystickMagBelow")]
    pub joystick_mag_below: f32,
    #[def("AverageJoystickMagAbove")]
    pub average_joystick_mag_above: f32,
    #[def("AverageDrift")]
    pub average_drift: f32,
    #[def("MaxPower")]
    pub max_power: f32,
    #[def("RestSpeed")]
    pub rest_speed: f32,
    #[def("Courses")]
    pub courses: DefIndex,
    #[def("GoalRadius")]
    pub goal_radius: f32,
    #[def("StartingCourse")]
    pub starting_course: DefIndex,
}
