use crate::DefStruct;
use crate::def::MultiStaticMeshEntryDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MultiStaticMeshDef {
    #[def("Meshes")]
    pub meshes: Vec<MultiStaticMeshEntryDef>,
}
