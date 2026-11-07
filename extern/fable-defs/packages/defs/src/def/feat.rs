use crate::DefStruct;
use crate::def::{enums::FeatAttackType, wire::DefString};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FeatDef {
    #[def("FeatName")]
    pub feat_name: DefString,
    #[def("Verb")]
    pub verb: DefString,
    #[def("TimeLimit")]
    pub time_limit: f32,
    // TargetNumber/GoldReward/XPReward/KN_CreatureType are `Transfer<long>` in
    // the decomp (script_def.cpp) — plain i32 counts/flags, NOT def references.
    // KN_CreatureType holds a header flag (e.g. CREATURE_GROUP_WASP = 1<<8), a
    // symbol id, not a def index.
    #[def("TargetNumber")]
    pub target_number: i32,
    #[def("GoldReward")]
    pub gold_reward: i32,
    #[def("XPReward")]
    pub xp_reward: i32,
    #[def("ItemReward")]
    pub item_reward: DefString,
    #[def("NoBlocking")]
    pub no_blocking: bool,
    #[def("KN_AttackType")]
    pub kn_attack_type: FeatAttackType,
    #[def("KN_Perfect")]
    pub kn_perfect: bool,
    #[def("KN_CreatureType")]
    pub kn_creature_type: i32,
    #[def("GF_FromRegion")]
    pub gf_from_region: DefString,
    #[def("GF_ToRegion")]
    pub gf_to_region: DefString,
    #[def("GF_NoTeleporting")]
    pub gf_no_teleporting: bool,
    #[def("CO_ItemName")]
    pub co_item_name: DefString,
}
