use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExplodingObjectDef {
    #[def("TriggeredOnCreatureProximity")]
    pub triggered_on_creature_proximity: bool,
    #[def("TriggerRadius", default = 0.5)]
    pub trigger_radius: f32,
    #[def("ExplosionDef")]
    pub explosion_def: DefIndex,
    #[def("ProximityParticleEffect")]
    pub proximity_particle_effect: i32,
}
