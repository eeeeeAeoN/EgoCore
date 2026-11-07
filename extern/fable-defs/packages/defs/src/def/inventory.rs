use crate::DefStruct;
use crate::def::{
    values::{EngineGraphic, RGBColour, Vector2D, Vector3D},
    wire::{DefIndex, DefString},
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct InventoryDef {
    #[def("InitialCategoryMovementRepeatDelay")]
    pub initial_category_movement_repeat_delay: f32,
    #[def("ContinuedCategoryMovementRepeatDelay")]
    pub continued_category_movement_repeat_delay: f32,
    #[def("InitialItemMovementRepeatDelay")]
    pub initial_item_movement_repeat_delay: f32,
    #[def("ContinuedItemMovementRepeatDelay")]
    pub continued_item_movement_repeat_delay: f32,
    #[def("MovementSound")]
    pub movement_sound: i32,
    #[def("NoMovementSound")]
    pub no_movement_sound: i32,
    #[def("InventoryOpenSound")]
    pub inventory_open_sound: i32,
    #[def("TrackSelectedItems")]
    pub track_selected_items: bool,
    #[def("CategoriesTLPos")]
    pub categories_tl_pos: Vector2D,
    #[def("MeshAreaTLPos")]
    pub mesh_area_tl_pos: Vector2D,
    #[def("MeshAreaBRPos")]
    pub mesh_area_br_pos: Vector2D,
    #[def("NextCategoryOffset")]
    pub next_category_offset: Vector2D,
    #[def("FirstItemOffsetFromCategory")]
    pub first_item_offset_from_category: Vector2D,
    #[def("NextItemOffset")]
    pub next_item_offset: Vector2D,
    #[def("GenericFont")]
    pub generic_font: DefString,
    #[def("BackGroundColour")]
    pub back_ground_colour: RGBColour,
    #[def("ClothingDoll")]
    pub clothing_doll: DefIndex,
    #[def("DollCentreOffset")]
    pub doll_centre_offset: Vector3D,
    #[def("DollBoundingSphereRadius")]
    pub doll_bounding_sphere_radius: f32,
    #[def("TotalExperienceTextPos")]
    pub total_experience_text_pos: Vector2D,
    #[def("TotalExperienceText")]
    pub total_experience_text: u32,
    #[def("SpendingExperienceTextPos")]
    pub spending_experience_text_pos: Vector2D,
    #[def("SpendingExperienceText")]
    pub spending_experience_text: u32,
    #[def("StrengthLevelTextPos")]
    pub strength_level_text_pos: Vector2D,
    #[def("StrengthLevelColour")]
    pub strength_level_colour: RGBColour,
    #[def("SkillLevelTextPos")]
    pub skill_level_text_pos: Vector2D,
    #[def("SkillLevelColour")]
    pub skill_level_colour: RGBColour,
    #[def("WillLevelTextPos")]
    pub will_level_text_pos: Vector2D,
    #[def("WillLevelColour")]
    pub will_level_colour: RGBColour,
    #[def("RenownLevelTextPos")]
    pub renown_level_text_pos: Vector2D,
    #[def("HeroLevelTextPos")]
    pub hero_level_text_pos: Vector2D,
    #[def("StatLevelsFirstLineTextPos")]
    pub stat_levels_first_line_text_pos: Vector2D,
    #[def("StatLevelsNextLineTextOffset")]
    pub stat_levels_next_line_text_offset: Vector2D,
    #[def("ButtonAssignmentTextPos")]
    pub button_assignment_text_pos: Vector2D,
    #[def("ButtonAssignmentTextOffset")]
    pub button_assignment_text_offset: Vector2D,
    #[def("StatLevelsTexts")]
    pub stat_levels_texts: Vec<i32>,
    #[def("StatLevelsColours")]
    pub stat_levels_colours: Vec<RGBColour>,
    #[def("WeaponsTextNumberOfSlots")]
    pub weapons_text_number_of_slots: i32,
    #[def("WeaponsTextSeparator")]
    pub weapons_text_separator: i32,
    #[def("WeaponsTextSlot")]
    pub weapons_text_slot: i32,
    #[def("WeaponsTextSuccessfulAugmentation")]
    pub weapons_text_successful_augmentation: i32,
    #[def("WeaponsTextNoFreeSlots")]
    pub weapons_text_no_free_slots: i32,
    #[def("WeaponsTextReadyToAugment")]
    pub weapons_text_ready_to_augment: i32,
    #[def("WeaponsTextNotReadyToAugment")]
    pub weapons_text_not_ready_to_augment: i32,
    #[def("WeaponsDescriptionBoxTLPos")]
    pub weapons_description_box_tl_pos: Vector2D,
    #[def("WeaponsDescriptionBoxExtents")]
    pub weapons_description_box_extents: Vector2D,
    #[def("WeaponsActiveMeleeIconPos")]
    pub weapons_active_melee_icon_pos: Vector2D,
    #[def("WeaponsActiveRangeIconPos")]
    pub weapons_active_range_icon_pos: Vector2D,
    #[def("WeaponAugmentationTextStartPos")]
    pub weapon_augmentation_text_start_pos: Vector2D,
    #[def("WeaponAugmentationTextNewLineOffset")]
    pub weapon_augmentation_text_new_line_offset: Vector2D,
    #[def("AugmentationStatusTextAreaTLPos")]
    pub augmentation_status_text_area_tl_pos: Vector2D,
    #[def("AugmentationStatusTextAreaBRPos")]
    pub augmentation_status_text_area_br_pos: Vector2D,
    #[def("TimeToDisplayWarningMessageInSeconds")]
    pub time_to_display_warning_message_in_seconds: f32,
    #[def("NumberOfSecondsForObjectRotation")]
    pub number_of_seconds_for_object_rotation: f32,
    #[def("MaxNumberOfItemsToDisplayPerCategory")]
    pub max_number_of_items_to_display_per_category: i32,
    #[def("MaxNumberOfCategoriesToDisplay")]
    pub max_number_of_categories_to_display: i32,
    #[def("MaxNumberOfScreenNamesToDisplay")]
    pub max_number_of_screen_names_to_display: i32,
    #[def("WeaponsShadedCircleTLPos")]
    pub weapons_shaded_circle_tl_pos: Vector2D,
    #[def("WeaponsShadedCircleExtents")]
    pub weapons_shaded_circle_extents: Vector2D,
    #[def("InventoryName")]
    pub inventory_name: u32,
    #[def("InventoryIdentifier")]
    pub inventory_identifier: i32,
    #[def("MapScreenMapDisplayBoxTLPos")]
    pub map_screen_map_display_box_tl_pos: Vector2D,
    #[def("MapScreenMapDisplayBoxBRPos")]
    pub map_screen_map_display_box_br_pos: Vector2D,
    #[def("MapScreenRegionNamePos")]
    pub map_screen_region_name_pos: Vector2D,
    #[def("MapScreenWorldMapGraphic")]
    pub map_screen_world_map_graphic: i32,
    #[def("MapScreenWorldMapPos")]
    pub map_screen_world_map_pos: Vector2D,
    #[def("MapScreenRegionUnknownGraphic")]
    pub map_screen_region_unknown_graphic: i32,
    #[def("MapScreenRegionUnknownHighlightGraphic")]
    pub map_screen_region_unknown_highlight_graphic: i32,
    #[def("MapScreenRegionHeroGraphic")]
    pub map_screen_region_hero_graphic: i32,
    #[def("MapScreenRegionQuestGraphic")]
    pub map_screen_region_quest_graphic: i32,
    #[def("MapScreenRegionTeleportGraphic")]
    pub map_screen_region_teleport_graphic: i32,
    #[def("MapScreenRegionHouseGraphic")]
    pub map_screen_region_house_graphic: i32,
    #[def("StatsTitleTLPos")]
    pub stats_title_tl_pos: Vector2D,
    #[def("StatsShadedCircleTLPos")]
    pub stats_shaded_circle_tl_pos: Vector2D,
    #[def("StatsShadedCircleExtents")]
    pub stats_shaded_circle_extents: Vector2D,
    #[def("StatsProgBarL")]
    pub stats_prog_bar_l: EngineGraphic,
    #[def("StatsProgBarC")]
    pub stats_prog_bar_c: EngineGraphic,
    #[def("StatsProgBarR")]
    pub stats_prog_bar_r: EngineGraphic,
    #[def("StatsProgBarValue")]
    pub stats_prog_bar_value: EngineGraphic,
    #[def("StatsProgBarValueOffset")]
    pub stats_prog_bar_value_offset: Vector2D,
    #[def("StatsPersonalityHeadingTLPos")]
    pub stats_personality_heading_tl_pos: Vector2D,
    #[def("StatsPersonalityHeadingBRPos")]
    pub stats_personality_heading_br_pos: Vector2D,
    #[def("StatsPersonalityRenownLowerLabel")]
    pub stats_personality_renown_lower_label: u32,
    #[def("StatsPersonalityRenownUpperLabel")]
    pub stats_personality_renown_upper_label: u32,
    #[def("StatsPersonalityGoodnessLowerLabel")]
    pub stats_personality_goodness_lower_label: u32,
    #[def("StatsPersonalityGoodnessUpperLabel")]
    pub stats_personality_goodness_upper_label: u32,
    #[def("StatsPersonalityAttractivenessLowerLabel")]
    pub stats_personality_attractiveness_lower_label: u32,
    #[def("StatsPersonalityAttractivenessUpperLabel")]
    pub stats_personality_attractiveness_upper_label: u32,
    #[def("StatsPersonalityScarinessLowerLabel")]
    pub stats_personality_scariness_lower_label: u32,
    #[def("StatsPersonalityScarinessUpperLabel")]
    pub stats_personality_scariness_upper_label: u32,
    #[def("StatsPersonalityNicenessLowerLabel")]
    pub stats_personality_niceness_lower_label: u32,
    #[def("StatsPersonalityNicenessUpperLabel")]
    pub stats_personality_niceness_upper_label: u32,
    #[def("StatsPersonalityLabelTextOffset")]
    pub stats_personality_label_text_offset: Vector2D,
    #[def("StatsPersonalityLowerLabelTLPos")]
    pub stats_personality_lower_label_tl_pos: Vector2D,
    #[def("StatsPersonalityLowerLabelWidth")]
    pub stats_personality_lower_label_width: f32,
    #[def("StatsPersonalityLowerLabelHeight")]
    pub stats_personality_lower_label_height: f32,
    #[def("StatsPersonalitySliderWidth")]
    pub stats_personality_slider_width: f32,
    #[def("StatsPersonalityUpperLabelWidth")]
    pub stats_personality_upper_label_width: f32,
    #[def("StatsPersonalityDoll")]
    pub stats_personality_doll: DefIndex,
    #[def("StatsPersonalityDollCentreOffset")]
    pub stats_personality_doll_centre_offset: Vector3D,
    #[def("StatsPersonalityDollBoundingSphereRadius")]
    pub stats_personality_doll_bounding_sphere_radius: f32,
    #[def("StatsStatisticsLabelTLPos")]
    pub stats_statistics_label_tl_pos: Vector2D,
    #[def("StatsStatisticsLabelWidth")]
    pub stats_statistics_label_width: f32,
    #[def("StatsStatisticsLabelHeight")]
    pub stats_statistics_label_height: f32,
    #[def("StatsStatisticsLabelTextOffset")]
    pub stats_statistics_label_text_offset: Vector2D,
    #[def("StatsStatisticsDataTextOffset")]
    pub stats_statistics_data_text_offset: Vector2D,
    #[def("StatsStatisticsHeadingTLPos")]
    pub stats_statistics_heading_tl_pos: Vector2D,
    #[def("StatsStatisticsHeadingBRPos")]
    pub stats_statistics_heading_br_pos: Vector2D,
    #[def("StatsStatisticsPeopleKilledLabel")]
    pub stats_statistics_people_killed_label: u32,
    #[def("StatsStatisticsMonstersKilledLabel")]
    pub stats_statistics_monsters_killed_label: u32,
    #[def("StatsStatisticsMaxMultiplierLabel")]
    pub stats_statistics_max_multiplier_label: u32,
    #[def("StatsStatisticsPreferredFoodLabel")]
    pub stats_statistics_preferred_food_label: u32,
    #[def("StatsStatisticsDietaryTypeLabel")]
    pub stats_statistics_dietary_type_label: u32,
    #[def("StatsStatisticsGameTimeElapsedLabel")]
    pub stats_statistics_game_time_elapsed_label: u32,
    #[def("StatsStatisticsNumberOfWivesLabel")]
    pub stats_statistics_number_of_wives_label: u32,
    #[def("StatsStatisticsQuestsActiveLabel")]
    pub stats_statistics_quests_active_label: u32,
    #[def("StatsStatisticsQuestsFinishedLabel")]
    pub stats_statistics_quests_finished_label: u32,
    #[def("StatsStatisticsBoastsAchievedLabel")]
    pub stats_statistics_boasts_achieved_label: u32,
    #[def("InitialInventoryStatsItems")]
    pub initial_inventory_stats_items: Vec<DefIndex>,
    #[def("InitialInventoryStandardItems")]
    pub initial_inventory_standard_items: Vec<i32>,
    #[def("InitialInventoryWeaponsItems")]
    pub initial_inventory_weapons_items: Vec<i32>,
    #[def("InitialInventoryClothingItems")]
    pub initial_inventory_clothing_items: Vec<DefIndex>,
    #[def("ClothingShadedCircleTLPos")]
    pub clothing_shaded_circle_tl_pos: Vector2D,
    #[def("ClothingShadedCircleExtents")]
    pub clothing_shaded_circle_extents: Vector2D,
    #[def("StatsExperienceStrengthTextOffset")]
    pub stats_experience_strength_text_offset: Vector2D,
    #[def("StatsExperienceWillTextOffset")]
    pub stats_experience_will_text_offset: Vector2D,
    #[def("StatsExperienceSkillTextOffset")]
    pub stats_experience_skill_text_offset: Vector2D,
    #[def("StatsExperienceStrengthText")]
    pub stats_experience_strength_text: u32,
    #[def("StatsExperienceWillText")]
    pub stats_experience_will_text: u32,
    #[def("StatsExperienceSkillText")]
    pub stats_experience_skill_text: u32,
    #[def("StatsExperiencePoolGeneralTLPos")]
    pub stats_experience_pool_general_tl_pos: Vector2D,
    #[def("StatsExperiencePoolGeneralTextOffset")]
    pub stats_experience_pool_general_text_offset: Vector2D,
    #[def("StatsExperiencePoolStrengthGroupTLPos")]
    pub stats_experience_pool_strength_group_tl_pos: Vector2D,
    #[def("StatsExperiencePoolSkillGroupTLPos")]
    pub stats_experience_pool_skill_group_tl_pos: Vector2D,
    #[def("StatsExperiencePoolWillGroupTLPos")]
    pub stats_experience_pool_will_group_tl_pos: Vector2D,
    #[def("StatsExperiencePoolLabelTLPos")]
    pub stats_experience_pool_label_tl_pos: Vector2D,
    #[def("StatsExperiencePoolLabelTextOffset")]
    pub stats_experience_pool_label_text_offset: Vector2D,
    #[def("StatsExperiencePoolLabelWidth")]
    pub stats_experience_pool_label_width: f32,
    #[def("StatsExperiencePoolLabelHeight")]
    pub stats_experience_pool_label_height: f32,
    #[def("StatsExperiencePoolSliderWidth")]
    pub stats_experience_pool_slider_width: f32,
    #[def("StatsExperiencePoolStrengthLabel")]
    pub stats_experience_pool_strength_label: u32,
    #[def("StatsExperiencePoolSkillLabel")]
    pub stats_experience_pool_skill_label: u32,
    #[def("StatsExperiencePoolWillLabel")]
    pub stats_experience_pool_will_label: u32,
    #[def("StatsExperiencePoolPhysiqueLabel")]
    pub stats_experience_pool_physique_label: u32,
    #[def("StatsExperiencePoolHealthLabel")]
    pub stats_experience_pool_health_label: u32,
    #[def("StatsExperiencePoolToughnessLabel")]
    pub stats_experience_pool_toughness_label: u32,
    #[def("StatsExperiencePoolSpeedLabel")]
    pub stats_experience_pool_speed_label: u32,
    #[def("StatsExperiencePoolAccuracyLabel")]
    pub stats_experience_pool_accuracy_label: u32,
    #[def("StatsExperiencePoolStealthLabel")]
    pub stats_experience_pool_stealth_label: u32,
    #[def("StatsExperiencePoolWeaponMagicLabel")]
    pub stats_experience_pool_weapon_magic_label: u32,
    #[def("StatsExperiencePoolAbilityMagicLabel")]
    pub stats_experience_pool_ability_magic_label: u32,
    #[def("StatsExperiencePoolPureMagicLabel")]
    pub stats_experience_pool_pure_magic_label: u32,
    #[def("StatsExperienceStrengthBarValue")]
    pub stats_experience_strength_bar_value: f32,
    #[def("StatsExperienceSkillBarValue")]
    pub stats_experience_skill_bar_value: f32,
    #[def("StatsExperienceWillBarValue")]
    pub stats_experience_will_bar_value: f32,
    #[def("ExperienceGeneralPoolLabelTLPos")]
    pub experience_general_pool_label_tl_pos: Vector2D,
    #[def("ExperienceGeneralPoolLabelTextOffset")]
    pub experience_general_pool_label_text_offset: Vector2D,
    #[def("ExperienceGeneralPoolWidth")]
    pub experience_general_pool_width: f32,
    #[def("ExperienceGeneralPoolSliderWidth")]
    pub experience_general_pool_slider_width: f32,
    #[def("ExperienceGeneralPoolLabel")]
    pub experience_general_pool_label: u32,
    #[def("ExperienceGeneralPoolBarValue")]
    pub experience_general_pool_bar_value: u32,
    #[def("ExperienceGeneralPoolSpendTextOffset")]
    pub experience_general_pool_spend_text_offset: Vector2D,
    #[def("ExperienceStrengthPoolTLPos")]
    pub experience_strength_pool_tl_pos: Vector2D,
    #[def("ExperienceSkillPoolTLPos")]
    pub experience_skill_pool_tl_pos: Vector2D,
    #[def("ExperienceWillPoolTLPos")]
    pub experience_will_pool_tl_pos: Vector2D,
    #[def("ExperiencePoolWidth")]
    pub experience_pool_width: f32,
    #[def("ExperiencePoolHeight")]
    pub experience_pool_height: f32,
    #[def("ExperienceTypeTLPos")]
    pub experience_type_tl_pos: Vector2D,
    #[def("ExperienceTypeTLPosOffset")]
    pub experience_type_tl_pos_offset: Vector2D,
    #[def("ExperienceTypeTextOffset")]
    pub experience_type_text_offset: Vector2D,
    #[def("ExperienceTypeSpendTextOffset")]
    pub experience_type_spend_text_offset: Vector2D,
    #[def("ExperienceTypeLabelWidth")]
    pub experience_type_label_width: f32,
    #[def("ExperienceTypeLabelHeight")]
    pub experience_type_label_height: f32,
    #[def("ExperienceTypeSliderWidth")]
    pub experience_type_slider_width: f32,
    #[def("ExperienceTypeStrengthLabel")]
    pub experience_type_strength_label: u32,
    #[def("ExperienceTypeSkillLabel")]
    pub experience_type_skill_label: u32,
    #[def("ExperienceTypeWillLabel")]
    pub experience_type_will_label: u32,
    #[def("ExperienceTypePureMagicLabel")]
    pub experience_type_pure_magic_label: u32,
    #[def("ExperienceTypeAbilityMagicLabel")]
    pub experience_type_ability_magic_label: u32,
    #[def("ExperienceTypeWeaponMagicLabel")]
    pub experience_type_weapon_magic_label: u32,
    #[def("ExperienceStrengthPhysiqueLabel")]
    pub experience_strength_physique_label: u32,
    #[def("ExperienceStrengthHealthLabel")]
    pub experience_strength_health_label: u32,
    #[def("ExperienceStrengthToughnessLabel")]
    pub experience_strength_toughness_label: u32,
    #[def("ExperienceSkillSpeedLabel")]
    pub experience_skill_speed_label: u32,
    #[def("ExperienceSkillAccuracyLabel")]
    pub experience_skill_accuracy_label: u32,
    #[def("ExperienceSkillStealthLabel")]
    pub experience_skill_stealth_label: u32,
    #[def("ExperienceWeaponMagicBerserkLabel")]
    pub experience_weapon_magic_berserk_label: u32,
    #[def("ExperienceWeaponMagicFlamingBladeLabel")]
    pub experience_weapon_magic_flaming_blade_label: u32,
    #[def("ExperienceWeaponMagicDoubleStrikeLabel")]
    pub experience_weapon_magic_double_strike_label: u32,
    #[def("ExperienceWeaponMagicLightningBladeLabel")]
    pub experience_weapon_magic_lightning_blade_label: u32,
    #[def("ExperienceAbilityMagicBattleChargeLabel")]
    pub experience_ability_magic_battle_charge_label: u32,
    #[def("ExperienceAbilityMagicAssassinRushLabel")]
    pub experience_ability_magic_assassin_rush_label: u32,
    #[def("ExperienceAbilityMagicTimeSpellLabel")]
    pub experience_ability_magic_time_spell_label: u32,
    #[def("ExperiencePureMagicBurningHandsLabel")]
    pub experience_pure_magic_burning_hands_label: u32,
    #[def("ExperiencePureMagicPhysicalShieldLabel")]
    pub experience_pure_magic_physical_shield_label: u32,
    #[def("ExperiencePureMagicForcePushLabel")]
    pub experience_pure_magic_force_push_label: u32,
    #[def("ExperiencePureMagicEnflameSpellLabel")]
    pub experience_pure_magic_enflame_spell_label: u32,
    #[def("ExperiencePureMagicDrainLifeLabel")]
    pub experience_pure_magic_drain_life_label: u32,
    #[def("ExperiencePureMagicHealLifeLabel")]
    pub experience_pure_magic_heal_life_label: u32,
    #[def("ExperiencePureMagicGhostSwordLabel")]
    pub experience_pure_magic_ghost_sword_label: u32,
    #[def("ExperienceStrengthBarValue")]
    pub experience_strength_bar_value: f32,
    #[def("ExperienceSkillBarValue")]
    pub experience_skill_bar_value: f32,
    #[def("ExperienceWillBarValue")]
    pub experience_will_bar_value: f32,
    #[def("ExperienceStrengthPhysiqueBarValue")]
    pub experience_strength_physique_bar_value: f32,
    #[def("ExperienceStrengthHealthBarValue")]
    pub experience_strength_health_bar_value: f32,
    #[def("ExperienceStrengthToughnessBarValue")]
    pub experience_strength_toughness_bar_value: f32,
    #[def("ExperienceSkillSpeedBarValue")]
    pub experience_skill_speed_bar_value: f32,
    #[def("ExperienceSkillAccuracyBarValue")]
    pub experience_skill_accuracy_bar_value: f32,
    #[def("ExperienceSkillHealthBarValue")]
    pub experience_skill_health_bar_value: f32,
    #[def("ExperienceWeaponMagicBerserkBarValue")]
    pub experience_weapon_magic_berserk_bar_value: f32,
    #[def("ExperienceWeaponMagicFlamingBladeBarValue")]
    pub experience_weapon_magic_flaming_blade_bar_value: f32,
    #[def("ExperienceWeaponMagicDoubleStrikeBarValue")]
    pub experience_weapon_magic_double_strike_bar_value: f32,
    #[def("ExperienceAbilityMagicBattleChargeBarValue")]
    pub experience_ability_magic_battle_charge_bar_value: f32,
    #[def("ExperienceAbilityMagicAssassinRushBarValue")]
    pub experience_ability_magic_assassin_rush_bar_value: f32,
    #[def("ExperienceAbilityMagicTimeSpellBarValue")]
    pub experience_ability_magic_time_spell_bar_value: f32,
    #[def("ExperiencePureMagicBurningHandsBarValue")]
    pub experience_pure_magic_burning_hands_bar_value: f32,
    #[def("ExperiencePureMagicPhysicalShieldBarValue")]
    pub experience_pure_magic_physical_shield_bar_value: f32,
    #[def("ExperiencePureMagicForcePushBarValue")]
    pub experience_pure_magic_force_push_bar_value: f32,
    #[def("ExperienceProgBarL")]
    pub experience_prog_bar_l: EngineGraphic,
    #[def("ExperienceProgBarC")]
    pub experience_prog_bar_c: EngineGraphic,
    #[def("ExperienceProgBarR")]
    pub experience_prog_bar_r: EngineGraphic,
    #[def("ExperienceProgBarValue")]
    pub experience_prog_bar_value: EngineGraphic,
    #[def("ExperienceProgBarValueOffset")]
    pub experience_prog_bar_value_offset: Vector2D,
    #[def("ExperienceSpendTimeOffset")]
    pub experience_spend_time_offset: i32,
    #[def("ExperienceSpendRate")]
    pub experience_spend_rate: i32,
    #[def("ExperienceItemLabelStat")]
    pub experience_item_label_stat: Vec<i32>,
    #[def("ExperienceItemLabelAbility")]
    pub experience_item_label_ability: Vec<i32>,
    #[def("ExperienceSpendGreen")]
    pub experience_spend_green: EngineGraphic,
    #[def("ExperienceSpendGreenOutline")]
    pub experience_spend_green_outline: EngineGraphic,
    #[def("ExperienceSpendGreenEnd")]
    pub experience_spend_green_end: EngineGraphic,
    #[def("ExperienceSpendRed")]
    pub experience_spend_red: EngineGraphic,
    #[def("ExperienceSpendRedOutline")]
    pub experience_spend_red_outline: EngineGraphic,
    #[def("ExperienceSpendRedEnd")]
    pub experience_spend_red_end: EngineGraphic,
    #[def("ExperienceDescriptionTextTLPos")]
    pub experience_description_text_tl_pos: Vector2D,
    #[def("ExperienceDescriptionTextOffset")]
    pub experience_description_text_offset: Vector2D,
    #[def("QuestCardFeatColour")]
    pub quest_card_feat_colour: RGBColour,
    #[def("QuestCardCoreColour")]
    pub quest_card_core_colour: RGBColour,
    #[def("QuestCardExclusiveColour")]
    pub quest_card_exclusive_colour: RGBColour,
    #[def("QuestCardInfoTLPos")]
    pub quest_card_info_tl_pos: Vector2D,
    #[def("QuestCardInfo2TLPos")]
    pub quest_card_info2_tl_pos: Vector2D,
    #[def("QuestCardNoQuestsTLPos")]
    pub quest_card_no_quests_tl_pos: Vector2D,
    #[def("QuestCardNoQuestsText")]
    pub quest_card_no_quests_text: u32,
    #[def("QuestCardEdgedButtonTitleTLPos")]
    pub quest_card_edged_button_title_tl_pos: Vector2D,
    #[def("QuestCardEdgedButtonExtents")]
    pub quest_card_edged_button_extents: Vector2D,
    #[def("QuestCardInfoTextOffset")]
    pub quest_card_info_text_offset: Vector2D,
    #[def("QuestCardInfoQuestNameLabel")]
    pub quest_card_info_quest_name_label: u32,
    #[def("QuestCardInfoMoralityLabel")]
    pub quest_card_info_morality_label: u32,
    #[def("QuestCardInfoCoreLabel")]
    pub quest_card_info_core_label: u32,
    #[def("QuestCardInfoDetailsLabel")]
    pub quest_card_info_details_label: u32,
    #[def("QuestCardInfoRegionLabel")]
    pub quest_card_info_region_label: u32,
    #[def("QuestCardInfoOriginatorLabel")]
    pub quest_card_info_originator_label: u32,
    #[def("QuestCardInfoDescriptionLabel")]
    pub quest_card_info_description_label: u32,
    #[def("QuestCardInfoSuccessLabel")]
    pub quest_card_info_success_label: u32,
    #[def("QuestCardInfoFailureLabel")]
    pub quest_card_info_failure_label: u32,
    #[def("QuestCardInfoRewardsLabel")]
    pub quest_card_info_rewards_label: u32,
    #[def("QuestCardInfoRenownLabel")]
    pub quest_card_info_renown_label: u32,
    #[def("QuestCardInfoTrophyLabel")]
    pub quest_card_info_trophy_label: u32,
    #[def("QuestCardInfoBoastsLabel")]
    pub quest_card_info_boasts_label: u32,
    #[def("QuestCardInfoStatusLabel")]
    pub quest_card_info_status_label: u32,
    #[def("QuestCardInfoQuestNameText")]
    pub quest_card_info_quest_name_text: u32,
    #[def("QuestCardInfoMoralityText")]
    pub quest_card_info_morality_text: u32,
    #[def("QuestCardInfoCoreText")]
    pub quest_card_info_core_text: u32,
    #[def("QuestCardInfoDetailsText")]
    pub quest_card_info_details_text: u32,
    #[def("QuestCardInfoRegionText")]
    pub quest_card_info_region_text: u32,
    #[def("QuestCardInfoOriginatorText")]
    pub quest_card_info_originator_text: u32,
    #[def("QuestCardInfoDescriptionText")]
    pub quest_card_info_description_text: u32,
    #[def("QuestCardInfoSuccessText")]
    pub quest_card_info_success_text: u32,
    #[def("QuestCardInfoFailureText")]
    pub quest_card_info_failure_text: u32,
    #[def("QuestCardInfoRewardsText")]
    pub quest_card_info_rewards_text: u32,
    #[def("QuestCardInfoRenownText")]
    pub quest_card_info_renown_text: u32,
    #[def("QuestCardInfoTrophyText")]
    pub quest_card_info_trophy_text: u32,
    #[def("QuestCardInfoBoastsText")]
    pub quest_card_info_boasts_text: u32,
    #[def("QuestCardInfoStatusText")]
    pub quest_card_info_status_text: u32,
    #[def("TradeEdgedButtonCostTLPos")]
    pub trade_edged_button_cost_tl_pos: Vector2D,
    #[def("TradeEdgedButtonOwnedTLPos")]
    pub trade_edged_button_owned_tl_pos: Vector2D,
    #[def("TradeEdgedButtonProfitTLPos")]
    pub trade_edged_button_profit_tl_pos: Vector2D,
    #[def("TradeEdgedButtonCashTLPos")]
    pub trade_edged_button_cash_tl_pos: Vector2D,
    #[def("TradeEdgedButtonTitleTLPos")]
    pub trade_edged_button_title_tl_pos: Vector2D,
    #[def("TradeEdgedButtonExtents")]
    pub trade_edged_button_extents: Vector2D,
    #[def("TradeAnimImageBoxTLPos")]
    pub trade_anim_image_box_tl_pos: Vector2D,
    #[def("TradeAnimImageBoxExtents")]
    pub trade_anim_image_box_extents: Vector2D,
    #[def("TradeAnimImageXRotationTime")]
    pub trade_anim_image_x_rotation_time: f32,
    #[def("TradeAnimImageYRotationTime")]
    pub trade_anim_image_y_rotation_time: f32,
    #[def("TradeAnimImageZRotationTime")]
    pub trade_anim_image_z_rotation_time: f32,
    #[def("TradeEquippedItemTLPos")]
    pub trade_equipped_item_tl_pos: Vector2D,
    #[def("TradeEquippedItemExtents")]
    pub trade_equipped_item_extents: Vector2D,
    #[def("TradeEquippedItemGraphic")]
    pub trade_equipped_item_graphic: EngineGraphic,
    #[def("TradeDescriptionBoxTLPos")]
    pub trade_description_box_tl_pos: Vector2D,
    #[def("TradeDescriptionBoxExtents")]
    pub trade_description_box_extents: Vector2D,
    #[def("TradeTextOwned")]
    pub trade_text_owned: u32,
    #[def("TradeTextDivider")]
    pub trade_text_divider: u32,
    #[def("TradeTextCost")]
    pub trade_text_cost: u32,
    #[def("TradeTextGold")]
    pub trade_text_gold: u32,
    #[def("TradeTextProfit")]
    pub trade_text_profit: u32,
    #[def("TradeShadedCircleTLPos")]
    pub trade_shaded_circle_tl_pos: Vector2D,
    #[def("TradeShadedCircleExtents")]
    pub trade_shaded_circle_extents: Vector2D,
    #[def("ExpSpendItemLabelStat")]
    pub exp_spend_item_label_stat: Vec<i32>,
    #[def("ExpSpendItemLabelAbility")]
    pub exp_spend_item_label_ability: Vec<i32>,
    #[def("ExpSpendMaxItemsToDisplay")]
    pub exp_spend_max_items_to_display: i32,
    #[def("ExpSpendButtonTLPos")]
    pub exp_spend_button_tl_pos: Vector2D,
    #[def("ExpSpendButtonTLPosOffset")]
    pub exp_spend_button_tl_pos_offset: Vector2D,
    #[def("ExpSpendButtonTextOffset")]
    pub exp_spend_button_text_offset: Vector2D,
    #[def("ExpSpendButtonSelL")]
    pub exp_spend_button_sel_l: EngineGraphic,
    #[def("ExpSpendButtonSelC")]
    pub exp_spend_button_sel_c: EngineGraphic,
    #[def("ExpSpendButtonSelR")]
    pub exp_spend_button_sel_r: EngineGraphic,
    #[def("ExpSpendButtonL")]
    pub exp_spend_button_l: EngineGraphic,
    #[def("ExpSpendButtonC")]
    pub exp_spend_button_c: EngineGraphic,
    #[def("ExpSpendButtonR")]
    pub exp_spend_button_r: EngineGraphic,
    #[def("ExpSpendDescriptionBoxTLPos")]
    pub exp_spend_description_box_tl_pos: Vector2D,
    #[def("ExpSpendDescriptionBoxExtents")]
    pub exp_spend_description_box_extents: Vector2D,
    #[def("ExpSpendArrowUpTLPos")]
    pub exp_spend_arrow_up_tl_pos: Vector2D,
    #[def("ExpSpendArrowDnTLPos")]
    pub exp_spend_arrow_dn_tl_pos: Vector2D,
    #[def("ExpSpendGeneralPoolLabelTLPos")]
    pub exp_spend_general_pool_label_tl_pos: Vector2D,
    #[def("ExpSpendGeneralPoolLabelTextOffset")]
    pub exp_spend_general_pool_label_text_offset: Vector2D,
    #[def("ExpSpendGeneralPoolSpendTextOffset")]
    pub exp_spend_general_pool_spend_text_offset: Vector2D,
    #[def("ExpSpendGeneralPoolWidth")]
    pub exp_spend_general_pool_width: f32,
    #[def("ExpSpendGeneralPoolSliderWidth")]
    pub exp_spend_general_pool_slider_width: f32,
    #[def("ExpSpendStrengthPoolTLPos")]
    pub exp_spend_strength_pool_tl_pos: Vector2D,
    #[def("ExpSpendSkillPoolTLPos")]
    pub exp_spend_skill_pool_tl_pos: Vector2D,
    #[def("ExpSpendWillPoolTLPos")]
    pub exp_spend_will_pool_tl_pos: Vector2D,
    #[def("ExpSpendPoolWidth")]
    pub exp_spend_pool_width: f32,
    #[def("ExpSpendPoolHeight")]
    pub exp_spend_pool_height: f32,
    #[def("ExpSpendProgBarL")]
    pub exp_spend_prog_bar_l: EngineGraphic,
    #[def("ExpSpendProgBarC")]
    pub exp_spend_prog_bar_c: EngineGraphic,
    #[def("ExpSpendProgBarR")]
    pub exp_spend_prog_bar_r: EngineGraphic,
    #[def("ExpSpendProgBarValue")]
    pub exp_spend_prog_bar_value: EngineGraphic,
    #[def("ExpSpendProgBarValueOffset")]
    pub exp_spend_prog_bar_value_offset: Vector2D,
    #[def("ExpSpendTypeTextOffset")]
    pub exp_spend_type_text_offset: Vector2D,
    #[def("ExpSpendGeneralPoolBarValue")]
    pub exp_spend_general_pool_bar_value: i32,
    #[def("ExpSpendGeneralPoolLabel")]
    pub exp_spend_general_pool_label: i32,
    #[def("ExpSpendTypeStrengthLabel")]
    pub exp_spend_type_strength_label: i32,
    #[def("ExpSpendTypeSkillLabel")]
    pub exp_spend_type_skill_label: i32,
    #[def("ExpSpendTypeWillLabel")]
    pub exp_spend_type_will_label: i32,
    #[def("ExpSpendStrengthBarValue")]
    pub exp_spend_strength_bar_value: f32,
    #[def("ExpSpendSkillBarValue")]
    pub exp_spend_skill_bar_value: f32,
    #[def("ExpSpendWillBarValue")]
    pub exp_spend_will_bar_value: f32,
    #[def("ExpSpendRate")]
    pub exp_spend_rate: i32,
    #[def("ExpSpendTimeOffset")]
    pub exp_spend_time_offset: i32,
    #[def("ExpSpendGreen")]
    pub exp_spend_green: EngineGraphic,
    #[def("ExpSpendGreenOutline")]
    pub exp_spend_green_outline: EngineGraphic,
    #[def("ExpSpendGreenEnd")]
    pub exp_spend_green_end: EngineGraphic,
    #[def("ExpSpendRed")]
    pub exp_spend_red: EngineGraphic,
    #[def("ExpSpendRedOutline")]
    pub exp_spend_red_outline: EngineGraphic,
    #[def("ExpSpendRedEnd")]
    pub exp_spend_red_end: EngineGraphic,
    #[def("ExpSpendDescriptionTextTLPos")]
    pub exp_spend_description_text_tl_pos: Vector2D,
    #[def("ExpSpendDescriptionTextOffset")]
    pub exp_spend_description_text_offset: Vector2D,
    #[def("InventoryAnimImageXRotationTime")]
    pub inventory_anim_image_x_rotation_time: f32,
    #[def("InventoryAnimImageYRotationTime")]
    pub inventory_anim_image_y_rotation_time: f32,
    #[def("InventoryAnimImageZRotationTime")]
    pub inventory_anim_image_z_rotation_time: f32,
    #[def("AugmentationMeshAreaTLPos")]
    pub augmentation_mesh_area_tl_pos: Vector2D,
    #[def("AugmentationMeshAreaBRPos")]
    pub augmentation_mesh_area_br_pos: Vector2D,
    #[def("AugmentDescriptionBoxTLPos")]
    pub augment_description_box_tl_pos: Vector2D,
    #[def("AugmentDescriptionBoxExtents")]
    pub augment_description_box_extents: Vector2D,
    #[def("AugmentAugmentationInfoTLPos")]
    pub augment_augmentation_info_tl_pos: Vector2D,
    #[def("AugmentAugmentationLabelTLPos")]
    pub augment_augmentation_label_tl_pos: Vector2D,
    #[def("AugmentAugmentationInfoTextOffset")]
    pub augment_augmentation_info_text_offset: Vector2D,
    #[def("AugmentAugmentationSlotTLPos")]
    pub augment_augmentation_slot_tl_pos: Vector2D,
    #[def("AugmentAugmentationSlotTextOffset")]
    pub augment_augmentation_slot_text_offset: Vector2D,
    #[def("AugmentAugmentationWeaponTLPos")]
    pub augment_augmentation_weapon_tl_pos: Vector2D,
    #[def("AugmentAugmentationWeaponTextOffset")]
    pub augment_augmentation_weapon_text_offset: Vector2D,
    #[def("AugmentTextMaxSlots")]
    pub augment_text_max_slots: u32,
    #[def("AugmentTextNumSlots")]
    pub augment_text_num_slots: u32,
    #[def("AugmentTextUpgradeCost")]
    pub augment_text_upgrade_cost: u32,
    #[def("AugmentTextGold")]
    pub augment_text_gold: u32,
    #[def("AugmentTextDamageMultiplier")]
    pub augment_text_damage_multiplier: u32,
    #[def("AugmentTextFireStrength")]
    pub augment_text_fire_strength: u32,
    #[def("AugmentTextSlot")]
    pub augment_text_slot: u32,
    #[def("AugmentTextWeapons")]
    pub augment_text_weapons: u32,
    #[def("AugmentTextAugmentations")]
    pub augment_text_augmentations: u32,
    #[def("AugmentTextDivider")]
    pub augment_text_divider: u32,
    #[def("AugmentShadedCircleTLPos")]
    pub augment_shaded_circle_tl_pos: Vector2D,
    #[def("AugmentShadedCircleExtents")]
    pub augment_shaded_circle_extents: Vector2D,
    #[def("MainEdgedButtonTitleTLPos")]
    pub main_edged_button_title_tl_pos: Vector2D,
    #[def("MainEdgedButtonOwnedTLPos")]
    pub main_edged_button_owned_tl_pos: Vector2D,
    #[def("MainEdgedButtonPriceTLPos")]
    pub main_edged_button_price_tl_pos: Vector2D,
    #[def("MainEdgedButtonCostTLPos")]
    pub main_edged_button_cost_tl_pos: Vector2D,
    #[def("MainEdgedButtonExtents")]
    pub main_edged_button_extents: Vector2D,
    #[def("MainDescriptionBoxC")]
    pub main_description_box_c: EngineGraphic,
    #[def("MainDescriptionBoxTL")]
    pub main_description_box_tl: EngineGraphic,
    #[def("MainDescriptionBoxTR")]
    pub main_description_box_tr: EngineGraphic,
    #[def("MainDescriptionBoxBL")]
    pub main_description_box_bl: EngineGraphic,
    #[def("MainDescriptionBoxBR")]
    pub main_description_box_br: EngineGraphic,
    #[def("MainDescriptionBoxTLPos")]
    pub main_description_box_tl_pos: Vector2D,
    #[def("MainDescriptionBoxExtents")]
    pub main_description_box_extents: Vector2D,
    #[def("MainShadedCircleTLPos")]
    pub main_shaded_circle_tl_pos: Vector2D,
    #[def("MainShadedCircleExtents")]
    pub main_shaded_circle_extents: Vector2D,
    #[def("MainAnimBoxTLPos")]
    pub main_anim_box_tl_pos: Vector2D,
    #[def("MainAnimBoxExtents")]
    pub main_anim_box_extents: Vector2D,
    #[def("MainQuickAccessItemFlashTime")]
    pub main_quick_access_item_flash_time: f32,
}
