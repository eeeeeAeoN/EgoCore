// Runtime class lookup for the CLI. Generated from classes.hpp.
#ifndef FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP
#define FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP

#include <string>
#include <utility>

#include <fable/defs.hpp>

namespace fable::defs::compiler {

/// Call `f` with a default-constructed instance of the named model class.
template <class F>
bool with_class(const std::string &name, F &&f) {
    if (name == "C2DBoxF") {
        f(::fable::defs::C2DBoxF{});
        return true;
    }
    if (name == "C2DBoxI") {
        f(::fable::defs::C2DBoxI{});
        return true;
    }
    if (name == "C2DVector") {
        f(::fable::defs::C2DVector{});
        return true;
    }
    if (name == "C3DVector") {
        f(::fable::defs::C3DVector{});
        return true;
    }
    if (name == "CAICreatureWillPowerIndicatorDef") {
        f(::fable::defs::CAICreatureWillPowerIndicatorDef{});
        return true;
    }
    if (name == "CAIScratchpadDef") {
        f(::fable::defs::CAIScratchpadDef{});
        return true;
    }
    if (name == "CAbilityButtonGuiDef") {
        f(::fable::defs::CAbilityButtonGuiDef{});
        return true;
    }
    if (name == "CAbilityDef") {
        f(::fable::defs::CAbilityDef{});
        return true;
    }
    if (name == "CAbilityTrackLevelDef") {
        f(::fable::defs::CAbilityTrackLevelDef{});
        return true;
    }
    if (name == "CAbilityTrackDef") {
        f(::fable::defs::CAbilityTrackDef{});
        return true;
    }
    if (name == "CActionInputControl") {
        f(::fable::defs::CActionInputControl{});
        return true;
    }
    if (name == "CActionUseDef") {
        f(::fable::defs::CActionUseDef{});
        return true;
    }
    if (name == "CActivateQuestDef") {
        f(::fable::defs::CActivateQuestDef{});
        return true;
    }
    if (name == "CAnimComponentBase") {
        f(::fable::defs::CAnimComponentBase{});
        return true;
    }
    if (name == "CAnimComponentMap") {
        f(::fable::defs::CAnimComponentMap{});
        return true;
    }
    if (name == "CAnimationComponent") {
        f(::fable::defs::CAnimationComponent{});
        return true;
    }
    if (name == "CAnimationEntry") {
        f(::fable::defs::CAnimationEntry{});
        return true;
    }
    if (name == "CAnimationSet") {
        f(::fable::defs::CAnimationSet{});
        return true;
    }
    if (name == "CAnimatingObjectDef") {
        f(::fable::defs::CAnimatingObjectDef{});
        return true;
    }
    if (name == "CEngineGraphic") {
        f(::fable::defs::CEngineGraphic{});
        return true;
    }
    if (name == "CAppearanceDef") {
        f(::fable::defs::CAppearanceDef{});
        return true;
    }
    if (name == "CAppearanceModifierGraphics") {
        f(::fable::defs::CAppearanceModifierGraphics{});
        return true;
    }
    if (name == "CAppearanceModifierGraphics::CEntry") {
        f(::fable::defs::CAppearanceModifierGraphics::CEntry{});
        return true;
    }
    if (name == "CAppearanceModifierDef") {
        f(::fable::defs::CAppearanceModifierDef{});
        return true;
    }
    if (name == "CAppearanceModifierScalingDef") {
        f(::fable::defs::CAppearanceModifierScalingDef{});
        return true;
    }
    if (name == "CAreaOfEffectAttackDef") {
        f(::fable::defs::CAreaOfEffectAttackDef{});
        return true;
    }
    if (name == "CArenaCreatureDef") {
        f(::fable::defs::CArenaCreatureDef{});
        return true;
    }
    if (name == "CArenaWaveDef") {
        f(::fable::defs::CArenaWaveDef{});
        return true;
    }
    if (name == "CArenaRoundDef") {
        f(::fable::defs::CArenaRoundDef{});
        return true;
    }
    if (name == "CArmourDef") {
        f(::fable::defs::CArmourDef{});
        return true;
    }
    if (name == "CAtmosBankEntry") {
        f(::fable::defs::CAtmosBankEntry{});
        return true;
    }
    if (name == "CAttackHistoryCombo") {
        f(::fable::defs::CAttackHistoryCombo{});
        return true;
    }
    if (name == "CAttackPatternDef") {
        f(::fable::defs::CAttackPatternDef{});
        return true;
    }
    if (name == "CAugmentationDef") {
        f(::fable::defs::CAugmentationDef{});
        return true;
    }
    if (name == "CBalverineBattleDef") {
        f(::fable::defs::CBalverineBattleDef{});
        return true;
    }
    if (name == "CBedDef") {
        f(::fable::defs::CBedDef{});
        return true;
    }
    if (name == "CBettingDef") {
        f(::fable::defs::CBettingDef{});
        return true;
    }
    if (name == "CBlendedParticleEffectSet") {
        f(::fable::defs::CBlendedParticleEffectSet{});
        return true;
    }
    if (name == "CBlurredAttitudesDef") {
        f(::fable::defs::CBlurredAttitudesDef{});
        return true;
    }
    if (name == "CBoastingPodiumDef") {
        f(::fable::defs::CBoastingPodiumDef{});
        return true;
    }
    if (name == "CBonusItemDef") {
        f(::fable::defs::CBonusItemDef{});
        return true;
    }
    if (name == "CBossDef") {
        f(::fable::defs::CBossDef{});
        return true;
    }
    if (name == "CBrainBehaviour") {
        f(::fable::defs::CBrainBehaviour{});
        return true;
    }
    if (name == "CBrainUpdateZone") {
        f(::fable::defs::CBrainUpdateZone{});
        return true;
    }
    if (name == "CBrainDef") {
        f(::fable::defs::CBrainDef{});
        return true;
    }
    if (name == "CBriarRoseDef") {
        f(::fable::defs::CBriarRoseDef{});
        return true;
    }
    if (name == "CBuyHouseDef") {
        f(::fable::defs::CBuyHouseDef{});
        return true;
    }
    if (name == "CBuyableHouseDef") {
        f(::fable::defs::CBuyableHouseDef{});
        return true;
    }
    if (name == "CCameraCollisionDef") {
        f(::fable::defs::CCameraCollisionDef{});
        return true;
    }
    if (name == "CCameraManagerDef") {
        f(::fable::defs::CCameraManagerDef{});
        return true;
    }
    if (name == "CCameraManagerSetDef") {
        f(::fable::defs::CCameraManagerSetDef{});
        return true;
    }
    if (name == "CCameraModeDef") {
        f(::fable::defs::CCameraModeDef{});
        return true;
    }
    if (name == "CCardDef") {
        f(::fable::defs::CCardDef{});
        return true;
    }
    if (name == "CCardPositionsDef") {
        f(::fable::defs::CCardPositionsDef{});
        return true;
    }
    if (name == "CCarriedReadableDef") {
        f(::fable::defs::CCarriedReadableDef{});
        return true;
    }
    if (name == "CCarrySlotDef") {
        f(::fable::defs::CCarrySlotDef{});
        return true;
    }
    if (name == "CCarryableDef") {
        f(::fable::defs::CCarryableDef{});
        return true;
    }
    if (name == "CCarryingDef") {
        f(::fable::defs::CCarryingDef{});
        return true;
    }
    if (name == "CChestDef") {
        f(::fable::defs::CChestDef{});
        return true;
    }
    if (name == "CClockDef") {
        f(::fable::defs::CClockDef{});
        return true;
    }
    if (name == "CCoinGameObstacleDef") {
        f(::fable::defs::CCoinGameObstacleDef{});
        return true;
    }
    if (name == "CCombatAbilityData") {
        f(::fable::defs::CCombatAbilityData{});
        return true;
    }
    if (name == "CCombatAbilityMeleeAttackData") {
        f(::fable::defs::CCombatAbilityMeleeAttackData{});
        return true;
    }
    if (name == "CCombatAbilityAttackBase") {
        f(::fable::defs::CCombatAbilityAttackBase{});
        return true;
    }
    if (name == "CCombatAbilityBlockCounterAttackDef") {
        f(::fable::defs::CCombatAbilityBlockCounterAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityBlockingData") {
        f(::fable::defs::CCombatAbilityBlockingData{});
        return true;
    }
    if (name == "CCombatAbilityBlockDefBase") {
        f(::fable::defs::CCombatAbilityBlockDefBase{});
        return true;
    }
    if (name == "CCombatAbilityBlockHeavyWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockHeavyWeaponAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityBlockLightWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockLightWeaponAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityBlockProjectileWeaponAttackDef") {
        f(::fable::defs::CCombatAbilityBlockProjectileWeaponAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityBlockUnarmedAttackDef") {
        f(::fable::defs::CCombatAbilityBlockUnarmedAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityFlourishCounterAttackDef") {
        f(::fable::defs::CCombatAbilityFlourishCounterAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityGetHitCounterAttackDef") {
        f(::fable::defs::CCombatAbilityGetHitCounterAttackDef{});
        return true;
    }
    if (name == "CCombatAbilityProjectileAttackData") {
        f(::fable::defs::CCombatAbilityProjectileAttackData{});
        return true;
    }
    if (name == "CCombatAbilityStrafingData") {
        f(::fable::defs::CCombatAbilityStrafingData{});
        return true;
    }
    if (name == "CCombatAbilityStrafeDef") {
        f(::fable::defs::CCombatAbilityStrafeDef{});
        return true;
    }
    if (name == "CCombatAbilityUseProjectileWeaponDef") {
        f(::fable::defs::CCombatAbilityUseProjectileWeaponDef{});
        return true;
    }
    if (name == "CCombatDialogueDef") {
        f(::fable::defs::CCombatDialogueDef{});
        return true;
    }
    if (name == "CCombatSequenceDef") {
        f(::fable::defs::CCombatSequenceDef{});
        return true;
    }
    if (name == "CCombatTypeDef") {
        f(::fable::defs::CCombatTypeDef{});
        return true;
    }
    if (name == "CComboMultiplierData") {
        f(::fable::defs::CComboMultiplierData{});
        return true;
    }
    if (name == "CConfigOptionsDefaultsDef") {
        f(::fable::defs::CConfigOptionsDefaultsDef{});
        return true;
    }
    if (name == "CContainerRewardHeroDef") {
        f(::fable::defs::CContainerRewardHeroDef{});
        return true;
    }
    if (name == "CContextSensitiveItemDef") {
        f(::fable::defs::CContextSensitiveItemDef{});
        return true;
    }
    if (name == "CControlsDef") {
        f(::fable::defs::CControlsDef{});
        return true;
    }
    if (name == "CConversation") {
        f(::fable::defs::CConversation{});
        return true;
    }
    if (name == "CCoopSpiritDef") {
        f(::fable::defs::CCoopSpiritDef{});
        return true;
    }
    if (name == "CCrateStackDef") {
        f(::fable::defs::CCrateStackDef{});
        return true;
    }
    if (name == "CCreatureAbilityDef") {
        f(::fable::defs::CCreatureAbilityDef{});
        return true;
    }
    if (name == "CFloatRange") {
        f(::fable::defs::CFloatRange{});
        return true;
    }
    if (name == "CHeroAbilityStatWeight") {
        f(::fable::defs::CHeroAbilityStatWeight{});
        return true;
    }
    if (name == "CCreatureDef") {
        f(::fable::defs::CCreatureDef{});
        return true;
    }
    if (name == "CCreatureGenerationFamilyDef") {
        f(::fable::defs::CCreatureGenerationFamilyDef{});
        return true;
    }
    if (name == "CCreatureGeneratorDef") {
        f(::fable::defs::CCreatureGeneratorDef{});
        return true;
    }
    if (name == "CCreatureModeDef") {
        f(::fable::defs::CCreatureModeDef{});
        return true;
    }
    if (name == "CCreatureNavigationDef") {
        f(::fable::defs::CCreatureNavigationDef{});
        return true;
    }
    if (name == "CCreatureStatsDef") {
        f(::fable::defs::CCreatureStatsDef{});
        return true;
    }
    if (name == "CCutsceneDef") {
        f(::fable::defs::CCutsceneDef{});
        return true;
    }
    if (name == "CDecapitationDef") {
        f(::fable::defs::CDecapitationDef{});
        return true;
    }
    if (name == "CDegradableInfo") {
        f(::fable::defs::CDegradableInfo{});
        return true;
    }
    if (name == "CDegradableDef") {
        f(::fable::defs::CDegradableDef{});
        return true;
    }
    if (name == "CDegradableDef::CIsHealth1GTHealth2") {
        f(::fable::defs::CDegradableDef::CIsHealth1GTHealth2{});
        return true;
    }
    if (name == "CDegradableDef::CSetEngineGraphicType") {
        f(::fable::defs::CDegradableDef::CSetEngineGraphicType{});
        return true;
    }
    if (name == "CDoorDef") {
        f(::fable::defs::CDoorDef{});
        return true;
    }
    if (name == "CDragonActionHoverDef") {
        f(::fable::defs::CDragonActionHoverDef{});
        return true;
    }
    if (name == "CDragonActionNapalmDef") {
        f(::fable::defs::CDragonActionNapalmDef{});
        return true;
    }
    if (name == "CDragonActionSwoopDef") {
        f(::fable::defs::CDragonActionSwoopDef{});
        return true;
    }
    if (name == "CDrunkennessDef") {
        f(::fable::defs::CDrunkennessDef{});
        return true;
    }
    if (name == "CEnemyDef") {
        f(::fable::defs::CEnemyDef{});
        return true;
    }
    if (name == "CEngineDef") {
        f(::fable::defs::CEngineDef{});
        return true;
    }
    if (name == "CEngineLocalDetailObjectDef") {
        f(::fable::defs::CEngineLocalDetailObjectDef{});
        return true;
    }
    if (name == "CEngineLocalDetailLayerDef") {
        f(::fable::defs::CEngineLocalDetailLayerDef{});
        return true;
    }
    if (name == "CEngineLocalDetailGeneratorDef") {
        f(::fable::defs::CEngineLocalDetailGeneratorDef{});
        return true;
    }
    if (name == "CEngineThemeDef") {
        f(::fable::defs::CEngineThemeDef{});
        return true;
    }
    if (name == "CEngineThemeGroupDef") {
        f(::fable::defs::CEngineThemeGroupDef{});
        return true;
    }
    if (name == "CEngineVideoOptionsDef") {
        f(::fable::defs::CEngineVideoOptionsDef{});
        return true;
    }
    if (name == "CSoundPair") {
        f(::fable::defs::CSoundPair{});
        return true;
    }
    if (name == "CSoundMap") {
        f(::fable::defs::CSoundMap{});
        return true;
    }
    if (name == "CEntitySoundDef") {
        f(::fable::defs::CEntitySoundDef{});
        return true;
    }
    if (name == "CEnvironmentDef") {
        f(::fable::defs::CEnvironmentDef{});
        return true;
    }
    if (name == "CEnvironmentThemeDef") {
        f(::fable::defs::CEnvironmentThemeDef{});
        return true;
    }
    if (name == "CRGBColour") {
        f(::fable::defs::CRGBColour{});
        return true;
    }
    if (name == "CEnvironmentThemeDaySetDef") {
        f(::fable::defs::CEnvironmentThemeDaySetDef{});
        return true;
    }
    if (name == "CExperienceDef") {
        f(::fable::defs::CExperienceDef{});
        return true;
    }
    if (name == "CExplodingObjectDef") {
        f(::fable::defs::CExplodingObjectDef{});
        return true;
    }
    if (name == "CExplosionRing") {
        f(::fable::defs::CExplosionRing{});
        return true;
    }
    if (name == "CExplosionDef") {
        f(::fable::defs::CExplosionDef{});
        return true;
    }
    if (name == "CExplosiveTrailDef") {
        f(::fable::defs::CExplosiveTrailDef{});
        return true;
    }
    if (name == "CExpressionDef") {
        f(::fable::defs::CExpressionDef{});
        return true;
    }
    if (name == "CExpressionSet") {
        f(::fable::defs::CExpressionSet{});
        return true;
    }
    if (name == "CExpressionSet::CEntry") {
        f(::fable::defs::CExpressionSet::CEntry{});
        return true;
    }
    if (name == "CExpressionSubDef") {
        f(::fable::defs::CExpressionSubDef{});
        return true;
    }
    if (name == "CFactionDef") {
        f(::fable::defs::CFactionDef{});
        return true;
    }
    if (name == "CFeatDef") {
        f(::fable::defs::CFeatDef{});
        return true;
    }
    if (name == "CFireEffectCreationSet") {
        f(::fable::defs::CFireEffectCreationSet{});
        return true;
    }
    if (name == "CFireHeartPatternDef") {
        f(::fable::defs::CFireHeartPatternDef{});
        return true;
    }
    if (name == "CFireballSpellLevelDef") {
        f(::fable::defs::CFireballSpellLevelDef{});
        return true;
    }
    if (name == "CFireheartMinigameDef") {
        f(::fable::defs::CFireheartMinigameDef{});
        return true;
    }
    if (name == "CFishDef") {
        f(::fable::defs::CFishDef{});
        return true;
    }
    if (name == "CFishingDef") {
        f(::fable::defs::CFishingDef{});
        return true;
    }
    if (name == "CFishingRodDef") {
        f(::fable::defs::CFishingRodDef{});
        return true;
    }
    if (name == "CFlammableDef") {
        f(::fable::defs::CFlammableDef{});
        return true;
    }
    if (name == "CFrontEndDef") {
        f(::fable::defs::CFrontEndDef{});
        return true;
    }
    if (name == "CGiftDef") {
        f(::fable::defs::CGiftDef{});
        return true;
    }
    if (name == "NSpeechGainManager::CDialogueLayerDef") {
        f(::fable::defs::NSpeechGainManager::CDialogueLayerDef{});
        return true;
    }
    if (name == "CGlobalDef") {
        f(::fable::defs::CGlobalDef{});
        return true;
    }
    if (name == "CGoldDef") {
        f(::fable::defs::CGoldDef{});
        return true;
    }
    if (name == "CGuardDef") {
        f(::fable::defs::CGuardDef{});
        return true;
    }
    if (name == "CGuiGraphicDef") {
        f(::fable::defs::CGuiGraphicDef{});
        return true;
    }
    if (name == "CGuildMasterDef") {
        f(::fable::defs::CGuildMasterDef{});
        return true;
    }
    if (name == "CHairCardDef") {
        f(::fable::defs::CHairCardDef{});
        return true;
    }
    if (name == "CHasNameDef") {
        f(::fable::defs::CHasNameDef{});
        return true;
    }
    if (name == "CHeroAbilityDef") {
        f(::fable::defs::CHeroAbilityDef{});
        return true;
    }
    if (name == "CHeroAbilityStatPackage") {
        f(::fable::defs::CHeroAbilityStatPackage{});
        return true;
    }
    if (name == "CHeroCentreDef") {
        f(::fable::defs::CHeroCentreDef{});
        return true;
    }
    if (name == "CHeroCombatDef") {
        f(::fable::defs::CHeroCombatDef{});
        return true;
    }
    if (name == "CHeroDef") {
        f(::fable::defs::CHeroDef{});
        return true;
    }
    if (name == "CHeroStatIncreaseData") {
        f(::fable::defs::CHeroStatIncreaseData{});
        return true;
    }
    if (name == "CHeroExperienceDef") {
        f(::fable::defs::CHeroExperienceDef{});
        return true;
    }
    if (name == "CHeroMarriageDef") {
        f(::fable::defs::CHeroMarriageDef{});
        return true;
    }
    if (name == "CParticleMorphs") {
        f(::fable::defs::CParticleMorphs{});
        return true;
    }
    if (name == "CParticleMorphs::CEntry") {
        f(::fable::defs::CParticleMorphs::CEntry{});
        return true;
    }
    if (name == "CSkeletalMorphs") {
        f(::fable::defs::CSkeletalMorphs{});
        return true;
    }
    if (name == "CSkeletalMorphs::CEntry") {
        f(::fable::defs::CSkeletalMorphs::CEntry{});
        return true;
    }
    if (name == "CTextureMorphEntry") {
        f(::fable::defs::CTextureMorphEntry{});
        return true;
    }
    if (name == "CTextureMorphs") {
        f(::fable::defs::CTextureMorphs{});
        return true;
    }
    if (name == "CHeroMorphDef") {
        f(::fable::defs::CHeroMorphDef{});
        return true;
    }
    if (name == "CHeroPostcardGeneratorDef") {
        f(::fable::defs::CHeroPostcardGeneratorDef{});
        return true;
    }
    if (name == "CHeroSoulsCreatureDef") {
        f(::fable::defs::CHeroSoulsCreatureDef{});
        return true;
    }
    if (name == "CHeroSoulsWaveDef") {
        f(::fable::defs::CHeroSoulsWaveDef{});
        return true;
    }
    if (name == "CHeroSoulsRoundDef") {
        f(::fable::defs::CHeroSoulsRoundDef{});
        return true;
    }
    if (name == "CHeroSpecialMovementDef") {
        f(::fable::defs::CHeroSpecialMovementDef{});
        return true;
    }
    if (name == "CHeroStatsDef") {
        f(::fable::defs::CHeroStatsDef{});
        return true;
    }
    if (name == "CHeroSuitDef") {
        f(::fable::defs::CHeroSuitDef{});
        return true;
    }
    if (name == "CHeroTitleDef") {
        f(::fable::defs::CHeroTitleDef{});
        return true;
    }
    if (name == "CHighlightItemDef") {
        f(::fable::defs::CHighlightItemDef{});
        return true;
    }
    if (name == "NHitLocations::CPhysicalPrimitiveBase") {
        f(::fable::defs::NHitLocations::CPhysicalPrimitiveBase{});
        return true;
    }
    if (name == "CPhysicalPrimitiveInit") {
        f(::fable::defs::CPhysicalPrimitiveInit{});
        return true;
    }
    if (name == "CHitLocationDef") {
        f(::fable::defs::CHitLocationDef{});
        return true;
    }
    if (name == "CHitLocationsDef") {
        f(::fable::defs::CHitLocationsDef{});
        return true;
    }
    if (name == "CIdleSchedulerDef") {
        f(::fable::defs::CIdleSchedulerDef{});
        return true;
    }
    if (name == "CInterestingToVillagersDef") {
        f(::fable::defs::CInterestingToVillagersDef{});
        return true;
    }
    if (name == "CInventoryCategoryDef") {
        f(::fable::defs::CInventoryCategoryDef{});
        return true;
    }
    if (name == "CInventoryDef") {
        f(::fable::defs::CInventoryDef{});
        return true;
    }
    if (name == "CInventoryItemDef") {
        f(::fable::defs::CInventoryItemDef{});
        return true;
    }
    if (name == "CJackDragonDef") {
        f(::fable::defs::CJackDragonDef{});
        return true;
    }
    if (name == "CJackOfBladesBattleDef") {
        f(::fable::defs::CJackOfBladesBattleDef{});
        return true;
    }
    if (name == "CKickableDef") {
        f(::fable::defs::CKickableDef{});
        return true;
    }
    if (name == "CKrakenDef") {
        f(::fable::defs::CKrakenDef{});
        return true;
    }
    if (name == "CKrakenTentacleDef") {
        f(::fable::defs::CKrakenTentacleDef{});
        return true;
    }
    if (name == "CLensFlareElementDef") {
        f(::fable::defs::CLensFlareElementDef{});
        return true;
    }
    if (name == "CLightDef") {
        f(::fable::defs::CLightDef{});
        return true;
    }
    if (name == "CLightningDef") {
        f(::fable::defs::CLightningDef{});
        return true;
    }
    if (name == "CLightningOrbDef") {
        f(::fable::defs::CLightningOrbDef{});
        return true;
    }
    if (name == "CLookDef") {
        f(::fable::defs::CLookDef{});
        return true;
    }
    if (name == "CMaterialDef") {
        f(::fable::defs::CMaterialDef{});
        return true;
    }
    if (name == "CMazeBattleDef") {
        f(::fable::defs::CMazeBattleDef{});
        return true;
    }
    if (name == "CMeleeCombatAbilityDef") {
        f(::fable::defs::CMeleeCombatAbilityDef{});
        return true;
    }
    if (name == "CMeleeCombatKnockdownEffects") {
        f(::fable::defs::CMeleeCombatKnockdownEffects{});
        return true;
    }
    if (name == "CMessageEventDef") {
        f(::fable::defs::CMessageEventDef{});
        return true;
    }
    if (name == "CMultiStaticMeshEntryDef") {
        f(::fable::defs::CMultiStaticMeshEntryDef{});
        return true;
    }
    if (name == "CMultiStaticMeshDef") {
        f(::fable::defs::CMultiStaticMeshDef{});
        return true;
    }
    if (name == "CMusicEntry") {
        f(::fable::defs::CMusicEntry{});
        return true;
    }
    if (name == "CMusicSetEntry") {
        f(::fable::defs::CMusicSetEntry{});
        return true;
    }
    if (name == "CNymphDef") {
        f(::fable::defs::CNymphDef{});
        return true;
    }
    if (name == "CObjectAugmentationParticleSet") {
        f(::fable::defs::CObjectAugmentationParticleSet{});
        return true;
    }
    if (name == "CObjectAugmentationsDef") {
        f(::fable::defs::CObjectAugmentationsDef{});
        return true;
    }
    if (name == "CObjectFamilyEntry") {
        f(::fable::defs::CObjectFamilyEntry{});
        return true;
    }
    if (name == "CObjectFamilyDef") {
        f(::fable::defs::CObjectFamilyDef{});
        return true;
    }
    if (name == "COccupiableDef") {
        f(::fable::defs::COccupiableDef{});
        return true;
    }
    if (name == "COpinionDeedReactionDef") {
        f(::fable::defs::COpinionDeedReactionDef{});
        return true;
    }
    if (name == "COpinionTransientOffset") {
        f(::fable::defs::COpinionTransientOffset{});
        return true;
    }
    if (name == "COpinionDeedEffectsDef") {
        f(::fable::defs::COpinionDeedEffectsDef{});
        return true;
    }
    if (name == "COpinionDeedMaskDef") {
        f(::fable::defs::COpinionDeedMaskDef{});
        return true;
    }
    if (name == "COpinionOfHeroDef") {
        f(::fable::defs::COpinionOfHeroDef{});
        return true;
    }
    if (name == "COpinionPersonalityTraits") {
        f(::fable::defs::COpinionPersonalityTraits{});
        return true;
    }
    if (name == "COpinionPersonalityDef") {
        f(::fable::defs::COpinionPersonalityDef{});
        return true;
    }
    if (name == "CReactionFrequencyTraits") {
        f(::fable::defs::CReactionFrequencyTraits{});
        return true;
    }
    if (name == "CReactionFrequencyTraitsArray") {
        f(::fable::defs::CReactionFrequencyTraitsArray{});
        return true;
    }
    if (name == "CReactionMatchEntry") {
        f(::fable::defs::CReactionMatchEntry{});
        return true;
    }
    if (name == "CReactionMatchList") {
        f(::fable::defs::CReactionMatchList{});
        return true;
    }
    if (name == "COpinionReactionManagerDef") {
        f(::fable::defs::COpinionReactionManagerDef{});
        return true;
    }
    if (name == "COpinionReactionMaskDef") {
        f(::fable::defs::COpinionReactionMaskDef{});
        return true;
    }
    if (name == "COpinionSet") {
        f(::fable::defs::COpinionSet{});
        return true;
    }
    if (name == "COpinionSourceDef") {
        f(::fable::defs::COpinionSourceDef{});
        return true;
    }
    if (name == "COracleMinigameDef") {
        f(::fable::defs::COracleMinigameDef{});
        return true;
    }
    if (name == "COverheadDisplayDef") {
        f(::fable::defs::COverheadDisplayDef{});
        return true;
    }
    if (name == "NParticleAttachment::CParticleAttachmentInfo") {
        f(::fable::defs::NParticleAttachment::CParticleAttachmentInfo{});
        return true;
    }
    if (name == "CParticleAttacherDef") {
        f(::fable::defs::CParticleAttacherDef{});
        return true;
    }
    if (name == "CPerceivedThingDef") {
        f(::fable::defs::CPerceivedThingDef{});
        return true;
    }
    if (name == "CPhysicsDef") {
        f(::fable::defs::CPhysicsDef{});
        return true;
    }
    if (name == "CPlayerDef") {
        f(::fable::defs::CPlayerDef{});
        return true;
    }
    if (name == "CQuickAccessItemButtonGuiDef") {
        f(::fable::defs::CQuickAccessItemButtonGuiDef{});
        return true;
    }
    if (name == "CPlayerGuiDef") {
        f(::fable::defs::CPlayerGuiDef{});
        return true;
    }
    if (name == "CPlayerInventoryDef") {
        f(::fable::defs::CPlayerInventoryDef{});
        return true;
    }
    if (name == "CPlayerMovementDef") {
        f(::fable::defs::CPlayerMovementDef{});
        return true;
    }
    if (name == "CPrizeScoreDef") {
        f(::fable::defs::CPrizeScoreDef{});
        return true;
    }
    if (name == "CQuestCardDef") {
        f(::fable::defs::CQuestCardDef{});
        return true;
    }
    if (name == "CRGBFloatColour") {
        f(::fable::defs::CRGBFloatColour{});
        return true;
    }
    if (name == "CRandomAppearanceMorph") {
        f(::fable::defs::CRandomAppearanceMorph{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CTextureMorphEntry") {
        f(::fable::defs::CRandomAppearanceMorph::CTextureMorphEntry{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CTextureMorphTable") {
        f(::fable::defs::CRandomAppearanceMorph::CTextureMorphTable{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart0Mesh") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart0Mesh{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart1Mesh") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart1Mesh{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart2Mesh") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart2Mesh{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart0") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart0{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart1") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart1{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CBodyPart2") {
        f(::fable::defs::CRandomAppearanceMorph::CBodyPart2{});
        return true;
    }
    if (name == "CRandomAppearanceMorph::CSkeletalMorphEntry") {
        f(::fable::defs::CRandomAppearanceMorph::CSkeletalMorphEntry{});
        return true;
    }
    if (name == "CReadableDef") {
        f(::fable::defs::CReadableDef{});
        return true;
    }
    if (name == "CRegionDef") {
        f(::fable::defs::CRegionDef{});
        return true;
    }
    if (name == "CRegionScriptDef") {
        f(::fable::defs::CRegionScriptDef{});
        return true;
    }
    if (name == "NGraphicAppearance::CReplaceableMeshEntry") {
        f(::fable::defs::NGraphicAppearance::CReplaceableMeshEntry{});
        return true;
    }
    if (name == "NGraphicAppearance::CReplaceableMeshes") {
        f(::fable::defs::NGraphicAppearance::CReplaceableMeshes{});
        return true;
    }
    if (name == "CReplaceableMeshDef") {
        f(::fable::defs::CReplaceableMeshDef{});
        return true;
    }
    if (name == "CResurrectionItemDef") {
        f(::fable::defs::CResurrectionItemDef{});
        return true;
    }
    if (name == "CReverbEnvironmentEntry") {
        f(::fable::defs::CReverbEnvironmentEntry{});
        return true;
    }
    if (name == "CRumbleDef") {
        f(::fable::defs::CRumbleDef{});
        return true;
    }
    if (name == "CScorpionKingBattleDef") {
        f(::fable::defs::CScorpionKingBattleDef{});
        return true;
    }
    if (name == "CScriptDef") {
        f(::fable::defs::CScriptDef{});
        return true;
    }
    if (name == "CShipDef") {
        f(::fable::defs::CShipDef{});
        return true;
    }
    if (name == "CShopItem") {
        f(::fable::defs::CShopItem{});
        return true;
    }
    if (name == "CShopItemDef") {
        f(::fable::defs::CShopItemDef{});
        return true;
    }
    if (name == "CShopDef") {
        f(::fable::defs::CShopDef{});
        return true;
    }
    if (name == "CShoveHaPennyRegionDef") {
        f(::fable::defs::CShoveHaPennyRegionDef{});
        return true;
    }
    if (name == "CSimBuildingDef") {
        f(::fable::defs::CSimBuildingDef{});
        return true;
    }
    if (name == "CSimVoice") {
        f(::fable::defs::CSimVoice{});
        return true;
    }
    if (name == "CSimVoicesDef") {
        f(::fable::defs::CSimVoicesDef{});
        return true;
    }
    if (name == "CSkeletalMorphDef") {
        f(::fable::defs::CSkeletalMorphDef{});
        return true;
    }
    if (name == "CSkyDef") {
        f(::fable::defs::CSkyDef{});
        return true;
    }
    if (name == "CSmashableDef") {
        f(::fable::defs::CSmashableDef{});
        return true;
    }
    if (name == "CSmokeGeneratorDef") {
        f(::fable::defs::CSmokeGeneratorDef{});
        return true;
    }
    if (name == "CSnowTrollDef") {
        f(::fable::defs::CSnowTrollDef{});
        return true;
    }
    if (name == "CSoundAtmospheresDef") {
        f(::fable::defs::CSoundAtmospheresDef{});
        return true;
    }
    if (name == "CSoundBankEntry") {
        f(::fable::defs::CSoundBankEntry{});
        return true;
    }
    if (name == "CSoundDef") {
        f(::fable::defs::CSoundDef{});
        return true;
    }
    if (name == "CSoundThemeDef") {
        f(::fable::defs::CSoundThemeDef{});
        return true;
    }
    if (name == "CSoundThemeDef::CAtmosGroup") {
        f(::fable::defs::CSoundThemeDef::CAtmosGroup{});
        return true;
    }
    if (name == "CSpecialAbilitiesAssassinRushDef") {
        f(::fable::defs::CSpecialAbilitiesAssassinRushDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesBattleChargeDef") {
        f(::fable::defs::CSpecialAbilitiesBattleChargeDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesBerserkDef") {
        f(::fable::defs::CSpecialAbilitiesBerserkDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesBulletTimeDef") {
        f(::fable::defs::CSpecialAbilitiesBulletTimeDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesBurntEffectDef") {
        f(::fable::defs::CSpecialAbilitiesBurntEffectDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesCreatureTintDef") {
        f(::fable::defs::CSpecialAbilitiesCreatureTintDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesDivineWrathDef") {
        f(::fable::defs::CSpecialAbilitiesDivineWrathDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesDrainLifeDataDef") {
        f(::fable::defs::CSpecialAbilitiesDrainLifeDataDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesDrainLifeDef") {
        f(::fable::defs::CSpecialAbilitiesDrainLifeDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesDrunkennessDef") {
        f(::fable::defs::CSpecialAbilitiesDrunkennessDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesElectrocutedEffectDef") {
        f(::fable::defs::CSpecialAbilitiesElectrocutedEffectDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesEnflameDef") {
        f(::fable::defs::CSpecialAbilitiesEnflameDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesFireballSpellDef") {
        f(::fable::defs::CSpecialAbilitiesFireballSpellDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesForcePushDataDef") {
        f(::fable::defs::CSpecialAbilitiesForcePushDataDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesForcePushDef") {
        f(::fable::defs::CSpecialAbilitiesForcePushDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesGhostSwordDef") {
        f(::fable::defs::CSpecialAbilitiesGhostSwordDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesHealLifeDef") {
        f(::fable::defs::CSpecialAbilitiesHealLifeDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesLightningSpellDef") {
        f(::fable::defs::CSpecialAbilitiesLightningSpellDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesMultiArrowDef") {
        f(::fable::defs::CSpecialAbilitiesMultiArrowDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesMultiStrikeDef") {
        f(::fable::defs::CSpecialAbilitiesMultiStrikeDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesPhysicalShieldDef") {
        f(::fable::defs::CSpecialAbilitiesPhysicalShieldDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesSummonSpellDef") {
        f(::fable::defs::CSpecialAbilitiesSummonSpellDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesThunderLightningStormDef") {
        f(::fable::defs::CSpecialAbilitiesThunderLightningStormDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesTurncoatSpellDef") {
        f(::fable::defs::CSpecialAbilitiesTurncoatSpellDef{});
        return true;
    }
    if (name == "CSpecialAbilitiesUnholyPowerDef") {
        f(::fable::defs::CSpecialAbilitiesUnholyPowerDef{});
        return true;
    }
    if (name == "CSpecialEffectsStringMap") {
        f(::fable::defs::CSpecialEffectsStringMap{});
        return true;
    }
    if (name == "CSpecialEffectsDef") {
        f(::fable::defs::CSpecialEffectsDef{});
        return true;
    }
    if (name == "CSpotLightDef") {
        f(::fable::defs::CSpotLightDef{});
        return true;
    }
    if (name == "CStealthDef") {
        f(::fable::defs::CStealthDef{});
        return true;
    }
    if (name == "CStockItemDef") {
        f(::fable::defs::CStockItemDef{});
        return true;
    }
    if (name == "CSummonDef") {
        f(::fable::defs::CSummonDef{});
        return true;
    }
    if (name == "CSummonableCreatureDef") {
        f(::fable::defs::CSummonableCreatureDef{});
        return true;
    }
    if (name == "CSummonerDef") {
        f(::fable::defs::CSummonerDef{});
        return true;
    }
    if (name == "CTCNoiseDef") {
        f(::fable::defs::CTCNoiseDef{});
        return true;
    }
    if (name == "CTCVolumeContainmentTrackerDef") {
        f(::fable::defs::CTCVolumeContainmentTrackerDef{});
        return true;
    }
    if (name == "CTargetingDef") {
        f(::fable::defs::CTargetingDef{});
        return true;
    }
    if (name == "CTattooDef") {
        f(::fable::defs::CTattooDef{});
        return true;
    }
    if (name == "CTavernDef") {
        f(::fable::defs::CTavernDef{});
        return true;
    }
    if (name == "CTavernGameCardBaseDef") {
        f(::fable::defs::CTavernGameCardBaseDef{});
        return true;
    }
    if (name == "CTavernGameCoinBaseDef") {
        f(::fable::defs::CTavernGameCoinBaseDef{});
        return true;
    }
    if (name == "CTavernGameCoinGolfDef") {
        f(::fable::defs::CTavernGameCoinGolfDef{});
        return true;
    }
    if (name == "CTavernGameDef") {
        f(::fable::defs::CTavernGameDef{});
        return true;
    }
    if (name == "CTavernGameShoveHaPennyDef") {
        f(::fable::defs::CTavernGameShoveHaPennyDef{});
        return true;
    }
    if (name == "CTavernGameSpotTheAdditionDef") {
        f(::fable::defs::CTavernGameSpotTheAdditionDef{});
        return true;
    }
    if (name == "CTavernTableDef") {
        f(::fable::defs::CTavernTableDef{});
        return true;
    }
    if (name == "CTeleporterDef") {
        f(::fable::defs::CTeleporterDef{});
        return true;
    }
    if (name == "CTextureReplacementDef") {
        f(::fable::defs::CTextureReplacementDef{});
        return true;
    }
    if (name == "CTextureReplacementDef::CEntry") {
        f(::fable::defs::CTextureReplacementDef::CEntry{});
        return true;
    }
    if (name == "CThingComponentSet") {
        f(::fable::defs::CThingComponentSet{});
        return true;
    }
    if (name == "CThingComponentSet::CEntry") {
        f(::fable::defs::CThingComponentSet::CEntry{});
        return true;
    }
    if (name == "CThingBaseDef") {
        f(::fable::defs::CThingBaseDef{});
        return true;
    }
    if (name == "CThingBuildingDef") {
        f(::fable::defs::CThingBuildingDef{});
        return true;
    }
    if (name == "CThingCreatureDef") {
        f(::fable::defs::CThingCreatureDef{});
        return true;
    }
    if (name == "CThingDef") {
        f(::fable::defs::CThingDef{});
        return true;
    }
    if (name == "CThingDrainLifeShotDef") {
        f(::fable::defs::CThingDrainLifeShotDef{});
        return true;
    }
    if (name == "CThingGroupDef") {
        f(::fable::defs::CThingGroupDef{});
        return true;
    }
    if (name == "CThingHolySiteDef") {
        f(::fable::defs::CThingHolySiteDef{});
        return true;
    }
    if (name == "CThingMarkerDef") {
        f(::fable::defs::CThingMarkerDef{});
        return true;
    }
    if (name == "CThingMultiArrowShotDef") {
        f(::fable::defs::CThingMultiArrowShotDef{});
        return true;
    }
    if (name == "CThingNoiseDef") {
        f(::fable::defs::CThingNoiseDef{});
        return true;
    }
    if (name == "CThingObjectDef") {
        f(::fable::defs::CThingObjectDef{});
        return true;
    }
    if (name == "CThingPhysicalSwitchDef") {
        f(::fable::defs::CThingPhysicalSwitchDef{});
        return true;
    }
    if (name == "CThingShotDef") {
        f(::fable::defs::CThingShotDef{});
        return true;
    }
    if (name == "CThingSwitchDef") {
        f(::fable::defs::CThingSwitchDef{});
        return true;
    }
    if (name == "CThingVillageDef") {
        f(::fable::defs::CThingVillageDef{});
        return true;
    }
    if (name == "CThunderBattleDef") {
        f(::fable::defs::CThunderBattleDef{});
        return true;
    }
    if (name == "CTimeAppearanceFadeDef") {
        f(::fable::defs::CTimeAppearanceFadeDef{});
        return true;
    }
    if (name == "CTrapDef") {
        f(::fable::defs::CTrapDef{});
        return true;
    }
    if (name == "CTrollBattleDef") {
        f(::fable::defs::CTrollBattleDef{});
        return true;
    }
    if (name == "CTrophyDef") {
        f(::fable::defs::CTrophyDef{});
        return true;
    }
    if (name == "CTurncoatDef") {
        f(::fable::defs::CTurncoatDef{});
        return true;
    }
    if (name == "CVillageDef") {
        f(::fable::defs::CVillageDef{});
        return true;
    }
    if (name == "CVillageMemberDef") {
        f(::fable::defs::CVillageMemberDef{});
        return true;
    }
    if (name == "CVillagePeopleDef") {
        f(::fable::defs::CVillagePeopleDef{});
        return true;
    }
    if (name == "CVillagerInteractionsDef") {
        f(::fable::defs::CVillagerInteractionsDef{});
        return true;
    }
    if (name == "CWallMountEffectsDef") {
        f(::fable::defs::CWallMountEffectsDef{});
        return true;
    }
    if (name == "CWaspQueenBattleDef") {
        f(::fable::defs::CWaspQueenBattleDef{});
        return true;
    }
    if (name == "CWeaponTrailGraphicSet") {
        f(::fable::defs::CWeaponTrailGraphicSet{});
        return true;
    }
    if (name == "CWeaponDef") {
        f(::fable::defs::CWeaponDef{});
        return true;
    }
    if (name == "CWhisperBattleDef") {
        f(::fable::defs::CWhisperBattleDef{});
        return true;
    }
    if (name == "CWifeDef") {
        f(::fable::defs::CWifeDef{});
        return true;
    }
    if (name == "CWillResponseDef") {
        f(::fable::defs::CWillResponseDef{});
        return true;
    }
    if (name == "CWoundMorphs") {
        f(::fable::defs::CWoundMorphs{});
        return true;
    }
    if (name == "CWoundMorphs::CEntry") {
        f(::fable::defs::CWoundMorphs::CEntry{});
        return true;
    }
    if (name == "NUISystem::CUIStateDef") {
        f(::fable::defs::NUISystem::CUIStateDef{});
        return true;
    }
    if (name == "NUISystem::CUIDef") {
        f(::fable::defs::NUISystem::CUIDef{});
        return true;
    }
    if (name == "NUISystem::CUIIconsDef") {
        f(::fable::defs::NUISystem::CUIIconsDef{});
        return true;
    }
    if (name == "NUISystem::CUILocaleGraphicsDef") {
        f(::fable::defs::NUISystem::CUILocaleGraphicsDef{});
        return true;
    }
    if (name == "NUISystem::CUIMiscThingsDef") {
        f(::fable::defs::NUISystem::CUIMiscThingsDef{});
        return true;
    }
    return false;
}

} // namespace fable::defs::compiler

#endif // FABLE_DEFS_COMPILER_MODEL_REGISTRY_HPP
