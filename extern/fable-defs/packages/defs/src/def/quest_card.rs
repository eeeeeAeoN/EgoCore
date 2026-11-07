use crate::DefStruct;
use crate::def::{enums::IdleStateGroup, wire::DefString, wire::VecMap};
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct QuestCardDef {
    #[def("QuestName")]
    pub quest_name: i32,
    #[def("QuestSummary")]
    pub quest_summary: i32,
    #[def("QuestObjective")]
    pub quest_objective: i32,
    #[def("SuccessSummary")]
    pub success_summary: i32,
    #[def("RegionName")]
    pub region_name: DefString,
    #[def("TeleporterRegionName")]
    pub teleporter_region_name: DefString,
    #[def("InventoryCategory")]
    pub inventory_category: DefIndex,
    #[def("RenownReward")]
    pub renown_reward: i32,
    #[def("GoldReward")]
    pub gold_reward: i32,
    #[def("RewardObjects")]
    pub reward_objects: Vec<DefIndex>,
    #[def("IsCoreQuest")]
    pub is_core_quest: bool,
    #[def("IsVignette")]
    pub is_vignette: bool,
    #[def("IsExclusive")]
    pub is_exclusive: bool,
    #[def("MakeVignetteRouteAppearOnMinimap")]
    pub make_vignette_route_appear_on_minimap: bool,
    #[def("Prerequisites")]
    pub prerequisites: VecMap<i32, IdleStateGroup>,
    #[def("NumBoasts", default = 1)]
    pub num_boasts: i32,
    #[def("CanPlayerCancel", default = true)]
    pub can_player_cancel: bool,
    #[def("QuestEpilogue")]
    pub quest_epilogue: i32,
}
