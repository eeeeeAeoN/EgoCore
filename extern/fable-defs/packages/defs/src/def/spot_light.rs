use crate::DefStruct;
use crate::def::values::RGBColour;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpotLightDef {
    #[def("Colour")]
    pub colour: RGBColour,
    #[def("InnerRadius")]
    pub inner_radius: f32,
    #[def("OuterRadius")]
    pub outer_radius: f32,
    #[def("Width")]
    pub width: f32,
    #[def("Flicker")]
    pub flicker: f32,
    #[def("FlickerSpeed", default = 0.3)]
    pub flicker_speed: f32,
    #[def("Angle")]
    pub angle: f32,
}
