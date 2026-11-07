use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CameraManagerSetDef {
    #[def("CameraManagerMain")]
    pub camera_manager_main: DefIndex,
    #[def("CameraManagerCombat")]
    pub camera_manager_combat: DefIndex,
    #[def("CameraManagerPCMain")]
    pub camera_manager_pc_main: DefIndex,
    #[def("CameraManagerPCCombat")]
    pub camera_manager_pc_combat: DefIndex,
}
