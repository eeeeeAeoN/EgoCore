use crate::DefStruct;
use crate::def::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SpecialAbilitiesLightningSpellDef {
    #[def("InitSound")]
    pub init_sound: DefString,
    #[def("LoopingSound")]
    pub looping_sound: DefString,
    #[def("EndHeroSound")]
    pub end_hero_sound: DefString,
    #[def("EndAttackeeSound")]
    pub end_attackee_sound: DefString,
    #[def("RandomBeamMinAngleX")]
    pub random_beam_min_angle_x: f32,
    #[def("RandomBeamMaxAngleX")]
    pub random_beam_max_angle_x: f32,
    #[def("RandomBeamMinAngleZ")]
    pub random_beam_min_angle_z: f32,
    #[def("RandomBeamMaxAngleZ")]
    pub random_beam_max_angle_z: f32,
    #[def("AreaEffect")]
    pub area_effect: Vec<f32>,
    #[def("DamagePerHit")]
    pub damage_per_hit: Vec<f32>,
    #[def("SubBeamDamagePerHit")]
    pub sub_beam_damage_per_hit: Vec<f32>,
    #[def("ActivationTime")]
    pub activation_time: Vec<f32>,
    #[def("DeActivationTime")]
    pub de_activation_time: Vec<f32>,
    #[def("MinMainBeams")]
    pub min_main_beams: Vec<i32>,
    #[def("MaxMainBeams")]
    pub max_main_beams: Vec<i32>,
    #[def("MinSubBeams")]
    pub min_sub_beams: Vec<i32>,
    #[def("MaxSubBeams")]
    pub max_sub_beams: Vec<i32>,
    #[def("TemporaryBeamLife")]
    pub temporary_beam_life: Vec<f32>,
    #[def("TargettingFOV")]
    pub targetting_fov: Vec<f32>,
    #[def("StaminaPerSecond")]
    pub stamina_per_second: Vec<f32>,
    #[def("MainBeamDecapitationTime")]
    pub main_beam_decapitation_time: Vec<f32>,
    #[def("SubBeamDecapitationTime")]
    pub sub_beam_decapitation_time: Vec<f32>,
    #[def("SubBeamDelay")]
    pub sub_beam_delay: Vec<f32>,
    #[def("OnhitEveryNSecs")]
    pub onhit_every_n_secs: Vec<f32>,
    #[def("CombatMultiplierIncreaseTime")]
    pub combat_multiplier_increase_time: f32,
    #[def("LightningThemeTransitionInSecs")]
    pub lightning_theme_transition_in_secs: f32,
    #[def("LightningThemeTransitionOutSecs")]
    pub lightning_theme_transition_out_secs: f32,
}
