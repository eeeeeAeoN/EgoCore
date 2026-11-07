use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WillResponseDef {
    #[def("IsForcePushable", default = true)]
    pub is_force_pushable: bool,
    #[def("IsLifeDrainable", default = true)]
    pub is_life_drainable: bool,
    #[def("IsLightningable", default = true)]
    pub is_lightningable: bool,
    #[def("IsNotBloodyWillable")]
    pub is_not_bloody_willable: bool,
    #[def("IsAffectedBySlowTime", default = true)]
    pub is_affected_by_slow_time: bool,
    #[def("IsAbleToBeLifted", default = true)]
    pub is_able_to_be_lifted: bool,
    #[def("IsEpicPowerExplodable", default = true)]
    pub is_epic_power_explodable: bool,
    #[def("IsEpicSpellable", default = true)]
    pub is_epic_spellable: bool,
}
