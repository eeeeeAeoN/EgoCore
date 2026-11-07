use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FishDef {
    #[def("MinLength")]
    pub min_length: f32,
    #[def("MaxLength")]
    pub max_length: f32,
    #[def("MinWeight")]
    pub min_weight: f32,
    #[def("MaxWeight")]
    pub max_weight: f32,
    #[def("MinSpeed")]
    pub min_speed: f32,
    #[def("MaxSpeed")]
    pub max_speed: f32,
    #[def("BasePullSeconds")]
    pub base_pull_seconds: f32,
    #[def("MaxRandomPullSeconds")]
    pub max_random_pull_seconds: f32,
    #[def("BasePullWaitSeconds")]
    pub base_pull_wait_seconds: f32,
    #[def("MaxRandomPullWaitSeconds")]
    pub max_random_pull_wait_seconds: f32,
    #[def("Level")]
    pub level: i32,
    #[def("InfoText")]
    pub info_text: i32,
}
