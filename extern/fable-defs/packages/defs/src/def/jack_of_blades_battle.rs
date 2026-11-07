use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct JackOfBladesBattleDef {
    #[def("IdealShootDistance")]
    pub ideal_shoot_distance: f32,
    #[def("NumGetHitsBeforeRush")]
    pub num_get_hits_before_rush: i32,
    #[def("TimeBetweenShots")]
    pub time_between_shots: f32,
    #[def("ScreamInAirTime", default = 3.0)]
    pub scream_in_air_time: f32,
    #[def("TimeBetweenPillars", default = 0.5)]
    pub time_between_pillars: f32,
    #[def("BattleFloatHeight")]
    pub battle_float_height: f32,
    #[def("BattleScale", default = 1.0)]
    pub battle_scale: f32,
    #[def("ScaleUpTime")]
    pub scale_up_time: f32,
    #[def("ScaleDownTime")]
    pub scale_down_time: f32,
    #[def("DeathScreamTime", default = 5.0)]
    pub death_scream_time: f32,
    #[def("DeathHeight", default = 5.0)]
    pub death_height: f32,
    #[def("BurnDamage", default = 3.0)]
    pub burn_damage: f32,
    #[def("FlashDuration", default = 2.0)]
    pub flash_duration: f32,
    #[def("FlashBuildUpTime", default = 1.4)]
    pub flash_build_up_time: f32,
    #[def("RushTriggerDistance", default = 25.0)]
    pub rush_trigger_distance: f32,
    #[def("RushToDistance", default = 19.0)]
    pub rush_to_distance: f32,
    #[def("RushAwayDistance", default = 15.0)]
    pub rush_away_distance: f32,
}
