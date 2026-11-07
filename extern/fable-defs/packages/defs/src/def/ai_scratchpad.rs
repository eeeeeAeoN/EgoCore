use crate::DefStruct;
use crate::enums::ScriptingStateGroups;
use crate::values::Vector3D;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AIScratchpadDef {
    #[def("ThankingPhrase")]
    pub thanking_phrase: u32,
    #[def("IgnoringPhrase")]
    pub ignoring_phrase: u32,
    #[def("WanderCentrePoint")]
    pub wander_centre_point: Vector3D,
    #[def("WanderMinDistance")]
    pub wander_min_distance: f32,
    #[def("WanderMaxDistance")]
    pub wander_max_distance: f32,
    #[def("GossipCounter")]
    pub gossip_counter: i32,
    #[def("MaxGossipPhrase")]
    pub max_gossip_phrase: i32,
    #[def("WarningPhrase")]
    pub warning_phrase: u32,
    #[def("BeerRequestPhrase")]
    pub beer_request_phrase: u32,
    #[def("ScriptingStateGroup")]
    pub scripting_state_group: ScriptingStateGroups,
    #[def("MaxHeroReactionDistance")]
    pub max_hero_reaction_distance: f32,
    #[def("ActionFrequency")]
    pub action_frequency: i32,
    #[def("ActionFrequencyVariation")]
    pub action_frequency_variation: f32,
    #[def("Action")]
    pub action: String,
    #[def("FaceHeroForAction")]
    pub face_hero_for_action: bool,
    #[def("TargetName")]
    pub target_name: String,
    #[def("FollowDistance")]
    pub follow_distance: f32,
    #[def("AttackHeroOnSight")]
    pub attack_hero_on_sight: bool,
    #[def("TimeToSpendHarassingHero")]
    pub time_to_spend_harassing_hero: i32,
    #[def("CombatNearbyEnemyFleeingBreakOffRange", default = 5.0)]
    pub combat_nearby_enemy_fleeing_break_off_range: f32,
    #[def("CombatNearbyBreakOffRange", default = 10.0)]
    pub combat_nearby_break_off_range: f32,
    #[def("StealStealableItems")]
    pub steal_stealable_items: bool,
    #[def("RecoverStealableItems")]
    pub recover_stealable_items: bool,
    #[def("TakeStealableItemToRandomDestination")]
    pub take_stealable_item_to_random_destination: bool,
    #[def("KillSelfAndStealableItemAfterReachingDestination", default = true)]
    pub kill_self_and_stealable_item_after_reaching_destination: bool,
    #[def("AllowedToFollow", default = true)]
    pub allowed_to_follow: bool,
    #[def("TableName")]
    pub table_name: String,
    #[def("SeatName")]
    pub seat_name: String,
    #[def("DisableHeadLooking")]
    pub disable_head_looking: bool,
    #[def("IsPushableByHero", default = true)]
    pub is_pushable_by_hero: bool,
    #[def("LookForFiniteTime", default = true)]
    pub look_for_finite_time: bool,
    #[def("AvoidRegionExits", default = true)]
    pub avoid_region_exits: bool,
    #[def("TargetingDistanceOffset")]
    pub targeting_distance_offset: f32,
}
