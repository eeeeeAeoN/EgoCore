// Generated from packages/def-compiler/src/manifest.rs.
#ifndef FABLE_DEFS_COMPILER_MANIFEST_HPP
#define FABLE_DEFS_COMPILER_MANIFEST_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fable::defs::compiler {

enum class BinaryKind { Game, Frontend, Script };

struct BinaryHeaderInfo {
    bool use_names_bin;
    std::uint32_t file_indicator;
    std::uint32_t platform_indicator;
};

inline constexpr const char *FRONTEND_DEF_FILES[] = {
    "ui_dialogs.def",
    "FrontEndDefs/engine.def",
    "FrontEndDefs/engine_video_options.def",
    "FrontEndDefs/front_end.def",
    "FrontEndDefs/frontend_test.def",
    "FrontEndDefs/pc_frontend.def",
    "config_options_defaults.def",
    "controls.def",
    "pc_controls.def",
};
inline constexpr std::size_t FRONTEND_DEF_FILE_COUNT = 9;

inline constexpr const char *GAME_NULLDEF_ENTRIES[] = {
    "ARMOUR",
    "ATTACK_PATTERN",
    "BRAIN",
    "BUILDING",
    "CAICreatureWillPowerIndicatorDef",
    "CAIScratchpadDef",
    "CAMERA_MANAGER",
    "CAMERA_MANAGER_SET",
    "CAMERA_MODE",
    "CARRY_SLOT",
    "CAbilityDef",
    "CActionUseDef",
    "CActivateQuestDef",
    "CAnimatingObjectDef",
    "CAppearanceDef",
    "CAppearanceModifierDef",
    "CAreaOfEffectAttackDef",
    "CAugmentationDef",
    "CBalverineBattleDef",
    "CBedDef",
    "CBettingDef",
    "CBoastingPodiumDef",
    "CBonusItemDef",
    "CBossDef",
    "CBriarRoseDef",
    "CBuyHouseDef",
    "CBuyableHouseDef",
    "CCameraCollisionDef",
    "CCarriedReadableDef",
    "CCarryableDef",
    "CCarryingDef",
    "CChestDef",
    "CClockDef",
    "CCoinGameObstacleDef",
    "CCombatAbilityBlockCounterAttackDef",
    "CCombatAbilityBlockHeavyWeaponAttackDef",
    "CCombatAbilityBlockLightWeaponAttackDef",
    "CCombatAbilityBlockProjectileWeaponAttackDef",
    "CCombatAbilityBlockUnarmedAttackDef",
    "CCombatAbilityFlourishCounterAttackDef",
    "CCombatAbilityGetHitCounterAttackDef",
    "CCombatAbilityStrafeDef",
    "CCombatAbilityUseProjectileWeaponDef",
    "CContainerRewardHeroDef",
    "CContextSensitiveItemDef",
    "CCoopSpiritDef",
    "CCrateStackDef",
    "CCreatureDef",
    "CCreatureGeneratorDef",
    "CCreatureModeDef",
    "CCreatureNavigationDef",
    "CCreatureStatsDef",
    "CDecapitationDef",
    "CDegradableDef",
    "CDoorDef",
    "CDragonActionHoverDef",
    "CDragonActionNapalmDef",
    "CDragonActionSwoopDef",
    "CDrunkennessDef",
    "CEnemyDef",
    "CEntitySoundDef",
    "CExperienceDef",
    "CExplodingObjectDef",
    "CExplosionDef",
    "CExplosiveTrailDef",
    "CExpressionSubDef",
    "CFireballSpellLevelDef",
    "CFireheartMinigameDef",
    "CFishDef",
    "CFishingDef",
    "CFishingRodDef",
    "CFlammableDef",
    "CGiftDef",
    "CGoldDef",
    "CGuardDef",
    "CGuildMasterDef",
    "CHairCardDef",
    "CHasNameDef",
    "CHeroCentreDef",
    "CHeroDef",
    "CHeroExperienceDef",
    "CHeroMarriageDef",
    "CHeroMorphDef",
    "CHeroPostcardGeneratorDef",
    "CHeroSpecialMovementDef",
    "CHeroSuitDef",
    "CHeroTitleDef",
    "CHighlightItemDef",
    "CHitLocationsDef",
    "CIdleSchedulerDef",
    "CInterestingToVillagersDef",
    "CInventoryItemDef",
    "CJackDragonDef",
    "CJackOfBladesBattleDef",
    "CKickableDef",
    "CKrakenDef",
    "CKrakenTentacleDef",
    "CLightDef",
    "CLightningOrbDef",
    "CLookDef",
    "CMazeBattleDef",
    "CMultiStaticMeshDef",
    "CNymphDef",
    "COMBAT_DIALOGUE_DEF",
    "COMBAT_SEQUENCE",
    "COMBAT_TYPE",
    "CONFIG_OPTIONS_DEFAULTS_DEF",
    "CONTROL_SCHEME",
    "CObjectAugmentationsDef",
    "COccupiableDef",
    "COpinionOfHeroDef",
    "COracleMinigameDef",
    "COverheadDisplayDef",
    "CParticleAttacherDef",
    "CPerceivedThingDef",
    "CPhysicsDef",
    "CQuestCardDef",
    "CREATURE",
    "CREATURE_ABILITY",
    "CREATURE_GENERATION_FAMILY",
    "CReadableDef",
    "CReplaceableMeshDef",
    "CResurrectionItemDef",
    "CRumbleDef",
    "CScorpionKingBattleDef",
    "CShipDef",
    "CShopDef",
    "CShopItemDef",
    "CSkeletalMorphDef",
    "CSmashableDef",
    "CSmokeGeneratorDef",
    "CSnowTrollDef",
    "CSoundAtmospheresDef",
    "CSpecialAbilitiesDrainLifeDataDef",
    "CSpecialAbilitiesForcePushDataDef",
    "CSpecialEffectsDef",
    "CSpotLightDef",
    "CStealthDef",
    "CStockItemDef",
    "CSummonDef",
    "CSummonableCreatureDef",
    "CSummonerDef",
    "CTCVolumeContainmentTrackerDef",
    "CTargetingDef",
    "CTattooDef",
    "CTavernDef",
    "CTavernGameCardBaseDef",
    "CTavernGameCoinBaseDef",
    "CTavernGameCoinGolfDef",
    "CTavernGameDef",
    "CTavernGameShoveHaPennyDef",
    "CTavernGameSpotTheAdditionDef",
    "CTavernTableDef",
    "CTeleporterDef",
    "CTextureReplacementDef",
    "CThingDrainLifeShotDef",
    "CThingMultiArrowShotDef",
    "CThunderBattleDef",
    "CTimeAppearanceFadeDef",
    "CTrapDef",
    "CTrollBattleDef",
    "CTrophyDef",
    "CTurncoatDef",
    "CVillageDef",
    "CVillageMemberDef",
    "CVillagePeopleDef",
    "CWallMountEffectsDef",
    "CWaspQueenBattleDef",
    "CWeaponDef",
    "CWhisperBattleDef",
    "CWifeDef",
    "CWillResponseDef",
    "ENGINE",
    "ENGINE_THEME",
    "ENGINE_THEME_GROUP",
    "ENGINE_VIDEO_OPTIONS",
    "ENVIRONMENT",
    "ENVIRONMENT_THEME_DAY",
    "EXPRESSION",
    "FACTION",
    "GLOBAL",
    "HERO_ABILITY",
    "HERO_COMBAT",
    "HERO_MELEE_COMBAT_ABILITY",
    "HERO_STATS",
    "HIT_LOCATION",
    "HOLY_SITE",
    "INVENTORY_CATEGORY",
    "INVENTORY_ITEM",
    "INVENTORY_TYPE",
    "LIGHTNING",
    "LOCAL_DETAIL_GENERATOR",
    "MARKER",
    "MATERIAL",
    "MELEE_COMBAT_KNOCKDOWN_EFFECTS",
    "MESSAGE_EVENT",
    "NOISE",
    "OBJECT",
    "OBJECT_FAMILY",
    "OPINION_DEED_EFFECTS",
    "OPINION_DEED_MASK",
    "OPINION_PERSONALITY",
    "OPINION_REACTION_MANAGER",
    "OPINION_REACTION_MASK",
    "OPINION_SOURCE",
    "PHYSICAL_SWITCH",
    "PLAYER",
    "PLAYER_GUI",
    "PLAYER_INVENTORY",
    "PLAYER_MOVEMENT",
    "REGION",
    "SHOT",
    "SIM_BUILDING",
    "SIM_VOICES",
    "SKY",
    "SOUND_SETUP",
    "SOUND_THEME",
    "SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF",
    "SPECIAL_ABILITIES_BATTLE_CHARGE_DEF",
    "SPECIAL_ABILITIES_BERSERK_DEF",
    "SPECIAL_ABILITIES_BULLET_TIME_DEF",
    "SPECIAL_ABILITIES_BURNT_EFFECT_DEF",
    "SPECIAL_ABILITIES_CREATURE_TINT_DEF",
    "SPECIAL_ABILITIES_DIVINE_WRATH_DEF",
    "SPECIAL_ABILITIES_DRAIN_LIFE_DEF",
    "SPECIAL_ABILITIES_DRUNKENNESS_DEF",
    "SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF",
    "SPECIAL_ABILITIES_ENFLAME_DEF",
    "SPECIAL_ABILITIES_FIREBALL_SPELL_DEF",
    "SPECIAL_ABILITIES_FORCE_PUSH_DEF",
    "SPECIAL_ABILITIES_GHOST_SWORD_DEF",
    "SPECIAL_ABILITIES_HEAL_LIFE_DEF",
    "SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF",
    "SPECIAL_ABILITIES_MULTI_ARROW_DEF",
    "SPECIAL_ABILITIES_MULTI_STRIKE_DEF",
    "SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF",
    "SPECIAL_ABILITIES_SUMMON_SPELL_DEF",
    "SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF",
    "SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF",
    "SPECIAL_ABILITIES_UNHOLY_POWER_DEF",
    "SWITCH",
    "THING",
    "THING_GROUP",
    "UI",
    "UI_ICONS_DEF",
    "UI_LOCALE_GRAPHICS_DEF",
    "UI_MISC_THINGS_DEF",
    "VILLAGE",
    "VILLAGER_INTERACTION",
};
inline constexpr std::size_t GAME_NULLDEF_COUNT = 249;

inline constexpr const char *FRONTEND_NULLDEF_ENTRIES[] = {
    "CONFIG_OPTIONS_DEFAULTS_DEF",
    "CONTROL_SCHEME",
    "CONTROL_SCHEME",
    "ENGINE",
    "ENGINE_VIDEO_OPTIONS",
    "FRONT_END",
    "UI",
    "UI_ICONS_DEF",
    "UI_MISC_THINGS_DEF",
};
inline constexpr std::size_t FRONTEND_NULLDEF_COUNT = 9;

inline constexpr const char *SCRIPT_NULLDEF_ENTRIES[] = {
    "CCutsceneDef",
    "CRegionScriptDef",
    "CScriptDef",
};
inline constexpr std::size_t SCRIPT_NULLDEF_COUNT = 3;

inline constexpr BinaryHeaderInfo GAME_BINARY_HEADER = {
    false, 0xC69C21A6, 0xA8E36C34,
};

inline constexpr BinaryHeaderInfo FRONTEND_BINARY_HEADER = {
    false, 0xE86E4CDE, 0xA8E36C34,
};

inline constexpr BinaryHeaderInfo SCRIPT_BINARY_HEADER = {
    false, 0x35A0BC99, 0xA8E36C34,
};

inline constexpr std::array<std::uint8_t, 20> NAMES_HEADER_BYTES = {
    0x1E, 0xAB, 0x07, 0x00, 0x34, 0x6C, 0xE3, 0xA8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

inline bool has_subdef_table(std::string_view name) {
    if (name == "ARMOUR") return true;
    if (name == "ATTACK_PATTERN") return true;
    if (name == "BRAIN") return true;
    if (name == "BUILDING") return true;
    if (name == "CAMERA_MANAGER") return true;
    if (name == "CAMERA_MANAGER_SET") return true;
    if (name == "CAMERA_MODE") return true;
    if (name == "CAreaOfEffectAttackDef") return true;
    if (name == "CBalverineBattleDef") return true;
    if (name == "CHeroPostcardGeneratorDef") return true;
    if (name == "CIdleSchedulerDef") return true;
    if (name == "CJackOfBladesBattleDef") return true;
    if (name == "CMazeBattleDef") return true;
    if (name == "COMBAT_DIALOGUE_DEF") return true;
    if (name == "COMBAT_SEQUENCE") return true;
    if (name == "COMBAT_TYPE") return true;
    if (name == "CONFIG_OPTIONS_DEFAULTS_DEF") return true;
    if (name == "CONTROL_SCHEME") return true;
    if (name == "CREATURE") return true;
    if (name == "CREATURE_ABILITY") return true;
    if (name == "CREATURE_GENERATION_FAMILY") return true;
    if (name == "CScorpionKingBattleDef") return true;
    if (name == "CScriptDef") return true;
    if (name == "CCutsceneDef") return true;
    if (name == "CRegionScriptDef") return true;
    if (name == "CThunderBattleDef") return true;
    if (name == "CTrollBattleDef") return true;
    if (name == "CWaspQueenBattleDef") return true;
    if (name == "CWhisperBattleDef") return true;
    if (name == "ENGINE") return true;
    if (name == "ENGINE_THEME") return true;
    if (name == "ENGINE_THEME_GROUP") return true;
    if (name == "ENGINE_VIDEO_OPTIONS") return true;
    if (name == "ENVIRONMENT") return true;
    if (name == "ENVIRONMENT_THEME_DAY") return true;
    if (name == "EXPRESSION") return true;
    if (name == "FACTION") return true;
    if (name == "FRONT_END") return true;
    if (name == "GLOBAL") return true;
    if (name == "HERO_ABILITY") return true;
    if (name == "HERO_COMBAT") return true;
    if (name == "HERO_MELEE_COMBAT_ABILITY") return true;
    if (name == "HERO_STATS") return true;
    if (name == "HIT_LOCATION") return true;
    if (name == "HOLY_SITE") return true;
    if (name == "INVENTORY_CATEGORY") return true;
    if (name == "INVENTORY_TYPE") return true;
    if (name == "LIGHTNING") return true;
    if (name == "LOCAL_DETAIL_GENERATOR") return true;
    if (name == "MARKER") return true;
    if (name == "MATERIAL") return true;
    if (name == "MELEE_COMBAT_KNOCKDOWN_EFFECTS") return true;
    if (name == "MESSAGE_EVENT") return true;
    if (name == "NOISE") return true;
    if (name == "OBJECT") return true;
    if (name == "OBJECT_FAMILY") return true;
    if (name == "OPINION_DEED_EFFECTS") return true;
    if (name == "OPINION_DEED_MASK") return true;
    if (name == "OPINION_PERSONALITY") return true;
    if (name == "OPINION_REACTION_MANAGER") return true;
    if (name == "OPINION_REACTION_MASK") return true;
    if (name == "OPINION_SOURCE") return true;
    if (name == "PHYSICAL_SWITCH") return true;
    if (name == "PLAYER") return true;
    if (name == "PLAYER_GUI") return true;
    if (name == "PLAYER_INVENTORY") return true;
    if (name == "PLAYER_MOVEMENT") return true;
    if (name == "REGION") return true;
    if (name == "SHOT") return true;
    if (name == "SIM_BUILDING") return true;
    if (name == "SIM_VOICES") return true;
    if (name == "SKY") return true;
    if (name == "SOUND_SETUP") return true;
    if (name == "SOUND_THEME") return true;
    if (name == "SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF") return true;
    if (name == "SPECIAL_ABILITIES_BATTLE_CHARGE_DEF") return true;
    if (name == "SPECIAL_ABILITIES_BERSERK_DEF") return true;
    if (name == "SPECIAL_ABILITIES_BULLET_TIME_DEF") return true;
    if (name == "SPECIAL_ABILITIES_BURNT_EFFECT_DEF") return true;
    if (name == "SPECIAL_ABILITIES_CREATURE_TINT_DEF") return true;
    if (name == "SPECIAL_ABILITIES_DIVINE_WRATH_DEF") return true;
    if (name == "SPECIAL_ABILITIES_DRAIN_LIFE_DEF") return true;
    if (name == "SPECIAL_ABILITIES_DRUNKENNESS_DEF") return true;
    if (name == "SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF") return true;
    if (name == "SPECIAL_ABILITIES_ENFLAME_DEF") return true;
    if (name == "SPECIAL_ABILITIES_FIREBALL_SPELL_DEF") return true;
    if (name == "SPECIAL_ABILITIES_FORCE_PUSH_DEF") return true;
    if (name == "SPECIAL_ABILITIES_GHOST_SWORD_DEF") return true;
    if (name == "SPECIAL_ABILITIES_HEAL_LIFE_DEF") return true;
    if (name == "SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF") return true;
    if (name == "SPECIAL_ABILITIES_MULTI_ARROW_DEF") return true;
    if (name == "SPECIAL_ABILITIES_MULTI_STRIKE_DEF") return true;
    if (name == "SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF") return true;
    if (name == "SPECIAL_ABILITIES_SUMMON_SPELL_DEF") return true;
    if (name == "SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF") return true;
    if (name == "SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF") return true;
    if (name == "SPECIAL_ABILITIES_UNHOLY_POWER_DEF") return true;
    if (name == "SWITCH") return true;
    if (name == "THING") return true;
    if (name == "THING_GROUP") return true;
    if (name == "UI") return true;
    if (name == "UI_ICONS_DEF") return true;
    if (name == "UI_LOCALE_GRAPHICS_DEF") return true;
    if (name == "UI_MISC_THINGS_DEF") return true;
    if (name == "VILLAGE") return true;
    if (name == "VILLAGER_INTERACTION") return true;
    return false;
}

inline const char *const *null_entries(BinaryKind kind) {
    switch (kind) {
    case BinaryKind::Game: return GAME_NULLDEF_ENTRIES;
    case BinaryKind::Frontend: return FRONTEND_NULLDEF_ENTRIES;
    case BinaryKind::Script: return SCRIPT_NULLDEF_ENTRIES;
    }
    return GAME_NULLDEF_ENTRIES;
}

inline std::size_t null_entry_count(BinaryKind kind) {
    switch (kind) {
    case BinaryKind::Game: return GAME_NULLDEF_COUNT;
    case BinaryKind::Frontend: return FRONTEND_NULLDEF_COUNT;
    case BinaryKind::Script: return SCRIPT_NULLDEF_COUNT;
    }
    return GAME_NULLDEF_COUNT;
}

inline bool is_allowed_type(BinaryKind kind, std::string_view type) {
    const char *const *entries = null_entries(kind);
    for (std::size_t i = 0; i < null_entry_count(kind); ++i) {
        if (type == entries[i]) {
            return true;
        }
    }
    return false;
}

} // namespace fable::defs::compiler

#endif // FABLE_DEFS_COMPILER_MANIFEST_HPP
