use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct StockItemDef {
    #[def("DefaultPrice")]
    pub default_price: i32,
    #[def("DefaultIsStealable", default = true)]
    pub default_is_stealable: bool,
    #[def("SellersBanter")]
    pub sellers_banter: u32,
    #[def("BuyersPhrase")]
    pub buyers_phrase: u32,
    #[def("CanOnlyBeSoldToPlayer")]
    pub can_only_be_sold_to_player: bool,
    #[def("ChanceOfBecomingAvailable", default = 0.15)]
    pub chance_of_becoming_available: f32,
    #[def("CanBeDisplayedInShop", default = true)]
    pub can_be_displayed_in_shop: bool,
}
