use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct JackDragonDef {
    #[def("MaxHitsBeforeFlyAway")]
    pub max_hits_before_fly_away: i32,
    #[def("MinTimeBetweenSlams")]
    pub min_time_between_slams: f32,
    #[def("MaxFirebeamDistance")]
    pub max_firebeam_distance: f32,
    #[def("MinSnapDistance")]
    pub min_snap_distance: f32,
    #[def("MaxSnapDistance")]
    pub max_snap_distance: f32,
    #[def("MaxTimeAtLongRangeBeforeFlyAway")]
    pub max_time_at_long_range_before_fly_away: f32,
    #[def("MaxSlamDistance")]
    pub max_slam_distance: f32,
    #[def("MaxHoverLoops")]
    pub max_hover_loops: i32,
    #[def("PercentChanceOfFakeLandingSwoop", default = 50.0)]
    pub percent_chance_of_fake_landing_swoop: f32,
    #[def("MaxTimeOnGround")]
    pub max_time_on_ground: f32,
    #[def("DragonGroundHeightOffsetBodge")]
    pub dragon_ground_height_offset_bodge: f32,
    #[def("HoverBackOffOffset")]
    pub hover_back_off_offset: f32,
    #[def("FramesBetweenFireBeamUpdate", default = 4)]
    pub frames_between_fire_beam_update: i32,
    #[def("FramesBetweenBurninateUpdate", default = 2)]
    pub frames_between_burninate_update: i32,
    #[def("HoverObstructionRadius")]
    pub hover_obstruction_radius: f32,
    #[def("StrafeTriggerDistance", default = 23.0)]
    pub strafe_trigger_distance: f32,
    #[def("GetHitSnapBuildupTime")]
    pub get_hit_snap_buildup_time: f32,
    #[def("NotHitSnapBuildupTime")]
    pub not_hit_snap_buildup_time: f32,
    #[def("MaxTimeBeforeSnap")]
    pub max_time_before_snap: f32,
    #[def("MinRoarDistance")]
    pub min_roar_distance: f32,
}
