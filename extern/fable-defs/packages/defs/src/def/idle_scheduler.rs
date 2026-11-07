use crate::DefStruct;
use crate::def::{enums::IdleStateGroup, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct IdleSchedulerDef {
    #[def("Period")]
    pub period: VecMap<IdleStateGroup, i32>,
}
