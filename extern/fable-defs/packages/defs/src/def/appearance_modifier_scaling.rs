use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AppearanceModifierScalingDef {
    #[def("Attractiveness")]
    pub attractiveness: f32,
    #[def("Scariness")]
    pub scariness: f32,
    #[def("Goodstrength")]
    pub goodstrength: f32,
}
