use crate::DefStruct;
use crate::def::{
    values::{MiniMapGraphics, RGBColour, Vector2D, Vector3D},
    wire::VecMap,
    {AbilityButtonGuiDef, QuickAccessItemButtonGuiDef},
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PlayerGuiDef {
    #[def("InventoryNoMovementSound")]
    pub inventory_no_movement_sound: i32,
    #[def("SecondsToFadeWholeGui")]
    pub seconds_to_fade_whole_gui: f32,
    #[def("AlphaValueToFadeWholeGui")]
    pub alpha_value_to_fade_whole_gui: f32,
    #[def("AmbientColour")]
    pub ambient_colour: RGBColour,
    #[def("DiffuseColour")]
    pub diffuse_colour: RGBColour,
    #[def("BacklightColour")]
    pub backlight_colour: RGBColour,
    #[def("DiffuseVector")]
    pub diffuse_vector: Vector3D,
    #[def("GuildSealPos")]
    pub guild_seal_pos: Vector2D,
    #[def("GuildSealFlashCycles")]
    pub guild_seal_flash_cycles: i32,
    #[def("GuildSealFlashCycleSeconds")]
    pub guild_seal_flash_cycle_seconds: f32,
    #[def("GuildSealReminderTimeSeconds")]
    pub guild_seal_reminder_time_seconds: f32,
    #[def("HealthBarPos")]
    pub health_bar_pos: Vector2D,
    #[def("HealthBarLeftGraphic")]
    pub health_bar_left_graphic: i32,
    #[def("HealthBarMiddleOffset")]
    pub health_bar_middle_offset: Vector2D,
    #[def("HealthBarMiddleLength")]
    pub health_bar_middle_length: f32,
    #[def("HealthBarMiddleGraphic")]
    pub health_bar_middle_graphic: i32,
    #[def("HealthBarRightGraphic")]
    pub health_bar_right_graphic: i32,
    #[def("HealthBarHealthOffset")]
    pub health_bar_health_offset: Vector2D,
    #[def("HealthBarHealthGraphic")]
    pub health_bar_health_graphic: i32,
    #[def("HealthBarHealthEvilGraphic")]
    pub health_bar_health_evil_graphic: i32,
    #[def("HealthBarHealthGoodGraphic")]
    pub health_bar_health_good_graphic: i32,
    #[def("HealthBarHealthEffectOffset")]
    pub health_bar_health_effect_offset: Vector2D,
    #[def("HealthBarHealthIncreaseEffectOffset")]
    pub health_bar_health_increase_effect_offset: Vector2D,
    #[def("HealthBarHealthDefaultEffect")]
    pub health_bar_health_default_effect: i32,
    #[def("HealthBarHealthIncreaseEffect")]
    pub health_bar_health_increase_effect: i32,
    #[def("HealthBarHealthLevelUpEffect")]
    pub health_bar_health_level_up_effect: i32,
    #[def("HealthBarHealthAdrenalineEffect")]
    pub health_bar_health_adrenaline_effect: i32,
    #[def("HealthBarHealthBerserkEffect")]
    pub health_bar_health_berserk_effect: i32,
    #[def("HealthBarHealthLowHealthEffect")]
    pub health_bar_health_low_health_effect: i32,
    #[def("HealthBarHealthLowPercentage")]
    pub health_bar_health_low_percentage: f32,
    #[def("HealthBarHealthInnerOffset")]
    pub health_bar_health_inner_offset: Vector2D,
    #[def("HealthBarHealthInnerGraphic")]
    pub health_bar_health_inner_graphic: i32,
    #[def("HealthBarHealthEndGraphic")]
    pub health_bar_health_end_graphic: i32,
    #[def("HealthBarResurrectionOffset")]
    pub health_bar_resurrection_offset: Vector2D,
    #[def("HealthBarResurrectionColour")]
    pub health_bar_resurrection_colour: RGBColour,
    #[def("HealthBarStaminaBackOffset")]
    pub health_bar_stamina_back_offset: Vector2D,
    #[def("HealthBarStaminaOffset")]
    pub health_bar_stamina_offset: Vector2D,
    #[def("HealthBarStaminaGraphic")]
    pub health_bar_stamina_graphic: i32,
    #[def("HealthBarStaminaInnerOffset")]
    pub health_bar_stamina_inner_offset: Vector2D,
    #[def("HealthBarStaminaInnerGraphic")]
    pub health_bar_stamina_inner_graphic: i32,
    #[def("HealthBarStaminaEndGraphic")]
    pub health_bar_stamina_end_graphic: i32,
    #[def("HealthBarStaminaEffectOffset")]
    pub health_bar_stamina_effect_offset: Vector2D,
    #[def("HealthBarStaminaIncreaseEffectOffset")]
    pub health_bar_stamina_increase_effect_offset: Vector2D,
    #[def("HealthBarStaminaIncreaseEffectWidth")]
    pub health_bar_stamina_increase_effect_width: f32,
    #[def("HealthBarStaminaIncreaseEffectSegmentWidth")]
    pub health_bar_stamina_increase_effect_segment_width: f32,
    #[def("HealthBarStaminaDefaultEffect")]
    pub health_bar_stamina_default_effect: i32,
    #[def("HealthBarStaminaIncreaseEffect")]
    pub health_bar_stamina_increase_effect: i32,
    #[def("HealthBarStaminaIncreaseSegmentEffect")]
    pub health_bar_stamina_increase_segment_effect: i32,
    #[def("StealthEyePos")]
    pub stealth_eye_pos: Vector2D,
    #[def("StealthEyeFriendGraphic")]
    pub stealth_eye_friend_graphic: i32,
    #[def("StealthEyeNeutralGraphic")]
    pub stealth_eye_neutral_graphic: i32,
    #[def("StealthEyeEnemyGraphic")]
    pub stealth_eye_enemy_graphic: i32,
    #[def("StealthEyeQuestionMarkGraphic")]
    pub stealth_eye_question_mark_graphic: i32,
    #[def("StealthEyeGraphicFrames")]
    pub stealth_eye_graphic_frames: Vec<i32>,
    #[def("StealthEyeTextOffset")]
    pub stealth_eye_text_offset: Vector2D,
    #[def("StealthEyeSecondsToOpen")]
    pub stealth_eye_seconds_to_open: f32,
    #[def("StealthEyeSecondsToThrob")]
    pub stealth_eye_seconds_to_throb: f32,
    #[def("StealthEyeThrobScale")]
    pub stealth_eye_throb_scale: f32,
    #[def("DPadRingsGraphic")]
    pub d_pad_rings_graphic: i32,
    #[def("DPadRingsPos")]
    pub d_pad_rings_pos: Vector2D,
    #[def("DPadRingsTopGraphic")]
    pub d_pad_rings_top_graphic: i32,
    #[def("DPadRingsTopOffset")]
    pub d_pad_rings_top_offset: Vector2D,
    #[def("DPadRingsExtendGraphic")]
    pub d_pad_rings_extend_graphic: i32,
    #[def("DPadRingsChargeGraphics")]
    pub d_pad_rings_charge_graphics: Vec<i32>,
    #[def("DPadRingsChargeOffsets")]
    pub d_pad_rings_charge_offsets: Vec<Vector2D>,
    #[def("DPadRingsChargeAngles")]
    pub d_pad_rings_charge_angles: Vec<f32>,
    #[def("DPadRingsFavouriteGraphic")]
    pub d_pad_rings_favourite_graphic: i32,
    #[def("DPadRingsTopFavouriteGraphic")]
    pub d_pad_rings_top_favourite_graphic: i32,
    #[def("DPadRingsTopAggressiveGraphic")]
    pub d_pad_rings_top_aggressive_graphic: i32,
    #[def("DPadGuildSealLevelUpGraphic")]
    pub d_pad_guild_seal_level_up_graphic: i32,
    #[def("DPadGuildSealTooltipOffset")]
    pub d_pad_guild_seal_tooltip_offset: Vector2D,
    #[def("DPadMiddleOffset")]
    pub d_pad_middle_offset: Vector2D,
    #[def("DPadArrowUpGraphic")]
    pub d_pad_arrow_up_graphic: i32,
    #[def("DPadArrowUpDepGraphic")]
    pub d_pad_arrow_up_dep_graphic: i32,
    #[def("DPadArrowRightGraphic")]
    pub d_pad_arrow_right_graphic: i32,
    #[def("DPadArrowRightDepGraphic")]
    pub d_pad_arrow_right_dep_graphic: i32,
    #[def("DPadArrowDownGraphic")]
    pub d_pad_arrow_down_graphic: i32,
    #[def("DPadArrowDownDepGraphic")]
    pub d_pad_arrow_down_dep_graphic: i32,
    #[def("DPadArrowLeftGraphic")]
    pub d_pad_arrow_left_graphic: i32,
    #[def("DPadArrowLeftDepGraphic")]
    pub d_pad_arrow_left_dep_graphic: i32,
    #[def("DPadGuildSealRecallGraphic")]
    pub d_pad_guild_seal_recall_graphic: i32,
    #[def("GuildSealLevelUpGraphic")]
    pub guild_seal_level_up_graphic: i32,
    #[def("GuildSealQuestGraphic")]
    pub guild_seal_quest_graphic: i32,
    #[def("GuildSealRecallGraphic")]
    pub guild_seal_recall_graphic: i32,
    #[def("GuildSealLevelUpOffset")]
    pub guild_seal_level_up_offset: Vector2D,
    #[def("GuildSealMainLevelUpOffset")]
    pub guild_seal_main_level_up_offset: Vector2D,
    #[def("ABXYRingsGraphic")]
    pub abxy_rings_graphic: i32,
    #[def("ABXYRingsPos")]
    pub abxy_rings_pos: Vector2D,
    #[def("ABXYRingsAlpha")]
    pub abxy_rings_alpha: i32,
    #[def("ABXYRingsTopGraphic")]
    pub abxy_rings_top_graphic: i32,
    #[def("ABXYRingsTopOffset")]
    pub abxy_rings_top_offset: Vector2D,
    #[def("ABXYMiddleGraphic")]
    pub abxy_middle_graphic: i32,
    #[def("ABXYMiddleOffset")]
    pub abxy_middle_offset: Vector2D,
    #[def("ABXYMiddleUpGraphic")]
    pub abxy_middle_up_graphic: i32,
    #[def("ABXYMiddleRightGraphic")]
    pub abxy_middle_right_graphic: i32,
    #[def("ABXYMiddleDownGraphic")]
    pub abxy_middle_down_graphic: i32,
    #[def("ABXYMiddleLeftGraphic")]
    pub abxy_middle_left_graphic: i32,
    #[def("ABXYRingsFavouriteGraphic")]
    pub abxy_rings_favourite_graphic: i32,
    #[def("ABXYRingsTopFavouriteGraphic")]
    pub abxy_rings_top_favourite_graphic: i32,
    #[def("WhiteButtonGraphic")]
    pub white_button_graphic: i32,
    #[def("WhiteButtonPos")]
    pub white_button_pos: Vector2D,
    #[def("WhiteButtonIconOffset")]
    pub white_button_icon_offset: Vector2D,
    #[def("WhiteButtonTooltipOffset")]
    pub white_button_tooltip_offset: Vector2D,
    #[def("BlackButtonGraphic")]
    pub black_button_graphic: i32,
    #[def("BlackButtonPos")]
    pub black_button_pos: Vector2D,
    #[def("BlackButtonIconOffset")]
    pub black_button_icon_offset: Vector2D,
    #[def("BlackButtonTooltipOffset")]
    pub black_button_tooltip_offset: Vector2D,
    #[def("TextBoxTLGraphic")]
    pub text_box_tl_graphic: i32,
    #[def("TextBoxTMGraphic")]
    pub text_box_tm_graphic: i32,
    #[def("TextBoxTRGraphic")]
    pub text_box_tr_graphic: i32,
    #[def("TextBoxMLGraphic")]
    pub text_box_ml_graphic: i32,
    #[def("TextBoxMRGraphic")]
    pub text_box_mr_graphic: i32,
    #[def("TextBoxBLGraphic")]
    pub text_box_bl_graphic: i32,
    #[def("TextBoxBMGraphic")]
    pub text_box_bm_graphic: i32,
    #[def("TextBoxBRGraphic")]
    pub text_box_br_graphic: i32,
    #[def("TextBoxBackGraphic")]
    pub text_box_back_graphic: i32,
    #[def("TextBoxMaxWidth")]
    pub text_box_max_width: f32,
    #[def("TextBoxMinWidth")]
    pub text_box_min_width: f32,
    #[def("TextBoxMaxHeight")]
    pub text_box_max_height: f32,
    #[def("TextBoxMinHeight")]
    pub text_box_min_height: f32,
    #[def("TextBoxBorderWidth")]
    pub text_box_border_width: f32,
    #[def("TextBoxBorderHeight")]
    pub text_box_border_height: f32,
    #[def("TextBoxCutsceneSecondsBeforeSkip")]
    pub text_box_cutscene_seconds_before_skip: f32,
    #[def("StatBoxMaxWidth")]
    pub stat_box_max_width: f32,
    #[def("StatBoxMiddleGraphic")]
    pub stat_box_middle_graphic: i32,
    #[def("StatBoxLeftOffset")]
    pub stat_box_left_offset: Vector2D,
    #[def("StatBoxRightOffset")]
    pub stat_box_right_offset: Vector2D,
    #[def("StatBoxStartGraphic")]
    pub stat_box_start_graphic: i32,
    #[def("StatBoxEndGraphic")]
    pub stat_box_end_graphic: i32,
    #[def("StatBoxGoldGraphic")]
    pub stat_box_gold_graphic: i32,
    #[def("StatBoxGoldGainEffect")]
    pub stat_box_gold_gain_effect: i32,
    #[def("StatBoxGoldLossEffect")]
    pub stat_box_gold_loss_effect: i32,
    #[def("ClockPos")]
    pub clock_pos: Vector2D,
    #[def("ClockFaceGraphic")]
    pub clock_face_graphic: i32,
    #[def("ClockHandGraphic")]
    pub clock_hand_graphic: i32,
    #[def("ClockRingPos")]
    pub clock_ring_pos: Vector2D,
    #[def("ClockRingGraphic")]
    pub clock_ring_graphic: i32,
    #[def("ModePos")]
    pub mode_pos: Vector2D,
    #[def("ModeOffset")]
    pub mode_offset: Vector2D,
    #[def("ModeInnerOffset")]
    pub mode_inner_offset: Vector2D,
    #[def("ModeBorderGraphic")]
    pub mode_border_graphic: i32,
    #[def("ModeLockOffGraphic")]
    pub mode_lock_off_graphic: i32,
    #[def("ModeLockOnGraphic")]
    pub mode_lock_on_graphic: i32,
    #[def("ModeSneakOffGraphic")]
    pub mode_sneak_off_graphic: i32,
    #[def("ModeSneakOnGraphic")]
    pub mode_sneak_on_graphic: i32,
    #[def("ModeSafetyOffGraphic")]
    pub mode_safety_off_graphic: i32,
    #[def("ModeSafetyOnGraphic")]
    pub mode_safety_on_graphic: i32,
    #[def("ModeTextOffset")]
    pub mode_text_offset: Vector2D,
    #[def("CombatPos")]
    pub combat_pos: Vector2D,
    #[def("CombatLockedOffset")]
    pub combat_locked_offset: Vector2D,
    #[def("CombatLockedGraphics")]
    pub combat_locked_graphics: Vec<i32>,
    #[def("CombatRingOffset")]
    pub combat_ring_offset: Vector2D,
    #[def("CombatRingGraphic")]
    pub combat_ring_graphic: i32,
    #[def("CombatCurrentOffset")]
    pub combat_current_offset: Vector2D,
    #[def("CombatCurrentTextOffset")]
    pub combat_current_text_offset: Vector2D,
    #[def("CombatCountOffsets")]
    pub combat_count_offsets: Vec<Vector2D>,
    #[def("CombatCountEmptyGraphic")]
    pub combat_count_empty_graphic: i32,
    #[def("CombatCountGraphics")]
    pub combat_count_graphics: Vec<i32>,
    #[def("CombatRingGraphics")]
    pub combat_ring_graphics: Vec<i32>,
    #[def("CombatRingBlackGraphic")]
    pub combat_ring_black_graphic: i32,
    #[def("CombatRingWhiteGraphic")]
    pub combat_ring_white_graphic: i32,
    #[def("CombatCrossGraphic")]
    pub combat_cross_graphic: i32,
    #[def("CombatCrossOffset")]
    pub combat_cross_offset: Vector2D,
    #[def("CombatMiddleOffset")]
    pub combat_middle_offset: Vector2D,
    #[def("CombatMiddleMinWidth")]
    pub combat_middle_min_width: f32,
    #[def("CombatMiddleGraphic")]
    pub combat_middle_graphic: i32,
    #[def("CombatEndGraphic")]
    pub combat_end_graphic: i32,
    #[def("ExperiencePos")]
    pub experience_pos: Vector2D,
    #[def("ExperienceGraphic")]
    pub experience_graphic: i32,
    #[def("ExperienceGeneralGraphic")]
    pub experience_general_graphic: i32,
    #[def("ExperienceSkillGraphic")]
    pub experience_skill_graphic: i32,
    #[def("ExperienceStrengthGraphic")]
    pub experience_strength_graphic: i32,
    #[def("ExperienceWillGraphic")]
    pub experience_will_graphic: i32,
    #[def("ExperienceTextOffset")]
    pub experience_text_offset: Vector2D,
    #[def("ExperienceTextColour")]
    pub experience_text_colour: RGBColour,
    #[def("ExperienceMiddleOffset")]
    pub experience_middle_offset: Vector2D,
    #[def("ExperienceMiddleMinWidth")]
    pub experience_middle_min_width: f32,
    #[def("ExperienceMiddleGraphic")]
    pub experience_middle_graphic: i32,
    #[def("ExperienceEndGraphic")]
    pub experience_end_graphic: i32,
    #[def("CrimePos")]
    pub crime_pos: Vector2D,
    #[def("CrimeTextOffset")]
    pub crime_text_offset: Vector2D,
    #[def("CrimeTextColour")]
    pub crime_text_colour: RGBColour,
    #[def("CrimeTextAlertColour")]
    pub crime_text_alert_colour: RGBColour,
    #[def("CrimeWarningGraphic")]
    pub crime_warning_graphic: i32,
    #[def("CrimeCriminalGraphic")]
    pub crime_criminal_graphic: i32,
    #[def("CrimeBarGraphic")]
    pub crime_bar_graphic: i32,
    #[def("CrimeBarIconOffset")]
    pub crime_bar_icon_offset: Vector2D,
    #[def("CrimeBarBorderOffset")]
    pub crime_bar_border_offset: Vector2D,
    #[def("CrosshairOffsets")]
    pub crosshair_offsets: Vec<Vector2D>,
    #[def("CrosshairGraphics")]
    pub crosshair_graphics: Vec<i32>,
    #[def("CrosshairGreenGraphics")]
    pub crosshair_green_graphics: Vec<i32>,
    #[def("MiniGameFishingLeftGraphic")]
    pub mini_game_fishing_left_graphic: i32,
    #[def("MiniGameFishingRightGraphic")]
    pub mini_game_fishing_right_graphic: i32,
    #[def("MiniGameBettingGraphic")]
    pub mini_game_betting_graphic: i32,
    #[def("MiniGameDiggingGraphic")]
    pub mini_game_digging_graphic: i32,
    #[def("MiniGamePickpocketGraphic")]
    pub mini_game_pickpocket_graphic: i32,
    #[def("MiniGamePicklockGraphic")]
    pub mini_game_picklock_graphic: i32,
    #[def("MiniGameStealGraphic")]
    pub mini_game_steal_graphic: i32,
    #[def("MiniGameTrophyGotGraphic")]
    pub mini_game_trophy_got_graphic: i32,
    #[def("MiniGameTrophyLeftGraphic")]
    pub mini_game_trophy_left_graphic: i32,
    #[def("SmackEffect")]
    pub smack_effect: i32,
    #[def("SmackAlphaFadePerFrame")]
    pub smack_alpha_fade_per_frame: i32,
    #[def("SmackAlphaStartValue")]
    pub smack_alpha_start_value: i32,
    #[def("SmackAlphaEndValue")]
    pub smack_alpha_end_value: i32,
    #[def("SmackScaleFactorPerFrame")]
    pub smack_scale_factor_per_frame: f32,
    #[def("SmackTimeToRotateSeconds")]
    pub smack_time_to_rotate_seconds: f32,
    #[def("MoneyPos")]
    pub money_pos: Vector2D,
    #[def("MoneyTextOffset")]
    pub money_text_offset: Vector2D,
    #[def("MoneyTextRightOffset")]
    pub money_text_right_offset: Vector2D,
    #[def("MoneyWidth")]
    pub money_width: f32,
    #[def("MoneyTextColour")]
    pub money_text_colour: RGBColour,
    #[def("MoneyIncreaseTextColour")]
    pub money_increase_text_colour: RGBColour,
    #[def("MoneyDecreaseTextColour")]
    pub money_decrease_text_colour: RGBColour,
    #[def("MoneyChangeStartOffset")]
    pub money_change_start_offset: Vector2D,
    #[def("MoneyChangeEndOffset")]
    pub money_change_end_offset: Vector2D,
    #[def("MoneyChangeTextSpeed")]
    pub money_change_text_speed: f32,
    #[def("StatBarsFontName")]
    pub stat_bars_font_name: String,
    #[def("SecondsForStatsBarsToAppear")]
    pub seconds_for_stats_bars_to_appear: f32,
    #[def("SecondsBeforeStatsBackBarMoves")]
    pub seconds_before_stats_back_bar_moves: f32,
    #[def("SecondsToShowStatsBars")]
    pub seconds_to_show_stats_bars: f32,
    #[def("SecondsForStatsBarsToDisappear")]
    pub seconds_for_stats_bars_to_disappear: f32,
    #[def("StatUpdatePos")]
    pub stat_update_pos: Vector2D,
    #[def("StatUpdateMoralityGoodPos")]
    pub stat_update_morality_good_pos: Vector2D,
    #[def("StatUpdateMoralityEvilPos")]
    pub stat_update_morality_evil_pos: Vector2D,
    #[def("StatUpdateRenownPos")]
    pub stat_update_renown_pos: Vector2D,
    #[def("StatUpdateTextOffset")]
    pub stat_update_text_offset: Vector2D,
    #[def("StatUpdateTextColour")]
    pub stat_update_text_colour: RGBColour,
    #[def("StatUpdateDisplayDuration")]
    pub stat_update_display_duration: f32,
    #[def("StatUpdateWaitingDuration")]
    pub stat_update_waiting_duration: f32,
    #[def("StatUpdateEffectOffset")]
    pub stat_update_effect_offset: Vector2D,
    #[def("StatUpdateMoralityGoodGraphic")]
    pub stat_update_morality_good_graphic: i32,
    #[def("StatUpdateMoralityEvilGraphic")]
    pub stat_update_morality_evil_graphic: i32,
    #[def("StatUpdateMoralityGoodEffect")]
    pub stat_update_morality_good_effect: i32,
    #[def("StatUpdateMoralityEvilEffect")]
    pub stat_update_morality_evil_effect: i32,
    #[def("StatUpdateColours")]
    pub stat_update_colours: Vec<RGBColour>,
    #[def("StatUpdateGraphics")]
    pub stat_update_graphics: Vec<i32>,
    #[def("StatUpdateEffects")]
    pub stat_update_effects: Vec<i32>,
    #[def("AutoPickupPos")]
    pub auto_pickup_pos: Vector2D,
    #[def("QuestInfoPos")]
    pub quest_info_pos: Vector2D,
    #[def("QuestInfoBarOffsetX")]
    pub quest_info_bar_offset_x: f32,
    #[def("QuestInfoGraphicWidth")]
    pub quest_info_graphic_width: i32,
    #[def("QuestInfoGraphicHeight")]
    pub quest_info_graphic_height: i32,
    #[def("QuestInfoGraphicMinHeight")]
    pub quest_info_graphic_min_height: f32,
    #[def("QuestInfoSecondsToScroll")]
    pub quest_info_seconds_to_scroll: f32,
    #[def("QuestInfoSecondsToAppear")]
    pub quest_info_seconds_to_appear: f32,
    #[def("QuestInfoTurncoatColour")]
    pub quest_info_turncoat_colour: RGBColour,
    #[def("QuestInfoTickGraphic")]
    pub quest_info_tick_graphic: i32,
    #[def("OracleSymbolPos")]
    pub oracle_symbol_pos: Vector2D,
    #[def("OracleSymbolOffset")]
    pub oracle_symbol_offset: Vector2D,
    #[def("TextBoxTextAreaTLPos")]
    pub text_box_text_area_tl_pos: Vector2D,
    #[def("TextBoxTextAreaBRPos")]
    pub text_box_text_area_br_pos: Vector2D,
    #[def("TextBoxTextColour")]
    pub text_box_text_colour: RGBColour,
    #[def("TextBoxTextGameInfoColour")]
    pub text_box_text_game_info_colour: RGBColour,
    #[def("TextBoxTextGameActionColour")]
    pub text_box_text_game_action_colour: RGBColour,
    #[def("TextBoxButtonGraphicOffset")]
    pub text_box_button_graphic_offset: Vector2D,
    #[def("TextBoxButtonOffsetY")]
    pub text_box_button_offset_y: f32,
    #[def("TextBoxButtonInBoxOffsetY")]
    pub text_box_button_in_box_offset_y: f32,
    #[def("TextBoxButtonGapWidth")]
    pub text_box_button_gap_width: f32,
    #[def("TextBoxButtonTextFontName")]
    pub text_box_button_text_font_name: String,
    #[def("TextBoxConfirmGraphic")]
    pub text_box_confirm_graphic: i32,
    #[def("TextBoxCancelGraphic")]
    pub text_box_cancel_graphic: i32,
    #[def("TextBoxThirdGraphic")]
    pub text_box_third_graphic: i32,
    #[def("TextBoxSkipCutsceneGraphic")]
    pub text_box_skip_cutscene_graphic: i32,
    #[def("TextBoxButtonASoundCriteria")]
    pub text_box_button_a_sound_criteria: String,
    #[def("TextBoxButtonBSoundCriteria")]
    pub text_box_button_b_sound_criteria: String,
    #[def("TextBoxHugeButtonGraphic")]
    pub text_box_huge_button_graphic: i32,
    #[def("TextBoxHugeMouseButtonGraphic")]
    pub text_box_huge_mouse_button_graphic: i32,
    #[def("TextBoxHugeMouseButtonLeftGraphic")]
    pub text_box_huge_mouse_button_left_graphic: i32,
    #[def("TextBoxHugeButtonOffset")]
    pub text_box_huge_button_offset: Vector2D,
    #[def("TextBoxHugeButtonLeftOffset")]
    pub text_box_huge_button_left_offset: Vector2D,
    #[def("TextBoxFishingReelTextPos")]
    pub text_box_fishing_reel_text_pos: Vector2D,
    #[def("TextBoxFishingReelGraphicOffset")]
    pub text_box_fishing_reel_graphic_offset: Vector2D,
    #[def("TextBoxBettingGraphicPos")]
    pub text_box_betting_graphic_pos: Vector2D,
    #[def("TextBoxBettingUpOffset")]
    pub text_box_betting_up_offset: Vector2D,
    #[def("TextBoxBettingDownOffset")]
    pub text_box_betting_down_offset: Vector2D,
    #[def("TextBoxBettingLeftOffset")]
    pub text_box_betting_left_offset: Vector2D,
    #[def("TextBoxBettingRightOffset")]
    pub text_box_betting_right_offset: Vector2D,
    #[def("ReceiveItemsSoundCriteria")]
    pub receive_items_sound_criteria: String,
    #[def("ControllerAGraphic")]
    pub controller_a_graphic: i32,
    #[def("ControllerBGraphic")]
    pub controller_b_graphic: i32,
    #[def("ControllerXGraphic")]
    pub controller_x_graphic: i32,
    #[def("ControllerYGraphic")]
    pub controller_y_graphic: i32,
    #[def("ControllerDPadGraphic")]
    pub controller_d_pad_graphic: i32,
    #[def("ControllerDPadDownGraphic")]
    pub controller_d_pad_down_graphic: i32,
    #[def("ControllerDPadLeftGraphic")]
    pub controller_d_pad_left_graphic: i32,
    #[def("ControllerDPadRightGraphic")]
    pub controller_d_pad_right_graphic: i32,
    #[def("ControllerDPadUpGraphic")]
    pub controller_d_pad_up_graphic: i32,
    #[def("ControllerThumbLeftGraphic")]
    pub controller_thumb_left_graphic: i32,
    #[def("ControllerThumbRightGraphic")]
    pub controller_thumb_right_graphic: i32,
    #[def("ControllerThumbClickLeftGraphic")]
    pub controller_thumb_click_left_graphic: i32,
    #[def("ControllerThumbClickRightGraphic")]
    pub controller_thumb_click_right_graphic: i32,
    #[def("ControllerBackGraphic")]
    pub controller_back_graphic: i32,
    #[def("ControllerStartGraphic")]
    pub controller_start_graphic: i32,
    #[def("ControllerTriggerLeftGraphic")]
    pub controller_trigger_left_graphic: i32,
    #[def("ControllerTriggerRightGraphic")]
    pub controller_trigger_right_graphic: i32,
    #[def("ControllerTriggerBlackGraphic")]
    pub controller_trigger_black_graphic: i32,
    #[def("ControllerTriggerWhiteGraphic")]
    pub controller_trigger_white_graphic: i32,
    #[def("ControllerGraphicOffset")]
    pub controller_graphic_offset: Vector2D,
    #[def("ControllerGraphicTextOffset")]
    pub controller_graphic_text_offset: Vector2D,
    #[def("ControllerClickGraphicTextOffset")]
    pub controller_click_graphic_text_offset: Vector2D,
    #[def("ControllerTriggerGraphicTextOffset")]
    pub controller_trigger_graphic_text_offset: Vector2D,
    #[def("MenuConfirmPos")]
    pub menu_confirm_pos: Vector2D,
    #[def("MenuConfirmTextPos")]
    pub menu_confirm_text_pos: Vector2D,
    #[def("MenuConfirmGraphic")]
    pub menu_confirm_graphic: i32,
    #[def("MenuCancelPos")]
    pub menu_cancel_pos: Vector2D,
    #[def("MenuCancelTextPos")]
    pub menu_cancel_text_pos: Vector2D,
    #[def("MenuCancelGraphic")]
    pub menu_cancel_graphic: i32,
    #[def("QuickAccessDPadGraphic")]
    pub quick_access_d_pad_graphic: i32,
    #[def("QuickAccessMenuGraphic")]
    pub quick_access_menu_graphic: i32,
    #[def("QuickAccessLampEffect")]
    pub quick_access_lamp_effect: i32,
    #[def("QuickAccessGuildSealEffect")]
    pub quick_access_guild_seal_effect: i32,
    #[def("QuickAccessMainGuildSealEffect")]
    pub quick_access_main_guild_seal_effect: i32,
    #[def("QuickAccessEffectOffset")]
    pub quick_access_effect_offset: Vector2D,
    #[def("QuickAccessAbilityCrossOffset")]
    pub quick_access_ability_cross_offset: Vector2D,
    #[def("HotBarSingleGraphic")]
    pub hot_bar_single_graphic: i32,
    #[def("HotBarMainGraphic")]
    pub hot_bar_main_graphic: i32,
    #[def("HotBarEndGraphics")]
    pub hot_bar_end_graphics: Vec<i32>,
    #[def("HotBarOffset")]
    pub hot_bar_offset: Vector2D,
    #[def("HotBarEndOffset")]
    pub hot_bar_end_offset: Vector2D,
    #[def("HotBarItemWidth")]
    pub hot_bar_item_width: i32,
    #[def("QuickAccessItemButtons")]
    pub quick_access_item_buttons: Vec<QuickAccessItemButtonGuiDef>,
    #[def("ContextSensitiveItemButtons")]
    pub context_sensitive_item_buttons: Vec<QuickAccessItemButtonGuiDef>,
    #[def("TargetingCursorSectionEnemyEmptyGraphic")]
    pub targeting_cursor_section_enemy_empty_graphic: i32,
    #[def("CompassGraphic")]
    pub compass_graphic: i32,
    #[def("CompassPos")]
    pub compass_pos: Vector2D,
    #[def("CompassShadowGraphic")]
    pub compass_shadow_graphic: i32,
    #[def("CompassShadowOffset")]
    pub compass_shadow_offset: Vector2D,
    #[def("AbilityButtonTalkGraphic")]
    pub ability_button_talk_graphic: i32,
    #[def("AbilityButtonActivateGraphic")]
    pub ability_button_activate_graphic: i32,
    #[def("AbilityButtonBlockGraphic")]
    pub ability_button_block_graphic: i32,
    #[def("AbilityButtonUnarmedGraphic")]
    pub ability_button_unarmed_graphic: i32,
    #[def("AbilityButtonMeleeGraphic")]
    pub ability_button_melee_graphic: i32,
    #[def("AbilityButtonRangedGraphic")]
    pub ability_button_ranged_graphic: i32,
    #[def("AbilityButtonSheatheAxeGraphic")]
    pub ability_button_sheathe_axe_graphic: i32,
    #[def("AbilityButtonSheatheBowGraphic")]
    pub ability_button_sheathe_bow_graphic: i32,
    #[def("AbilityButtonSheatheMaceGraphic")]
    pub ability_button_sheathe_mace_graphic: i32,
    #[def("AbilityButtonSheatheStickGraphic")]
    pub ability_button_sheathe_stick_graphic: i32,
    #[def("AbilityButtonSheatheSwordGraphic")]
    pub ability_button_sheathe_sword_graphic: i32,
    #[def("AbilityButtonFlourishEffectPos")]
    pub ability_button_flourish_effect_pos: Vector2D,
    #[def("AbilityButtonFlourishGraphic")]
    pub ability_button_flourish_graphic: i32,
    #[def("AbilityButtonFlourishGraphicOffset")]
    pub ability_button_flourish_graphic_offset: Vector2D,
    #[def("ABXYButtonMiddleGraphic")]
    pub abxy_button_middle_graphic: i32,
    #[def("ABXYAbilityLevelInitialOffset")]
    pub abxy_ability_level_initial_offset: Vector2D,
    #[def("ABXYAbilityLevelOffset")]
    pub abxy_ability_level_offset: Vector2D,
    #[def("ABXYAbilityUnavailableGraphic")]
    pub abxy_ability_unavailable_graphic: i32,
    #[def("ABXYAbilityUncastableGraphic")]
    pub abxy_ability_uncastable_graphic: i32,
    #[def("ABXYAbilitySwitchGraphic")]
    pub abxy_ability_switch_graphic: i32,
    #[def("ABXYAbilityShieldOffGraphic")]
    pub abxy_ability_shield_off_graphic: i32,
    #[def("ABXYRunGraphic")]
    pub abxy_run_graphic: i32,
    #[def("ABXYTargetingEnterGraphic")]
    pub abxy_targeting_enter_graphic: i32,
    #[def("ABXYTargetingCancelGraphic")]
    pub abxy_targeting_cancel_graphic: i32,
    #[def("ABXYButtonFadeDurationSeconds")]
    pub abxy_button_fade_duration_seconds: f32,
    #[def("TargetingBorderLeftGraphic")]
    pub targeting_border_left_graphic: i32,
    #[def("TargetingBorderLeftPos")]
    pub targeting_border_left_pos: Vector2D,
    #[def("TargetingBorderRightGraphic")]
    pub targeting_border_right_graphic: i32,
    #[def("TargetingBorderRightPos")]
    pub targeting_border_right_pos: Vector2D,
    #[def("InteractButtonBorderGraphic")]
    pub interact_button_border_graphic: i32,
    #[def("InteractButtonBorderOffset")]
    pub interact_button_border_offset: Vector2D,
    #[def("InteractButtonTooltipOffset")]
    pub interact_button_tooltip_offset: Vector2D,
    #[def("AbilityButtons")]
    pub ability_buttons: Vec<AbilityButtonGuiDef>,
    #[def("SpecialAbilityButtons")]
    pub special_ability_buttons: Vec<AbilityButtonGuiDef>,
    #[def("DigitPos")]
    pub digit_pos: Vector2D,
    #[def("DigitHeroPos")]
    pub digit_hero_pos: Vector2D,
    #[def("DigitHeroOffset")]
    pub digit_hero_offset: Vector2D,
    #[def("DigitGraphics")]
    pub digit_graphics: Vec<i32>,
    #[def("RegionDisplayPos")]
    pub region_display_pos: Vector2D,
    #[def("InfoDisplayPos")]
    pub info_display_pos: Vector2D,
    #[def("HeroSprintBarLeftGraphic")]
    pub hero_sprint_bar_left_graphic: i32,
    #[def("HeroSprintBarLeftGlowGraphic")]
    pub hero_sprint_bar_left_glow_graphic: i32,
    #[def("HeroSprintBarMiddleGraphic")]
    pub hero_sprint_bar_middle_graphic: i32,
    #[def("HeroSprintBarMiddleGlowGraphic")]
    pub hero_sprint_bar_middle_glow_graphic: i32,
    #[def("HeroSprintBarRightGraphic")]
    pub hero_sprint_bar_right_graphic: i32,
    #[def("HeroSprintBarRightGlowGraphic")]
    pub hero_sprint_bar_right_glow_graphic: i32,
    #[def("HeroSprintBarInnerGraphic")]
    pub hero_sprint_bar_inner_graphic: i32,
    #[def("HeroSprintBarInnerGraphicOffset")]
    pub hero_sprint_bar_inner_graphic_offset: Vector2D,
    #[def("HighlightStartColour")]
    pub highlight_start_colour: Vec<RGBColour>,
    #[def("HighlightEndColour")]
    pub highlight_end_colour: Vec<RGBColour>,
    #[def("HighlightStartWidthWorldSpace")]
    pub highlight_start_width_world_space: Vec<f32>,
    #[def("HighlightEndWidthWorldSpace")]
    pub highlight_end_width_world_space: Vec<f32>,
    #[def("HighlightStartWidthScreenSpace")]
    pub highlight_start_width_screen_space: Vec<f32>,
    #[def("HighlightEndWidthScreenSpace")]
    pub highlight_end_width_screen_space: Vec<f32>,
    #[def("HighlightFadeSteps")]
    pub highlight_fade_steps: Vec<i32>,
    #[def("NumberOfSecondsForContainerItemToReachFullSize")]
    pub number_of_seconds_for_container_item_to_reach_full_size: f32,
    #[def("ContainerItemDisplayAreaTLPos")]
    pub container_item_display_area_tl_pos: Vector2D,
    #[def("ContainerItemDisplayAreaBRPos")]
    pub container_item_display_area_br_pos: Vector2D,
    #[def("NumberOfSecondsForContainerItemToRotate")]
    pub number_of_seconds_for_container_item_to_rotate: f32,
    #[def("NumberOfSecondsForContainerItemToStayAtFullSize")]
    pub number_of_seconds_for_container_item_to_stay_at_full_size: f32,
    #[def("ContainerItemDisplayBackgroundColour")]
    pub container_item_display_background_colour: RGBColour,
    #[def("ScreenMessageTextPos")]
    pub screen_message_text_pos: Vector2D,
    #[def("ScreenMessageTextColour")]
    pub screen_message_text_colour: RGBColour,
    #[def("ScreenMessageTextDropShadowOffset")]
    pub screen_message_text_drop_shadow_offset: Vector2D,
    #[def("ScreenMessageTextDropShadowColour")]
    pub screen_message_text_drop_shadow_colour: RGBColour,
    #[def("ScreenMessageFont")]
    pub screen_message_font: String,
    #[def("ScreenMessageNextMessageTextOffset")]
    pub screen_message_next_message_text_offset: Vector2D,
    #[def("MaximumNumberOfScreenMessages")]
    pub maximum_number_of_screen_messages: i32,
    #[def("NumberOfSecondsScreenMessageStaysOnScreen")]
    pub number_of_seconds_screen_message_stays_on_screen: f32,
    #[def("NumberOfCharactersScreenMessageAddsPerFrame")]
    pub number_of_characters_screen_message_adds_per_frame: i32,
    #[def("TutorialText")]
    pub tutorial_text: Vec<String>,
    #[def("TutorialLogBookText")]
    pub tutorial_log_book_text: Vec<String>,
    #[def("TutorialGuildSealGraphics")]
    pub tutorial_guild_seal_graphics: Vec<i32>,
    #[def("ScriptSprites")]
    pub script_sprites: VecMap<String, i32>,
    #[def("GameActionValues")]
    pub game_action_values: VecMap<String, i32>,
    #[def("MiniMapGraphics")]
    pub mini_map_graphics: MiniMapGraphics,
    #[def("MiniMapMarkerGraphics")]
    pub mini_map_marker_graphics: Vec<i32>,
    #[def("MiniMapScreenOrigin")]
    pub mini_map_screen_origin: Vector2D,
    #[def("MiniMapScreenSize")]
    pub mini_map_screen_size: Vector2D,
    #[def("MiniMapScreenRadius")]
    pub mini_map_screen_radius: i32,
    #[def("MiniMapTextureWidth")]
    pub mini_map_texture_width: i32,
    #[def("MiniMapMarkerTextureWidth")]
    pub mini_map_marker_texture_width: i32,
    #[def("MiniMapWorldResolution")]
    pub mini_map_world_resolution: f32,
    #[def("MiniMapPassableColour")]
    pub mini_map_passable_colour: RGBColour,
    #[def("MiniMapImpassableColour")]
    pub mini_map_impassable_colour: RGBColour,
    #[def("MiniMapImpassableWaterColour")]
    pub mini_map_impassable_water_colour: RGBColour,
    #[def("MiniMapWaterColour")]
    pub mini_map_water_colour: RGBColour,
    #[def("MiniMapMaplessColour")]
    pub mini_map_mapless_colour: RGBColour,
    #[def("MaxMinimapThemeColours")]
    pub max_minimap_theme_colours: u32,
    #[def("MinimapThemeColours")]
    pub minimap_theme_colours: Vec<RGBColour>,
    #[def("MaxMinimapZoneColours")]
    pub max_minimap_zone_colours: u32,
    #[def("MinimapZoneColours")]
    pub minimap_zone_colours: Vec<RGBColour>,
    #[def("RegionChangeTLOffset")]
    pub region_change_tl_offset: Vector2D,
    #[def("RegionChangeTMOffset")]
    pub region_change_tm_offset: Vector2D,
    #[def("RegionChangeTROffset")]
    pub region_change_tr_offset: Vector2D,
    #[def("RegionChangeBLOffset")]
    pub region_change_bl_offset: Vector2D,
    #[def("RegionChangeBMOffset")]
    pub region_change_bm_offset: Vector2D,
    #[def("RegionChangeBROffset")]
    pub region_change_br_offset: Vector2D,
    #[def("RegionChangeTLGraphic")]
    pub region_change_tl_graphic: i32,
    #[def("RegionChangeTMGraphic")]
    pub region_change_tm_graphic: i32,
    #[def("RegionChangeTRGraphic")]
    pub region_change_tr_graphic: i32,
    #[def("RegionChangeBLGraphic")]
    pub region_change_bl_graphic: i32,
    #[def("RegionChangeBMGraphic")]
    pub region_change_bm_graphic: i32,
    #[def("RegionChangeBRGraphic")]
    pub region_change_br_graphic: i32,
    #[def("RegionChangeBorderLeftOffset")]
    pub region_change_border_left_offset: Vector2D,
    #[def("RegionChangeBorderRightOffset")]
    pub region_change_border_right_offset: Vector2D,
    #[def("RegionChangeBorderLeftGraphic")]
    pub region_change_border_left_graphic: i32,
    #[def("RegionChangeBorderRightGraphic")]
    pub region_change_border_right_graphic: i32,
    #[def("RegionChangeGuildSealBorderGraphic")]
    pub region_change_guild_seal_border_graphic: i32,
    #[def("RegionChangeGuildSealBorderOffset")]
    pub region_change_guild_seal_border_offset: Vector2D,
    #[def("RegionChangeGuildSealGraphics")]
    pub region_change_guild_seal_graphics: Vec<i32>,
    #[def("LoadProgressBarLeftGraphic")]
    pub load_progress_bar_left_graphic: i32,
    #[def("LoadProgressBarMidGraphic")]
    pub load_progress_bar_mid_graphic: i32,
    #[def("LoadProgressBarRightGraphic")]
    pub load_progress_bar_right_graphic: i32,
    #[def("LoadProgressBarInnerGraphic")]
    pub load_progress_bar_inner_graphic: i32,
    #[def("LoadProgressBackdropGraphics")]
    pub load_progress_backdrop_graphics: Vec<i32>,
    #[def("TavernGameBlockTLOffset")]
    pub tavern_game_block_tl_offset: Vector2D,
    #[def("TavernGameBlockTMOffset")]
    pub tavern_game_block_tm_offset: Vector2D,
    #[def("TavernGameBlockTROffset")]
    pub tavern_game_block_tr_offset: Vector2D,
    #[def("TavernGameBlockBLOffset")]
    pub tavern_game_block_bl_offset: Vector2D,
    #[def("TavernGameBlockBMOffset")]
    pub tavern_game_block_bm_offset: Vector2D,
    #[def("TavernGameBlockBROffset")]
    pub tavern_game_block_br_offset: Vector2D,
    #[def("TavernGameBlockTLGraphic")]
    pub tavern_game_block_tl_graphic: i32,
    #[def("TavernGameBlockTMGraphic")]
    pub tavern_game_block_tm_graphic: i32,
    #[def("TavernGameBlockTRGraphic")]
    pub tavern_game_block_tr_graphic: i32,
    #[def("TavernGameBlockBLGraphic")]
    pub tavern_game_block_bl_graphic: i32,
    #[def("TavernGameBlockBMGraphic")]
    pub tavern_game_block_bm_graphic: i32,
    #[def("TavernGameBlockBRGraphic")]
    pub tavern_game_block_br_graphic: i32,
    #[def("RegionGraphFileName")]
    pub region_graph_file_name: String,
    #[def("MouseCursorScaleX")]
    pub mouse_cursor_scale_x: f32,
    #[def("MouseCursorScaleY")]
    pub mouse_cursor_scale_y: f32,
    #[def("MouseCursorGraphic")]
    pub mouse_cursor_graphic: i32,
    #[def("MouseLMBGraphic")]
    pub mouse_lmb_graphic: i32,
    #[def("MouseMMBGraphic")]
    pub mouse_mmb_graphic: i32,
    #[def("MouseRMBGraphic")]
    pub mouse_rmb_graphic: i32,
    #[def("MouseOverlayGraphic")]
    pub mouse_overlay_graphic: i32,
}
