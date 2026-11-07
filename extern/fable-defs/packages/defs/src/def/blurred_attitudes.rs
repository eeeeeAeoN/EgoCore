use crate::DefStruct;
use crate::enums::OpinionAttitudeType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BlurredAttitudesDef {
    #[def("Attitudes")]
    pub attitudes: Vec<OpinionAttitudeType>,
}
