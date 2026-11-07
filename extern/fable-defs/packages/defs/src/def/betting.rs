use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BettingDef {
    #[def("BetX")]
    pub bet_x: f32,
    #[def("BetY")]
    pub bet_y: f32,
    #[def("MoneyX")]
    pub money_x: f32,
    #[def("MoneyY")]
    pub money_y: f32,
    #[def("MinBet")]
    pub min_bet: i32,
    #[def("MaxBet")]
    pub max_bet: i32,
    #[def("BetIncrement")]
    pub bet_increment: i32,
    #[def("Bet")]
    pub bet: u32,
    #[def("PlayersMoney")]
    pub players_money: u32,
    #[def("Question")]
    pub question: u32,
    #[def("Continue")]
    pub continue_: u32,
    #[def("Cancel")]
    pub cancel: u32,
}
