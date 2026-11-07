use crate::DefStruct;
use crate::def::{values::RGBColour, wire::DefIndex};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PlayerDef {
    #[def("CharacterDef")]
    pub character_def: DefIndex,
    #[def("Colour")]
    pub colour: RGBColour,
}
