use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct GuildMasterDef {
    #[def("MinSecondsBetweenTeleport")]
    pub min_seconds_between_teleport: f32,
    #[def("MinSecondsBetweenWillUse")]
    pub min_seconds_between_will_use: f32,
}
