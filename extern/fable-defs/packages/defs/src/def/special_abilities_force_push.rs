use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesForcePushDef {
    #[def("MaxDistanceToPush")]
    pub max_distance_to_push: f32,
    #[def("MinDistanceToPush")]
    pub min_distance_to_push: f32,
    #[def("DustBoxWidth")]
    pub dust_box_width: f32,
    #[def("MaxNumOfExtraEffects")]
    pub max_num_of_extra_effects: i32,
    #[def("MinNumOfExtraEffects")]
    pub min_num_of_extra_effects: i32,
    #[def("LevelUpTimings0")]
    pub level_up_timings0: f32,
    #[def("LevelUpTimings1")]
    pub level_up_timings1: f32,
    #[def("LevelUpTimings2")]
    pub level_up_timings2: f32,
    #[def("ReleaseLevel1Delay")]
    pub release_level1_delay: f32,
    #[def("ReleaseLevel2Delay")]
    pub release_level2_delay: f32,
    #[def("ReleaseLevel3DelayA")]
    pub release_level3_delay_a: f32,
    #[def("ReleaseLevel3DelayB")]
    pub release_level3_delay_b: f32,
    #[def("ReleaseLevel4Delay")]
    pub release_level4_delay: f32,
    #[def("HeightOffGround")]
    pub height_off_ground: f32,
    #[def("Level4OffsetMagnitude")]
    pub level4_offset_magnitude: f32,
    #[def("RadiusOfEffect")]
    pub radius_of_effect: Vec<f32>,
    #[def("ExtraPush")]
    pub extra_push: f32,
    #[def("PushSpeed")]
    pub push_speed: f32,
    #[def("ObjectDamage")]
    pub object_damage: Vec<f32>,
    #[def("TimeDelayBetweenCastsSec")]
    pub time_delay_between_casts_sec: Vec<f32>,
}
