use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ActivateQuestDef {
    #[def("ScriptName")]
    pub script_name: DefString,
    #[def("LoadResources", default = true)]
    pub load_resources: bool,
}
