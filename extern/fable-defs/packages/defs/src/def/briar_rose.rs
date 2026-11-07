use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct BriarRoseDef {
    #[def("SecondsToTeleport")]
    pub seconds_to_teleport: f32,
    #[def("SecondsToBeIntangible")]
    pub seconds_to_be_intangible: f32,
    #[def("SecondsToChargeDrainLife")]
    pub seconds_to_charge_drain_life: f32,
    #[def("GuranteedImpostersHitBeforeAllowedToHitBriar")]
    pub guranteed_imposters_hit_before_allowed_to_hit_briar: i32,
    #[def("NumberOfHitsToKillImposters")]
    pub number_of_hits_to_kill_imposters: i32,
    #[def("EnvironmentThemeForIntangibleMode")]
    pub environment_theme_for_intangible_mode: DefIndex,
    #[def("SecondsForIntangibleEnvironmentThemeTransitionIn")]
    pub seconds_for_intangible_environment_theme_transition_in: f32,
    #[def("SecondsForIntangibleEnvironmentThemeTransitionOut")]
    pub seconds_for_intangible_environment_theme_transition_out: f32,
    #[def("SecondsBetweenWillUse")]
    pub seconds_between_will_use: f32,
    #[def("SecondsBetweenTeleportAway")]
    pub seconds_between_teleport_away: f32,
    #[def("DistanceForcePushThreshold")]
    pub distance_force_push_threshold: f32,
}
