use crate::DefStruct;
use crate::def::{
    enums::ReverbEnvironmentType, values::EngineGraphic, values::ThingComponentSet, wire::DefIndex,
    wire::DefString,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThingBuildingDef {
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
    #[def("DieOffTimer")]
    pub die_off_timer: i32,
    #[def("SimBuildingDef")]
    pub sim_building_def: DefIndex,
    #[def("Graphic")]
    pub graphic: EngineGraphic,
    #[def("WallMaterial")]
    pub wall_material: DefIndex,
    #[def("FloorMaterial")]
    pub floor_material: DefIndex,
    #[def("CameraManagerSetIndex")]
    pub camera_manager_set_index: i32,
    #[def("ReverbEnvironmentType")]
    pub reverb_environment_type: ReverbEnvironmentType,
    #[def("ReverbEnvironmentLevel", default = 1.0)]
    pub reverb_environment_level: f32,
    #[def("OutsideOcclusionLevel", default = 1.0)]
    pub outside_occlusion_level: f32,
    #[def("UseHighDetailQuadTree")]
    pub use_high_detail_quad_tree: bool,
    #[def("IsSelfIlluminating")]
    pub is_self_illuminating: bool,
    #[def("IsPartOfHeroGuild")]
    pub is_part_of_hero_guild: bool,
    #[def("HasWeatherMask", default = true)]
    pub has_weather_mask: bool,
}
