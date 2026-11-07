use crate::DefStruct;
use crate::def::CreatureStatsDef;
use crate::enums::Opinion;
use crate::values::{ExpressionSet, FloatRange, RandomAppearanceMorph, WoundMorphs};
use crate::wire::DefIndex;
use crate::wire::{DefString, VecMap};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureDef {
    #[def("WoundMorphs")]
    pub wound_morphs: WoundMorphs,
    #[def("RandomAppearanceMorph")]
    pub random_appearance_morph: RandomAppearanceMorph,
    #[def("Expressions")]
    pub expressions: ExpressionSet,
    #[def("Stats")]
    pub stats: CreatureStatsDef,
    #[def("InitialActions")]
    pub initial_actions: Vec<i32>,
    #[def("InitialAppearanceModifiers")]
    pub initial_appearance_modifiers: Vec<DefIndex>,
    #[def("PhonemeAnim")]
    pub phoneme_anim: VecMap<String, i32>,
    #[def("DialogueVoices")]
    pub dialogue_voices: Vec<DefString>,
    #[def("Inventory")]
    pub inventory: DefIndex,
    #[def("ClothingInventory")]
    pub clothing_inventory: DefIndex,
    #[def("WeaponsInventory")]
    pub weapons_inventory: DefIndex,
    #[def("HeroAbilitiesScreenInventory")]
    pub hero_abilities_screen_inventory: DefIndex,
    #[def("QuestCardScreenInventory")]
    pub quest_card_screen_inventory: DefIndex,
    #[def("MapScreenInventory")]
    pub map_screen_inventory: DefIndex,
    #[def("MagicScreenInventory")]
    pub magic_screen_inventory: DefIndex,
    #[def("StatsScreenInventory")]
    pub stats_screen_inventory: DefIndex,
    #[def("ExperienceScreenInventory")]
    pub experience_screen_inventory: DefIndex,
    #[def("TradeScreenInventory")]
    pub trade_screen_inventory: DefIndex,
    #[def("AugmentScreenInventory")]
    pub augment_screen_inventory: DefIndex,
    #[def("QuestsScreenInventory")]
    pub quests_screen_inventory: DefIndex,
    #[def("InGameMenuScreen")]
    pub in_game_menu_screen: DefIndex,
    #[def("CreatureGroup")]
    pub creature_group: u32,
    #[def("BattleCrySound")]
    pub battle_cry_sound: DefString,
    #[def("OpinionSourceDef")]
    pub opinion_source_def: DefIndex,
    #[def("RespawnWaitInDaysMin")]
    pub respawn_wait_in_days_min: i32,
    #[def("RespawnWaitInDaysMax")]
    pub respawn_wait_in_days_max: i32,
    #[def("RespawnWaitInFramesMin")]
    pub respawn_wait_in_frames_min: i32,
    #[def("RespawnWaitInFramesMax")]
    pub respawn_wait_in_frames_max: i32,
    #[def("UseActualHitPosForHitEffects")]
    pub use_actual_hit_pos_for_hit_effects: bool,
    #[def("Short")]
    pub short: bool,
    #[def("FlashOnHit")]
    pub flash_on_hit: bool,
    #[def("PickPocketable")]
    pub pick_pocketable: bool,
    #[def("ChildThatCanRegionFollow")]
    pub child_that_can_region_follow: bool,
    #[def("AllowedInProtectedTowns", default = true)]
    pub allowed_in_protected_towns: bool,
    #[def("RadiusWeight")]
    pub radius_weight: VecMap<Opinion, f32>,
    #[def("RadiusSpread")]
    pub radius_spread: FloatRange,
    #[def("AGhostIsWhatIBe")]
    pub a_ghost_is_what_i_be: bool,
}
