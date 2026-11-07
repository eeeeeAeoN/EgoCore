use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CameraModeDef {
    #[def("CameraMode")]
    pub camera_mode: i32,
    #[def("StringLengthXY")]
    pub string_length_xy: f32,
    #[def("CageRadius")]
    pub cage_radius: f32,
    #[def("HeightOffsetZ")]
    pub height_offset_z: f32,
    #[def("LookOffsetZ")]
    pub look_offset_z: f32,
    #[def("FOV")]
    pub fov: f32,
    #[def("RollAngle")]
    pub roll_angle: f32,
    #[def("TransitionInFrames")]
    pub transition_in_frames: i32,
    #[def("LazyLookAtPosWeight")]
    pub lazy_look_at_pos_weight: f32,
    #[def("RaiseUpWhenClose", default = true)]
    pub raise_up_when_close: bool,
    #[def("StretchAroundHero")]
    pub stretch_around_hero: bool,
    #[def("ZTargetBaseOffset")]
    pub z_target_base_offset: f32,
    #[def("ZTargetLookAtTargetOffset")]
    pub z_target_look_at_target_offset: f32,
    #[def("ZTargetStringLengthToObservedThing")]
    pub z_target_string_length_to_observed_thing: f32,
    #[def("ZTargetStringLengthPerpendicularToObservedThing")]
    pub z_target_string_length_perpendicular_to_observed_thing: f32,
    #[def("PerformExtendedOcclusionTests")]
    pub perform_extended_occlusion_tests: bool,
    #[def("XAxisDisplacementCoupling")]
    pub x_axis_displacement_coupling: f32,
    #[def("XAxisVelocityDamping")]
    pub x_axis_velocity_damping: f32,
    #[def("XAxisAttractorCoupling")]
    pub x_axis_attractor_coupling: f32,
    #[def("XAxisAttractorFallOff")]
    pub x_axis_attractor_fall_off: f32,
    #[def("YAxisDisplacementCoupling")]
    pub y_axis_displacement_coupling: f32,
    #[def("YAxisVelocityDamping")]
    pub y_axis_velocity_damping: f32,
    #[def("YAxisAttractorCoupling")]
    pub y_axis_attractor_coupling: f32,
    #[def("YAxisAttractorFallOff")]
    pub y_axis_attractor_fall_off: f32,
    #[def("ProjectileCameraFOVZoom")]
    pub projectile_camera_fov_zoom: bool,
    #[def("ProjectileCameraZoomToggle")]
    pub projectile_camera_zoom_toggle: bool,
    #[def("ProjectileCameraDefaultTranslationalZoomOffset")]
    pub projectile_camera_default_translational_zoom_offset: f32,
    #[def("ProjectileCameraInitialPhiOffsetForCamera")]
    pub projectile_camera_initial_phi_offset_for_camera: f32,
    #[def("ProjectileCameraInitialHeightParamForCamera")]
    pub projectile_camera_initial_height_param_for_camera: f32,
    #[def("PhiRotationCouple")]
    pub phi_rotation_couple: f32,
    #[def("ThetaRotationCouple")]
    pub theta_rotation_couple: f32,
    #[def("DollyLength")]
    pub dolly_length: f32,
    #[def("NumberOfFramesToAverageInputsOver")]
    pub number_of_frames_to_average_inputs_over: i32,
    #[def("DollyLengthBlendResistance")]
    pub dolly_length_blend_resistance: f32,
    #[def("ThetaAngleLimitUp", default = 10.0)]
    pub theta_angle_limit_up: f32,
    #[def("ThetaAngleLimitDown", default = 40.0)]
    pub theta_angle_limit_down: f32,
    #[def("SecondsToReset", default = 0.5)]
    pub seconds_to_reset: f32,
    #[def("ViewHeroXYRotationMaxVelocity", default = 0.025)]
    pub view_hero_xy_rotation_max_velocity: f32,
    #[def("ViewHeroHeightParamMaxVelocity", default = 0.02)]
    pub view_hero_height_param_max_velocity: f32,
    #[def("ViewHeroXYOffsetMultiplier", default = 1.5)]
    pub view_hero_xy_offset_multiplier: f32,
    #[def("ViewHeroGroundHeightOffset", default = 0.5)]
    pub view_hero_ground_height_offset: f32,
}
