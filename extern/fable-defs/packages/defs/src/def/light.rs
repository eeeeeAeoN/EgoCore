use crate::DefStruct;
use crate::def::{enums::PointLightChannelEffect, values::RGBColour};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct LightDef {
    #[def("Colour")]
    pub colour: RGBColour,
    #[def("HaloGraphic")]
    pub halo_graphic: i32,
    #[def("InnerRadius")]
    pub inner_radius: f32,
    #[def("OuterRadius")]
    pub outer_radius: f32,
    #[def("Flicker")]
    pub flicker: f32,
    #[def("FlickerSpeed", default = 0.3)]
    pub flicker_speed: f32,
    #[def("StartActive", default = true)]
    pub start_active: bool,
    #[def("SunlightAttenuator")]
    pub sunlight_attenuator: bool,
    #[def("DaylightAttenuator")]
    pub daylight_attenuator: bool,
    #[def("EffectChannel")]
    pub effect_channel: PointLightChannelEffect,
    #[def("Inverted")]
    pub inverted: bool,
    #[def("DaylightAttenuatorFadeInStart")]
    pub daylight_attenuator_fade_in_start: f32,
    #[def("DaylightAttenuatorFadeInEnd", default = 12.0)]
    pub daylight_attenuator_fade_in_end: f32,
    #[def("DaylightAttenuatorFadeOutStart", default = 12.0)]
    pub daylight_attenuator_fade_out_start: f32,
    #[def("DaylightAttenuatorFadeOutEnd")]
    pub daylight_attenuator_fade_out_end: f32,
}
