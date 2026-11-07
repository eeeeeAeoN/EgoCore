use crate::DefStruct;
use crate::def::{
    DialogueLayerDef,
    values::EngineGraphic,
    wire::{DefIndex, VecMap},
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct GlobalDef {
    #[def("ControlSchemePC")]
    pub control_scheme_pc: DefIndex,
    #[def("ControlSchemePCAggressive")]
    pub control_scheme_pc_aggressive: DefIndex,
    #[def("ControlSchemeXBOX")]
    pub control_scheme_xbox: DefIndex,
    #[def("ControlSchemeXBOXAggressive")]
    pub control_scheme_xbox_aggressive: DefIndex,
    #[def("PhysicsTexture")]
    pub physics_texture: i32,
    #[def("InvalidThemeStandin")]
    pub invalid_theme_standin: DefIndex,
    #[def("DefaultEngineThemeDef")]
    pub default_engine_theme_def: DefIndex,
    #[def("StaticMapGenerationEstimatedMaxCameraHeightAboveLandscape")]
    pub static_map_generation_estimated_max_camera_height_above_landscape: f32,
    #[def("GraphicWeaponTrail")]
    pub graphic_weapon_trail: i32,
    #[def("GraphicWeaponTrail2")]
    pub graphic_weapon_trail2: i32,
    #[def("GraphicWeaponTrailArrow")]
    pub graphic_weapon_trail_arrow: i32,
    #[def("WeaponTrailAlpha")]
    pub weapon_trail_alpha: i32,
    #[def("WeaponTrailAlphaFadeOffTime")]
    pub weapon_trail_alpha_fade_off_time: f32,
    #[def("WeaponTrailArrowAlpha")]
    pub weapon_trail_arrow_alpha: i32,
    #[def("WeaponTrailArrowAlphaFadeOffTime")]
    pub weapon_trail_arrow_alpha_fade_off_time: f32,
    #[def("WeaponTrialNumFramesToFadeOut")]
    pub weapon_trial_num_frames_to_fade_out: i32,
    #[def("HealthPickupValue")]
    pub health_pickup_value: i32,
    #[def("RespawnDelay")]
    pub respawn_delay: i32,
    #[def("ScrabbleAngle")]
    pub scrabble_angle: i32,
    #[def("SlideAngle")]
    pub slide_angle: i32,
    #[def("MaxFollowers")]
    pub max_followers: i32,
    #[def("PickupGoldSoundCriteria")]
    pub pickup_gold_sound_criteria: String,
    #[def("PickupItemSoundCriteria")]
    pub pickup_item_sound_criteria: String,
    #[def("OnClickSoundCriteria")]
    pub on_click_sound_criteria: String,
    #[def("EditorSpangleCursor")]
    pub editor_spangle_cursor: i32,
    #[def("EditorCircleSprite")]
    pub editor_circle_sprite: i32,
    #[def("EditorSecondaryCircleSprite")]
    pub editor_secondary_circle_sprite: i32,
    #[def("TestSpeech")]
    pub test_speech: i32,
    #[def("MimeSpeechIndex")]
    pub mime_speech_index: String,
    #[def("AdultSleepTimeStart")]
    pub adult_sleep_time_start: i32,
    #[def("AdultSleepTimeEnd")]
    pub adult_sleep_time_end: i32,
    #[def("ChildSleepTimeStart")]
    pub child_sleep_time_start: i32,
    #[def("ChildSleepTimeEnd")]
    pub child_sleep_time_end: i32,
    #[def("ElderlySleepTimeStart")]
    pub elderly_sleep_time_start: i32,
    #[def("ElderlySleepTimeEnd")]
    pub elderly_sleep_time_end: i32,
    #[def("CreatureGeneratorRegenDelaySeconds")]
    pub creature_generator_regen_delay_seconds: f32,
    #[def("Music")]
    pub music: i32,
    #[def("AgeIncreasePerLevelUp")]
    pub age_increase_per_level_up: f32,
    #[def("E3BodgeFireflyGraphic")]
    pub e3_bodge_firefly_graphic: i32,
    #[def("SwitchGraphic")]
    pub switch_graphic: EngineGraphic,
    #[def("TrackGraphic")]
    pub track_graphic: EngineGraphic,
    #[def("ConversationCameras")]
    pub conversation_cameras: VecMap<i32, String>,
    #[def("PermittedAINarrators")]
    pub permitted_ai_narrators: Vec<String>,
    #[def("DialogueLayers")]
    pub dialogue_layers: Vec<DialogueLayerDef>,
    #[def("OpinionDebugIndicatorIcon")]
    pub opinion_debug_indicator_icon: i32,
    #[def("WifeLoveIndicatorIcon")]
    pub wife_love_indicator_icon: i32,
    #[def("WifeEngagedIndicatorIcon")]
    pub wife_engaged_indicator_icon: i32,
    #[def("WifeMarriedIndicatorIcon")]
    pub wife_married_indicator_icon: i32,
    #[def("ExpressionIndicatorIcon")]
    pub expression_indicator_icon: i32,
    #[def("FollowerIndicatorIcon")]
    pub follower_indicator_icon: i32,
    #[def("EmoteIconDrawDistance", default = 24.0)]
    pub emote_icon_draw_distance: f32,
    #[def("AngleToBeConsideredVertical")]
    pub angle_to_be_considered_vertical: f32,
    #[def("SoundGainLowerValueWhenInsideOutside")]
    pub sound_gain_lower_value_when_inside_outside: f32,
    #[def("TeleportOutParticleEffect")]
    pub teleport_out_particle_effect: i32,
    #[def("TeleportFadeOutTimeInSeconds")]
    pub teleport_fade_out_time_in_seconds: f32,
    #[def("TeleportInParticleEffect")]
    pub teleport_in_particle_effect: i32,
    #[def("TeleportFadeInTimeInSeconds")]
    pub teleport_fade_in_time_in_seconds: f32,
    #[def("DamageForMaximumBlood")]
    pub damage_for_maximum_blood: f32,
}
