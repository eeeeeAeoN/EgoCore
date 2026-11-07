use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesUnholyPowerDef {
    #[def("StartPowerupSound")]
    pub start_powerup_sound: Vec<DefString>,
    #[def("LoopingPowerupSound")]
    pub looping_powerup_sound: Vec<DefString>,
    #[def("ReleaseSound")]
    pub release_sound: Vec<DefString>,
    #[def("ReleaseLoopSound")]
    pub release_loop_sound: Vec<DefString>,
    #[def("FinishSound")]
    pub finish_sound: Vec<DefString>,
    #[def("CancelSound")]
    pub cancel_sound: Vec<DefString>,
    #[def("RandomFactorForNumSecsBetweenDamageHit", default = 1.0)]
    pub random_factor_for_num_secs_between_damage_hit: f32,
    #[def("NumSecsBetweenDamageHit")]
    pub num_secs_between_damage_hit: Vec<f32>,
    #[def("DamagePerHit")]
    pub damage_per_hit: Vec<f32>,
    #[def("DurationOfSpellSecs")]
    pub duration_of_spell_secs: Vec<f32>,
    #[def("RadiusOfEffect")]
    pub radius_of_effect: Vec<f32>,
    #[def("EnvChangeSecs")]
    pub env_change_secs: Vec<f32>,
    #[def("EnvReturnSecs")]
    pub env_return_secs: Vec<f32>,
    #[def("DoLifting")]
    pub do_lifting: bool,
    #[def("VelocityOfLift", default = 0.1)]
    pub velocity_of_lift: f32,
    #[def("DistanceToLift", default = 1.0)]
    pub distance_to_lift: f32,
    #[def("StaminaCostPerSec")]
    pub stamina_cost_per_sec: Vec<f32>,
    #[def("DamageTakenMultiplier")]
    pub damage_taken_multiplier: Vec<f32>,
    #[def("SmallRingRadius")]
    pub small_ring_radius: f32,
    #[def("LargeRingRadius")]
    pub large_ring_radius: f32,
}
