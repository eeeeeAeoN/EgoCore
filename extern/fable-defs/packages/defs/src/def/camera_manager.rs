use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CameraManagerDef {
    #[def("CameraList")]
    pub camera_list: Vec<DefIndex>,
    #[def("SpecialityCameraList")]
    pub speciality_camera_list: Vec<DefIndex>,
    #[def("InitialMode")]
    pub initial_mode: DefIndex,
    #[def("ZTargetMode")]
    pub z_target_mode: DefIndex,
    #[def("ProjectileWeaponMode")]
    pub projectile_weapon_mode: DefIndex,
    #[def("BaseHeightToKeepAboveFlatLandscape", default = 0.25)]
    pub base_height_to_keep_above_flat_landscape: f32,
    #[def("BaseHeightToKeepAboveWater", default = 1.25)]
    pub base_height_to_keep_above_water: f32,
    #[def("MaxHeightRelativeToGround", default = 10.0)]
    pub max_height_relative_to_ground: f32,
    #[def("MaxHeightStepForPointAcceptance", default = 3.0)]
    pub max_height_step_for_point_acceptance: f32,
    #[def(
        "AngleBetweenGroundNormalAndVerticalForLandscapeToBeConsideredNotFlat",
        default = 30.0
    )]
    pub angle_between_ground_normal_and_vertical_for_landscape_to_be_considered_not_flat: f32,
}
