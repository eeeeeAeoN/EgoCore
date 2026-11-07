use crate::DefStruct;
use crate::def::{ShopItemDef, enums::Opinion, wire::DefIndex, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ShopDef {
    #[def("Name")]
    pub name: DefIndex,
    #[def("InitialHawkingPhrase")]
    pub initial_hawking_phrase: DefIndex,
    #[def("ReplyToRequestPhrase")]
    pub reply_to_request_phrase: DefIndex,
    #[def("CustomerFollowPhrase")]
    pub customer_follow_phrase: DefIndex,
    #[def("ThankingPhrase")]
    pub thanking_phrase: DefIndex,
    #[def("ShopkeeperMeshType")]
    pub shopkeeper_mesh_type: String,
    #[def("DefaultStock")]
    pub default_stock: Vec<ShopItemDef>,
    #[def("StockCategoryDefaults")]
    pub stock_category_defaults: Vec<ShopItemDef>,
    #[def("BuyPriceMultiplier", default = 1.0)]
    pub buy_price_multiplier: f32,
    #[def("SellPriceMultiplier", default = 1.0)]
    pub sell_price_multiplier: f32,
    #[def("AttitudeBuyPriceMultiplierDefault", default = 1.0)]
    pub attitude_buy_price_multiplier_default: f32,
    #[def("AttitudeBuyPriceMultiplier")]
    pub attitude_buy_price_multiplier: VecMap<Opinion, f32>,
    #[def("AttitudeSellPriceMultiplierDefault", default = 1.0)]
    pub attitude_sell_price_multiplier_default: f32,
    #[def("AttitudeSellPriceMultiplier")]
    pub attitude_sell_price_multiplier: VecMap<Opinion, f32>,
    #[def("MaxStockChangePerSell")]
    pub max_stock_change_per_sell: f32,
    #[def("MaxStockChangePerBuy")]
    pub max_stock_change_per_buy: f32,
    #[def("MaxStockRevertPerDay", default = 1.0)]
    pub max_stock_revert_per_day: f32,
    #[def("PriceReductionFractionPerItem")]
    pub price_reduction_fraction_per_item: f32,
    #[def("MaxPriceReductionFractionDueToStockCount", default = 0.6)]
    pub max_price_reduction_fraction_due_to_stock_count: f32,
    #[def("IsStockDisplayPermanent", default = true)]
    pub is_stock_display_permanent: bool,
    #[def("PreferredShopkeeper")]
    pub preferred_shopkeeper: DefIndex,
    #[def("IsTattooShop")]
    pub is_tattoo_shop: bool,
    #[def("IsBarberShop")]
    pub is_barber_shop: bool,
    #[def("IsTitleShop")]
    pub is_title_shop: bool,
    #[def("MinPriceMultiplierForItemToBeWanted", default = 1.1)]
    pub min_price_multiplier_for_item_to_be_wanted: f32,
    #[def("AlwaysOpen")]
    pub always_open: bool,
    #[def("MaxPriceToDisplay")]
    pub max_price_to_display: f32,
    #[def("MinTimeToSteal")]
    pub min_time_to_steal: f32,
    #[def("MaxTimeToSteal")]
    pub max_time_to_steal: f32,
    #[def("MinGoldForDonation")]
    pub min_gold_for_donation: i32,
    #[def("MaxGoldForDonation")]
    pub max_gold_for_donation: i32,
    #[def("MaxMoralityForDonation")]
    pub max_morality_for_donation: f32,
    #[def("LogarithmicPower")]
    pub logarithmic_power: f32,
    #[def("NumDaysToKeepSpecialItem")]
    pub num_days_to_keep_special_item: i32,
    #[def("MinAmountBeforeSpecialItem")]
    pub min_amount_before_special_item: i32,
    #[def("ProbabilityOfSpecialItemPerDay")]
    pub probability_of_special_item_per_day: f32,
    #[def("SpecialItemPriceMult")]
    pub special_item_price_mult: f32,
}
