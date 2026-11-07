use crate::DefStruct;
use crate::enums::{CombatCreatureType, CombatStrikeRecoilStyle};
use crate::wire::DefIndex;
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CombatTypeDef {
    #[def("DefenderCombatRingOccupationNumbers")]
    pub defender_combat_ring_occupation_numbers: Vec<i32>,
    #[def("DefenderWheelOuterRingStartIndex")]
    pub defender_wheel_outer_ring_start_index: i32,
    #[def("DefenderWheelAllowAttackerStrafing")]
    pub defender_wheel_allow_attacker_strafing: bool,
    #[def("CombatSequence")]
    pub combat_sequence: Vec<String>,
    #[def("CombatSequenceDef")]
    pub combat_sequence_def: Vec<DefIndex>,
    #[def("PreferOuterRing")]
    pub prefer_outer_ring: bool,
    #[def("FrontFarLimit")]
    pub front_far_limit: f32,
    #[def("FrontMiddleLimit")]
    pub front_middle_limit: f32,
    #[def("FrontNearLimit")]
    pub front_near_limit: f32,
    #[def("SideLimit")]
    pub side_limit: f32,
    #[def("RearLimit")]
    pub rear_limit: f32,
    #[def("PreferredCombatDistance")]
    pub preferred_combat_distance: f32,
    // `std::map<ECombatCreatureType, long>` — key-sorted (BTreeMap), value
    // is an ATTACK_PATTERN def index (verified against retail:
    // HOBBE_ATTACK_STYLE_BASE stores keys 0,1 → 417,418 =
    // ATTACK_PATTERN_MEDIUM/EASY). The decomp template's `EIdleStateGroup`
    // ValueT is a misattribution; the slot holds a `long` def index.
    #[def("AttackPatterns")]
    pub attack_patterns: BTreeMap<i32, DefIndex>,
    #[def("CombatCreatureType")]
    pub combat_creature_type: CombatCreatureType,
    #[def("AlwaysAttackIfNearest")]
    pub always_attack_if_nearest: bool,
    #[def("KnockdownCausesStun")]
    pub knockdown_causes_stun: bool,
    #[def("RecoilStyle")]
    pub recoil_style: CombatStrikeRecoilStyle,
    #[def("CanBeKnockedDown", default = true)]
    pub can_be_knocked_down: bool,
    #[def("ComboContinueSpeed")]
    pub combo_continue_speed: f32,
    #[def("CombatFOV", default = 36.0)]
    pub combat_fov: f32,
    #[def("StartsAsReadyForMelee", default = true)]
    pub starts_as_ready_for_melee: bool,
}
