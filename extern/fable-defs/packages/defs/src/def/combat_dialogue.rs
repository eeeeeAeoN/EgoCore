use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatDialogueDef {
    #[def("MinTimeBetweenSpeech")]
    pub min_time_between_speech: f32,
    #[def("MinTimeBetweenSpeechRepeat")]
    pub min_time_between_speech_repeat: f32,
    #[def("MinDelayAfterBattleCry")]
    pub min_delay_after_battle_cry: f32,
    #[def("EnemySighted")]
    pub enemy_sighted: i32,
    #[def("RunForCover")]
    pub run_for_cover: i32,
    #[def("Hiding")]
    pub hiding: i32,
    #[def("Ambushing")]
    pub ambushing: i32,
    #[def("WillSeenUsed")]
    pub will_seen_used: i32,
    #[def("Threat")]
    pub threat: i32,
    #[def("Encourage")]
    pub encourage: i32,
    #[def("FoeKnockedDownByColleague")]
    pub foe_knocked_down_by_colleague: i32,
    #[def("MenLosing")]
    pub men_losing: i32,
    #[def("EnemyKnockedDown")]
    pub enemy_knocked_down: i32,
    #[def("RangedCombatAiming")]
    pub ranged_combat_aiming: i32,
    #[def("RangedCombatAimingBlocked")]
    pub ranged_combat_aiming_blocked: i32,
    #[def("Injured")]
    pub injured: i32,
    #[def("ReactionToDecapitation")]
    pub reaction_to_decapitation: i32,
    #[def("ColleagueInjured")]
    pub colleague_injured: i32,
    #[def("Scared")]
    pub scared: i32,
    #[def("Flee")]
    pub flee: i32,
}
