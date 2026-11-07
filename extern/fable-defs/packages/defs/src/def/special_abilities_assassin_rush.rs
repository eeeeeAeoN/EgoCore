use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesAssassinRushDef {
    #[def("DistanceAwayFromtargetToAssassinRushTo")]
    pub distance_away_fromtarget_to_assassin_rush_to: f32,
    #[def("SafeCollideRadiusPercentage")]
    pub safe_collide_radius_percentage: f32,
    #[def("DistFromFloor")]
    pub dist_from_floor: f32,
    #[def("AssassinStartSeconds")]
    pub assassin_start_seconds: f32,
    #[def("AssassinEndSeconds")]
    pub assassin_end_seconds: f32,
    #[def("AssassinThreshold")]
    pub assassin_threshold: f32,
    #[def("AssassinLength")]
    pub assassin_length: f32,
    #[def("AssassinIntensity")]
    pub assassin_intensity: f32,
    #[def("AssassinFade")]
    pub assassin_fade: f32,
    #[def("AssassinRange")]
    pub assassin_range: f32,
    #[def("AssassinTwist")]
    pub assassin_twist: f32,
    #[def("RunSpeed")]
    pub run_speed: f32,
    #[def("TrailDelaySecs")]
    pub trail_delay_secs: f32,
    #[def("SlowTimeLength")]
    pub slow_time_length: f32,
    #[def("AssassinAlpha")]
    pub assassin_alpha: i32,
    #[def("SlowTimeMultiplier")]
    pub slow_time_multiplier: i32,
    #[def("RushDelay")]
    pub rush_delay: f32,
    #[def("DashDistance")]
    pub dash_distance: Vec<f32>,
}
