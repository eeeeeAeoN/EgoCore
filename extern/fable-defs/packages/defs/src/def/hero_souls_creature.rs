use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroSoulsCreatureDef {
    #[def("CreatureType")]
    pub creature_type: String,
    #[def("Location")]
    pub location: String,
    #[def("NumCreatures")]
    pub num_creatures: i32,
}
