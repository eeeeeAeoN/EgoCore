use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OverheadDisplayDef {
    #[def("DisplayDamage")]
    pub display_damage: bool,
    #[def("DisplayRepair")]
    pub display_repair: bool,
    #[def("DisplayRenown")]
    pub display_renown: bool,
}
