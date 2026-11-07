use crate::DefStruct;
use crate::def::{
    enums::ControlledMovementType, enums::CreatureInteractionType, enums::CreatureType,
    enums::NavigatorType, enums::Sex, enums::ThingCreatureProperty, values::EngineGraphic,
    values::ThingComponentSet, wire::DefIndex, wire::DefString,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThingCreatureDef {
    #[def("Components")]
    pub components: ThingComponentSet,
    #[def("AvailableInEditor", default = true)]
    pub available_in_editor: bool,
    #[def("GameStatePersisted")]
    pub game_state_persisted: bool,
    #[def("LevelLess")]
    pub level_less: bool,
    #[def("IsDrawable")]
    pub is_drawable: bool,
    #[def("AlwaysFrameUpdate", default = true)]
    pub always_frame_update: bool,
    #[def("InMapSearches", default = true)]
    pub in_map_searches: bool,
    #[def("StartKillLocked")]
    pub start_kill_locked: bool,
    #[def("IsLoadableGlobal")]
    pub is_loadable_global: bool,
    #[def("CanComeBetweenCameraAndObservedThing", default = true)]
    pub can_come_between_camera_and_observed_thing: bool,
    #[def("IsVulnerableToMelee")]
    pub is_vulnerable_to_melee: bool,
    #[def("Damageable")]
    pub damageable: bool,
    #[def("ForceNoSerialise")]
    pub force_no_serialise: bool,
    #[def("DrawWeaponTrails", default = true)]
    pub draw_weapon_trails: bool,
    #[def("DrawProjectileWeaponTrails", default = true)]
    pub draw_projectile_weapon_trails: bool,
    #[def("AlwaysIncludeInObjectStrikeScans")]
    pub always_include_in_object_strike_scans: bool,
    #[def("DisableOcclusionTesting")]
    pub disable_occlusion_testing: bool,
    #[def("DrawAfterWater")]
    pub draw_after_water: bool,
    #[def("ForceRenderedLastFrameOverride")]
    pub force_rendered_last_frame_override: bool,
    #[def("AddToComboMultiplierOnHit")]
    pub add_to_combo_multiplier_on_hit: bool,
    #[def("GiveHeroStatChangesOnBeingHit")]
    pub give_hero_stat_changes_on_being_hit: bool,
    #[def("GroupDef")]
    pub group_def: DefIndex,
    #[def("RenderFadeDistance", default = 64.0)]
    pub render_fade_distance: f32,
    #[def("PersistenceFlags", default = 1)] // EPF_STATIC (retail NULLDEF default)
    pub persistence_flags: i32,
    #[def("Health")]
    pub health: f32,
    #[def("MinHealth", default = -0.1)]
    pub min_health: f32,
    #[def("MeshHeightOffset")]
    pub mesh_height_offset: f32,
    #[def("MeshHeight", default = 2.0)]
    pub mesh_height: f32,
    #[def("MeshRadius", default = 1.0)]
    pub mesh_radius: f32,
    #[def("ApproxMaxMeshHeight", default = 2.0)]
    pub approx_max_mesh_height: f32,
    #[def("DefaultScriptName")]
    pub default_script_name: DefString,
    #[def("DefaultScriptData")]
    pub default_script_data: DefString,
    #[def("StunTimeMin")]
    pub stun_time_min: f32,
    #[def("StunTimeMax")]
    pub stun_time_max: f32,
    #[def("StunHealthThreshold")]
    pub stun_health_threshold: f32,
    #[def("DazedTimeMin")]
    pub dazed_time_min: f32,
    #[def("DazedTimeMax")]
    pub dazed_time_max: f32,
    #[def("DazedHealthThreshold")]
    pub dazed_health_threshold: i32,
    #[def("NumSecondsToPersistOnDeath", default = 30.0)]
    pub num_seconds_to_persist_on_death: f32,
    #[def("MaxSlowWalkingSpeed")]
    pub max_slow_walking_speed: f32,
    #[def("MaxWalkingSpeed")]
    pub max_walking_speed: f32,
    #[def("MaxJoggingSpeed")]
    pub max_jogging_speed: f32,
    #[def("MaxRunningSpeed")]
    pub max_running_speed: f32,
    #[def("MaxPushingSpeed")]
    pub max_pushing_speed: f32,
    #[def("MaxFlyingSpeed")]
    pub max_flying_speed: f32,
    #[def("MaxHoverSpeedSlow")]
    pub max_hover_speed_slow: f32,
    #[def("MaxHoverSpeedMedium")]
    pub max_hover_speed_medium: f32,
    #[def("MaxHoverSpeedFast")]
    pub max_hover_speed_fast: f32,
    #[def("SpeedRandomisationFactor")]
    pub speed_randomisation_factor: f32,
    #[def("MaxAcceleration")]
    pub max_acceleration: f32,
    #[def("HearingRadius")]
    pub hearing_radius: f32,
    #[def("FOV")]
    pub fov: f32,
    #[def("Sex")]
    pub sex: Sex,
    #[def("CreatureType")]
    pub creature_type: CreatureType,
    #[def("NavigatorTypes")]
    pub navigator_types: Vec<NavigatorType>,
    #[def("InitialWeaponDef", default = DefIndex(-1))]
    pub initial_weapon_def: DefIndex,
    #[def("PrimaryMeleeWeaponDef")]
    pub primary_melee_weapon_def: DefIndex,
    #[def("SecondaryMeleeWeaponDef")]
    pub secondary_melee_weapon_def: DefIndex,
    #[def("RangedWeaponDef", default = DefIndex(-1))]
    pub ranged_weapon_def: DefIndex,
    #[def("InitialSheathedWeapon")]
    pub initial_sheathed_weapon: DefIndex,
    #[def("PBrain")]
    pub p_brain: DefIndex,
    #[def("CreatureInteractionType")]
    pub creature_interaction_type: CreatureInteractionType,
    #[def("Property")]
    pub property: ThingCreatureProperty,
    #[def("Damage", default = 1.0)]
    pub damage: f32,
    #[def("DamageEffectDef")]
    pub damage_effect_def: i32,
    #[def("StunnedParticle")]
    pub stunned_particle: i32,
    #[def("Graphic")]
    pub graphic: EngineGraphic,
    #[def("EyeGraphic")]
    pub eye_graphic: EngineGraphic,
    #[def("DefaultOwner")]
    pub default_owner: i32,
    #[def("ControlledMovementType")]
    pub controlled_movement_type: ControlledMovementType,
    #[def("CombatTypeDef")]
    pub combat_type_def: DefIndex,
    #[def("ResetCombatTypeOnUnsheatheDefaultWeapons")]
    pub reset_combat_type_on_unsheathe_default_weapons: bool,
    #[def("HeroCombatDef")]
    pub hero_combat_def: DefIndex,
    #[def("HeroCombatPCDef")]
    pub hero_combat_pc_def: DefIndex,
    #[def("CombatDiameter")]
    pub combat_diameter: f32,
    #[def("DeferHitsIfBehindHitter", default = true)]
    pub defer_hits_if_behind_hitter: bool,
    #[def("LeaderTypeDef")]
    pub leader_type_def: DefIndex,
    #[def("RecoverEvadeHitsPercent", default = 75.0)]
    pub recover_evade_hits_percent: f32,
    #[def("DeadBodyDecayEffect")]
    pub dead_body_decay_effect: i32,
    #[def("LeaveDeadCreature")]
    pub leave_dead_creature: bool,
    #[def("AbleToStrafe")]
    pub able_to_strafe: bool,
    #[def("CreateDummyOnDeath")]
    pub create_dummy_on_death: bool,
    #[def("ImmuneToAttack")]
    pub immune_to_attack: bool,
    #[def("UseMeleeWeaponAsDefault", default = true)]
    pub use_melee_weapon_as_default: bool,
    #[def("UnsheatheWeaponsAutomatically", default = true)]
    pub unsheathe_weapons_automatically: bool,
    #[def("IsWillable")]
    pub is_willable: bool,
    #[def("IsIndestructible")]
    pub is_indestructible: bool,
    #[def("IsInvulnerableDuringGenericResponse")]
    pub is_invulnerable_during_generic_response: bool,
    #[def("IsPushedByExplosions", default = true)]
    pub is_pushed_by_explosions: bool,
}
