use crate::DefStruct;
use crate::def::{values::MapPathEntry, wire::VecMap, wire::WStr};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct UiMiscThingsDef {
    #[def("SpaceSeparator")]
    pub space_separator: WStr,
    #[def("CommaSeparator")]
    pub comma_separator: WStr,
    #[def("NewLineSeparator")]
    pub new_line_separator: WStr,
    #[def("OpenBracket")]
    pub open_bracket: WStr,
    #[def("CloseBracket")]
    pub close_bracket: WStr,
    #[def("Positive")]
    pub positive: WStr,
    #[def("WeaponValueString")]
    pub weapon_value_string: WStr,
    #[def("WeaponAugString")]
    pub weapon_aug_string: WStr,
    #[def("WeaponAugNone")]
    pub weapon_aug_none: WStr,
    #[def("WeaponWeightString")]
    pub weapon_weight_string: WStr,
    #[def("WeaponLightString")]
    pub weapon_light_string: WStr,
    #[def("WeaponHeavyString")]
    pub weapon_heavy_string: WStr,
    #[def("WeaponKillsString")]
    pub weapon_kills_string: WStr,
    #[def("WeaponCatMeleeString")]
    pub weapon_cat_melee_string: WStr,
    #[def("WeaponCatRangedString")]
    pub weapon_cat_ranged_string: WStr,
    #[def("WeaponDamageString")]
    pub weapon_damage_string: WStr,
    #[def("TradeCostString")]
    pub trade_cost_string: WStr,
    #[def("ColonSeparator")]
    pub colon_separator: WStr,
    #[def("TradeProfitString")]
    pub trade_profit_string: WStr,
    #[def("TradeLossString")]
    pub trade_loss_string: WStr,
    #[def("TradeAlreadyOwnsString")]
    pub trade_already_owns_string: WStr,
    #[def("TradeNumberInStockString")]
    pub trade_number_in_stock_string: WStr,
    #[def("TradeDeliveryString")]
    pub trade_delivery_string: WStr,
    #[def("TradeNoDeliveryString")]
    pub trade_no_delivery_string: WStr,
    #[def("TradeDaysString")]
    pub trade_days_string: WStr,
    #[def("TradeBuyString")]
    pub trade_buy_string: WStr,
    #[def("TradeSellString")]
    pub trade_sell_string: WStr,
    #[def("TradeWantedString")]
    pub trade_wanted_string: WStr,
    #[def("QuestFailedString")]
    pub quest_failed_string: WStr,
    #[def("FailedString")]
    pub failed_string: WStr,
    #[def("SucceededString")]
    pub succeeded_string: WStr,
    #[def("Plus")]
    pub plus: WStr,
    #[def("Minus")]
    pub minus: WStr,
    #[def("CoreGraphic")]
    pub core_graphic: u32,
    #[def("VignetteGraphic")]
    pub vignette_graphic: u32,
    #[def("OptionalGraphic")]
    pub optional_graphic: u32,
    #[def("FeatGraphic")]
    pub feat_graphic: u32,
    #[def("ObjectsRewardString")]
    pub objects_reward_string: WStr,
    #[def("NoneString")]
    pub none_string: WStr,
    #[def("CheckGuildString")]
    pub check_guild_string: WStr,
    #[def("QuestStartingString")]
    pub quest_starting_string: WStr,
    #[def("RingCenterX")]
    pub ring_center_x: f32,
    #[def("RingCenterY")]
    pub ring_center_y: f32,
    #[def("PCRingCenterX")]
    pub pc_ring_center_x: f32,
    #[def("PCRingCenterY")]
    pub pc_ring_center_y: f32,
    #[def("WorldMapOffsetX")]
    pub world_map_offset_x: f32,
    #[def("WorldMapOffsetY")]
    pub world_map_offset_y: f32,
    #[def("WorldMapWidth")]
    pub world_map_width: f32,
    #[def("WorldMapHeight")]
    pub world_map_height: f32,
    #[def("YouString")]
    pub you_string: WStr,
    #[def("OwnString")]
    pub own_string: WStr,
    #[def("NoString")]
    pub no_string: WStr,
    #[def("HousesString")]
    pub houses_string: WStr,
    #[def("HouseString")]
    pub house_string: WStr,
    #[def("InString")]
    pub in_string: WStr,
    #[def("ShopsString")]
    pub shops_string: WStr,
    #[def("ShopString")]
    pub shop_string: WStr,
    #[def("ThereString")]
    pub there_string: WStr,
    #[def("AreString")]
    pub are_string: WStr,
    #[def("IsString")]
    pub is_string: WStr,
    #[def("ForString")]
    pub for_string: WStr,
    #[def("SaleString")]
    pub sale_string: WStr,
    #[def("GeneralString")]
    pub general_string: WStr,
    #[def("TatooString")]
    pub tatoo_string: WStr,
    #[def("BarberString")]
    pub barber_string: WStr,
    #[def("TitleString")]
    pub title_string: WStr,
    #[def("LevelString")]
    pub level_string: WStr,
    #[def("TotalSpellsInPalettes", default = 18)]
    pub total_spells_in_palettes: u32,
    #[def("TotalSpellsInContainer", default = 3)]
    pub total_spells_in_container: u32,
    #[def("TotalAssignableThings", default = 8)]
    pub total_assignable_things: u32,
    #[def("LogBookBasicsCategoryString")]
    pub log_book_basics_category_string: WStr,
    #[def("LogBookObjectsCategoryString")]
    pub log_book_objects_category_string: WStr,
    #[def("LogBookTownsCategoryString")]
    pub log_book_towns_category_string: WStr,
    #[def("LogBookHeroCategoryString")]
    pub log_book_hero_category_string: WStr,
    #[def("LogBookCombatCategoryString")]
    pub log_book_combat_category_string: WStr,
    #[def("LogBookQuestCategoryString")]
    pub log_book_quest_category_string: WStr,
    #[def("LogBookStoryCategoryString")]
    pub log_book_story_category_string: WStr,
    #[def("LogBookBasicsCategoryNameString")]
    pub log_book_basics_category_name_string: WStr,
    #[def("LogBookObjectsCategoryNameString")]
    pub log_book_objects_category_name_string: WStr,
    #[def("LogBookTownsCategoryNameString")]
    pub log_book_towns_category_name_string: WStr,
    #[def("LogBookHeroCategoryNameString")]
    pub log_book_hero_category_name_string: WStr,
    #[def("LogBookCombatCategoryNameString")]
    pub log_book_combat_category_name_string: WStr,
    #[def("LogBookQuestCategoryNameString")]
    pub log_book_quest_category_name_string: WStr,
    #[def("LogBookStoryCategoryNameString")]
    pub log_book_story_category_name_string: WStr,
    #[def("MapPaths")]
    pub map_paths: Vec<MapPathEntry>,
    #[def("SoundUpDown")]
    pub sound_up_down: String,
    #[def("SoundSlider")]
    pub sound_slider: String,
    #[def("SoundBack")]
    pub sound_back: String,
    #[def("SoundForward")]
    pub sound_forward: String,
    #[def("SoundError")]
    pub sound_error: String,
    #[def("SoundExit")]
    pub sound_exit: String,
    #[def("HeroDollTLX", default = 310.0)]
    pub hero_doll_tlx: f32,
    #[def("HeroDollTLY", default = 33.0)]
    pub hero_doll_tly: f32,
    #[def("HeroDollBRX", default = 560.0)]
    pub hero_doll_brx: f32,
    #[def("HeroDollBRY", default = 300.0)]
    pub hero_doll_bry: f32,
    #[def("HeroDollSphereRadius", default = 1.3)]
    pub hero_doll_sphere_radius: f32,
    #[def("HeroDollTLX_PC", default = 310.0)]
    pub hero_doll_tlx_pc: f32,
    #[def("HeroDollTLY_PC", default = 33.0)]
    pub hero_doll_tly_pc: f32,
    #[def("HeroDollBRX_PC", default = 560.0)]
    pub hero_doll_brx_pc: f32,
    #[def("HeroDollBRY_PC", default = 300.0)]
    pub hero_doll_bry_pc: f32,
    #[def("HeroDollSphereRadius_PC", default = 1.3)]
    pub hero_doll_sphere_radius_pc: f32,
    #[def("HeroDollFrameTLX_PC")]
    pub hero_doll_frame_tlx_pc: f32,
    #[def("HeroDollFrameTLY_PC")]
    pub hero_doll_frame_tly_pc: f32,
    #[def("HeroDollFrameEmulateListOffset")]
    pub hero_doll_frame_emulate_list_offset: f32,
    #[def("QuestStartScreenMusic", default = 2)]
    pub quest_start_screen_music: u32,
    #[def("QuestCompleteScreenMusic", default = 3)]
    pub quest_complete_screen_music: u32,
    #[def("QuestFailureScreenMusic", default = 9)]
    pub quest_failure_screen_music: u32,
    #[def("DeathScreenMusic", default = 14)]
    pub death_screen_music: u32,
    #[def("CountUpSound")]
    pub count_up_sound: String,
    #[def("DigitCountTime", default = 3.0)]
    pub digit_count_time: f32,
    #[def("SaveHeroGraphicIndex")]
    pub save_hero_graphic_index: u32,
    #[def("MiniMapGraphics")]
    pub mini_map_graphics: VecMap<String, i32>,
    #[def("SoundKeyboardUp")]
    pub sound_keyboard_up: String,
    #[def("SoundKeyboardDown")]
    pub sound_keyboard_down: String,
    #[def("SoundKeyboardLeft")]
    pub sound_keyboard_left: String,
    #[def("SoundKeyboardRight")]
    pub sound_keyboard_right: String,
    #[def("SoundKeyboardEnterCharacter")]
    pub sound_keyboard_enter_character: String,
    #[def("SoundKeyboardDeleteCharacter")]
    pub sound_keyboard_delete_character: String,
    #[def("SoundKeyboardDone")]
    pub sound_keyboard_done: String,
    #[def("FrontEndMusic")]
    pub front_end_music: WStr,
    #[def("KeyboardSmallKeyGraphic")]
    pub keyboard_small_key_graphic: u32,
    #[def("KeyboardLargeKeyGraphic")]
    pub keyboard_large_key_graphic: u32,
    #[def("TimeInSecsForFade")]
    pub time_in_secs_for_fade: f32,
    #[def("BackBufferFilterSaturation")]
    pub back_buffer_filter_saturation: f32,
    #[def("BackBufferFilterContrast")]
    pub back_buffer_filter_contrast: f32,
    #[def("BackBufferFilterBrightness")]
    pub back_buffer_filter_brightness: f32,
    #[def("BackBufferFilterTintR")]
    pub back_buffer_filter_tint_r: f32,
    #[def("BackBufferFilterTintG")]
    pub back_buffer_filter_tint_g: f32,
    #[def("BackBufferFilterTintB")]
    pub back_buffer_filter_tint_b: f32,
    #[def("BackBufferFilterTintScale")]
    pub back_buffer_filter_tint_scale: f32,
    #[def("BackBufferDiffuseScale")]
    pub back_buffer_diffuse_scale: f32,
    #[def("BackBufferAmbientScale")]
    pub back_buffer_ambient_scale: f32,
    #[def("MinimumFilterColor")]
    pub minimum_filter_color: f32,
}
