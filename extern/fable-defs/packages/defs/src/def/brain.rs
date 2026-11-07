use crate::DefStruct;
use crate::values::{BrainBehaviour, BrainUpdateZone};
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BrainDef {
    #[def("UpdateZones")]
    pub update_zones: Vec<BrainUpdateZone>,
    #[def("Behaviours")]
    pub behaviours: Vec<BrainBehaviour>,
    #[def("DisabledBehaviours")]
    pub disabled_behaviours: Vec<DefString>,
}
