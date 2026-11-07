use crate::DefStruct;
use crate::values::AnimationSet;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AnimatingObjectDef {
    #[def("Animation")]
    pub animation: AnimationSet,
}
