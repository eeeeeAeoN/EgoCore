use crate::DefStruct;
use crate::def::{
    enums::ReverbEnvironmentType, enums::WorldMapNameGraphic, values::SoundMap, wire::DefIndex,
    wire::DefString,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct RegionDef {
    #[def("EnvironmentTheme")]
    pub environment_theme: DefIndex,
    #[def("InternalEnvironmentTheme")]
    pub internal_environment_theme: DefIndex,
    #[def("InternalLightingChannelCount", default = 1)]
    pub internal_lighting_channel_count: i32,
    #[def("SoundMap")]
    pub sound_map: SoundMap,
    #[def("PreventHeroWeaponUse")]
    pub prevent_hero_weapon_use: bool,
    #[def("PreventHeroWillUse")]
    pub prevent_hero_will_use: bool,
    #[def("WanderingPopulationScriptDefName")]
    pub wandering_population_script_def_name: DefString,
    #[def("Music")]
    pub music: i32,
    #[def("ReverbEnvironmentType", default = ReverbEnvironmentType::EXTERNAL)]
    pub reverb_environment_type: ReverbEnvironmentType,
    #[def("ReverbEnvironmentLevel", default = 1.0)]
    pub reverb_environment_level: f32,
    #[def("InBuildingOcclusionLevel", default = 1.0)]
    pub in_building_occlusion_level: f32,
    #[def("UseVillageTime")]
    pub use_village_time: bool,
    #[def("AllowHenchmen", default = true)]
    pub allow_henchmen: bool,
    #[def("AttachToCameraFX")]
    pub attach_to_camera_fx: i32,
    #[def("EnableSpotlightShadowMode")]
    pub enable_spotlight_shadow_mode: bool,
    #[def("CheckAllThingsOnMapForCombatCollision")]
    pub check_all_things_on_map_for_combat_collision: bool,
    #[def("HeroBreathParticle")]
    pub hero_breath_particle: i32,
    #[def("MaxNumAttackersOnHero", default = -1)]
    pub max_num_attackers_on_hero: i32,
    #[def("CameraManagerSetIndex", default = DefIndex(-1))]
    pub camera_manager_set_index: DefIndex,
    #[def("CameraManagerSetBuildingIndex", default = DefIndex(-1))]
    pub camera_manager_set_building_index: DefIndex,
    #[def("RegionDescription")]
    pub region_description: i32,
    #[def("WeaponOutCrime")]
    pub weapon_out_crime: bool,
    #[def("UseCrimes")]
    pub use_crimes: bool,
    #[def("DisplayCrimes")]
    pub display_crimes: bool,
    #[def("PermitCrimes")]
    pub permit_crimes: bool,
    #[def("HasSea")]
    pub has_sea: bool,
    #[def("LockToFrameRate", default = 30)]
    pub lock_to_frame_rate: i32,
    #[def("WorldMapOffsetX")]
    pub world_map_offset_x: f32,
    #[def("WorldMapOffsetY")]
    pub world_map_offset_y: f32,
    #[def("NameGraphicOffsetX")]
    pub name_graphic_offset_x: f32,
    #[def("NameGraphicOffsetY")]
    pub name_graphic_offset_y: f32,
    #[def("NameGraphic")]
    pub name_graphic: WorldMapNameGraphic,
    #[def("FishWeightMult", default = 1.0)]
    pub fish_weight_mult: f32,
}
