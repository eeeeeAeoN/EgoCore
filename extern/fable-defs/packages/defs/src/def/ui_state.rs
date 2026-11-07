use crate::DefStruct;
use crate::enums::StateChangeType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct UiStateDef {
    #[def("GraphicIndex")]
    pub graphic_index: u32,
    #[def("PositionX")]
    pub position_x: f32,
    #[def("PositionY")]
    pub position_y: f32,
    #[def("ZoomX")]
    pub zoom_x: f32,
    #[def("ZoomY")]
    pub zoom_y: f32,
    #[def("ColourR")]
    pub colour_r: f32,
    #[def("ColourG")]
    pub colour_g: f32,
    #[def("ColourB")]
    pub colour_b: f32,
    #[def("ColourA")]
    pub colour_a: f32,
    #[def("UpdateTime")]
    pub update_time: f32,
    #[def("StateChangeType")]
    pub state_change_type: StateChangeType,
    #[def("LinearChange")]
    pub linear_change: bool,
    #[def("StateChangeFlag")]
    pub state_change_flag: u32,
    #[def("ChildrenNotAffected")]
    pub children_not_affected: Vec<i32>,
}
