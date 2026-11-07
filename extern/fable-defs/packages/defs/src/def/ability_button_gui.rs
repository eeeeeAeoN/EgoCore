use crate::DefStruct;
use crate::enums::{ControllerType, MouseButtonControl, XboxControllerButton};
use crate::values::Vector2D;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AbilityButtonGuiDef {
    #[def("ControllerType")]
    pub controller_type: ControllerType,
    #[def("ControllerButton")]
    pub controller_button: XboxControllerButton,
    #[def("MouseButton")]
    pub mouse_button: MouseButtonControl,
    #[def("ButtonGraphic")]
    pub button_graphic: i32,
    #[def("Position")]
    pub position: Vector2D,
    #[def("Offset")]
    pub offset: Vector2D,
}
