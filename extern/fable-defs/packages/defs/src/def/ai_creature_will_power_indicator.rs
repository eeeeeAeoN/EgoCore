use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AICreatureWillPowerIndicatorDef {
    #[def("RechargeTime", default = 5.0)]
    pub recharge_time: f32,
}
