use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ContainerRewardHeroDef {
    #[def("ObjectFamilies")]
    pub object_families: Vec<DefIndex>,
}
