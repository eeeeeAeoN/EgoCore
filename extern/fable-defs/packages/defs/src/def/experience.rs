use crate::DefStruct;
use crate::wire::VecMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExperienceDef {
    #[def("OrbCombiningParticleEffect")]
    pub orb_combining_particle_effect: i32,
    #[def("ParticleScalesPerExperiencePoints")]
    pub particle_scales_per_experience_points: VecMap<i32, f32>,
    #[def("PickupParticleEffectsPerExperiencePoints")]
    pub pickup_particle_effects_per_experience_points: VecMap<i32, i32>,
    #[def("LifeTimeForDistanceThresholds")]
    pub life_time_for_distance_thresholds: VecMap<i32, f32>,
}
