use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BoastingPodiumDef {
    #[def("HeroOnPodiumRadius", default = 1.0)]
    pub hero_on_podium_radius: f32,
}
