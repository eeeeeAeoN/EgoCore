use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FireheartMinigameDef {
    #[def("MoonFX")]
    pub moon_fx: String,
    #[def("SunFX")]
    pub sun_fx: String,
    #[def("QuitText")]
    pub quit_text: u32,
    #[def("YesText")]
    pub yes_text: u32,
    #[def("NoText")]
    pub no_text: u32,
}
