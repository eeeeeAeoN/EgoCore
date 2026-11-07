use crate::DefStruct;
use crate::def::{AbilityTrackDef, AppearanceModifierScalingDef};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroStatsDef {
    #[def("MaximumStamina")]
    pub maximum_stamina: i32,
    #[def("NumberOfSecondsAfterUseStaminaStartsRefilling")]
    pub number_of_seconds_after_use_stamina_starts_refilling: f32,
    #[def("NumberOfSecondsAfterChangeHealthStartsRefilling")]
    pub number_of_seconds_after_change_health_starts_refilling: f32,
    #[def("NumberOfSecondsBetweenHealthRefills")]
    pub number_of_seconds_between_health_refills: f32,
    #[def("StaminaToRegainPerSecond")]
    pub stamina_to_regain_per_second: i32,
    #[def("HealthToRegainPerRefill")]
    pub health_to_regain_per_refill: i32,
    #[def("RenownLossOnDeath")]
    pub renown_loss_on_death: i32,
    #[def("MoneyLossOnDeath")]
    pub money_loss_on_death: i32,
    #[def("MaximumMoney")]
    pub maximum_money: i32,
    #[def("MaximumExpToSpend")]
    pub maximum_exp_to_spend: i32,
    #[def("ExpToSpendAtWhichToDisplayTutorial")]
    pub exp_to_spend_at_which_to_display_tutorial: i32,
    #[def("MinAge")]
    pub min_age: f32,
    #[def("MaxAge")]
    pub max_age: f32,
    #[def("AppearanceModifierScaling")]
    pub appearance_modifier_scaling: Vec<AppearanceModifierScalingDef>,
    #[def("AmountFatnessIncreasesPerPointOfOverEating")]
    pub amount_fatness_increases_per_point_of_over_eating: f32,
    #[def("NumberOfMinutesBetweenFatnessReductions")]
    pub number_of_minutes_between_fatness_reductions: f32,
    #[def("AmountFatnessDecreasesPerReduction")]
    pub amount_fatness_decreases_per_reduction: f32,
    #[def("DrinkAffectMultiplierMultiplier")]
    pub drink_affect_multiplier_multiplier: f32,
    #[def("MinVomitTime")]
    pub min_vomit_time: i32,
    #[def("MaxVomitTime")]
    pub max_vomit_time: i32,
    #[def("MinTimeBeforeVomiting")]
    pub min_time_before_vomiting: i32,
    #[def("MaxTimeBeforeVomiting")]
    pub max_time_before_vomiting: i32,
    #[def("PintsOfSobrietyPerSecondOfPuke")]
    pub pints_of_sobriety_per_second_of_puke: f32,
    #[def("PintsToDrunk")]
    pub pints_to_drunk: f32,
    #[def("PintsToVomit")]
    pub pints_to_vomit: f32,
    #[def("AlcoholAbsorbtionPerSecond")]
    pub alcohol_absorbtion_per_second: f32,
    #[def("RenownLevelCosts")]
    pub renown_level_costs: Vec<i32>,
    #[def("RenownLevelMaxFollowers")]
    pub renown_level_max_followers: Vec<i32>,
    #[def("RenownLevelMaxBoastingCrowd")]
    pub renown_level_max_boasting_crowd: Vec<i32>,
    #[def("MaxMorality")]
    pub max_morality: i32,
    #[def("MaxMoralityAsChild")]
    pub max_morality_as_child: i32,
    #[def("MaxExtraMaxHitPoints")]
    pub max_extra_max_hit_points: f32,
    #[def("MaxExtraMaxStaminaPoints")]
    pub max_extra_max_stamina_points: i32,
    #[def("MoralityChangeOnTheftFromHome")]
    pub morality_change_on_theft_from_home: i32,
    #[def("MoralityChangeOnTheftFromShop")]
    pub morality_change_on_theft_from_shop: i32,
    #[def("MoralityChangeOnVandalism")]
    pub morality_change_on_vandalism: i32,
    #[def("MoralityChangeOnPicklock")]
    pub morality_change_on_picklock: i32,
    #[def("TradeSkillForGuileLevel")]
    pub trade_skill_for_guile_level: Vec<f32>,
    #[def("StatLevels")]
    pub stat_levels: Vec<AbilityTrackDef>,
    #[def("MaxHitPointsPerHealthLevel")]
    pub max_hit_points_per_health_level: Vec<f32>,
    #[def("MaxStaminaPointsPerMagicPowerLevel")]
    pub max_stamina_points_per_magic_power_level: Vec<i32>,
    #[def("SoundDistanceMultiplierPerStealthLevel")]
    pub sound_distance_multiplier_per_stealth_level: Vec<f32>,
    #[def("VisibilityMultiplierPerStealthLevel")]
    pub visibility_multiplier_per_stealth_level: Vec<f32>,
}
