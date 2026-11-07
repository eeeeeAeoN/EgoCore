use crate::DefStruct;
use crate::def::EnvironmentThemeDef;
use crate::values::RGBColour;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EnvironmentThemeDaySetDef {
    #[def("Time")]
    pub time: Vec<EnvironmentThemeDef>,
    #[def("SunTilt")]
    pub sun_tilt: f32,
    #[def("SunRotate")]
    pub sun_rotate: f32,
    #[def("SunHeight")]
    pub sun_height: f32,
    #[def("MoonTilt")]
    pub moon_tilt: f32,
    #[def("MoonRotate")]
    pub moon_rotate: f32,
    #[def("MoonHeight")]
    pub moon_height: f32,
    #[def("ColourLookupColumn")]
    pub colour_lookup_column: i32,
    #[def("EditorColour")]
    pub editor_colour: RGBColour,
    #[def("FishWeightMult", default = 1.0)]
    pub fish_weight_mult: f32,
}
