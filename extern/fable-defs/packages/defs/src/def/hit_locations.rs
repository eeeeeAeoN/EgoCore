use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HitLocationsDef {
    #[def("HitLocations")]
    pub hit_locations: Vec<DefIndex>,
}
