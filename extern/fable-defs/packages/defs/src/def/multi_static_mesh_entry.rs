use crate::DefStruct;
use crate::enums::LightingChannel;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MultiStaticMeshEntryDef {
    #[def("Mesh")]
    pub mesh: i32,
    #[def("OverrideLightingChannel")]
    pub override_lighting_channel: bool,
    #[def("OverrideRenderFadeDistance")]
    pub override_render_fade_distance: bool,
    #[def("RenderFadeDistance")]
    pub render_fade_distance: f32,
    #[def("LightingChannel")]
    pub lighting_channel: LightingChannel,
}
