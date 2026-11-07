use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DrunkennessDef {
    #[def("DrunkennessThresholdMult")]
    pub drunkenness_threshold_mult: f32,
}
