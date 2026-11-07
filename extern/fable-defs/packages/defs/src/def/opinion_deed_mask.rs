use crate::DefStruct;
use crate::def::{enums::OpinionDeedType, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionDeedMaskDef {
    #[def("DeedEnabledDefault", default = true)]
    pub deed_enabled_default: bool,
    #[def("DeedEnabled")]
    pub deed_enabled: VecMap<OpinionDeedType, bool>,
}
