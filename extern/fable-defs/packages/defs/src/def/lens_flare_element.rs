use crate::DefStruct;
use crate::values::RGBColour;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct LensFlareElementDef {
    #[def("Radius")]
    pub radius: f32,
    #[def("Texture")]
    pub texture: i32,
    #[def("Position")]
    pub position: f32,
    #[def("Colour")]
    pub colour: RGBColour,
}
