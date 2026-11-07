use crate::DefStruct;
use crate::enums::{QuakeLength, QuakeStrength};
use crate::values::ExplosionRing;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExplosionDef {
    #[def("Damage", default = 10.0)]
    pub damage: f32,
    #[def("ExplosionRadius", default = 10.0)]
    pub explosion_radius: f32,
    #[def("FireDamage", default = 10)]
    pub fire_damage: i32,
    #[def("ExplosionLifeTime", default = 1.5)]
    pub explosion_life_time: f32,
    #[def("DamageFalloffWithRadius", default = true)]
    pub damage_falloff_with_radius: bool,
    #[def("ExplosionBlendSmall", default = -1)]
    pub explosion_blend_small: i32,
    #[def("ExplosionBlendLarge", default = -1)]
    pub explosion_blend_large: i32,
    #[def("ExplosionReplacementBlendSmall", default = -1)]
    pub explosion_replacement_blend_small: i32,
    #[def("ExplosionReplacementBlendLarge", default = -1)]
    pub explosion_replacement_blend_large: i32,
    #[def("SecondsBeforeEffectReplacement")]
    pub seconds_before_effect_replacement: f32,
    #[def("ExplosionRadiusSmall")]
    pub explosion_radius_small: f32,
    #[def("ExplosionRadiusLarge")]
    pub explosion_radius_large: f32,
    #[def("ExplosionRings")]
    pub explosion_rings: Vec<ExplosionRing>,
    #[def("ExplosionShouldKnockdown", default = true)]
    pub explosion_should_knockdown: bool,
    #[def("CauseFireEffectOnHitCreatures")]
    pub cause_fire_effect_on_hit_creatures: bool,
    #[def("CausesRumble")]
    pub causes_rumble: bool,
    #[def("RumbleStrength")]
    pub rumble_strength: QuakeStrength,
    #[def("RumbleLength")]
    pub rumble_length: QuakeLength,
}
