use crate::DefStruct;
use crate::def::{values::BlendedParticleEffectSet, wire::VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MaterialDef {
    #[def("FirstHitSound")]
    pub first_hit_sound: i32,
    #[def("LastHitSound")]
    pub last_hit_sound: i32,
    #[def("OnHitRumbleLevel")]
    pub on_hit_rumble_level: f32,
    #[def("OnHitRumbleFalloff")]
    pub on_hit_rumble_falloff: f32,
    #[def("DeflectAllHits")]
    pub deflect_all_hits: bool,
    #[def("IsDiggable")]
    pub is_diggable: bool,
    #[def("SlideFootParticleEffect")]
    pub slide_foot_particle_effect: Vec<i32>,
    #[def("SwordHitParticleEffect")]
    pub sword_hit_particle_effect: Vec<i32>,
    #[def("KnockdownHitFloorParticleEffect")]
    pub knockdown_hit_floor_particle_effect: Vec<i32>,
    #[def("WindBlowDustEffect")]
    pub wind_blow_dust_effect: Vec<i32>,
    #[def("BerserkedSwordHitParticleEffect")]
    pub berserked_sword_hit_particle_effect: Vec<i32>,
    #[def("BlendedSwordHitEffects")]
    pub blended_sword_hit_effects: VecMap<i32, BlendedParticleEffectSet>,
}
