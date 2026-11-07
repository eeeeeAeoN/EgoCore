use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FrontEndDef {
    #[def("vAttractModeMovie")]
    pub v_attract_mode_movie: Vec<String>,
    #[def("ErrorMessageBackgroundGraphic")]
    pub error_message_background_graphic: i32,
    #[def("ButtonABigGraphic")]
    pub button_a_big_graphic: i32,
    #[def("ButtonBBigGraphic")]
    pub button_b_big_graphic: i32,
}
