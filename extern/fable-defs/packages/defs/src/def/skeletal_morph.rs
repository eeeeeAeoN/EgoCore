use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SkeletalMorphDef {
    #[def("SkeletalMorphs")]
    pub skeletal_morphs: Vec<DefString>,
}
