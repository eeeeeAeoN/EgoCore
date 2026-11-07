use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CoopSpiritDef {
    #[def("OnHitParticle")]
    pub on_hit_particle: DefString,
    #[def("OnStrikeParticle")]
    pub on_strike_particle: DefString,
    #[def("DefaultParticle")]
    pub default_particle: DefString,
    #[def("MeleeTargetRange")]
    pub melee_target_range: f32,
    #[def("AttackTargetParticle")]
    pub attack_target_particle: DefString,
}
