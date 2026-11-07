use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesForcePushDataDef {
    #[def("Damage")]
    pub damage: f32,
}
