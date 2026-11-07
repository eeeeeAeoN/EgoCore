use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FactionDef {
    #[def("EnemyFactions")]
    pub enemy_factions: Vec<String>,
    #[def("AlliedFactions")]
    pub allied_factions: Vec<String>,
}
