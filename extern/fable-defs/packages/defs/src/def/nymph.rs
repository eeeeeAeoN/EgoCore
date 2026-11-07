use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct NymphDef {
    #[def("MinAttackRange")]
    pub min_attack_range: f32,
    #[def("MinHeight")]
    pub min_height: f32,
    #[def("MaxHeight")]
    pub max_height: f32,
    #[def("CircleRange")]
    pub circle_range: f32,
    #[def("FlyAroundTime")]
    pub fly_around_time: i32,
    #[def("ShotTrapType")]
    pub shot_trap_type: DefIndex,
    #[def("ShotsBeforeIdle")]
    pub shots_before_idle: i32,
    #[def("IdleTimeBetweenShots", default = 1.0)]
    pub idle_time_between_shots: f32,
    #[def("FadeInTime", default = 1.0)]
    pub fade_in_time: f32,
    #[def("FadeOutTime", default = 1.0)]
    pub fade_out_time: f32,
}
