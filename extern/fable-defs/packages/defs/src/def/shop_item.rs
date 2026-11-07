use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ShopItemDef {
    // The object being sold — a def reference, not a numeric type.
    #[def("Type")]
    pub type_: DefIndex,
    #[def("Quantity")]
    pub quantity: f32,
    #[def("PriceMultiplier", default = 1.0)]
    pub price_multiplier: f32,
    #[def("MaxStock")]
    pub max_stock: f32,
    #[def("RestockPeriod")]
    pub restock_period: i32,
    #[def("RestockPeriodRandomness")]
    pub restock_period_randomness: f32,
    #[def("SalesPerDay")]
    pub sales_per_day: f32,
    #[def("SalesPerDayRandomness")]
    pub sales_per_day_randomness: f32,
}
