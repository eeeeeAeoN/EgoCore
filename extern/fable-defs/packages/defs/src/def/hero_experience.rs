use std::collections::BTreeMap;

use crate::DefStruct;
use crate::def::{
    enums::DamageAttribute,
    values::{AttackHistoryCombo, ComboMultiplierData, HeroStatIncreaseData},
    wire::{DefString, VecMap},
};
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroExperienceDef {
    #[def("ExperienceLevelThresholds")]
    pub experience_level_thresholds: Vec<i32>,
    #[def("HeroLevelTitles")]
    pub hero_level_titles: Vec<DefIndex>,
    #[def("FallbackMultipliers")]
    pub fallback_multipliers: Vec<i32>,
    #[def("SoundIntensityThresholds")]
    pub sound_intensity_thresholds: VecMap<i32, DefString>,
    #[def("ComboMultiplierMap")]
    pub combo_multiplier_map: VecMap<i32, ComboMultiplierData>,
    #[def("ExperienceMultiplierPerUnitComboMultiplierParam")]
    pub experience_multiplier_per_unit_combo_multiplier_param: f32,
    #[def("ExperienceMultiplierRegionChangeMultiplier")]
    pub experience_multiplier_region_change_multiplier: f32,
    #[def("StatIncreasePerHitType")]
    pub stat_increase_per_hit_type: BTreeMap<DamageAttribute, HeroStatIncreaseData>,
    #[def("ComboMultiplierIncrease")]
    pub combo_multiplier_increase: VecMap<i32, DefString>,
    #[def("ComboMultiplierLock")]
    pub combo_multiplier_lock: VecMap<i32, DefString>,
    #[def("ComboMultiplierLost")]
    pub combo_multiplier_lost: VecMap<i32, DefString>,
    #[def("ComboMultiplierRunningHitsIncrease")]
    pub combo_multiplier_running_hits_increase: VecMap<i32, DefString>,
    #[def("StatIncreasePerHitType")]
    pub stat_increase_per_hit_type2: BTreeMap<DamageAttribute, HeroStatIncreaseData>,
    #[def("AttackHistoryCombos")]
    pub attack_history_combos: Vec<AttackHistoryCombo>,
}
