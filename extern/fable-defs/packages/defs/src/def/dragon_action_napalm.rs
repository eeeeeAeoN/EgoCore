use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct DragonActionNapalmDef {
    #[def("ChargeUpTime")]
    pub charge_up_time: f32,
    #[def("WindSpeed")]
    pub wind_speed: f32,
    #[def("TimeBeforeAddOfMouthEffect")]
    pub time_before_add_of_mouth_effect: f32,
    #[def("TimeBeforeAddExplosion")]
    pub time_before_add_explosion: f32,
}
