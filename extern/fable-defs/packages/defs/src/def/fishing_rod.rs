use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct FishingRodDef {
    #[def("StrainReductionPerSecond")]
    pub strain_reduction_per_second: f32,
    #[def("DistanceDecreasePerPress")]
    pub distance_decrease_per_press: f32,
    #[def("PullingDistanceDecreasePerPress")]
    pub pulling_distance_decrease_per_press: f32,
    #[def("MaxStrain")]
    pub max_strain: f32,
    #[def("MaxLengthMetres")]
    pub max_length_metres: f32,
}
