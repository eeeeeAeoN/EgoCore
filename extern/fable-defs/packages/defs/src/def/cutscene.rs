use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CutsceneDef {
    #[def("Macro")]
    pub macro_: Vec<String>,
    #[def("SkipCond")]
    pub skip_cond: Vec<String>,
    #[def("SetupCond")]
    pub setup_cond: Vec<String>,
    #[def("Lights")]
    pub lights: Vec<String>,
    #[def("LightScene")]
    pub light_scene: Vec<String>,
    #[def("Sound")]
    pub sound: Vec<String>,
    #[def("Answer0")]
    pub answer0: Vec<String>,
    #[def("Answer1")]
    pub answer1: Vec<String>,
}
