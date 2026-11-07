use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BuyableHouseDef {
    #[def("IsBuyable")]
    pub is_buyable: bool,
    #[def("InitiallyEmpty")]
    pub initially_empty: bool,
    #[def("Price")]
    pub price: Vec<i32>,
    #[def("Rent")]
    pub rent: Vec<i32>,
    #[def("DaysBetweenRentPayments", default = 2)]
    pub days_between_rent_payments: i32,
    #[def("IsScripted")]
    pub is_scripted: bool,
    #[def("MaxRentBags", default = 6)]
    pub max_rent_bags: i32,
    #[def("IsBuyableWithoutSignpost")]
    pub is_buyable_without_signpost: bool,
}
