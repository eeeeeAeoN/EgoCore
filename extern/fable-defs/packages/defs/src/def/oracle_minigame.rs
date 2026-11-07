use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OracleMinigameDef {
    #[def("WaitBetweenRoundsSeconds")]
    pub wait_between_rounds_seconds: f32,
    #[def("PauseBetweenOutputSeconds")]
    pub pause_between_output_seconds: f32,
    #[def("PauseBetweenOutputsLevelMultiplier")]
    pub pause_between_outputs_level_multiplier: f32,
    #[def("MaxLevel")]
    pub max_level: i32,
    #[def("TintRed")]
    pub tint_red: u32,
    #[def("TintGreen")]
    pub tint_green: u32,
    #[def("TintBlue")]
    pub tint_blue: u32,
    #[def("TextWin")]
    pub text_win: u32,
    #[def("TextLose")]
    pub text_lose: u32,
    #[def("TextWatch")]
    pub text_watch: u32,
    #[def("TextRepeat")]
    pub text_repeat: u32,
    #[def("TextOfferGame")]
    pub text_offer_game: u32,
    #[def("TextWonRound")]
    pub text_won_round: u32,
}
