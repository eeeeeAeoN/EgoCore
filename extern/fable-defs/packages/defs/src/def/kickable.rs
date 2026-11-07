use crate::DefStruct;
use crate::enums::OpinionDeedType;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct KickableDef {
    #[def("BaseDistanceToTravel")]
    pub base_distance_to_travel: f32,
    #[def("StrengthScaleRange")]
    pub strength_scale_range: f32,
    #[def("SpeedScaleRange")]
    pub speed_scale_range: f32,
    #[def("BaseSpeedScaleRange")]
    pub base_speed_scale_range: f32,
    #[def("RandomBase")]
    pub random_base: f32,
    #[def("RandomRange")]
    pub random_range: f32,
    #[def("ShotToAttach")]
    pub shot_to_attach: DefIndex,
    #[def("OpinionDeedType")]
    pub opinion_deed_type: OpinionDeedType,
    #[def("OpinionDeedFaction")]
    pub opinion_deed_faction: DefIndex,
    #[def("OpinionDeedTypeToEnemies")]
    pub opinion_deed_type_to_enemies: OpinionDeedType,
    #[def("IgnoreNavigationTest")]
    pub ignore_navigation_test: bool,
}
