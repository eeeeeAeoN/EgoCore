use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BonusItemDef {
    #[def("HealthModifier")]
    pub health_modifier: f32,
    #[def("AddsFatness")]
    pub adds_fatness: bool,
    #[def("StaminaModifier")]
    pub stamina_modifier: i32,
    #[def("SecondsToApplyHealthIncreaseOver")]
    pub seconds_to_apply_health_increase_over: f32,
    #[def("WillExperience")]
    pub will_experience: i32,
    #[def("SkillExperience")]
    pub skill_experience: i32,
    #[def("StrengthExperience")]
    pub strength_experience: i32,
    #[def("MoralityChange")]
    pub morality_change: i32,
    #[def("AgreeablenessChange")]
    pub agreeableness_change: f32,
    #[def("AttractivenessChange")]
    pub attractiveness_change: f32,
    #[def("ScarinessChange")]
    pub scariness_change: f32,
    #[def("MaxHealthIncrease")]
    pub max_health_increase: f32,
    #[def("MaxStaminaIncrease")]
    pub max_stamina_increase: i32,
    #[def("ChangeTimeOfDayToDay")]
    pub change_time_of_day_to_day: bool,
    #[def("ChangeTimeOfDayToNight")]
    pub change_time_of_day_to_night: bool,
    #[def("SpeedMultiplier", default = 1.0)]
    pub speed_multiplier: f32,
    #[def("SecondsForSpeedMultiplierToLast")]
    pub seconds_for_speed_multiplier_to_last: f32,
    #[def("ParticleEffect")]
    pub particle_effect: i32,
    #[def("AddsDrunkenness")]
    pub adds_drunkenness: bool,
    #[def("AlcoholicStrength")]
    pub alcoholic_strength: f32,
}
