use crate::DefStruct;
use crate::def::{PrizeScoreDef, ShoveHaPennyRegionDef, wire::DefIndex, wire::DefString};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernGameShoveHaPennyDef {
    #[def("Banter")]
    pub banter: u32,
    #[def("Greeting")]
    pub greeting: u32,
    #[def("OptionsInitial")]
    pub options_initial: u32,
    #[def("OptionsSubsequent")]
    pub options_subsequent: u32,
    #[def("Instructions")]
    pub instructions: u32,
    #[def("InstructionsPC")]
    pub instructions_pc: u32,
    #[def("Betting")]
    pub betting: u32,
    #[def("Play")]
    pub play: u32,
    #[def("ReactionWin")]
    pub reaction_win: u32,
    #[def("ReactionLose")]
    pub reaction_lose: u32,
    #[def("ReactionDraw")]
    pub reaction_draw: u32,
    #[def("ReactionWinNewBestScore")]
    pub reaction_win_new_best_score: u32,
    #[def("FarewellInitial")]
    pub farewell_initial: u32,
    #[def("FarewellSubsequent")]
    pub farewell_subsequent: u32,
    #[def("WinRoundPhrase")]
    pub win_round_phrase: u32,
    #[def("OutOfTimePhrase")]
    pub out_of_time_phrase: u32,
    #[def("NoMoney")]
    pub no_money: u32,
    #[def("GreetingForward")]
    pub greeting_forward: u32,
    #[def("OptionsForward")]
    pub options_forward: u32,
    #[def("OptionsBack")]
    pub options_back: u32,
    #[def("OptionsAlternative")]
    pub options_alternative: u32,
    #[def("InstructionsBack")]
    pub instructions_back: u32,
    #[def("BettingForward")]
    pub betting_forward: u32,
    #[def("BettingBack")]
    pub betting_back: u32,
    #[def("ReactionForward")]
    pub reaction_forward: u32,
    #[def("FarewellForward")]
    pub farewell_forward: u32,
    #[def("CameraName")]
    pub camera_name: DefString,
    #[def("BoxGraphicL")]
    pub box_graphic_l: u32,
    #[def("BoxGraphicC")]
    pub box_graphic_c: u32,
    #[def("BoxGraphicR")]
    pub box_graphic_r: u32,
    #[def("ClickToContinue")]
    pub click_to_continue: u32,
    #[def("WinPhrase")]
    pub win_phrase: u32,
    #[def("LosePhrase")]
    pub lose_phrase: u32,
    #[def("DrawPhrase")]
    pub draw_phrase: u32,
    #[def("NewGame")]
    pub new_game: u32,
    #[def("BestScore")]
    pub best_score: u32,
    #[def("CurrentScore")]
    pub current_score: u32,
    #[def("RequiredScore")]
    pub required_score: u32,
    #[def("AdditionalInfo")]
    pub additional_info: u32,
    #[def("BlackjackBusted")]
    pub blackjack_busted: u32,
    #[def("BlackjackSplit")]
    pub blackjack_split: u32,
    #[def("BlackjackDouble")]
    pub blackjack_double: u32,
    #[def("BlackjackHit")]
    pub blackjack_hit: u32,
    #[def("BlackjackStand")]
    pub blackjack_stand: u32,
    #[def("BlackjackDealerTakesCard")]
    pub blackjack_dealer_takes_card: u32,
    #[def("BlackjackSplitGUI")]
    pub blackjack_split_gui: u32,
    #[def("BlackjackDoubleGUI")]
    pub blackjack_double_gui: u32,
    #[def("BlackjackHitGUI")]
    pub blackjack_hit_gui: u32,
    #[def("BlackjackStandGUI")]
    pub blackjack_stand_gui: u32,
    #[def("Bet")]
    pub bet: u32,
    #[def("PlayersMoney")]
    pub players_money: u32,
    #[def("TotalWinnings")]
    pub total_winnings: u32,
    #[def("Continue")]
    pub continue_: u32,
    #[def("Quit")]
    pub quit: u32,
    #[def("Yes")]
    pub yes: u32,
    #[def("No")]
    pub no: u32,
    #[def("PrizeGiven")]
    pub prize_given: u32,
    #[def("MoneyBagGraphic")]
    pub money_bag_graphic: u32,
    #[def("MinBet")]
    pub min_bet: i32,
    #[def("MaxBet")]
    pub max_bet: i32,
    #[def("BetIncrement")]
    pub bet_increment: i32,
    #[def("ScoreFont")]
    pub score_font: DefString,
    #[def("TargetFont")]
    pub target_font: DefString,
    #[def("StatsFont")]
    pub stats_font: DefString,
    #[def("ScoreX")]
    pub score_x: f32,
    #[def("ScoreY")]
    pub score_y: f32,
    #[def("TargetX")]
    pub target_x: f32,
    #[def("TargetY")]
    pub target_y: f32,
    #[def("BestX")]
    pub best_x: f32,
    #[def("BestY")]
    pub best_y: f32,
    #[def("AdditionalX")]
    pub additional_x: f32,
    #[def("AdditionalY")]
    pub additional_y: f32,
    #[def("BetX")]
    pub bet_x: f32,
    #[def("BetY")]
    pub bet_y: f32,
    #[def("MoneyX")]
    pub money_x: f32,
    #[def("MoneyY")]
    pub money_y: f32,
    #[def("WinningsX")]
    pub winnings_x: f32,
    #[def("WinningsY")]
    pub winnings_y: f32,
    #[def("MainBetX")]
    pub main_bet_x: f32,
    #[def("MainBetY")]
    pub main_bet_y: f32,
    #[def("MainMoneyX")]
    pub main_money_x: f32,
    #[def("MainMoneyY")]
    pub main_money_y: f32,
    #[def("BestScoreHigh")]
    pub best_score_high: bool,
    #[def("PrizeScores")]
    pub prize_scores: Vec<PrizeScoreDef>,
    #[def("Prize")]
    pub prize: DefIndex,
    #[def("PrizeRenown")]
    pub prize_renown: i32,
    #[def("MainGameScoreBoxX")]
    pub main_game_score_box_x: f32,
    #[def("MainGameScoreBoxY")]
    pub main_game_score_box_y: f32,
    #[def("MainGameScoreBoxWidthXbox")]
    pub main_game_score_box_width_xbox: f32,
    #[def("MainGameScoreBoxWidthPC")]
    pub main_game_score_box_width_pc: f32,
    #[def("MainGameScoreBoxHeight")]
    pub main_game_score_box_height: f32,
    #[def("DisplayErrata")]
    pub display_errata: bool,
    #[def("PointerPhaseSpeed")]
    pub pointer_phase_speed: f32,
    #[def("Regions")]
    pub regions: Vec<ShoveHaPennyRegionDef>,
    #[def("BoardDefIndex")]
    pub board_def_index: DefIndex,
    #[def("ArrowDefIndex")]
    pub arrow_def_index: DefIndex,
    #[def("ImpulseScale")]
    pub impulse_scale: f32,
    #[def("TargetDistanceScale")]
    pub target_distance_scale: f32,
    #[def("JoystickScaleXboxX")]
    pub joystick_scale_xbox_x: f32,
    #[def("JoystickScaleXboxY")]
    pub joystick_scale_xbox_y: f32,
    #[def("JoystickScalePCX")]
    pub joystick_scale_pcx: f32,
    #[def("JoystickScalePCY")]
    pub joystick_scale_pcy: f32,
    #[def("JoystickTranslateY")]
    pub joystick_translate_y: f32,
    #[def("JoystickMagBelow")]
    pub joystick_mag_below: f32,
    #[def("AverageJoystickMagAbove")]
    pub average_joystick_mag_above: f32,
    #[def("MaxPower")]
    pub max_power: f32,
    #[def("AverageDrift")]
    pub average_drift: f32,
    #[def("PositionCoinSpeedXbox")]
    pub position_coin_speed_xbox: f32,
    #[def("PositionCoinSpeedPC")]
    pub position_coin_speed_pc: f32,
    #[def("PositionCoinMinimum")]
    pub position_coin_minimum: f32,
    #[def("PositionCoinMaximum")]
    pub position_coin_maximum: f32,
    #[def("PositionCoinDepth")]
    pub position_coin_depth: f32,
    #[def("RestSpeed")]
    pub rest_speed: f32,
    #[def("Rounds")]
    pub rounds: DefIndex,
}
