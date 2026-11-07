use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct InterestingToVillagersDef {
    #[def("Interest")]
    pub interest: f32,
}
