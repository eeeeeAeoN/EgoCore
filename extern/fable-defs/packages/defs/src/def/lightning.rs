use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct LightningDef {
    #[def("Range")]
    pub range: f32,
    #[def("LifeTime")]
    pub life_time: f32,
    #[def("MaxNumberOfBranches")]
    pub max_number_of_branches: i32,
    #[def("NeedLOSToBranch")]
    pub need_los_to_branch: bool,
    #[def("DamagePeriod")]
    pub damage_period: f32,
    #[def("DamagePerCycle")]
    pub damage_per_cycle: f32,
    #[def("NodeDefIndex")]
    pub node_def_index: DefIndex,
}
