use crate::DefStruct;
use crate::def::{
    enums::{IdleStateGroup, WeaponClass},
    wire::{DefString, VecMap},
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MeleeCombatAbilityDef {
    #[def("FlourishAnimName")]
    pub flourish_anim_name: DefString,
    #[def("FlourishResponseAnimName")]
    pub flourish_response_anim_name: DefString,
    #[def("DamageMultiplier", default = 1.0)]
    pub damage_multiplier: f32,
    #[def("Flourish")]
    pub flourish: bool,
    #[def("BlockCounter")]
    pub block_counter: bool,
    #[def("AbilityButtonGraphicOverride")]
    pub ability_button_graphic_override: i32,
    #[def("ValidWeaponClasses")]
    pub valid_weapon_classes: Vec<WeaponClass>,
    #[def("Decapitate")]
    pub decapitate: bool,
    #[def("KnockDownEffects", default = -1)]
    pub knock_down_effects: i32,
    #[def("EvadeAllHits")]
    pub evade_all_hits: bool,
    #[def("CauseRecoil")]
    pub cause_recoil: bool,
    #[def("StatsNeeded")]
    pub stats_needed: VecMap<i32, IdleStateGroup>,
    #[def("ComboMultiplierLowerBound", default = -1)]
    pub combo_multiplier_lower_bound: i32,
    #[def("ComboMultiplierUpperBound", default = -1)]
    pub combo_multiplier_upper_bound: i32,
}
