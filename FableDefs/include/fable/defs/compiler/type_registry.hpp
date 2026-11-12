// Text def type -> C++ model class. Generated from the Rust DefBody table.
#ifndef FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP
#define FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP

#include <string>

#include <fable/defs.hpp>

namespace fable::defs::compiler {

template <class F>
bool with_class_for_type(const std::string &type, F &&f) {
    if (type == "ARMOUR") {
        f(::fable::defs::CArmourDef{});
        return true;
    }
    if (type == "ATTACK_PATTERN") {
        f(::fable::defs::CAttackPatternDef{});
        return true;
    }
    if (type == "BRAIN") {
        f(::fable::defs::CBrainDef{});
        return true;
    }
    if (type == "BUILDING") {
        f(::fable::defs::CThingBuildingDef{});
        return true;
    }
    if (type == "CAICreatureWillPowerIndicatorDef") {
        f(::fable::defs::CAICreatureWillPowerIndicatorDef{});
        return true;
    }
    if (type == "CAIScratchpadDef") {
        f(::fable::defs::CAIScratchpadDef{});
        return true;
    }
    if (type == "CAMERA_MANAGER") {
        f(::fable::defs::CCameraManagerDef{});
        return true;
    }
    if (type == "CAMERA_MANAGER_SET") {
        f(::fable::defs::CCameraManagerSetDef{});
        return true;
    }
    if (type == "CAMERA_MODE") {
        f(::fable::defs::CCameraModeDef{});
        return true;
    }
    if (type == "CARRY_SLOT") {
        f(::fable::defs::CCarrySlotDef{});
        return true;
    }
    if (type == "CAbilityDef") {
        f(::fable::defs::CAbilityDef{});
        return true;
    }
    if (type == "CActionUseDef") {
        f(::fable::defs::CActionUseDef{});
        return true;
    }
    if (type == "CActivateQuestDef") {
        f(::fable::defs::CActivateQuestDef{});
        return true;
    }
    if (type == "CAnimatingObjectDef") {
        f(::fable::defs::CAnimatingObjectDef{});
        return true;
    }
    if (type == "CAppearanceDef") {
        f(::fable::defs::CAppearanceDef{});
        return true;
    }
    if (type == "CAppearanceModifierDef") {
        f(::fable::defs::CAppearanceModifierDef{});
        return true;
    }
    if (type == "CAreaOfEffectAttackDef") {
        f(::fable::defs::CAreaOfEffectAttackDef{});
        return true;
    }
    if (type == "CAugmentationDef") {
        f(::fable::defs::CAugmentationDef{});
        return true;
    }
    if (type == "CBalverineBattleDef") {
        f(::fable::defs::CBalverineBattleDef{});
        return true;
    }
    if (type == "CBedDef") {
        f(::fable::defs::CBedDef{});
        return true;
    }
    if (type == "CBettingDef") {
        f(::fable::defs::CBettingDef{});
        return true;
    }
    if (type == "CBoastingPodiumDef") {
        f(::fable::defs::CBoastingPodiumDef{});
        return true;
    }
    if (type == "CBonusItemDef") {
        f(::fable::defs::CBonusItemDef{});
        return true;
    }
    if (type == "CBossDef") {
        f(::fable::defs::CBossDef{});
        return true;
    }
    if (type == "CBriarRoseDef") {
        f(::fable::defs::CBriarRoseDef{});
        return true;
    }
    if (type == "CBuyHouseDef") {
        f(::fable::defs::CBuyHouseDef{});
        return true;
    }
    if (type == "CBuyableHouseDef") {
        f(::fable::defs::CBuyableHouseDef{});
        return true;
    }
    if (type == "CCameraCollisionDef") {
        f(::fable::defs::CCameraCollisionDef{});
        return true;
    }
    if (type == "CCarriedReadableDef") {
        f(::fable::defs::CCarriedReadableDef{});
        return true;
    }
    if (type == "CCarryableDef") {
        f(::fable::defs::CCarryableDef{});
        return true;
    }
    if (type == "CCarryingDef") {
        f(::fable::defs::CCarryingDef{});
        return true;
    }
    if (type == "CChestDef") {
        f(::fable::defs::CChestDef{});
        return true;
    }
    if (type == "CClockDef") {
        f(::fable::defs::CClockDef{});
        return true;
    }
    if (type == "CCoinGameObstacleDef") {
        f(::fable::defs::CCoinGameObstacleDef{});
        return true;
    }
    if (type == "CCombatAbilityBlockCounterAttackDef") {
        f(::fable::defs::CCombatAbilityAttackBase{});
        return true;
    }
    if (type == "CCombatAbilityBlockHeavyWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockDefBase{});
        return true;
    }
    if (type == "CCombatAbilityBlockLightWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockDefBase{});
        return true;
    }
    if (type == "CCombatAbilityBlockProjectileWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockDefBase{});
        return true;
    }
    if (type == "CCombatAbilityBlockUnarmedAttackDef") {
        f(::fable::defs::CCombatAbilityBlockDefBase{});
        return true;
    }
    if (type == "CCombatAbilityFlourishCounterAttackDef") {
        f(::fable::defs::CCombatAbilityAttackBase{});
        return true;
    }
    if (type == "CCombatAbilityGetHitCounterAttackDef") {
        f(::fable::defs::CCombatAbilityAttackBase{});
        return true;
    }
    if (type == "CCombatAbilityStrafeDef") {
        f(::fable::defs::CCombatAbilityStrafeDef{});
        return true;
    }
    if (type == "CCombatAbilityUseProjectileWeaponDef") {
        f(::fable::defs::CCombatAbilityUseProjectileWeaponDef{});
        return true;
    }
    if (type == "CContainerRewardHeroDef") {
        f(::fable::defs::CContainerRewardHeroDef{});
        return true;
    }
    if (type == "CContextSensitiveItemDef") {
        f(::fable::defs::CContextSensitiveItemDef{});
        return true;
    }
    if (type == "CCoopSpiritDef") {
        f(::fable::defs::CCoopSpiritDef{});
        return true;
    }
    if (type == "CCrateStackDef") {
        f(::fable::defs::CCrateStackDef{});
        return true;
    }
    if (type == "CCreatureDef") {
        f(::fable::defs::CCreatureDef{});
        return true;
    }
    if (type == "CCreatureGeneratorDef") {
        f(::fable::defs::CCreatureGeneratorDef{});
        return true;
    }
    if (type == "CCreatureModeDef") {
        f(::fable::defs::CCreatureModeDef{});
        return true;
    }
    if (type == "CCreatureNavigationDef") {
        f(::fable::defs::CCreatureNavigationDef{});
        return true;
    }
    if (type == "CCreatureStatsDef") {
        f(::fable::defs::CCreatureStatsDef{});
        return true;
    }
    if (type == "CCutsceneDef") {
        f(::fable::defs::CCutsceneDef{});
        return true;
    }
    if (type == "CDecapitationDef") {
        f(::fable::defs::CDecapitationDef{});
        return true;
    }
    if (type == "CDegradableDef") {
        f(::fable::defs::CDegradableDef{});
        return true;
    }
    if (type == "CDoorDef") {
        f(::fable::defs::CDoorDef{});
        return true;
    }
    if (type == "CDragonActionHoverDef") {
        f(::fable::defs::CDragonActionHoverDef{});
        return true;
    }
    if (type == "CDragonActionNapalmDef") {
        f(::fable::defs::CDragonActionNapalmDef{});
        return true;
    }
    if (type == "CDragonActionSwoopDef") {
        f(::fable::defs::CDragonActionSwoopDef{});
        return true;
    }
    if (type == "CDrunkennessDef") {
        f(::fable::defs::CDrunkennessDef{});
        return true;
    }
    if (type == "CENVIRONMENT_DEF") {
        f(::fable::defs::CEnvironmentDef{});
        return true;
    }
    if (type == "CENVIRONMENT_THEME_DAY") {
        f(::fable::defs::CEnvironmentThemeDaySetDef{});
        return true;
    }
    if (type == "CEnemyDef") {
        f(::fable::defs::CEnemyDef{});
        return true;
    }
    if (type == "CEntitySoundDef") {
        f(::fable::defs::CEntitySoundDef{});
        return true;
    }
    if (type == "CExperienceDef") {
        f(::fable::defs::CExperienceDef{});
        return true;
    }
    if (type == "CExplodingObjectDef") {
        f(::fable::defs::CExplodingObjectDef{});
        return true;
    }
    if (type == "CExplosionDef") {
        f(::fable::defs::CExplosionDef{});
        return true;
    }
    if (type == "CExplosiveTrailDef") {
        f(::fable::defs::CExplosiveTrailDef{});
        return true;
    }
    if (type == "CExpressionSubDef") {
        f(::fable::defs::CExpressionSubDef{});
        return true;
    }
    if (type == "CFireballSpellLevelDef") {
        f(::fable::defs::CFireballSpellLevelDef{});
        return true;
    }
    if (type == "CFireheartMinigameDef") {
        f(::fable::defs::CFireheartMinigameDef{});
        return true;
    }
    if (type == "CFishDef") {
        f(::fable::defs::CFishDef{});
        return true;
    }
    if (type == "CFishingDef") {
        f(::fable::defs::CFishingDef{});
        return true;
    }
    if (type == "CFishingRodDef") {
        f(::fable::defs::CFishingRodDef{});
        return true;
    }
    if (type == "CFlammableDef") {
        f(::fable::defs::CFlammableDef{});
        return true;
    }
    if (type == "CGiftDef") {
        f(::fable::defs::CGiftDef{});
        return true;
    }
    if (type == "CGoldDef") {
        f(::fable::defs::CGoldDef{});
        return true;
    }
    if (type == "CGuardDef") {
        f(::fable::defs::CGuardDef{});
        return true;
    }
    if (type == "CGuildMasterDef") {
        f(::fable::defs::CGuildMasterDef{});
        return true;
    }
    if (type == "CHairCardDef") {
        f(::fable::defs::CHairCardDef{});
        return true;
    }
    if (type == "CHasNameDef") {
        f(::fable::defs::CHasNameDef{});
        return true;
    }
    if (type == "CHeroCentreDef") {
        f(::fable::defs::CHeroCentreDef{});
        return true;
    }
    if (type == "CHeroDef") {
        f(::fable::defs::CHeroDef{});
        return true;
    }
    if (type == "CHeroExperienceDef") {
        f(::fable::defs::CHeroExperienceDef{});
        return true;
    }
    if (type == "CHeroMarriageDef") {
        f(::fable::defs::CHeroMarriageDef{});
        return true;
    }
    if (type == "CHeroMorphDef") {
        f(::fable::defs::CHeroMorphDef{});
        return true;
    }
    if (type == "CHeroPostcardGeneratorDef") {
        f(::fable::defs::CHeroPostcardGeneratorDef{});
        return true;
    }
    if (type == "CHeroSpecialMovementDef") {
        f(::fable::defs::CHeroSpecialMovementDef{});
        return true;
    }
    if (type == "CHeroSuitDef") {
        f(::fable::defs::CHeroSuitDef{});
        return true;
    }
    if (type == "CHeroTitleDef") {
        f(::fable::defs::CHeroTitleDef{});
        return true;
    }
    if (type == "CHighlightItemDef") {
        f(::fable::defs::CHighlightItemDef{});
        return true;
    }
    if (type == "CHitLocationsDef") {
        f(::fable::defs::CHitLocationsDef{});
        return true;
    }
    if (type == "CIdleSchedulerDef") {
        f(::fable::defs::CIdleSchedulerDef{});
        return true;
    }
    if (type == "CInterestingToVillagersDef") {
        f(::fable::defs::CInterestingToVillagersDef{});
        return true;
    }
    if (type == "CInventoryItemDef") {
        f(::fable::defs::CInventoryItemDef{});
        return true;
    }
    if (type == "CJackDragonDef") {
        f(::fable::defs::CJackDragonDef{});
        return true;
    }
    if (type == "CJackOfBladesBattleDef") {
        f(::fable::defs::CJackOfBladesBattleDef{});
        return true;
    }
    if (type == "CKickableDef") {
        f(::fable::defs::CKickableDef{});
        return true;
    }
    if (type == "CKrakenDef") {
        f(::fable::defs::CKrakenDef{});
        return true;
    }
    if (type == "CKrakenTentacleDef") {
        f(::fable::defs::CKrakenTentacleDef{});
        return true;
    }
    if (type == "CLightDef") {
        f(::fable::defs::CLightDef{});
        return true;
    }
    if (type == "CLightningOrbDef") {
        f(::fable::defs::CLightningOrbDef{});
        return true;
    }
    if (type == "CLookDef") {
        f(::fable::defs::CLookDef{});
        return true;
    }
    if (type == "CMazeBattleDef") {
        f(::fable::defs::CMazeBattleDef{});
        return true;
    }
    if (type == "CMultiStaticMeshDef") {
        f(::fable::defs::CMultiStaticMeshDef{});
        return true;
    }
    if (type == "CNymphDef") {
        f(::fable::defs::CNymphDef{});
        return true;
    }
    if (type == "COMBAT_DIALOGUE_DEF") {
        f(::fable::defs::CCombatDialogueDef{});
        return true;
    }
    if (type == "COMBAT_SEQUENCE") {
        f(::fable::defs::CCombatSequenceDef{});
        return true;
    }
    if (type == "COMBAT_TYPE") {
        f(::fable::defs::CCombatTypeDef{});
        return true;
    }
    if (type == "CONFIG_OPTIONS_DEFAULTS_DEF") {
        f(::fable::defs::CConfigOptionsDefaultsDef{});
        return true;
    }
    if (type == "CONTROL_SCHEME") {
        f(::fable::defs::CControlsDef{});
        return true;
    }
    if (type == "CObjectAugmentationsDef") {
        f(::fable::defs::CObjectAugmentationsDef{});
        return true;
    }
    if (type == "COccupiableDef") {
        f(::fable::defs::COccupiableDef{});
        return true;
    }
    if (type == "COpinionOfHeroDef") {
        f(::fable::defs::COpinionOfHeroDef{});
        return true;
    }
    if (type == "COracleMinigameDef") {
        f(::fable::defs::COracleMinigameDef{});
        return true;
    }
    if (type == "COverheadDisplayDef") {
        f(::fable::defs::COverheadDisplayDef{});
        return true;
    }
    if (type == "CParticleAttacherDef") {
        f(::fable::defs::CParticleAttacherDef{});
        return true;
    }
    if (type == "CPerceivedThingDef") {
        f(::fable::defs::CPerceivedThingDef{});
        return true;
    }
    if (type == "CPhysicsDef") {
        f(::fable::defs::CPhysicsDef{});
        return true;
    }
    if (type == "CQuestCardDef") {
        f(::fable::defs::CQuestCardDef{});
        return true;
    }
    if (type == "CREATURE") {
        f(::fable::defs::CThingCreatureDef{});
        return true;
    }
    if (type == "CREATURE_ABILITY") {
        f(::fable::defs::CCreatureAbilityDef{});
        return true;
    }
    if (type == "CREATURE_GENERATION_FAMILY") {
        f(::fable::defs::CCreatureGenerationFamilyDef{});
        return true;
    }
    if (type == "CReadableDef") {
        f(::fable::defs::CReadableDef{});
        return true;
    }
    if (type == "CRegionScriptDef") {
        f(::fable::defs::CRegionScriptDef{});
        return true;
    }
    if (type == "CReplaceableMeshDef") {
        f(::fable::defs::CReplaceableMeshDef{});
        return true;
    }
    if (type == "CResurrectionItemDef") {
        f(::fable::defs::CResurrectionItemDef{});
        return true;
    }
    if (type == "CRumbleDef") {
        f(::fable::defs::CRumbleDef{});
        return true;
    }
    if (type == "CScorpionKingBattleDef") {
        f(::fable::defs::CScorpionKingBattleDef{});
        return true;
    }
    if (type == "CScriptDef") {
        f(::fable::defs::CScriptDef{});
        return true;
    }
    if (type == "CShipDef") {
        f(::fable::defs::CShipDef{});
        return true;
    }
    if (type == "CShopDef") {
        f(::fable::defs::CShopDef{});
        return true;
    }
    if (type == "CShopItemDef") {
        f(::fable::defs::CShopItemDef{});
        return true;
    }
    if (type == "CSkeletalMorphDef") {
        f(::fable::defs::CSkeletalMorphDef{});
        return true;
    }
    if (type == "CSmashableDef") {
        f(::fable::defs::CSmashableDef{});
        return true;
    }
    if (type == "CSmokeGeneratorDef") {
        f(::fable::defs::CSmokeGeneratorDef{});
        return true;
    }
    if (type == "CSnowTrollDef") {
        f(::fable::defs::CSnowTrollDef{});
        return true;
    }
    if (type == "CSoundAtmospheresDef") {
        f(::fable::defs::CSoundAtmospheresDef{});
        return true;
    }
    if (type == "CSpecialAbilitiesDrainLifeDataDef") {
        f(::fable::defs::CSpecialAbilitiesForcePushDataDef{});
        return true;
    }
    if (type == "CSpecialAbilitiesForcePushDataDef") {
        f(::fable::defs::CSpecialAbilitiesForcePushDataDef{});
        return true;
    }
    if (type == "CSpecialEffectsDef") {
        f(::fable::defs::CSpecialEffectsDef{});
        return true;
    }
    if (type == "CSpotLightDef") {
        f(::fable::defs::CSpotLightDef{});
        return true;
    }
    if (type == "CStealthDef") {
        f(::fable::defs::CStealthDef{});
        return true;
    }
    if (type == "CStockItemDef") {
        f(::fable::defs::CStockItemDef{});
        return true;
    }
    if (type == "CSummonDef") {
        f(::fable::defs::CSummonDef{});
        return true;
    }
    if (type == "CSummonableCreatureDef") {
        f(::fable::defs::CSummonableCreatureDef{});
        return true;
    }
    if (type == "CSummonerDef") {
        f(::fable::defs::CSummonerDef{});
        return true;
    }
    if (type == "CTCVolumeContainmentTrackerDef") {
        f(::fable::defs::CTCVolumeContainmentTrackerDef{});
        return true;
    }
    if (type == "CTargetingDef") {
        f(::fable::defs::CTargetingDef{});
        return true;
    }
    if (type == "CTattooDef") {
        f(::fable::defs::CTattooDef{});
        return true;
    }
    if (type == "CTavernDef") {
        f(::fable::defs::CTavernDef{});
        return true;
    }
    if (type == "CTavernGameCardBaseDef") {
        f(::fable::defs::CTavernGameCardBaseDef{});
        return true;
    }
    if (type == "CTavernGameCoinBaseDef") {
        f(::fable::defs::CTavernGameCoinBaseDef{});
        return true;
    }
    if (type == "CTavernGameCoinGolfDef") {
        f(::fable::defs::CTavernGameCoinGolfDef{});
        return true;
    }
    if (type == "CTavernGameDef") {
        f(::fable::defs::CTavernGameDef{});
        return true;
    }
    if (type == "CTavernGameShoveHaPennyDef") {
        f(::fable::defs::CTavernGameShoveHaPennyDef{});
        return true;
    }
    if (type == "CTavernGameSpotTheAdditionDef") {
        f(::fable::defs::CTavernGameSpotTheAdditionDef{});
        return true;
    }
    if (type == "CTavernTableDef") {
        f(::fable::defs::CTavernTableDef{});
        return true;
    }
    if (type == "CTeleporterDef") {
        f(::fable::defs::CTeleporterDef{});
        return true;
    }
    if (type == "CTextureReplacementDef") {
        f(::fable::defs::CTextureReplacementDef{});
        return true;
    }
    if (type == "CThingDrainLifeShotDef") {
        f(::fable::defs::CThingDrainLifeShotDef{});
        return true;
    }
    if (type == "CThingMultiArrowShotDef") {
        f(::fable::defs::CThingMultiArrowShotDef{});
        return true;
    }
    if (type == "CThunderBattleDef") {
        f(::fable::defs::CThunderBattleDef{});
        return true;
    }
    if (type == "CTimeAppearanceFadeDef") {
        f(::fable::defs::CTimeAppearanceFadeDef{});
        return true;
    }
    if (type == "CTrapDef") {
        f(::fable::defs::CTrapDef{});
        return true;
    }
    if (type == "CTrollBattleDef") {
        f(::fable::defs::CTrollBattleDef{});
        return true;
    }
    if (type == "CTrophyDef") {
        f(::fable::defs::CTrophyDef{});
        return true;
    }
    if (type == "CTurncoatDef") {
        f(::fable::defs::CTurncoatDef{});
        return true;
    }
    if (type == "CVillageDef") {
        f(::fable::defs::CVillageDef{});
        return true;
    }
    if (type == "CVillageMemberDef") {
        f(::fable::defs::CVillageMemberDef{});
        return true;
    }
    if (type == "CVillagePeopleDef") {
        f(::fable::defs::CVillagePeopleDef{});
        return true;
    }
    if (type == "CWallMountEffectsDef") {
        f(::fable::defs::CWallMountEffectsDef{});
        return true;
    }
    if (type == "CWaspQueenBattleDef") {
        f(::fable::defs::CWaspQueenBattleDef{});
        return true;
    }
    if (type == "CWeaponDef") {
        f(::fable::defs::CWeaponDef{});
        return true;
    }
    if (type == "CWhisperBattleDef") {
        f(::fable::defs::CWhisperBattleDef{});
        return true;
    }
    if (type == "CWifeDef") {
        f(::fable::defs::CWifeDef{});
        return true;
    }
    if (type == "CWillResponseDef") {
        f(::fable::defs::CWillResponseDef{});
        return true;
    }
    if (type == "ENGINE") {
        f(::fable::defs::CEngineDef{});
        return true;
    }
    if (type == "ENGINE_THEME") {
        f(::fable::defs::CEngineThemeDef{});
        return true;
    }
    if (type == "ENGINE_THEME_GROUP") {
        f(::fable::defs::CEngineThemeGroupDef{});
        return true;
    }
    if (type == "ENGINE_VIDEO_OPTIONS") {
        f(::fable::defs::CEngineVideoOptionsDef{});
        return true;
    }
    if (type == "ENVIRONMENT") {
        f(::fable::defs::CEnvironmentDef{});
        return true;
    }
    if (type == "ENVIRONMENT_THEME_DAY") {
        f(::fable::defs::CEnvironmentThemeDaySetDef{});
        return true;
    }
    if (type == "EXPRESSION") {
        f(::fable::defs::CExpressionDef{});
        return true;
    }
    if (type == "FACTION") {
        f(::fable::defs::CFactionDef{});
        return true;
    }
    if (type == "FRONT_END") {
        f(::fable::defs::CFrontEndDef{});
        return true;
    }
    if (type == "GLOBAL") {
        f(::fable::defs::CGlobalDef{});
        return true;
    }
    if (type == "HERO_ABILITY") {
        f(::fable::defs::CHeroAbilityDef{});
        return true;
    }
    if (type == "HERO_COMBAT") {
        f(::fable::defs::CHeroCombatDef{});
        return true;
    }
    if (type == "HERO_MELEE_COMBAT_ABILITY") {
        f(::fable::defs::CMeleeCombatAbilityDef{});
        return true;
    }
    if (type == "HERO_STATS") {
        f(::fable::defs::CHeroStatsDef{});
        return true;
    }
    if (type == "HIT_LOCATION") {
        f(::fable::defs::CHitLocationDef{});
        return true;
    }
    if (type == "HOLY_SITE") {
        f(::fable::defs::CThingHolySiteDef{});
        return true;
    }
    if (type == "INVENTORY_CATEGORY") {
        f(::fable::defs::CInventoryCategoryDef{});
        return true;
    }
    if (type == "INVENTORY_ITEM") {
        f(::fable::defs::CInventoryItemDef{});
        return true;
    }
    if (type == "INVENTORY_TYPE") {
        f(::fable::defs::CInventoryDef{});
        return true;
    }
    if (type == "LIGHTNING") {
        f(::fable::defs::CLightningDef{});
        return true;
    }
    if (type == "LOCAL_DETAIL_GENERATOR") {
        f(::fable::defs::CEngineLocalDetailGeneratorDef{});
        return true;
    }
    if (type == "MARKER") {
        f(::fable::defs::CThingMarkerDef{});
        return true;
    }
    if (type == "MATERIAL") {
        f(::fable::defs::CMaterialDef{});
        return true;
    }
    if (type == "MELEE_COMBAT_KNOCKDOWN_EFFECTS") {
        f(::fable::defs::CMeleeCombatKnockdownEffects{});
        return true;
    }
    if (type == "MESSAGE_EVENT") {
        f(::fable::defs::CMessageEventDef{});
        return true;
    }
    if (type == "NOISE") {
        f(::fable::defs::CThingNoiseDef{});
        return true;
    }
    if (type == "OBJECT") {
        f(::fable::defs::CThingObjectDef{});
        return true;
    }
    if (type == "OBJECT_FAMILY") {
        f(::fable::defs::CObjectFamilyDef{});
        return true;
    }
    if (type == "OPINION_DEED_EFFECTS") {
        f(::fable::defs::COpinionDeedEffectsDef{});
        return true;
    }
    if (type == "OPINION_DEED_MASK") {
        f(::fable::defs::COpinionDeedMaskDef{});
        return true;
    }
    if (type == "OPINION_PERSONALITY") {
        f(::fable::defs::COpinionPersonalityDef{});
        return true;
    }
    if (type == "OPINION_REACTION_MANAGER") {
        f(::fable::defs::COpinionReactionManagerDef{});
        return true;
    }
    if (type == "OPINION_REACTION_MASK") {
        f(::fable::defs::COpinionReactionMaskDef{});
        return true;
    }
    if (type == "OPINION_SOURCE") {
        f(::fable::defs::COpinionSourceDef{});
        return true;
    }
    if (type == "PHYSICAL_SWITCH") {
        f(::fable::defs::CThingPhysicalSwitchDef{});
        return true;
    }
    if (type == "PLAYER") {
        f(::fable::defs::CPlayerDef{});
        return true;
    }
    if (type == "PLAYER_GUI") {
        f(::fable::defs::CPlayerGuiDef{});
        return true;
    }
    if (type == "PLAYER_INVENTORY") {
        f(::fable::defs::CPlayerInventoryDef{});
        return true;
    }
    if (type == "PLAYER_MOVEMENT") {
        f(::fable::defs::CPlayerMovementDef{});
        return true;
    }
    if (type == "REGION") {
        f(::fable::defs::CRegionDef{});
        return true;
    }
    if (type == "SHOT") {
        f(::fable::defs::CThingShotDef{});
        return true;
    }
    if (type == "SIM_BUILDING") {
        f(::fable::defs::CSimBuildingDef{});
        return true;
    }
    if (type == "SIM_VOICES") {
        f(::fable::defs::CSimVoicesDef{});
        return true;
    }
    if (type == "SKY") {
        f(::fable::defs::CSkyDef{});
        return true;
    }
    if (type == "SOUND_SETUP") {
        f(::fable::defs::CSoundDef{});
        return true;
    }
    if (type == "SOUND_THEME") {
        f(::fable::defs::CSoundThemeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF") {
        f(::fable::defs::CSpecialAbilitiesAssassinRushDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_BATTLE_CHARGE_DEF") {
        f(::fable::defs::CSpecialAbilitiesBattleChargeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_BERSERK_DEF") {
        f(::fable::defs::CSpecialAbilitiesBerserkDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_BULLET_TIME_DEF") {
        f(::fable::defs::CSpecialAbilitiesBulletTimeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_BURNT_EFFECT_DEF") {
        f(::fable::defs::CSpecialAbilitiesBurntEffectDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_CREATURE_TINT_DEF") {
        f(::fable::defs::CSpecialAbilitiesCreatureTintDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_DIVINE_WRATH_DEF") {
        f(::fable::defs::CSpecialAbilitiesUnholyPowerDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_DRAIN_LIFE_DEF") {
        f(::fable::defs::CSpecialAbilitiesDrainLifeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_DRUNKENNESS_DEF") {
        f(::fable::defs::CSpecialAbilitiesDrunkennessDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF") {
        f(::fable::defs::CSpecialAbilitiesElectrocutedEffectDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_ENFLAME_DEF") {
        f(::fable::defs::CSpecialAbilitiesEnflameDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_FIREBALL_SPELL_DEF") {
        f(::fable::defs::CSpecialAbilitiesFireballSpellDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_FORCE_PUSH_DEF") {
        f(::fable::defs::CSpecialAbilitiesForcePushDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_GHOST_SWORD_DEF") {
        f(::fable::defs::CSpecialAbilitiesGhostSwordDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_HEAL_LIFE_DEF") {
        f(::fable::defs::CSpecialAbilitiesHealLifeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF") {
        f(::fable::defs::CSpecialAbilitiesLightningSpellDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_MULTI_ARROW_DEF") {
        f(::fable::defs::CSpecialAbilitiesMultiArrowDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_MULTI_STRIKE_DEF") {
        f(::fable::defs::CSpecialAbilitiesMultiStrikeDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF") {
        f(::fable::defs::CSpecialAbilitiesPhysicalShieldDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_SUMMON_SPELL_DEF") {
        f(::fable::defs::CSpecialAbilitiesSummonSpellDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF") {
        f(::fable::defs::CSpecialAbilitiesThunderLightningStormDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF") {
        f(::fable::defs::CSpecialAbilitiesTurncoatSpellDef{});
        return true;
    }
    if (type == "SPECIAL_ABILITIES_UNHOLY_POWER_DEF") {
        f(::fable::defs::CSpecialAbilitiesUnholyPowerDef{});
        return true;
    }
    if (type == "SWITCH") {
        f(::fable::defs::CThingSwitchDef{});
        return true;
    }
    if (type == "THING") {
        f(::fable::defs::CThingBaseDef{});
        return true;
    }
    if (type == "THING_GROUP") {
        f(::fable::defs::CEngineThemeGroupDef{});
        return true;
    }
    if (type == "UI") {
        f(::fable::defs::NUISystem::CUIDef{});
        return true;
    }
    if (type == "UI_ICONS_DEF") {
        f(::fable::defs::NUISystem::CUIIconsDef{});
        return true;
    }
    if (type == "UI_LOCALE_GRAPHICS_DEF") {
        f(::fable::defs::NUISystem::CUILocaleGraphicsDef{});
        return true;
    }
    if (type == "UI_MISC_THINGS_DEF") {
        f(::fable::defs::NUISystem::CUIMiscThingsDef{});
        return true;
    }
    if (type == "VILLAGE") {
        f(::fable::defs::CThingVillageDef{});
        return true;
    }
    if (type == "VILLAGER_INTERACTION") {
        f(::fable::defs::CVillagerInteractionsDef{});
        return true;
    }
    return false;
}

inline const char *class_name_for_type(const std::string &type) {
    if (type == "ARMOUR") return "CArmourDef";
    if (type == "ATTACK_PATTERN") return "CAttackPatternDef";
    if (type == "BRAIN") return "CBrainDef";
    if (type == "BUILDING") return "CThingBuildingDef";
    if (type == "CAICreatureWillPowerIndicatorDef") return "CAICreatureWillPowerIndicatorDef";
    if (type == "CAIScratchpadDef") return "CAIScratchpadDef";
    if (type == "CAMERA_MANAGER") return "CCameraManagerDef";
    if (type == "CAMERA_MANAGER_SET") return "CCameraManagerSetDef";
    if (type == "CAMERA_MODE") return "CCameraModeDef";
    if (type == "CARRY_SLOT") return "CCarrySlotDef";
    if (type == "CAbilityDef") return "CAbilityDef";
    if (type == "CActionUseDef") return "CActionUseDef";
    if (type == "CActivateQuestDef") return "CActivateQuestDef";
    if (type == "CAnimatingObjectDef") return "CAnimatingObjectDef";
    if (type == "CAppearanceDef") return "CAppearanceDef";
    if (type == "CAppearanceModifierDef") return "CAppearanceModifierDef";
    if (type == "CAreaOfEffectAttackDef") return "CAreaOfEffectAttackDef";
    if (type == "CAugmentationDef") return "CAugmentationDef";
    if (type == "CBalverineBattleDef") return "CBalverineBattleDef";
    if (type == "CBedDef") return "CBedDef";
    if (type == "CBettingDef") return "CBettingDef";
    if (type == "CBoastingPodiumDef") return "CBoastingPodiumDef";
    if (type == "CBonusItemDef") return "CBonusItemDef";
    if (type == "CBossDef") return "CBossDef";
    if (type == "CBriarRoseDef") return "CBriarRoseDef";
    if (type == "CBuyHouseDef") return "CBuyHouseDef";
    if (type == "CBuyableHouseDef") return "CBuyableHouseDef";
    if (type == "CCameraCollisionDef") return "CCameraCollisionDef";
    if (type == "CCarriedReadableDef") return "CCarriedReadableDef";
    if (type == "CCarryableDef") return "CCarryableDef";
    if (type == "CCarryingDef") return "CCarryingDef";
    if (type == "CChestDef") return "CChestDef";
    if (type == "CClockDef") return "CClockDef";
    if (type == "CCoinGameObstacleDef") return "CCoinGameObstacleDef";
    if (type == "CCombatAbilityBlockCounterAttackDef") return "CCombatAbilityAttackBase";
    if (type == "CCombatAbilityBlockHeavyWeaponAttackDef") return "CCombatAbilityBlockDefBase";
    if (type == "CCombatAbilityBlockLightWeaponAttackDef") return "CCombatAbilityBlockDefBase";
    if (type == "CCombatAbilityBlockProjectileWeaponAttackDef") return "CCombatAbilityBlockDefBase";
    if (type == "CCombatAbilityBlockUnarmedAttackDef") return "CCombatAbilityBlockDefBase";
    if (type == "CCombatAbilityFlourishCounterAttackDef") return "CCombatAbilityAttackBase";
    if (type == "CCombatAbilityGetHitCounterAttackDef") return "CCombatAbilityAttackBase";
    if (type == "CCombatAbilityStrafeDef") return "CCombatAbilityStrafeDef";
    if (type == "CCombatAbilityUseProjectileWeaponDef") return "CCombatAbilityUseProjectileWeaponDef";
    if (type == "CContainerRewardHeroDef") return "CContainerRewardHeroDef";
    if (type == "CContextSensitiveItemDef") return "CContextSensitiveItemDef";
    if (type == "CCoopSpiritDef") return "CCoopSpiritDef";
    if (type == "CCrateStackDef") return "CCrateStackDef";
    if (type == "CCreatureDef") return "CCreatureDef";
    if (type == "CCreatureGeneratorDef") return "CCreatureGeneratorDef";
    if (type == "CCreatureModeDef") return "CCreatureModeDef";
    if (type == "CCreatureNavigationDef") return "CCreatureNavigationDef";
    if (type == "CCreatureStatsDef") return "CCreatureStatsDef";
    if (type == "CCutsceneDef") return "CCutsceneDef";
    if (type == "CDecapitationDef") return "CDecapitationDef";
    if (type == "CDegradableDef") return "CDegradableDef";
    if (type == "CDoorDef") return "CDoorDef";
    if (type == "CDragonActionHoverDef") return "CDragonActionHoverDef";
    if (type == "CDragonActionNapalmDef") return "CDragonActionNapalmDef";
    if (type == "CDragonActionSwoopDef") return "CDragonActionSwoopDef";
    if (type == "CDrunkennessDef") return "CDrunkennessDef";
    if (type == "CENVIRONMENT_DEF") return "CEnvironmentDef";
    if (type == "CENVIRONMENT_THEME_DAY") return "CEnvironmentThemeDaySetDef";
    if (type == "CEnemyDef") return "CEnemyDef";
    if (type == "CEntitySoundDef") return "CEntitySoundDef";
    if (type == "CExperienceDef") return "CExperienceDef";
    if (type == "CExplodingObjectDef") return "CExplodingObjectDef";
    if (type == "CExplosionDef") return "CExplosionDef";
    if (type == "CExplosiveTrailDef") return "CExplosiveTrailDef";
    if (type == "CExpressionSubDef") return "CExpressionSubDef";
    if (type == "CFireballSpellLevelDef") return "CFireballSpellLevelDef";
    if (type == "CFireheartMinigameDef") return "CFireheartMinigameDef";
    if (type == "CFishDef") return "CFishDef";
    if (type == "CFishingDef") return "CFishingDef";
    if (type == "CFishingRodDef") return "CFishingRodDef";
    if (type == "CFlammableDef") return "CFlammableDef";
    if (type == "CGiftDef") return "CGiftDef";
    if (type == "CGoldDef") return "CGoldDef";
    if (type == "CGuardDef") return "CGuardDef";
    if (type == "CGuildMasterDef") return "CGuildMasterDef";
    if (type == "CHairCardDef") return "CHairCardDef";
    if (type == "CHasNameDef") return "CHasNameDef";
    if (type == "CHeroCentreDef") return "CHeroCentreDef";
    if (type == "CHeroDef") return "CHeroDef";
    if (type == "CHeroExperienceDef") return "CHeroExperienceDef";
    if (type == "CHeroMarriageDef") return "CHeroMarriageDef";
    if (type == "CHeroMorphDef") return "CHeroMorphDef";
    if (type == "CHeroPostcardGeneratorDef") return "CHeroPostcardGeneratorDef";
    if (type == "CHeroSpecialMovementDef") return "CHeroSpecialMovementDef";
    if (type == "CHeroSuitDef") return "CHeroSuitDef";
    if (type == "CHeroTitleDef") return "CHeroTitleDef";
    if (type == "CHighlightItemDef") return "CHighlightItemDef";
    if (type == "CHitLocationsDef") return "CHitLocationsDef";
    if (type == "CIdleSchedulerDef") return "CIdleSchedulerDef";
    if (type == "CInterestingToVillagersDef") return "CInterestingToVillagersDef";
    if (type == "CInventoryItemDef") return "CInventoryItemDef";
    if (type == "CJackDragonDef") return "CJackDragonDef";
    if (type == "CJackOfBladesBattleDef") return "CJackOfBladesBattleDef";
    if (type == "CKickableDef") return "CKickableDef";
    if (type == "CKrakenDef") return "CKrakenDef";
    if (type == "CKrakenTentacleDef") return "CKrakenTentacleDef";
    if (type == "CLightDef") return "CLightDef";
    if (type == "CLightningOrbDef") return "CLightningOrbDef";
    if (type == "CLookDef") return "CLookDef";
    if (type == "CMazeBattleDef") return "CMazeBattleDef";
    if (type == "CMultiStaticMeshDef") return "CMultiStaticMeshDef";
    if (type == "CNymphDef") return "CNymphDef";
    if (type == "COMBAT_DIALOGUE_DEF") return "CCombatDialogueDef";
    if (type == "COMBAT_SEQUENCE") return "CCombatSequenceDef";
    if (type == "COMBAT_TYPE") return "CCombatTypeDef";
    if (type == "CONFIG_OPTIONS_DEFAULTS_DEF") return "CConfigOptionsDefaultsDef";
    if (type == "CONTROL_SCHEME") return "CControlsDef";
    if (type == "CObjectAugmentationsDef") return "CObjectAugmentationsDef";
    if (type == "COccupiableDef") return "COccupiableDef";
    if (type == "COpinionOfHeroDef") return "COpinionOfHeroDef";
    if (type == "COracleMinigameDef") return "COracleMinigameDef";
    if (type == "COverheadDisplayDef") return "COverheadDisplayDef";
    if (type == "CParticleAttacherDef") return "CParticleAttacherDef";
    if (type == "CPerceivedThingDef") return "CPerceivedThingDef";
    if (type == "CPhysicsDef") return "CPhysicsDef";
    if (type == "CQuestCardDef") return "CQuestCardDef";
    if (type == "CREATURE") return "CThingCreatureDef";
    if (type == "CREATURE_ABILITY") return "CCreatureAbilityDef";
    if (type == "CREATURE_GENERATION_FAMILY") return "CCreatureGenerationFamilyDef";
    if (type == "CReadableDef") return "CReadableDef";
    if (type == "CRegionScriptDef") return "CRegionScriptDef";
    if (type == "CReplaceableMeshDef") return "CReplaceableMeshDef";
    if (type == "CResurrectionItemDef") return "CResurrectionItemDef";
    if (type == "CRumbleDef") return "CRumbleDef";
    if (type == "CScorpionKingBattleDef") return "CScorpionKingBattleDef";
    if (type == "CScriptDef") return "CScriptDef";
    if (type == "CShipDef") return "CShipDef";
    if (type == "CShopDef") return "CShopDef";
    if (type == "CShopItemDef") return "CShopItemDef";
    if (type == "CSkeletalMorphDef") return "CSkeletalMorphDef";
    if (type == "CSmashableDef") return "CSmashableDef";
    if (type == "CSmokeGeneratorDef") return "CSmokeGeneratorDef";
    if (type == "CSnowTrollDef") return "CSnowTrollDef";
    if (type == "CSoundAtmospheresDef") return "CSoundAtmospheresDef";
    if (type == "CSpecialAbilitiesDrainLifeDataDef") return "CSpecialAbilitiesForcePushDataDef";
    if (type == "CSpecialAbilitiesForcePushDataDef") return "CSpecialAbilitiesForcePushDataDef";
    if (type == "CSpecialEffectsDef") return "CSpecialEffectsDef";
    if (type == "CSpotLightDef") return "CSpotLightDef";
    if (type == "CStealthDef") return "CStealthDef";
    if (type == "CStockItemDef") return "CStockItemDef";
    if (type == "CSummonDef") return "CSummonDef";
    if (type == "CSummonableCreatureDef") return "CSummonableCreatureDef";
    if (type == "CSummonerDef") return "CSummonerDef";
    if (type == "CTCVolumeContainmentTrackerDef") return "CTCVolumeContainmentTrackerDef";
    if (type == "CTargetingDef") return "CTargetingDef";
    if (type == "CTattooDef") return "CTattooDef";
    if (type == "CTavernDef") return "CTavernDef";
    if (type == "CTavernGameCardBaseDef") return "CTavernGameCardBaseDef";
    if (type == "CTavernGameCoinBaseDef") return "CTavernGameCoinBaseDef";
    if (type == "CTavernGameCoinGolfDef") return "CTavernGameCoinGolfDef";
    if (type == "CTavernGameDef") return "CTavernGameDef";
    if (type == "CTavernGameShoveHaPennyDef") return "CTavernGameShoveHaPennyDef";
    if (type == "CTavernGameSpotTheAdditionDef") return "CTavernGameSpotTheAdditionDef";
    if (type == "CTavernTableDef") return "CTavernTableDef";
    if (type == "CTeleporterDef") return "CTeleporterDef";
    if (type == "CTextureReplacementDef") return "CTextureReplacementDef";
    if (type == "CThingDrainLifeShotDef") return "CThingDrainLifeShotDef";
    if (type == "CThingMultiArrowShotDef") return "CThingMultiArrowShotDef";
    if (type == "CThunderBattleDef") return "CThunderBattleDef";
    if (type == "CTimeAppearanceFadeDef") return "CTimeAppearanceFadeDef";
    if (type == "CTrapDef") return "CTrapDef";
    if (type == "CTrollBattleDef") return "CTrollBattleDef";
    if (type == "CTrophyDef") return "CTrophyDef";
    if (type == "CTurncoatDef") return "CTurncoatDef";
    if (type == "CVillageDef") return "CVillageDef";
    if (type == "CVillageMemberDef") return "CVillageMemberDef";
    if (type == "CVillagePeopleDef") return "CVillagePeopleDef";
    if (type == "CWallMountEffectsDef") return "CWallMountEffectsDef";
    if (type == "CWaspQueenBattleDef") return "CWaspQueenBattleDef";
    if (type == "CWeaponDef") return "CWeaponDef";
    if (type == "CWhisperBattleDef") return "CWhisperBattleDef";
    if (type == "CWifeDef") return "CWifeDef";
    if (type == "CWillResponseDef") return "CWillResponseDef";
    if (type == "ENGINE") return "CEngineDef";
    if (type == "ENGINE_THEME") return "CEngineThemeDef";
    if (type == "ENGINE_THEME_GROUP") return "CEngineThemeGroupDef";
    if (type == "ENGINE_VIDEO_OPTIONS") return "CEngineVideoOptionsDef";
    if (type == "ENVIRONMENT") return "CEnvironmentDef";
    if (type == "ENVIRONMENT_THEME_DAY") return "CEnvironmentThemeDaySetDef";
    if (type == "EXPRESSION") return "CExpressionDef";
    if (type == "FACTION") return "CFactionDef";
    if (type == "FRONT_END") return "CFrontEndDef";
    if (type == "GLOBAL") return "CGlobalDef";
    if (type == "HERO_ABILITY") return "CHeroAbilityDef";
    if (type == "HERO_COMBAT") return "CHeroCombatDef";
    if (type == "HERO_MELEE_COMBAT_ABILITY") return "CMeleeCombatAbilityDef";
    if (type == "HERO_STATS") return "CHeroStatsDef";
    if (type == "HIT_LOCATION") return "CHitLocationDef";
    if (type == "HOLY_SITE") return "CThingHolySiteDef";
    if (type == "INVENTORY_CATEGORY") return "CInventoryCategoryDef";
    if (type == "INVENTORY_ITEM") return "CInventoryItemDef";
    if (type == "INVENTORY_TYPE") return "CInventoryDef";
    if (type == "LIGHTNING") return "CLightningDef";
    if (type == "LOCAL_DETAIL_GENERATOR") return "CEngineLocalDetailGeneratorDef";
    if (type == "MARKER") return "CThingMarkerDef";
    if (type == "MATERIAL") return "CMaterialDef";
    if (type == "MELEE_COMBAT_KNOCKDOWN_EFFECTS") return "CMeleeCombatKnockdownEffects";
    if (type == "MESSAGE_EVENT") return "CMessageEventDef";
    if (type == "NOISE") return "CThingNoiseDef";
    if (type == "OBJECT") return "CThingObjectDef";
    if (type == "OBJECT_FAMILY") return "CObjectFamilyDef";
    if (type == "OPINION_DEED_EFFECTS") return "COpinionDeedEffectsDef";
    if (type == "OPINION_DEED_MASK") return "COpinionDeedMaskDef";
    if (type == "OPINION_PERSONALITY") return "COpinionPersonalityDef";
    if (type == "OPINION_REACTION_MANAGER") return "COpinionReactionManagerDef";
    if (type == "OPINION_REACTION_MASK") return "COpinionReactionMaskDef";
    if (type == "OPINION_SOURCE") return "COpinionSourceDef";
    if (type == "PHYSICAL_SWITCH") return "CThingPhysicalSwitchDef";
    if (type == "PLAYER") return "CPlayerDef";
    if (type == "PLAYER_GUI") return "CPlayerGuiDef";
    if (type == "PLAYER_INVENTORY") return "CPlayerInventoryDef";
    if (type == "PLAYER_MOVEMENT") return "CPlayerMovementDef";
    if (type == "REGION") return "CRegionDef";
    if (type == "SHOT") return "CThingShotDef";
    if (type == "SIM_BUILDING") return "CSimBuildingDef";
    if (type == "SIM_VOICES") return "CSimVoicesDef";
    if (type == "SKY") return "CSkyDef";
    if (type == "SOUND_SETUP") return "CSoundDef";
    if (type == "SOUND_THEME") return "CSoundThemeDef";
    if (type == "SPECIAL_ABILITIES_ASSASSIN_RUSH_DEF") return "CSpecialAbilitiesAssassinRushDef";
    if (type == "SPECIAL_ABILITIES_BATTLE_CHARGE_DEF") return "CSpecialAbilitiesBattleChargeDef";
    if (type == "SPECIAL_ABILITIES_BERSERK_DEF") return "CSpecialAbilitiesBerserkDef";
    if (type == "SPECIAL_ABILITIES_BULLET_TIME_DEF") return "CSpecialAbilitiesBulletTimeDef";
    if (type == "SPECIAL_ABILITIES_BURNT_EFFECT_DEF") return "CSpecialAbilitiesBurntEffectDef";
    if (type == "SPECIAL_ABILITIES_CREATURE_TINT_DEF") return "CSpecialAbilitiesCreatureTintDef";
    if (type == "SPECIAL_ABILITIES_DIVINE_WRATH_DEF") return "CSpecialAbilitiesUnholyPowerDef";
    if (type == "SPECIAL_ABILITIES_DRAIN_LIFE_DEF") return "CSpecialAbilitiesDrainLifeDef";
    if (type == "SPECIAL_ABILITIES_DRUNKENNESS_DEF") return "CSpecialAbilitiesDrunkennessDef";
    if (type == "SPECIAL_ABILITIES_ELECTROCUTED_EFFECT_DEF") return "CSpecialAbilitiesElectrocutedEffectDef";
    if (type == "SPECIAL_ABILITIES_ENFLAME_DEF") return "CSpecialAbilitiesEnflameDef";
    if (type == "SPECIAL_ABILITIES_FIREBALL_SPELL_DEF") return "CSpecialAbilitiesFireballSpellDef";
    if (type == "SPECIAL_ABILITIES_FORCE_PUSH_DEF") return "CSpecialAbilitiesForcePushDef";
    if (type == "SPECIAL_ABILITIES_GHOST_SWORD_DEF") return "CSpecialAbilitiesGhostSwordDef";
    if (type == "SPECIAL_ABILITIES_HEAL_LIFE_DEF") return "CSpecialAbilitiesHealLifeDef";
    if (type == "SPECIAL_ABILITIES_LIGHTNING_SPELL_DEF") return "CSpecialAbilitiesLightningSpellDef";
    if (type == "SPECIAL_ABILITIES_MULTI_ARROW_DEF") return "CSpecialAbilitiesMultiArrowDef";
    if (type == "SPECIAL_ABILITIES_MULTI_STRIKE_DEF") return "CSpecialAbilitiesMultiStrikeDef";
    if (type == "SPECIAL_ABILITIES_PHYSICAL_SHIELD_DEF") return "CSpecialAbilitiesPhysicalShieldDef";
    if (type == "SPECIAL_ABILITIES_SUMMON_SPELL_DEF") return "CSpecialAbilitiesSummonSpellDef";
    if (type == "SPECIAL_ABILITIES_THUNDER_LIGHTNING_STORM_DEF") return "CSpecialAbilitiesThunderLightningStormDef";
    if (type == "SPECIAL_ABILITIES_TURNCOAT_SPELL_DEF") return "CSpecialAbilitiesTurncoatSpellDef";
    if (type == "SPECIAL_ABILITIES_UNHOLY_POWER_DEF") return "CSpecialAbilitiesUnholyPowerDef";
    if (type == "SWITCH") return "CThingSwitchDef";
    if (type == "THING") return "CThingBaseDef";
    if (type == "THING_GROUP") return "CEngineThemeGroupDef";
    if (type == "UI") return "NUISystem::CUIDef";
    if (type == "UI_ICONS_DEF") return "NUISystem::CUIIconsDef";
    if (type == "UI_LOCALE_GRAPHICS_DEF") return "NUISystem::CUILocaleGraphicsDef";
    if (type == "UI_MISC_THINGS_DEF") return "NUISystem::CUIMiscThingsDef";
    if (type == "VILLAGE") return "CThingVillageDef";
    if (type == "VILLAGER_INTERACTION") return "CVillagerInteractionsDef";
    return nullptr;
}

} // namespace fable::defs::compiler

#endif // FABLE_DEFS_COMPILER_TYPE_REGISTRY_HPP
