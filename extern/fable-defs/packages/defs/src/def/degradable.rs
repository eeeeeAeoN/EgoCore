use crate::enums::EngineGraphicType;
use crate::{DefStruct, WireStruct};

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct DegradableInfo {
    pub health_percentage: f32,
    pub bank_index: i32,
    pub anim_step: f32,
    // CEngineGraphic ctor default is 1.0 (matches `EngineGraphic.render_size_x`).
    #[def(default = 1.0)]
    pub render_size_x: f32,
    // The embedded CEngineGraphic serializes as a 14-byte block ending
    // `AdditiveAlpha(u8), Type(u8)` (tc_degradable_def.cpp): each degradation's
    // `Type` is set to the def's `GraphicType` byte (line 352), not left at 0.
    pub additive_alpha: u8,
    pub type_: u8,
    pub smash_particle_emitter: i32,
    pub blocks_navigation: bool,
    pub skip: [u8; 4],
}

/// `CDegradableDef` — original PC release.
#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DegradableDef {
    #[def("Degradable")]
    pub degradable: bool,
    #[def("GraphicType")]
    pub graphic_type: EngineGraphicType,
    #[def("InitiallyBlocksNavigation", default = true)]
    pub initially_blocks_navigation: bool,
    #[def("Degradations")]
    pub degradations: Vec<DegradableInfo>,
}
