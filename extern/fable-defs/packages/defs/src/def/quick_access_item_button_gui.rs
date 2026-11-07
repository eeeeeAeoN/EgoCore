use crate::DefStruct;
use crate::def::values::Vector2D;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct QuickAccessItemButtonGuiDef {
    #[def("ButtonGraphic")]
    pub button_graphic: i32,
    #[def("Position")]
    pub position: Vector2D,
    #[def("Offset")]
    pub offset: Vector2D,
}
