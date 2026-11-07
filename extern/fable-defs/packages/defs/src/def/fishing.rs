use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FishingDef {
    #[def("FishTypes")]
    pub fish_types: Vec<DefIndex>,
    #[def("FishingSpotProximity")]
    pub fishing_spot_proximity: f32,
    #[def("FishingSpotMinDepth")]
    pub fishing_spot_min_depth: f32,
    #[def("FishingSpotMinHeightDiff")]
    pub fishing_spot_min_height_diff: f32,
    #[def("FishingSpotMaxHeightDiff")]
    pub fishing_spot_max_height_diff: f32,
    #[def("PointsPerFishingLevel")]
    pub points_per_fishing_level: i32,
    #[def("SlowFishingSpeed")]
    pub slow_fishing_speed: f32,
    #[def("NormalFishingSpeed")]
    pub normal_fishing_speed: f32,
    #[def("FastFishingSpeed")]
    pub fast_fishing_speed: f32,
    #[def("FastestFishingSpeed")]
    pub fastest_fishing_speed: f32,
    #[def("FishWeightRainMult")]
    pub fish_weight_rain_mult: f32,
    #[def("FishWeightSnowMult")]
    pub fish_weight_snow_mult: f32,
    #[def("FishWeightMistMult")]
    pub fish_weight_mist_mult: f32,
    #[def("BaseWaitPeriodSeconds")]
    pub base_wait_period_seconds: f32,
    #[def("MaxRandomWaitPeriodSeconds")]
    pub max_random_wait_period_seconds: f32,
    #[def("HookWindowSeconds")]
    pub hook_window_seconds: f32,
    #[def("BaseHookPresses")]
    pub base_hook_presses: i32,
    #[def("MaxRandomHookPresses")]
    pub max_random_hook_presses: i32,
    #[def("StrainIncreasePerPress")]
    pub strain_increase_per_press: f32,
}
