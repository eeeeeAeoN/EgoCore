use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct PhysicsDef {
    #[def("Diameter")]
    pub diameter: f32,
    #[def("Friction", default = 1.0)]
    pub friction: f32,
    #[def("Elasticity")]
    pub elasticity: f32,
    #[def("InteractionFlags", default = 22)]
    pub interaction_flags: i32,
    #[def("AirResistance")]
    pub air_resistance: f32,
    #[def("Mass", default = 50.0)]
    pub mass: f32,
    #[def("PushableByHero", default = true)]
    pub pushable_by_hero: bool,
}
