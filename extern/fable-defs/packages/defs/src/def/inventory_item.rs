use crate::DefStruct;
use crate::enums::{GameAction, TutorialCategory};
use crate::values::EngineGraphic;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct InventoryItemDef {
    #[def("Graphic")]
    pub graphic: EngineGraphic,
    // ItemDescription/ItemDetails are `Transfer<unsigned long>` (tc_inventory_def.cpp):
    // numeric text-ids (e.g. `TEXT_OBJECT_IRON_LONGSWORD_TITLE = 8323` from text.h),
    // NOT names.bin offsets. Typed `u32` so the differ compares the raw id instead of
    // mis-resolving it as a def-string offset against two different name tables.
    #[def("ItemDescription")]
    pub item_description: u32,
    #[def("ItemDetails")]
    pub item_details: u32,
    #[def("InventoryCategory")]
    pub inventory_category: DefIndex,
    #[def("MaxNumberItems", default = 1)]
    pub max_number_items: i32,
    #[def("SlotIndex", default = -1)]
    pub slot_index: i32,
    #[def("ActivationTime")]
    pub activation_time: f32,
    #[def("UseButtonAction")]
    pub use_button_action: GameAction,
    #[def("InventoryType", default = 17)]
    pub inventory_type: i32,
    #[def("Orientation")]
    pub orientation: i32,
    #[def("HeroAbilityDef")]
    pub hero_ability_def: DefIndex,
    #[def("IsSellable", default = true)]
    pub is_sellable: bool,
    #[def("IsBuyable", default = true)]
    pub is_buyable: bool,
    #[def("IsConfiscatable", default = true)]
    pub is_confiscatable: bool,
    #[def("DoNotPersistUntilQuestCompleted")]
    pub do_not_persist_until_quest_completed: bool,
    #[def("DoNotAutoPickUp")]
    pub do_not_auto_pick_up: bool,
    #[def("AutoPickUpAfterFirstPickUp")]
    pub auto_pick_up_after_first_pick_up: bool,
    #[def("ItemToSelectUponRemoval")]
    pub item_to_select_upon_removal: DefIndex,
    #[def("TutorialCategory")]
    pub tutorial_category: TutorialCategory,
    #[def("UIInventoryCategory", default = 4)]
    pub ui_inventory_category: i32,
}
