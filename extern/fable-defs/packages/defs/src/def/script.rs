use crate::DefStruct;
use crate::def::{
    ArenaRoundDef, Conversation, FeatDef, FireHeartPatternDef, HeroSoulsRoundDef, wire::DefString,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ScriptDef {
    #[def("EXPERIENCE_GRANT_SMALL")]
    pub experience_grant_small: i32,
    #[def("EXPERIENCE_GRANT_MEDIUM")]
    pub experience_grant_medium: i32,
    #[def("EXPERIENCE_GRANT_LARGE")]
    pub experience_grant_large: i32,
    #[def("EXPERIENCE_GRANT_XLARGE")]
    pub experience_grant_xlarge: i32,
    #[def("EXPERIENCE_GRANT_HEROIC")]
    pub experience_grant_heroic: i32,
    #[def("NPC_COMMENT_FREQUENCY")]
    pub npc_comment_frequency: i32,
    #[def("NPC_COMMENT_DISTANCE")]
    pub npc_comment_distance: i32,
    #[def("AmbushTradersNakedBoastCost")]
    pub ambush_traders_naked_boast_cost: i32,
    #[def("AmbushTradersNakedBoastReward")]
    pub ambush_traders_naked_boast_reward: i32,
    #[def("AmbushTradersNoDamageBoastCost")]
    pub ambush_traders_no_damage_boast_cost: i32,
    #[def("AmbushTradersNoDamageBoastReward")]
    pub ambush_traders_no_damage_boast_reward: i32,
    #[def("AmbushTradersNoWeaponsBoastCost")]
    pub ambush_traders_no_weapons_boast_cost: i32,
    #[def("AmbushTradersNoWeaponsBoastReward")]
    pub ambush_traders_no_weapons_boast_reward: i32,
    #[def("ArenaNakedBoastCost")]
    pub arena_naked_boast_cost: i32,
    #[def("ArenaNakedBoastReward")]
    pub arena_naked_boast_reward: i32,
    #[def("ArenaNoDamageBoastCost")]
    pub arena_no_damage_boast_cost: i32,
    #[def("ArenaNoDamageBoastReward")]
    pub arena_no_damage_boast_reward: i32,
    #[def("ArenaNoWeaponsBoastCost")]
    pub arena_no_weapons_boast_cost: i32,
    #[def("ArenaNoWeaponsBoastReward")]
    pub arena_no_weapons_boast_reward: i32,
    #[def("BanditCampNoDamageBoastCost")]
    pub bandit_camp_no_damage_boast_cost: i32,
    #[def("BanditCampNoDamageBoastReward")]
    pub bandit_camp_no_damage_boast_reward: i32,
    #[def("BanditCampNoWeaponsBoastCost")]
    pub bandit_camp_no_weapons_boast_cost: i32,
    #[def("BanditCampNoWeaponsBoastReward")]
    pub bandit_camp_no_weapons_boast_reward: i32,
    #[def("BountyHuntNakedBoastCost")]
    pub bounty_hunt_naked_boast_cost: i32,
    #[def("BountyHuntNakedBoastReward")]
    pub bounty_hunt_naked_boast_reward: i32,
    #[def("BountyHuntNoDamageBoastCost")]
    pub bounty_hunt_no_damage_boast_cost: i32,
    #[def("BountyHuntNoDamageBoastReward")]
    pub bounty_hunt_no_damage_boast_reward: i32,
    #[def("BreakSiegeNakedBoastCost")]
    pub break_siege_naked_boast_cost: i32,
    #[def("BreakSiegeNakedBoastReward")]
    pub break_siege_naked_boast_reward: i32,
    #[def("BreakSiegeNoDamageBoastCost")]
    pub break_siege_no_damage_boast_cost: i32,
    #[def("BreakSiegeNoDamageBoastReward")]
    pub break_siege_no_damage_boast_reward: i32,
    #[def("BreakSiegeNoWeaponsBoastCost")]
    pub break_siege_no_weapons_boast_cost: i32,
    #[def("BreakSiegeNoWeaponsBoastReward")]
    pub break_siege_no_weapons_boast_reward: i32,
    #[def("GraveyardNakedBoastCost")]
    pub graveyard_naked_boast_cost: i32,
    #[def("GraveyardNakedBoastReward")]
    pub graveyard_naked_boast_reward: i32,
    #[def("GraveyardNoDamageBoastCost")]
    pub graveyard_no_damage_boast_cost: i32,
    #[def("GraveyardNoDamageBoastReward")]
    pub graveyard_no_damage_boast_reward: i32,
    #[def("GraveyardNoWeaponsBoastCost")]
    pub graveyard_no_weapons_boast_cost: i32,
    #[def("GraveyardNoWeaponsBoastReward")]
    pub graveyard_no_weapons_boast_reward: i32,
    #[def("HobbeCaveNakedBoastCost")]
    pub hobbe_cave_naked_boast_cost: i32,
    #[def("HobbeCaveNakedBoastReward")]
    pub hobbe_cave_naked_boast_reward: i32,
    #[def("HobbeCaveNoDamageBoastCost")]
    pub hobbe_cave_no_damage_boast_cost: i32,
    #[def("HobbeCaveNoDamageBoastReward")]
    pub hobbe_cave_no_damage_boast_reward: i32,
    #[def("HobbeCaveNoWeaponsBoastCost")]
    pub hobbe_cave_no_weapons_boast_cost: i32,
    #[def("HobbeCaveNoWeaponsBoastReward")]
    pub hobbe_cave_no_weapons_boast_reward: i32,
    #[def("HTContestNakedBoastCost")]
    pub ht_contest_naked_boast_cost: i32,
    #[def("HTContestNakedBoastReward")]
    pub ht_contest_naked_boast_reward: i32,
    #[def("HTContestNoDamageBoastCost")]
    pub ht_contest_no_damage_boast_cost: i32,
    #[def("HTContestNoDamageBoastReward")]
    pub ht_contest_no_damage_boast_reward: i32,
    #[def("HTContestNoWeaponsBoastCost")]
    pub ht_contest_no_weapons_boast_cost: i32,
    #[def("HTContestNoWeaponsBoastReward")]
    pub ht_contest_no_weapons_boast_reward: i32,
    #[def("HTEvilNakedBoastCost")]
    pub ht_evil_naked_boast_cost: i32,
    #[def("HTEvilNakedBoastReward")]
    pub ht_evil_naked_boast_reward: i32,
    #[def("HTEvilNoDamageBoastCost")]
    pub ht_evil_no_damage_boast_cost: i32,
    #[def("HTEvilNoDamageBoastReward")]
    pub ht_evil_no_damage_boast_reward: i32,
    #[def("HTEvilNoWeaponsBoastCost")]
    pub ht_evil_no_weapons_boast_cost: i32,
    #[def("HTEvilNoWeaponsBoastReward")]
    pub ht_evil_no_weapons_boast_reward: i32,
    #[def("HTGoodNakedBoastCost")]
    pub ht_good_naked_boast_cost: i32,
    #[def("HTGoodNakedBoastReward")]
    pub ht_good_naked_boast_reward: i32,
    #[def("HTGoodNoDamageBoastCost")]
    pub ht_good_no_damage_boast_cost: i32,
    #[def("HTGoodNoDamageBoastReward")]
    pub ht_good_no_damage_boast_reward: i32,
    #[def("HTGoodNoWeaponsBoastCost")]
    pub ht_good_no_weapons_boast_cost: i32,
    #[def("HTGoodNoWeaponsBoastReward")]
    pub ht_good_no_weapons_boast_reward: i32,
    #[def("MCCNakedBoastCost")]
    pub mcc_naked_boast_cost: i32,
    #[def("MCCNakedBoastReward")]
    pub mcc_naked_boast_reward: i32,
    #[def("MCCNoDamageBoastCost")]
    pub mcc_no_damage_boast_cost: i32,
    #[def("MCCNoDamageBoastReward")]
    pub mcc_no_damage_boast_reward: i32,
    #[def("MCCNoWeaponsBoastCost")]
    pub mcc_no_weapons_boast_cost: i32,
    #[def("MCCNoWeaponsBoastReward")]
    pub mcc_no_weapons_boast_reward: i32,
    #[def("MinionCampNakedBoastCost")]
    pub minion_camp_naked_boast_cost: i32,
    #[def("MinionCampNakedBoastReward")]
    pub minion_camp_naked_boast_reward: i32,
    #[def("MinionCampNoDamageBoastCost")]
    pub minion_camp_no_damage_boast_cost: i32,
    #[def("MinionCampNoDamageBoastReward")]
    pub minion_camp_no_damage_boast_reward: i32,
    #[def("MinionCampNoWeaponsBoastCost")]
    pub minion_camp_no_weapons_boast_cost: i32,
    #[def("MinionCampNoWeaponsBoastReward")]
    pub minion_camp_no_weapons_boast_reward: i32,
    #[def("OFEvilNakedBoastCost")]
    pub of_evil_naked_boast_cost: i32,
    #[def("OFEvilNakedBoastReward")]
    pub of_evil_naked_boast_reward: i32,
    #[def("OFEvilNoDamageBoastCost")]
    pub of_evil_no_damage_boast_cost: i32,
    #[def("OFEvilNoDamageBoastReward")]
    pub of_evil_no_damage_boast_reward: i32,
    #[def("OFEvilNoWeaponsBoastCost")]
    pub of_evil_no_weapons_boast_cost: i32,
    #[def("OFEvilNoWeaponsBoastReward")]
    pub of_evil_no_weapons_boast_reward: i32,
    #[def("OFGoodNakedBoastCost")]
    pub of_good_naked_boast_cost: i32,
    #[def("OFGoodNakedBoastReward")]
    pub of_good_naked_boast_reward: i32,
    #[def("OFGoodNoDamageBoastCost")]
    pub of_good_no_damage_boast_cost: i32,
    #[def("OFGoodNoDamageBoastReward")]
    pub of_good_no_damage_boast_reward: i32,
    #[def("OFGoodNoWeaponsBoastCost")]
    pub of_good_no_weapons_boast_cost: i32,
    #[def("OFGoodNoWeaponsBoastReward")]
    pub of_good_no_weapons_boast_reward: i32,
    #[def("PrisonNakedBoastCost")]
    pub prison_naked_boast_cost: i32,
    #[def("PrisonNakedBoastReward")]
    pub prison_naked_boast_reward: i32,
    #[def("PrisonNoDamageBoastCost")]
    pub prison_no_damage_boast_cost: i32,
    #[def("PrisonNoDamageBoastReward")]
    pub prison_no_damage_boast_reward: i32,
    #[def("PrisonNoWeaponsBoastCost")]
    pub prison_no_weapons_boast_cost: i32,
    #[def("PrisonNoWeaponsBoastReward")]
    pub prison_no_weapons_boast_reward: i32,
    #[def("RansomVictimNakedBoastCost")]
    pub ransom_victim_naked_boast_cost: i32,
    #[def("RansomVictimNakedBoastReward")]
    pub ransom_victim_naked_boast_reward: i32,
    #[def("RansomVictimNoDamageBoastCost")]
    pub ransom_victim_no_damage_boast_cost: i32,
    #[def("RansomVictimNoDamageBoastReward")]
    pub ransom_victim_no_damage_boast_reward: i32,
    #[def("RansomVictimNoWeaponsBoastCost")]
    pub ransom_victim_no_weapons_boast_cost: i32,
    #[def("RansomVictimNoWeaponsBoastReward")]
    pub ransom_victim_no_weapons_boast_reward: i32,
    #[def("TraderConflictEvilNakedBoastCost")]
    pub trader_conflict_evil_naked_boast_cost: i32,
    #[def("TraderConflictEvilNakedBoastReward")]
    pub trader_conflict_evil_naked_boast_reward: i32,
    #[def("TraderConflictEvilNoDamageBoastCost")]
    pub trader_conflict_evil_no_damage_boast_cost: i32,
    #[def("TraderConflictEvilNoDamageBoastReward")]
    pub trader_conflict_evil_no_damage_boast_reward: i32,
    #[def("TraderConflictEvilNoWeaponsBoastCost")]
    pub trader_conflict_evil_no_weapons_boast_cost: i32,
    #[def("TraderConflictEvilNoWeaponsBoastReward")]
    pub trader_conflict_evil_no_weapons_boast_reward: i32,
    #[def("TraderConflictGoodNakedBoastCost")]
    pub trader_conflict_good_naked_boast_cost: i32,
    #[def("TraderConflictGoodNakedBoastReward")]
    pub trader_conflict_good_naked_boast_reward: i32,
    #[def("TraderConflictGoodNoDamageBoastCost")]
    pub trader_conflict_good_no_damage_boast_cost: i32,
    #[def("TraderConflictGoodNoDamageBoastReward")]
    pub trader_conflict_good_no_damage_boast_reward: i32,
    #[def("TraderConflictGoodNoWeaponsBoastCost")]
    pub trader_conflict_good_no_weapons_boast_cost: i32,
    #[def("TraderConflictGoodNoWeaponsBoastReward")]
    pub trader_conflict_good_no_weapons_boast_reward: i32,
    #[def("TraderEscortNakedBoastCost")]
    pub trader_escort_naked_boast_cost: i32,
    #[def("TraderEscortNakedBoastReward")]
    pub trader_escort_naked_boast_reward: i32,
    #[def("TraderEscortNoDamageBoastCost")]
    pub trader_escort_no_damage_boast_cost: i32,
    #[def("TraderEscortNoDamageBoastReward")]
    pub trader_escort_no_damage_boast_reward: i32,
    #[def("TraderEscortNoWeaponsBoastCost")]
    pub trader_escort_no_weapons_boast_cost: i32,
    #[def("TraderEscortNoWeaponsBoastReward")]
    pub trader_escort_no_weapons_boast_reward: i32,
    #[def("WhiteBalvNakedBoastCost")]
    pub white_balv_naked_boast_cost: i32,
    #[def("WhiteBalvNakedBoastReward")]
    pub white_balv_naked_boast_reward: i32,
    #[def("WhiteBalvNoDamageBoastCost")]
    pub white_balv_no_damage_boast_cost: i32,
    #[def("WhiteBalvNoDamageBoastReward")]
    pub white_balv_no_damage_boast_reward: i32,
    #[def("WhiteBalvNoWeaponsBoastCost")]
    pub white_balv_no_weapons_boast_cost: i32,
    #[def("WhiteBalvNoWeaponsBoastReward")]
    pub white_balv_no_weapons_boast_reward: i32,
    #[def("SummoningShipNakedBoastCost")]
    pub summoning_ship_naked_boast_cost: i32,
    #[def("SummoningShipNakedBoastReward")]
    pub summoning_ship_naked_boast_reward: i32,
    #[def("SummoningShipNoDamageBoastCost")]
    pub summoning_ship_no_damage_boast_cost: i32,
    #[def("SummoningShipNoDamageBoastReward")]
    pub summoning_ship_no_damage_boast_reward: i32,
    #[def("TCGKillNoBanditsCost")]
    pub tcg_kill_no_bandits_cost: i32,
    #[def("TCGKillNoBanditsReward")]
    pub tcg_kill_no_bandits_reward: i32,
    #[def("TCGMadeTimeLimitCost")]
    pub tcg_made_time_limit_cost: i32,
    #[def("TCGMadeTimeLimitReward")]
    pub tcg_made_time_limit_reward: i32,
    #[def("TCEKeepBanditFollowerAliveCost")]
    pub tce_keep_bandit_follower_alive_cost: i32,
    #[def("TCEKeepBanditFollowerAliveReward")]
    pub tce_keep_bandit_follower_alive_reward: i32,
    #[def("TCEMadeTimeLimitCost")]
    pub tce_made_time_limit_cost: i32,
    #[def("TCEMadeTimeLimitReward")]
    pub tce_made_time_limit_reward: i32,
    #[def("RansomVictimVictimNoDamageCost")]
    pub ransom_victim_victim_no_damage_cost: i32,
    #[def("RansomVictimVictimNoDamageReward")]
    pub ransom_victim_victim_no_damage_reward: i32,
    #[def("RansomVictimKillKidnapperCost")]
    pub ransom_victim_kill_kidnapper_cost: i32,
    #[def("RansomVictimKillKidnapperReward")]
    pub ransom_victim_kill_kidnapper_reward: i32,
    #[def("RansomVictimSaveVictimCost")]
    pub ransom_victim_save_victim_cost: i32,
    #[def("RansomVictimSaveVictimReward")]
    pub ransom_victim_save_victim_reward: i32,
    #[def("RansomVictimHaveVictimKilledCost")]
    pub ransom_victim_have_victim_killed_cost: i32,
    #[def("RansomVictimHaveVictimKilledReward")]
    pub ransom_victim_have_victim_killed_reward: i32,
    #[def("MinionCampBriarNoDamageCost")]
    pub minion_camp_briar_no_damage_cost: i32,
    #[def("MinionCampBriarNoDamageReward")]
    pub minion_camp_briar_no_damage_reward: i32,
    #[def("AmbushScamTricksterApproachProximityLow")]
    pub ambush_scam_trickster_approach_proximity_low: f32,
    #[def("AmbushScamTricksterApproachProximityHigh")]
    pub ambush_scam_trickster_approach_proximity_high: f32,
    #[def("AmbushScamTricksterHeroProximity")]
    pub ambush_scam_trickster_hero_proximity: f32,
    #[def("AmbushScamTricksterRunAwayProximity")]
    pub ambush_scam_trickster_run_away_proximity: f32,
    #[def("AmbushScamAmbushTriggerProximity")]
    pub ambush_scam_ambush_trigger_proximity: f32,
    #[def("AmbushScamApproachTriggerProximity")]
    pub ambush_scam_approach_trigger_proximity: f32,
    #[def("AmbushScamBuddyWanderMax")]
    pub ambush_scam_buddy_wander_max: f32,
    #[def("AmbushScamTricksterWanderMax")]
    pub ambush_scam_trickster_wander_max: f32,
    #[def("AmbushScamGoldReward")]
    pub ambush_scam_gold_reward: i32,
    #[def("AmbushScamBonusObject")]
    pub ambush_scam_bonus_object: DefString,
    #[def("AmbushScamExperience")]
    pub ambush_scam_experience: i32,
    #[def("AmbushScamRenown")]
    pub ambush_scam_renown: i32,
    #[def("AmbushScamMoralityEvil")]
    pub ambush_scam_morality_evil: f32,
    #[def("AmbushScamMoralityGood")]
    pub ambush_scam_morality_good: f32,
    #[def("AmbushScamRandomSpeechTimerMax")]
    pub ambush_scam_random_speech_timer_max: i32,
    #[def("AmbushScamRandomSpeechTimerVar")]
    pub ambush_scam_random_speech_timer_var: i32,
    #[def("AmbushScamScreamSoundIds")]
    pub ambush_scam_scream_sound_ids: Vec<DefString>,
    #[def("AmbushScamScreamProximity")]
    pub ambush_scam_scream_proximity: f32,
    #[def("ArcheryCompetitionCost")]
    pub archery_competition_cost: i32,
    #[def("ArcheryGameTimeLimit")]
    pub archery_game_time_limit: i32,
    #[def("ArcheryTimesUpDuration")]
    pub archery_times_up_duration: f32,
    #[def("ArcheryInitialHighScoreMax")]
    pub archery_initial_high_score_max: i32,
    #[def("ArcheryInitialHighScoreVar")]
    pub archery_initial_high_score_var: i32,
    #[def("ArcheryScoreForPrize")]
    pub archery_score_for_prize: f32,
    #[def("ArcheryScoreForPrizeHigh")]
    pub archery_score_for_prize_high: f32,
    #[def("ArcheryScoreForPrizeMedium")]
    pub archery_score_for_prize_medium: f32,
    #[def("ArcheryScoreForPrizeLow")]
    pub archery_score_for_prize_low: f32,
    #[def("ArcheryMaxOutsideRing")]
    pub archery_max_outside_ring: f32,
    #[def("ArcheryDummySpeedFast")]
    pub archery_dummy_speed_fast: i32,
    #[def("ArcheryDummySpeedMedium")]
    pub archery_dummy_speed_medium: i32,
    #[def("ArcheryDummySpeedSlow")]
    pub archery_dummy_speed_slow: i32,
    #[def("ArcheryNumberOfMoves")]
    pub archery_number_of_moves: i32,
    #[def("ArcheryRewardObject")]
    pub archery_reward_object: DefString,
    #[def("ArcheryNormalPrizes")]
    pub archery_normal_prizes: Vec<DefString>,
    #[def("ArcheryNormalPrizesHigh")]
    pub archery_normal_prizes_high: Vec<DefString>,
    #[def("ArcheryNormalPrizesMedium")]
    pub archery_normal_prizes_medium: Vec<DefString>,
    #[def("ArcheryNormalPrizesLow")]
    pub archery_normal_prizes_low: Vec<DefString>,
    #[def("ArcheryMatchHighScorePrizes")]
    pub archery_match_high_score_prizes: Vec<DefString>,
    #[def("ArcheryNewHighScorePrizes")]
    pub archery_new_high_score_prizes: Vec<DefString>,
    #[def("ArcheryScorePowerLevels")]
    pub archery_score_power_levels: Vec<f32>,
    #[def("ArcheryReallyHighScorePrize")]
    pub archery_really_high_score_prize: DefString,
    #[def("ArcheryReallyHighScore")]
    pub archery_really_high_score: i32,
    #[def("ArcheryExperience")]
    pub archery_experience: i32,
    #[def("ArcheryRenown")]
    pub archery_renown: i32,
    #[def("ArcheryMoralityCheatUsedMagic")]
    pub archery_morality_cheat_used_magic: f32,
    #[def("ArcheryMoralityCheatSteppedOut")]
    pub archery_morality_cheat_stepped_out: f32,
    #[def("ArcheryMoralityShotOwner")]
    pub archery_morality_shot_owner: f32,
    #[def("ArcheryScoreDummyStatic")]
    pub archery_score_dummy_static: i32,
    #[def("ArcheryScoreDummyFront")]
    pub archery_score_dummy_front: i32,
    #[def("ArcheryScoreDummyCentre")]
    pub archery_score_dummy_centre: i32,
    #[def("ArcheryScoreDummyLeft")]
    pub archery_score_dummy_left: i32,
    #[def("ArcheryScoreDummyRight")]
    pub archery_score_dummy_right: i32,
    #[def("ArcheryRandomSpeechTimerMax")]
    pub archery_random_speech_timer_max: i32,
    #[def("ArcheryRandomSpeechTimerVar")]
    pub archery_random_speech_timer_var: i32,
    #[def("AssassinActivateRenownLevel")]
    pub assassin_activate_renown_level: i32,
    #[def("AssassinTriggerProximity")]
    pub assassin_trigger_proximity: i32,
    #[def("AssassinScreamProximity")]
    pub assassin_scream_proximity: i32,
    #[def("AssassinChestProximity")]
    pub assassin_chest_proximity: i32,
    #[def("AssassinCameraOffScreenTime")]
    pub assassin_camera_off_screen_time: i32,
    #[def("AssassinScreamSoundIds")]
    pub assassin_scream_sound_ids: Vec<DefString>,
    #[def("AssassinReward")]
    pub assassin_reward: i32,
    #[def("AssassinRewardDelay")]
    pub assassin_reward_delay: f32,
    #[def("AssassinObjects")]
    pub assassin_objects: Vec<String>,
    #[def("AssassinLocationName")]
    pub assassin_location_name: Vec<String>,
    #[def("AssassinRegionKillCount")]
    pub assassin_region_kill_count: Vec<i32>,
    #[def("AssassinExperience")]
    pub assassin_experience: i32,
    #[def("AssassinRenown")]
    pub assassin_renown: i32,
    #[def("AssassinMorality")]
    pub assassin_morality: f32,
    #[def("BanditCampPathEavesDropProximity")]
    pub bandit_camp_path_eaves_drop_proximity: i32,
    #[def("BanditCampKillNoBanditsBoastCost")]
    pub bandit_camp_kill_no_bandits_boast_cost: i32,
    #[def("BanditCampKillNoBanditsBoastReward")]
    pub bandit_camp_kill_no_bandits_boast_reward: i32,
    #[def("BanditCampKillManyBanditsBoastCost")]
    pub bandit_camp_kill_many_bandits_boast_cost: i32,
    #[def("BanditCampKillManyBanditsBoastReward")]
    pub bandit_camp_kill_many_bandits_boast_reward: i32,
    #[def("KillTraderTriggerDistance")]
    pub kill_trader_trigger_distance: i32,
    #[def("TollBanditCallHeroDistance")]
    pub toll_bandit_call_hero_distance: i32,
    #[def("TollBanditWarningOverDistance")]
    pub toll_bandit_warning_over_distance: i32,
    #[def("TollBanditHeroLeftDistance")]
    pub toll_bandit_hero_left_distance: i32,
    #[def("TollBanditPayDistance")]
    pub toll_bandit_pay_distance: i32,
    #[def("TollBanditCallHeroTimer")]
    pub toll_bandit_call_hero_timer: i32,
    #[def("TollBanditWarnHeroTimer")]
    pub toll_bandit_warn_hero_timer: i32,
    #[def("TollBanditTauntTimer")]
    pub toll_bandit_taunt_timer: i32,
    #[def("BanditTollSneakDistance")]
    pub bandit_toll_sneak_distance: i32,
    #[def("BanditTollBanditWanderDistance")]
    pub bandit_toll_bandit_wander_distance: i32,
    #[def("BanditTollTraderWarnDistance")]
    pub bandit_toll_trader_warn_distance: i32,
    #[def("TollBanditFee")]
    pub toll_bandit_fee: i32,
    #[def("TollBanditFeeWithFollower")]
    pub toll_bandit_fee_with_follower: i32,
    #[def("TollBanditCreatureType")]
    pub toll_bandit_creature_type: DefString,
    #[def("TollBanditCloseToExit")]
    pub toll_bandit_close_to_exit: f32,
    #[def("TollBanditTimeToFade")]
    pub toll_bandit_time_to_fade: f32,
    #[def("BanditCountForToll")]
    pub bandit_count_for_toll: i32,
    #[def("BeardyBaldyWanderDistanceMin")]
    pub beardy_baldy_wander_distance_min: f32,
    #[def("BeardyBaldyWanderDistanceMax")]
    pub beardy_baldy_wander_distance_max: f32,
    #[def("BeardyBaldyHaircutStyle1")]
    pub beardy_baldy_haircut_style1: DefString,
    #[def("BeardyBaldyHaircutCard1")]
    pub beardy_baldy_haircut_card1: DefString,
    #[def("BeardyBaldyHaircutStyle2")]
    pub beardy_baldy_haircut_style2: DefString,
    #[def("BeardyBaldyHaircutCard2")]
    pub beardy_baldy_haircut_card2: DefString,
    #[def("BeardyBaldyBeardStyle1")]
    pub beardy_baldy_beard_style1: DefString,
    #[def("BeardyBaldyBeardCard1")]
    pub beardy_baldy_beard_card1: DefString,
    #[def("BeardyBaldyBeardStyle2")]
    pub beardy_baldy_beard_style2: DefString,
    #[def("BeardyBaldyBeardCard2")]
    pub beardy_baldy_beard_card2: DefString,
    #[def("BeardyBaldyTashStyle1")]
    pub beardy_baldy_tash_style1: DefString,
    #[def("BeardyBaldyTashCard1")]
    pub beardy_baldy_tash_card1: DefString,
    #[def("BeardyBaldyTashStyle2")]
    pub beardy_baldy_tash_style2: DefString,
    #[def("BeardyBaldyTashCard2")]
    pub beardy_baldy_tash_card2: DefString,
    #[def("BeardyBaldyRewardObjectFullComplete")]
    pub beardy_baldy_reward_object_full_complete: DefString,
    #[def("BeardyBaldyRewardObjectHalfComplete")]
    pub beardy_baldy_reward_object_half_complete: DefString,
    #[def("BeardyBaldyExperience")]
    pub beardy_baldy_experience: i32,
    #[def("BeardyBaldyRenown")]
    pub beardy_baldy_renown: i32,
    #[def("BeardyBaldyMorality")]
    pub beardy_baldy_morality: f32,
    #[def("BeardyBaldyRandomSpeechTimerMax")]
    pub beardy_baldy_random_speech_timer_max: i32,
    #[def("BeardyBaldyRandomSpeechTimerVar")]
    pub beardy_baldy_random_speech_timer_var: i32,
    #[def("HenchmanHealthLow")]
    pub henchman_health_low: i32,
    #[def("HenchmanHealthCritical")]
    pub henchman_health_critical: i32,
    #[def("HenchmanPayTimeDelayInSeconds")]
    pub henchman_pay_time_delay_in_seconds: i32,
    #[def("HenchmanCheapGoldCost")]
    pub henchman_cheap_gold_cost: i32,
    #[def("HenchmanExpensiveGoldCost")]
    pub henchman_expensive_gold_cost: i32,
    #[def("BookReactions")]
    pub book_reactions: Vec<Conversation>,
    #[def("BookDefNames")]
    pub book_def_names: Vec<String>,
    #[def("BookBoyMarker")]
    pub book_boy_marker: Vec<String>,
    #[def("BookGirlMarker")]
    pub book_girl_marker: Vec<String>,
    #[def("BooksMoralityReward")]
    pub books_morality_reward: f32,
    #[def("BooksNumberWanted")]
    pub books_number_wanted: i32,
    #[def("BooksNumberAccepted")]
    pub books_number_accepted: i32,
    #[def("BooksNumberComment")]
    pub books_number_comment: i32,
    #[def("BooksNumberTotal")]
    pub books_number_total: i32,
    #[def("BadBooksForHat")]
    pub bad_books_for_hat: i32,
    #[def("GoodBooksForHat")]
    pub good_books_for_hat: i32,
    #[def("BordelloHeroTavernPriceMultiplier")]
    pub bordello_hero_tavern_price_multiplier: f32,
    #[def("ChapelActivationAge")]
    pub chapel_activation_age: i32,
    #[def("ChapelAltarProximity")]
    pub chapel_altar_proximity: f32,
    #[def("ChapelAcolyteWanderDistance")]
    pub chapel_acolyte_wander_distance: f32,
    #[def("ChapelAcolytePrayerTimer")]
    pub chapel_acolyte_prayer_timer: i32,
    #[def("ChapelAcolytePrayerTimerVar")]
    pub chapel_acolyte_prayer_timer_var: i32,
    #[def("ChapelDonationLevel")]
    pub chapel_donation_level: Vec<f32>,
    #[def("ChapelMoralityRewardFraction")]
    pub chapel_morality_reward_fraction: Vec<f32>,
    #[def("ChapelDonationRewardObject")]
    pub chapel_donation_reward_object: String,
    #[def("ChapelWeaponBrandishProximity")]
    pub chapel_weapon_brandish_proximity: f32,
    #[def("ChapelDonationLevelCoolThing1")]
    pub chapel_donation_level_cool_thing1: f32,
    #[def("ChapelDonationLevelCoolThing2")]
    pub chapel_donation_level_cool_thing2: f32,
    #[def("ChapelMoralityChangeCoolThing1")]
    pub chapel_morality_change_cool_thing1: f32,
    #[def("ChapelMoralityChangeCoolThing2")]
    pub chapel_morality_change_cool_thing2: f32,
    #[def("ChapelDonationRewardAgeMin")]
    pub chapel_donation_reward_age_min: i32,
    #[def("ChapelDonationRewardAgeVar")]
    pub chapel_donation_reward_age_var: i32,
    #[def("ChapelDonationRewardTitle")]
    pub chapel_donation_reward_title: String,
    #[def("ChapelDonationLevelFiddleSuper")]
    pub chapel_donation_level_fiddle_super: f32,
    #[def("ChapelDonationLevelFiddleVery")]
    pub chapel_donation_level_fiddle_very: f32,
    #[def("ChapelDonationLevelFiddle")]
    pub chapel_donation_level_fiddle: f32,
    #[def("ChapelExperience")]
    pub chapel_experience: i32,
    #[def("ChapelRenown")]
    pub chapel_renown: i32,
    #[def("ChapelMorality")]
    pub chapel_morality: f32,
    #[def("DealerAttentionDistance")]
    pub dealer_attention_distance: i32,
    #[def("DealerAttentionTimer")]
    pub dealer_attention_timer: i32,
    #[def("TraderLeavingDelay")]
    pub trader_leaving_delay: i32,
    #[def("TraderAttentionDistance")]
    pub trader_attention_distance: i32,
    #[def("TraderConversationDelay")]
    pub trader_conversation_delay: i32,
    #[def("TraderStopWalkingTimer")]
    pub trader_stop_walking_timer: i32,
    #[def("TraderStandStillTimer")]
    pub trader_stand_still_timer: i32,
    #[def("EasyPickingsReward")]
    pub easy_pickings_reward: i32,
    #[def("ExposeMayorEvidenceDef")]
    pub expose_mayor_evidence_def: String,
    #[def("EMMaxFlashDistFromStables")]
    pub em_max_flash_dist_from_stables: f32,
    #[def("F_FishermanHelpDelay")]
    pub f_fisherman_help_delay: i32,
    #[def("F_FishermanIdleChatDelay")]
    pub f_fisherman_idle_chat_delay: i32,
    #[def("F_WaspType")]
    pub f_wasp_type: DefString,
    #[def("FishermanSaveHimMorality")]
    pub fisherman_save_him_morality: f32,
    #[def("FishingPrizes")]
    pub fishing_prizes: i32,
    #[def("FishTypesInGame")]
    pub fish_types_in_game: i32,
    #[def("FishingRodsInGame")]
    pub fishing_rods_in_game: i32,
    #[def("FishPrizeDefNames")]
    pub fish_prize_def_names: Vec<String>,
    #[def("FishPrizeWeights")]
    pub fish_prize_weights: Vec<f32>,
    #[def("FishRodDefNames")]
    pub fish_rod_def_names: Vec<String>,
    #[def("FishDefNames")]
    pub fish_def_names: Vec<String>,
    #[def("FC_FighterDelay")]
    pub fc_fighter_delay: i32,
    #[def("FC_MinimumFighterHealth")]
    pub fc_minimum_fighter_health: i32,
    #[def("FC_OutsideRingRange")]
    pub fc_outside_ring_range: f32,
    #[def("FC_RingTimeOut")]
    pub fc_ring_time_out: f32,
    #[def("FC_BowerstoneAllowedHits")]
    pub fc_bowerstone_allowed_hits: i32,
    #[def("FC_OakvaleAllowedHits")]
    pub fc_oakvale_allowed_hits: i32,
    #[def("FC_BanditCampAllowedHits")]
    pub fc_bandit_camp_allowed_hits: i32,
    #[def("FC_KnotholeGladeAllowedHits")]
    pub fc_knothole_glade_allowed_hits: i32,
    #[def("FC_HitsDepletionOverSession")]
    pub fc_hits_depletion_over_session: i32,
    #[def("FC_BowerstoneFightCount")]
    pub fc_bowerstone_fight_count: i32,
    #[def("FC_OakvaleFightCount")]
    pub fc_oakvale_fight_count: i32,
    #[def("FC_BanditCampFightCount")]
    pub fc_bandit_camp_fight_count: i32,
    #[def("FC_KnotholeGladeFightCount")]
    pub fc_knothole_glade_fight_count: i32,
    #[def("FC_StartTimeOfDay")]
    pub fc_start_time_of_day: i32,
    #[def("FC_EndTimeOfDay")]
    pub fc_end_time_of_day: i32,
    #[def("FC_MoneyAddedPerFight")]
    pub fc_money_added_per_fight: i32,
    #[def("FC_RingRadius")]
    pub fc_ring_radius: f32,
    #[def("FC_InsideRingMessageDelay")]
    pub fc_inside_ring_message_delay: i32,
    #[def("FC_AttackWarnings")]
    pub fc_attack_warnings: i32,
    #[def("FC_BowerstoneRewardItem")]
    pub fc_bowerstone_reward_item: DefString,
    #[def("FC_OakvaleRewardItem")]
    pub fc_oakvale_reward_item: DefString,
    #[def("FC_BanditCampRewardItem")]
    pub fc_bandit_camp_reward_item: DefString,
    #[def("FC_KnotholeGladeRewardItem")]
    pub fc_knothole_glade_reward_item: DefString,
    #[def("FC_DistanceAwayToTriggerFC")]
    pub fc_distance_away_to_trigger_fc: f32,
    #[def("FC_MinimumFee")]
    pub fc_minimum_fee: i32,
    #[def("FC_ShoutOutDistance")]
    pub fc_shout_out_distance: f32,
    #[def("FC_LostRenownLoss")]
    pub fc_lost_renown_loss: i32,
    #[def("FC_WinRenownGain")]
    pub fc_win_renown_gain: i32,
    #[def("FC_WinExperienceGain")]
    pub fc_win_experience_gain: i32,
    #[def("GGN_GhostSpawnNearby")]
    pub ggn_ghost_spawn_nearby: i32,
    #[def("GGN_GhostSpawnDistance")]
    pub ggn_ghost_spawn_distance: i32,
    #[def("GGN_ReturnNecklaceReward")]
    pub ggn_return_necklace_reward: i32,
    #[def("GGN_GhostClueTime")]
    pub ggn_ghost_clue_time: i32,
    #[def("GGN_GhostTalkDistance")]
    pub ggn_ghost_talk_distance: i32,
    #[def("GGN_ReturnNecklaceMoralityGain")]
    pub ggn_return_necklace_morality_gain: f32,
    #[def("GGN_ReturnNecklaceExperienceGain")]
    pub ggn_return_necklace_experience_gain: i32,
    #[def("HH_UpperBarLimit")]
    pub hh_upper_bar_limit: f32,
    #[def("HH_BarDecrementAmount")]
    pub hh_bar_decrement_amount: f32,
    #[def("HH_BarDecrementInterval")]
    pub hh_bar_decrement_interval: f32,
    #[def("HH_ClearedHHRenown")]
    pub hh_cleared_hh_renown: i32,
    #[def("HeroDuelDefeatThunderMorality")]
    pub hero_duel_defeat_thunder_morality: f32,
    #[def("HeroDuelDefeatThunderXP")]
    pub hero_duel_defeat_thunder_xp: i32,
    #[def("HiddenBootyTreasureFake")]
    pub hidden_booty_treasure_fake: DefString,
    #[def("HiddenBootyTreasureReal")]
    pub hidden_booty_treasure_real: DefString,
    #[def("HiddenBootyExperience")]
    pub hidden_booty_experience: i32,
    #[def("HiddenBootyRenownFake")]
    pub hidden_booty_renown_fake: i32,
    #[def("HiddenBootyRenownReal")]
    pub hidden_booty_renown_real: i32,
    #[def("HiddenBootyMorality")]
    pub hidden_booty_morality: f32,
    #[def("TrophiesOpinionDeedTimer")]
    pub trophies_opinion_deed_timer: i32,
    #[def("TrophiesBriarRoseTrophyLeft")]
    pub trophies_briar_rose_trophy_left: DefString,
    #[def("TrophiesBriarRoseTrophyRight")]
    pub trophies_briar_rose_trophy_right: DefString,
    #[def("TrophiesBriarRoseProximityNear")]
    pub trophies_briar_rose_proximity_near: f32,
    #[def("TrophiesBriarRoseProximityFar")]
    pub trophies_briar_rose_proximity_far: f32,
    #[def("TrophiesStandStillTimer")]
    pub trophies_stand_still_timer: f32,
    #[def("TrophiesFacingWatcherTimer")]
    pub trophies_facing_watcher_timer: f32,
    #[def("TrophiesBriarRoseWalkingSpeed")]
    pub trophies_briar_rose_walking_speed: f32,
    #[def("TrophiesCrowdWanderDistanceMin")]
    pub trophies_crowd_wander_distance_min: f32,
    #[def("TrophiesCrowdWanderDistanceMax")]
    pub trophies_crowd_wander_distance_max: f32,
    #[def("TrophiesBriarRoseAnimTime")]
    pub trophies_briar_rose_anim_time: i32,
    #[def("TrophiesBriarRoseAnimTimeVar")]
    pub trophies_briar_rose_anim_time_var: i32,
    #[def("TrophiesInsideTavernTimer")]
    pub trophies_inside_tavern_timer: i32,
    #[def("TrophiesOutsideTavernTimer")]
    pub trophies_outside_tavern_timer: i32,
    #[def("TrophiesHitCountForBriarAttack")]
    pub trophies_hit_count_for_briar_attack: i32,
    #[def("OuterTraderDistance")]
    pub outer_trader_distance: f32,
    #[def("MiddleTraderDistance")]
    pub middle_trader_distance: f32,
    #[def("InnerTraderDistance")]
    pub inner_trader_distance: f32,
    #[def("TraderYellForHelpRandomValue")]
    pub trader_yell_for_help_random_value: i32,
    #[def("TraderYellAtHeroRandomValue")]
    pub trader_yell_at_hero_random_value: i32,
    #[def("TraderCommentRandomValue")]
    pub trader_comment_random_value: i32,
    #[def("BrotherStartDistance")]
    pub brother_start_distance: f32,
    #[def("BrotherScreamForHelpRandomValue")]
    pub brother_scream_for_help_random_value: i32,
    #[def("BrotherWaitCommentRandomValue")]
    pub brother_wait_comment_random_value: i32,
    #[def("HeroEvilMoralityValue")]
    pub hero_evil_morality_value: f32,
    #[def("HeroGoodMoralityValue")]
    pub hero_good_morality_value: f32,
    #[def("HeroKillsAssassinMorality")]
    pub hero_kills_assassin_morality: f32,
    #[def("HeroKillsSheriffMorality")]
    pub hero_kills_sheriff_morality: f32,
    #[def("HeroTakesAssassinGoldMorality")]
    pub hero_takes_assassin_gold_morality: f32,
    #[def("HeroTakesAssassinGoldMoralityKillsAssassin")]
    pub hero_takes_assassin_gold_morality_kills_assassin: f32,
    #[def("HeroKillsAssassinRenown")]
    pub hero_kills_assassin_renown: i32,
    #[def("HeroKillsAssassinProofOfKill")]
    pub hero_kills_assassin_proof_of_kill: DefString,
    #[def("HeroKillsSheriffRenown")]
    pub hero_kills_sheriff_renown: i32,
    #[def("AssassinThreatDistSq")]
    pub assassin_threat_dist_sq: f32,
    #[def("RandomPopulationTraderGoldMinimum")]
    pub random_population_trader_gold_minimum: i32,
    #[def("RandomPopulationTraderGoldVariation")]
    pub random_population_trader_gold_variation: i32,
    #[def("RockTrollTriggerProximity")]
    pub rock_troll_trigger_proximity: f32,
    #[def("RockTrollReward1")]
    pub rock_troll_reward1: DefString,
    #[def("RockTrollReward2")]
    pub rock_troll_reward2: DefString,
    #[def("IngredientObject")]
    pub ingredient_object: DefString,
    #[def("ObjectToGiveToMansLover")]
    pub object_to_give_to_mans_lover: DefString,
    #[def("ObjectFromMansLover")]
    pub object_from_mans_lover: DefString,
    #[def("EndPotion")]
    pub end_potion: DefString,
    #[def("SickChildSleepingAnim")]
    pub sick_child_sleeping_anim: DefString,
    #[def("SickChildsSisterAttentionAnims")]
    pub sick_childs_sister_attention_anims: Vec<DefString>,
    #[def("TellingTheTruth")]
    pub telling_the_truth: f32,
    #[def("LyingToWomanMorality")]
    pub lying_to_woman_morality: f32,
    #[def("LyingToManAboutWomanNotLikingHer")]
    pub lying_to_man_about_woman_not_liking_her: f32,
    #[def("LyingToManAboutWomanLikingHer")]
    pub lying_to_man_about_woman_liking_her: f32,
    #[def("KilledSomeoneForMushroom")]
    pub killed_someone_for_mushroom: f32,
    #[def("StoleMushroomInBarrowFields")]
    pub stole_mushroom_in_barrow_fields: f32,
    #[def("BoughtMushroomInBarrowFields")]
    pub bought_mushroom_in_barrow_fields: f32,
    #[def("MadeWomanLaugh")]
    pub made_woman_laugh: f32,
    #[def("SingingStonesDoorManWanderMax")]
    pub singing_stones_door_man_wander_max: f32,
    #[def("SwordInTheStoneWanderDistanceMin")]
    pub sword_in_the_stone_wander_distance_min: f32,
    #[def("SwordInTheStoneWanderDistanceMax")]
    pub sword_in_the_stone_wander_distance_max: f32,
    #[def("SwordInTheStoneSuccessExperience")]
    pub sword_in_the_stone_success_experience: i32,
    #[def("SwordInTheStoneSuccessRenown")]
    pub sword_in_the_stone_success_renown: i32,
    #[def("SwordInTheStoneSuccessMorality")]
    pub sword_in_the_stone_success_morality: f32,
    #[def("SwordInTheStonePhysiqueChange")]
    pub sword_in_the_stone_physique_change: i32,
    #[def("SwordInTheStoneToughnessChange")]
    pub sword_in_the_stone_toughness_change: i32,
    #[def("SwordInTheStoneHealthChange")]
    pub sword_in_the_stone_health_change: i32,
    #[def("BardSingingFee")]
    pub bard_singing_fee: i32,
    #[def("BardNumVersesPerVisit")]
    pub bard_num_verses_per_visit: i32,
    #[def("BardConsiderGood")]
    pub bard_consider_good: f32,
    #[def("BardConsiderEvil")]
    pub bard_consider_evil: f32,
    #[def("BardConsiderRich")]
    pub bard_consider_rich: i32,
    #[def("BardConsiderBoaster")]
    pub bard_consider_boaster: i32,
    #[def("BardConsiderFailure")]
    pub bard_consider_failure: i32,
    #[def("BardSpecial")]
    pub bard_special: i32,
    #[def("BardGoAwayCost")]
    pub bard_go_away_cost: i32,
    #[def("BardWanderDistanceMin")]
    pub bard_wander_distance_min: f32,
    #[def("BardWanderDistanceMax")]
    pub bard_wander_distance_max: f32,
    #[def("BardWanderResetTime")]
    pub bard_wander_reset_time: i32,
    #[def("BardFollowDistance")]
    pub bard_follow_distance: f32,
    #[def("BardRandomSpeechTimerMax")]
    pub bard_random_speech_timer_max: i32,
    #[def("BardRandomSpeechTimerVar")]
    pub bard_random_speech_timer_var: i32,
    #[def("BardHeroAgeHigh")]
    pub bard_hero_age_high: i32,
    #[def("BardHeroAgeMedium")]
    pub bard_hero_age_medium: i32,
    #[def("BardHeroHealthHigh")]
    pub bard_hero_health_high: f32,
    #[def("BardHeroHealthLow")]
    pub bard_hero_health_low: f32,
    #[def("BardHeroWillHigh")]
    pub bard_hero_will_high: f32,
    #[def("BardHeroWillLow")]
    pub bard_hero_will_low: f32,
    #[def("TempleActivationAge")]
    pub temple_activation_age: i32,
    #[def("TempleAltarProximity")]
    pub temple_altar_proximity: f32,
    #[def("TempleAcolyteWanderDistance")]
    pub temple_acolyte_wander_distance: f32,
    #[def("TempleAcolytePrayerTimer")]
    pub temple_acolyte_prayer_timer: i32,
    #[def("TempleAcolytePrayerTimerVar")]
    pub temple_acolyte_prayer_timer_var: i32,
    #[def("TempleDonationLevel")]
    pub temple_donation_level: Vec<f32>,
    #[def("TempleMoralityRewardFraction")]
    pub temple_morality_reward_fraction: Vec<f32>,
    #[def("TempleDonationRewardObject")]
    pub temple_donation_reward_object: String,
    #[def("TempleDonationLevelCoolThing1")]
    pub temple_donation_level_cool_thing1: f32,
    #[def("TempleDonationLevelCoolThing2")]
    pub temple_donation_level_cool_thing2: f32,
    #[def("TempleMoralityChangeCoolThing1")]
    pub temple_morality_change_cool_thing1: f32,
    #[def("TempleMoralityChangeCoolThing2")]
    pub temple_morality_change_cool_thing2: f32,
    #[def("TempleDonationRewardAgeMin")]
    pub temple_donation_reward_age_min: i32,
    #[def("TempleDonationRewardAgeVar")]
    pub temple_donation_reward_age_var: i32,
    #[def("TempleDonationRewardTitle")]
    pub temple_donation_reward_title: String,
    #[def("TempleDonationLevelFiddleSuper")]
    pub temple_donation_level_fiddle_super: f32,
    #[def("TempleDonationLevelFiddleVery")]
    pub temple_donation_level_fiddle_very: f32,
    #[def("TempleDonationLevelFiddle")]
    pub temple_donation_level_fiddle: f32,
    #[def("TempleExperience")]
    pub temple_experience: i32,
    #[def("TempleRenown")]
    pub temple_renown: i32,
    #[def("TempleMorality")]
    pub temple_morality: f32,
    #[def("TemplePeasantWalkSpeed")]
    pub temple_peasant_walk_speed: f32,
    #[def("TemplePrayerCounterActivation")]
    pub temple_prayer_counter_activation: i32,
    #[def("TempleDonationBoxRequirement")]
    pub temple_donation_box_requirement: i32,
    #[def("TempleWeaponBrandishProximity")]
    pub temple_weapon_brandish_proximity: f32,
    #[def("TempleTeleportEnableProximity")]
    pub temple_teleport_enable_proximity: f32,
    #[def("TemplePrayerFactorLowest")]
    pub temple_prayer_factor_lowest: i32,
    #[def("TemplePrayerFactorLow")]
    pub temple_prayer_factor_low: i32,
    #[def("TemplePrayerFactorMiddle")]
    pub temple_prayer_factor_middle: i32,
    #[def("TemplePrayerFactorHigh")]
    pub temple_prayer_factor_high: i32,
    #[def("TemplePrayerFactorHighest")]
    pub temple_prayer_factor_highest: i32,
    // Retail's CScriptDef serializes `TemplePrayerFactorHighest` TWICE (two
    // consecutive controls with the same crc). Confirmed by positional parse:
    // removing this field makes retail's script.bin fail to load at the next
    // field (WrongId). The decomp copy-operator shows only 5 factor scalars,
    // but the binary Transfer emits 6 — trust the binary, not the copy op.
    #[def("TemplePrayerFactorHighest")]
    pub temple_prayer_factor_highest2: i32,
    #[def("TemplePrayerFactorBurnChance")]
    pub temple_prayer_factor_burn_chance: Vec<i32>,
    #[def("TemplePrayerWorshipRewardHealth")]
    pub temple_prayer_worship_reward_health: Vec<f32>,
    #[def("TemplePrayerWorshipRewardMorality")]
    pub temple_prayer_worship_reward_morality: Vec<f32>,
    #[def("TemplePrayerWorshipRewardGold")]
    pub temple_prayer_worship_reward_gold: Vec<i32>,
    #[def("TemplePrayerWorshipRewardDonationModifier")]
    pub temple_prayer_worship_reward_donation_modifier: Vec<f32>,
    #[def("TemplePrayerWorshipBurnHealth")]
    pub temple_prayer_worship_burn_health: Vec<f32>,
    #[def("TemplePrayerWorshipBurnGold")]
    pub temple_prayer_worship_burn_gold: Vec<i32>,
    #[def("TourGuideFollowerDistance")]
    pub tour_guide_follower_distance: f32,
    #[def("TourGuideOverhearDistance")]
    pub tour_guide_overhear_distance: f32,
    #[def("TourGuideTalkDistance")]
    pub tour_guide_talk_distance: f32,
    #[def("TourGuideStandStillTimer")]
    pub tour_guide_stand_still_timer: i32,
    #[def("TourGuideTalkToHeroTimer")]
    pub tour_guide_talk_to_hero_timer: i32,
    #[def("TourGuideFollowerStandStillTimer")]
    pub tour_guide_follower_stand_still_timer: i32,
    #[def("TourGuideOpeningTime")]
    pub tour_guide_opening_time: i32,
    #[def("TourGuideClosingTime")]
    pub tour_guide_closing_time: i32,
    #[def("SlumsBeggarGoldAmount")]
    pub slums_beggar_gold_amount: i32,
    #[def("PickUpLitterTimeInSeconds")]
    pub pick_up_litter_time_in_seconds: i32,
    #[def("NumPiecesOfLitter")]
    pub num_pieces_of_litter: i32,
    #[def("OakValeBeggarGoldAmount")]
    pub oak_vale_beggar_gold_amount: i32,
    #[def("TBRMoralityGoodSmall")]
    pub tbr_morality_good_small: f32,
    #[def("TBRMoralityGoodMedium")]
    pub tbr_morality_good_medium: f32,
    #[def("TBRMoralityGoodLarge")]
    pub tbr_morality_good_large: f32,
    #[def("TBYMoralityBadSmall")]
    pub tby_morality_bad_small: f32,
    #[def("TBYMoralityBadMedium")]
    pub tby_morality_bad_medium: f32,
    #[def("TBYMoralityBadLarge")]
    pub tby_morality_bad_large: f32,
    #[def("OakShopKeepGoldAmount")]
    pub oak_shop_keep_gold_amount: i32,
    #[def("ArsonGoldAmount")]
    pub arson_gold_amount: i32,
    #[def("KGExtortAmount")]
    pub kg_extort_amount: i32,
    #[def("BULLY_WEAPON")]
    pub bully_weapon: DefString,
    #[def("TH_ThunderConv_Length")]
    pub th_thunder_conv_length: i32,
    #[def("TH_ThunderConv_Speaker")]
    pub th_thunder_conv_speaker: Vec<String>,
    #[def("TH_ThunderConv_Dialogue")]
    pub th_thunder_conv_dialogue: Vec<String>,
    #[def("TH_ThunderConv_Actor")]
    pub th_thunder_conv_actor: Vec<String>,
    #[def("TH_ThunderConv_Animation")]
    pub th_thunder_conv_animation: Vec<String>,
    #[def("TH_ThunderConv_AnimDelay")]
    pub th_thunder_conv_anim_delay: Vec<f32>,
    #[def("EscortTraderRewardDownpayment")]
    pub escort_trader_reward_downpayment: i32,
    #[def("EscortTraderAttackComplaintTime")]
    pub escort_trader_attack_complaint_time: i32,
    #[def("EscortTraderRewardObjectPerfect")]
    pub escort_trader_reward_object_perfect: Vec<DefString>,
    #[def("EscortTraderRegionExitWait")]
    pub escort_trader_region_exit_wait: f32,
    #[def("EscortTraderRegionExitStop")]
    pub escort_trader_region_exit_stop: f32,
    #[def("EscortTraderRegionExitDist")]
    pub escort_trader_region_exit_dist: f32,
    #[def("EscortTraderHitCountHigh")]
    pub escort_trader_hit_count_high: i32,
    #[def("EscortTraderHitCountMedium")]
    pub escort_trader_hit_count_medium: i32,
    #[def("EscortTraderHealthHigh")]
    pub escort_trader_health_high: f32,
    #[def("EscortTraderHealthMedium")]
    pub escort_trader_health_medium: f32,
    #[def("EscortTraderFollowProximity1")]
    pub escort_trader_follow_proximity1: f32,
    #[def("EscortTraderFollowProximity2")]
    pub escort_trader_follow_proximity2: f32,
    #[def("EscortTraderProximityTimer")]
    pub escort_trader_proximity_timer: i32,
    #[def("EscortTraderExpressionDetectTimer")]
    pub escort_trader_expression_detect_timer: i32,
    #[def("EscortTraderHitDetectTimer")]
    pub escort_trader_hit_detect_timer: i32,
    #[def("EscortTraderSpawnedBanditType")]
    pub escort_trader_spawned_bandit_type: Vec<String>,
    #[def("EscortTraderExperience")]
    pub escort_trader_experience: i32,
    #[def("EscortTraderRenown")]
    pub escort_trader_renown: i32,
    #[def("EscortTraderMorality")]
    pub escort_trader_morality: f32,
    #[def("EscortTraderFailureRenown")]
    pub escort_trader_failure_renown: i32,
    #[def("TraderRandomSpeechTimerMax")]
    pub trader_random_speech_timer_max: i32,
    #[def("TraderRandomSpeechTimerVar")]
    pub trader_random_speech_timer_var: i32,
    #[def("EscortTraderWanderDistance")]
    pub escort_trader_wander_distance: f32,
    #[def("EscortTraderQuestDayInterval")]
    pub escort_trader_quest_day_interval: i32,
    #[def("EscortTraderQuestStartTime")]
    pub escort_trader_quest_start_time: i32,
    #[def("AmbushTradersBanditHireCost")]
    pub ambush_traders_bandit_hire_cost: i32,
    #[def("AmbushTradersConvoyExitProximity")]
    pub ambush_traders_convoy_exit_proximity: f32,
    #[def("AmbushTradersPhase1GuardWanderDistance")]
    pub ambush_traders_phase1_guard_wander_distance: f32,
    #[def("AmbushTradersContactWanderDistance")]
    pub ambush_traders_contact_wander_distance: f32,
    #[def("AmbushTradersConvoyTriggerProximity")]
    pub ambush_traders_convoy_trigger_proximity: f32,
    #[def("AmbushTradersSentryWanderDistance")]
    pub ambush_traders_sentry_wander_distance: f32,
    #[def("AmbushTradersSpySentryProximity")]
    pub ambush_traders_spy_sentry_proximity: f32,
    #[def("AmbushTradersSpyHeroProximity")]
    pub ambush_traders_spy_hero_proximity: f32,
    #[def("AmbushTradersQuestSuccessReward")]
    pub ambush_traders_quest_success_reward: f32,
    #[def("AmbushTradersQuestSuccessBonus")]
    pub ambush_traders_quest_success_bonus: DefString,
    #[def("AmbushTradersBanditWanderMax")]
    pub ambush_traders_bandit_wander_max: f32,
    #[def("AmbushTradersExperience")]
    pub ambush_traders_experience: i32,
    #[def("AmbushTradersRenown")]
    pub ambush_traders_renown: i32,
    #[def("AmbushTradersMorality")]
    pub ambush_traders_morality: f32,
    #[def("AmbushTradersBoastCostKillAllGuards")]
    pub ambush_traders_boast_cost_kill_all_guards: i32,
    #[def("AmbushTradersBoastRewardKillAllGuards")]
    pub ambush_traders_boast_reward_kill_all_guards: i32,
    #[def("AmbushTradersBoastCostKillAllTraders")]
    pub ambush_traders_boast_cost_kill_all_traders: i32,
    #[def("AmbushTradersBoastRewardKillAllTraders")]
    pub ambush_traders_boast_reward_kill_all_traders: i32,
    #[def("AmbushTradersBoastCostKillNone")]
    pub ambush_traders_boast_cost_kill_none: i32,
    #[def("AmbushTradersBoastRewardKillNone")]
    pub ambush_traders_boast_reward_kill_none: i32,
    #[def("AmbushTradersBoastCostUnassisted")]
    pub ambush_traders_boast_cost_unassisted: i32,
    #[def("AmbushTradersBoastRewardUnassisted")]
    pub ambush_traders_boast_reward_unassisted: i32,
    #[def("AmbushTradersBanditFollowDistance")]
    pub ambush_traders_bandit_follow_distance: f32,
    #[def("AmbushTradersPhase2GuardWanderDistance")]
    pub ambush_traders_phase2_guard_wander_distance: f32,
    #[def("AmbushTradersSpeechBanditProximity")]
    pub ambush_traders_speech_bandit_proximity: f32,
    #[def("AmbushTradersSpeechSpyProximity")]
    pub ambush_traders_speech_spy_proximity: f32,
    #[def("AmbushTradersBanditSpeechTimerLong")]
    pub ambush_traders_bandit_speech_timer_long: i32,
    #[def("AmbushTradersBanditSpeechTimerShort")]
    pub ambush_traders_bandit_speech_timer_short: i32,
    #[def("AmbushTradersSpySpeechTimerProximity")]
    pub ambush_traders_spy_speech_timer_proximity: i32,
    #[def("AmbushTradersSpySpeechTimerLong")]
    pub ambush_traders_spy_speech_timer_long: i32,
    #[def("AmbushTradersSpySpeechTimerShort")]
    pub ambush_traders_spy_speech_timer_short: i32,
    #[def("AmbushTradersSpySpeechTimerSentry")]
    pub ambush_traders_spy_speech_timer_sentry: i32,
    #[def("AmbushTradersInvalidRegionTimerMax")]
    pub ambush_traders_invalid_region_timer_max: i32,
    #[def("AmbushTradersInvalidRegionTimerVar")]
    pub ambush_traders_invalid_region_timer_var: i32,
    #[def("ArenaCrowdShoutDelay")]
    pub arena_crowd_shout_delay: i32,
    #[def("ArenaCrowdShoutDelayRandom")]
    pub arena_crowd_shout_delay_random: i32,
    #[def("RewardObjects")]
    pub reward_objects: Vec<DefString>,
    #[def("ArenaCountdownDuration")]
    pub arena_countdown_duration: i32,
    #[def("ArenaCreatureRound1")]
    pub arena_creature_round1: String,
    #[def("ArenaNumberOrCreaturesRound1")]
    pub arena_number_or_creatures_round1: i32,
    #[def("ArenaNumberOfWavesRound1")]
    pub arena_number_of_waves_round1: i32,
    #[def("ArenaCreatureRound2")]
    pub arena_creature_round2: String,
    #[def("ArenaNumberOrCreaturesRound2")]
    pub arena_number_or_creatures_round2: i32,
    #[def("ArenaNumberOfWavesRound2")]
    pub arena_number_of_waves_round2: i32,
    #[def("ArenaCreatureRound3")]
    pub arena_creature_round3: String,
    #[def("ArenaNumberOrCreaturesRound3")]
    pub arena_number_or_creatures_round3: i32,
    #[def("ArenaNumberOfWavesRound3")]
    pub arena_number_of_waves_round3: i32,
    #[def("ArenaCreatureRound4")]
    pub arena_creature_round4: String,
    #[def("ArenaNumberOrCreaturesRound4")]
    pub arena_number_or_creatures_round4: i32,
    #[def("ArenaNumberOfWavesRound4")]
    pub arena_number_of_waves_round4: i32,
    #[def("ArenaCreatureRound5")]
    pub arena_creature_round5: String,
    #[def("ArenaNumberOrCreaturesRound5")]
    pub arena_number_or_creatures_round5: i32,
    #[def("ArenaNumberOfWavesRound5")]
    pub arena_number_of_waves_round5: i32,
    #[def("ArenaCreatureRound6")]
    pub arena_creature_round6: String,
    #[def("ArenaNumberOrCreaturesRound6")]
    pub arena_number_or_creatures_round6: i32,
    #[def("ArenaNumberOfWavesRound6")]
    pub arena_number_of_waves_round6: i32,
    #[def("ArenaCreatureRound7")]
    pub arena_creature_round7: String,
    #[def("ArenaNumberOrCreaturesRound7")]
    pub arena_number_or_creatures_round7: i32,
    #[def("ArenaNumberOfWavesRound7")]
    pub arena_number_of_waves_round7: i32,
    #[def("ArenaCreatureRound8")]
    pub arena_creature_round8: String,
    #[def("ArenaNumberOrCreaturesRound8")]
    pub arena_number_or_creatures_round8: i32,
    #[def("ArenaNumberOfWavesRound8")]
    pub arena_number_of_waves_round8: i32,
    #[def("ArenaShowNoMercyBoastAmount")]
    pub arena_show_no_mercy_boast_amount: i32,
    #[def("ArenaShowMercyBoastAmount")]
    pub arena_show_mercy_boast_amount: i32,
    #[def("ArenaShowNoMercyBoastReward")]
    pub arena_show_no_mercy_boast_reward: i32,
    #[def("ArenaShowMercyBoastReward")]
    pub arena_show_mercy_boast_reward: i32,
    #[def("ArenaSpareWhisperMorality")]
    pub arena_spare_whisper_morality: f32,
    #[def("ArenaKillWhisperMorality")]
    pub arena_kill_whisper_morality: f32,
    #[def("ATO_DiggingRewards")]
    pub ato_digging_rewards: Vec<DefString>,
    #[def("ATO_MaxSpawnedCreatures")]
    pub ato_max_spawned_creatures: i32,
    #[def("BountyHuntGoldReward")]
    pub bounty_hunt_gold_reward: i32,
    #[def("BountyHuntVillagerIntroProximity")]
    pub bounty_hunt_villager_intro_proximity: f32,
    #[def("BountyHuntVillagerWanderDistanceMin")]
    pub bounty_hunt_villager_wander_distance_min: f32,
    #[def("BountyHuntVillagerWanderDistanceMax")]
    pub bounty_hunt_villager_wander_distance_max: f32,
    #[def("BountyHuntVillagerHeroProximity")]
    pub bounty_hunt_villager_hero_proximity: f32,
    #[def("BountyHuntVillagerHeroStartConversation")]
    pub bounty_hunt_villager_hero_start_conversation: f32,
    #[def("BountyHuntCutsceneTriggerProximity")]
    pub bounty_hunt_cutscene_trigger_proximity: f32,
    #[def("BountyHuntBanditGruntWanderDistance")]
    pub bounty_hunt_bandit_grunt_wander_distance: f32,
    #[def("BountyHuntBanditLeaderWanderDistance")]
    pub bounty_hunt_bandit_leader_wander_distance: f32,
    #[def("BountyHuntHostage2WanderDistance")]
    pub bounty_hunt_hostage2_wander_distance: f32,
    #[def("BountyHuntBanditDeputy1WanderDistance")]
    pub bounty_hunt_bandit_deputy1_wander_distance: f32,
    #[def("BountyHuntBanditDeputy1AttackProximity")]
    pub bounty_hunt_bandit_deputy1_attack_proximity: f32,
    #[def("BountyHuntBanditDeputy2WanderDistance")]
    pub bounty_hunt_bandit_deputy2_wander_distance: f32,
    #[def("BountyHuntBanditDeputy2AttackProximity")]
    pub bounty_hunt_bandit_deputy2_attack_proximity: f32,
    #[def("BountyHuntLeaderHealthDiffStage1")]
    pub bounty_hunt_leader_health_diff_stage1: f32,
    #[def("BountyHuntLeaderHealthDiffStage2")]
    pub bounty_hunt_leader_health_diff_stage2: f32,
    #[def("BountyHuntLeaderHealthDiffStage3")]
    pub bounty_hunt_leader_health_diff_stage3: f32,
    #[def("BountyHuntLeaderHealthDiffStage4")]
    pub bounty_hunt_leader_health_diff_stage4: f32,
    #[def("BountyHuntExperience")]
    pub bounty_hunt_experience: i32,
    #[def("BountyHuntRenownHostage1")]
    pub bounty_hunt_renown_hostage1: i32,
    #[def("BountyHuntRenownHostage2")]
    pub bounty_hunt_renown_hostage2: i32,
    #[def("BountyHuntMorality")]
    pub bounty_hunt_morality: f32,
    #[def("BountyHuntCompletionMorality")]
    pub bounty_hunt_completion_morality: f32,
    #[def("BountyHuntBoastTimeLimit")]
    pub bounty_hunt_boast_time_limit: i32,
    #[def("BountyHuntBoastCostTimeLimit")]
    pub bounty_hunt_boast_cost_time_limit: i32,
    #[def("BountyHuntBoastRewardTimeLimit")]
    pub bounty_hunt_boast_reward_time_limit: i32,
    #[def("BountyHuntBoastCostDecapitation")]
    pub bounty_hunt_boast_cost_decapitation: i32,
    #[def("BountyHuntBoastRewardDecapitation")]
    pub bounty_hunt_boast_reward_decapitation: i32,
    #[def("BountyHuntRansomGold")]
    pub bounty_hunt_ransom_gold: i32,
    #[def("BountyHuntRewardObject")]
    pub bounty_hunt_reward_object: DefString,
    #[def("BowerstoneIntroGuardProximity")]
    pub bowerstone_intro_guard_proximity: f32,
    #[def("BowerstoneIntroPoshGuardProximity")]
    pub bowerstone_intro_posh_guard_proximity: f32,
    #[def("BowerstoneIntroCrowdWanderDistanceMin")]
    pub bowerstone_intro_crowd_wander_distance_min: f32,
    #[def("BowerstoneIntroCrowdWanderDistanceMax")]
    pub bowerstone_intro_crowd_wander_distance_max: f32,
    #[def("BS_BanditAwarenessRange")]
    pub bs_bandit_awareness_range: f32,
    #[def("BS_RewardGold")]
    pub bs_reward_gold: i32,
    #[def("BS_NoAssistanceBoastAmount")]
    pub bs_no_assistance_boast_amount: i32,
    #[def("BS_KilledLeaderBoastAmount")]
    pub bs_killed_leader_boast_amount: i32,
    #[def("BS_NoAssistanceBoastReward")]
    pub bs_no_assistance_boast_reward: i32,
    #[def("BS_KilledLeaderBoastReward")]
    pub bs_killed_leader_boast_reward: i32,
    #[def("BS_NoOfWavesInFirstAttack")]
    pub bs_no_of_waves_in_first_attack: i32,
    #[def("BS_NoOfBanditsInWave")]
    pub bs_no_of_bandits_in_wave: i32,
    #[def("BS_BanditTypeInFirstAttack")]
    pub bs_bandit_type_in_first_attack: DefString,
    #[def("BS_EnvTheme")]
    pub bs_env_theme: DefString,
    #[def("DBDragonMediumHealth")]
    pub db_dragon_medium_health: f32,
    #[def("DBDragonLowHealth")]
    pub db_dragon_low_health: f32,
    #[def("DBDragonVeryLowHealth")]
    pub db_dragon_very_low_health: f32,
    #[def("DBNumMinionsHighHealth")]
    pub db_num_minions_high_health: i32,
    #[def("DBNumMinionsMediumHealth")]
    pub db_num_minions_medium_health: i32,
    #[def("DBNumMinionsLowHealth")]
    pub db_num_minions_low_health: i32,
    #[def("DBNumMinionsVeryLowHealth")]
    pub db_num_minions_very_low_health: i32,
    #[def("DBNumSummonersHighHealth")]
    pub db_num_summoners_high_health: i32,
    #[def("DBNumSummonersMediumHealth")]
    pub db_num_summoners_medium_health: i32,
    #[def("DBNumSummonersLowHealth")]
    pub db_num_summoners_low_health: i32,
    #[def("DBNumSummonersVeryLowHealth")]
    pub db_num_summoners_very_low_health: i32,
    #[def("DBNumFlyBysBetweenSummonerSpawns")]
    pub db_num_fly_bys_between_summoner_spawns: i32,
    #[def("EndGame_UpperBarLimit")]
    pub end_game_upper_bar_limit: f32,
    #[def("EndGame_BarDecrementAmount")]
    pub end_game_bar_decrement_amount: f32,
    #[def("EndGame_BarDecrementInterval")]
    pub end_game_bar_decrement_interval: f32,
    #[def("EndGame_TeleportEffect")]
    pub end_game_teleport_effect: DefString,
    #[def("EndGame_TeleportHeroDistance")]
    pub end_game_teleport_hero_distance: i32,
    #[def("EndGame_Stage1FX")]
    pub end_game_stage1_fx: f32,
    #[def("EndGame_Stage2FX")]
    pub end_game_stage2_fx: f32,
    #[def("EndGame_Stage3FX")]
    pub end_game_stage3_fx: f32,
    #[def("FHSunPadFX")]
    pub fh_sun_pad_fx: String,
    #[def("FHMoonPadFX")]
    pub fh_moon_pad_fx: String,
    #[def("FHPadRadius")]
    pub fh_pad_radius: f32,
    #[def("FHPadFXHeight")]
    pub fh_pad_fx_height: f32,
    #[def("FHNumberOfPads")]
    pub fh_number_of_pads: i32,
    #[def("FHNumberOfProphets")]
    pub fh_number_of_prophets: i32,
    #[def("FHNumberOfPatterns")]
    pub fh_number_of_patterns: i32,
    #[def("FHNumberOfRounds")]
    pub fh_number_of_rounds: i32,
    #[def("FHNumberOfSunsEvil")]
    pub fh_number_of_suns_evil: i32,
    #[def("FHNumberOfSunsGood")]
    pub fh_number_of_suns_good: i32,
    #[def("FHTimeLimit")]
    pub fh_time_limit: Vec<i32>,
    #[def("FHPatterns")]
    pub fh_patterns: Vec<FireHeartPatternDef>,
    #[def("FHWarnTime")]
    pub fh_warn_time: i32,
    #[def("FHHealthChangeWhenZapped")]
    pub fh_health_change_when_zapped: f32,
    #[def("FHNumberOfVillagers")]
    pub fh_number_of_villagers: i32,
    #[def("FHMoralityChangeOnKilled")]
    pub fh_morality_change_on_killed: f32,
    #[def("FHMoralityChangeOnFreed")]
    pub fh_morality_change_on_freed: f32,
    #[def("FHFreeAllPrisonersMoralityChange")]
    pub fh_free_all_prisoners_morality_change: f32,
    #[def("FHFreeAllPrisonersRenownBoost")]
    pub fh_free_all_prisoners_renown_boost: i32,
    #[def("FHKillAllPrisonersMoralityChange")]
    pub fh_kill_all_prisoners_morality_change: f32,
    #[def("FHKillAllPrisonersRenownBoost")]
    pub fh_kill_all_prisoners_renown_boost: i32,
    #[def("GSI_MazeCallsOverDistance")]
    pub gsi_maze_calls_over_distance: i32,
    #[def("GSI_MazeRegionExitDistance")]
    pub gsi_maze_region_exit_distance: i32,
    #[def("HangingTreeProtectBanditsBoastReward")]
    pub hanging_tree_protect_bandits_boast_reward: i32,
    #[def("HangingTreeProtectBanditsBoastAmount")]
    pub hanging_tree_protect_bandits_boast_amount: i32,
    #[def("HangingTreeEvilMissionTimer")]
    pub hanging_tree_evil_mission_timer: i32,
    #[def("HTEvilCompletionMorality")]
    pub ht_evil_completion_morality: f32,
    #[def("HangingTreeProtectGuardsBoastReward")]
    pub hanging_tree_protect_guards_boast_reward: i32,
    #[def("HangingTreeProtectGuardsBoastAmount")]
    pub hanging_tree_protect_guards_boast_amount: i32,
    #[def("HTGoodCompletionMorality")]
    pub ht_good_completion_morality: f32,
    #[def("HTGoodHeroKilledPrisonerGoldReward")]
    pub ht_good_hero_killed_prisoner_gold_reward: i32,
    #[def("HTGoodHeroKilledPrisonerRenownReward")]
    pub ht_good_hero_killed_prisoner_renown_reward: i32,
    #[def("HS_ScreamerCount")]
    pub hs_screamer_count: f32,
    #[def("HS_ScreamersAtOnce")]
    pub hs_screamers_at_once: i32,
    #[def("HS_ScreamerInc")]
    pub hs_screamer_inc: f32,
    #[def("HS_SecondsBeforeSuperScreamerDamage")]
    pub hs_seconds_before_super_screamer_damage: i32,
    #[def("HS_SuperScreamerDamagePerSecond")]
    pub hs_super_screamer_damage_per_second: f32,
    #[def("HS_SecondsBeforeNewSuperScreamer")]
    pub hs_seconds_before_new_super_screamer: i32,
    #[def("HS_TimePerUndeadKilled")]
    pub hs_time_per_undead_killed: i32,
    #[def("HS_MaxTimeNostroExposed")]
    pub hs_max_time_nostro_exposed: i32,
    #[def("HS_GMShortDistance")]
    pub hs_gm_short_distance: i32,
    #[def("HS_GMLongDistance")]
    pub hs_gm_long_distance: i32,
    #[def("HS_GMGuardDistance")]
    pub hs_gm_guard_distance: i32,
    #[def("HS_GoodSoulTaken")]
    pub hs_good_soul_taken: f32,
    #[def("HC_SpawnCountPostReleaseGSon")]
    pub hc_spawn_count_post_release_g_son: i32,
    #[def("HC_ReduceCountPostSideRoom")]
    pub hc_reduce_count_post_side_room: i32,
    #[def("HC_SpawnGenerationRadius")]
    pub hc_spawn_generation_radius: f32,
    #[def("HC_BanditTimeBeforeComment")]
    pub hc_bandit_time_before_comment: i32,
    #[def("HC_BanditCallsForHelpDelay")]
    pub hc_bandit_calls_for_help_delay: i32,
    #[def("HC_BanditGoldDemand")]
    pub hc_bandit_gold_demand: i32,
    #[def("HC_BanditFadeOut")]
    pub hc_bandit_fade_out: f32,
    #[def("HC_HobbeSpawnDelay")]
    pub hc_hobbe_spawn_delay: i32,
    #[def("HC_BanditCommentTime")]
    pub hc_bandit_comment_time: i32,
    #[def("HC_BanditCloseToMarkerDistance")]
    pub hc_bandit_close_to_marker_distance: i32,
    #[def("HC_HobbeGruntDelay")]
    pub hc_hobbe_grunt_delay: i32,
    #[def("HC_HobbeSpawnCount")]
    pub hc_hobbe_spawn_count: i32,
    #[def("HC_RescueBanditMoralityGain")]
    pub hc_rescue_bandit_morality_gain: f32,
    #[def("HC_RescueBanditExperienceGain")]
    pub hc_rescue_bandit_experience_gain: i32,
    #[def("HC_SacrificeToNymphExperienceGain")]
    pub hc_sacrifice_to_nymph_experience_gain: i32,
    #[def("HC_SacrificeBanditMoralityLoss")]
    pub hc_sacrifice_bandit_morality_loss: f32,
    #[def("HC_SacrificeToNymphMoralityLoss")]
    pub hc_sacrifice_to_nymph_morality_loss: f32,
    #[def("HC_LetBanditGoAndPayMoralityGain")]
    pub hc_let_bandit_go_and_pay_morality_gain: f32,
    #[def("HC_LetBanditGoRenown")]
    pub hc_let_bandit_go_renown: i32,
    #[def("HC_DontPayBanditMoralityLoss")]
    pub hc_dont_pay_bandit_morality_loss: f32,
    #[def("HC_KillNymphBoastCost")]
    pub hc_kill_nymph_boast_cost: i32,
    #[def("HC_KillNymphBoastReward")]
    pub hc_kill_nymph_boast_reward: i32,
    #[def("HC_SacrificeBoastCost")]
    pub hc_sacrifice_boast_cost: i32,
    #[def("HC_SacrificeBoastReward")]
    pub hc_sacrifice_boast_reward: i32,
    #[def("HC_BoyNoHarmBoastCost")]
    pub hc_boy_no_harm_boast_cost: i32,
    #[def("HC_BoyNoHarmBoastReward")]
    pub hc_boy_no_harm_boast_reward: i32,
    #[def("HC_HobbeMaxNumberInRegion")]
    pub hc_hobbe_max_number_in_region: i32,
    #[def("JackFightDefeatJackXP")]
    pub jack_fight_defeat_jack_xp: i32,
    #[def("MB_MadBomberStartDistance")]
    pub mb_mad_bomber_start_distance: i32,
    #[def("MB_MadBomberProximityToBomb")]
    pub mb_mad_bomber_proximity_to_bomb: i32,
    #[def("MB_MadBomberHeroCloseDelay")]
    pub mb_mad_bomber_hero_close_delay: i32,
    #[def("MB_MadBomberRunToWalkDistance")]
    pub mb_mad_bomber_run_to_walk_distance: i32,
    #[def("MB_BlacksmithCallHeroDistance")]
    pub mb_blacksmith_call_hero_distance: i32,
    #[def("MB_BombInitialTimeToBoom")]
    pub mb_bomb_initial_time_to_boom: i32,
    #[def("MB_FailMissionBombCount")]
    pub mb_fail_mission_bomb_count: i32,
    #[def("MB_BombGoesOffRenownLoss")]
    pub mb_bomb_goes_off_renown_loss: i32,
    #[def("MB_BombDefusedExperience")]
    pub mb_bomb_defused_experience: i32,
    #[def("MB_BombDefusedMorality")]
    pub mb_bomb_defused_morality: f32,
    #[def("MCNumHitsBetweenForcePushes")]
    pub mc_num_hits_between_force_pushes: i32,
    #[def("MCMinMinionSpawnPeriod")]
    pub mc_min_minion_spawn_period: f32,
    #[def("MCMaxMinionSpawnPeriod")]
    pub mc_max_minion_spawn_period: f32,
    #[def("MCC_GuardTalkingDistance")]
    pub mcc_guard_talking_distance: i32,
    #[def("MCC_MinionRunDistance")]
    pub mcc_minion_run_distance: i32,
    #[def("MCC_MinionBreakoffDistance")]
    pub mcc_minion_breakoff_distance: f32,
    #[def("MCC_SavedGuardMoralityGain")]
    pub mcc_saved_guard_morality_gain: f32,
    #[def("MCC_Timer")]
    pub mcc_timer: i32,
    #[def("MCC_MinionCounter")]
    pub mcc_minion_counter: i32,
    #[def("OVBRBanditSightRadius")]
    pub ovbr_bandit_sight_radius: f32,
    #[def("OVBRBanditSoundRadius")]
    pub ovbr_bandit_sound_radius: f32,
    #[def("OVI_MoralityChangePerDeed")]
    pub ovi_morality_change_per_deed: f32,
    #[def("ExperienceForDefeatingWhisperInOrchardFarm")]
    pub experience_for_defeating_whisper_in_orchard_farm: i32,
    #[def("GuardReinforcementsYourTeam")]
    pub guard_reinforcements_your_team: i32,
    #[def("GuardReinforcementsEnemyTeam")]
    pub guard_reinforcements_enemy_team: i32,
    #[def("BanditReinforcementDelay")]
    pub bandit_reinforcement_delay: i32,
    #[def("OFE_NoHealthPotionBoastCost")]
    pub ofe_no_health_potion_boast_cost: i32,
    #[def("OFE_NoHealthPotionBoastReward")]
    pub ofe_no_health_potion_boast_reward: i32,
    #[def("OFE_NoBanditsDieBoastCost")]
    pub ofe_no_bandits_die_boast_cost: i32,
    #[def("OFE_NoBanditsDieBoastReward")]
    pub ofe_no_bandits_die_boast_reward: i32,
    #[def("OFG_NoCratesStolenBoastCost")]
    pub ofg_no_crates_stolen_boast_cost: i32,
    #[def("OFG_NoCratesStolenBoastReward")]
    pub ofg_no_crates_stolen_boast_reward: i32,
    #[def("OFG_NoGuardsDieBoastCost")]
    pub ofg_no_guards_die_boast_cost: i32,
    #[def("OFG_NoGuardsDieBoastReward")]
    pub ofg_no_guards_die_boast_reward: i32,
    #[def("OFEvilCompletionMorality")]
    pub of_evil_completion_morality: f32,
    #[def("OFGoodCompletionMorality")]
    pub of_good_completion_morality: f32,
    #[def("PrisonExerciseStartingTimeLimit")]
    pub prison_exercise_starting_time_limit: i32,
    #[def("PrisonWayPointHitRange")]
    pub prison_way_point_hit_range: i32,
    #[def("PrisonKeyRackDangerRange")]
    pub prison_key_rack_danger_range: i32,
    #[def("PrisonClothesChestDangerRange")]
    pub prison_clothes_chest_danger_range: i32,
    #[def("PrisonGuardProximityToAskQuestion")]
    pub prison_guard_proximity_to_ask_question: i32,
    #[def("PrisonTrainingPartnerHintDistance")]
    pub prison_training_partner_hint_distance: i32,
    #[def("PrisonTrainingPartnerHintDistance2")]
    pub prison_training_partner_hint_distance2: i32,
    #[def("PrisonDistanceForGuardsToJeer")]
    pub prison_distance_for_guards_to_jeer: i32,
    #[def("PrisonRadiusGuardWillAttackHeroWithin")]
    pub prison_radius_guard_will_attack_hero_within: i32,
    #[def("PrisonGuardMumbleAudioDistance")]
    pub prison_guard_mumble_audio_distance: i32,
    #[def("PrisonRacerRunSpeed")]
    pub prison_racer_run_speed: f32,
    #[def("PrisonRacerSprintSpeed")]
    pub prison_racer_sprint_speed: f32,
    #[def("PrisonRacerSprintMaxTimeSeconds")]
    pub prison_racer_sprint_max_time_seconds: i32,
    #[def("PrisonRacerSprintRestTimeSeconds")]
    pub prison_racer_sprint_rest_time_seconds: i32,
    #[def("PrisonWardenGameSneakModifier")]
    pub prison_warden_game_sneak_modifier: f32,
    #[def("PrisonPoetryMaxLines")]
    pub prison_poetry_max_lines: i32,
    #[def("PrisonExperienceForKillingKraken")]
    pub prison_experience_for_killing_kraken: i32,
    #[def("PrisonEscapeExperienceFromMother")]
    pub prison_escape_experience_from_mother: i32,
    #[def("PrisonFreePrisonerMorality")]
    pub prison_free_prisoner_morality: f32,
    #[def("MotherFollowingHeroKrakenSafetyDistance")]
    pub mother_following_hero_kraken_safety_distance: i32,
    #[def("TE_TraderCommentDelay")]
    pub te_trader_comment_delay: i32,
    #[def("TE_TraderToBalverineDelay")]
    pub te_trader_to_balverine_delay: i32,
    #[def("TE_TraderLowInfectionTime")]
    pub te_trader_low_infection_time: i32,
    #[def("TE_TraderMediumInfectionTime")]
    pub te_trader_medium_infection_time: i32,
    #[def("TE_TraderHighInfectionTime")]
    pub te_trader_high_infection_time: i32,
    #[def("TE_AllTradersAliveBoastCost")]
    pub te_all_traders_alive_boast_cost: i32,
    #[def("TE_AllTradersAliveBoastReward")]
    pub te_all_traders_alive_boast_reward: i32,
    #[def("TE_BalverineTimeToGoDistance")]
    pub te_balverine_time_to_go_distance: i32,
    #[def("TE_TraderCommentDistance")]
    pub te_trader_comment_distance: i32,
    #[def("TE_AssassinSpawnDistance")]
    pub te_assassin_spawn_distance: i32,
    #[def("TE_AssassinName")]
    pub te_assassin_name: DefString,
    #[def("TE_EarthTrollTriggerDistance")]
    pub te_earth_troll_trigger_distance: i32,
    #[def("TE_TrollName")]
    pub te_troll_name: DefString,
    #[def("TE_TraderGoldReward")]
    pub te_trader_gold_reward: i32,
    #[def("TE_LetTraderFollowMorality")]
    pub te_let_trader_follow_morality: f32,
    #[def("TraderEscortLeaveTraderMorality")]
    pub trader_escort_leave_trader_morality: f32,
    #[def("WB_MoralityGain")]
    pub wb_morality_gain: f32,
    #[def("WB_LowHealth")]
    pub wb_low_health: i32,
    #[def("WB_GoldReward")]
    pub wb_gold_reward: i32,
    #[def("WB_CreaturesSpawned")]
    pub wb_creatures_spawned: DefString,
    #[def("WB_WaspBossName")]
    pub wb_wasp_boss_name: DefString,
    #[def("WB_ScreamingVillagerDistance")]
    pub wb_screaming_villager_distance: i32,
    #[def("WB_ScreamingVillagerScreamsDistance")]
    pub wb_screaming_villager_screams_distance: i32,
    #[def("WB_ScreamingVillagerFadeOutTime")]
    pub wb_screaming_villager_fade_out_time: f32,
    #[def("WB_WaspHelperCallsOutDistance")]
    pub wb_wasp_helper_calls_out_distance: i32,
    #[def("WB_WaspChaseWomanPanicTime")]
    pub wb_wasp_chase_woman_panic_time: i32,
    #[def("WB_ExperienceReward")]
    pub wb_experience_reward: i32,
    #[def("BC_EvilMoralityLoss")]
    pub bc_evil_morality_loss: f32,
    #[def("BC_GoodMoralityGain")]
    pub bc_good_morality_gain: f32,
    #[def("BAC_RenownAward")]
    pub bac_renown_award: f32,
    #[def("BAC_AgeAmount")]
    pub bac_age_amount: f32,
    #[def("BAC_HostageMoralityGain")]
    pub bac_hostage_morality_gain: f32,
    #[def("BAC_SingleHostageMoralityLoss")]
    pub bac_single_hostage_morality_loss: f32,
    #[def("BAC_DoubleHostageMoralityLoss")]
    pub bac_double_hostage_morality_loss: f32,
    #[def("BAC_EnemiesToKillInFirstArea")]
    pub bac_enemies_to_kill_in_first_area: f32,
    #[def("BAC_EnemiesToKillInSecondArea")]
    pub bac_enemies_to_kill_in_second_area: f32,
    #[def("BAC_AssassinGoldAmount")]
    pub bac_assassin_gold_amount: f32,
    #[def("BAC_ForgerGoldAmount")]
    pub bac_forger_gold_amount: f32,
    #[def("BAC_BoastBanditKill")]
    pub bac_boast_bandit_kill: f32,
    #[def("BAC_TheresaExperienceGift")]
    pub bac_theresa_experience_gift: f32,
    #[def("BAC_TwinBladeKilledMorality")]
    pub bac_twin_blade_killed_morality: f32,
    #[def("BAC_TwinBladeSavedMorality")]
    pub bac_twin_blade_saved_morality: f32,
    #[def("EGJ_MoralityLoss")]
    pub egj_morality_loss: f32,
    #[def("EGJ_MoralityGain")]
    pub egj_morality_gain: f32,
    #[def("ENF_TotalTime")]
    pub enf_total_time: f32,
    #[def("ENF_TimeAddition")]
    pub enf_time_addition: f32,
    #[def("ENF_TimeSubtraction")]
    pub enf_time_subtraction: f32,
    #[def("HOB_TotalTime")]
    pub hob_total_time: f32,
    #[def("HOB_BoastHobbeMany")]
    pub hob_boast_hobbe_many: f32,
    #[def("HOB_RenownAwardSuccess")]
    pub hob_renown_award_success: f32,
    #[def("HOB_RenownAwardDraw")]
    pub hob_renown_award_draw: f32,
    #[def("HOB_RenownAwardLost")]
    pub hob_renown_award_lost: f32,
    #[def("HOB_KillMoreHobbesBoastCost")]
    pub hob_kill_more_hobbes_boast_cost: i32,
    #[def("HOB_KillMoreHobbesBoastReward")]
    pub hob_kill_more_hobbes_boast_reward: i32,
    #[def("OGSP_KillAllBoastCost")]
    pub ogsp_kill_all_boast_cost: i32,
    #[def("OGSP_KillAllBoastReward")]
    pub ogsp_kill_all_boast_reward: i32,
    #[def("GUI_MeleeGrades")]
    pub gui_melee_grades: Vec<f32>,
    #[def("GUI_SkillGrades")]
    pub gui_skill_grades: Vec<f32>,
    #[def("GUI_WillGrades")]
    pub gui_will_grades: Vec<f32>,
    #[def("GUI_MinHealth")]
    pub gui_min_health: f32,
    #[def("GUI_RaceTime")]
    pub gui_race_time: f32,
    #[def("GUI_RaceGold")]
    pub gui_race_gold: f32,
    #[def("GUI_RearDummySegements")]
    pub gui_rear_dummy_segements: f32,
    #[def("GUI_MiddleDummySegements")]
    pub gui_middle_dummy_segements: f32,
    #[def("GUI_FrontDummySegements")]
    pub gui_front_dummy_segements: f32,
    #[def("GUI_RearDummyWorth")]
    pub gui_rear_dummy_worth: f32,
    #[def("GUI_MiddleDummyWorth")]
    pub gui_middle_dummy_worth: f32,
    #[def("GUI_FrontDummyWorth")]
    pub gui_front_dummy_worth: f32,
    #[def("GUI_GoldPerBird")]
    pub gui_gold_per_bird: f32,
    #[def("GUI_BirdGoldBonus")]
    pub gui_bird_gold_bonus: f32,
    #[def("GUI_SkillTimer")]
    pub gui_skill_timer: f32,
    #[def("GUI_WillTimer")]
    pub gui_will_timer: f32,
    #[def("GUI_DepartureBeetles")]
    pub gui_departure_beetles: f32,
    #[def("GUI_MeleeBeetles")]
    pub gui_melee_beetles: f32,
    #[def("GUI_LampCost")]
    pub gui_lamp_cost: f32,
    #[def("GUI_WillDummySpinTimer")]
    pub gui_will_dummy_spin_timer: f32,
    #[def("GUI_EndGuildXP")]
    pub gui_end_guild_xp: f32,
    #[def("HOH_MoralityGain")]
    pub hoh_morality_gain: f32,
    #[def("HOH_MoralityLoss")]
    pub hoh_morality_loss: f32,
    #[def("RV_AcceptedGuardLoweredGoldReward")]
    pub rv_accepted_guard_lowered_gold_reward: i32,
    #[def("RV_AcceptedGuardLoweredRenownReward")]
    pub rv_accepted_guard_lowered_renown_reward: i32,
    #[def("RV_LowerGoldRewardFromKillingChiefsGuard")]
    pub rv_lower_gold_reward_from_killing_chiefs_guard: i32,
    #[def("RV_HeroEvilMoralityValue")]
    pub rv_hero_evil_morality_value: f32,
    #[def("RV_HeroGoodMoralityValue")]
    pub rv_hero_good_morality_value: f32,
    #[def("RV_HeroTellMoralityValue")]
    pub rv_hero_tell_morality_value: f32,
    #[def("TCE_TimeLimit")]
    pub tce_time_limit: i32,
    #[def("TCG_TimeLimit")]
    pub tcg_time_limit: i32,
    #[def("TCE_GuardRangeHighest")]
    pub tce_guard_range_highest: i32,
    #[def("TCE_GuardRangeHigh")]
    pub tce_guard_range_high: i32,
    #[def("TCE_GuardRangeLow")]
    pub tce_guard_range_low: i32,
    #[def("WBK_NumberHits")]
    pub wbk_number_hits: f32,
    #[def("WBK_RenownAwardForGates")]
    pub wbk_renown_award_for_gates: f32,
    #[def("WBK_RenownAwardForFirstDefence")]
    pub wbk_renown_award_for_first_defence: f32,
    #[def("WBK_RenownAwardForSecondDefence")]
    pub wbk_renown_award_for_second_defence: f32,
    #[def("WizardBattleDefeatMazeXP")]
    pub wizard_battle_defeat_maze_xp: i32,
    #[def("CHK_LowPrize")]
    pub chk_low_prize: f32,
    #[def("CHK_MidPrize")]
    pub chk_mid_prize: f32,
    #[def("CHK_HighPrize")]
    pub chk_high_prize: f32,
    #[def("TeleporterActivationDist")]
    pub teleporter_activation_dist: i32,
    #[def("TimeAdvancePointFadeOutTime")]
    pub time_advance_point_fade_out_time: f32,
    #[def("TimeAdvancePointFadeInTime")]
    pub time_advance_point_fade_in_time: f32,
    #[def("TimeAdvancePointFadedOutPauseTime")]
    pub time_advance_point_faded_out_pause_time: f32,
    #[def("TimeAdvancePointTimeToAdvance")]
    pub time_advance_point_time_to_advance: f32,
    #[def("CoreQuestReminderIntervalSeconds")]
    pub core_quest_reminder_interval_seconds: i32,
    #[def("FlourishReminderFrequency")]
    pub flourish_reminder_frequency: i32,
    #[def("PercentageFlourishesTakenLowerBound")]
    pub percentage_flourishes_taken_lower_bound: f32,
    #[def("StopTeachingFlourishesAfterDoneThisMany")]
    pub stop_teaching_flourishes_after_done_this_many: i32,
    #[def("CombatMultTooLongUnchanged")]
    pub combat_mult_too_long_unchanged: i32,
    #[def("CombatMultStopBotheringAfter")]
    pub combat_mult_stop_bothering_after: i32,
    #[def("LowHealthWarning")]
    pub low_health_warning: i32,
    #[def("VeryLowHealthWarning")]
    pub very_low_health_warning: i32,
    #[def("LowHealthMessageInterval")]
    pub low_health_message_interval: i32,
    #[def("VeryLowHealthMessageInterval")]
    pub very_low_health_message_interval: i32,
    #[def("HealthMessageLimit")]
    pub health_message_limit: i32,
    #[def("LowWillEnergyWarningLevel")]
    pub low_will_energy_warning_level: f32,
    #[def("LowWillEnergyMessageInterval")]
    pub low_will_energy_message_interval: i32,
    #[def("WillEnergyMessageLimit")]
    pub will_energy_message_limit: i32,
    #[def("SaveMessageLimit")]
    pub save_message_limit: i32,
    #[def("SaveMessageInterval")]
    pub save_message_interval: i32,
    #[def("FishingMaxDistanceFromSpot")]
    pub fishing_max_distance_from_spot: f32,
    #[def("DiggingMaxDistanceFromSpot")]
    pub digging_max_distance_from_spot: f32,
    #[def("PickpocketDurationSeconds")]
    pub pickpocket_duration_seconds: f32,
    #[def("PickpocketSpottedDurationSeconds")]
    pub pickpocket_spotted_duration_seconds: f32,
    #[def("PickpocketSpottedChancePerSecond")]
    pub pickpocket_spotted_chance_per_second: i32,
    #[def("PicklockDurationSeconds")]
    pub picklock_duration_seconds: f32,
    #[def("PicklockSpottedDurationSeconds")]
    pub picklock_spotted_duration_seconds: f32,
    #[def("PicklockSpottedChancePerSecond")]
    pub picklock_spotted_chance_per_second: i32,
    #[def("StealDurationSeconds")]
    pub steal_duration_seconds: f32,
    #[def("StealSpottedDurationSeconds")]
    pub steal_spotted_duration_seconds: f32,
    #[def("StealSpottedChancePerSecond")]
    pub steal_spotted_chance_per_second: i32,
    #[def("NumberOfConcurrentlyAvailableFeats")]
    pub number_of_concurrently_available_feats: i32,
    #[def("Feats")]
    pub feats: Vec<FeatDef>,
    #[def("ArenaRounds")]
    pub arena_rounds: Vec<ArenaRoundDef>,
    #[def("HeroSoulsRounds")]
    pub hero_souls_rounds: Vec<HeroSoulsRoundDef>,
    #[def("DemonDoor_ShineLight_TriggerDistance")]
    pub demon_door_shine_light_trigger_distance: f32,
    #[def("DemonDoor_HighRenown_TriggerLevel")]
    pub demon_door_high_renown_trigger_level: f32,
    #[def("DemonDoor_CombatMult_TriggerLevel")]
    pub demon_door_combat_mult_trigger_level: i32,
    #[def("DemonDoor_SummonCreature_NumCreatureTypes")]
    pub demon_door_summon_creature_num_creature_types: i32,
    #[def("DemonDoor_SummonCreature_CreatureType")]
    pub demon_door_summon_creature_creature_type: Vec<String>,
    #[def("DemonDoor_BeEvil_TriggerDistance")]
    pub demon_door_be_evil_trigger_distance: i32,
    #[def("DemonDoor_BeEvil_EvilnessNeeded")]
    pub demon_door_be_evil_evilness_needed: f32,
    #[def("DemonDoor_DoDamage_DamageLimit")]
    pub demon_door_do_damage_damage_limit: f32,
    #[def("DemonDoor_EatPies_FatnessLevel")]
    pub demon_door_eat_pies_fatness_level: f32,
}
