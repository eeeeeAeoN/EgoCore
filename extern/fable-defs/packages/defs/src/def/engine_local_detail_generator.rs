use crate::DefStruct;
use crate::def::EngineLocalDetailLayerDef;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct EngineLocalDetailGeneratorDef {
    #[def("Layers")]
    pub layers: Vec<EngineLocalDetailLayerDef>,
}
