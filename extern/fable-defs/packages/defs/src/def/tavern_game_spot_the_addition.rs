use crate::DefStruct;
use crate::def::{values::RGBColour, wire::DefIndex};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct TavernGameSpotTheAdditionDef {
    #[def("Pointer")]
    pub pointer: DefIndex,
    #[def("Cover")]
    pub cover: DefIndex,
    #[def("CoverWidth")]
    pub cover_width: f32,
    #[def("CoverLength")]
    pub cover_length: f32,
    #[def("Knicknacks")]
    pub knicknacks: Vec<DefIndex>,
    #[def("ItemsPerRound")]
    pub items_per_round: Vec<i32>,
    #[def("JoystickScale")]
    pub joystick_scale: f32,
    #[def("TimeToThink")]
    pub time_to_think: f32,
    #[def("MaxCursorHeight")]
    pub max_cursor_height: f32,
    #[def("KnickKnackRadius")]
    pub knick_knack_radius: f32,
    #[def("ImpulseScale")]
    pub impulse_scale: f32,
    #[def("PointerModel")]
    pub pointer_model: DefIndex,
    #[def("PointerHeightOffset")]
    pub pointer_height_offset: f32,
    #[def("TimeToMoveCover")]
    pub time_to_move_cover: f32,
    #[def("TimeToPauseCover")]
    pub time_to_pause_cover: f32,
    #[def("MaxCoverHeight")]
    pub max_cover_height: f32,
    #[def("MinimumKnickKnackGap")]
    pub minimum_knick_knack_gap: f32,
    #[def("LengthMovementScale")]
    pub length_movement_scale: f32,
    #[def("HeightMovementScale")]
    pub height_movement_scale: f32,
    #[def("HighlightWidth")]
    pub highlight_width: f32,
    #[def("HighlightColour")]
    pub highlight_colour: RGBColour,
    #[def("HighlightColourError")]
    pub highlight_colour_error: RGBColour,
}
