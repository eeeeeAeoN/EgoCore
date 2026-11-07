use crate::DefStruct;
use crate::values::FireEffectCreationSet;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FlammableDef {
    #[def("FireResistance")]
    pub fire_resistance: i32,
    #[def("MaxDamageCaused", default = 1.0)]
    pub max_damage_caused: f32,
    #[def("FireDamagePerCycle")]
    pub fire_damage_per_cycle: f32,
    #[def("FireDamagePeriod")]
    pub fire_damage_period: f32,
    #[def("EffectCreationSet")]
    pub effect_creation_set: FireEffectCreationSet,
}
