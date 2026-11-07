use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct RegionScriptDef {
    #[def("RandomVillagerMax")]
    pub random_villager_max: i32,
    #[def("RandomGuardMax")]
    pub random_guard_max: i32,
    #[def("RandomBanditMax")]
    pub random_bandit_max: i32,
    #[def("RegionDangerLevel")]
    pub region_danger_level: i32,
}
