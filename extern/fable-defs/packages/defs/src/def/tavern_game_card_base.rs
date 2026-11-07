use crate::DefStruct;
use crate::def::{
    CardDef, CardPositionsDef, enums::TavernGameControlType, values::RGBColour, values::Vector2D,
    wire::DefIndex,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernGameCardBaseDef {
    #[def("ControlType")]
    pub control_type: TavernGameControlType,
    #[def("CardWidth")]
    pub card_width: f32,
    #[def("CardHeight")]
    pub card_height: f32,
    #[def("ImpulseScaleXbox")]
    pub impulse_scale_xbox: f32,
    #[def("ImpulseScalePC")]
    pub impulse_scale_pc: f32,
    #[def("PointerModel")]
    pub pointer_model: DefIndex,
    #[def("MoveSpeed")]
    pub move_speed: f32,
    #[def("TurnSpeed")]
    pub turn_speed: f32,
    #[def("PointerHeightOffset")]
    pub pointer_height_offset: f32,
    #[def("PointerParticleEffect")]
    pub pointer_particle_effect: DefIndex,
    #[def("HighlightWidth")]
    pub highlight_width: f32,
    #[def("HighlightColourGood")]
    pub highlight_colour_good: RGBColour,
    #[def("HighlightColourBad")]
    pub highlight_colour_bad: RGBColour,
    #[def("CardSeparation")]
    pub card_separation: f32,
    #[def("TableSeparation")]
    pub table_separation: f32,
    #[def("PackOffset")]
    pub pack_offset: Vector2D,
    #[def("CardOrders")]
    pub card_orders: Vec<CardPositionsDef>,
    #[def("CardPack")]
    pub card_pack: Vec<CardDef>,
}
