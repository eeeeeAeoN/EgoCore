use crate::DefStruct;
use crate::def::wire::DefString;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct VillageDef {
    #[def("Tasks")]
    pub tasks: Vec<i32>,
    #[def("PatrolTrackName")]
    pub patrol_track_name: DefString,
    #[def("DefaultStanceToHero")]
    pub default_stance_to_hero: i32,
    #[def("SecondsSecuritySweepLasts")]
    pub seconds_security_sweep_lasts: i32,
    #[def("WarehouseCrateDef")]
    pub warehouse_crate_def: DefIndex,
    #[def("SleepAdultStartTime")]
    pub sleep_adult_start_time: i32,
    #[def("SleepAdultEndTime")]
    pub sleep_adult_end_time: i32,
    #[def("SleepChildStartTime")]
    pub sleep_child_start_time: i32,
    #[def("SleepChildEndTime")]
    pub sleep_child_end_time: i32,
    #[def("SleepElderlyStartTime")]
    pub sleep_elderly_start_time: i32,
    #[def("SleepElderlyEndTime")]
    pub sleep_elderly_end_time: i32,
    #[def("HouseLightsOnTime")]
    pub house_lights_on_time: i32,
    #[def("HouseLightsOffTime")]
    pub house_lights_off_time: i32,
    #[def("SociableStartTime")]
    pub sociable_start_time: i32,
    #[def("SociableFinishTime")]
    pub sociable_finish_time: i32,
    #[def("CurfewStartTime")]
    pub curfew_start_time: i32,
    #[def("CurfewFinishTime")]
    pub curfew_finish_time: i32,
    #[def("StoryStartTimes")]
    pub story_start_times: Vec<i32>,
    #[def("StoryDuration")]
    pub story_duration: i32,
    #[def("SchoolStartTimes")]
    pub school_start_times: Vec<i32>,
    #[def("SchoolDuration")]
    pub school_duration: i32,
    #[def("WorkStartTimes")]
    pub work_start_times: Vec<i32>,
    #[def("WorkDuration")]
    pub work_duration: i32,
    #[def("CookStartTimes")]
    pub cook_start_times: Vec<i32>,
    #[def("CookDuration")]
    pub cook_duration: i32,
    #[def("EatStartTimes")]
    pub eat_start_times: Vec<i32>,
    #[def("EatDuration")]
    pub eat_duration: i32,
    #[def("TavernStartTimes")]
    pub tavern_start_times: Vec<i32>,
    #[def("TavernDuration")]
    pub tavern_duration: i32,
    #[def("GamingStartTimes")]
    pub gaming_start_times: Vec<i32>,
    #[def("GamingDuration")]
    pub gaming_duration: i32,
    #[def("EnableAutoPlacement", default = true)]
    pub enable_auto_placement: bool,
    #[def("AllowHouseBuying", default = true)]
    pub allow_house_buying: bool,
    #[def("HearTownCrier")]
    pub hear_town_crier: bool,
    #[def("CurfewStartSound")]
    pub curfew_start_sound: String,
    #[def("CurfewEndSound")]
    pub curfew_end_sound: String,
    #[def("WorkStartSound")]
    pub work_start_sound: String,
    #[def("WorkEndSound")]
    pub work_end_sound: String,
    #[def("BribeCost")]
    pub bribe_cost: i32,
    #[def("FramesBetweenGuardSnoop")]
    pub frames_between_guard_snoop: i32,
    #[def("FramesBetweenGuardSnoopNightShops")]
    pub frames_between_guard_snoop_night_shops: i32,
    #[def("FramesBeforeGuardSnoop")]
    pub frames_before_guard_snoop: i32,
    #[def("FramesBeforeGuardSnoopNightShops")]
    pub frames_before_guard_snoop_night_shops: i32,
}
