use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct GuardDef {
    #[def("BribeCostWeaponOut")]
    pub bribe_cost_weapon_out: i32,
    #[def("BribeCostTrespass")]
    pub bribe_cost_trespass: i32,
    #[def("BribeCostVandalism")]
    pub bribe_cost_vandalism: i32,
    #[def("BribeCostShoplifter")]
    pub bribe_cost_shoplifter: i32,
    #[def("BribeCostPickPocket")]
    pub bribe_cost_pick_pocket: i32,
    #[def("BribeCostAssault")]
    pub bribe_cost_assault: i32,
    #[def("BribeCostGuardAssault")]
    pub bribe_cost_guard_assault: i32,
    #[def("BribeCostGBH")]
    pub bribe_cost_gbh: i32,
    #[def("BribeCostGuardGBH")]
    pub bribe_cost_guard_gbh: i32,
    #[def("BribeCostMurder")]
    pub bribe_cost_murder: i32,
}
