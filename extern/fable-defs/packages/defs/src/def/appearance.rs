use crate::DefStruct;
use crate::enums::LightingChannel;
use crate::values::{AnimationSet, EngineGraphic};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AppearanceDef {
    #[def("Graphic")]
    pub graphic: EngineGraphic,
    #[def("Animation")]
    pub animation: AnimationSet,
    #[def("OutlineEffectAlphaRef", default = 128)]
    pub outline_effect_alpha_ref: i32,
    #[def("OutlineEffectAlphaBias")]
    pub outline_effect_alpha_bias: i32,
    #[def("OverrideLightingChannel")]
    pub override_lighting_channel: bool,
    #[def("LightingChannel")]
    pub lighting_channel: LightingChannel,
    #[def("FadeOutWhenCloseGraphic")]
    pub fade_out_when_close_graphic: EngineGraphic,
    #[def("HasFadeOutWhenCloseGraphic")]
    pub has_fade_out_when_close_graphic: bool,
    #[def("FadeOutGraphicRenderFadeDistance")]
    pub fade_out_graphic_render_fade_distance: f32,
    #[def("FadeOutGraphicNearRenderFadeDistance", default = 35.0)]
    pub fade_out_graphic_near_render_fade_distance: f32,
    #[def("FadeOutGraphicNearRenderClipDistance", default = 33.0)]
    pub fade_out_graphic_near_render_clip_distance: f32,
}
