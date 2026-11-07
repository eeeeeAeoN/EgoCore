use crate::DefStruct;
use crate::enums::OpinionDeedType;
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionReactionMaskDef {
    #[def("ReactionEnabledDefault", default = true)]
    pub reaction_enabled_default: bool,
    #[def("ReactionEnabled")]
    pub reaction_enabled: BTreeMap<OpinionDeedType, bool>,
}
