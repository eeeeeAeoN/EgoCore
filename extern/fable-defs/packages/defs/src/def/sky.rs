use crate::DefStruct;
use crate::def::LensFlareElementDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SkyDef {
    #[def("SunRadius")]
    pub sun_radius: i32,
    #[def("SunTexture")]
    pub sun_texture: i32,
    #[def("SunFlareRadius")]
    pub sun_flare_radius: i32,
    #[def("SunFlareTexture")]
    pub sun_flare_texture: i32,
    #[def("MoonRadius")]
    pub moon_radius: i32,
    #[def("MoonTexture")]
    pub moon_texture: i32,
    #[def("StarTexture")]
    pub star_texture: i32,
    #[def("StarChartTextureSize")]
    pub star_chart_texture_size: i32,
    #[def("StarSize")]
    pub star_size: f32,
    #[def("StarChartFilter")]
    pub star_chart_filter: i32,
    #[def("TwinkleInterval")]
    pub twinkle_interval: i32,
    #[def("TwinkleSpeed")]
    pub twinkle_speed: f32,
    #[def("TwinkleMin")]
    pub twinkle_min: f32,
    #[def("TwinkleMax")]
    pub twinkle_max: f32,
    #[def("FlareElements")]
    pub flare_elements: Vec<LensFlareElementDef>,
}
