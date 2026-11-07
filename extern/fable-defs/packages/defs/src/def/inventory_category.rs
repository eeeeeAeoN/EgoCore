use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct InventoryCategoryDef {
    #[def("Inventory")]
    pub inventory: DefIndex,
    #[def("NumberOfSlots")]
    pub number_of_slots: i32,
    #[def("DrawItemSlots")]
    pub draw_item_slots: bool,
    #[def("SelectEmptySlots")]
    pub select_empty_slots: bool,
    #[def("WrapHighlightCursor")]
    pub wrap_highlight_cursor: bool,
    // `Transfer<unsigned long>` (tc_inventory_def.cpp:3047) — a numeric text-id
    // (`TEXT_GUI_*` from text.h), NOT a names.bin offset. Typed `u32` so the
    // differ compares the raw id instead of mis-resolving it as a DefString
    // offset against two different name tables (same fix as ItemDescription).
    #[def("CategoryName")]
    pub category_name: u32,
    #[def("AllowItemsToFillMoreThanOneSlot")]
    pub allow_items_to_fill_more_than_one_slot: bool,
    #[def("CategoryIdentifier")]
    pub category_identifier: i32,
    #[def("AddCategoryOnCreate", default = true)]
    pub add_category_on_create: bool,
}
