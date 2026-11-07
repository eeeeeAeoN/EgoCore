use crate::DefStruct;
use std::collections::BTreeMap;

/// Locale-specific UI graphics mappings.  The `WorldMapRegionName` BTreeMap
/// keys are `WorldMapNameGraphic` selectors, but the **values** are
/// `UI_REGION_NAME_*` text ids — not `WorldMapNameGraphic` values — so the
/// map value type is `u32` (validated against retail NULLDEF value 4962).
#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct UILocaleGraphicsDef {
    #[def("WorldMapRegionName")]
    pub world_map_region_name: BTreeMap<u32, u32>,
    #[def("HelpScreenGraphics")]
    pub help_screen_graphics: Vec<u32>,
    #[def("HelpRingPic")]
    pub help_ring_pic: u32,
}
