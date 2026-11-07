use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThingDrainLifeShotDef {
    #[def("MaxAngle")]
    pub max_angle: f32,
    #[def("MaxDistance")]
    pub max_distance: f32,
    #[def("MaxAngleToTurnDegs")]
    pub max_angle_to_turn_degs: f32,
    #[def("InitialSpeed")]
    pub initial_speed: f32,
    #[def("SpeedRange")]
    pub speed_range: f32,
}
