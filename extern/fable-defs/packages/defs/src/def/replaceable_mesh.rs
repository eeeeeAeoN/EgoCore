use crate::{DefStruct, WireStruct};

/// One entry of `NGraphicAppearance::CReplaceableMeshes` — original PC release layout.
/// Built by `Meshes.Add(<EEngineGraphicType>, <mesh>)`: `bank_index` = the mesh
/// (arg1), `graphic_type` (the trailing byte) = the graphic type (arg0), and
/// `anim_step`/`render_size_x` take the ctor default 1.0. Verified against retail.
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ReplaceableMeshesEntry {
    pub bank_index: i32,
    pub anim_step: f32,
    pub render_size_x: f32,
    pub additive_alpha: u8,
    pub graphic_type: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ReplaceableMeshes {
    pub vector: Vec<ReplaceableMeshesEntry>,
}

/// `CReplaceableMeshDef` — original PC release.
#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ReplaceableMeshDef {
    #[def("Meshes")]
    pub meshes: ReplaceableMeshes,
}
