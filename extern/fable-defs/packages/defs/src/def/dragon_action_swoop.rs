use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DragonActionSwoopDef {
    #[def("IntoDelay")]
    pub into_delay: f32,
    #[def("OutOfDelay")]
    pub out_of_delay: f32,
    #[def("Distance")]
    pub distance: f32,
    #[def("MidPointsControlOffset")]
    pub mid_points_control_offset: f32,
    #[def("StartHeight")]
    pub start_height: f32,
    #[def("MidHeight")]
    pub mid_height: f32,
    #[def("EndHeight")]
    pub end_height: f32,
    #[def("FlightSpeed")]
    pub flight_speed: f32,
    #[def("MaxSplineProgressForSplineAdjustment")]
    pub max_spline_progress_for_spline_adjustment: f32,
    #[def("MaxSplineProgressForBurninationEffect")]
    pub max_spline_progress_for_burnination_effect: f32,
    #[def("MinSplineProgressForWingFlap")]
    pub min_spline_progress_for_wing_flap: f32,
    #[def("MaxExplosionRange")]
    pub max_explosion_range: f32,
    #[def("ExplosionLineProjectionAngle")]
    pub explosion_line_projection_angle: f32,
    #[def("FramesBetweenExplosions")]
    pub frames_between_explosions: i32,
    #[def("WindSpeed")]
    pub wind_speed: f32,
}
