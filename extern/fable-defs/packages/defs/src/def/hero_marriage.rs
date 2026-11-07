use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroMarriageDef {
    #[def("WeddingTimeForScreenToFadeOut")]
    pub wedding_time_for_screen_to_fade_out: f32,
    #[def("WeddingTimeForScreenToFadeIn")]
    pub wedding_time_for_screen_to_fade_in: f32,
    #[def("WeddingAfterFrescoTimeOfDayFastForwardTo")]
    pub wedding_after_fresco_time_of_day_fast_forward_to: f32,
    #[def("SecondsJustMarriedDuration")]
    pub seconds_just_married_duration: f32,
    #[def("HeroStatRenownIncreasePerMarriage")]
    pub hero_stat_renown_increase_per_marriage: f32,
    #[def("SecondsBetweenCheckingPlayersAppearance")]
    pub seconds_between_checking_players_appearance: i32,
    #[def("SecondsMaxTimeWithinNoticingCanReact")]
    pub seconds_max_time_within_noticing_can_react: i32,
    #[def("FatnessChangeThresholdForComment")]
    pub fatness_change_threshold_for_comment: f32,
    #[def("SecondsBetweenReceivingGiftOpinionReactionCanHappen")]
    pub seconds_between_receiving_gift_opinion_reaction_can_happen: i32,
    #[def("SecondsThatGiftsReceivedWillBeRemembered")]
    pub seconds_that_gifts_received_will_be_remembered: i32,
    #[def("FramesBetweenCullingOldGifts")]
    pub frames_between_culling_old_gifts: i32,
    #[def("SecondsBetweenEvaluatingGiftOpinion")]
    pub seconds_between_evaluating_gift_opinion: i32,
    #[def("SecondsBetweenGiftGivingOpportunities")]
    pub seconds_between_gift_giving_opportunities: i32,
    #[def("ChanceThatGiftWillBeGiven")]
    pub chance_that_gift_will_be_given: f32,
    #[def("AmountOfGoldPerHourOfMarriageToIncreaseGiftGivingPriceValuePerHourOfPlay")]
    pub amount_of_gold_per_hour_of_marriage_to_increase_gift_giving_price_value_per_hour_of_play:
        f32,
    #[def("MaxNumHoursOfMarriageToIncreaseGiftGivingPriceValue")]
    pub max_num_hours_of_marriage_to_increase_gift_giving_price_value: f32,
    #[def("MultiplierForGiftGivingValueEveryOpportunity")]
    pub multiplier_for_gift_giving_value_every_opportunity: f32,
    #[def("MaxDistanceFromMaxStatsThatGiftsCanBeGiven")]
    pub max_distance_from_max_stats_that_gifts_can_be_given: f32,
    #[def("SecondsBetweenEvaluatingLoveAttitude")]
    pub seconds_between_evaluating_love_attitude: i32,
    #[def("SecondsOutOfTheRegionForLoveAttitudeToDecayOneToZero")]
    pub seconds_out_of_the_region_for_love_attitude_to_decay_one_to_zero: f32,
    #[def("ThresholdForLoveAttitudeOverride")]
    pub threshold_for_love_attitude_override: f32,
    #[def("FallenInLoveSoundCriteria")]
    pub fallen_in_love_sound_criteria: String,
    #[def("FallenInLoveSoundGain")]
    pub fallen_in_love_sound_gain: f32,
    #[def("FianceeOpinionPersonalityDef")]
    pub fiancee_opinion_personality_def: DefIndex,
    #[def("ProbabilityOfCourtingAdviceOnGreet")]
    pub probability_of_courting_advice_on_greet: f32,
    #[def("SecondsTimeSinceSeeingYouMinThreshold")]
    pub seconds_time_since_seeing_you_min_threshold: f32,
    #[def("SecondsRunOutWifeOnViolenceHateSpike")]
    pub seconds_run_out_wife_on_violence_hate_spike: f32,
    #[def("PeakEffectWifeOnViolenceHateSpike")]
    pub peak_effect_wife_on_violence_hate_spike: f32,
    #[def("PersistEffectWifeOnViolenceHateSpike")]
    pub persist_effect_wife_on_violence_hate_spike: f32,
    #[def("SecsInHateBeforeDivorceOccurs")]
    pub secs_in_hate_before_divorce_occurs: f32,
    #[def("SecsInHateBetweenDivorceWarnings")]
    pub secs_in_hate_between_divorce_warnings: f32,
    #[def("TextGuiWifeDivorceOccurred")]
    pub text_gui_wife_divorce_occurred: String,
    #[def("TextGuiWifeDivorceRenting")]
    pub text_gui_wife_divorce_renting: String,
    #[def("HeroStatMoralityInOpinionFormChangePerDivorce")]
    pub hero_stat_morality_in_opinion_form_change_per_divorce: f32,
    #[def("SecsInLoveAndHusbandPresentBeforeSexOffered")]
    pub secs_in_love_and_husband_present_before_sex_offered: f32,
    #[def("SecsInLoveAndHusbandPresentBetweenSexOffers")]
    pub secs_in_love_and_husband_present_between_sex_offers: f32,
    #[def("TextGuiWifeSexOffered")]
    pub text_gui_wife_sex_offered: String,
    #[def("SecsMinSoundDurationInSexCutscene")]
    pub secs_min_sound_duration_in_sex_cutscene: f32,
    #[def("SecsWaitAfterSoundInSexCutscene")]
    pub secs_wait_after_sound_in_sex_cutscene: f32,
    #[def("DialogueGroupForSexCutscene")]
    pub dialogue_group_for_sex_cutscene: String,
}
