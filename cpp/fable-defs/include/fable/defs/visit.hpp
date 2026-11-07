// Field reflection for the def data model. Generated from classes.hpp.
#ifndef FABLE_DEFS_VISIT_HPP
#define FABLE_DEFS_VISIT_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fable/defs.hpp>

namespace fable {
namespace defs {
namespace detail {

/// A mutable handle to a bit-field, which cannot bind to a reference.
template <class T>
class BitfieldRef {
public:
    using Getter = bool (*)(const T &);
    using Setter = void (*)(T &, bool);
    BitfieldRef(T &value, Getter get, Setter set)
        : value_(&value), get_(get), set_(set) {}
    bool get() const { return get_(*value_); }
    void set(bool value) { set_(*value_, value); }

private:
    T *value_;
    Getter get_;
    Setter set_;
};

template <class T, class Get, class Set>
BitfieldRef<T> bitfield(T &value, Get get, Set set) {
    return BitfieldRef<T>(value, get, set);
}

} // namespace detail

/// Whether a type is one of the observed model classes.
template <class T> struct is_model_class : std::false_type {};
template <> struct is_model_class<C2DBoxF> : std::true_type {};
template <> struct is_model_class<C2DBoxI> : std::true_type {};
template <> struct is_model_class<C2DVector> : std::true_type {};
template <> struct is_model_class<C3DVector> : std::true_type {};
template <> struct is_model_class<CAICreatureWillPowerIndicatorDef> : std::true_type {};
template <> struct is_model_class<CAIScratchpadDef> : std::true_type {};
template <> struct is_model_class<CAbilityButtonGuiDef> : std::true_type {};
template <> struct is_model_class<CAbilityDef> : std::true_type {};
template <> struct is_model_class<CAbilityTrackLevelDef> : std::true_type {};
template <> struct is_model_class<CAbilityTrackDef> : std::true_type {};
template <> struct is_model_class<CActionInputControl> : std::true_type {};
template <> struct is_model_class<CActionUseDef> : std::true_type {};
template <> struct is_model_class<CActivateQuestDef> : std::true_type {};
template <> struct is_model_class<CAnimComponentBase> : std::true_type {};
template <> struct is_model_class<CAnimComponentMap> : std::true_type {};
template <> struct is_model_class<CAnimationComponent> : std::true_type {};
template <> struct is_model_class<CAnimationEntry> : std::true_type {};
template <> struct is_model_class<CAnimationSet> : std::true_type {};
template <> struct is_model_class<CAnimatingObjectDef> : std::true_type {};
template <> struct is_model_class<CEngineGraphic> : std::true_type {};
template <> struct is_model_class<CAppearanceDef> : std::true_type {};
template <> struct is_model_class<CAppearanceModifierGraphics> : std::true_type {};
template <> struct is_model_class<CAppearanceModifierGraphics::CEntry> : std::true_type {};
template <> struct is_model_class<CAppearanceModifierDef> : std::true_type {};
template <> struct is_model_class<CAppearanceModifierScalingDef> : std::true_type {};
template <> struct is_model_class<CAreaOfEffectAttackDef> : std::true_type {};
template <> struct is_model_class<CArenaCreatureDef> : std::true_type {};
template <> struct is_model_class<CArenaWaveDef> : std::true_type {};
template <> struct is_model_class<CArenaRoundDef> : std::true_type {};
template <> struct is_model_class<CArmourDef> : std::true_type {};
template <> struct is_model_class<CAtmosBankEntry> : std::true_type {};
template <> struct is_model_class<CAttackHistoryCombo> : std::true_type {};
template <> struct is_model_class<CAttackPatternDef> : std::true_type {};
template <> struct is_model_class<CAugmentationDef> : std::true_type {};
template <> struct is_model_class<CBalverineBattleDef> : std::true_type {};
template <> struct is_model_class<CBedDef> : std::true_type {};
template <> struct is_model_class<CBettingDef> : std::true_type {};
template <> struct is_model_class<CBlendedParticleEffectSet> : std::true_type {};
template <> struct is_model_class<CBlurredAttitudesDef> : std::true_type {};
template <> struct is_model_class<CBoastingPodiumDef> : std::true_type {};
template <> struct is_model_class<CBonusItemDef> : std::true_type {};
template <> struct is_model_class<CBossDef> : std::true_type {};
template <> struct is_model_class<CBrainBehaviour> : std::true_type {};
template <> struct is_model_class<CBrainUpdateZone> : std::true_type {};
template <> struct is_model_class<CBrainDef> : std::true_type {};
template <> struct is_model_class<CBriarRoseDef> : std::true_type {};
template <> struct is_model_class<CBuyHouseDef> : std::true_type {};
template <> struct is_model_class<CBuyableHouseDef> : std::true_type {};
template <> struct is_model_class<CCameraCollisionDef> : std::true_type {};
template <> struct is_model_class<CCameraManagerDef> : std::true_type {};
template <> struct is_model_class<CCameraManagerSetDef> : std::true_type {};
template <> struct is_model_class<CCameraModeDef> : std::true_type {};
template <> struct is_model_class<CCardDef> : std::true_type {};
template <> struct is_model_class<CCardPositionsDef> : std::true_type {};
template <> struct is_model_class<CCarriedReadableDef> : std::true_type {};
template <> struct is_model_class<CCarrySlotDef> : std::true_type {};
template <> struct is_model_class<CCarryableDef> : std::true_type {};
template <> struct is_model_class<CCarryingDef> : std::true_type {};
template <> struct is_model_class<CChestDef> : std::true_type {};
template <> struct is_model_class<CClockDef> : std::true_type {};
template <> struct is_model_class<CCoinGameObstacleDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityData> : std::true_type {};
template <> struct is_model_class<CCombatAbilityMeleeAttackData> : std::true_type {};
template <> struct is_model_class<CCombatAbilityAttackBase> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockCounterAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockingData> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockDefBase> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockHeavyWeaponAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockLightWeaponAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockProjectileWeaponAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityBlockUnarmedAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityFlourishCounterAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityGetHitCounterAttackDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityProjectileAttackData> : std::true_type {};
template <> struct is_model_class<CCombatAbilityStrafingData> : std::true_type {};
template <> struct is_model_class<CCombatAbilityStrafeDef> : std::true_type {};
template <> struct is_model_class<CCombatAbilityUseProjectileWeaponDef> : std::true_type {};
template <> struct is_model_class<CCombatDialogueDef> : std::true_type {};
template <> struct is_model_class<CCombatSequenceDef> : std::true_type {};
template <> struct is_model_class<CCombatTypeDef> : std::true_type {};
template <> struct is_model_class<CComboMultiplierData> : std::true_type {};
template <> struct is_model_class<CConfigOptionsDefaultsDef> : std::true_type {};
template <> struct is_model_class<CContainerRewardHeroDef> : std::true_type {};
template <> struct is_model_class<CContextSensitiveItemDef> : std::true_type {};
template <> struct is_model_class<CControlsDef> : std::true_type {};
template <> struct is_model_class<CConversation> : std::true_type {};
template <> struct is_model_class<CCoopSpiritDef> : std::true_type {};
template <> struct is_model_class<CCrateStackDef> : std::true_type {};
template <> struct is_model_class<CCreatureAbilityDef> : std::true_type {};
template <> struct is_model_class<CFloatRange> : std::true_type {};
template <> struct is_model_class<CHeroAbilityStatWeight> : std::true_type {};
template <> struct is_model_class<CCreatureDef> : std::true_type {};
template <> struct is_model_class<CCreatureGenerationFamilyDef> : std::true_type {};
template <> struct is_model_class<CCreatureGeneratorDef> : std::true_type {};
template <> struct is_model_class<CCreatureModeDef> : std::true_type {};
template <> struct is_model_class<CCreatureNavigationDef> : std::true_type {};
template <> struct is_model_class<CCreatureStatsDef> : std::true_type {};
template <> struct is_model_class<CCutsceneDef> : std::true_type {};
template <> struct is_model_class<CDecapitationDef> : std::true_type {};
template <> struct is_model_class<CDegradableInfo> : std::true_type {};
template <> struct is_model_class<CDegradableDef> : std::true_type {};
template <> struct is_model_class<CDegradableDef::CIsHealth1GTHealth2> : std::true_type {};
template <> struct is_model_class<CDegradableDef::CSetEngineGraphicType> : std::true_type {};
template <> struct is_model_class<CDoorDef> : std::true_type {};
template <> struct is_model_class<CDragonActionHoverDef> : std::true_type {};
template <> struct is_model_class<CDragonActionNapalmDef> : std::true_type {};
template <> struct is_model_class<CDragonActionSwoopDef> : std::true_type {};
template <> struct is_model_class<CDrunkennessDef> : std::true_type {};
template <> struct is_model_class<CEnemyDef> : std::true_type {};
template <> struct is_model_class<CEngineDef> : std::true_type {};
template <> struct is_model_class<CEngineLocalDetailObjectDef> : std::true_type {};
template <> struct is_model_class<CEngineLocalDetailLayerDef> : std::true_type {};
template <> struct is_model_class<CEngineLocalDetailGeneratorDef> : std::true_type {};
template <> struct is_model_class<CEngineThemeDef> : std::true_type {};
template <> struct is_model_class<CEngineThemeGroupDef> : std::true_type {};
template <> struct is_model_class<CEngineVideoOptionsDef> : std::true_type {};
template <> struct is_model_class<CSoundPair> : std::true_type {};
template <> struct is_model_class<CSoundMap> : std::true_type {};
template <> struct is_model_class<CEntitySoundDef> : std::true_type {};
template <> struct is_model_class<CEnvironmentDef> : std::true_type {};
template <> struct is_model_class<CEnvironmentThemeDef> : std::true_type {};
template <> struct is_model_class<CRGBColour> : std::true_type {};
template <> struct is_model_class<CEnvironmentThemeDaySetDef> : std::true_type {};
template <> struct is_model_class<CExperienceDef> : std::true_type {};
template <> struct is_model_class<CExplodingObjectDef> : std::true_type {};
template <> struct is_model_class<CExplosionRing> : std::true_type {};
template <> struct is_model_class<CExplosionDef> : std::true_type {};
template <> struct is_model_class<CExplosiveTrailDef> : std::true_type {};
template <> struct is_model_class<CExpressionDef> : std::true_type {};
template <> struct is_model_class<CExpressionSet> : std::true_type {};
template <> struct is_model_class<CExpressionSet::CEntry> : std::true_type {};
template <> struct is_model_class<CExpressionSubDef> : std::true_type {};
template <> struct is_model_class<CFactionDef> : std::true_type {};
template <> struct is_model_class<CFeatDef> : std::true_type {};
template <> struct is_model_class<CFireEffectCreationSet> : std::true_type {};
template <> struct is_model_class<CFireHeartPatternDef> : std::true_type {};
template <> struct is_model_class<CFireballSpellLevelDef> : std::true_type {};
template <> struct is_model_class<CFireheartMinigameDef> : std::true_type {};
template <> struct is_model_class<CFishDef> : std::true_type {};
template <> struct is_model_class<CFishingDef> : std::true_type {};
template <> struct is_model_class<CFishingRodDef> : std::true_type {};
template <> struct is_model_class<CFlammableDef> : std::true_type {};
template <> struct is_model_class<CFrontEndDef> : std::true_type {};
template <> struct is_model_class<CGiftDef> : std::true_type {};
template <> struct is_model_class<NSpeechGainManager::CDialogueLayerDef> : std::true_type {};
template <> struct is_model_class<CGlobalDef> : std::true_type {};
template <> struct is_model_class<CGoldDef> : std::true_type {};
template <> struct is_model_class<CGuardDef> : std::true_type {};
template <> struct is_model_class<CGuiGraphicDef> : std::true_type {};
template <> struct is_model_class<CGuildMasterDef> : std::true_type {};
template <> struct is_model_class<CHairCardDef> : std::true_type {};
template <> struct is_model_class<CHasNameDef> : std::true_type {};
template <> struct is_model_class<CHeroAbilityDef> : std::true_type {};
template <> struct is_model_class<CHeroAbilityStatPackage> : std::true_type {};
template <> struct is_model_class<CHeroCentreDef> : std::true_type {};
template <> struct is_model_class<CHeroCombatDef> : std::true_type {};
template <> struct is_model_class<CHeroDef> : std::true_type {};
template <> struct is_model_class<CHeroStatIncreaseData> : std::true_type {};
template <> struct is_model_class<CHeroExperienceDef> : std::true_type {};
template <> struct is_model_class<CHeroMarriageDef> : std::true_type {};
template <> struct is_model_class<CParticleMorphs> : std::true_type {};
template <> struct is_model_class<CParticleMorphs::CEntry> : std::true_type {};
template <> struct is_model_class<CSkeletalMorphs> : std::true_type {};
template <> struct is_model_class<CSkeletalMorphs::CEntry> : std::true_type {};
template <> struct is_model_class<CTextureMorphEntry> : std::true_type {};
template <> struct is_model_class<CTextureMorphs> : std::true_type {};
template <> struct is_model_class<CHeroMorphDef> : std::true_type {};
template <> struct is_model_class<CHeroPostcardGeneratorDef> : std::true_type {};
template <> struct is_model_class<CHeroSoulsCreatureDef> : std::true_type {};
template <> struct is_model_class<CHeroSoulsWaveDef> : std::true_type {};
template <> struct is_model_class<CHeroSoulsRoundDef> : std::true_type {};
template <> struct is_model_class<CHeroSpecialMovementDef> : std::true_type {};
template <> struct is_model_class<CHeroStatsDef> : std::true_type {};
template <> struct is_model_class<CHeroSuitDef> : std::true_type {};
template <> struct is_model_class<CHeroTitleDef> : std::true_type {};
template <> struct is_model_class<CHighlightItemDef> : std::true_type {};
template <> struct is_model_class<NHitLocations::CPhysicalPrimitiveBase> : std::true_type {};
template <> struct is_model_class<CPhysicalPrimitiveInit> : std::true_type {};
template <> struct is_model_class<CHitLocationDef> : std::true_type {};
template <> struct is_model_class<CHitLocationsDef> : std::true_type {};
template <> struct is_model_class<CIdleSchedulerDef> : std::true_type {};
template <> struct is_model_class<CInterestingToVillagersDef> : std::true_type {};
template <> struct is_model_class<CInventoryCategoryDef> : std::true_type {};
template <> struct is_model_class<CInventoryDef> : std::true_type {};
template <> struct is_model_class<CInventoryItemDef> : std::true_type {};
template <> struct is_model_class<CJackDragonDef> : std::true_type {};
template <> struct is_model_class<CJackOfBladesBattleDef> : std::true_type {};
template <> struct is_model_class<CKickableDef> : std::true_type {};
template <> struct is_model_class<CKrakenDef> : std::true_type {};
template <> struct is_model_class<CKrakenTentacleDef> : std::true_type {};
template <> struct is_model_class<CLensFlareElementDef> : std::true_type {};
template <> struct is_model_class<CLightDef> : std::true_type {};
template <> struct is_model_class<CLightningDef> : std::true_type {};
template <> struct is_model_class<CLightningOrbDef> : std::true_type {};
template <> struct is_model_class<CLookDef> : std::true_type {};
template <> struct is_model_class<CMaterialDef> : std::true_type {};
template <> struct is_model_class<CMazeBattleDef> : std::true_type {};
template <> struct is_model_class<CMeleeCombatAbilityDef> : std::true_type {};
template <> struct is_model_class<CMeleeCombatKnockdownEffects> : std::true_type {};
template <> struct is_model_class<CMessageEventDef> : std::true_type {};
template <> struct is_model_class<CMultiStaticMeshEntryDef> : std::true_type {};
template <> struct is_model_class<CMultiStaticMeshDef> : std::true_type {};
template <> struct is_model_class<CMusicEntry> : std::true_type {};
template <> struct is_model_class<CMusicSetEntry> : std::true_type {};
template <> struct is_model_class<CNymphDef> : std::true_type {};
template <> struct is_model_class<CObjectAugmentationParticleSet> : std::true_type {};
template <> struct is_model_class<CObjectAugmentationsDef> : std::true_type {};
template <> struct is_model_class<CObjectFamilyEntry> : std::true_type {};
template <> struct is_model_class<CObjectFamilyDef> : std::true_type {};
template <> struct is_model_class<COccupiableDef> : std::true_type {};
template <> struct is_model_class<COpinionDeedReactionDef> : std::true_type {};
template <> struct is_model_class<COpinionTransientOffset> : std::true_type {};
template <> struct is_model_class<COpinionDeedEffectsDef> : std::true_type {};
template <> struct is_model_class<COpinionDeedMaskDef> : std::true_type {};
template <> struct is_model_class<COpinionOfHeroDef> : std::true_type {};
template <> struct is_model_class<COpinionPersonalityTraits> : std::true_type {};
template <> struct is_model_class<COpinionPersonalityDef> : std::true_type {};
template <> struct is_model_class<CReactionFrequencyTraits> : std::true_type {};
template <> struct is_model_class<CReactionFrequencyTraitsArray> : std::true_type {};
template <> struct is_model_class<CReactionMatchEntry> : std::true_type {};
template <> struct is_model_class<CReactionMatchList> : std::true_type {};
template <> struct is_model_class<COpinionReactionManagerDef> : std::true_type {};
template <> struct is_model_class<COpinionReactionMaskDef> : std::true_type {};
template <> struct is_model_class<COpinionSet> : std::true_type {};
template <> struct is_model_class<COpinionSourceDef> : std::true_type {};
template <> struct is_model_class<COracleMinigameDef> : std::true_type {};
template <> struct is_model_class<COverheadDisplayDef> : std::true_type {};
template <> struct is_model_class<NParticleAttachment::CParticleAttachmentInfo> : std::true_type {};
template <> struct is_model_class<CParticleAttacherDef> : std::true_type {};
template <> struct is_model_class<CPerceivedThingDef> : std::true_type {};
template <> struct is_model_class<CPhysicsDef> : std::true_type {};
template <> struct is_model_class<CPlayerDef> : std::true_type {};
template <> struct is_model_class<CQuickAccessItemButtonGuiDef> : std::true_type {};
template <> struct is_model_class<CPlayerGuiDef> : std::true_type {};
template <> struct is_model_class<CPlayerInventoryDef> : std::true_type {};
template <> struct is_model_class<CPlayerMovementDef> : std::true_type {};
template <> struct is_model_class<CPrizeScoreDef> : std::true_type {};
template <> struct is_model_class<CQuestCardDef> : std::true_type {};
template <> struct is_model_class<CRGBFloatColour> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CTextureMorphEntry> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CTextureMorphTable> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart0Mesh> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart1Mesh> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart2Mesh> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart0> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart1> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CBodyPart2> : std::true_type {};
template <> struct is_model_class<CRandomAppearanceMorph::CSkeletalMorphEntry> : std::true_type {};
template <> struct is_model_class<CReadableDef> : std::true_type {};
template <> struct is_model_class<CRegionDef> : std::true_type {};
template <> struct is_model_class<CRegionScriptDef> : std::true_type {};
template <> struct is_model_class<NGraphicAppearance::CReplaceableMeshEntry> : std::true_type {};
template <> struct is_model_class<NGraphicAppearance::CReplaceableMeshes> : std::true_type {};
template <> struct is_model_class<CReplaceableMeshDef> : std::true_type {};
template <> struct is_model_class<CResurrectionItemDef> : std::true_type {};
template <> struct is_model_class<CReverbEnvironmentEntry> : std::true_type {};
template <> struct is_model_class<CRumbleDef> : std::true_type {};
template <> struct is_model_class<CScorpionKingBattleDef> : std::true_type {};
template <> struct is_model_class<CScriptDef> : std::true_type {};
template <> struct is_model_class<CShipDef> : std::true_type {};
template <> struct is_model_class<CShopItem> : std::true_type {};
template <> struct is_model_class<CShopItemDef> : std::true_type {};
template <> struct is_model_class<CShopDef> : std::true_type {};
template <> struct is_model_class<CShoveHaPennyRegionDef> : std::true_type {};
template <> struct is_model_class<CSimBuildingDef> : std::true_type {};
template <> struct is_model_class<CSimVoice> : std::true_type {};
template <> struct is_model_class<CSimVoicesDef> : std::true_type {};
template <> struct is_model_class<CSkeletalMorphDef> : std::true_type {};
template <> struct is_model_class<CSkyDef> : std::true_type {};
template <> struct is_model_class<CSmashableDef> : std::true_type {};
template <> struct is_model_class<CSmokeGeneratorDef> : std::true_type {};
template <> struct is_model_class<CSnowTrollDef> : std::true_type {};
template <> struct is_model_class<CSoundAtmospheresDef> : std::true_type {};
template <> struct is_model_class<CSoundBankEntry> : std::true_type {};
template <> struct is_model_class<CSoundDef> : std::true_type {};
template <> struct is_model_class<CSoundThemeDef> : std::true_type {};
template <> struct is_model_class<CSoundThemeDef::CAtmosGroup> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesAssassinRushDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesBattleChargeDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesBerserkDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesBulletTimeDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesBurntEffectDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesCreatureTintDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesDivineWrathDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesDrainLifeDataDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesDrainLifeDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesDrunkennessDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesElectrocutedEffectDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesEnflameDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesFireballSpellDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesForcePushDataDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesForcePushDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesGhostSwordDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesHealLifeDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesLightningSpellDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesMultiArrowDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesMultiStrikeDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesPhysicalShieldDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesSummonSpellDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesThunderLightningStormDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesTurncoatSpellDef> : std::true_type {};
template <> struct is_model_class<CSpecialAbilitiesUnholyPowerDef> : std::true_type {};
template <> struct is_model_class<CSpecialEffectsStringMap> : std::true_type {};
template <> struct is_model_class<CSpecialEffectsDef> : std::true_type {};
template <> struct is_model_class<CSpotLightDef> : std::true_type {};
template <> struct is_model_class<CStealthDef> : std::true_type {};
template <> struct is_model_class<CStockItemDef> : std::true_type {};
template <> struct is_model_class<CSummonDef> : std::true_type {};
template <> struct is_model_class<CSummonableCreatureDef> : std::true_type {};
template <> struct is_model_class<CSummonerDef> : std::true_type {};
template <> struct is_model_class<CTCNoiseDef> : std::true_type {};
template <> struct is_model_class<CTCVolumeContainmentTrackerDef> : std::true_type {};
template <> struct is_model_class<CTargetingDef> : std::true_type {};
template <> struct is_model_class<CTattooDef> : std::true_type {};
template <> struct is_model_class<CTavernDef> : std::true_type {};
template <> struct is_model_class<CTavernGameCardBaseDef> : std::true_type {};
template <> struct is_model_class<CTavernGameCoinBaseDef> : std::true_type {};
template <> struct is_model_class<CTavernGameCoinGolfDef> : std::true_type {};
template <> struct is_model_class<CTavernGameDef> : std::true_type {};
template <> struct is_model_class<CTavernGameShoveHaPennyDef> : std::true_type {};
template <> struct is_model_class<CTavernGameSpotTheAdditionDef> : std::true_type {};
template <> struct is_model_class<CTavernTableDef> : std::true_type {};
template <> struct is_model_class<CTeleporterDef> : std::true_type {};
template <> struct is_model_class<CTextureReplacementDef> : std::true_type {};
template <> struct is_model_class<CTextureReplacementDef::CEntry> : std::true_type {};
template <> struct is_model_class<CThingComponentSet> : std::true_type {};
template <> struct is_model_class<CThingComponentSet::CEntry> : std::true_type {};
template <> struct is_model_class<CThingBaseDef> : std::true_type {};
template <> struct is_model_class<CThingBuildingDef> : std::true_type {};
template <> struct is_model_class<CThingCreatureDef> : std::true_type {};
template <> struct is_model_class<CThingDef> : std::true_type {};
template <> struct is_model_class<CThingDrainLifeShotDef> : std::true_type {};
template <> struct is_model_class<CThingGroupDef> : std::true_type {};
template <> struct is_model_class<CThingHolySiteDef> : std::true_type {};
template <> struct is_model_class<CThingMarkerDef> : std::true_type {};
template <> struct is_model_class<CThingMultiArrowShotDef> : std::true_type {};
template <> struct is_model_class<CThingNoiseDef> : std::true_type {};
template <> struct is_model_class<CThingObjectDef> : std::true_type {};
template <> struct is_model_class<CThingPhysicalSwitchDef> : std::true_type {};
template <> struct is_model_class<CThingShotDef> : std::true_type {};
template <> struct is_model_class<CThingSwitchDef> : std::true_type {};
template <> struct is_model_class<CThingVillageDef> : std::true_type {};
template <> struct is_model_class<CThunderBattleDef> : std::true_type {};
template <> struct is_model_class<CTimeAppearanceFadeDef> : std::true_type {};
template <> struct is_model_class<CTrapDef> : std::true_type {};
template <> struct is_model_class<CTrollBattleDef> : std::true_type {};
template <> struct is_model_class<CTrophyDef> : std::true_type {};
template <> struct is_model_class<CTurncoatDef> : std::true_type {};
template <> struct is_model_class<CVillageDef> : std::true_type {};
template <> struct is_model_class<CVillageMemberDef> : std::true_type {};
template <> struct is_model_class<CVillagePeopleDef> : std::true_type {};
template <> struct is_model_class<CVillagerInteractionsDef> : std::true_type {};
template <> struct is_model_class<CWallMountEffectsDef> : std::true_type {};
template <> struct is_model_class<CWaspQueenBattleDef> : std::true_type {};
template <> struct is_model_class<CWeaponTrailGraphicSet> : std::true_type {};
template <> struct is_model_class<CWeaponDef> : std::true_type {};
template <> struct is_model_class<CWhisperBattleDef> : std::true_type {};
template <> struct is_model_class<CWifeDef> : std::true_type {};
template <> struct is_model_class<CWillResponseDef> : std::true_type {};
template <> struct is_model_class<CWoundMorphs> : std::true_type {};
template <> struct is_model_class<CWoundMorphs::CEntry> : std::true_type {};
template <> struct is_model_class<NUISystem::CUIStateDef> : std::true_type {};
template <> struct is_model_class<NUISystem::CUIDef> : std::true_type {};
template <> struct is_model_class<NUISystem::CUIIconsDef> : std::true_type {};
template <> struct is_model_class<NUISystem::CUILocaleGraphicsDef> : std::true_type {};
template <> struct is_model_class<NUISystem::CUIMiscThingsDef> : std::true_type {};

template <class V> void visit_fields(C2DBoxF &value, V &&visitor);
template <class V> void visit_fields(C2DBoxI &value, V &&visitor);
template <class V> void visit_fields(C2DVector &value, V &&visitor);
template <class V> void visit_fields(C3DVector &value, V &&visitor);
template <class V> void visit_fields(CAICreatureWillPowerIndicatorDef &value, V &&visitor);
template <class V> void visit_fields(CAIScratchpadDef &value, V &&visitor);
template <class V> void visit_fields(CAbilityButtonGuiDef &value, V &&visitor);
template <class V> void visit_fields(CAbilityDef &value, V &&visitor);
template <class V> void visit_fields(CAbilityTrackLevelDef &value, V &&visitor);
template <class V> void visit_fields(CAbilityTrackDef &value, V &&visitor);
template <class V> void visit_fields(CActionInputControl &value, V &&visitor);
template <class V> void visit_fields(CActionUseDef &value, V &&visitor);
template <class V> void visit_fields(CActivateQuestDef &value, V &&visitor);
template <class V> void visit_fields(CAnimComponentBase &value, V &&visitor);
template <class V> void visit_fields(CAnimComponentMap &value, V &&visitor);
template <class V> void visit_fields(CAnimationComponent &value, V &&visitor);
template <class V> void visit_fields(CAnimationEntry &value, V &&visitor);
template <class V> void visit_fields(CAnimationSet &value, V &&visitor);
template <class V> void visit_fields(CAnimatingObjectDef &value, V &&visitor);
template <class V> void visit_fields(CEngineGraphic &value, V &&visitor);
template <class V> void visit_fields(CAppearanceDef &value, V &&visitor);
template <class V> void visit_fields(CAppearanceModifierGraphics &value, V &&visitor);
template <class V> void visit_fields(CAppearanceModifierGraphics::CEntry &value, V &&visitor);
template <class V> void visit_fields(CAppearanceModifierDef &value, V &&visitor);
template <class V> void visit_fields(CAppearanceModifierScalingDef &value, V &&visitor);
template <class V> void visit_fields(CAreaOfEffectAttackDef &value, V &&visitor);
template <class V> void visit_fields(CArenaCreatureDef &value, V &&visitor);
template <class V> void visit_fields(CArenaWaveDef &value, V &&visitor);
template <class V> void visit_fields(CArenaRoundDef &value, V &&visitor);
template <class V> void visit_fields(CArmourDef &value, V &&visitor);
template <class V> void visit_fields(CAtmosBankEntry &value, V &&visitor);
template <class V> void visit_fields(CAttackHistoryCombo &value, V &&visitor);
template <class V> void visit_fields(CAttackPatternDef &value, V &&visitor);
template <class V> void visit_fields(CAugmentationDef &value, V &&visitor);
template <class V> void visit_fields(CBalverineBattleDef &value, V &&visitor);
template <class V> void visit_fields(CBedDef &value, V &&visitor);
template <class V> void visit_fields(CBettingDef &value, V &&visitor);
template <class V> void visit_fields(CBlendedParticleEffectSet &value, V &&visitor);
template <class V> void visit_fields(CBlurredAttitudesDef &value, V &&visitor);
template <class V> void visit_fields(CBoastingPodiumDef &value, V &&visitor);
template <class V> void visit_fields(CBonusItemDef &value, V &&visitor);
template <class V> void visit_fields(CBossDef &value, V &&visitor);
template <class V> void visit_fields(CBrainBehaviour &value, V &&visitor);
template <class V> void visit_fields(CBrainUpdateZone &value, V &&visitor);
template <class V> void visit_fields(CBrainDef &value, V &&visitor);
template <class V> void visit_fields(CBriarRoseDef &value, V &&visitor);
template <class V> void visit_fields(CBuyHouseDef &value, V &&visitor);
template <class V> void visit_fields(CBuyableHouseDef &value, V &&visitor);
template <class V> void visit_fields(CCameraCollisionDef &value, V &&visitor);
template <class V> void visit_fields(CCameraManagerDef &value, V &&visitor);
template <class V> void visit_fields(CCameraManagerSetDef &value, V &&visitor);
template <class V> void visit_fields(CCameraModeDef &value, V &&visitor);
template <class V> void visit_fields(CCardDef &value, V &&visitor);
template <class V> void visit_fields(CCardPositionsDef &value, V &&visitor);
template <class V> void visit_fields(CCarriedReadableDef &value, V &&visitor);
template <class V> void visit_fields(CCarrySlotDef &value, V &&visitor);
template <class V> void visit_fields(CCarryableDef &value, V &&visitor);
template <class V> void visit_fields(CCarryingDef &value, V &&visitor);
template <class V> void visit_fields(CChestDef &value, V &&visitor);
template <class V> void visit_fields(CClockDef &value, V &&visitor);
template <class V> void visit_fields(CCoinGameObstacleDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityData &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityMeleeAttackData &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityAttackBase &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockCounterAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockingData &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockDefBase &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockHeavyWeaponAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockLightWeaponAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockProjectileWeaponAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityBlockUnarmedAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityFlourishCounterAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityGetHitCounterAttackDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityProjectileAttackData &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityStrafingData &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityStrafeDef &value, V &&visitor);
template <class V> void visit_fields(CCombatAbilityUseProjectileWeaponDef &value, V &&visitor);
template <class V> void visit_fields(CCombatDialogueDef &value, V &&visitor);
template <class V> void visit_fields(CCombatSequenceDef &value, V &&visitor);
template <class V> void visit_fields(CCombatTypeDef &value, V &&visitor);
template <class V> void visit_fields(CComboMultiplierData &value, V &&visitor);
template <class V> void visit_fields(CConfigOptionsDefaultsDef &value, V &&visitor);
template <class V> void visit_fields(CContainerRewardHeroDef &value, V &&visitor);
template <class V> void visit_fields(CContextSensitiveItemDef &value, V &&visitor);
template <class V> void visit_fields(CControlsDef &value, V &&visitor);
template <class V> void visit_fields(CConversation &value, V &&visitor);
template <class V> void visit_fields(CCoopSpiritDef &value, V &&visitor);
template <class V> void visit_fields(CCrateStackDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureAbilityDef &value, V &&visitor);
template <class V> void visit_fields(CFloatRange &value, V &&visitor);
template <class V> void visit_fields(CHeroAbilityStatWeight &value, V &&visitor);
template <class V> void visit_fields(CCreatureDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureGenerationFamilyDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureGeneratorDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureModeDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureNavigationDef &value, V &&visitor);
template <class V> void visit_fields(CCreatureStatsDef &value, V &&visitor);
template <class V> void visit_fields(CCutsceneDef &value, V &&visitor);
template <class V> void visit_fields(CDecapitationDef &value, V &&visitor);
template <class V> void visit_fields(CDegradableInfo &value, V &&visitor);
template <class V> void visit_fields(CDegradableDef &value, V &&visitor);
template <class V> void visit_fields(CDegradableDef::CIsHealth1GTHealth2 &value, V &&visitor);
template <class V> void visit_fields(CDegradableDef::CSetEngineGraphicType &value, V &&visitor);
template <class V> void visit_fields(CDoorDef &value, V &&visitor);
template <class V> void visit_fields(CDragonActionHoverDef &value, V &&visitor);
template <class V> void visit_fields(CDragonActionNapalmDef &value, V &&visitor);
template <class V> void visit_fields(CDragonActionSwoopDef &value, V &&visitor);
template <class V> void visit_fields(CDrunkennessDef &value, V &&visitor);
template <class V> void visit_fields(CEnemyDef &value, V &&visitor);
template <class V> void visit_fields(CEngineDef &value, V &&visitor);
template <class V> void visit_fields(CEngineLocalDetailObjectDef &value, V &&visitor);
template <class V> void visit_fields(CEngineLocalDetailLayerDef &value, V &&visitor);
template <class V> void visit_fields(CEngineLocalDetailGeneratorDef &value, V &&visitor);
template <class V> void visit_fields(CEngineThemeDef &value, V &&visitor);
template <class V> void visit_fields(CEngineThemeGroupDef &value, V &&visitor);
template <class V> void visit_fields(CEngineVideoOptionsDef &value, V &&visitor);
template <class V> void visit_fields(CSoundPair &value, V &&visitor);
template <class V> void visit_fields(CSoundMap &value, V &&visitor);
template <class V> void visit_fields(CEntitySoundDef &value, V &&visitor);
template <class V> void visit_fields(CEnvironmentDef &value, V &&visitor);
template <class V> void visit_fields(CEnvironmentThemeDef &value, V &&visitor);
template <class V> void visit_fields(CRGBColour &value, V &&visitor);
template <class V> void visit_fields(CEnvironmentThemeDaySetDef &value, V &&visitor);
template <class V> void visit_fields(CExperienceDef &value, V &&visitor);
template <class V> void visit_fields(CExplodingObjectDef &value, V &&visitor);
template <class V> void visit_fields(CExplosionRing &value, V &&visitor);
template <class V> void visit_fields(CExplosionDef &value, V &&visitor);
template <class V> void visit_fields(CExplosiveTrailDef &value, V &&visitor);
template <class V> void visit_fields(CExpressionDef &value, V &&visitor);
template <class V> void visit_fields(CExpressionSet &value, V &&visitor);
template <class V> void visit_fields(CExpressionSet::CEntry &value, V &&visitor);
template <class V> void visit_fields(CExpressionSubDef &value, V &&visitor);
template <class V> void visit_fields(CFactionDef &value, V &&visitor);
template <class V> void visit_fields(CFeatDef &value, V &&visitor);
template <class V> void visit_fields(CFireEffectCreationSet &value, V &&visitor);
template <class V> void visit_fields(CFireHeartPatternDef &value, V &&visitor);
template <class V> void visit_fields(CFireballSpellLevelDef &value, V &&visitor);
template <class V> void visit_fields(CFireheartMinigameDef &value, V &&visitor);
template <class V> void visit_fields(CFishDef &value, V &&visitor);
template <class V> void visit_fields(CFishingDef &value, V &&visitor);
template <class V> void visit_fields(CFishingRodDef &value, V &&visitor);
template <class V> void visit_fields(CFlammableDef &value, V &&visitor);
template <class V> void visit_fields(CFrontEndDef &value, V &&visitor);
template <class V> void visit_fields(CGiftDef &value, V &&visitor);
template <class V> void visit_fields(NSpeechGainManager::CDialogueLayerDef &value, V &&visitor);
template <class V> void visit_fields(CGlobalDef &value, V &&visitor);
template <class V> void visit_fields(CGoldDef &value, V &&visitor);
template <class V> void visit_fields(CGuardDef &value, V &&visitor);
template <class V> void visit_fields(CGuiGraphicDef &value, V &&visitor);
template <class V> void visit_fields(CGuildMasterDef &value, V &&visitor);
template <class V> void visit_fields(CHairCardDef &value, V &&visitor);
template <class V> void visit_fields(CHasNameDef &value, V &&visitor);
template <class V> void visit_fields(CHeroAbilityDef &value, V &&visitor);
template <class V> void visit_fields(CHeroAbilityStatPackage &value, V &&visitor);
template <class V> void visit_fields(CHeroCentreDef &value, V &&visitor);
template <class V> void visit_fields(CHeroCombatDef &value, V &&visitor);
template <class V> void visit_fields(CHeroDef &value, V &&visitor);
template <class V> void visit_fields(CHeroStatIncreaseData &value, V &&visitor);
template <class V> void visit_fields(CHeroExperienceDef &value, V &&visitor);
template <class V> void visit_fields(CHeroMarriageDef &value, V &&visitor);
template <class V> void visit_fields(CParticleMorphs &value, V &&visitor);
template <class V> void visit_fields(CParticleMorphs::CEntry &value, V &&visitor);
template <class V> void visit_fields(CSkeletalMorphs &value, V &&visitor);
template <class V> void visit_fields(CSkeletalMorphs::CEntry &value, V &&visitor);
template <class V> void visit_fields(CTextureMorphEntry &value, V &&visitor);
template <class V> void visit_fields(CTextureMorphs &value, V &&visitor);
template <class V> void visit_fields(CHeroMorphDef &value, V &&visitor);
template <class V> void visit_fields(CHeroPostcardGeneratorDef &value, V &&visitor);
template <class V> void visit_fields(CHeroSoulsCreatureDef &value, V &&visitor);
template <class V> void visit_fields(CHeroSoulsWaveDef &value, V &&visitor);
template <class V> void visit_fields(CHeroSoulsRoundDef &value, V &&visitor);
template <class V> void visit_fields(CHeroSpecialMovementDef &value, V &&visitor);
template <class V> void visit_fields(CHeroStatsDef &value, V &&visitor);
template <class V> void visit_fields(CHeroSuitDef &value, V &&visitor);
template <class V> void visit_fields(CHeroTitleDef &value, V &&visitor);
template <class V> void visit_fields(CHighlightItemDef &value, V &&visitor);
template <class V> void visit_fields(NHitLocations::CPhysicalPrimitiveBase &value, V &&visitor);
template <class V> void visit_fields(CPhysicalPrimitiveInit &value, V &&visitor);
template <class V> void visit_fields(CHitLocationDef &value, V &&visitor);
template <class V> void visit_fields(CHitLocationsDef &value, V &&visitor);
template <class V> void visit_fields(CIdleSchedulerDef &value, V &&visitor);
template <class V> void visit_fields(CInterestingToVillagersDef &value, V &&visitor);
template <class V> void visit_fields(CInventoryCategoryDef &value, V &&visitor);
template <class V> void visit_fields(CInventoryDef &value, V &&visitor);
template <class V> void visit_fields(CInventoryItemDef &value, V &&visitor);
template <class V> void visit_fields(CJackDragonDef &value, V &&visitor);
template <class V> void visit_fields(CJackOfBladesBattleDef &value, V &&visitor);
template <class V> void visit_fields(CKickableDef &value, V &&visitor);
template <class V> void visit_fields(CKrakenDef &value, V &&visitor);
template <class V> void visit_fields(CKrakenTentacleDef &value, V &&visitor);
template <class V> void visit_fields(CLensFlareElementDef &value, V &&visitor);
template <class V> void visit_fields(CLightDef &value, V &&visitor);
template <class V> void visit_fields(CLightningDef &value, V &&visitor);
template <class V> void visit_fields(CLightningOrbDef &value, V &&visitor);
template <class V> void visit_fields(CLookDef &value, V &&visitor);
template <class V> void visit_fields(CMaterialDef &value, V &&visitor);
template <class V> void visit_fields(CMazeBattleDef &value, V &&visitor);
template <class V> void visit_fields(CMeleeCombatAbilityDef &value, V &&visitor);
template <class V> void visit_fields(CMeleeCombatKnockdownEffects &value, V &&visitor);
template <class V> void visit_fields(CMessageEventDef &value, V &&visitor);
template <class V> void visit_fields(CMultiStaticMeshEntryDef &value, V &&visitor);
template <class V> void visit_fields(CMultiStaticMeshDef &value, V &&visitor);
template <class V> void visit_fields(CMusicEntry &value, V &&visitor);
template <class V> void visit_fields(CMusicSetEntry &value, V &&visitor);
template <class V> void visit_fields(CNymphDef &value, V &&visitor);
template <class V> void visit_fields(CObjectAugmentationParticleSet &value, V &&visitor);
template <class V> void visit_fields(CObjectAugmentationsDef &value, V &&visitor);
template <class V> void visit_fields(CObjectFamilyEntry &value, V &&visitor);
template <class V> void visit_fields(CObjectFamilyDef &value, V &&visitor);
template <class V> void visit_fields(COccupiableDef &value, V &&visitor);
template <class V> void visit_fields(COpinionDeedReactionDef &value, V &&visitor);
template <class V> void visit_fields(COpinionTransientOffset &value, V &&visitor);
template <class V> void visit_fields(COpinionDeedEffectsDef &value, V &&visitor);
template <class V> void visit_fields(COpinionDeedMaskDef &value, V &&visitor);
template <class V> void visit_fields(COpinionOfHeroDef &value, V &&visitor);
template <class V> void visit_fields(COpinionPersonalityTraits &value, V &&visitor);
template <class V> void visit_fields(COpinionPersonalityDef &value, V &&visitor);
template <class V> void visit_fields(CReactionFrequencyTraits &value, V &&visitor);
template <class V> void visit_fields(CReactionFrequencyTraitsArray &value, V &&visitor);
template <class V> void visit_fields(CReactionMatchEntry &value, V &&visitor);
template <class V> void visit_fields(CReactionMatchList &value, V &&visitor);
template <class V> void visit_fields(COpinionReactionManagerDef &value, V &&visitor);
template <class V> void visit_fields(COpinionReactionMaskDef &value, V &&visitor);
template <class V> void visit_fields(COpinionSet &value, V &&visitor);
template <class V> void visit_fields(COpinionSourceDef &value, V &&visitor);
template <class V> void visit_fields(COracleMinigameDef &value, V &&visitor);
template <class V> void visit_fields(COverheadDisplayDef &value, V &&visitor);
template <class V> void visit_fields(NParticleAttachment::CParticleAttachmentInfo &value, V &&visitor);
template <class V> void visit_fields(CParticleAttacherDef &value, V &&visitor);
template <class V> void visit_fields(CPerceivedThingDef &value, V &&visitor);
template <class V> void visit_fields(CPhysicsDef &value, V &&visitor);
template <class V> void visit_fields(CPlayerDef &value, V &&visitor);
template <class V> void visit_fields(CQuickAccessItemButtonGuiDef &value, V &&visitor);
template <class V> void visit_fields(CPlayerGuiDef &value, V &&visitor);
template <class V> void visit_fields(CPlayerInventoryDef &value, V &&visitor);
template <class V> void visit_fields(CPlayerMovementDef &value, V &&visitor);
template <class V> void visit_fields(CPrizeScoreDef &value, V &&visitor);
template <class V> void visit_fields(CQuestCardDef &value, V &&visitor);
template <class V> void visit_fields(CRGBFloatColour &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CTextureMorphEntry &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CTextureMorphTable &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart0Mesh &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart1Mesh &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart2Mesh &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart0 &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart1 &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CBodyPart2 &value, V &&visitor);
template <class V> void visit_fields(CRandomAppearanceMorph::CSkeletalMorphEntry &value, V &&visitor);
template <class V> void visit_fields(CReadableDef &value, V &&visitor);
template <class V> void visit_fields(CRegionDef &value, V &&visitor);
template <class V> void visit_fields(CRegionScriptDef &value, V &&visitor);
template <class V> void visit_fields(NGraphicAppearance::CReplaceableMeshEntry &value, V &&visitor);
template <class V> void visit_fields(NGraphicAppearance::CReplaceableMeshes &value, V &&visitor);
template <class V> void visit_fields(CReplaceableMeshDef &value, V &&visitor);
template <class V> void visit_fields(CResurrectionItemDef &value, V &&visitor);
template <class V> void visit_fields(CReverbEnvironmentEntry &value, V &&visitor);
template <class V> void visit_fields(CRumbleDef &value, V &&visitor);
template <class V> void visit_fields(CScorpionKingBattleDef &value, V &&visitor);
template <class V> void visit_fields(CScriptDef &value, V &&visitor);
template <class V> void visit_fields(CShipDef &value, V &&visitor);
template <class V> void visit_fields(CShopItem &value, V &&visitor);
template <class V> void visit_fields(CShopItemDef &value, V &&visitor);
template <class V> void visit_fields(CShopDef &value, V &&visitor);
template <class V> void visit_fields(CShoveHaPennyRegionDef &value, V &&visitor);
template <class V> void visit_fields(CSimBuildingDef &value, V &&visitor);
template <class V> void visit_fields(CSimVoice &value, V &&visitor);
template <class V> void visit_fields(CSimVoicesDef &value, V &&visitor);
template <class V> void visit_fields(CSkeletalMorphDef &value, V &&visitor);
template <class V> void visit_fields(CSkyDef &value, V &&visitor);
template <class V> void visit_fields(CSmashableDef &value, V &&visitor);
template <class V> void visit_fields(CSmokeGeneratorDef &value, V &&visitor);
template <class V> void visit_fields(CSnowTrollDef &value, V &&visitor);
template <class V> void visit_fields(CSoundAtmospheresDef &value, V &&visitor);
template <class V> void visit_fields(CSoundBankEntry &value, V &&visitor);
template <class V> void visit_fields(CSoundDef &value, V &&visitor);
template <class V> void visit_fields(CSoundThemeDef &value, V &&visitor);
template <class V> void visit_fields(CSoundThemeDef::CAtmosGroup &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesAssassinRushDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesBattleChargeDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesBerserkDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesBulletTimeDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesBurntEffectDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesCreatureTintDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesDivineWrathDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesDrainLifeDataDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesDrainLifeDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesDrunkennessDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesElectrocutedEffectDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesEnflameDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesFireballSpellDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesForcePushDataDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesForcePushDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesGhostSwordDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesHealLifeDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesLightningSpellDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesMultiArrowDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesMultiStrikeDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesPhysicalShieldDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesSummonSpellDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesThunderLightningStormDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesTurncoatSpellDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialAbilitiesUnholyPowerDef &value, V &&visitor);
template <class V> void visit_fields(CSpecialEffectsStringMap &value, V &&visitor);
template <class V> void visit_fields(CSpecialEffectsDef &value, V &&visitor);
template <class V> void visit_fields(CSpotLightDef &value, V &&visitor);
template <class V> void visit_fields(CStealthDef &value, V &&visitor);
template <class V> void visit_fields(CStockItemDef &value, V &&visitor);
template <class V> void visit_fields(CSummonDef &value, V &&visitor);
template <class V> void visit_fields(CSummonableCreatureDef &value, V &&visitor);
template <class V> void visit_fields(CSummonerDef &value, V &&visitor);
template <class V> void visit_fields(CTCNoiseDef &value, V &&visitor);
template <class V> void visit_fields(CTCVolumeContainmentTrackerDef &value, V &&visitor);
template <class V> void visit_fields(CTargetingDef &value, V &&visitor);
template <class V> void visit_fields(CTattooDef &value, V &&visitor);
template <class V> void visit_fields(CTavernDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameCardBaseDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameCoinBaseDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameCoinGolfDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameShoveHaPennyDef &value, V &&visitor);
template <class V> void visit_fields(CTavernGameSpotTheAdditionDef &value, V &&visitor);
template <class V> void visit_fields(CTavernTableDef &value, V &&visitor);
template <class V> void visit_fields(CTeleporterDef &value, V &&visitor);
template <class V> void visit_fields(CTextureReplacementDef &value, V &&visitor);
template <class V> void visit_fields(CTextureReplacementDef::CEntry &value, V &&visitor);
template <class V> void visit_fields(CThingComponentSet &value, V &&visitor);
template <class V> void visit_fields(CThingComponentSet::CEntry &value, V &&visitor);
template <class V> void visit_fields(CThingBaseDef &value, V &&visitor);
template <class V> void visit_fields(CThingBuildingDef &value, V &&visitor);
template <class V> void visit_fields(CThingCreatureDef &value, V &&visitor);
template <class V> void visit_fields(CThingDef &value, V &&visitor);
template <class V> void visit_fields(CThingDrainLifeShotDef &value, V &&visitor);
template <class V> void visit_fields(CThingGroupDef &value, V &&visitor);
template <class V> void visit_fields(CThingHolySiteDef &value, V &&visitor);
template <class V> void visit_fields(CThingMarkerDef &value, V &&visitor);
template <class V> void visit_fields(CThingMultiArrowShotDef &value, V &&visitor);
template <class V> void visit_fields(CThingNoiseDef &value, V &&visitor);
template <class V> void visit_fields(CThingObjectDef &value, V &&visitor);
template <class V> void visit_fields(CThingPhysicalSwitchDef &value, V &&visitor);
template <class V> void visit_fields(CThingShotDef &value, V &&visitor);
template <class V> void visit_fields(CThingSwitchDef &value, V &&visitor);
template <class V> void visit_fields(CThingVillageDef &value, V &&visitor);
template <class V> void visit_fields(CThunderBattleDef &value, V &&visitor);
template <class V> void visit_fields(CTimeAppearanceFadeDef &value, V &&visitor);
template <class V> void visit_fields(CTrapDef &value, V &&visitor);
template <class V> void visit_fields(CTrollBattleDef &value, V &&visitor);
template <class V> void visit_fields(CTrophyDef &value, V &&visitor);
template <class V> void visit_fields(CTurncoatDef &value, V &&visitor);
template <class V> void visit_fields(CVillageDef &value, V &&visitor);
template <class V> void visit_fields(CVillageMemberDef &value, V &&visitor);
template <class V> void visit_fields(CVillagePeopleDef &value, V &&visitor);
template <class V> void visit_fields(CVillagerInteractionsDef &value, V &&visitor);
template <class V> void visit_fields(CWallMountEffectsDef &value, V &&visitor);
template <class V> void visit_fields(CWaspQueenBattleDef &value, V &&visitor);
template <class V> void visit_fields(CWeaponTrailGraphicSet &value, V &&visitor);
template <class V> void visit_fields(CWeaponDef &value, V &&visitor);
template <class V> void visit_fields(CWhisperBattleDef &value, V &&visitor);
template <class V> void visit_fields(CWifeDef &value, V &&visitor);
template <class V> void visit_fields(CWillResponseDef &value, V &&visitor);
template <class V> void visit_fields(CWoundMorphs &value, V &&visitor);
template <class V> void visit_fields(CWoundMorphs::CEntry &value, V &&visitor);
template <class V> void visit_fields(NUISystem::CUIStateDef &value, V &&visitor);
template <class V> void visit_fields(NUISystem::CUIDef &value, V &&visitor);
template <class V> void visit_fields(NUISystem::CUIIconsDef &value, V &&visitor);
template <class V> void visit_fields(NUISystem::CUILocaleGraphicsDef &value, V &&visitor);
template <class V> void visit_fields(NUISystem::CUIMiscThingsDef &value, V &&visitor);

template <class V>
void visit_fields(C2DBoxF &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TLX", value.TLX);
    visitor("TLY", value.TLY);
    visitor("BRX", value.BRX);
    visitor("BRY", value.BRY);
}

template <class V>
void visit_fields(C2DBoxI &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TLX", value.TLX);
    visitor("TLY", value.TLY);
    visitor("BRX", value.BRX);
    visitor("BRY", value.BRY);
}

template <class V>
void visit_fields(C2DVector &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("X", value.X);
    visitor("Y", value.Y);
}

template <class V>
void visit_fields(C3DVector &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("X", value.X);
    visitor("Y", value.Y);
    visitor("Z", value.Z);
}

template <class V>
void visit_fields(CAICreatureWillPowerIndicatorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("RechargeTime", value.RechargeTime);
}

template <class V>
void visit_fields(CAIScratchpadDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ThankingPhrase", value.ThankingPhrase);
    visitor("IgnoringPhrase", value.IgnoringPhrase);
    visitor("WanderCentrePoint", value.WanderCentrePoint);
    visitor("WanderMinDistance", value.WanderMinDistance);
    visitor("WanderMaxDistance", value.WanderMaxDistance);
    visitor("GossipCounter", value.GossipCounter);
    visitor("MaxGossipPhrase", value.MaxGossipPhrase);
    visitor("WarningPhrase", value.WarningPhrase);
    visitor("BeerRequestPhrase", value.BeerRequestPhrase);
    visitor("ScriptingStateGroup", value.ScriptingStateGroup);
    visitor("MaxHeroReactionDistance", value.MaxHeroReactionDistance);
    visitor("ActionFrequency", value.ActionFrequency);
    visitor("ActionFrequencyVariation", value.ActionFrequencyVariation);
    visitor("Action", value.Action);
    visitor("FaceHeroForAction", value.FaceHeroForAction);
    visitor("TargetName", value.TargetName);
    visitor("FollowDistance", value.FollowDistance);
    visitor("AttackHeroOnSight", value.AttackHeroOnSight);
    visitor("TimeToSpendHarassingHero", value.TimeToSpendHarassingHero);
    visitor("CombatNearbyEnemyFleeingBreakOffRange", value.CombatNearbyEnemyFleeingBreakOffRange);
    visitor("CombatNearbyBreakOffRange", value.CombatNearbyBreakOffRange);
    visitor("StealStealableItems", value.StealStealableItems);
    visitor("RecoverStealableItems", value.RecoverStealableItems);
    visitor("TakeStealableItemToRandomDestination", value.TakeStealableItemToRandomDestination);
    visitor("KillSelfAndStealableItemAfterReachingDestination", value.KillSelfAndStealableItemAfterReachingDestination);
    visitor("AllowedToFollow", value.AllowedToFollow);
    visitor("TableName", value.TableName);
    visitor("SeatName", value.SeatName);
    visitor("DisableHeadLooking", value.DisableHeadLooking);
    visitor("IsPushableByHero", value.IsPushableByHero);
    visitor("LookForFiniteTime", value.LookForFiniteTime);
    visitor("AvoidRegionExits", value.AvoidRegionExits);
    visitor("TargetingDistanceOffset", value.TargetingDistanceOffset);
}

template <class V>
void visit_fields(CAbilityButtonGuiDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ControllerType", value.ControllerType);
    visitor("ControllerButton", value.ControllerButton);
    visitor("MouseButton", value.MouseButton);
    visitor("ButtonGraphic", value.ButtonGraphic);
    visitor("Position", value.Position);
    visitor("Offset", value.Offset);
}

template <class V>
void visit_fields(CAbilityDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Ability", value.Ability);
}

template <class V>
void visit_fields(CAbilityTrackLevelDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ExperienceCost", value.ExperienceCost);
    visitor("AbilityUnlocked", value.AbilityUnlocked);
    visitor("Description", value.Description);
}

template <class V>
void visit_fields(CAbilityTrackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ExperienceStat", value.ExperienceStat);
    visitor("Abilities", value.Abilities);
    visitor("MaxedOutDescription", value.MaxedOutDescription);
}

template <class V>
void visit_fields(CActionInputControl &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GameAction", value.GameAction);
    visitor("ControllerType", value.ControllerType);
    visitor("KeyboardKey", value.KeyboardKey);
    visitor("XboxButton", value.XboxButton);
    visitor("MouseButton", value.MouseButton);
    visitor("ControlDirection", value.ControlDirection);
}

template <class V>
void visit_fields(CActionUseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AnimationGroup", value.AnimationGroup);
    visitor("SnapToActionPoint", value.SnapToActionPoint);
    visitor("IgnoreActionPoints", value.IgnoreActionPoints);
    visitor("TutorialCategory", value.TutorialCategory);
}

template <class V>
void visit_fields(CActivateQuestDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ScriptName", value.ScriptName);
    visitor("LoadResources", value.LoadResources);
}

template <class V>
void visit_fields(CAnimComponentBase &value, V &&visitor) {
    (void)value;
    (void)visitor;
}

template <class V>
void visit_fields(CAnimComponentMap &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Map", value.Map);
}

template <class V>
void visit_fields(CAnimationComponent &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Tag", value.Tag);
    visitor("Flags", value.Flags);
    visitor("TransitionInTime", value.TransitionInTime);
    visitor("Delay", value.Delay);
    visitor("StartHandedness", value.StartHandedness);
    visitor("EndHandedness", value.EndHandedness);
    visitor("ComboStage", value.ComboStage);
    visitor("ComboId", value.ComboId);
    visitor("RecoilAnimIndex", value.RecoilAnimIndex);
    visitor("MeleeFlourish", value.MeleeFlourish);
    visitor("MeleeKnockdown", value.MeleeKnockdown);
    visitor("TargetOffset", value.TargetOffset);
    visitor("TargetOffsetVectorX", value.TargetOffsetVectorX);
    visitor("TargetOffsetVectorY", value.TargetOffsetVectorY);
    visitor("AnimationSpeedMultiplier", value.AnimationSpeedMultiplier);
    visitor("Mode", value.Mode);
    visitor("NextAnimName", value.NextAnimName);
    visitor("NextFilter", value.NextFilter);
    visitor("ResponseAnimName", value.ResponseAnimName);
}

template <class V>
void visit_fields(CAnimationEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BankIndex", value.BankIndex);
    visitor("AnimName", value.AnimName);
    visitor("GroupName", value.GroupName);
    visitor("Components", value.Components);
}

template <class V>
void visit_fields(CAnimationSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Anims", value.Anims);
    visitor("DefaultFlags", value.DefaultFlags);
    visitor("DefaultTransitionTime", value.DefaultTransitionTime);
    visitor("DefaultDelay", value.DefaultDelay);
    visitor("DefaultGroup", value.DefaultGroup);
}

template <class V>
void visit_fields(CAnimatingObjectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Animation", value.Animation);
}

template <class V>
void visit_fields(CEngineGraphic &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("BankIndex", value.BankIndex);
    visitor("AnimStep", value.AnimStep);
    visitor("RenderSizeX", value.RenderSizeX);
    visitor("AdditiveAlpha", value.AdditiveAlpha);
}

template <class V>
void visit_fields(CAppearanceDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Graphic", value.Graphic);
    visitor("Animation", value.Animation);
    visitor("OutlineEffectAlphaRef", value.OutlineEffectAlphaRef);
    visitor("OutlineEffectAlphaBias", value.OutlineEffectAlphaBias);
    visitor("OverrideLightingChannel", value.OverrideLightingChannel);
    visitor("LightingChannel", value.LightingChannel);
    visitor("FadeOutWhenCloseGraphic", value.FadeOutWhenCloseGraphic);
    visitor("HasFadeOutWhenCloseGraphic", value.HasFadeOutWhenCloseGraphic);
    visitor("FadeOutGraphicRenderFadeDistance", value.FadeOutGraphicRenderFadeDistance);
    visitor("FadeOutGraphicNearRenderFadeDistance", value.FadeOutGraphicNearRenderFadeDistance);
    visitor("FadeOutGraphicNearRenderClipDistance", value.FadeOutGraphicNearRenderClipDistance);
}

template <class V>
void visit_fields(CAppearanceModifierGraphics &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Graphics", value.Graphics);
}

template <class V>
void visit_fields(CAppearanceModifierGraphics::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("MorphType", value.MorphType);
    visitor("BankIndex", value.BankIndex);
    visitor("RenderSizeX", value.RenderSizeX);
    visitor("StartMorphStrength", value.StartMorphStrength);
    visitor("EndMorphStrength", value.EndMorphStrength);
}

template <class V>
void visit_fields(CAppearanceModifierDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HideMaterials", value.HideMaterials);
    visitor("MorphGraphics", value.MorphGraphics);
    visitor("CoversBodyAreaFlags", value.CoversBodyAreaFlags);
    visitor("IsRemoveable", value.IsRemoveable);
    visitor("HeroSuit", value.HeroSuit);
    visitor("SuitPart", value.SuitPart);
    visitor("AppearanceType", value.AppearanceType);
    visitor("SoundRadiusMultiplier", value.SoundRadiusMultiplier);
    visitor("VisibilityMultiplier", value.VisibilityMultiplier);
    visitor("ArmourDefIndex", value.ArmourDefIndex);
    visitor("OveralProtectionWeighting", value.OveralProtectionWeighting);
    visitor("Scariness", value.Scariness);
    visitor("Attractiveness", value.Attractiveness);
    visitor("Goodstrength", value.Goodstrength);
}

template <class V>
void visit_fields(CAppearanceModifierScalingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Attractiveness", value.Attractiveness);
    visitor("Scariness", value.Scariness);
    visitor("Goodstrength", value.Goodstrength);
}

template <class V>
void visit_fields(CAreaOfEffectAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TrapIndex", value.TrapIndex);
    visitor("ExplosionIndex", value.ExplosionIndex);
    visitor("ObstructionRadius", value.ObstructionRadius);
    visitor("ObstructionLifeTime", value.ObstructionLifeTime);
}

template <class V>
void visit_fields(CArenaCreatureDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CreatureType", value.CreatureType);
    visitor("NumCreatures", value.NumCreatures);
    visitor("HUDType", value.HUDType);
    visitor("DeathScore", value.DeathScore);
}

template <class V>
void visit_fields(CArenaWaveDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumWaveCreatures", value.NumWaveCreatures);
    visitor("Creatures", value.Creatures);
    visitor("ShortWave", value.ShortWave);
}

template <class V>
void visit_fields(CArenaRoundDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumWaves", value.NumWaves);
    visitor("Waves", value.Waves);
}

template <class V>
void visit_fields(CArmourDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AugmentationResponse", value.AugmentationResponse);
    visitor("DamageTypeResponse", value.DamageTypeResponse);
    visitor("ArmourThresholds", value.ArmourThresholds);
    visitor("AllHitsKnockdown", value.AllHitsKnockdown);
    visitor("AllHitsNegated", value.AllHitsNegated);
    visitor("AllHitsCauseRecoil", value.AllHitsCauseRecoil);
    visitor("DamageMaterial", value.DamageMaterial);
    visitor("BlockMaterial", value.BlockMaterial);
}

template <class V>
void visit_fields(CAtmosBankEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ID", value.ID);
    visitor("Filename", value.Filename);
}

template <class V>
void visit_fields(CAttackHistoryCombo &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AttackCombo", value.AttackCombo);
    visitor("Multiplier", value.Multiplier);
}

template <class V>
void visit_fields(CAttackPatternDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AttackPattern", value.AttackPattern);
}

template <class V>
void visit_fields(CAugmentationDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Name", value.Name);
    visitor("Type", value.Type);
    visitor("InventoryGraphic", value.InventoryGraphic);
    visitor("DamageMultiplier", value.DamageMultiplier);
    visitor("ExperienceMultiplier", value.ExperienceMultiplier);
    visitor("FrequencyToAddHealthInSeconds", value.FrequencyToAddHealthInSeconds);
    visitor("HealthToAddPerIncrease", value.HealthToAddPerIncrease);
    visitor("FrequencyToAddStaminaInSeconds", value.FrequencyToAddStaminaInSeconds);
    visitor("StaminaToAddPerIncrease", value.StaminaToAddPerIncrease);
}

template <class V>
void visit_fields(CBalverineBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SecondsBetweenLunges", value.SecondsBetweenLunges);
    visitor("LungeAttackDamage", value.LungeAttackDamage);
    visitor("TimeInTrees", value.TimeInTrees);
}

template <class V>
void visit_fields(CBedDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GetInBedAnimName", value.GetInBedAnimName);
    visitor("SleepInBedAnimName", value.SleepInBedAnimName);
    visitor("GetUpFromBedAnimName", value.GetUpFromBedAnimName);
    visitor("GetUpFromBedFastAnimName", value.GetUpFromBedFastAnimName);
    visitor("Animating", value.Animating);
}

template <class V>
void visit_fields(CBettingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BetX", value.BetX);
    visitor("BetY", value.BetY);
    visitor("MoneyX", value.MoneyX);
    visitor("MoneyY", value.MoneyY);
    visitor("MinBet", value.MinBet);
    visitor("MaxBet", value.MaxBet);
    visitor("BetIncrement", value.BetIncrement);
    visitor("Bet", value.Bet);
    visitor("PlayersMoney", value.PlayersMoney);
    visitor("Question", value.Question);
    visitor("Continue", value.Continue);
    visitor("Cancel", value.Cancel);
}

template <class V>
void visit_fields(CBlendedParticleEffectSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Particle1Index", value.Particle1Index);
    visitor("Particle2Index", value.Particle2Index);
    visitor("Particle1Value", value.Particle1Value);
    visitor("Particle2Value", value.Particle2Value);
}

template <class V>
void visit_fields(CBlurredAttitudesDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Attitudes", value.Attitudes);
}

template <class V>
void visit_fields(CBoastingPodiumDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HeroOnPodiumRadius", value.HeroOnPodiumRadius);
}

template <class V>
void visit_fields(CBonusItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HealthModifier", value.HealthModifier);
    visitor("AddsFatness", value.AddsFatness);
    visitor("StaminaModifier", value.StaminaModifier);
    visitor("SecondsToApplyHealthIncreaseOver", value.SecondsToApplyHealthIncreaseOver);
    visitor("WillExperience", value.WillExperience);
    visitor("SkillExperience", value.SkillExperience);
    visitor("StrengthExperience", value.StrengthExperience);
    visitor("MoralityChange", value.MoralityChange);
    visitor("AgreeablenessChange", value.AgreeablenessChange);
    visitor("AttractivenessChange", value.AttractivenessChange);
    visitor("ScarinessChange", value.ScarinessChange);
    visitor("MaxHealthIncrease", value.MaxHealthIncrease);
    visitor("MaxStaminaIncrease", value.MaxStaminaIncrease);
    visitor("ChangeTimeOfDayToDay", value.ChangeTimeOfDayToDay);
    visitor("ChangeTimeOfDayToNight", value.ChangeTimeOfDayToNight);
    visitor("SpeedMultiplier", value.SpeedMultiplier);
    visitor("SecondsForSpeedMultiplierToLast", value.SecondsForSpeedMultiplierToLast);
    visitor("ParticleEffect", value.ParticleEffect);
    visitor("AddsDrunkenness", value.AddsDrunkenness);
    visitor("AlcoholicStrength", value.AlcoholicStrength);
}

template <class V>
void visit_fields(CBossDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PhaseHealthPercent", value.PhaseHealthPercent);
    visitor("Shots", value.Shots);
    visitor("TwinBladeMissesBeforeLunge", value.TwinBladeMissesBeforeLunge);
    visitor("TwinBladeStuckLoops", value.TwinBladeStuckLoops);
    visitor("TwinBladeTimeBeforeLunge", value.TwinBladeTimeBeforeLunge);
}

template <class V>
void visit_fields(CBrainBehaviour &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Name", value.Name);
    visitor("Priority", value.Priority);
    visitor("ScriptAIPriority", value.ScriptAIPriority);
}

template <class V>
void visit_fields(CBrainUpdateZone &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Distance", value.Distance);
    visitor("UpdateFrequency", value.UpdateFrequency);
    visitor("InterruptionFrequency", value.InterruptionFrequency);
}

template <class V>
void visit_fields(CBrainDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Behaviours", value.Behaviours);
    visitor("DisabledBehaviours", value.DisabledBehaviours);
    visitor("UpdateZones", value.UpdateZones);
}

template <class V>
void visit_fields(CBriarRoseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SecondsToTeleport", value.SecondsToTeleport);
    visitor("SecondsToBeIntangible", value.SecondsToBeIntangible);
    visitor("SecondsToChargeDrainLife", value.SecondsToChargeDrainLife);
    visitor("GuranteedImpostersHitBeforeAllowedToHitBriar", value.GuranteedImpostersHitBeforeAllowedToHitBriar);
    visitor("NumberOfHitsToKillImposters", value.NumberOfHitsToKillImposters);
    visitor("EnvironmentThemeForIntangibleMode", value.EnvironmentThemeForIntangibleMode);
    visitor("SecondsForIntangibleEnvironmentThemeTransitionIn", value.SecondsForIntangibleEnvironmentThemeTransitionIn);
    visitor("SecondsForIntangibleEnvironmentThemeTransitionOut", value.SecondsForIntangibleEnvironmentThemeTransitionOut);
    visitor("SecondsBetweenWillUse", value.SecondsBetweenWillUse);
    visitor("SecondsBetweenTeleportAway", value.SecondsBetweenTeleportAway);
    visitor("DistanceForcePushThreshold", value.DistanceForcePushThreshold);
}

template <class V>
void visit_fields(CBuyHouseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Main", value.Main);
}

template <class V>
void visit_fields(CBuyableHouseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IsBuyable", value.IsBuyable);
    visitor("InitiallyEmpty", value.InitiallyEmpty);
    visitor("Price", value.Price);
    visitor("Rent", value.Rent);
    visitor("DaysBetweenRentPayments", value.DaysBetweenRentPayments);
    visitor("MaxRentBags", value.MaxRentBags);
    visitor("IsScripted", value.IsScripted);
    visitor("IsBuyableWithoutSignpost", value.IsBuyableWithoutSignpost);
}

template <class V>
void visit_fields(CCameraCollisionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CameraCollisionMesh", value.CameraCollisionMesh);
}

template <class V>
void visit_fields(CCameraManagerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CameraList", value.CameraList);
    visitor("SpecialityCameraList", value.SpecialityCameraList);
    visitor("InitialMode", value.InitialMode);
    visitor("ZTargetMode", value.ZTargetMode);
    visitor("ProjectileWeaponMode", value.ProjectileWeaponMode);
    visitor("BaseHeightToKeepAboveFlatLandscape", value.BaseHeightToKeepAboveFlatLandscape);
    visitor("BaseHeightToKeepAboveWater", value.BaseHeightToKeepAboveWater);
    visitor("MaxHeightRelativeToGround", value.MaxHeightRelativeToGround);
    visitor("MaxHeightStepForPointAcceptance", value.MaxHeightStepForPointAcceptance);
    visitor("AngleBetweenGroundNormalAndVerticalForLandscapeToBeConsideredNotFlat", value.AngleBetweenGroundNormalAndVerticalForLandscapeToBeConsideredNotFlat);
}

template <class V>
void visit_fields(CCameraManagerSetDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CameraManagerMain", value.CameraManagerMain);
    visitor("CameraManagerCombat", value.CameraManagerCombat);
    visitor("CameraManagerPCMain", value.CameraManagerPCMain);
    visitor("CameraManagerPCCombat", value.CameraManagerPCCombat);
}

template <class V>
void visit_fields(CCameraModeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CameraMode", value.CameraMode);
    visitor("StringLengthXY", value.StringLengthXY);
    visitor("CageRadius", value.CageRadius);
    visitor("HeightOffsetZ", value.HeightOffsetZ);
    visitor("LookOffsetZ", value.LookOffsetZ);
    visitor("FOV", value.FOV);
    visitor("RollAngle", value.RollAngle);
    visitor("TransitionInFrames", value.TransitionInFrames);
    visitor("LazyLookAtPosWeight", value.LazyLookAtPosWeight);
    visitor("RaiseUpWhenClose", value.RaiseUpWhenClose);
    visitor("StretchAroundHero", value.StretchAroundHero);
    visitor("ZTargetBaseOffset", value.ZTargetBaseOffset);
    visitor("ZTargetLookAtTargetOffset", value.ZTargetLookAtTargetOffset);
    visitor("ZTargetStringLengthToObservedThing", value.ZTargetStringLengthToObservedThing);
    visitor("ZTargetStringLengthPerpendicularToObservedThing", value.ZTargetStringLengthPerpendicularToObservedThing);
    visitor("PerformExtendedOcclusionTests", value.PerformExtendedOcclusionTests);
    visitor("XAxisDisplacementCoupling", value.XAxisDisplacementCoupling);
    visitor("XAxisVelocityDamping", value.XAxisVelocityDamping);
    visitor("XAxisAttractorCoupling", value.XAxisAttractorCoupling);
    visitor("XAxisAttractorFallOff", value.XAxisAttractorFallOff);
    visitor("YAxisDisplacementCoupling", value.YAxisDisplacementCoupling);
    visitor("YAxisVelocityDamping", value.YAxisVelocityDamping);
    visitor("YAxisAttractorCoupling", value.YAxisAttractorCoupling);
    visitor("YAxisAttractorFallOff", value.YAxisAttractorFallOff);
    visitor("ProjectileCameraFOVZoom", value.ProjectileCameraFOVZoom);
    visitor("ProjectileCameraZoomToggle", value.ProjectileCameraZoomToggle);
    visitor("ProjectileCameraDefaultTranslationalZoomOffset", value.ProjectileCameraDefaultTranslationalZoomOffset);
    visitor("ProjectileCameraInitialPhiOffsetForCamera", value.ProjectileCameraInitialPhiOffsetForCamera);
    visitor("ProjectileCameraInitialHeightParamForCamera", value.ProjectileCameraInitialHeightParamForCamera);
    visitor("PhiRotationCouple", value.PhiRotationCouple);
    visitor("ThetaRotationCouple", value.ThetaRotationCouple);
    visitor("DollyLength", value.DollyLength);
    visitor("NumberOfFramesToAverageInputsOver", value.NumberOfFramesToAverageInputsOver);
    visitor("DollyLengthBlendResistance", value.DollyLengthBlendResistance);
    visitor("ThetaAngleLimitUp", value.ThetaAngleLimitUp);
    visitor("ThetaAngleLimitDown", value.ThetaAngleLimitDown);
    visitor("SecondsToReset", value.SecondsToReset);
    visitor("ViewHeroXYOffsetMultiplier", value.ViewHeroXYOffsetMultiplier);
    visitor("ViewHeroXYRotationMaxVelocity", value.ViewHeroXYRotationMaxVelocity);
    visitor("ViewHeroHeightParamMaxVelocity", value.ViewHeroHeightParamMaxVelocity);
    visitor("ViewHeroGroundHeightOffset", value.ViewHeroGroundHeightOffset);
}

template <class V>
void visit_fields(CCardDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CardName", value.CardName);
    visitor("CardVal", value.CardVal);
}

template <class V>
void visit_fields(CCardPositionsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Offset", value.Offset);
}

template <class V>
void visit_fields(CCarriedReadableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TitleTextTag", value.TitleTextTag);
    visitor("BodyTextTag", value.BodyTextTag);
}

template <class V>
void visit_fields(CCarrySlotDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DummyPosName", value.DummyPosName);
    visitor("DummyPosIndex", value.DummyPosIndex);
    visitor("PrimarySlot", value.PrimarySlot);
    visitor("SecondaryDummyPosName", value.SecondaryDummyPosName);
}

template <class V>
void visit_fields(CCarryableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ActiveCarrySlot", value.ActiveCarrySlot);
    visitor("SecondaryActiveCarrySlot", value.SecondaryActiveCarrySlot);
    visitor("PassiveCarrySlot", value.PassiveCarrySlot);
    visitor("OnKillFX", value.OnKillFX);
    visitor("OffsetCoordRelativeToAttachToDummy", value.OffsetCoordRelativeToAttachToDummy);
    visitor("OffsetAmountForThingsOnTopOfMe", value.OffsetAmountForThingsOnTopOfMe);
    visitor("PassiveCarrySlotScale", value.PassiveCarrySlotScale);
    visitor("IncludeInThingScansWhenCarried", value.IncludeInThingScansWhenCarried);
}

template <class V>
void visit_fields(CCarryingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AvailableCarrySlots", value.AvailableCarrySlots);
    visitor("OverriddenDummyObject", value.OverriddenDummyObject);
}

template <class V>
void visit_fields(CChestDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PersistOnOpening", value.PersistOnOpening);
    visitor("OpenParticleEffect", value.OpenParticleEffect);
    visitor("DisplayMessageOnEmpty", value.DisplayMessageOnEmpty);
    visitor("OpenAnimationForCreature", value.OpenAnimationForCreature);
    visitor("OpenerObject", value.OpenerObject);
    visitor("OpenersRequired", value.OpenersRequired);
}

template <class V>
void visit_fields(CClockDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Sound", value.Sound);
    visitor("HandType", value.HandType);
}

template <class V>
void visit_fields(CCoinGameObstacleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Radius", value.Radius);
}

template <class V>
void visit_fields(CCombatAbilityData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PercentageChance", value.PercentageChance);
    visitor("SecondsDuration", value.SecondsDuration);
    visitor("SecondsToWaitBeforeRepeat", value.SecondsToWaitBeforeRepeat);
    visitor("Anim", value.Anim);
}

template <class V>
void visit_fields(CCombatAbilityMeleeAttackData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityData &>(value), visitor);
    visitor("Blockable", value.Blockable);
    visitor("Knockdown", value.Knockdown);
    visitor("Damage", value.Damage);
}

template <class V>
void visit_fields(CCombatAbilityAttackBase &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CombatAbility", value.CombatAbility);
}

template <class V>
void visit_fields(CCombatAbilityBlockCounterAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityAttackBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityBlockingData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityData &>(value), visitor);
    visitor("BlockAngle", value.BlockAngle);
    visitor("BlockCanBeBroken", value.BlockCanBeBroken);
    visitor("ForceRecoilWhenBlocking", value.ForceRecoilWhenBlocking);
    visitor("ValidBlockWeaponTypes", value.ValidBlockWeaponTypes);
    visitor("WeaponClass", value.WeaponClass);
}

template <class V>
void visit_fields(CCombatAbilityBlockDefBase &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CombatAbility", value.CombatAbility);
}

template <class V>
void visit_fields(CCombatAbilityBlockHeavyWeaponAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityBlockDefBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityBlockLightWeaponAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityBlockDefBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityBlockProjectileWeaponAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityBlockDefBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityBlockUnarmedAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityBlockDefBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityFlourishCounterAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityAttackBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityGetHitCounterAttackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityAttackBase &>(value), visitor);
}

template <class V>
void visit_fields(CCombatAbilityProjectileAttackData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityData &>(value), visitor);
    visitor("AbleToStrafeAndShoot", value.AbleToStrafeAndShoot);
    visitor("ShotAccuracyPercentage", value.ShotAccuracyPercentage);
}

template <class V>
void visit_fields(CCombatAbilityStrafingData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CCombatAbilityData &>(value), visitor);
    visitor("RotationSpeed", value.RotationSpeed);
}

template <class V>
void visit_fields(CCombatAbilityStrafeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CombatAbility", value.CombatAbility);
}

template <class V>
void visit_fields(CCombatAbilityUseProjectileWeaponDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CombatAbility", value.CombatAbility);
}

template <class V>
void visit_fields(CCombatDialogueDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinTimeBetweenSpeech", value.MinTimeBetweenSpeech);
    visitor("MinTimeBetweenSpeechRepeat", value.MinTimeBetweenSpeechRepeat);
    visitor("MinDelayAfterBattleCry", value.MinDelayAfterBattleCry);
    visitor("EnemySighted", value.EnemySighted);
    visitor("RunForCover", value.RunForCover);
    visitor("Hiding", value.Hiding);
    visitor("Ambushing", value.Ambushing);
    visitor("WillSeenUsed", value.WillSeenUsed);
    visitor("Threat", value.Threat);
    visitor("Encourage", value.Encourage);
    visitor("FoeKnockedDownByColleague", value.FoeKnockedDownByColleague);
    visitor("MenLosing", value.MenLosing);
    visitor("EnemyKnockedDown", value.EnemyKnockedDown);
    visitor("RangedCombatAiming", value.RangedCombatAiming);
    visitor("RangedCombatAimingBlocked", value.RangedCombatAimingBlocked);
    visitor("Injured", value.Injured);
    visitor("ReactionToDecapitation", value.ReactionToDecapitation);
    visitor("ColleagueInjured", value.ColleagueInjured);
    visitor("Scared", value.Scared);
    visitor("Flee", value.Flee);
}

template <class V>
void visit_fields(CCombatSequenceDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ValidCombatZones", value.ValidCombatZones);
    visitor("Type", value.Type);
    visitor("InterruptionType", value.InterruptionType);
    visitor("IsValidType", value.IsValidType);
    visitor("OnStartType", value.OnStartType);
    visitor("OnStopType", value.OnStopType);
    visitor("ActionList", value.ActionList);
    visitor("SecondsToPredictTargetPosition", value.SecondsToPredictTargetPosition);
    visitor("PreferIfValid", value.PreferIfValid);
}

template <class V>
void visit_fields(CCombatTypeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DefenderCombatRingOccupationNumbers", value.DefenderCombatRingOccupationNumbers);
    visitor("DefenderWheelOuterRingStartIndex", value.DefenderWheelOuterRingStartIndex);
    visitor("DefenderWheelAllowAttackerStrafing", value.DefenderWheelAllowAttackerStrafing);
    visitor("CombatSequence", value.CombatSequence);
    visitor("CombatSequenceDef", value.CombatSequenceDef);
    visitor("PreferOuterRing", value.PreferOuterRing);
    visitor("FrontFarLimit", value.FrontFarLimit);
    visitor("FrontMiddleLimit", value.FrontMiddleLimit);
    visitor("FrontNearLimit", value.FrontNearLimit);
    visitor("SideLimit", value.SideLimit);
    visitor("RearLimit", value.RearLimit);
    visitor("PreferredCombatDistance", value.PreferredCombatDistance);
    visitor("AttackPatterns", value.AttackPatterns);
    visitor("CombatCreatureType", value.CombatCreatureType);
    visitor("AlwaysAttackIfNearest", value.AlwaysAttackIfNearest);
    visitor("KnockdownCausesStun", value.KnockdownCausesStun);
    visitor("RecoilStyle", value.RecoilStyle);
    visitor("CanBeKnockedDown", value.CanBeKnockedDown);
    visitor("ComboContinueSpeed", value.ComboContinueSpeed);
    visitor("CombatFOV", value.CombatFOV);
    visitor("StartsAsReadyForMelee", value.StartsAsReadyForMelee);
}

template <class V>
void visit_fields(CComboMultiplierData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ExperienceMultiplierNumSecondsForAttenuation", value.ExperienceMultiplierNumSecondsForAttenuation);
    visitor("ExperienceMultiplierParamDecayDeltaPerSecond", value.ExperienceMultiplierParamDecayDeltaPerSecond);
    visitor("ComboMultiplier", value.ComboMultiplier);
}

template <class V>
void visit_fields(CConfigOptionsDefaultsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Antialiasing", value.Antialiasing);
    visitor("ResolutionWidth", value.ResolutionWidth);
    visitor("ResolutionHeight", value.ResolutionHeight);
    visitor("BitDepth", value.BitDepth);
    visitor("MinResolutionWidth", value.MinResolutionWidth);
    visitor("MinResolutionHeight", value.MinResolutionHeight);
    visitor("TextureDetail", value.TextureDetail);
    visitor("MaxTextureDetail", value.MaxTextureDetail);
    visitor("ShadowDetail", value.ShadowDetail);
    visitor("MaxShadowDetail", value.MaxShadowDetail);
    visitor("MeshDetail", value.MeshDetail);
    visitor("MaxMeshDetail", value.MaxMeshDetail);
    visitor("EffectsDetail", value.EffectsDetail);
    visitor("MaxEffectsDetail", value.MaxEffectsDetail);
}

template <class V>
void visit_fields(CContainerRewardHeroDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ObjectFamilies", value.ObjectFamilies);
}

template <class V>
void visit_fields(CContextSensitiveItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("Priority", value.Priority);
    visitor("SlotIndex", value.SlotIndex);
    visitor("ExtraData1", value.ExtraData1);
    visitor("ExtraData2", value.ExtraData2);
}

template <class V>
void visit_fields(CControlsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Controls", value.Controls);
    visitor("ToggleZTarget", value.ToggleZTarget);
    visitor("ToggleSpells", value.ToggleSpells);
    visitor("ToggleSneak", value.ToggleSneak);
    visitor("ToggleExpressionMenu", value.ToggleExpressionMenu);
    visitor("ToggleExpressionShift", value.ToggleExpressionShift);
    visitor("FlourishNeedsAttackButtonHeld", value.FlourishNeedsAttackButtonHeld);
}

template <class V>
void visit_fields(CConversation &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Lines", value.Lines);
    visitor("Speaker", value.Speaker);
    visitor("Dialogue", value.Dialogue);
    visitor("Animation", value.Animation);
    visitor("AnimLoop", value.AnimLoop);
}

template <class V>
void visit_fields(CCoopSpiritDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("OnHitParticle", value.OnHitParticle);
    visitor("OnStrikeParticle", value.OnStrikeParticle);
    visitor("DefaultParticle", value.DefaultParticle);
    visitor("AttackTargetParticle", value.AttackTargetParticle);
    visitor("MeleeTargetRange", value.MeleeTargetRange);
}

template <class V>
void visit_fields(CCrateStackDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StackType", value.StackType);
    visitor("Priority", value.Priority);
}

template <class V>
void visit_fields(CCreatureAbilityDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
}

template <class V>
void visit_fields(CFloatRange &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HighValue", value.HighValue);
    visitor("LowValue", value.LowValue);
}

template <class V>
void visit_fields(CHeroAbilityStatWeight &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StatWeights", value.StatWeights);
}

template <class V>
void visit_fields(CCreatureDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PWoundMorphs", value.PWoundMorphs);
    visitor("PRandomAppearanceMorph", value.PRandomAppearanceMorph);
    visitor("PExpressions", value.PExpressions);
    visitor("PStats", value.PStats);
    visitor("InitialActions", value.InitialActions);
    visitor("InitialAppearanceModifiers", value.InitialAppearanceModifiers);
    visitor("PhonemeAnim", value.PhonemeAnim);
    visitor("DialogueVoices", value.DialogueVoices);
    visitor("Inventory", value.Inventory);
    visitor("ClothingInventory", value.ClothingInventory);
    visitor("WeaponsInventory", value.WeaponsInventory);
    visitor("HeroAbilitiesScreenInventory", value.HeroAbilitiesScreenInventory);
    visitor("QuestCardScreenInventory", value.QuestCardScreenInventory);
    visitor("MapScreenInventory", value.MapScreenInventory);
    visitor("StatsScreenInventory", value.StatsScreenInventory);
    visitor("MagicScreenInventory", value.MagicScreenInventory);
    visitor("ExperienceScreenInventory", value.ExperienceScreenInventory);
    visitor("TradeScreenInventory", value.TradeScreenInventory);
    visitor("AugmentScreenInventory", value.AugmentScreenInventory);
    visitor("QuestsScreenInventory", value.QuestsScreenInventory);
    visitor("InGameMenuScreen", value.InGameMenuScreen);
    visitor("CreatureGroup", value.CreatureGroup);
    visitor("BattleCrySound", value.BattleCrySound);
    visitor("OpinionSourceDef", value.OpinionSourceDef);
    visitor("RespawnWaitInDaysMin", value.RespawnWaitInDaysMin);
    visitor("RespawnWaitInDaysMax", value.RespawnWaitInDaysMax);
    visitor("RespawnWaitInFramesMin", value.RespawnWaitInFramesMin);
    visitor("RespawnWaitInFramesMax", value.RespawnWaitInFramesMax);
    visitor("Short", value.Short);
    visitor("FlashOnHit", value.FlashOnHit);
    visitor("PickPocketable", value.PickPocketable);
    visitor("ChildThatCanRegionFollow", value.ChildThatCanRegionFollow);
    visitor("AllowedInProtectedTowns", value.AllowedInProtectedTowns);
    visitor("UseActualHitPosForHitEffects", value.UseActualHitPosForHitEffects);
    visitor("RadiusWeight", value.RadiusWeight);
    visitor("RadiusSpread", value.RadiusSpread);
    visitor("AGhostIsWhatIBe", value.AGhostIsWhatIBe);
}

template <class V>
void visit_fields(CCreatureGenerationFamilyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Creatures", value.Creatures);
    visitor("DifficultyLevel", value.DifficultyLevel);
    visitor("GeneratorObject", value.GeneratorObject);
    visitor("GenerationTypes", value.GenerationTypes);
}

template <class V>
void visit_fields(CCreatureGeneratorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("RandomEffects", value.RandomEffects);
    visitor("RandomSoundCriteria", value.RandomSoundCriteria);
    visitor("SecondsBetweenEffects", value.SecondsBetweenEffects);
    visitor("SecondsBetweenSoundEffects", value.SecondsBetweenSoundEffects);
}

template <class V>
void visit_fields(CCreatureModeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DefaultCreatureMode", value.DefaultCreatureMode);
    visitor("InitialCreatureMode", value.InitialCreatureMode);
    visitor("DefaultWeaponCreatureMode", value.DefaultWeaponCreatureMode);
    visitor("AvailableIdleModes", value.AvailableIdleModes);
}

template <class V>
void visit_fields(CCreatureNavigationDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinPathCost", value.MinPathCost);
    visitor("MaxPathCost", value.MaxPathCost);
    visitor("Radius", value.Radius);
    visitor("AvoidDynamicObstacles", value.AvoidDynamicObstacles);
}

template <class V>
void visit_fields(CCreatureStatsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MoralityForKill", value.MoralityForKill);
    visitor("MoralityForPunch", value.MoralityForPunch);
    visitor("MoralityForWeaponHit", value.MoralityForWeaponHit);
    visitor("ExperienceWorth", value.ExperienceWorth);
    visitor("RenownWorth", value.RenownWorth);
    visitor("Speed", value.Speed);
    visitor("Dexterity", value.Dexterity);
    visitor("StrengthDamageMultiplier", value.StrengthDamageMultiplier);
}

template <class V>
void visit_fields(CCutsceneDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Macro", value.Macro);
    visitor("SkipCond", value.SkipCond);
    visitor("Lights", value.Lights);
    visitor("LightScene", value.LightScene);
    visitor("SetupCond", value.SetupCond);
    visitor("Sound", value.Sound);
    visitor("Answer0", value.Answer0);
    visitor("Answer1", value.Answer1);
}

template <class V>
void visit_fields(CDecapitationDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BodyParticleEffect", value.BodyParticleEffect);
    visitor("Head", value.Head);
}

template <class V>
void visit_fields(CDegradableInfo &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HealthPercentage", value.HealthPercentage);
    visitor("BankIndex", value.BankIndex);
    visitor("AnimStep", value.AnimStep);
    visitor("RenderSizeX", value.RenderSizeX);
    visitor("AdditiveAlpha", value.AdditiveAlpha);
    visitor("Type", value.Type);
    visitor("SmashParticleEmitter", value.SmashParticleEmitter);
    visitor("BlocksNavigation", value.BlocksNavigation);
    visitor("Skip", value.Skip);
}

template <class V>
void visit_fields(CDegradableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Degradable", value.Degradable);
    visitor("GraphicType", value.GraphicType);
    visitor("InitiallyBlocksNavigation", value.InitiallyBlocksNavigation);
    visitor("Degradations", value.Degradations);
}

template <class V>
void visit_fields(CDegradableDef::CIsHealth1GTHealth2 &value, V &&visitor) {
    (void)value;
    (void)visitor;
}

template <class V>
void visit_fields(CDegradableDef::CSetEngineGraphicType &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GraphicType", value.GraphicType);
}

template <class V>
void visit_fields(CDoorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TriggerType", value.TriggerType);
    visitor("TriggerRadius", value.TriggerRadius);
    visitor("OpenCollisionMesh", value.OpenCollisionMesh);
    visitor("ClosedCollisionMesh", value.ClosedCollisionMesh);
    visitor("PostVandalismCrimeIfDestroyed", value.PostVandalismCrimeIfDestroyed);
    visitor("LockPickable", value.LockPickable);
    visitor("PreventLockingByOwner", value.PreventLockingByOwner);
}

template <class V>
void visit_fields(CDragonActionHoverDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("WindSpeed", value.WindSpeed);
}

template <class V>
void visit_fields(CDragonActionNapalmDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ChargeUpTime", value.ChargeUpTime);
    visitor("WindSpeed", value.WindSpeed);
    visitor("TimeBeforeAddOfMouthEffect", value.TimeBeforeAddOfMouthEffect);
    visitor("TimeBeforeAddExplosion", value.TimeBeforeAddExplosion);
}

template <class V>
void visit_fields(CDragonActionSwoopDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IntoDelay", value.IntoDelay);
    visitor("OutOfDelay", value.OutOfDelay);
    visitor("Distance", value.Distance);
    visitor("MidPointsControlOffset", value.MidPointsControlOffset);
    visitor("StartHeight", value.StartHeight);
    visitor("MidHeight", value.MidHeight);
    visitor("EndHeight", value.EndHeight);
    visitor("FlightSpeed", value.FlightSpeed);
    visitor("MaxSplineProgressForSplineAdjustment", value.MaxSplineProgressForSplineAdjustment);
    visitor("MaxSplineProgressForBurninationEffect", value.MaxSplineProgressForBurninationEffect);
    visitor("MinSplineProgressForWingFlap", value.MinSplineProgressForWingFlap);
    visitor("MaxExplosionRange", value.MaxExplosionRange);
    visitor("ExplosionLineProjectionAngle", value.ExplosionLineProjectionAngle);
    visitor("FramesBetweenExplosions", value.FramesBetweenExplosions);
    visitor("WindSpeed", value.WindSpeed);
}

template <class V>
void visit_fields(CDrunkennessDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DrunkennessThresholdMult", value.DrunkennessThresholdMult);
}

template <class V>
void visit_fields(CEnemyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Faction", value.Faction);
}

template <class V>
void visit_fields(CEngineDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("LODErrorTolerance", value.LODErrorTolerance);
    visitor("CharacterLODErrorTolerance", value.CharacterLODErrorTolerance);
    visitor("LODErrorFactor", value.LODErrorFactor);
    visitor("SeaHeight", value.SeaHeight);
    visitor("GamePrimitiveDefaultFadeStart", value.GamePrimitiveDefaultFadeStart);
    visitor("GamePrimitiveDefaultFadeRangeRatio", value.GamePrimitiveDefaultFadeRangeRatio);
    visitor("LocalDetailDefaultFadeStart", value.LocalDetailDefaultFadeStart);
    visitor("LocalDetailDefaultFadeRangeRatio", value.LocalDetailDefaultFadeRangeRatio);
    visitor("FOV_2D", value.FOV_2D);
    visitor("LocalDetailBooleanAlphaDefaultAlphaRef", value.LocalDetailBooleanAlphaDefaultAlphaRef);
    visitor("DefaultPrimitiveAlphaRef", value.DefaultPrimitiveAlphaRef);
    visitor("TestStaticMesh", value.TestStaticMesh);
    visitor("TestAnimatedMesh", value.TestAnimatedMesh);
    visitor("TestAnim", value.TestAnim);
    visitor("TestGraphic", value.TestGraphic);
    visitor("InvalidTextureStandin", value.InvalidTextureStandin);
    visitor("InvalidThemeStandin", value.InvalidThemeStandin);
}

template <class V>
void visit_fields(CEngineLocalDetailObjectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Mesh", value.Mesh);
    visitor("ShadowMesh", value.ShadowMesh);
    visitor("Probability", value.Probability);
    visitor("ThemeBlendThreshold", value.ThemeBlendThreshold);
    visitor("Scale", value.Scale);
    visitor("ScaleRandomElement", value.ScaleRandomElement);
    visitor("FadeStart", value.FadeStart);
    visitor("FadeEnd", value.FadeEnd);
    visitor("ZSpriteMesh", value.ZSpriteMesh);
    visitor("ZSpriteFadeStart", value.ZSpriteFadeStart);
    visitor("ZSpriteFadeEnd", value.ZSpriteFadeEnd);
    visitor("SlopeFadeStart", value.SlopeFadeStart);
    visitor("SlopeFadeEnd", value.SlopeFadeEnd);
    visitor("AlphaRef", value.AlphaRef);
    visitor("AlphaMipBias", value.AlphaMipBias);
    visitor("WindSkewConstantFactor", value.WindSkewConstantFactor);
    visitor("WindSkewRandomFactor", value.WindSkewRandomFactor);
    visitor("WindSkewSpeedFactor", value.WindSkewSpeedFactor);
    visitor("CastShadows", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.CastShadows); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.CastShadows = v; }));
    visitor("ReceiveShadows", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.ReceiveShadows); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.ReceiveShadows = v; }));
    visitor("IsRepeatedMesh", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.IsRepeatedMesh); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.IsRepeatedMesh = v; }));
    visitor("IsZSprite", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.IsZSprite); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.IsZSprite = v; }));
    visitor("AlphaIsBoolean", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.AlphaIsBoolean); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.AlphaIsBoolean = v; }));
    visitor("HasLandscapeNormalLighting", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.HasLandscapeNormalLighting); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.HasLandscapeNormalLighting = v; }));
    visitor("TiltToSlope", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.TiltToSlope); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.TiltToSlope = v; }));
    visitor("HasWindSkew", detail::bitfield(value,
        [](const CEngineLocalDetailObjectDef &o) { return bool(o.HasWindSkew); },
        [](CEngineLocalDetailObjectDef &o, bool v) { o.HasWindSkew = v; }));
}

template <class V>
void visit_fields(CEngineLocalDetailLayerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SpacingFromLayer", value.SpacingFromLayer);
    visitor("Objects", value.Objects);
}

template <class V>
void visit_fields(CEngineLocalDetailGeneratorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Layers", value.Layers);
}

template <class V>
void visit_fields(CEngineThemeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BaseTexture", value.BaseTexture);
    visitor("BackgroundTexture", value.BackgroundTexture);
    visitor("CliffBaseTexture", value.CliffBaseTexture);
    visitor("CliffBackgroundTexture", value.CliffBackgroundTexture);
    visitor("BaseBumpMap", value.BaseBumpMap);
    visitor("CliffBumpMap", value.CliffBumpMap);
    visitor("BaseTextureMaxSize", value.BaseTextureMaxSize);
    visitor("CliffTextureMaxSize", value.CliffTextureMaxSize);
    visitor("BaseBumpMapMaxSize", value.BaseBumpMapMaxSize);
    visitor("CliffBumpMapMaxSize", value.CliffBumpMapMaxSize);
    visitor("BaseTextureSelfIllumination", value.BaseTextureSelfIllumination);
    visitor("CliffTextureSelfIllumination", value.CliffTextureSelfIllumination);
    visitor("WaterHeight", value.WaterHeight);
    visitor("WaterType", value.WaterType);
    visitor("NoWaterThemeDef", value.NoWaterThemeDef);
    visitor("LocalDetailGeneratorDef", value.LocalDetailGeneratorDef);
    visitor("GroupDef", value.GroupDef);
    visitor("Height", value.Height);
    visitor("MinCameraHeight", value.MinCameraHeight);
    visitor("CoverValue", value.CoverValue);
    visitor("MaterialDef", value.MaterialDef);
    visitor("MinimapTheme", value.MinimapTheme);
    visitor("DestructionEffectDef", value.DestructionEffectDef);
    visitor("DestructionThemeDef", value.DestructionThemeDef);
    visitor("Friction", value.Friction);
    visitor("Passable", detail::bitfield(value,
        [](const CEngineThemeDef &o) { return bool(o.Passable); },
        [](CEngineThemeDef &o, bool v) { o.Passable = v; }));
}

template <class V>
void visit_fields(CEngineThemeGroupDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Dummy", value.Dummy);
}

template <class V>
void visit_fields(CEngineVideoOptionsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HiresTextureMemory", value.HiresTextureMemory);
    visitor("LODErrorTolerance", value.LODErrorTolerance);
    visitor("CharacterLODErrorTolerance", value.CharacterLODErrorTolerance);
    visitor("DrawDistanceMultiplier", value.DrawDistanceMultiplier);
    visitor("DrawDistanceMinimum", value.DrawDistanceMinimum);
    visitor("DrawDistanceMaximum", value.DrawDistanceMaximum);
    visitor("RepeatedMeshDrawDistanceFactor", value.RepeatedMeshDrawDistanceFactor);
    visitor("MinimumZSpriteAsMeshDistance", value.MinimumZSpriteAsMeshDistance);
    visitor("MaximumZSpriteAsMeshDistance", value.MaximumZSpriteAsMeshDistance);
    visitor("ZSpriteDrawDistanceMultiplier", value.ZSpriteDrawDistanceMultiplier);
    visitor("ShadowBufferSize", value.ShadowBufferSize);
    visitor("ShadowDistanceScale", value.ShadowDistanceScale);
    visitor("Enable2DDisplacement", value.Enable2DDisplacement);
    visitor("Enable3DDisplacement", value.Enable3DDisplacement);
    visitor("EnableGlow", value.EnableGlow);
    visitor("EnableRadialBlur", value.EnableRadialBlur);
    visitor("EnableWaterReflection", value.EnableWaterReflection);
    visitor("EnableWeatherEffects", value.EnableWeatherEffects);
    visitor("EnableColourFilter", value.EnableColourFilter);
    visitor("EnableRepeatedMeshes", value.EnableRepeatedMeshes);
    visitor("WeatherDensity", value.WeatherDensity);
}

template <class V>
void visit_fields(CSoundPair &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FirstSound", value.FirstSound);
    visitor("LastSound", value.LastSound);
}

template <class V>
void visit_fields(CSoundMap &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SoundMap", value.SoundMap);
    visitor("CriteriaMap", value.CriteriaMap);
}

template <class V>
void visit_fields(CEntitySoundDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SoundMap", value.SoundMap);
    visitor("SoundIdentifier", value.SoundIdentifier);
    visitor("VoicePitchOverride", value.VoicePitchOverride);
    visitor("AnimCriteriaClipDistance", value.AnimCriteriaClipDistance);
}

template <class V>
void visit_fields(CEnvironmentDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ColourLookupTexture", value.ColourLookupTexture);
    visitor("DiffuseLookupRow", value.DiffuseLookupRow);
    visitor("AmbientLookupRow", value.AmbientLookupRow);
    visitor("BacklightLookupRow", value.BacklightLookupRow);
    visitor("ReflectionLookupRow", value.ReflectionLookupRow);
    visitor("MistEffectColourLookupRow", value.MistEffectColourLookupRow);
    visitor("FogColourLookupRow", value.FogColourLookupRow);
    visitor("FogAlphaLookupRow", value.FogAlphaLookupRow);
    visitor("SunColourLookupRow", value.SunColourLookupRow);
    visitor("CloudColourLookupRow", value.CloudColourLookupRow);
    visitor("MoonColourLookupRow", value.MoonColourLookupRow);
    visitor("StarsColourLookupRow", value.StarsColourLookupRow);
    visitor("SunFlareColourLookupRow", value.SunFlareColourLookupRow);
    visitor("LensFlareColourLookupRow", value.LensFlareColourLookupRow);
    visitor("SkyGradientTopLookupRow", value.SkyGradientTopLookupRow);
    visitor("SkyGradientTopAlphaLookupRow", value.SkyGradientTopAlphaLookupRow);
    visitor("SkyGradientBottomLookupRow", value.SkyGradientBottomLookupRow);
    visitor("SkyGradientBottomAlphaLookupRow", value.SkyGradientBottomAlphaLookupRow);
    visitor("WaterColourLookupRow", value.WaterColourLookupRow);
    visitor("SeaColourLookupRow", value.SeaColourLookupRow);
    visitor("GlowThresholdColourLookupRow", value.GlowThresholdColourLookupRow);
    visitor("GlowBloomColourLookupRow", value.GlowBloomColourLookupRow);
    visitor("SunlightAttenuatorColourLookupRow", value.SunlightAttenuatorColourLookupRow);
    visitor("DiffuseClampAngle", value.DiffuseClampAngle);
    visitor("SunlightAttenuatorAngleFadeStart", value.SunlightAttenuatorAngleFadeStart);
    visitor("SunlightAttenuatorAngleFadeEnd", value.SunlightAttenuatorAngleFadeEnd);
    visitor("SeaTexture", value.SeaTexture);
    visitor("SeaRadius", value.SeaRadius);
    visitor("SeaFlatSectionStart", value.SeaFlatSectionStart);
    visitor("SeaFlatSectionEnd", value.SeaFlatSectionEnd);
    visitor("DayStartTime", value.DayStartTime);
    visitor("DaySpeed", value.DaySpeed);
    visitor("WaterBumpMapPC", value.WaterBumpMapPC);
    visitor("SeaBumpMapPC", value.SeaBumpMapPC);
    visitor("IceBumpMapPC", value.IceBumpMapPC);
    visitor("IceBumpMap", value.IceBumpMap);
    visitor("IceTexture", value.IceTexture);
    visitor("WaterEdgeAlphaMap", value.WaterEdgeAlphaMap);
    visitor("WaterSurfMap", value.WaterSurfMap);
    visitor("WaterBumpMap", value.WaterBumpMap);
    visitor("WaterBumpMap2", value.WaterBumpMap2);
    visitor("SeaBumpMap", value.SeaBumpMap);
    visitor("SeaBumpMap2", value.SeaBumpMap2);
    visitor("WaterEnvMapOverlayTexture", value.WaterEnvMapOverlayTexture);
    visitor("MistAlphaGraphic", value.MistAlphaGraphic);
    visitor("MistAlphaGraphicPC", value.MistAlphaGraphicPC);
    visitor("RainTexture", value.RainTexture);
    visitor("RainTexturePC", value.RainTexturePC);
    visitor("SnowTexture", value.SnowTexture);
    visitor("RainSplashParticle", value.RainSplashParticle);
    visitor("LightningFadeInDuration", value.LightningFadeInDuration);
    visitor("LightningFadeOutDuration", value.LightningFadeOutDuration);
    visitor("LightningFlashDuration", value.LightningFlashDuration);
    visitor("LightningRainThreshold", value.LightningRainThreshold);
    visitor("LightningTheme", value.LightningTheme);
    visitor("CloudSpeedMultiplier", value.CloudSpeedMultiplier);
    visitor("CloudMaxSpeed", value.CloudMaxSpeed);
    visitor("CloudTextureCoordMultiplier", value.CloudTextureCoordMultiplier);
    visitor("CloudTextureCoordOffset", value.CloudTextureCoordOffset);
    visitor("CloudHeightOffset", value.CloudHeightOffset);
    visitor("WaterLakeMinimumFlowSpeed", value.WaterLakeMinimumFlowSpeed);
    visitor("WaterLakeMaximumFlowSpeed", value.WaterLakeMaximumFlowSpeed);
}

template <class V>
void visit_fields(CEnvironmentThemeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TimeOfDay", value.TimeOfDay);
    visitor("MoonLit", value.MoonLit);
    visitor("FogStartZ", value.FogStartZ);
    visitor("FogEndZ", value.FogEndZ);
    visitor("SkyTexture0", value.SkyTexture0);
    visitor("SkyTexture1", value.SkyTexture1);
    visitor("SkyTexture1Blend", value.SkyTexture1Blend);
    visitor("CloudLowerLayerTexture0", value.CloudLowerLayerTexture0);
    visitor("CloudLowerLayerTexture1", value.CloudLowerLayerTexture1);
    visitor("CloudLowerLayerTexture1Blend", value.CloudLowerLayerTexture1Blend);
    visitor("CloudLowerLayerSpeedMultiplier", value.CloudLowerLayerSpeedMultiplier);
    visitor("CloudUpperLayerTexture0", value.CloudUpperLayerTexture0);
    visitor("CloudUpperLayerTexture1", value.CloudUpperLayerTexture1);
    visitor("CloudUpperLayerTexture1Blend", value.CloudUpperLayerTexture1Blend);
    visitor("CloudUpperLayerSpeedMultiplier", value.CloudUpperLayerSpeedMultiplier);
    visitor("WaterColourToReflectionBlend", value.WaterColourToReflectionBlend);
    visitor("WaterAlphaFactor", value.WaterAlphaFactor);
    visitor("WaterSpecularHightlightFactor", value.WaterSpecularHightlightFactor);
    visitor("RainStrength", value.RainStrength);
    visitor("SnowStrength", value.SnowStrength);
    visitor("MistMaxAlpha", value.MistMaxAlpha);
    visitor("LightningFrequency", value.LightningFrequency);
    visitor("GlowThresholdScale", value.GlowThresholdScale);
    visitor("GlowBloomScale", value.GlowBloomScale);
    visitor("GlowMotionBlur", value.GlowMotionBlur);
    visitor("ShadowFactor", value.ShadowFactor);
    visitor("FadedShadowFactor", value.FadedShadowFactor);
    visitor("WaterRefractionBlendStart", value.WaterRefractionBlendStart);
    visitor("WaterRefractionBlendEnd", value.WaterRefractionBlendEnd);
    visitor("WaterFlowSpeedFactor", value.WaterFlowSpeedFactor);
    visitor("WaterOscilationSpeed", value.WaterOscilationSpeed);
    visitor("WaterSwellFactor", value.WaterSwellFactor);
    visitor("WaterShoreSwellFactor", value.WaterShoreSwellFactor);
    visitor("WaterReflectionOffset", value.WaterReflectionOffset);
}

template <class V>
void visit_fields(CRGBColour &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IntValue", value.IntValue);
}

template <class V>
void visit_fields(CEnvironmentThemeDaySetDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TimeTheme", value.TimeTheme);
    visitor("SunTilt", value.SunTilt);
    visitor("SunRotate", value.SunRotate);
    visitor("SunHeight", value.SunHeight);
    visitor("MoonTilt", value.MoonTilt);
    visitor("MoonRotate", value.MoonRotate);
    visitor("MoonHeight", value.MoonHeight);
    visitor("ColourLookupColumn", value.ColourLookupColumn);
    visitor("EditorColour", value.EditorColour);
    visitor("FishWeightMult", value.FishWeightMult);
}

template <class V>
void visit_fields(CExperienceDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("OrbCombiningParticleEffect", value.OrbCombiningParticleEffect);
    visitor("ParticleScalesPerExperiencePoints", value.ParticleScalesPerExperiencePoints);
    visitor("PickupParticleEffectsPerExperiencePoints", value.PickupParticleEffectsPerExperiencePoints);
    visitor("LifeTimeForDistanceThresholds", value.LifeTimeForDistanceThresholds);
}

template <class V>
void visit_fields(CExplodingObjectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TriggeredOnCreatureProximity", value.TriggeredOnCreatureProximity);
    visitor("TriggerRadius", value.TriggerRadius);
    visitor("ExplosionDef", value.ExplosionDef);
    visitor("ProximityParticleEffect", value.ProximityParticleEffect);
}

template <class V>
void visit_fields(CExplosionRing &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumExplosions", value.NumExplosions);
    visitor("RingRadius", value.RingRadius);
    visitor("AngleOffset", value.AngleOffset);
    visitor("SecondsBeforeSpawnNewRing", value.SecondsBeforeSpawnNewRing);
}

template <class V>
void visit_fields(CExplosionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Damage", value.Damage);
    visitor("ExplosionRadius", value.ExplosionRadius);
    visitor("FireDamage", value.FireDamage);
    visitor("ExplosionLifeTime", value.ExplosionLifeTime);
    visitor("DamageFalloffWithRadius", value.DamageFalloffWithRadius);
    visitor("ExplosionBlendSmall", value.ExplosionBlendSmall);
    visitor("ExplosionBlendLarge", value.ExplosionBlendLarge);
    visitor("ExplosionReplacementBlendSmall", value.ExplosionReplacementBlendSmall);
    visitor("ExplosionReplacementBlendLarge", value.ExplosionReplacementBlendLarge);
    visitor("SecondsBeforeEffectReplacement", value.SecondsBeforeEffectReplacement);
    visitor("ExplosionRadiusSmall", value.ExplosionRadiusSmall);
    visitor("ExplosionRadiusLarge", value.ExplosionRadiusLarge);
    visitor("ExplosionRings", value.ExplosionRings);
    visitor("ExplosionShouldKnockdown", value.ExplosionShouldKnockdown);
    visitor("CauseFireEffectOnHitCreatures", value.CauseFireEffectOnHitCreatures);
    visitor("CausesRumble", value.CausesRumble);
    visitor("RumbleStrength", value.RumbleStrength);
    visitor("RumbleLength", value.RumbleLength);
}

template <class V>
void visit_fields(CExplosiveTrailDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TrapDefIndex", value.TrapDefIndex);
    visitor("SecondsBetweenTraps", value.SecondsBetweenTraps);
    visitor("DistanceBetweenTraps", value.DistanceBetweenTraps);
    visitor("SecondsForTrapPersistance", value.SecondsForTrapPersistance);
    visitor("SecondsTrailLifetime", value.SecondsTrailLifetime);
    visitor("DistanceForLateralDeviation", value.DistanceForLateralDeviation);
    visitor("DegreesMaxTurnTowardsTargetPerTrap", value.DegreesMaxTurnTowardsTargetPerTrap);
    visitor("FractionOfLifetimeToTurnTowardsTarget", value.FractionOfLifetimeToTurnTowardsTarget);
    visitor("SecondsIcicleAttackTargetPositionPredictionTime", value.SecondsIcicleAttackTargetPositionPredictionTime);
}

template <class V>
void visit_fields(CExpressionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Animation", value.Animation);
    visitor("AnimationInto", value.AnimationInto);
    visitor("AnimationLoop", value.AnimationLoop);
    visitor("AnimationOutOf", value.AnimationOutOf);
    visitor("AnimationToOther", value.AnimationToOther);
    visitor("AnimationToOtherInto", value.AnimationToOtherInto);
    visitor("AnimationToOtherLoop", value.AnimationToOtherLoop);
    visitor("AnimationToOtherOutOf", value.AnimationToOtherOutOf);
    visitor("SpriteDuration", value.SpriteDuration);
    visitor("SpriteGraphic", value.SpriteGraphic);
    visitor("SpriteOverrideText", value.SpriteOverrideText);
    visitor("SoundCriteria", value.SoundCriteria);
    visitor("LoopSoundCriteria", value.LoopSoundCriteria);
    visitor("DummyObject", value.DummyObject);
    visitor("GameEventType", value.GameEventType);
    visitor("ScriptName", value.ScriptName);
    visitor("ScriptLoadResources", value.ScriptLoadResources);
    visitor("DeactivateScriptAtEnd", value.DeactivateScriptAtEnd);
    visitor("PerformAfterScript", value.PerformAfterScript);
    visitor("OpinionDeedType", value.OpinionDeedType);
    visitor("Extendable", value.Extendable);
    visitor("SheatheWeapons", value.SheatheWeapons);
    visitor("ReplaceHeldItemWhenDone", value.ReplaceHeldItemWhenDone);
    visitor("TextTag", value.TextTag);
    visitor("ExpressionType", value.ExpressionType);
}

template <class V>
void visit_fields(CExpressionSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Expressions", value.Expressions);
    visitor("TrailingU32", value.TrailingU32);
}

template <class V>
void visit_fields(CExpressionSet::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("AnimIndex", value.AnimIndex);
}

template <class V>
void visit_fields(CExpressionSubDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ExpressionDef", value.ExpressionDef);
}

template <class V>
void visit_fields(CFactionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("EnemyFactions", value.EnemyFactions);
    visitor("AlliedFactions", value.AlliedFactions);
}

template <class V>
void visit_fields(CFeatDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FeatName", value.FeatName);
    visitor("Verb", value.Verb);
    visitor("TimeLimit", value.TimeLimit);
    visitor("TargetNumber", value.TargetNumber);
    visitor("GoldReward", value.GoldReward);
    visitor("XPReward", value.XPReward);
    visitor("ItemReward", value.ItemReward);
    visitor("NoBlocking", value.NoBlocking);
    visitor("KN_AttackType", value.KN_AttackType);
    visitor("KN_Perfect", value.KN_Perfect);
    visitor("KN_CreatureType", value.KN_CreatureType);
    visitor("GF_FromRegion", value.GF_FromRegion);
    visitor("GF_ToRegion", value.GF_ToRegion);
    visitor("GF_NoTeleporting", value.GF_NoTeleporting);
    visitor("CO_ItemName", value.CO_ItemName);
}

template <class V>
void visit_fields(CFireEffectCreationSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ContainmentMap", value.ContainmentMap);
}

template <class V>
void visit_fields(CFireHeartPatternDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PadActive", value.PadActive);
}

template <class V>
void visit_fields(CFireballSpellLevelDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SpellLevel", value.SpellLevel);
}

template <class V>
void visit_fields(CFireheartMinigameDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MoonFX", value.MoonFX);
    visitor("SunFX", value.SunFX);
    visitor("QuitText", value.QuitText);
    visitor("YesText", value.YesText);
    visitor("NoText", value.NoText);
}

template <class V>
void visit_fields(CFishDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinLength", value.MinLength);
    visitor("MaxLength", value.MaxLength);
    visitor("MinWeight", value.MinWeight);
    visitor("MaxWeight", value.MaxWeight);
    visitor("MinSpeed", value.MinSpeed);
    visitor("MaxSpeed", value.MaxSpeed);
    visitor("BasePullSeconds", value.BasePullSeconds);
    visitor("MaxRandomPullSeconds", value.MaxRandomPullSeconds);
    visitor("BasePullWaitSeconds", value.BasePullWaitSeconds);
    visitor("MaxRandomPullWaitSeconds", value.MaxRandomPullWaitSeconds);
    visitor("Level", value.Level);
    visitor("InfoText", value.InfoText);
}

template <class V>
void visit_fields(CFishingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FishTypes", value.FishTypes);
    visitor("FishingSpotProximity", value.FishingSpotProximity);
    visitor("FishingSpotMinDepth", value.FishingSpotMinDepth);
    visitor("FishingSpotMinHeightDiff", value.FishingSpotMinHeightDiff);
    visitor("FishingSpotMaxHeightDiff", value.FishingSpotMaxHeightDiff);
    visitor("PointsPerFishingLevel", value.PointsPerFishingLevel);
    visitor("SlowFishingSpeed", value.SlowFishingSpeed);
    visitor("NormalFishingSpeed", value.NormalFishingSpeed);
    visitor("FastFishingSpeed", value.FastFishingSpeed);
    visitor("FastestFishingSpeed", value.FastestFishingSpeed);
    visitor("FishWeightRainMult", value.FishWeightRainMult);
    visitor("FishWeightSnowMult", value.FishWeightSnowMult);
    visitor("FishWeightMistMult", value.FishWeightMistMult);
    visitor("BaseWaitPeriodSeconds", value.BaseWaitPeriodSeconds);
    visitor("MaxRandomWaitPeriodSeconds", value.MaxRandomWaitPeriodSeconds);
    visitor("HookWindowSeconds", value.HookWindowSeconds);
    visitor("BaseHookPresses", value.BaseHookPresses);
    visitor("MaxRandomHookPresses", value.MaxRandomHookPresses);
    visitor("StrainIncreasePerPress", value.StrainIncreasePerPress);
}

template <class V>
void visit_fields(CFishingRodDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StrainReductionPerSecond", value.StrainReductionPerSecond);
    visitor("DistanceDecreasePerPress", value.DistanceDecreasePerPress);
    visitor("PullingDistanceDecreasePerPress", value.PullingDistanceDecreasePerPress);
    visitor("MaxStrain", value.MaxStrain);
    visitor("MaxLengthMetres", value.MaxLengthMetres);
}

template <class V>
void visit_fields(CFlammableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FireResistance", value.FireResistance);
    visitor("MaxDamageCaused", value.MaxDamageCaused);
    visitor("FireDamagePerCycle", value.FireDamagePerCycle);
    visitor("FireDamagePeriod", value.FireDamagePeriod);
    visitor("EffectCreationSet", value.EffectCreationSet);
}

template <class V>
void visit_fields(CFrontEndDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("vAttractModeMovie", value.vAttractModeMovie);
    visitor("ErrorMessageBackgroundGraphic", value.ErrorMessageBackgroundGraphic);
    visitor("ButtonABigGraphic", value.ButtonABigGraphic);
    visitor("ButtonBBigGraphic", value.ButtonBBigGraphic);
}

template <class V>
void visit_fields(CGiftDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("IsWeddingRing", value.IsWeddingRing);
}

template <class V>
void visit_fields(NSpeechGainManager::CDialogueLayerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ResponseTimeSecsAttack", value.ResponseTimeSecsAttack);
    visitor("ResponseTimeSecsRelease", value.ResponseTimeSecsRelease);
    visitor("GainLevelHigh", value.GainLevelHigh);
    visitor("GainLevelMid", value.GainLevelMid);
    visitor("GainLevelLow", value.GainLevelLow);
    visitor("FocusShiftDistanceThreshold", value.FocusShiftDistanceThreshold);
}

template <class V>
void visit_fields(CGlobalDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ControlSchemePC", value.ControlSchemePC);
    visitor("ControlSchemePCAggressive", value.ControlSchemePCAggressive);
    visitor("ControlSchemeXBOX", value.ControlSchemeXBOX);
    visitor("ControlSchemeXBOXAggressive", value.ControlSchemeXBOXAggressive);
    visitor("PhysicsTexture", value.PhysicsTexture);
    visitor("InvalidThemeStandin", value.InvalidThemeStandin);
    visitor("DefaultEngineThemeDef", value.DefaultEngineThemeDef);
    visitor("StaticMapGenerationEstimatedMaxCameraHeightAboveLandscape", value.StaticMapGenerationEstimatedMaxCameraHeightAboveLandscape);
    visitor("GraphicWeaponTrail", value.GraphicWeaponTrail);
    visitor("GraphicWeaponTrail2", value.GraphicWeaponTrail2);
    visitor("GraphicWeaponTrailArrow", value.GraphicWeaponTrailArrow);
    visitor("WeaponTrailAlpha", value.WeaponTrailAlpha);
    visitor("WeaponTrailAlphaFadeOffTime", value.WeaponTrailAlphaFadeOffTime);
    visitor("WeaponTrailArrowAlpha", value.WeaponTrailArrowAlpha);
    visitor("WeaponTrailArrowAlphaFadeOffTime", value.WeaponTrailArrowAlphaFadeOffTime);
    visitor("WeaponTrialNumFramesToFadeOut", value.WeaponTrialNumFramesToFadeOut);
    visitor("HealthPickupValue", value.HealthPickupValue);
    visitor("RespawnDelay", value.RespawnDelay);
    visitor("ScrabbleAngle", value.ScrabbleAngle);
    visitor("SlideAngle", value.SlideAngle);
    visitor("MaxFollowers", value.MaxFollowers);
    visitor("PickupGoldSoundCriteria", value.PickupGoldSoundCriteria);
    visitor("PickupItemSoundCriteria", value.PickupItemSoundCriteria);
    visitor("OnClickSoundCriteria", value.OnClickSoundCriteria);
    visitor("EditorSpangleCursor", value.EditorSpangleCursor);
    visitor("EditorCircleSprite", value.EditorCircleSprite);
    visitor("EditorSecondaryCircleSprite", value.EditorSecondaryCircleSprite);
    visitor("TestSpeech", value.TestSpeech);
    visitor("MimeSpeechIndex", value.MimeSpeechIndex);
    visitor("AdultSleepTimeStart", value.AdultSleepTimeStart);
    visitor("AdultSleepTimeEnd", value.AdultSleepTimeEnd);
    visitor("ChildSleepTimeStart", value.ChildSleepTimeStart);
    visitor("ChildSleepTimeEnd", value.ChildSleepTimeEnd);
    visitor("ElderlySleepTimeStart", value.ElderlySleepTimeStart);
    visitor("ElderlySleepTimeEnd", value.ElderlySleepTimeEnd);
    visitor("CreatureGeneratorRegenDelaySeconds", value.CreatureGeneratorRegenDelaySeconds);
    visitor("Music", value.Music);
    visitor("AgeIncreasePerLevelUp", value.AgeIncreasePerLevelUp);
    visitor("E3BodgeFireflyGraphic", value.E3BodgeFireflyGraphic);
    visitor("SwitchGraphic", value.SwitchGraphic);
    visitor("TrackGraphic", value.TrackGraphic);
    visitor("ConversationCameras", value.ConversationCameras);
    visitor("PermittedAINarrators", value.PermittedAINarrators);
    visitor("DialogueLayers", value.DialogueLayers);
    visitor("OpinionDebugIndicatorIcon", value.OpinionDebugIndicatorIcon);
    visitor("WifeLoveIndicatorIcon", value.WifeLoveIndicatorIcon);
    visitor("WifeEngagedIndicatorIcon", value.WifeEngagedIndicatorIcon);
    visitor("WifeMarriedIndicatorIcon", value.WifeMarriedIndicatorIcon);
    visitor("ExpressionIndicatorIcon", value.ExpressionIndicatorIcon);
    visitor("FollowerIndicatorIcon", value.FollowerIndicatorIcon);
    visitor("EmoteIconDrawDistance", value.EmoteIconDrawDistance);
    visitor("AngleToBeConsideredVertical", value.AngleToBeConsideredVertical);
    visitor("SoundGainLowerValueWhenInsideOutside", value.SoundGainLowerValueWhenInsideOutside);
    visitor("TeleportOutParticleEffect", value.TeleportOutParticleEffect);
    visitor("TeleportFadeOutTimeInSeconds", value.TeleportFadeOutTimeInSeconds);
    visitor("TeleportInParticleEffect", value.TeleportInParticleEffect);
    visitor("TeleportFadeInTimeInSeconds", value.TeleportFadeInTimeInSeconds);
    visitor("DamageForMaximumBlood", value.DamageForMaximumBlood);
}

template <class V>
void visit_fields(CGoldDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Gold", value.Gold);
}

template <class V>
void visit_fields(CGuardDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BribeCostWeaponOut", value.BribeCostWeaponOut);
    visitor("BribeCostTrespass", value.BribeCostTrespass);
    visitor("BribeCostVandalism", value.BribeCostVandalism);
    visitor("BribeCostShoplifter", value.BribeCostShoplifter);
    visitor("BribeCostPickPocket", value.BribeCostPickPocket);
    visitor("BribeCostAssault", value.BribeCostAssault);
    visitor("BribeCostGuardAssault", value.BribeCostGuardAssault);
    visitor("BribeCostGBH", value.BribeCostGBH);
    visitor("BribeCostGuardGBH", value.BribeCostGuardGBH);
    visitor("BribeCostMurder", value.BribeCostMurder);
}

template <class V>
void visit_fields(CGuiGraphicDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Outer", value.Outer);
    visitor("InnerLeft", value.InnerLeft);
    visitor("InnerMiddle", value.InnerMiddle);
    visitor("InnerRight", value.InnerRight);
    visitor("InnerBackLeft", value.InnerBackLeft);
    visitor("InnerBackMiddle", value.InnerBackMiddle);
    visitor("InnerBackRight", value.InnerBackRight);
    visitor("TextColour", value.TextColour);
}

template <class V>
void visit_fields(CGuildMasterDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinSecondsBetweenTeleport", value.MinSecondsBetweenTeleport);
    visitor("MinSecondsBetweenWillUse", value.MinSecondsBetweenWillUse);
}

template <class V>
void visit_fields(CHairCardDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HairObject", value.HairObject);
}

template <class V>
void visit_fields(CHasNameDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DefaultNameTag", value.DefaultNameTag);
    visitor("Home", value.Home);
    visitor("Occupation", value.Occupation);
}

template <class V>
void visit_fields(CHeroAbilityDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Ability", value.Ability);
    visitor("Stat", value.Stat);
    visitor("IconGraphicIndex", value.IconGraphicIndex);
    visitor("IconEffectIndex", value.IconEffectIndex);
    visitor("IconActiveEffectIndex", value.IconActiveEffectIndex);
    visitor("IconEffectOffset", value.IconEffectOffset);
    visitor("Name", value.Name);
    visitor("Description", value.Description);
    visitor("LevDescription", value.LevDescription);
    visitor("MaxedOutDescription", value.MaxedOutDescription);
    visitor("StaminaCost", value.StaminaCost);
    visitor("StatExperienceGainedOnUse", value.StatExperienceGainedOnUse);
    visitor("ExperienceCostsToUpgrade", value.ExperienceCostsToUpgrade);
    visitor("ABXYPriority", value.ABXYPriority);
    visitor("MoralityCostFactor", value.MoralityCostFactor);
    visitor("Aggressive", value.Aggressive);
    visitor("DummyObject", value.DummyObject);
}

template <class V>
void visit_fields(CHeroAbilityStatPackage &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StatThresholds", value.StatThresholds);
}

template <class V>
void visit_fields(CHeroCentreDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
}

template <class V>
void visit_fields(CHeroCombatDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("RangedAccuracySecondsForMaximumWithSkill", value.RangedAccuracySecondsForMaximumWithSkill);
    visitor("RangedAccuracyPercentFractionWithBuildup", value.RangedAccuracyPercentFractionWithBuildup);
    visitor("RangedAccuracyPercentWithStats", value.RangedAccuracyPercentWithStats);
    visitor("RangedAccuracyWeighting", value.RangedAccuracyWeighting);
    visitor("RangedAccuracyShakeyCursor", value.RangedAccuracyShakeyCursor);
    visitor("RangedAccuracyShakeyCursorWeighting", value.RangedAccuracyShakeyCursorWeighting);
    visitor("RangedAccuracyShakeFractionOfRadius", value.RangedAccuracyShakeFractionOfRadius);
    visitor("RangedDamageMultiplierWeighting", value.RangedDamageMultiplierWeighting);
    visitor("RangedDamageMultiplierSpeedWithStatParameter", value.RangedDamageMultiplierSpeedWithStatParameter);
    visitor("RangedDamageMultiplierSpeedWithDistance", value.RangedDamageMultiplierSpeedWithDistance);
    visitor("RangedDamageMultiplierSpeedNominalBuildup", value.RangedDamageMultiplierSpeedNominalBuildup);
    visitor("RangedDamageMultiplierScaleNominalBuildup", value.RangedDamageMultiplierScaleNominalBuildup);
    visitor("RangedDamageMultiplierSpeedSuperBuildup", value.RangedDamageMultiplierSpeedSuperBuildup);
    visitor("RangedDamageMultiplierScaleSuperBuildup", value.RangedDamageMultiplierScaleSuperBuildup);
    visitor("RangedWeaponReloadSpeedWeighting", value.RangedWeaponReloadSpeedWeighting);
    visitor("MeleeDamageMultiplierWithStats", value.MeleeDamageMultiplierWithStats);
    visitor("MeleeDamageWeighting", value.MeleeDamageWeighting);
    visitor("HitDamageMultiplierWithStats", value.HitDamageMultiplierWithStats);
    visitor("HitDamageWeighting", value.HitDamageWeighting);
    visitor("MeleeFlourishValidOnMultiplierLevelIncrease", value.MeleeFlourishValidOnMultiplierLevelIncrease);
    visitor("MeleeFlourishNumMultiplierLevelsPerFlourish", value.MeleeFlourishNumMultiplierLevelsPerFlourish);
    visitor("MeleeFlourishAllowBreakingOfRotation", value.MeleeFlourishAllowBreakingOfRotation);
    visitor("MeleeFlourishSecondsForAvailibility", value.MeleeFlourishSecondsForAvailibility);
    visitor("MeleeFlourishOneShot", value.MeleeFlourishOneShot);
    visitor("MeleeFlourishUsable", value.MeleeFlourishUsable);
    visitor("MeleeCombatPlayStrikeSounds", value.MeleeCombatPlayStrikeSounds);
    visitor("TargetingPressureFractionThresholdForToggle", value.TargetingPressureFractionThresholdForToggle);
    visitor("MaxAngleToTurnToShootAtTargetInThirdPersonBowMode", value.MaxAngleToTurnToShootAtTargetInThirdPersonBowMode);
    visitor("MeleeStrikeNumFramesPauseOnHeroStrike", value.MeleeStrikeNumFramesPauseOnHeroStrike);
    visitor("MeleeStrikeAnimationSpeedFractionWithStats", value.MeleeStrikeAnimationSpeedFractionWithStats);
    visitor("MeleeStrikeAnimationSpeedWeighting", value.MeleeStrikeAnimationSpeedWeighting);
    visitor("AutoBlockSkillThreshold", value.AutoBlockSkillThreshold);
    visitor("AutoBlockStrengthThreshold", value.AutoBlockStrengthThreshold);
    visitor("MeleeRecoilMaintainHandednessSkillThreshold", value.MeleeRecoilMaintainHandednessSkillThreshold);
    visitor("MeleeRecoilMaintainHandednessStrengthThreshold", value.MeleeRecoilMaintainHandednessStrengthThreshold);
    visitor("MeleeRecoilMaintainComboChainSkillThreshold", value.MeleeRecoilMaintainComboChainSkillThreshold);
    visitor("MeleeRecoilMaintainComboChainStrengthThreshold", value.MeleeRecoilMaintainComboChainStrengthThreshold);
    visitor("DefaultCombatAbilities", value.DefaultCombatAbilities);
    visitor("ProjectileWeaponFirstPersonModeMaxFOVValues", value.ProjectileWeaponFirstPersonModeMaxFOVValues);
    visitor("ProjectileTargetingAcquireTargetTag", value.ProjectileTargetingAcquireTargetTag);
    visitor("ProjectileTargetingAcquireTargetsAuto", value.ProjectileTargetingAcquireTargetsAuto);
    visitor("ProjectileTargetingUseAnalogueZoom", value.ProjectileTargetingUseAnalogueZoom);
    visitor("ProjectileWeaponAnalogueZoomControlWeight", value.ProjectileWeaponAnalogueZoomControlWeight);
    visitor("HeavyWeapons", value.HeavyWeapons);
}

template <class V>
void visit_fields(CHeroDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DefaultTitle", value.DefaultTitle);
    visitor("DefaultHeroTrainingStatus", value.DefaultHeroTrainingStatus);
}

template <class V>
void visit_fields(CHeroStatIncreaseData &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StatIncreases", value.StatIncreases);
}

template <class V>
void visit_fields(CHeroExperienceDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ExperienceLevelThresholds", value.ExperienceLevelThresholds);
    visitor("HeroLevelTitles", value.HeroLevelTitles);
    visitor("FallbackMultipliers", value.FallbackMultipliers);
    visitor("SoundIntensityThresholds", value.SoundIntensityThresholds);
    visitor("ComboMultiplierMap", value.ComboMultiplierMap);
    visitor("ExperienceMultiplierParamPerUnitComboMultiplier", value.ExperienceMultiplierParamPerUnitComboMultiplier);
    visitor("ExperienceMultiplierRegionChangeMultiplier", value.ExperienceMultiplierRegionChangeMultiplier);
    visitor("ComboMultiplierIncrease", value.ComboMultiplierIncrease);
    visitor("ComboMultiplierLock", value.ComboMultiplierLock);
    visitor("ComboMultiplierLost", value.ComboMultiplierLost);
    visitor("ComboMultiplierRunningHitsIncrease", value.ComboMultiplierRunningHitsIncrease);
    visitor("StatIncreasePerHitType", value.StatIncreasePerHitType);
    visitor("StatIncreasePerHitType2", value.StatIncreasePerHitType2);
    visitor("AttackHistoryCombos", value.AttackHistoryCombos);
}

template <class V>
void visit_fields(CHeroMarriageDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("WeddingTimeForScreenToFadeOut", value.WeddingTimeForScreenToFadeOut);
    visitor("WeddingTimeForScreenToFadeIn", value.WeddingTimeForScreenToFadeIn);
    visitor("WeddingAfterFrescoTimeOfDayFastForwardTo", value.WeddingAfterFrescoTimeOfDayFastForwardTo);
    visitor("SecondsJustMarriedDuration", value.SecondsJustMarriedDuration);
    visitor("HeroStatRenownIncreasePerMarriage", value.HeroStatRenownIncreasePerMarriage);
    visitor("SecondsBetweenCheckingPlayersAppearance", value.SecondsBetweenCheckingPlayersAppearance);
    visitor("SecondsMaxTimeWithinNoticingCanReact", value.SecondsMaxTimeWithinNoticingCanReact);
    visitor("FatnessChangeThresholdForComment", value.FatnessChangeThresholdForComment);
    visitor("SecondsBetweenReceivingGiftOpinionReactionCanHappen", value.SecondsBetweenReceivingGiftOpinionReactionCanHappen);
    visitor("SecondsThatGiftsReceivedWillBeRemembered", value.SecondsThatGiftsReceivedWillBeRemembered);
    visitor("FramesBetweenCullingOldGifts", value.FramesBetweenCullingOldGifts);
    visitor("SecondsBetweenEvaluatingGiftOpinion", value.SecondsBetweenEvaluatingGiftOpinion);
    visitor("SecondsBetweenGiftGivingOpportunities", value.SecondsBetweenGiftGivingOpportunities);
    visitor("ChanceThatGiftWillBeGiven", value.ChanceThatGiftWillBeGiven);
    visitor("AmountOfGoldPerHourOfMarriageToIncreaseGiftGivingPriceValuePerHourOfPlay", value.AmountOfGoldPerHourOfMarriageToIncreaseGiftGivingPriceValuePerHourOfPlay);
    visitor("MaxNumHoursOfMarriageToIncreaseGiftGivingPriceValue", value.MaxNumHoursOfMarriageToIncreaseGiftGivingPriceValue);
    visitor("MultiplierForGiftGivingValueEveryOpportunity", value.MultiplierForGiftGivingValueEveryOpportunity);
    visitor("MaxDistanceFromMaxStatsThatGiftsCanBeGiven", value.MaxDistanceFromMaxStatsThatGiftsCanBeGiven);
    visitor("SecondsBetweenEvaluatingLoveAttitude", value.SecondsBetweenEvaluatingLoveAttitude);
    visitor("SecondsOutOfTheRegionForLoveAttitudeToDecayOneToZero", value.SecondsOutOfTheRegionForLoveAttitudeToDecayOneToZero);
    visitor("ThresholdForLoveAttitudeOverride", value.ThresholdForLoveAttitudeOverride);
    visitor("FallenInLoveSoundCriteria", value.FallenInLoveSoundCriteria);
    visitor("FallenInLoveSoundGain", value.FallenInLoveSoundGain);
    visitor("FianceeOpinionPersonalityDef", value.FianceeOpinionPersonalityDef);
    visitor("ProbabilityOfCourtingAdviceOnGreet", value.ProbabilityOfCourtingAdviceOnGreet);
    visitor("SecondsTimeSinceSeeingYouMinThreshold", value.SecondsTimeSinceSeeingYouMinThreshold);
    visitor("SecondsRunOutWifeOnViolenceHateSpike", value.SecondsRunOutWifeOnViolenceHateSpike);
    visitor("PeakEffectWifeOnViolenceHateSpike", value.PeakEffectWifeOnViolenceHateSpike);
    visitor("PersistEffectWifeOnViolenceHateSpike", value.PersistEffectWifeOnViolenceHateSpike);
    visitor("SecsInHateBeforeDivorceOccurs", value.SecsInHateBeforeDivorceOccurs);
    visitor("SecsInHateBetweenDivorceWarnings", value.SecsInHateBetweenDivorceWarnings);
    visitor("TextGuiWifeDivorceOccurred", value.TextGuiWifeDivorceOccurred);
    visitor("TextGuiWifeDivorceRenting", value.TextGuiWifeDivorceRenting);
    visitor("HeroStatMoralityInOpinionFormChangePerDivorce", value.HeroStatMoralityInOpinionFormChangePerDivorce);
    visitor("SecsInLoveAndHusbandPresentBeforeSexOffered", value.SecsInLoveAndHusbandPresentBeforeSexOffered);
    visitor("SecsInLoveAndHusbandPresentBetweenSexOffers", value.SecsInLoveAndHusbandPresentBetweenSexOffers);
    visitor("TextGuiWifeSexOffered", value.TextGuiWifeSexOffered);
    visitor("SecsMinSoundDurationInSexCutscene", value.SecsMinSoundDurationInSexCutscene);
    visitor("SecsWaitAfterSoundInSexCutscene", value.SecsWaitAfterSoundInSexCutscene);
    visitor("DialogueGroupForSexCutscene", value.DialogueGroupForSexCutscene);
}

template <class V>
void visit_fields(CParticleMorphs &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Morphs", value.Morphs);
    visitor("TrailingU32", value.TrailingU32);
}

template <class V>
void visit_fields(CParticleMorphs::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MorphType", value.MorphType);
    visitor("Blending", value.Blending);
    visitor("Location", value.Location);
    visitor("LocationName", value.LocationName);
    visitor("LocationIndex", value.LocationIndex);
    visitor("ParticleIndex", value.ParticleIndex);
    visitor("SecondParticleIndex", value.SecondParticleIndex);
    visitor("StartAtMorphStrength", value.StartAtMorphStrength);
    visitor("EndAtMorphStrength", value.EndAtMorphStrength);
}

template <class V>
void visit_fields(CSkeletalMorphs &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Morphs", value.Morphs);
}

template <class V>
void visit_fields(CSkeletalMorphs::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MorphName", value.MorphName);
    visitor("Priority", value.Priority);
    visitor("SupressesLowerMorphs", value.SupressesLowerMorphs);
}

template <class V>
void visit_fields(CTextureMorphEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MorphType", value.MorphType);
    visitor("TextureLayer", value.TextureLayer);
    visitor("ReplacingTextureIndex", value.ReplacingTextureIndex);
    visitor("BankIndex", value.BankIndex);
    visitor("Blend", value.Blend);
}

template <class V>
void visit_fields(CTextureMorphs &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Morphs", value.Morphs);
    visitor("TrailingU32", value.TrailingU32);
}

template <class V>
void visit_fields(CHeroMorphDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TextureMorphs", value.TextureMorphs);
    visitor("SkeletalMorphs", value.SkeletalMorphs);
    visitor("ParticleMorphs", value.ParticleMorphs);
    visitor("IdleParticleMorphs", value.IdleParticleMorphs);
}

template <class V>
void visit_fields(CHeroPostcardGeneratorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MeshAreaTLPos", value.MeshAreaTLPos);
    visitor("MeshAreaBRPos", value.MeshAreaBRPos);
    visitor("DollCentreOffset", value.DollCentreOffset);
    visitor("DollBoundingSphereRadius", value.DollBoundingSphereRadius);
}

template <class V>
void visit_fields(CHeroSoulsCreatureDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CreatureType", value.CreatureType);
    visitor("Location", value.Location);
    visitor("NumCreatures", value.NumCreatures);
}

template <class V>
void visit_fields(CHeroSoulsWaveDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumWaveCreatures", value.NumWaveCreatures);
    visitor("Creatures", value.Creatures);
}

template <class V>
void visit_fields(CHeroSoulsRoundDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumWaves", value.NumWaves);
    visitor("Waves", value.Waves);
}

template <class V>
void visit_fields(CHeroSpecialMovementDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TotalTimeToSprintSeconds", value.TotalTimeToSprintSeconds);
    visitor("MinimumNumberOfSecondsLeftToBeAllowedToSprint", value.MinimumNumberOfSecondsLeftToBeAllowedToSprint);
    visitor("TotalTimeToFullyRestoreSprintBarSeconds", value.TotalTimeToFullyRestoreSprintBarSeconds);
    visitor("NoSecondsToRunBeforeSprintStarts", value.NoSecondsToRunBeforeSprintStarts);
}

template <class V>
void visit_fields(CHeroStatsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MaximumStamina", value.MaximumStamina);
    visitor("NumberOfSecondsAfterUseStaminaStartsRefilling", value.NumberOfSecondsAfterUseStaminaStartsRefilling);
    visitor("NumberOfSecondsAfterChangeHealthStartsRefilling", value.NumberOfSecondsAfterChangeHealthStartsRefilling);
    visitor("NumberOfSecondsBetweenHealthRefills", value.NumberOfSecondsBetweenHealthRefills);
    visitor("StaminaToRegainPerSecond", value.StaminaToRegainPerSecond);
    visitor("HealthToRegainPerRefill", value.HealthToRegainPerRefill);
    visitor("RenownLossOnDeath", value.RenownLossOnDeath);
    visitor("MoneyLossOnDeath", value.MoneyLossOnDeath);
    visitor("MaximumMoney", value.MaximumMoney);
    visitor("MaximumExpToSpend", value.MaximumExpToSpend);
    visitor("ExpToSpendAtWhichToDisplayTutorial", value.ExpToSpendAtWhichToDisplayTutorial);
    visitor("MinAge", value.MinAge);
    visitor("MaxAge", value.MaxAge);
    visitor("AppearanceModifierScaling", value.AppearanceModifierScaling);
    visitor("AmountFatnessIncreasesPerPointOfOverEating", value.AmountFatnessIncreasesPerPointOfOverEating);
    visitor("NumberOfMinutesBetweenFatnessReductions", value.NumberOfMinutesBetweenFatnessReductions);
    visitor("AmountFatnessDecreasesPerReduction", value.AmountFatnessDecreasesPerReduction);
    visitor("DrinkAffectMultiplierMultiplier", value.DrinkAffectMultiplierMultiplier);
    visitor("MinVomitTime", value.MinVomitTime);
    visitor("MaxVomitTime", value.MaxVomitTime);
    visitor("MinTimeBeforeVomiting", value.MinTimeBeforeVomiting);
    visitor("MaxTimeBeforeVomiting", value.MaxTimeBeforeVomiting);
    visitor("PintsOfSobrietyPerSecondOfPuke", value.PintsOfSobrietyPerSecondOfPuke);
    visitor("PintsToDrunk", value.PintsToDrunk);
    visitor("PintsToVomit", value.PintsToVomit);
    visitor("AlcoholAbsorbtionPerSecond", value.AlcoholAbsorbtionPerSecond);
    visitor("RenownLevelCosts", value.RenownLevelCosts);
    visitor("RenownLevelMaxFollowers", value.RenownLevelMaxFollowers);
    visitor("RenownLevelMaxBoastingCrowd", value.RenownLevelMaxBoastingCrowd);
    visitor("MaxMorality", value.MaxMorality);
    visitor("MaxMoralityAsChild", value.MaxMoralityAsChild);
    visitor("MaxExtraMaxHitPoints", value.MaxExtraMaxHitPoints);
    visitor("MaxExtraMaxStaminaPoints", value.MaxExtraMaxStaminaPoints);
    visitor("MoralityChangeOnTheftFromHome", value.MoralityChangeOnTheftFromHome);
    visitor("MoralityChangeOnTheftFromShop", value.MoralityChangeOnTheftFromShop);
    visitor("MoralityChangeOnPicklock", value.MoralityChangeOnPicklock);
    visitor("MoralityChangeOnVandalism", value.MoralityChangeOnVandalism);
    visitor("TradeSkillForGuileLevel", value.TradeSkillForGuileLevel);
    visitor("StatLevels", value.StatLevels);
    visitor("MaxHitPointsPerHealthLevel", value.MaxHitPointsPerHealthLevel);
    visitor("MaxStaminaPointsPerMagicPowerLevel", value.MaxStaminaPointsPerMagicPowerLevel);
    visitor("SoundDistanceMultiplierPerStealthLevel", value.SoundDistanceMultiplierPerStealthLevel);
    visitor("VisibilityMultiplierPerStealthLevel", value.VisibilityMultiplierPerStealthLevel);
}

template <class V>
void visit_fields(CHeroSuitDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SuitParts", value.SuitParts);
}

template <class V>
void visit_fields(CHeroTitleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Title", value.Title);
    visitor("HeroTitleGuiTag", value.HeroTitleGuiTag);
    visitor("HeroTitleGreetToHeroTag", value.HeroTitleGreetToHeroTag);
    visitor("HeroTitleCommentAtHeroTag", value.HeroTitleCommentAtHeroTag);
    visitor("HeroTitleCommentToSelfTag", value.HeroTitleCommentToSelfTag);
    visitor("RequiredRenownLevel", value.RequiredRenownLevel);
    visitor("RequiredStrengthExpLevel", value.RequiredStrengthExpLevel);
    visitor("RequiredSkillExpLevel", value.RequiredSkillExpLevel);
    visitor("RequiredWillExpLevel", value.RequiredWillExpLevel);
    visitor("IsBuyable", value.IsBuyable);
    visitor("IsAutomatic", value.IsAutomatic);
}

template <class V>
void visit_fields(CHighlightItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ParticleEmitter", value.ParticleEmitter);
    visitor("PickupParticleEmitter", value.PickupParticleEmitter);
    visitor("RotationSpeed", value.RotationSpeed);
    visitor("DistanceFromGround", value.DistanceFromGround);
    visitor("BobDistance", value.BobDistance);
    visitor("BobTime", value.BobTime);
    visitor("StartAsRunning", value.StartAsRunning);
    visitor("StartAsRunningDelay", value.StartAsRunningDelay);
}

template <class V>
void visit_fields(NHitLocations::CPhysicalPrimitiveBase &value, V &&visitor) {
    (void)value;
    (void)visitor;
}

template <class V>
void visit_fields(CPhysicalPrimitiveInit &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("BaseName", value.BaseName);
    visitor("Radius", value.Radius);
    visitor("Height", value.Height);
}

template <class V>
void visit_fields(CHitLocationDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PhysicalPrimitive", value.PhysicalPrimitive);
    visitor("Armour", value.Armour);
    visitor("HitResponseAction", value.HitResponseAction);
    visitor("HitResponseToEpicSpellsAction", value.HitResponseToEpicSpellsAction);
    visitor("HitKnockdownAction", value.HitKnockdownAction);
    visitor("HitKnockdownDeathAction", value.HitKnockdownDeathAction);
    visitor("BlockResponseAction", value.BlockResponseAction);
    visitor("HitDamageEffect", value.HitDamageEffect);
    visitor("HitBlockedEffect", value.HitBlockedEffect);
    visitor("VulnerableEffect", value.VulnerableEffect);
    visitor("Flags", value.Flags);
    visitor("PlayGetHitResponse", value.PlayGetHitResponse);
    visitor("DecapitateOnHit", value.DecapitateOnHit);
    visitor("DefendableAgainstShots", value.DefendableAgainstShots);
    visitor("DefendableAgainstMelee", value.DefendableAgainstMelee);
    visitor("Priority", value.Priority);
    visitor("Default", value.Default);
    visitor("EnableOnCreate", value.EnableOnCreate);
    visitor("HitLocationCodelet", value.HitLocationCodelet);
}

template <class V>
void visit_fields(CHitLocationsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HitLocations", value.HitLocations);
}

template <class V>
void visit_fields(CIdleSchedulerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Period", value.Period);
}

template <class V>
void visit_fields(CInterestingToVillagersDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Interest", value.Interest);
}

template <class V>
void visit_fields(CInventoryCategoryDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Inventory", value.Inventory);
    visitor("NumberOfSlots", value.NumberOfSlots);
    visitor("DrawItemSlots", value.DrawItemSlots);
    visitor("SelectEmptySlots", value.SelectEmptySlots);
    visitor("WrapHighlightCursor", value.WrapHighlightCursor);
    visitor("CategoryName", value.CategoryName);
    visitor("AllowItemsToFillMoreThanOneSlot", value.AllowItemsToFillMoreThanOneSlot);
    visitor("CategoryIdentifier", value.CategoryIdentifier);
    visitor("AddCategoryOnCreate", value.AddCategoryOnCreate);
}

template <class V>
void visit_fields(CInventoryDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("InitialCategoryMovementRepeatDelay", value.InitialCategoryMovementRepeatDelay);
    visitor("ContinuedCategoryMovementRepeatDelay", value.ContinuedCategoryMovementRepeatDelay);
    visitor("InitialItemMovementRepeatDelay", value.InitialItemMovementRepeatDelay);
    visitor("ContinuedItemMovementRepeatDelay", value.ContinuedItemMovementRepeatDelay);
    visitor("MovementSound", value.MovementSound);
    visitor("NoMovementSound", value.NoMovementSound);
    visitor("InventoryOpenSound", value.InventoryOpenSound);
    visitor("TrackSelectedItems", value.TrackSelectedItems);
    visitor("InitialInventoryClothingItems", value.InitialInventoryClothingItems);
    visitor("InitialInventoryWeaponsItems", value.InitialInventoryWeaponsItems);
    visitor("InitialInventoryStatsItems", value.InitialInventoryStatsItems);
    visitor("InitialInventoryStandardItems", value.InitialInventoryStandardItems);
    visitor("ClothingShadedCircleTLPos", value.ClothingShadedCircleTLPos);
    visitor("ClothingShadedCircleExtents", value.ClothingShadedCircleExtents);
    visitor("CategoriesTLPos", value.CategoriesTLPos);
    visitor("MeshAreaTLPos", value.MeshAreaTLPos);
    visitor("MeshAreaBRPos", value.MeshAreaBRPos);
    visitor("NextCategoryOffset", value.NextCategoryOffset);
    visitor("FirstItemOffsetFromCategory", value.FirstItemOffsetFromCategory);
    visitor("NextItemOffset", value.NextItemOffset);
    visitor("GenericFont", value.GenericFont);
    visitor("BackGroundColour", value.BackGroundColour);
    visitor("ClothingDoll", value.ClothingDoll);
    visitor("DollCentreOffset", value.DollCentreOffset);
    visitor("DollBoundingSphereRadius", value.DollBoundingSphereRadius);
    visitor("TotalExperienceTextPos", value.TotalExperienceTextPos);
    visitor("TotalExperienceText", value.TotalExperienceText);
    visitor("SpendingExperienceTextPos", value.SpendingExperienceTextPos);
    visitor("SpendingExperienceText", value.SpendingExperienceText);
    visitor("StrengthLevelTextPos", value.StrengthLevelTextPos);
    visitor("StrengthLevelColour", value.StrengthLevelColour);
    visitor("SkillLevelTextPos", value.SkillLevelTextPos);
    visitor("SkillLevelColour", value.SkillLevelColour);
    visitor("WillLevelTextPos", value.WillLevelTextPos);
    visitor("WillLevelColour", value.WillLevelColour);
    visitor("RenownLevelTextPos", value.RenownLevelTextPos);
    visitor("HeroLevelTextPos", value.HeroLevelTextPos);
    visitor("StatLevelsFirstLineTextPos", value.StatLevelsFirstLineTextPos);
    visitor("StatLevelsNextLineTextOffset", value.StatLevelsNextLineTextOffset);
    visitor("ButtonAssignmentTextPos", value.ButtonAssignmentTextPos);
    visitor("ButtonAssignmentTextOffset", value.ButtonAssignmentTextOffset);
    visitor("StatLevelsTexts", value.StatLevelsTexts);
    visitor("StatLevelsColours", value.StatLevelsColours);
    visitor("WeaponsTextSeparator", value.WeaponsTextSeparator);
    visitor("WeaponsTextNumberOfSlots", value.WeaponsTextNumberOfSlots);
    visitor("WeaponsTextSlot", value.WeaponsTextSlot);
    visitor("WeaponsTextSuccessfulAugmentation", value.WeaponsTextSuccessfulAugmentation);
    visitor("WeaponsTextNoFreeSlots", value.WeaponsTextNoFreeSlots);
    visitor("WeaponsTextReadyToAugment", value.WeaponsTextReadyToAugment);
    visitor("WeaponsTextNotReadyToAugment", value.WeaponsTextNotReadyToAugment);
    visitor("WeaponsDescriptionBoxTLPos", value.WeaponsDescriptionBoxTLPos);
    visitor("WeaponsDescriptionBoxExtents", value.WeaponsDescriptionBoxExtents);
    visitor("WeaponsActiveMeleeIconPos", value.WeaponsActiveMeleeIconPos);
    visitor("WeaponsActiveRangeIconPos", value.WeaponsActiveRangeIconPos);
    visitor("WeaponAugmentationTextStartPos", value.WeaponAugmentationTextStartPos);
    visitor("WeaponAugmentationTextNewLineOffset", value.WeaponAugmentationTextNewLineOffset);
    visitor("AugmentationStatusTextAreaTLPos", value.AugmentationStatusTextAreaTLPos);
    visitor("AugmentationStatusTextAreaBRPos", value.AugmentationStatusTextAreaBRPos);
    visitor("TimeToDisplayWarningMessageInSeconds", value.TimeToDisplayWarningMessageInSeconds);
    visitor("WeaponsShadedCircleTLPos", value.WeaponsShadedCircleTLPos);
    visitor("WeaponsShadedCircleExtents", value.WeaponsShadedCircleExtents);
    visitor("NumberOfSecondsForObjectRotation", value.NumberOfSecondsForObjectRotation);
    visitor("MaxNumberOfItemsToDisplayPerCategory", value.MaxNumberOfItemsToDisplayPerCategory);
    visitor("MaxNumberOfCategoriesToDisplay", value.MaxNumberOfCategoriesToDisplay);
    visitor("MaxNumberOfScreenNamesToDisplay", value.MaxNumberOfScreenNamesToDisplay);
    visitor("InventoryName", value.InventoryName);
    visitor("InventoryIdentifier", value.InventoryIdentifier);
    visitor("MapScreenMapDisplayBoxTLPos", value.MapScreenMapDisplayBoxTLPos);
    visitor("MapScreenMapDisplayBoxBRPos", value.MapScreenMapDisplayBoxBRPos);
    visitor("MapScreenRegionNamePos", value.MapScreenRegionNamePos);
    visitor("MapScreenWorldMapGraphic", value.MapScreenWorldMapGraphic);
    visitor("MapScreenWorldMapPos", value.MapScreenWorldMapPos);
    visitor("MapScreenRegionUnknownGraphic", value.MapScreenRegionUnknownGraphic);
    visitor("MapScreenRegionUnknownHighlightGraphic", value.MapScreenRegionUnknownHighlightGraphic);
    visitor("MapScreenRegionHeroGraphic", value.MapScreenRegionHeroGraphic);
    visitor("MapScreenRegionQuestGraphic", value.MapScreenRegionQuestGraphic);
    visitor("MapScreenRegionTeleportGraphic", value.MapScreenRegionTeleportGraphic);
    visitor("MapScreenRegionHouseGraphic", value.MapScreenRegionHouseGraphic);
    visitor("StatsTitleTLPos", value.StatsTitleTLPos);
    visitor("StatsShadedCircleTLPos", value.StatsShadedCircleTLPos);
    visitor("StatsShadedCircleExtents", value.StatsShadedCircleExtents);
    visitor("StatsProgBarL", value.StatsProgBarL);
    visitor("StatsProgBarC", value.StatsProgBarC);
    visitor("StatsProgBarR", value.StatsProgBarR);
    visitor("StatsProgBarValue", value.StatsProgBarValue);
    visitor("StatsProgBarValueOffset", value.StatsProgBarValueOffset);
    visitor("StatsPersonalityHeadingTLPos", value.StatsPersonalityHeadingTLPos);
    visitor("StatsPersonalityHeadingBRPos", value.StatsPersonalityHeadingBRPos);
    visitor("StatsPersonalityRenownLowerLabel", value.StatsPersonalityRenownLowerLabel);
    visitor("StatsPersonalityRenownUpperLabel", value.StatsPersonalityRenownUpperLabel);
    visitor("StatsPersonalityGoodnessLowerLabel", value.StatsPersonalityGoodnessLowerLabel);
    visitor("StatsPersonalityGoodnessUpperLabel", value.StatsPersonalityGoodnessUpperLabel);
    visitor("StatsPersonalityAttractivenessLowerLabel", value.StatsPersonalityAttractivenessLowerLabel);
    visitor("StatsPersonalityAttractivenessUpperLabel", value.StatsPersonalityAttractivenessUpperLabel);
    visitor("StatsPersonalityScarinessLowerLabel", value.StatsPersonalityScarinessLowerLabel);
    visitor("StatsPersonalityScarinessUpperLabel", value.StatsPersonalityScarinessUpperLabel);
    visitor("StatsPersonalityNicenessLowerLabel", value.StatsPersonalityNicenessLowerLabel);
    visitor("StatsPersonalityNicenessUpperLabel", value.StatsPersonalityNicenessUpperLabel);
    visitor("StatsPersonalityLabelTextOffset", value.StatsPersonalityLabelTextOffset);
    visitor("StatsPersonalityLowerLabelTLPos", value.StatsPersonalityLowerLabelTLPos);
    visitor("StatsPersonalityLowerLabelWidth", value.StatsPersonalityLowerLabelWidth);
    visitor("StatsPersonalityLowerLabelHeight", value.StatsPersonalityLowerLabelHeight);
    visitor("StatsPersonalitySliderWidth", value.StatsPersonalitySliderWidth);
    visitor("StatsPersonalityUpperLabelWidth", value.StatsPersonalityUpperLabelWidth);
    visitor("StatsPersonalityDoll", value.StatsPersonalityDoll);
    visitor("StatsPersonalityDollCentreOffset", value.StatsPersonalityDollCentreOffset);
    visitor("StatsPersonalityDollBoundingSphereRadius", value.StatsPersonalityDollBoundingSphereRadius);
    visitor("StatsStatisticsLabelTLPos", value.StatsStatisticsLabelTLPos);
    visitor("StatsStatisticsLabelWidth", value.StatsStatisticsLabelWidth);
    visitor("StatsStatisticsLabelHeight", value.StatsStatisticsLabelHeight);
    visitor("StatsStatisticsLabelTextOffset", value.StatsStatisticsLabelTextOffset);
    visitor("StatsStatisticsDataTextOffset", value.StatsStatisticsDataTextOffset);
    visitor("StatsStatisticsHeadingTLPos", value.StatsStatisticsHeadingTLPos);
    visitor("StatsStatisticsHeadingBRPos", value.StatsStatisticsHeadingBRPos);
    visitor("StatsStatisticsPeopleKilledLabel", value.StatsStatisticsPeopleKilledLabel);
    visitor("StatsStatisticsMonstersKilledLabel", value.StatsStatisticsMonstersKilledLabel);
    visitor("StatsStatisticsMaxMultiplierLabel", value.StatsStatisticsMaxMultiplierLabel);
    visitor("StatsStatisticsPreferredFoodLabel", value.StatsStatisticsPreferredFoodLabel);
    visitor("StatsStatisticsDietaryTypeLabel", value.StatsStatisticsDietaryTypeLabel);
    visitor("StatsStatisticsGameTimeElapsedLabel", value.StatsStatisticsGameTimeElapsedLabel);
    visitor("StatsStatisticsNumberOfWivesLabel", value.StatsStatisticsNumberOfWivesLabel);
    visitor("StatsStatisticsQuestsActiveLabel", value.StatsStatisticsQuestsActiveLabel);
    visitor("StatsStatisticsQuestsFinishedLabel", value.StatsStatisticsQuestsFinishedLabel);
    visitor("StatsStatisticsBoastsAchievedLabel", value.StatsStatisticsBoastsAchievedLabel);
    visitor("StatsExperienceStrengthTextOffset", value.StatsExperienceStrengthTextOffset);
    visitor("StatsExperienceWillTextOffset", value.StatsExperienceWillTextOffset);
    visitor("StatsExperienceSkillTextOffset", value.StatsExperienceSkillTextOffset);
    visitor("StatsExperienceStrengthText", value.StatsExperienceStrengthText);
    visitor("StatsExperienceWillText", value.StatsExperienceWillText);
    visitor("StatsExperienceSkillText", value.StatsExperienceSkillText);
    visitor("StatsExperiencePoolGeneralTLPos", value.StatsExperiencePoolGeneralTLPos);
    visitor("StatsExperiencePoolGeneralTextOffset", value.StatsExperiencePoolGeneralTextOffset);
    visitor("StatsExperiencePoolStrengthGroupTLPos", value.StatsExperiencePoolStrengthGroupTLPos);
    visitor("StatsExperiencePoolSkillGroupTLPos", value.StatsExperiencePoolSkillGroupTLPos);
    visitor("StatsExperiencePoolWillGroupTLPos", value.StatsExperiencePoolWillGroupTLPos);
    visitor("StatsExperiencePoolLabelTLPos", value.StatsExperiencePoolLabelTLPos);
    visitor("StatsExperiencePoolLabelTextOffset", value.StatsExperiencePoolLabelTextOffset);
    visitor("StatsExperiencePoolLabelWidth", value.StatsExperiencePoolLabelWidth);
    visitor("StatsExperiencePoolLabelHeight", value.StatsExperiencePoolLabelHeight);
    visitor("StatsExperiencePoolSliderWidth", value.StatsExperiencePoolSliderWidth);
    visitor("StatsExperiencePoolStrengthLabel", value.StatsExperiencePoolStrengthLabel);
    visitor("StatsExperiencePoolSkillLabel", value.StatsExperiencePoolSkillLabel);
    visitor("StatsExperiencePoolWillLabel", value.StatsExperiencePoolWillLabel);
    visitor("StatsExperiencePoolPhysiqueLabel", value.StatsExperiencePoolPhysiqueLabel);
    visitor("StatsExperiencePoolHealthLabel", value.StatsExperiencePoolHealthLabel);
    visitor("StatsExperiencePoolToughnessLabel", value.StatsExperiencePoolToughnessLabel);
    visitor("StatsExperiencePoolSpeedLabel", value.StatsExperiencePoolSpeedLabel);
    visitor("StatsExperiencePoolAccuracyLabel", value.StatsExperiencePoolAccuracyLabel);
    visitor("StatsExperiencePoolStealthLabel", value.StatsExperiencePoolStealthLabel);
    visitor("StatsExperiencePoolWeaponMagicLabel", value.StatsExperiencePoolWeaponMagicLabel);
    visitor("StatsExperiencePoolAbilityMagicLabel", value.StatsExperiencePoolAbilityMagicLabel);
    visitor("StatsExperiencePoolPureMagicLabel", value.StatsExperiencePoolPureMagicLabel);
    visitor("StatsExperienceStrengthBarValue", value.StatsExperienceStrengthBarValue);
    visitor("StatsExperienceSkillBarValue", value.StatsExperienceSkillBarValue);
    visitor("StatsExperienceWillBarValue", value.StatsExperienceWillBarValue);
    visitor("ExperienceGeneralPoolLabelTLPos", value.ExperienceGeneralPoolLabelTLPos);
    visitor("ExperienceGeneralPoolLabelTextOffset", value.ExperienceGeneralPoolLabelTextOffset);
    visitor("ExperienceGeneralPoolWidth", value.ExperienceGeneralPoolWidth);
    visitor("ExperienceGeneralPoolSliderWidth", value.ExperienceGeneralPoolSliderWidth);
    visitor("ExperienceGeneralPoolBarValue", value.ExperienceGeneralPoolBarValue);
    visitor("ExperienceGeneralPoolLabel", value.ExperienceGeneralPoolLabel);
    visitor("ExperienceGeneralPoolSpendTextOffset", value.ExperienceGeneralPoolSpendTextOffset);
    visitor("ExperienceStrengthPoolTLPos", value.ExperienceStrengthPoolTLPos);
    visitor("ExperienceSkillPoolTLPos", value.ExperienceSkillPoolTLPos);
    visitor("ExperienceWillPoolTLPos", value.ExperienceWillPoolTLPos);
    visitor("ExperiencePoolWidth", value.ExperiencePoolWidth);
    visitor("ExperiencePoolHeight", value.ExperiencePoolHeight);
    visitor("ExperienceTypeStrengthLabel", value.ExperienceTypeStrengthLabel);
    visitor("ExperienceTypeSkillLabel", value.ExperienceTypeSkillLabel);
    visitor("ExperienceTypeWillLabel", value.ExperienceTypeWillLabel);
    visitor("ExperienceTypePureMagicLabel", value.ExperienceTypePureMagicLabel);
    visitor("ExperienceTypeAbilityMagicLabel", value.ExperienceTypeAbilityMagicLabel);
    visitor("ExperienceTypeWeaponMagicLabel", value.ExperienceTypeWeaponMagicLabel);
    visitor("ExperienceTypeTLPos", value.ExperienceTypeTLPos);
    visitor("ExperienceTypeTLPosOffset", value.ExperienceTypeTLPosOffset);
    visitor("ExperienceTypeTextOffset", value.ExperienceTypeTextOffset);
    visitor("ExperienceTypeSpendTextOffset", value.ExperienceTypeSpendTextOffset);
    visitor("ExperienceTypeLabelWidth", value.ExperienceTypeLabelWidth);
    visitor("ExperienceTypeLabelHeight", value.ExperienceTypeLabelHeight);
    visitor("ExperienceTypeSliderWidth", value.ExperienceTypeSliderWidth);
    visitor("ExperienceStrengthPhysiqueLabel", value.ExperienceStrengthPhysiqueLabel);
    visitor("ExperienceStrengthHealthLabel", value.ExperienceStrengthHealthLabel);
    visitor("ExperienceStrengthToughnessLabel", value.ExperienceStrengthToughnessLabel);
    visitor("ExperienceSkillSpeedLabel", value.ExperienceSkillSpeedLabel);
    visitor("ExperienceSkillAccuracyLabel", value.ExperienceSkillAccuracyLabel);
    visitor("ExperienceSkillStealthLabel", value.ExperienceSkillStealthLabel);
    visitor("ExperienceWeaponMagicBerserkLabel", value.ExperienceWeaponMagicBerserkLabel);
    visitor("ExperienceWeaponMagicFlamingBladeLabel", value.ExperienceWeaponMagicFlamingBladeLabel);
    visitor("ExperienceWeaponMagicDoubleStrikeLabel", value.ExperienceWeaponMagicDoubleStrikeLabel);
    visitor("ExperienceWeaponMagicLightningBladeLabel", value.ExperienceWeaponMagicLightningBladeLabel);
    visitor("ExperienceAbilityMagicBattleChargeLabel", value.ExperienceAbilityMagicBattleChargeLabel);
    visitor("ExperienceAbilityMagicAssassinRushLabel", value.ExperienceAbilityMagicAssassinRushLabel);
    visitor("ExperienceAbilityMagicTimeSpellLabel", value.ExperienceAbilityMagicTimeSpellLabel);
    visitor("ExperiencePureMagicBurningHandsLabel", value.ExperiencePureMagicBurningHandsLabel);
    visitor("ExperiencePureMagicPhysicalShieldLabel", value.ExperiencePureMagicPhysicalShieldLabel);
    visitor("ExperiencePureMagicForcePushLabel", value.ExperiencePureMagicForcePushLabel);
    visitor("ExperiencePureMagicEnflameSpellLabel", value.ExperiencePureMagicEnflameSpellLabel);
    visitor("ExperiencePureMagicDrainLifeLabel", value.ExperiencePureMagicDrainLifeLabel);
    visitor("ExperiencePureMagicHealLifeLabel", value.ExperiencePureMagicHealLifeLabel);
    visitor("ExperiencePureMagicGhostSwordLabel", value.ExperiencePureMagicGhostSwordLabel);
    visitor("ExperienceStrengthBarValue", value.ExperienceStrengthBarValue);
    visitor("ExperienceSkillBarValue", value.ExperienceSkillBarValue);
    visitor("ExperienceWillBarValue", value.ExperienceWillBarValue);
    visitor("ExperienceStrengthPhysiqueBarValue", value.ExperienceStrengthPhysiqueBarValue);
    visitor("ExperienceStrengthHealthBarValue", value.ExperienceStrengthHealthBarValue);
    visitor("ExperienceStrengthToughnessBarValue", value.ExperienceStrengthToughnessBarValue);
    visitor("ExperienceSkillSpeedBarValue", value.ExperienceSkillSpeedBarValue);
    visitor("ExperienceSkillAccuracyBarValue", value.ExperienceSkillAccuracyBarValue);
    visitor("ExperienceSkillHealthBarValue", value.ExperienceSkillHealthBarValue);
    visitor("ExperienceWeaponMagicBerserkBarValue", value.ExperienceWeaponMagicBerserkBarValue);
    visitor("ExperienceWeaponMagicFlamingBladeBarValue", value.ExperienceWeaponMagicFlamingBladeBarValue);
    visitor("ExperienceWeaponMagicDoubleStrikeBarValue", value.ExperienceWeaponMagicDoubleStrikeBarValue);
    visitor("ExperienceAbilityMagicBattleChargeBarValue", value.ExperienceAbilityMagicBattleChargeBarValue);
    visitor("ExperienceAbilityMagicAssassinRushBarValue", value.ExperienceAbilityMagicAssassinRushBarValue);
    visitor("ExperienceAbilityMagicTimeSpellBarValue", value.ExperienceAbilityMagicTimeSpellBarValue);
    visitor("ExperiencePureMagicBurningHandsBarValue", value.ExperiencePureMagicBurningHandsBarValue);
    visitor("ExperiencePureMagicPhysicalShieldBarValue", value.ExperiencePureMagicPhysicalShieldBarValue);
    visitor("ExperiencePureMagicForcePushBarValue", value.ExperiencePureMagicForcePushBarValue);
    visitor("ExperienceProgBarL", value.ExperienceProgBarL);
    visitor("ExperienceProgBarC", value.ExperienceProgBarC);
    visitor("ExperienceProgBarR", value.ExperienceProgBarR);
    visitor("ExperienceProgBarValue", value.ExperienceProgBarValue);
    visitor("ExperienceProgBarValueOffset", value.ExperienceProgBarValueOffset);
    visitor("ExperienceSpendTimeOffset", value.ExperienceSpendTimeOffset);
    visitor("ExperienceSpendRate", value.ExperienceSpendRate);
    visitor("ExperienceItemLabelStat", value.ExperienceItemLabelStat);
    visitor("ExperienceItemLabelAbility", value.ExperienceItemLabelAbility);
    visitor("ExperienceSpendGreen", value.ExperienceSpendGreen);
    visitor("ExperienceSpendGreenOutline", value.ExperienceSpendGreenOutline);
    visitor("ExperienceSpendGreenEnd", value.ExperienceSpendGreenEnd);
    visitor("ExperienceSpendRed", value.ExperienceSpendRed);
    visitor("ExperienceSpendRedOutline", value.ExperienceSpendRedOutline);
    visitor("ExperienceSpendRedEnd", value.ExperienceSpendRedEnd);
    visitor("ExperienceDescriptionTextTLPos", value.ExperienceDescriptionTextTLPos);
    visitor("ExperienceDescriptionTextOffset", value.ExperienceDescriptionTextOffset);
    visitor("QuestCardInfoQuestNameLabel", value.QuestCardInfoQuestNameLabel);
    visitor("QuestCardInfoMoralityLabel", value.QuestCardInfoMoralityLabel);
    visitor("QuestCardInfoCoreLabel", value.QuestCardInfoCoreLabel);
    visitor("QuestCardInfoDetailsLabel", value.QuestCardInfoDetailsLabel);
    visitor("QuestCardInfoRegionLabel", value.QuestCardInfoRegionLabel);
    visitor("QuestCardInfoOriginatorLabel", value.QuestCardInfoOriginatorLabel);
    visitor("QuestCardInfoDescriptionLabel", value.QuestCardInfoDescriptionLabel);
    visitor("QuestCardInfoSuccessLabel", value.QuestCardInfoSuccessLabel);
    visitor("QuestCardInfoFailureLabel", value.QuestCardInfoFailureLabel);
    visitor("QuestCardInfoRewardsLabel", value.QuestCardInfoRewardsLabel);
    visitor("QuestCardInfoRenownLabel", value.QuestCardInfoRenownLabel);
    visitor("QuestCardInfoTrophyLabel", value.QuestCardInfoTrophyLabel);
    visitor("QuestCardInfoBoastsLabel", value.QuestCardInfoBoastsLabel);
    visitor("QuestCardInfoStatusLabel", value.QuestCardInfoStatusLabel);
    visitor("QuestCardInfoQuestNameText", value.QuestCardInfoQuestNameText);
    visitor("QuestCardInfoMoralityText", value.QuestCardInfoMoralityText);
    visitor("QuestCardInfoCoreText", value.QuestCardInfoCoreText);
    visitor("QuestCardInfoDetailsText", value.QuestCardInfoDetailsText);
    visitor("QuestCardInfoRegionText", value.QuestCardInfoRegionText);
    visitor("QuestCardInfoOriginatorText", value.QuestCardInfoOriginatorText);
    visitor("QuestCardInfoDescriptionText", value.QuestCardInfoDescriptionText);
    visitor("QuestCardInfoSuccessText", value.QuestCardInfoSuccessText);
    visitor("QuestCardInfoFailureText", value.QuestCardInfoFailureText);
    visitor("QuestCardInfoRewardsText", value.QuestCardInfoRewardsText);
    visitor("QuestCardInfoRenownText", value.QuestCardInfoRenownText);
    visitor("QuestCardInfoTrophyText", value.QuestCardInfoTrophyText);
    visitor("QuestCardInfoBoastsText", value.QuestCardInfoBoastsText);
    visitor("QuestCardInfoStatusText", value.QuestCardInfoStatusText);
    visitor("TradeEdgedButtonCostTLPos", value.TradeEdgedButtonCostTLPos);
    visitor("TradeEdgedButtonOwnedTLPos", value.TradeEdgedButtonOwnedTLPos);
    visitor("TradeEdgedButtonProfitTLPos", value.TradeEdgedButtonProfitTLPos);
    visitor("TradeEdgedButtonTitleTLPos", value.TradeEdgedButtonTitleTLPos);
    visitor("TradeEdgedButtonCashTLPos", value.TradeEdgedButtonCashTLPos);
    visitor("TradeEdgedButtonExtents", value.TradeEdgedButtonExtents);
    visitor("TradeAnimImageBoxTLPos", value.TradeAnimImageBoxTLPos);
    visitor("TradeAnimImageBoxExtents", value.TradeAnimImageBoxExtents);
    visitor("TradeAnimImageXRotationTime", value.TradeAnimImageXRotationTime);
    visitor("TradeAnimImageYRotationTime", value.TradeAnimImageYRotationTime);
    visitor("TradeAnimImageZRotationTime", value.TradeAnimImageZRotationTime);
    visitor("TradeEquippedItemTLPos", value.TradeEquippedItemTLPos);
    visitor("TradeEquippedItemExtents", value.TradeEquippedItemExtents);
    visitor("TradeEquippedItemGraphic", value.TradeEquippedItemGraphic);
    visitor("TradeDescriptionBoxTLPos", value.TradeDescriptionBoxTLPos);
    visitor("TradeDescriptionBoxExtents", value.TradeDescriptionBoxExtents);
    visitor("TradeTextOwned", value.TradeTextOwned);
    visitor("TradeTextDivider", value.TradeTextDivider);
    visitor("TradeTextCost", value.TradeTextCost);
    visitor("TradeTextGold", value.TradeTextGold);
    visitor("TradeTextProfit", value.TradeTextProfit);
    visitor("TradeShadedCircleTLPos", value.TradeShadedCircleTLPos);
    visitor("TradeShadedCircleExtents", value.TradeShadedCircleExtents);
    visitor("AugmentationMeshAreaTLPos", value.AugmentationMeshAreaTLPos);
    visitor("AugmentationMeshAreaBRPos", value.AugmentationMeshAreaBRPos);
    visitor("AugmentDescriptionBoxTLPos", value.AugmentDescriptionBoxTLPos);
    visitor("AugmentDescriptionBoxExtents", value.AugmentDescriptionBoxExtents);
    visitor("AugmentAugmentationInfoTLPos", value.AugmentAugmentationInfoTLPos);
    visitor("AugmentAugmentationLabelTLPos", value.AugmentAugmentationLabelTLPos);
    visitor("AugmentAugmentationInfoTextOffset", value.AugmentAugmentationInfoTextOffset);
    visitor("AugmentAugmentationSlotTLPos", value.AugmentAugmentationSlotTLPos);
    visitor("AugmentAugmentationSlotTextOffset", value.AugmentAugmentationSlotTextOffset);
    visitor("AugmentAugmentationWeaponTLPos", value.AugmentAugmentationWeaponTLPos);
    visitor("AugmentAugmentationWeaponTextOffset", value.AugmentAugmentationWeaponTextOffset);
    visitor("AugmentTitleTLPos", value.AugmentTitleTLPos);
    visitor("AugmentTitleTextOffset", value.AugmentTitleTextOffset);
    visitor("AugmentTextMaxSlots", value.AugmentTextMaxSlots);
    visitor("AugmentTextNumSlots", value.AugmentTextNumSlots);
    visitor("AugmentTextUpgradeCost", value.AugmentTextUpgradeCost);
    visitor("AugmentTextGold", value.AugmentTextGold);
    visitor("AugmentTextDamageMultiplier", value.AugmentTextDamageMultiplier);
    visitor("AugmentTextFireStrength", value.AugmentTextFireStrength);
    visitor("AugmentTextSlot", value.AugmentTextSlot);
    visitor("AugmentTextAugmentations", value.AugmentTextAugmentations);
    visitor("AugmentTextWeapons", value.AugmentTextWeapons);
    visitor("AugmentTextDivider", value.AugmentTextDivider);
    visitor("AugmentShadedCircleTLPos", value.AugmentShadedCircleTLPos);
    visitor("AugmentShadedCircleExtents", value.AugmentShadedCircleExtents);
    visitor("InventoryAnimImageXRotationTime", value.InventoryAnimImageXRotationTime);
    visitor("InventoryAnimImageYRotationTime", value.InventoryAnimImageYRotationTime);
    visitor("InventoryAnimImageZRotationTime", value.InventoryAnimImageZRotationTime);
    visitor("QuestCardFeatColour", value.QuestCardFeatColour);
    visitor("QuestCardCoreColour", value.QuestCardCoreColour);
    visitor("QuestCardExclusiveColour", value.QuestCardExclusiveColour);
    visitor("QuestCardInfoTLPos", value.QuestCardInfoTLPos);
    visitor("QuestCardInfo2TLPos", value.QuestCardInfo2TLPos);
    visitor("QuestCardNoQuestsTLPos", value.QuestCardNoQuestsTLPos);
    visitor("QuestCardNoQuestsText", value.QuestCardNoQuestsText);
    visitor("QuestCardInfoTextOffset", value.QuestCardInfoTextOffset);
    visitor("QuestCardEdgedButtonTitleTLPos", value.QuestCardEdgedButtonTitleTLPos);
    visitor("QuestCardEdgedButtonExtents", value.QuestCardEdgedButtonExtents);
    visitor("ExpSpendItemLabelStat", value.ExpSpendItemLabelStat);
    visitor("ExpSpendItemLabelAbility", value.ExpSpendItemLabelAbility);
    visitor("ExpSpendMaxItemsToDisplay", value.ExpSpendMaxItemsToDisplay);
    visitor("ExpSpendButtonTLPos", value.ExpSpendButtonTLPos);
    visitor("ExpSpendButtonTLPosOffset", value.ExpSpendButtonTLPosOffset);
    visitor("ExpSpendButtonTextOffset", value.ExpSpendButtonTextOffset);
    visitor("ExpSpendButtonSelL", value.ExpSpendButtonSelL);
    visitor("ExpSpendButtonSelC", value.ExpSpendButtonSelC);
    visitor("ExpSpendButtonSelR", value.ExpSpendButtonSelR);
    visitor("ExpSpendButtonL", value.ExpSpendButtonL);
    visitor("ExpSpendButtonC", value.ExpSpendButtonC);
    visitor("ExpSpendButtonR", value.ExpSpendButtonR);
    visitor("ExpSpendDescriptionBoxTLPos", value.ExpSpendDescriptionBoxTLPos);
    visitor("ExpSpendDescriptionBoxExtents", value.ExpSpendDescriptionBoxExtents);
    visitor("ExpSpendArrowUpTLPos", value.ExpSpendArrowUpTLPos);
    visitor("ExpSpendArrowDnTLPos", value.ExpSpendArrowDnTLPos);
    visitor("ExpSpendGeneralPoolLabelTLPos", value.ExpSpendGeneralPoolLabelTLPos);
    visitor("ExpSpendGeneralPoolLabelTextOffset", value.ExpSpendGeneralPoolLabelTextOffset);
    visitor("ExpSpendGeneralPoolSpendTextOffset", value.ExpSpendGeneralPoolSpendTextOffset);
    visitor("ExpSpendGeneralPoolWidth", value.ExpSpendGeneralPoolWidth);
    visitor("ExpSpendGeneralPoolSliderWidth", value.ExpSpendGeneralPoolSliderWidth);
    visitor("ExpSpendStrengthPoolTLPos", value.ExpSpendStrengthPoolTLPos);
    visitor("ExpSpendSkillPoolTLPos", value.ExpSpendSkillPoolTLPos);
    visitor("ExpSpendWillPoolTLPos", value.ExpSpendWillPoolTLPos);
    visitor("ExpSpendPoolWidth", value.ExpSpendPoolWidth);
    visitor("ExpSpendPoolHeight", value.ExpSpendPoolHeight);
    visitor("ExpSpendProgBarL", value.ExpSpendProgBarL);
    visitor("ExpSpendProgBarC", value.ExpSpendProgBarC);
    visitor("ExpSpendProgBarR", value.ExpSpendProgBarR);
    visitor("ExpSpendProgBarValue", value.ExpSpendProgBarValue);
    visitor("ExpSpendProgBarValueOffset", value.ExpSpendProgBarValueOffset);
    visitor("ExpSpendTypeTextOffset", value.ExpSpendTypeTextOffset);
    visitor("ExpSpendGeneralPoolBarValue", value.ExpSpendGeneralPoolBarValue);
    visitor("ExpSpendGeneralPoolLabel", value.ExpSpendGeneralPoolLabel);
    visitor("ExpSpendTypeStrengthLabel", value.ExpSpendTypeStrengthLabel);
    visitor("ExpSpendTypeSkillLabel", value.ExpSpendTypeSkillLabel);
    visitor("ExpSpendTypeWillLabel", value.ExpSpendTypeWillLabel);
    visitor("ExpSpendStrengthBarValue", value.ExpSpendStrengthBarValue);
    visitor("ExpSpendSkillBarValue", value.ExpSpendSkillBarValue);
    visitor("ExpSpendWillBarValue", value.ExpSpendWillBarValue);
    visitor("ExpSpendRate", value.ExpSpendRate);
    visitor("ExpSpendTimeOffset", value.ExpSpendTimeOffset);
    visitor("ExpSpendGreen", value.ExpSpendGreen);
    visitor("ExpSpendGreenOutline", value.ExpSpendGreenOutline);
    visitor("ExpSpendGreenEnd", value.ExpSpendGreenEnd);
    visitor("ExpSpendRed", value.ExpSpendRed);
    visitor("ExpSpendRedOutline", value.ExpSpendRedOutline);
    visitor("ExpSpendRedEnd", value.ExpSpendRedEnd);
    visitor("ExpSpendDescriptionTextTLPos", value.ExpSpendDescriptionTextTLPos);
    visitor("ExpSpendDescriptionTextOffset", value.ExpSpendDescriptionTextOffset);
    visitor("MainEdgedButtonTitleTLPos", value.MainEdgedButtonTitleTLPos);
    visitor("MainEdgedButtonOwnedTLPos", value.MainEdgedButtonOwnedTLPos);
    visitor("MainEdgedButtonPriceTLPos", value.MainEdgedButtonPriceTLPos);
    visitor("MainEdgedButtonCostTLPos", value.MainEdgedButtonCostTLPos);
    visitor("MainEdgedButtonExtents", value.MainEdgedButtonExtents);
    visitor("MainDescriptionBoxC", value.MainDescriptionBoxC);
    visitor("MainDescriptionBoxTL", value.MainDescriptionBoxTL);
    visitor("MainDescriptionBoxTR", value.MainDescriptionBoxTR);
    visitor("MainDescriptionBoxBL", value.MainDescriptionBoxBL);
    visitor("MainDescriptionBoxBR", value.MainDescriptionBoxBR);
    visitor("MainDescriptionBoxTLPos", value.MainDescriptionBoxTLPos);
    visitor("MainDescriptionBoxExtents", value.MainDescriptionBoxExtents);
    visitor("MainShadedCircleTLPos", value.MainShadedCircleTLPos);
    visitor("MainShadedCircleExtents", value.MainShadedCircleExtents);
    visitor("MainAnimBoxTLPos", value.MainAnimBoxTLPos);
    visitor("MainAnimBoxExtents", value.MainAnimBoxExtents);
    visitor("MainQuickAccessItemFlashTime", value.MainQuickAccessItemFlashTime);
}

template <class V>
void visit_fields(CInventoryItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Graphic", value.Graphic);
    visitor("ItemDescription", value.ItemDescription);
    visitor("ItemDetails", value.ItemDetails);
    visitor("InventoryCategory", value.InventoryCategory);
    visitor("MaxNumberItems", value.MaxNumberItems);
    visitor("SlotIndex", value.SlotIndex);
    visitor("ActivationTime", value.ActivationTime);
    visitor("UseButtonAction", value.UseButtonAction);
    visitor("InventoryType", value.InventoryType);
    visitor("Orientation", value.Orientation);
    visitor("HeroAbilityDef", value.HeroAbilityDef);
    visitor("IsSellable", value.IsSellable);
    visitor("IsBuyable", value.IsBuyable);
    visitor("IsConfiscatable", value.IsConfiscatable);
    visitor("DoNotPersistUntilQuestCompleted", value.DoNotPersistUntilQuestCompleted);
    visitor("DoNotAutoPickUp", value.DoNotAutoPickUp);
    visitor("AutoPickUpAfterFirstPickUp", value.AutoPickUpAfterFirstPickUp);
    visitor("ItemToSelectUponRemoval", value.ItemToSelectUponRemoval);
    visitor("TutorialCategory", value.TutorialCategory);
    visitor("UIInventoryCategory", value.UIInventoryCategory);
}

template <class V>
void visit_fields(CJackDragonDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinTimeBetweenSlams", value.MinTimeBetweenSlams);
    visitor("MaxHitsBeforeFlyAway", value.MaxHitsBeforeFlyAway);
    visitor("MaxFirebeamDistance", value.MaxFirebeamDistance);
    visitor("MinSnapDistance", value.MinSnapDistance);
    visitor("MaxSnapDistance", value.MaxSnapDistance);
    visitor("MaxTimeAtLongRangeBeforeFlyAway", value.MaxTimeAtLongRangeBeforeFlyAway);
    visitor("MaxSlamDistance", value.MaxSlamDistance);
    visitor("MaxHoverLoops", value.MaxHoverLoops);
    visitor("PercentChanceOfFakeLandingSwoop", value.PercentChanceOfFakeLandingSwoop);
    visitor("MaxTimeOnGround", value.MaxTimeOnGround);
    visitor("DragonGroundHeightOffsetBodge", value.DragonGroundHeightOffsetBodge);
    visitor("HoverBackOffOffset", value.HoverBackOffOffset);
    visitor("FramesBetweenBurninateUpdate", value.FramesBetweenBurninateUpdate);
    visitor("FramesBetweenFireBeamUpdate", value.FramesBetweenFireBeamUpdate);
    visitor("HoverObstructionRadius", value.HoverObstructionRadius);
    visitor("StrafeTriggerDistance", value.StrafeTriggerDistance);
    visitor("GetHitSnapBuildupTime", value.GetHitSnapBuildupTime);
    visitor("NotHitSnapBuildupTime", value.NotHitSnapBuildupTime);
    visitor("MaxTimeBeforeSnap", value.MaxTimeBeforeSnap);
    visitor("MinRoarDistance", value.MinRoarDistance);
}

template <class V>
void visit_fields(CJackOfBladesBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IdealShootDistance", value.IdealShootDistance);
    visitor("NumGetHitsBeforeRush", value.NumGetHitsBeforeRush);
    visitor("TimeBetweenShots", value.TimeBetweenShots);
    visitor("ScreamInAirTime", value.ScreamInAirTime);
    visitor("TimeBetweenPillars", value.TimeBetweenPillars);
    visitor("BattleFloatHeight", value.BattleFloatHeight);
    visitor("BattleScale", value.BattleScale);
    visitor("ScaleUpTime", value.ScaleUpTime);
    visitor("ScaleDownTime", value.ScaleDownTime);
    visitor("DeathScreamTime", value.DeathScreamTime);
    visitor("DeathHeight", value.DeathHeight);
    visitor("BurnDamage", value.BurnDamage);
    visitor("FlashDuration", value.FlashDuration);
    visitor("FlashBuildUpTime", value.FlashBuildUpTime);
    visitor("RushTriggerDistance", value.RushTriggerDistance);
    visitor("RushToDistance", value.RushToDistance);
    visitor("RushAwayDistance", value.RushAwayDistance);
}

template <class V>
void visit_fields(CKickableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BaseDistanceToTravel", value.BaseDistanceToTravel);
    visitor("StrengthScaleRange", value.StrengthScaleRange);
    visitor("SpeedScaleRange", value.SpeedScaleRange);
    visitor("BaseSpeedScaleRange", value.BaseSpeedScaleRange);
    visitor("RandomBase", value.RandomBase);
    visitor("RandomRange", value.RandomRange);
    visitor("ShotToAttach", value.ShotToAttach);
    visitor("OpinionDeedType", value.OpinionDeedType);
    visitor("OpinionDeedFaction", value.OpinionDeedFaction);
    visitor("OpinionDeedTypeToEnemies", value.OpinionDeedTypeToEnemies);
    visitor("IgnoreNavigationTest", value.IgnoreNavigationTest);
}

template <class V>
void visit_fields(CKrakenDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HeightOffset", value.HeightOffset);
    visitor("PoolRadius", value.PoolRadius);
    visitor("HeadRiseDelay", value.HeadRiseDelay);
    visitor("HeadIdleTime", value.HeadIdleTime);
    visitor("LowStrikesBeforeHigh", value.LowStrikesBeforeHigh);
    visitor("LowStrikeChargeLoops", value.LowStrikeChargeLoops);
    visitor("LowStrikeAttackLoops", value.LowStrikeAttackLoops);
    visitor("HighStrikeAttackLoops", value.HighStrikeAttackLoops);
    visitor("RoarLoops", value.RoarLoops);
    visitor("DelayBeforeLowBeam", value.DelayBeforeLowBeam);
    visitor("DelayBeforeHighBeam", value.DelayBeforeHighBeam);
    visitor("BeamExplosionRadius", value.BeamExplosionRadius);
    visitor("BeamTrackSpeed", value.BeamTrackSpeed);
    visitor("TargetingTrackSpeed", value.TargetingTrackSpeed);
    visitor("BeamMouthDistLow", value.BeamMouthDistLow);
    visitor("BeamMouthDistHigh", value.BeamMouthDistHigh);
    visitor("TicksBetweenBeamExplosions", value.TicksBetweenBeamExplosions);
    visitor("TentacleDefIndex", value.TentacleDefIndex);
    visitor("TentacleMaxRiseDelay", value.TentacleMaxRiseDelay);
}

template <class V>
void visit_fields(CKrakenTentacleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TentacleSmackDownTime", value.TentacleSmackDownTime);
    visitor("TentacleTurnSpeed", value.TentacleTurnSpeed);
    visitor("TentacleMoveSpeed", value.TentacleMoveSpeed);
    visitor("TentacleStrikeAngleTolerence", value.TentacleStrikeAngleTolerence);
    visitor("TentacleStrikeDistTolerence", value.TentacleStrikeDistTolerence);
    visitor("TentacleHeightOffset", value.TentacleHeightOffset);
    visitor("TentacleTimeBeforeExplosion", value.TentacleTimeBeforeExplosion);
    visitor("TentacleTimeBetweenAttacks", value.TentacleTimeBetweenAttacks);
    visitor("TentacleTimeBeforeObstructionRemoval", value.TentacleTimeBeforeObstructionRemoval);
    visitor("TentacleObstructionRadius", value.TentacleObstructionRadius);
    visitor("TentacleExplosionBodgeAngle", value.TentacleExplosionBodgeAngle);
    visitor("TentacleExplosionRadius", value.TentacleExplosionRadius);
    visitor("TentacleObstructionRadiusMin", value.TentacleObstructionRadiusMin);
    visitor("TentacleObstructionRadiusMax", value.TentacleObstructionRadiusMax);
}

template <class V>
void visit_fields(CLensFlareElementDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Radius", value.Radius);
    visitor("Colour", value.Colour);
    visitor("Position", value.Position);
    visitor("Texture", value.Texture);
}

template <class V>
void visit_fields(CLightDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Colour", value.Colour);
    visitor("HaloGraphic", value.HaloGraphic);
    visitor("InnerRadius", value.InnerRadius);
    visitor("OuterRadius", value.OuterRadius);
    visitor("Flicker", value.Flicker);
    visitor("FlickerSpeed", value.FlickerSpeed);
    visitor("StartActive", value.StartActive);
    visitor("SunlightAttenuator", value.SunlightAttenuator);
    visitor("DaylightAttenuator", value.DaylightAttenuator);
    visitor("DaylightAttenuatorFadeInStart", value.DaylightAttenuatorFadeInStart);
    visitor("DaylightAttenuatorFadeInEnd", value.DaylightAttenuatorFadeInEnd);
    visitor("DaylightAttenuatorFadeOutStart", value.DaylightAttenuatorFadeOutStart);
    visitor("DaylightAttenuatorFadeOutEnd", value.DaylightAttenuatorFadeOutEnd);
    visitor("EffectChannel", value.EffectChannel);
    visitor("Inverted", value.Inverted);
}

template <class V>
void visit_fields(CLightningDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Range", value.Range);
    visitor("LifeTime", value.LifeTime);
    visitor("MaxNumberOfBranches", value.MaxNumberOfBranches);
    visitor("NeedLOSToBranch", value.NeedLOSToBranch);
    visitor("DamagePeriod", value.DamagePeriod);
    visitor("DamagePerCycle", value.DamagePerCycle);
    visitor("NodeDefIndex", value.NodeDefIndex);
}

template <class V>
void visit_fields(CLightningOrbDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("LightningDefIndex", value.LightningDefIndex);
    visitor("LifeTime", value.LifeTime);
    visitor("Speed", value.Speed);
    visitor("ExplosionRange", value.ExplosionRange);
    visitor("ExplosionDefIndex", value.ExplosionDefIndex);
}

template <class V>
void visit_fields(CLookDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("EyesTurnRangeXY", value.EyesTurnRangeXY);
    visitor("EyesTurnRangeYZ", value.EyesTurnRangeYZ);
    visitor("HeadTurnRangeXY", value.HeadTurnRangeXY);
    visitor("HeadTurnRangeYZUp", value.HeadTurnRangeYZUp);
    visitor("HeadTurnRangeYZDown", value.HeadTurnRangeYZDown);
    visitor("MaxTurnSpeed", value.MaxTurnSpeed);
    visitor("AllowTurningViaAnimation", value.AllowTurningViaAnimation);
    visitor("AnimationTurnSpeedMultiplier", value.AnimationTurnSpeedMultiplier);
    visitor("CombatMaxTurnSpeed", value.CombatMaxTurnSpeed);
    visitor("CombatAllowTurningViaAnimation", value.CombatAllowTurningViaAnimation);
    visitor("AllowOversteer", value.AllowOversteer);
    visitor("AllowUndersteer", value.AllowUndersteer);
    visitor("DrawEyePrimitives", value.DrawEyePrimitives);
    visitor("CutOffNoLooking", value.CutOffNoLooking);
    visitor("AlwaysUpdateHeadPos", value.AlwaysUpdateHeadPos);
}

template <class V>
void visit_fields(CMaterialDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FirstHitSound", value.FirstHitSound);
    visitor("LastHitSound", value.LastHitSound);
    visitor("OnHitRumbleLevel", value.OnHitRumbleLevel);
    visitor("OnHitRumbleFalloff", value.OnHitRumbleFalloff);
    visitor("DeflectAllHits", value.DeflectAllHits);
    visitor("IsDiggable", value.IsDiggable);
    visitor("SlideFootParticleEffect", value.SlideFootParticleEffect);
    visitor("SwordHitParticleEffect", value.SwordHitParticleEffect);
    visitor("KnockdownHitFloorParticleEffect", value.KnockdownHitFloorParticleEffect);
    visitor("WindBlowDustEffect", value.WindBlowDustEffect);
    visitor("BerserkedSwordHitParticleEffect", value.BerserkedSwordHitParticleEffect);
    visitor("BlendedSwordHitEffects", value.BlendedSwordHitEffects);
}

template <class V>
void visit_fields(CMazeBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ShieldUpTime", value.ShieldUpTime);
    visitor("LightningDuration", value.LightningDuration);
    visitor("MinThrowShotRange", value.MinThrowShotRange);
    visitor("LightningRange", value.LightningRange);
    visitor("EnflameRange", value.EnflameRange);
    visitor("TutorialNumGetHits", value.TutorialNumGetHits);
    visitor("MinTimeBetweenHeals", value.MinTimeBetweenHeals);
    visitor("HealSpellLevel", value.HealSpellLevel);
    visitor("TutorialTeleportTime", value.TutorialTeleportTime);
}

template <class V>
void visit_fields(CMeleeCombatAbilityDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FlourishAnimName", value.FlourishAnimName);
    visitor("FlourishResponseAnimName", value.FlourishResponseAnimName);
    visitor("DamageMultiplier", value.DamageMultiplier);
    visitor("Flourish", value.Flourish);
    visitor("BlockCounter", value.BlockCounter);
    visitor("AbilityButtonGraphicOverride", value.AbilityButtonGraphicOverride);
    visitor("ValidWeaponClasses", value.ValidWeaponClasses);
    visitor("Decapitate", value.Decapitate);
    visitor("KnockDownEffects", value.KnockDownEffects);
    visitor("EvadeAllHits", value.EvadeAllHits);
    visitor("CauseRecoil", value.CauseRecoil);
    visitor("StatsNeeded", value.StatsNeeded);
    visitor("ComboMultiplierLowerBound", value.ComboMultiplierLowerBound);
    visitor("ComboMultiplierUpperBound", value.ComboMultiplierUpperBound);
}

template <class V>
void visit_fields(CMeleeCombatKnockdownEffects &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BlendInFrames", value.BlendInFrames);
    visitor("BlendOutFrames", value.BlendOutFrames);
    visitor("AttackerLightingChannel", value.AttackerLightingChannel);
    visitor("TargetLightingChannel", value.TargetLightingChannel);
    visitor("EnvironmentTheme", value.EnvironmentTheme);
    visitor("CameraFOVChange", value.CameraFOVChange);
    visitor("CameraPan", value.CameraPan);
    visitor("AttackerAndTargetSpeedMultiplier", value.AttackerAndTargetSpeedMultiplier);
    visitor("PauseEntities", value.PauseEntities);
    visitor("NonAttackingEntities", value.NonAttackingEntities);
    visitor("AttackerInvulnerable", value.AttackerInvulnerable);
}

template <class V>
void visit_fields(CMessageEventDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MaxSeenRadius", value.MaxSeenRadius);
    visitor("MaxHeardRadius", value.MaxHeardRadius);
    visitor("Lifespan", value.Lifespan);
    visitor("ReactionSpeechType", value.ReactionSpeechType);
    visitor("Type", value.Type);
}

template <class V>
void visit_fields(CMultiStaticMeshEntryDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Mesh", value.Mesh);
    visitor("OverrideLightingChannel", value.OverrideLightingChannel);
    visitor("OverrideRenderFadeDistance", value.OverrideRenderFadeDistance);
    visitor("RenderFadeDistance", value.RenderFadeDistance);
    visitor("LightingChannel", value.LightingChannel);
}

template <class V>
void visit_fields(CMultiStaticMeshDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Meshes", value.Meshes);
}

template <class V>
void visit_fields(CMusicEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ID", value.ID);
    visitor("Filename", value.Filename);
    visitor("Gain", value.Gain);
    visitor("CacheToHD", value.CacheToHD);
}

template <class V>
void visit_fields(CMusicSetEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NormalMusicID", value.NormalMusicID);
    visitor("DangerMusicID", value.DangerMusicID);
    visitor("LoopCount", value.LoopCount);
}

template <class V>
void visit_fields(CNymphDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinAttackRange", value.MinAttackRange);
    visitor("MaxHeight", value.MaxHeight);
    visitor("MinHeight", value.MinHeight);
    visitor("CircleRange", value.CircleRange);
    visitor("FlyAroundTime", value.FlyAroundTime);
    visitor("ShotTrapType", value.ShotTrapType);
    visitor("ShotsBeforeIdle", value.ShotsBeforeIdle);
    visitor("IdleTimeBetweenShots", value.IdleTimeBetweenShots);
    visitor("FadeInTime", value.FadeInTime);
    visitor("FadeOutTime", value.FadeOutTime);
}

template <class V>
void visit_fields(CObjectAugmentationParticleSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PeriodSecs", value.PeriodSecs);
    visitor("ParticleEffect", value.ParticleEffect);
    visitor("ParticleEffectToBlendTo", value.ParticleEffectToBlendTo);
    visitor("AttackParticleEffect", value.AttackParticleEffect);
    visitor("AttackParticleEffectToBlendTo", value.AttackParticleEffectToBlendTo);
    visitor("StartPoint", value.StartPoint);
    visitor("EndPoint", value.EndPoint);
}

template <class V>
void visit_fields(CObjectAugmentationsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumberOfSlots", value.NumberOfSlots);
    visitor("MaximumNumberOfSlots", value.MaximumNumberOfSlots);
    visitor("AugmentationParticles", value.AugmentationParticles);
    visitor("EmptySlotName", value.EmptySlotName);
    visitor("InitialAugmentations", value.InitialAugmentations);
    visitor("SlotUpgradeCosts", value.SlotUpgradeCosts);
    visitor("InitSounds", value.InitSounds);
    visitor("LoopingSounds", value.LoopingSounds);
    visitor("FireProjectileTrailEffect", value.FireProjectileTrailEffect);
    visitor("LightningProjectileTrailEffect", value.LightningProjectileTrailEffect);
    visitor("FireAndLightningProjectileTrailEffect", value.FireAndLightningProjectileTrailEffect);
    visitor("FireProjectileOnHitEffect", value.FireProjectileOnHitEffect);
    visitor("LightningProjectileOnHitEffect", value.LightningProjectileOnHitEffect);
    visitor("FireAndLightningProjectileOnHitEffect", value.FireAndLightningProjectileOnHitEffect);
    visitor("WeaponTrailAttachmentPoint", value.WeaponTrailAttachmentPoint);
}

template <class V>
void visit_fields(CObjectFamilyEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Object", value.Object);
    visitor("Probability", value.Probability);
}

template <class V>
void visit_fields(CObjectFamilyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Objects", value.Objects);
}

template <class V>
void visit_fields(COccupiableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TypeFlags", value.TypeFlags);
}

template <class V>
void visit_fields(COpinionDeedReactionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Attitudes", value.Attitudes);
    visitor("Animation", value.Animation);
    visitor("DialogueTag", value.DialogueTag);
}

template <class V>
void visit_fields(COpinionTransientOffset &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Opinion", value.Opinion);
    visitor("OffsetPerFrameRunIn", value.OffsetPerFrameRunIn);
    visitor("FramesToRunIn", value.FramesToRunIn);
    visitor("FramesOfCappedPeak", value.FramesOfCappedPeak);
    visitor("OffsetPerFrameRunOut", value.OffsetPerFrameRunOut);
    visitor("FramesToRunOut", value.FramesToRunOut);
}

template <class V>
void visit_fields(COpinionDeedEffectsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("KnowledgeLifeInSeconds", value.KnowledgeLifeInSeconds);
    visitor("MinimumSecondsBetweenPostings", value.MinimumSecondsBetweenPostings);
    visitor("MinimumSecondsBetweenEffectsForIndividual", value.MinimumSecondsBetweenEffectsForIndividual);
    visitor("PreReactionDelayIgnoreOnDelay", value.PreReactionDelayIgnoreOnDelay);
    visitor("Effects", value.Effects);
    visitor("ReactionPriority", value.ReactionPriority);
    visitor("Reaction", value.Reaction);
    visitor("ReactionDialogueLayer", value.ReactionDialogueLayer);
    visitor("WarningText", value.WarningText);
    visitor("CrimeSeverity", value.CrimeSeverity);
    visitor("CrimeFineAmount", value.CrimeFineAmount);
    visitor("Tutorial", value.Tutorial);
    visitor("CanBeHeard", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.CanBeHeard); },
        [](COpinionDeedEffectsDef &o, bool v) { o.CanBeHeard = v; }));
    visitor("ReactionRestrictToTarget", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.ReactionRestrictToTarget); },
        [](COpinionDeedEffectsDef &o, bool v) { o.ReactionRestrictToTarget = v; }));
    visitor("AllowSimultaneousReactionDialogue", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.AllowSimultaneousReactionDialogue); },
        [](COpinionDeedEffectsDef &o, bool v) { o.AllowSimultaneousReactionDialogue = v; }));
    visitor("CanApologise", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.CanApologise); },
        [](COpinionDeedEffectsDef &o, bool v) { o.CanApologise = v; }));
    visitor("MustNotRunWhenWarned", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.MustNotRunWhenWarned); },
        [](COpinionDeedEffectsDef &o, bool v) { o.MustNotRunWhenWarned = v; }));
    visitor("OnlyGetFinedForOneInstance", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.OnlyGetFinedForOneInstance); },
        [](COpinionDeedEffectsDef &o, bool v) { o.OnlyGetFinedForOneInstance = v; }));
    visitor("IsFlirting", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.IsFlirting); },
        [](COpinionDeedEffectsDef &o, bool v) { o.IsFlirting = v; }));
    visitor("OnReactSetGreeted", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.OnReactSetGreeted); },
        [](COpinionDeedEffectsDef &o, bool v) { o.OnReactSetGreeted = v; }));
    visitor("AllowSpeechOnNonPureAISpeaker", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.AllowSpeechOnNonPureAISpeaker); },
        [](COpinionDeedEffectsDef &o, bool v) { o.AllowSpeechOnNonPureAISpeaker = v; }));
    visitor("AllowIndirectReactWhileCarrying", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.AllowIndirectReactWhileCarrying); },
        [](COpinionDeedEffectsDef &o, bool v) { o.AllowIndirectReactWhileCarrying = v; }));
    visitor("BlockWhileFollowingPlayer", detail::bitfield(value,
        [](const COpinionDeedEffectsDef &o) { return bool(o.BlockWhileFollowingPlayer); },
        [](COpinionDeedEffectsDef &o, bool v) { o.BlockWhileFollowingPlayer = v; }));
}

template <class V>
void visit_fields(COpinionDeedMaskDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DeedEnabledDefault", value.DeedEnabledDefault);
    visitor("DeedEnabled", value.DeedEnabled);
}

template <class V>
void visit_fields(COpinionOfHeroDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ThreatenGift", value.ThreatenGift);
    visitor("WillAttackCriminals", value.WillAttackCriminals);
    visitor("Personality", value.Personality);
    visitor("ReactionMask", value.ReactionMask);
    visitor("DeedMask", value.DeedMask);
}

template <class V>
void visit_fields(COpinionPersonalityTraits &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Data", value.Data);
}

template <class V>
void visit_fields(COpinionPersonalityDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("PersonalityTraits", value.PersonalityTraits);
    visitor("AttitudeEnabledDefaultForNotNone", value.AttitudeEnabledDefaultForNotNone);
    visitor("AttitudeEnabled", value.AttitudeEnabled);
    visitor("ToleranceToBeingHit", value.ToleranceToBeingHit);
}

template <class V>
void visit_fields(CReactionFrequencyTraits &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("MinWait", value.MinWait);
    visitor("WaitRange", value.WaitRange);
    visitor("LowFreqPerMin", value.LowFreqPerMin);
    visitor("HighLowFreqInterval", value.HighLowFreqInterval);
    visitor("FramesPerMinute", value.FramesPerMinute);
    visitor("RampUpTimeFrames", value.RampUpTimeFrames);
    visitor("CutoffIntervalFrames", value.CutoffIntervalFrames);
    visitor("AllowIndividualRepeats", value.AllowIndividualRepeats);
    visitor("MinGapFrames", value.MinGapFrames);
    visitor("CountRechargePerFrame", value.CountRechargePerFrame);
    visitor("CurrentAvailableCount", value.CurrentAvailableCount);
    visitor("MaxCount", value.MaxCount);
    visitor("InvMaxCount", value.InvMaxCount);
}

template <class V>
void visit_fields(CReactionFrequencyTraitsArray &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Traits", value.Traits);
}

template <class V>
void visit_fields(CReactionMatchEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Tag", value.Tag);
    visitor("ReactionType", value.ReactionType);
    visitor("Axis", value.Axis);
    visitor("XAxisOpinionType", value.XAxisOpinionType);
    visitor("YAxisOpinionType", value.YAxisOpinionType);
    visitor("LowerBoundOnAxis", value.LowerBoundOnAxis);
    visitor("InvIntervalOnAxis", value.InvIntervalOnAxis);
    visitor("CentreOnAxis", value.CentreOnAxis);
    visitor("RadiusOnAxis", value.RadiusOnAxis);
    visitor("XCentre", value.XCentre);
    visitor("NegInvXRadius", value.NegInvXRadius);
    visitor("PosInvXRadius", value.PosInvXRadius);
    visitor("YCentre", value.YCentre);
    visitor("NegInvYRadius", value.NegInvYRadius);
    visitor("PosInvYRadius", value.PosInvYRadius);
    visitor("MShiftZero", value.MShiftZero);
    visitor("MShiftWeight", value.MShiftWeight);
    visitor("RShiftZero", value.RShiftZero);
    visitor("RShiftWeight", value.RShiftWeight);
    visitor("ScarinessShift", value.ScarinessShift);
    visitor("AgreeablenessShift", value.AgreeablenessShift);
    visitor("AttractivenessShift", value.AttractivenessShift);
}

template <class V>
void visit_fields(CReactionMatchList &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Elements", value.Elements);
}

template <class V>
void visit_fields(COpinionReactionManagerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AttitudeBlur", value.AttitudeBlur);
    visitor("AttitudeCondition", value.AttitudeCondition);
    visitor("TargetingCondition", value.TargetingCondition);
    visitor("PreReactionDelay", value.PreReactionDelay);
    visitor("ToleranceToBeingHit", value.ToleranceToBeingHit);
    visitor("BlockFurtherReactions", value.BlockFurtherReactions);
    visitor("AllowSpeechOnNonPureAISpeaker", value.AllowSpeechOnNonPureAISpeaker);
    visitor("AllowWhileCarrying", value.AllowWhileCarrying);
    visitor("AllowWhileFollowingPlayer", value.AllowWhileFollowingPlayer);
    visitor("Matches", value.Matches);
    visitor("FrequencyTraitsArray", value.FrequencyTraitsArray);
    visitor("FrequencyTraits", value.FrequencyTraits);
    visitor("FrequencyTraitsHigh", value.FrequencyTraitsHigh);
}

template <class V>
void visit_fields(COpinionReactionMaskDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ReactionEnabledDefault", value.ReactionEnabledDefault);
    visitor("ReactionEnabled", value.ReactionEnabled);
}

template <class V>
void visit_fields(COpinionSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("OpinionValues", value.OpinionValues);
}

template <class V>
void visit_fields(COpinionSourceDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ProducedOpinion", value.ProducedOpinion);
    visitor("AlwaysAwareOf", value.AlwaysAwareOf);
    visitor("UnknownThreatSource", value.UnknownThreatSource);
    visitor("ReactNonstop", value.ReactNonstop);
    visitor("ReactToSelfPermitted", value.ReactToSelfPermitted);
    visitor("ReactionFlagDefault", value.ReactionFlagDefault);
    visitor("ReactionFlag", value.ReactionFlag);
    visitor("UseNavLineOfSight", value.UseNavLineOfSight);
    visitor("WitnessesWillNotAttack", value.WitnessesWillNotAttack);
    visitor("WitnessesWillOverrideFactionEnemy", value.WitnessesWillOverrideFactionEnemy);
    visitor("BypassesPersonality", value.BypassesPersonality);
    visitor("RelativeEffect", value.RelativeEffect);
    visitor("RestrictToFactionAndAllies", value.RestrictToFactionAndAllies);
    visitor("RestrictToFactionIsReversed", value.RestrictToFactionIsReversed);
    visitor("OpinionProduced", value.OpinionProduced);
    visitor("ReactionFlags", value.ReactionFlags);
}

template <class V>
void visit_fields(COracleMinigameDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("WaitBetweenRoundsSeconds", value.WaitBetweenRoundsSeconds);
    visitor("PauseBetweenOutputSeconds", value.PauseBetweenOutputSeconds);
    visitor("PauseBetweenOutputsLevelMultiplier", value.PauseBetweenOutputsLevelMultiplier);
    visitor("MaxLevel", value.MaxLevel);
    visitor("TintRed", value.TintRed);
    visitor("TintGreen", value.TintGreen);
    visitor("TintBlue", value.TintBlue);
    visitor("TextLose", value.TextLose);
    visitor("TextWin", value.TextWin);
    visitor("TextWatch", value.TextWatch);
    visitor("TextRepeat", value.TextRepeat);
    visitor("TextOfferGame", value.TextOfferGame);
    visitor("TextWonRound", value.TextWonRound);
}

template <class V>
void visit_fields(COverheadDisplayDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DisplayDamage", value.DisplayDamage);
    visitor("DisplayRepair", value.DisplayRepair);
    visitor("DisplayRenown", value.DisplayRenown);
}

template <class V>
void visit_fields(NParticleAttachment::CParticleAttachmentInfo &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ParticleIndex", value.ParticleIndex);
    visitor("AttachmentObjectName", value.AttachmentObjectName);
    visitor("GenerationCutoffDistance", value.GenerationCutoffDistance);
    visitor("DummyObject", value.DummyObject);
}

template <class V>
void visit_fields(CParticleAttacherDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ParticlesToAttach", value.ParticlesToAttach);
}

template <class V>
void visit_fields(CPerceivedThingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FieldOfView", value.FieldOfView);
    visitor("GiveUpChaseRadius", value.GiveUpChaseRadius);
    visitor("SightRadius", value.SightRadius);
    visitor("ExtendedSightRadius", value.ExtendedSightRadius);
    visitor("SoundRadius", value.SoundRadius);
    visitor("SmellRadius", value.SmellRadius);
    visitor("Type", value.Type);
    visitor("AwarenessTimer", value.AwarenessTimer);
    visitor("DayTimeVisibilityMultiplier", value.DayTimeVisibilityMultiplier);
    visitor("NightTimeVisibilityMultiplier", value.NightTimeVisibilityMultiplier);
}

template <class V>
void visit_fields(CPhysicsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Diameter", value.Diameter);
    visitor("Friction", value.Friction);
    visitor("Elasticity", value.Elasticity);
    visitor("AirResistance", value.AirResistance);
    visitor("Mass", value.Mass);
    visitor("InteractionFlags", value.InteractionFlags);
    visitor("PushableByHero", value.PushableByHero);
}

template <class V>
void visit_fields(CPlayerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CharacterDef", value.CharacterDef);
    visitor("Colour", value.Colour);
}

template <class V>
void visit_fields(CQuickAccessItemButtonGuiDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ButtonGraphic", value.ButtonGraphic);
    visitor("Position", value.Position);
    visitor("Offset", value.Offset);
}

template <class V>
void visit_fields(CPlayerGuiDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("InventoryNoMovementSound", value.InventoryNoMovementSound);
    visitor("SecondsToFadeWholeGui", value.SecondsToFadeWholeGui);
    visitor("AlphaValueToFadeWholeGui", value.AlphaValueToFadeWholeGui);
    visitor("AmbientColour", value.AmbientColour);
    visitor("DiffuseColour", value.DiffuseColour);
    visitor("BacklightColour", value.BacklightColour);
    visitor("DiffuseVector", value.DiffuseVector);
    visitor("GuildSealPos", value.GuildSealPos);
    visitor("GuildSealFlashCycles", value.GuildSealFlashCycles);
    visitor("GuildSealFlashCycleSeconds", value.GuildSealFlashCycleSeconds);
    visitor("GuildSealReminderTimeSeconds", value.GuildSealReminderTimeSeconds);
    visitor("HealthBarPos", value.HealthBarPos);
    visitor("HealthBarLeftGraphic", value.HealthBarLeftGraphic);
    visitor("HealthBarMiddleOffset", value.HealthBarMiddleOffset);
    visitor("HealthBarMiddleLength", value.HealthBarMiddleLength);
    visitor("HealthBarMiddleGraphic", value.HealthBarMiddleGraphic);
    visitor("HealthBarRightGraphic", value.HealthBarRightGraphic);
    visitor("HealthBarHealthOffset", value.HealthBarHealthOffset);
    visitor("HealthBarHealthGraphic", value.HealthBarHealthGraphic);
    visitor("HealthBarHealthEvilGraphic", value.HealthBarHealthEvilGraphic);
    visitor("HealthBarHealthGoodGraphic", value.HealthBarHealthGoodGraphic);
    visitor("HealthBarHealthEffectOffset", value.HealthBarHealthEffectOffset);
    visitor("HealthBarHealthIncreaseEffectOffset", value.HealthBarHealthIncreaseEffectOffset);
    visitor("HealthBarHealthDefaultEffect", value.HealthBarHealthDefaultEffect);
    visitor("HealthBarHealthIncreaseEffect", value.HealthBarHealthIncreaseEffect);
    visitor("HealthBarHealthLevelUpEffect", value.HealthBarHealthLevelUpEffect);
    visitor("HealthBarHealthAdrenalineEffect", value.HealthBarHealthAdrenalineEffect);
    visitor("HealthBarHealthBerserkEffect", value.HealthBarHealthBerserkEffect);
    visitor("HealthBarHealthLowHealthEffect", value.HealthBarHealthLowHealthEffect);
    visitor("HealthBarHealthLowPercentage", value.HealthBarHealthLowPercentage);
    visitor("HealthBarHealthInnerOffset", value.HealthBarHealthInnerOffset);
    visitor("HealthBarHealthInnerGraphic", value.HealthBarHealthInnerGraphic);
    visitor("HealthBarHealthEndGraphic", value.HealthBarHealthEndGraphic);
    visitor("HealthBarResurrectionOffset", value.HealthBarResurrectionOffset);
    visitor("HealthBarResurrectionColour", value.HealthBarResurrectionColour);
    visitor("HealthBarStaminaBackOffset", value.HealthBarStaminaBackOffset);
    visitor("HealthBarStaminaOffset", value.HealthBarStaminaOffset);
    visitor("HealthBarStaminaGraphic", value.HealthBarStaminaGraphic);
    visitor("HealthBarStaminaInnerOffset", value.HealthBarStaminaInnerOffset);
    visitor("HealthBarStaminaInnerGraphic", value.HealthBarStaminaInnerGraphic);
    visitor("HealthBarStaminaEndGraphic", value.HealthBarStaminaEndGraphic);
    visitor("HealthBarStaminaEffectOffset", value.HealthBarStaminaEffectOffset);
    visitor("HealthBarStaminaIncreaseEffectOffset", value.HealthBarStaminaIncreaseEffectOffset);
    visitor("HealthBarStaminaIncreaseEffectWidth", value.HealthBarStaminaIncreaseEffectWidth);
    visitor("HealthBarStaminaIncreaseEffectSegmentWidth", value.HealthBarStaminaIncreaseEffectSegmentWidth);
    visitor("HealthBarStaminaDefaultEffect", value.HealthBarStaminaDefaultEffect);
    visitor("HealthBarStaminaIncreaseEffect", value.HealthBarStaminaIncreaseEffect);
    visitor("HealthBarStaminaIncreaseSegmentEffect", value.HealthBarStaminaIncreaseSegmentEffect);
    visitor("StealthEyePos", value.StealthEyePos);
    visitor("StealthEyeFriendGraphic", value.StealthEyeFriendGraphic);
    visitor("StealthEyeNeutralGraphic", value.StealthEyeNeutralGraphic);
    visitor("StealthEyeEnemyGraphic", value.StealthEyeEnemyGraphic);
    visitor("StealthEyeQuestionMarkGraphic", value.StealthEyeQuestionMarkGraphic);
    visitor("StealthEyeGraphicFrames", value.StealthEyeGraphicFrames);
    visitor("StealthEyeTextOffset", value.StealthEyeTextOffset);
    visitor("StealthEyeSecondsToOpen", value.StealthEyeSecondsToOpen);
    visitor("StealthEyeSecondsToThrob", value.StealthEyeSecondsToThrob);
    visitor("StealthEyeThrobScale", value.StealthEyeThrobScale);
    visitor("DPadRingsGraphic", value.DPadRingsGraphic);
    visitor("DPadRingsPos", value.DPadRingsPos);
    visitor("DPadRingsTopGraphic", value.DPadRingsTopGraphic);
    visitor("DPadRingsTopOffset", value.DPadRingsTopOffset);
    visitor("DPadRingsExtendGraphic", value.DPadRingsExtendGraphic);
    visitor("DPadRingsChargeGraphics", value.DPadRingsChargeGraphics);
    visitor("DPadRingsChargeOffsets", value.DPadRingsChargeOffsets);
    visitor("DPadRingsChargeAngles", value.DPadRingsChargeAngles);
    visitor("DPadRingsFavouriteGraphic", value.DPadRingsFavouriteGraphic);
    visitor("DPadRingsTopFavouriteGraphic", value.DPadRingsTopFavouriteGraphic);
    visitor("DPadRingsTopAggressiveGraphic", value.DPadRingsTopAggressiveGraphic);
    visitor("DPadGuildSealLevelUpGraphic", value.DPadGuildSealLevelUpGraphic);
    visitor("DPadGuildSealTooltipOffset", value.DPadGuildSealTooltipOffset);
    visitor("DPadMiddleOffset", value.DPadMiddleOffset);
    visitor("DPadArrowUpGraphic", value.DPadArrowUpGraphic);
    visitor("DPadArrowUpDepGraphic", value.DPadArrowUpDepGraphic);
    visitor("DPadArrowRightGraphic", value.DPadArrowRightGraphic);
    visitor("DPadArrowRightDepGraphic", value.DPadArrowRightDepGraphic);
    visitor("DPadArrowDownGraphic", value.DPadArrowDownGraphic);
    visitor("DPadArrowDownDepGraphic", value.DPadArrowDownDepGraphic);
    visitor("DPadArrowLeftGraphic", value.DPadArrowLeftGraphic);
    visitor("DPadArrowLeftDepGraphic", value.DPadArrowLeftDepGraphic);
    visitor("DPadGuildSealRecallGraphic", value.DPadGuildSealRecallGraphic);
    visitor("GuildSealLevelUpGraphic", value.GuildSealLevelUpGraphic);
    visitor("GuildSealQuestGraphic", value.GuildSealQuestGraphic);
    visitor("GuildSealRecallGraphic", value.GuildSealRecallGraphic);
    visitor("GuildSealLevelUpOffset", value.GuildSealLevelUpOffset);
    visitor("GuildSealMainLevelUpOffset", value.GuildSealMainLevelUpOffset);
    visitor("ABXYRingsGraphic", value.ABXYRingsGraphic);
    visitor("ABXYRingsPos", value.ABXYRingsPos);
    visitor("ABXYRingsAlpha", value.ABXYRingsAlpha);
    visitor("ABXYRingsTopGraphic", value.ABXYRingsTopGraphic);
    visitor("ABXYRingsTopOffset", value.ABXYRingsTopOffset);
    visitor("ABXYMiddleGraphic", value.ABXYMiddleGraphic);
    visitor("ABXYMiddleOffset", value.ABXYMiddleOffset);
    visitor("ABXYMiddleUpGraphic", value.ABXYMiddleUpGraphic);
    visitor("ABXYMiddleRightGraphic", value.ABXYMiddleRightGraphic);
    visitor("ABXYMiddleDownGraphic", value.ABXYMiddleDownGraphic);
    visitor("ABXYMiddleLeftGraphic", value.ABXYMiddleLeftGraphic);
    visitor("ABXYRingsFavouriteGraphic", value.ABXYRingsFavouriteGraphic);
    visitor("ABXYRingsTopFavouriteGraphic", value.ABXYRingsTopFavouriteGraphic);
    visitor("WhiteButtonGraphic", value.WhiteButtonGraphic);
    visitor("WhiteButtonPos", value.WhiteButtonPos);
    visitor("WhiteButtonIconOffset", value.WhiteButtonIconOffset);
    visitor("WhiteButtonTooltipOffset", value.WhiteButtonTooltipOffset);
    visitor("BlackButtonGraphic", value.BlackButtonGraphic);
    visitor("BlackButtonPos", value.BlackButtonPos);
    visitor("BlackButtonIconOffset", value.BlackButtonIconOffset);
    visitor("BlackButtonTooltipOffset", value.BlackButtonTooltipOffset);
    visitor("TextBoxTLGraphic", value.TextBoxTLGraphic);
    visitor("TextBoxTMGraphic", value.TextBoxTMGraphic);
    visitor("TextBoxTRGraphic", value.TextBoxTRGraphic);
    visitor("TextBoxMLGraphic", value.TextBoxMLGraphic);
    visitor("TextBoxMRGraphic", value.TextBoxMRGraphic);
    visitor("TextBoxBLGraphic", value.TextBoxBLGraphic);
    visitor("TextBoxBMGraphic", value.TextBoxBMGraphic);
    visitor("TextBoxBRGraphic", value.TextBoxBRGraphic);
    visitor("TextBoxBackGraphic", value.TextBoxBackGraphic);
    visitor("TextBoxMaxWidth", value.TextBoxMaxWidth);
    visitor("TextBoxMinWidth", value.TextBoxMinWidth);
    visitor("TextBoxMaxHeight", value.TextBoxMaxHeight);
    visitor("TextBoxMinHeight", value.TextBoxMinHeight);
    visitor("TextBoxBorderWidth", value.TextBoxBorderWidth);
    visitor("TextBoxBorderHeight", value.TextBoxBorderHeight);
    visitor("TextBoxCutsceneSecondsBeforeSkip", value.TextBoxCutsceneSecondsBeforeSkip);
    visitor("StatBoxMaxWidth", value.StatBoxMaxWidth);
    visitor("StatBoxMiddleGraphic", value.StatBoxMiddleGraphic);
    visitor("StatBoxLeftOffset", value.StatBoxLeftOffset);
    visitor("StatBoxRightOffset", value.StatBoxRightOffset);
    visitor("StatBoxStartGraphic", value.StatBoxStartGraphic);
    visitor("StatBoxEndGraphic", value.StatBoxEndGraphic);
    visitor("StatBoxGoldGraphic", value.StatBoxGoldGraphic);
    visitor("StatBoxGoldGainEffect", value.StatBoxGoldGainEffect);
    visitor("StatBoxGoldLossEffect", value.StatBoxGoldLossEffect);
    visitor("ClockPos", value.ClockPos);
    visitor("ClockFaceGraphic", value.ClockFaceGraphic);
    visitor("ClockHandGraphic", value.ClockHandGraphic);
    visitor("ClockRingPos", value.ClockRingPos);
    visitor("ClockRingGraphic", value.ClockRingGraphic);
    visitor("ModePos", value.ModePos);
    visitor("ModeOffset", value.ModeOffset);
    visitor("ModeInnerOffset", value.ModeInnerOffset);
    visitor("ModeBorderGraphic", value.ModeBorderGraphic);
    visitor("ModeLockOffGraphic", value.ModeLockOffGraphic);
    visitor("ModeLockOnGraphic", value.ModeLockOnGraphic);
    visitor("ModeSneakOffGraphic", value.ModeSneakOffGraphic);
    visitor("ModeSneakOnGraphic", value.ModeSneakOnGraphic);
    visitor("ModeSafetyOffGraphic", value.ModeSafetyOffGraphic);
    visitor("ModeSafetyOnGraphic", value.ModeSafetyOnGraphic);
    visitor("ModeTextOffset", value.ModeTextOffset);
    visitor("CombatPos", value.CombatPos);
    visitor("CombatLockedOffset", value.CombatLockedOffset);
    visitor("CombatLockedGraphics", value.CombatLockedGraphics);
    visitor("CombatRingOffset", value.CombatRingOffset);
    visitor("CombatRingGraphic", value.CombatRingGraphic);
    visitor("CombatCurrentOffset", value.CombatCurrentOffset);
    visitor("CombatCurrentTextOffset", value.CombatCurrentTextOffset);
    visitor("CombatCountOffsets", value.CombatCountOffsets);
    visitor("CombatCountEmptyGraphic", value.CombatCountEmptyGraphic);
    visitor("CombatCountGraphics", value.CombatCountGraphics);
    visitor("CombatRingGraphics", value.CombatRingGraphics);
    visitor("CombatRingBlackGraphic", value.CombatRingBlackGraphic);
    visitor("CombatRingWhiteGraphic", value.CombatRingWhiteGraphic);
    visitor("CombatCrossGraphic", value.CombatCrossGraphic);
    visitor("CombatCrossOffset", value.CombatCrossOffset);
    visitor("CombatMiddleOffset", value.CombatMiddleOffset);
    visitor("CombatMiddleMinWidth", value.CombatMiddleMinWidth);
    visitor("CombatMiddleGraphic", value.CombatMiddleGraphic);
    visitor("CombatEndGraphic", value.CombatEndGraphic);
    visitor("ExperiencePos", value.ExperiencePos);
    visitor("ExperienceGraphic", value.ExperienceGraphic);
    visitor("ExperienceGeneralGraphic", value.ExperienceGeneralGraphic);
    visitor("ExperienceSkillGraphic", value.ExperienceSkillGraphic);
    visitor("ExperienceStrengthGraphic", value.ExperienceStrengthGraphic);
    visitor("ExperienceWillGraphic", value.ExperienceWillGraphic);
    visitor("ExperienceTextOffset", value.ExperienceTextOffset);
    visitor("ExperienceTextColour", value.ExperienceTextColour);
    visitor("ExperienceMiddleOffset", value.ExperienceMiddleOffset);
    visitor("ExperienceMiddleMinWidth", value.ExperienceMiddleMinWidth);
    visitor("ExperienceMiddleGraphic", value.ExperienceMiddleGraphic);
    visitor("ExperienceEndGraphic", value.ExperienceEndGraphic);
    visitor("CrimePos", value.CrimePos);
    visitor("CrimeTextOffset", value.CrimeTextOffset);
    visitor("CrimeTextColour", value.CrimeTextColour);
    visitor("CrimeTextAlertColour", value.CrimeTextAlertColour);
    visitor("CrimeWarningGraphic", value.CrimeWarningGraphic);
    visitor("CrimeCriminalGraphic", value.CrimeCriminalGraphic);
    visitor("CrimeBarGraphic", value.CrimeBarGraphic);
    visitor("CrimeBarIconOffset", value.CrimeBarIconOffset);
    visitor("CrimeBarBorderOffset", value.CrimeBarBorderOffset);
    visitor("CrosshairOffsets", value.CrosshairOffsets);
    visitor("CrosshairGraphics", value.CrosshairGraphics);
    visitor("CrosshairGreenGraphics", value.CrosshairGreenGraphics);
    visitor("MiniGameFishingLeftGraphic", value.MiniGameFishingLeftGraphic);
    visitor("MiniGameFishingRightGraphic", value.MiniGameFishingRightGraphic);
    visitor("MiniGameBettingGraphic", value.MiniGameBettingGraphic);
    visitor("MiniGameDiggingGraphic", value.MiniGameDiggingGraphic);
    visitor("MiniGamePickpocketGraphic", value.MiniGamePickpocketGraphic);
    visitor("MiniGamePicklockGraphic", value.MiniGamePicklockGraphic);
    visitor("MiniGameStealGraphic", value.MiniGameStealGraphic);
    visitor("MiniGameTrophyGotGraphic", value.MiniGameTrophyGotGraphic);
    visitor("MiniGameTrophyLeftGraphic", value.MiniGameTrophyLeftGraphic);
    visitor("SmackEffect", value.SmackEffect);
    visitor("SmackAlphaFadePerFrame", value.SmackAlphaFadePerFrame);
    visitor("SmackAlphaStartValue", value.SmackAlphaStartValue);
    visitor("SmackAlphaEndValue", value.SmackAlphaEndValue);
    visitor("SmackScaleFactorPerFrame", value.SmackScaleFactorPerFrame);
    visitor("SmackTimeToRotateSeconds", value.SmackTimeToRotateSeconds);
    visitor("MoneyPos", value.MoneyPos);
    visitor("MoneyTextOffset", value.MoneyTextOffset);
    visitor("MoneyTextRightOffset", value.MoneyTextRightOffset);
    visitor("MoneyWidth", value.MoneyWidth);
    visitor("MoneyTextColour", value.MoneyTextColour);
    visitor("MoneyIncreaseTextColour", value.MoneyIncreaseTextColour);
    visitor("MoneyDecreaseTextColour", value.MoneyDecreaseTextColour);
    visitor("MoneyChangeStartOffset", value.MoneyChangeStartOffset);
    visitor("MoneyChangeEndOffset", value.MoneyChangeEndOffset);
    visitor("MoneyChangeTextSpeed", value.MoneyChangeTextSpeed);
    visitor("StatBarsFontName", value.StatBarsFontName);
    visitor("SecondsForStatsBarsToAppear", value.SecondsForStatsBarsToAppear);
    visitor("SecondsBeforeStatsBackBarMoves", value.SecondsBeforeStatsBackBarMoves);
    visitor("SecondsToShowStatsBars", value.SecondsToShowStatsBars);
    visitor("SecondsForStatsBarsToDisappear", value.SecondsForStatsBarsToDisappear);
    visitor("StatUpdatePos", value.StatUpdatePos);
    visitor("StatUpdateMoralityGoodPos", value.StatUpdateMoralityGoodPos);
    visitor("StatUpdateMoralityEvilPos", value.StatUpdateMoralityEvilPos);
    visitor("StatUpdateRenownPos", value.StatUpdateRenownPos);
    visitor("StatUpdateTextOffset", value.StatUpdateTextOffset);
    visitor("StatUpdateTextColour", value.StatUpdateTextColour);
    visitor("StatUpdateDisplayDuration", value.StatUpdateDisplayDuration);
    visitor("StatUpdateWaitingDuration", value.StatUpdateWaitingDuration);
    visitor("StatUpdateEffectOffset", value.StatUpdateEffectOffset);
    visitor("StatUpdateMoralityGoodGraphic", value.StatUpdateMoralityGoodGraphic);
    visitor("StatUpdateMoralityEvilGraphic", value.StatUpdateMoralityEvilGraphic);
    visitor("StatUpdateMoralityGoodEffect", value.StatUpdateMoralityGoodEffect);
    visitor("StatUpdateMoralityEvilEffect", value.StatUpdateMoralityEvilEffect);
    visitor("StatUpdateColours", value.StatUpdateColours);
    visitor("StatUpdateGraphics", value.StatUpdateGraphics);
    visitor("StatUpdateEffects", value.StatUpdateEffects);
    visitor("AutoPickupPos", value.AutoPickupPos);
    visitor("QuestInfoPos", value.QuestInfoPos);
    visitor("QuestInfoBarOffsetX", value.QuestInfoBarOffsetX);
    visitor("QuestInfoGraphicWidth", value.QuestInfoGraphicWidth);
    visitor("QuestInfoGraphicHeight", value.QuestInfoGraphicHeight);
    visitor("QuestInfoGraphicMinHeight", value.QuestInfoGraphicMinHeight);
    visitor("QuestInfoSecondsToScroll", value.QuestInfoSecondsToScroll);
    visitor("QuestInfoSecondsToAppear", value.QuestInfoSecondsToAppear);
    visitor("QuestInfoTurncoatColour", value.QuestInfoTurncoatColour);
    visitor("QuestInfoTickGraphic", value.QuestInfoTickGraphic);
    visitor("OracleSymbolPos", value.OracleSymbolPos);
    visitor("OracleSymbolOffset", value.OracleSymbolOffset);
    visitor("TextBoxTextAreaTLPos", value.TextBoxTextAreaTLPos);
    visitor("TextBoxTextAreaBRPos", value.TextBoxTextAreaBRPos);
    visitor("TextBoxTextColour", value.TextBoxTextColour);
    visitor("TextBoxTextGameInfoColour", value.TextBoxTextGameInfoColour);
    visitor("TextBoxTextGameActionColour", value.TextBoxTextGameActionColour);
    visitor("TextBoxButtonGraphicOffset", value.TextBoxButtonGraphicOffset);
    visitor("TextBoxButtonOffsetY", value.TextBoxButtonOffsetY);
    visitor("TextBoxButtonInBoxOffsetY", value.TextBoxButtonInBoxOffsetY);
    visitor("TextBoxButtonGapWidth", value.TextBoxButtonGapWidth);
    visitor("TextBoxButtonTextFontName", value.TextBoxButtonTextFontName);
    visitor("TextBoxConfirmGraphic", value.TextBoxConfirmGraphic);
    visitor("TextBoxCancelGraphic", value.TextBoxCancelGraphic);
    visitor("TextBoxThirdGraphic", value.TextBoxThirdGraphic);
    visitor("TextBoxSkipCutsceneGraphic", value.TextBoxSkipCutsceneGraphic);
    visitor("TextBoxButtonASoundCriteria", value.TextBoxButtonASoundCriteria);
    visitor("TextBoxButtonBSoundCriteria", value.TextBoxButtonBSoundCriteria);
    visitor("TextBoxHugeButtonGraphic", value.TextBoxHugeButtonGraphic);
    visitor("TextBoxHugeMouseButtonGraphic", value.TextBoxHugeMouseButtonGraphic);
    visitor("TextBoxHugeMouseButtonLeftGraphic", value.TextBoxHugeMouseButtonLeftGraphic);
    visitor("TextBoxHugeButtonOffset", value.TextBoxHugeButtonOffset);
    visitor("TextBoxHugeButtonLeftOffset", value.TextBoxHugeButtonLeftOffset);
    visitor("TextBoxFishingReelTextPos", value.TextBoxFishingReelTextPos);
    visitor("TextBoxFishingReelGraphicOffset", value.TextBoxFishingReelGraphicOffset);
    visitor("TextBoxBettingGraphicPos", value.TextBoxBettingGraphicPos);
    visitor("TextBoxBettingUpOffset", value.TextBoxBettingUpOffset);
    visitor("TextBoxBettingDownOffset", value.TextBoxBettingDownOffset);
    visitor("TextBoxBettingLeftOffset", value.TextBoxBettingLeftOffset);
    visitor("TextBoxBettingRightOffset", value.TextBoxBettingRightOffset);
    visitor("ReceiveItemsSoundCriteria", value.ReceiveItemsSoundCriteria);
    visitor("ControllerAGraphic", value.ControllerAGraphic);
    visitor("ControllerBGraphic", value.ControllerBGraphic);
    visitor("ControllerXGraphic", value.ControllerXGraphic);
    visitor("ControllerYGraphic", value.ControllerYGraphic);
    visitor("ControllerDPadGraphic", value.ControllerDPadGraphic);
    visitor("ControllerDPadDownGraphic", value.ControllerDPadDownGraphic);
    visitor("ControllerDPadLeftGraphic", value.ControllerDPadLeftGraphic);
    visitor("ControllerDPadRightGraphic", value.ControllerDPadRightGraphic);
    visitor("ControllerDPadUpGraphic", value.ControllerDPadUpGraphic);
    visitor("ControllerThumbLeftGraphic", value.ControllerThumbLeftGraphic);
    visitor("ControllerThumbRightGraphic", value.ControllerThumbRightGraphic);
    visitor("ControllerThumbClickLeftGraphic", value.ControllerThumbClickLeftGraphic);
    visitor("ControllerThumbClickRightGraphic", value.ControllerThumbClickRightGraphic);
    visitor("ControllerBackGraphic", value.ControllerBackGraphic);
    visitor("ControllerStartGraphic", value.ControllerStartGraphic);
    visitor("ControllerTriggerLeftGraphic", value.ControllerTriggerLeftGraphic);
    visitor("ControllerTriggerRightGraphic", value.ControllerTriggerRightGraphic);
    visitor("ControllerTriggerBlackGraphic", value.ControllerTriggerBlackGraphic);
    visitor("ControllerTriggerWhiteGraphic", value.ControllerTriggerWhiteGraphic);
    visitor("ControllerGraphicOffset", value.ControllerGraphicOffset);
    visitor("ControllerGraphicTextOffset", value.ControllerGraphicTextOffset);
    visitor("ControllerClickGraphicTextOffset", value.ControllerClickGraphicTextOffset);
    visitor("ControllerTriggerGraphicTextOffset", value.ControllerTriggerGraphicTextOffset);
    visitor("MenuConfirmPos", value.MenuConfirmPos);
    visitor("MenuConfirmTextPos", value.MenuConfirmTextPos);
    visitor("MenuConfirmGraphic", value.MenuConfirmGraphic);
    visitor("MenuCancelPos", value.MenuCancelPos);
    visitor("MenuCancelTextPos", value.MenuCancelTextPos);
    visitor("MenuCancelGraphic", value.MenuCancelGraphic);
    visitor("QuickAccessDPadGraphic", value.QuickAccessDPadGraphic);
    visitor("QuickAccessMenuGraphic", value.QuickAccessMenuGraphic);
    visitor("QuickAccessLampEffect", value.QuickAccessLampEffect);
    visitor("QuickAccessGuildSealEffect", value.QuickAccessGuildSealEffect);
    visitor("QuickAccessMainGuildSealEffect", value.QuickAccessMainGuildSealEffect);
    visitor("QuickAccessEffectOffset", value.QuickAccessEffectOffset);
    visitor("QuickAccessAbilityCrossOffset", value.QuickAccessAbilityCrossOffset);
    visitor("HotBarSingleGraphic", value.HotBarSingleGraphic);
    visitor("HotBarMainGraphic", value.HotBarMainGraphic);
    visitor("HotBarEndGraphics", value.HotBarEndGraphics);
    visitor("HotBarOffset", value.HotBarOffset);
    visitor("HotBarEndOffset", value.HotBarEndOffset);
    visitor("HotBarItemWidth", value.HotBarItemWidth);
    visitor("QuickAccessItemButtons", value.QuickAccessItemButtons);
    visitor("ContextSensitiveItemButtons", value.ContextSensitiveItemButtons);
    visitor("TargetingCursorSectionEnemyEmptyGraphic", value.TargetingCursorSectionEnemyEmptyGraphic);
    visitor("CompassGraphic", value.CompassGraphic);
    visitor("CompassPos", value.CompassPos);
    visitor("CompassShadowGraphic", value.CompassShadowGraphic);
    visitor("CompassShadowOffset", value.CompassShadowOffset);
    visitor("AbilityButtonTalkGraphic", value.AbilityButtonTalkGraphic);
    visitor("AbilityButtonActivateGraphic", value.AbilityButtonActivateGraphic);
    visitor("AbilityButtonBlockGraphic", value.AbilityButtonBlockGraphic);
    visitor("AbilityButtonUnarmedGraphic", value.AbilityButtonUnarmedGraphic);
    visitor("AbilityButtonMeleeGraphic", value.AbilityButtonMeleeGraphic);
    visitor("AbilityButtonRangedGraphic", value.AbilityButtonRangedGraphic);
    visitor("AbilityButtonSheatheAxeGraphic", value.AbilityButtonSheatheAxeGraphic);
    visitor("AbilityButtonSheatheBowGraphic", value.AbilityButtonSheatheBowGraphic);
    visitor("AbilityButtonSheatheMaceGraphic", value.AbilityButtonSheatheMaceGraphic);
    visitor("AbilityButtonSheatheStickGraphic", value.AbilityButtonSheatheStickGraphic);
    visitor("AbilityButtonSheatheSwordGraphic", value.AbilityButtonSheatheSwordGraphic);
    visitor("AbilityButtonFlourishEffectPos", value.AbilityButtonFlourishEffectPos);
    visitor("AbilityButtonFlourishGraphic", value.AbilityButtonFlourishGraphic);
    visitor("AbilityButtonFlourishGraphicOffset", value.AbilityButtonFlourishGraphicOffset);
    visitor("ABXYButtonMiddleGraphic", value.ABXYButtonMiddleGraphic);
    visitor("ABXYAbilityLevelInitialOffset", value.ABXYAbilityLevelInitialOffset);
    visitor("ABXYAbilityLevelOffset", value.ABXYAbilityLevelOffset);
    visitor("ABXYAbilityUnavailableGraphic", value.ABXYAbilityUnavailableGraphic);
    visitor("ABXYAbilityUncastableGraphic", value.ABXYAbilityUncastableGraphic);
    visitor("ABXYAbilitySwitchGraphic", value.ABXYAbilitySwitchGraphic);
    visitor("ABXYAbilityShieldOffGraphic", value.ABXYAbilityShieldOffGraphic);
    visitor("ABXYRunGraphic", value.ABXYRunGraphic);
    visitor("ABXYTargetingEnterGraphic", value.ABXYTargetingEnterGraphic);
    visitor("ABXYTargetingCancelGraphic", value.ABXYTargetingCancelGraphic);
    visitor("ABXYButtonFadeDurationSeconds", value.ABXYButtonFadeDurationSeconds);
    visitor("TargetingBorderLeftGraphic", value.TargetingBorderLeftGraphic);
    visitor("TargetingBorderLeftPos", value.TargetingBorderLeftPos);
    visitor("TargetingBorderRightGraphic", value.TargetingBorderRightGraphic);
    visitor("TargetingBorderRightPos", value.TargetingBorderRightPos);
    visitor("InteractButtonBorderGraphic", value.InteractButtonBorderGraphic);
    visitor("InteractButtonBorderOffset", value.InteractButtonBorderOffset);
    visitor("InteractButtonTooltipOffset", value.InteractButtonTooltipOffset);
    visitor("AbilityButtons", value.AbilityButtons);
    visitor("SpecialAbilityButtons", value.SpecialAbilityButtons);
    visitor("DigitPos", value.DigitPos);
    visitor("DigitHeroPos", value.DigitHeroPos);
    visitor("DigitHeroOffset", value.DigitHeroOffset);
    visitor("DigitGraphics", value.DigitGraphics);
    visitor("RegionDisplayPos", value.RegionDisplayPos);
    visitor("InfoDisplayPos", value.InfoDisplayPos);
    visitor("HeroSprintBarLeftGraphic", value.HeroSprintBarLeftGraphic);
    visitor("HeroSprintBarLeftGlowGraphic", value.HeroSprintBarLeftGlowGraphic);
    visitor("HeroSprintBarMiddleGraphic", value.HeroSprintBarMiddleGraphic);
    visitor("HeroSprintBarMiddleGlowGraphic", value.HeroSprintBarMiddleGlowGraphic);
    visitor("HeroSprintBarRightGraphic", value.HeroSprintBarRightGraphic);
    visitor("HeroSprintBarRightGlowGraphic", value.HeroSprintBarRightGlowGraphic);
    visitor("HeroSprintBarInnerGraphic", value.HeroSprintBarInnerGraphic);
    visitor("HeroSprintBarInnerGraphicOffset", value.HeroSprintBarInnerGraphicOffset);
    visitor("HighlightStartColour", value.HighlightStartColour);
    visitor("HighlightEndColour", value.HighlightEndColour);
    visitor("HighlightStartWidthWorldSpace", value.HighlightStartWidthWorldSpace);
    visitor("HighlightEndWidthWorldSpace", value.HighlightEndWidthWorldSpace);
    visitor("HighlightStartWidthScreenSpace", value.HighlightStartWidthScreenSpace);
    visitor("HighlightEndWidthScreenSpace", value.HighlightEndWidthScreenSpace);
    visitor("HighlightFadeSteps", value.HighlightFadeSteps);
    visitor("NumberOfSecondsForContainerItemToReachFullSize", value.NumberOfSecondsForContainerItemToReachFullSize);
    visitor("ContainerItemDisplayAreaTLPos", value.ContainerItemDisplayAreaTLPos);
    visitor("ContainerItemDisplayAreaBRPos", value.ContainerItemDisplayAreaBRPos);
    visitor("NumberOfSecondsForContainerItemToRotate", value.NumberOfSecondsForContainerItemToRotate);
    visitor("NumberOfSecondsForContainerItemToStayAtFullSize", value.NumberOfSecondsForContainerItemToStayAtFullSize);
    visitor("ContainerItemDisplayBackgroundColour", value.ContainerItemDisplayBackgroundColour);
    visitor("ScreenMessageTextPos", value.ScreenMessageTextPos);
    visitor("ScreenMessageTextColour", value.ScreenMessageTextColour);
    visitor("ScreenMessageTextDropShadowOffset", value.ScreenMessageTextDropShadowOffset);
    visitor("ScreenMessageTextDropShadowColour", value.ScreenMessageTextDropShadowColour);
    visitor("ScreenMessageFont", value.ScreenMessageFont);
    visitor("ScreenMessageNextMessageTextOffset", value.ScreenMessageNextMessageTextOffset);
    visitor("MaximumNumberOfScreenMessages", value.MaximumNumberOfScreenMessages);
    visitor("NumberOfSecondsScreenMessageStaysOnScreen", value.NumberOfSecondsScreenMessageStaysOnScreen);
    visitor("NumberOfCharactersScreenMessageAddsPerFrame", value.NumberOfCharactersScreenMessageAddsPerFrame);
    visitor("TutorialText", value.TutorialText);
    visitor("TutorialLogBookText", value.TutorialLogBookText);
    visitor("TutorialGuildSealGraphics", value.TutorialGuildSealGraphics);
    visitor("ScriptSprites", value.ScriptSprites);
    visitor("GameActionValues", value.GameActionValues);
    visitor("MiniMapGraphics", value.MiniMapGraphics);
    visitor("MiniMapMarkerGraphics", value.MiniMapMarkerGraphics);
    visitor("MiniMapScreenOrigin", value.MiniMapScreenOrigin);
    visitor("MiniMapScreenSize", value.MiniMapScreenSize);
    visitor("MiniMapScreenRadius", value.MiniMapScreenRadius);
    visitor("MiniMapTextureWidth", value.MiniMapTextureWidth);
    visitor("MiniMapMarkerTextureWidth", value.MiniMapMarkerTextureWidth);
    visitor("MiniMapWorldResolution", value.MiniMapWorldResolution);
    visitor("MiniMapPassableColour", value.MiniMapPassableColour);
    visitor("MiniMapImpassableColour", value.MiniMapImpassableColour);
    visitor("MiniMapWaterColour", value.MiniMapWaterColour);
    visitor("MiniMapMaplessColour", value.MiniMapMaplessColour);
    visitor("MiniMapImpassableWaterColour", value.MiniMapImpassableWaterColour);
    visitor("MaxMinimapZoneColours", value.MaxMinimapZoneColours);
    visitor("MaxMinimapThemeColours", value.MaxMinimapThemeColours);
    visitor("MinimapZoneColours", value.MinimapZoneColours);
    visitor("MinimapThemeColours", value.MinimapThemeColours);
    visitor("RegionChangeTLOffset", value.RegionChangeTLOffset);
    visitor("RegionChangeTMOffset", value.RegionChangeTMOffset);
    visitor("RegionChangeTROffset", value.RegionChangeTROffset);
    visitor("RegionChangeBLOffset", value.RegionChangeBLOffset);
    visitor("RegionChangeBMOffset", value.RegionChangeBMOffset);
    visitor("RegionChangeBROffset", value.RegionChangeBROffset);
    visitor("RegionChangeTLGraphic", value.RegionChangeTLGraphic);
    visitor("RegionChangeTMGraphic", value.RegionChangeTMGraphic);
    visitor("RegionChangeTRGraphic", value.RegionChangeTRGraphic);
    visitor("RegionChangeBLGraphic", value.RegionChangeBLGraphic);
    visitor("RegionChangeBMGraphic", value.RegionChangeBMGraphic);
    visitor("RegionChangeBRGraphic", value.RegionChangeBRGraphic);
    visitor("RegionChangeBorderLeftOffset", value.RegionChangeBorderLeftOffset);
    visitor("RegionChangeBorderRightOffset", value.RegionChangeBorderRightOffset);
    visitor("RegionChangeBorderLeftGraphic", value.RegionChangeBorderLeftGraphic);
    visitor("RegionChangeBorderRightGraphic", value.RegionChangeBorderRightGraphic);
    visitor("RegionChangeGuildSealBorderGraphic", value.RegionChangeGuildSealBorderGraphic);
    visitor("RegionChangeGuildSealBorderOffset", value.RegionChangeGuildSealBorderOffset);
    visitor("RegionChangeGuildSealGraphics", value.RegionChangeGuildSealGraphics);
    visitor("LoadProgressBarLeftGraphic", value.LoadProgressBarLeftGraphic);
    visitor("LoadProgressBarMidGraphic", value.LoadProgressBarMidGraphic);
    visitor("LoadProgressBarRightGraphic", value.LoadProgressBarRightGraphic);
    visitor("LoadProgressBarInnerGraphic", value.LoadProgressBarInnerGraphic);
    visitor("LoadProgressBackdropGraphics", value.LoadProgressBackdropGraphics);
    visitor("TavernGameBlockTLOffset", value.TavernGameBlockTLOffset);
    visitor("TavernGameBlockTMOffset", value.TavernGameBlockTMOffset);
    visitor("TavernGameBlockTROffset", value.TavernGameBlockTROffset);
    visitor("TavernGameBlockBLOffset", value.TavernGameBlockBLOffset);
    visitor("TavernGameBlockBMOffset", value.TavernGameBlockBMOffset);
    visitor("TavernGameBlockBROffset", value.TavernGameBlockBROffset);
    visitor("TavernGameBlockTLGraphic", value.TavernGameBlockTLGraphic);
    visitor("TavernGameBlockTMGraphic", value.TavernGameBlockTMGraphic);
    visitor("TavernGameBlockTRGraphic", value.TavernGameBlockTRGraphic);
    visitor("TavernGameBlockBLGraphic", value.TavernGameBlockBLGraphic);
    visitor("TavernGameBlockBMGraphic", value.TavernGameBlockBMGraphic);
    visitor("TavernGameBlockBRGraphic", value.TavernGameBlockBRGraphic);
    visitor("RegionGraphFileName", value.RegionGraphFileName);
    visitor("MouseCursorScaleX", value.MouseCursorScaleX);
    visitor("MouseCursorScaleY", value.MouseCursorScaleY);
    visitor("MouseCursorGraphic", value.MouseCursorGraphic);
    visitor("MouseLMBGraphic", value.MouseLMBGraphic);
    visitor("MouseMMBGraphic", value.MouseMMBGraphic);
    visitor("MouseRMBGraphic", value.MouseRMBGraphic);
    visitor("MouseOverlayGraphic", value.MouseOverlayGraphic);
}

template <class V>
void visit_fields(CPlayerInventoryDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Slot", value.Slot);
    visitor("MaxInSlot", value.MaxInSlot);
    visitor("InitialAmount", value.InitialAmount);
    visitor("IsUsable", value.IsUsable);
    visitor("InitialSelectedSlot", value.InitialSelectedSlot);
}

template <class V>
void visit_fields(CPlayerMovementDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("JoystickMovementStationaryBand", value.JoystickMovementStationaryBand);
    visitor("JoystickMovementSlowBand", value.JoystickMovementSlowBand);
    visitor("JoystickMovementWalkingBand", value.JoystickMovementWalkingBand);
    visitor("JoystickMovementJoggingBand", value.JoystickMovementJoggingBand);
    visitor("MinRequiredAngleFor180", value.MinRequiredAngleFor180);
    visitor("SecondsToStoreLastMovementFor", value.SecondsToStoreLastMovementFor);
    visitor("NoMovementFramesToCheckFor180", value.NoMovementFramesToCheckFor180);
    visitor("NoMovementFramesToCheckFor180PriorStraightLine", value.NoMovementFramesToCheckFor180PriorStraightLine);
    visitor("MinNoRunSecondsForSkidHaltMove", value.MinNoRunSecondsForSkidHaltMove);
    visitor("NoRunningFramesToPreventSkidHalt", value.NoRunningFramesToPreventSkidHalt);
    visitor("MaxAllowedAngleChangeForSkidHalt", value.MaxAllowedAngleChangeForSkidHalt);
    visitor("MaxTurnSpeed", value.MaxTurnSpeed);
    visitor("MaxTurnSpeedHeavyWeapon", value.MaxTurnSpeedHeavyWeapon);
    visitor("LeanAngleTurnFactor", value.LeanAngleTurnFactor);
    visitor("LeanAngleLimitAngle", value.LeanAngleLimitAngle);
    visitor("LeanAngleStepPerFrame", value.LeanAngleStepPerFrame);
}

template <class V>
void visit_fields(CPrizeScoreDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Score", value.Score);
    visitor("Mult", value.Mult);
}

template <class V>
void visit_fields(CQuestCardDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("QuestName", value.QuestName);
    visitor("QuestSummary", value.QuestSummary);
    visitor("QuestObjective", value.QuestObjective);
    visitor("SuccessSummary", value.SuccessSummary);
    visitor("RegionName", value.RegionName);
    visitor("TeleporterRegionName", value.TeleporterRegionName);
    visitor("InventoryCategory", value.InventoryCategory);
    visitor("RenownReward", value.RenownReward);
    visitor("GoldReward", value.GoldReward);
    visitor("RewardObjects", value.RewardObjects);
    visitor("IsCore", value.IsCore);
    visitor("IsVignette", value.IsVignette);
    visitor("IsExclusive", value.IsExclusive);
    visitor("MakeVignetteRouteAppearOnMinimap", value.MakeVignetteRouteAppearOnMinimap);
    visitor("Prerequisites", value.Prerequisites);
    visitor("NumBoasts", value.NumBoasts);
    visitor("CanPlayerCancel", value.CanPlayerCancel);
    visitor("QuestEpilogue", value.QuestEpilogue);
}

template <class V>
void visit_fields(CRGBFloatColour &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("R", value.R);
    visitor("G", value.G);
    visitor("B", value.B);
    visitor("A", value.A);
}

template <class V>
void visit_fields(CRandomAppearanceMorph &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumBodyParts", value.NumBodyParts);
    visitor("BodyParts0", value.BodyParts0);
    visitor("BodyParts1", value.BodyParts1);
    visitor("BodyParts2", value.BodyParts2);
    visitor("BodyPartTextureMorphs0", value.BodyPartTextureMorphs0);
    visitor("BodyPartTextureMorphs1", value.BodyPartTextureMorphs1);
    visitor("BodyPartTextureMorphs2", value.BodyPartTextureMorphs2);
    visitor("SkeletalMorphs", value.SkeletalMorphs);
    visitor("GeneralTextureMorphs", value.GeneralTextureMorphs);
    visitor("FinalTrailing", value.FinalTrailing);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CTextureMorphEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Morph", value.Morph);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CTextureMorphTable &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Entries", value.Entries);
    visitor("Trailing", value.Trailing);
    visitor("Dirty", value.Dirty);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart0Mesh &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MeshId", value.MeshId);
    visitor("TextureId", value.TextureId);
    visitor("TextureMorphs", value.TextureMorphs);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart1Mesh &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MeshId", value.MeshId);
    visitor("TextureId", value.TextureId);
    visitor("TextureMorphs", value.TextureMorphs);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart2Mesh &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MeshId", value.MeshId);
    visitor("TextureId", value.TextureId);
    visitor("TextureMorphs", value.TextureMorphs);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart0 &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Meshes", value.Meshes);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart1 &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Meshes", value.Meshes);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CBodyPart2 &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Meshes", value.Meshes);
}

template <class V>
void visit_fields(CRandomAppearanceMorph::CSkeletalMorphEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Key", value.Key);
    visitor("MorphId", value.MorphId);
    visitor("BoneOrMorphIndex", value.BoneOrMorphIndex);
    visitor("AllowVariation", value.AllowVariation);
}

template <class V>
void visit_fields(CReadableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("UseInfoDisplay", value.UseInfoDisplay);
}

template <class V>
void visit_fields(CRegionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("EnvironmentTheme", value.EnvironmentTheme);
    visitor("InternalEnvironmentTheme", value.InternalEnvironmentTheme);
    visitor("InternalLightingChannelCount", value.InternalLightingChannelCount);
    visitor("CameraManagerSetIndex", value.CameraManagerSetIndex);
    visitor("CameraManagerSetBuildingIndex", value.CameraManagerSetBuildingIndex);
    visitor("SoundMap", value.SoundMap);
    visitor("PreventHeroWeaponUse", value.PreventHeroWeaponUse);
    visitor("PreventHeroWillUse", value.PreventHeroWillUse);
    visitor("WanderingPopulationScriptDefName", value.WanderingPopulationScriptDefName);
    visitor("Music", value.Music);
    visitor("ReverbEnvironmentType", value.ReverbEnvironmentType);
    visitor("ReverbEnvironmentLevel", value.ReverbEnvironmentLevel);
    visitor("InBuildingOcclusionLevel", value.InBuildingOcclusionLevel);
    visitor("UseVillageTime", value.UseVillageTime);
    visitor("AllowHenchmen", value.AllowHenchmen);
    visitor("AttachToCameraFX", value.AttachToCameraFX);
    visitor("EnableSpotlightShadowMode", value.EnableSpotlightShadowMode);
    visitor("CheckAllThingsOnMapForCombatCollision", value.CheckAllThingsOnMapForCombatCollision);
    visitor("HeroBreathParticle", value.HeroBreathParticle);
    visitor("MaxNumAttackersOnHero", value.MaxNumAttackersOnHero);
    visitor("WeaponOutCrime", value.WeaponOutCrime);
    visitor("UseCrimes", value.UseCrimes);
    visitor("DisplayCrimes", value.DisplayCrimes);
    visitor("PermitCrimes", value.PermitCrimes);
    visitor("RegionDescription", value.RegionDescription);
    visitor("HasSea", value.HasSea);
    visitor("LockToFrameRate", value.LockToFrameRate);
    visitor("WorldMapOffset", value.WorldMapOffset);
    visitor("NameGraphicOffset", value.NameGraphicOffset);
    visitor("NameGraphic", value.NameGraphic);
    visitor("FishWeightMult", value.FishWeightMult);
}

template <class V>
void visit_fields(CRegionScriptDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("RandomVillagerMax", value.RandomVillagerMax);
    visitor("RandomGuardMax", value.RandomGuardMax);
    visitor("RandomBanditMax", value.RandomBanditMax);
    visitor("RegionDangerLevel", value.RegionDangerLevel);
}

template <class V>
void visit_fields(NGraphicAppearance::CReplaceableMeshEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BankIndex", value.BankIndex);
    visitor("AnimStep", value.AnimStep);
    visitor("RenderSizeX", value.RenderSizeX);
    visitor("AdditiveAlpha", value.AdditiveAlpha);
    visitor("GraphicType", value.GraphicType);
}

template <class V>
void visit_fields(NGraphicAppearance::CReplaceableMeshes &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Vector", value.Vector);
}

template <class V>
void visit_fields(CReplaceableMeshDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Meshes", value.Meshes);
}

template <class V>
void visit_fields(CResurrectionItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("OnUseParticleEffect", value.OnUseParticleEffect);
}

template <class V>
void visit_fields(CReverbEnvironmentEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Params", value.Params);
}

template <class V>
void visit_fields(CRumbleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("QuakeIntensities", value.QuakeIntensities);
    visitor("QuakeDurations", value.QuakeDurations);
}

template <class V>
void visit_fields(CScorpionKingBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumChargesBeforeTailStrike", value.NumChargesBeforeTailStrike);
    visitor("UnderGroundTime", value.UnderGroundTime);
    visitor("TailPlungeTime", value.TailPlungeTime);
    visitor("SpikeTime", value.SpikeTime);
    visitor("SpikeDecisionTime", value.SpikeDecisionTime);
    visitor("TailStrikePrepareTime", value.TailStrikePrepareTime);
    visitor("DamageToCauseTailStrike", value.DamageToCauseTailStrike);
    visitor("ChargeDamage", value.ChargeDamage);
    visitor("ClawObstructionRadius", value.ClawObstructionRadius);
}

template <class V>
void visit_fields(CScriptDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("EXPERIENCE_GRANT_SMALL", value.EXPERIENCE_GRANT_SMALL);
    visitor("EXPERIENCE_GRANT_MEDIUM", value.EXPERIENCE_GRANT_MEDIUM);
    visitor("EXPERIENCE_GRANT_LARGE", value.EXPERIENCE_GRANT_LARGE);
    visitor("EXPERIENCE_GRANT_XLARGE", value.EXPERIENCE_GRANT_XLARGE);
    visitor("EXPERIENCE_GRANT_HEROIC", value.EXPERIENCE_GRANT_HEROIC);
    visitor("NPC_COMMENT_FREQUENCY", value.NPC_COMMENT_FREQUENCY);
    visitor("NPC_COMMENT_DISTANCE", value.NPC_COMMENT_DISTANCE);
    visitor("AmbushTradersNakedBoastCost", value.AmbushTradersNakedBoastCost);
    visitor("AmbushTradersNakedBoastReward", value.AmbushTradersNakedBoastReward);
    visitor("AmbushTradersNoDamageBoastCost", value.AmbushTradersNoDamageBoastCost);
    visitor("AmbushTradersNoDamageBoastReward", value.AmbushTradersNoDamageBoastReward);
    visitor("AmbushTradersNoWeaponsBoastCost", value.AmbushTradersNoWeaponsBoastCost);
    visitor("AmbushTradersNoWeaponsBoastReward", value.AmbushTradersNoWeaponsBoastReward);
    visitor("ArenaNakedBoastCost", value.ArenaNakedBoastCost);
    visitor("ArenaNakedBoastReward", value.ArenaNakedBoastReward);
    visitor("ArenaNoDamageBoastCost", value.ArenaNoDamageBoastCost);
    visitor("ArenaNoDamageBoastReward", value.ArenaNoDamageBoastReward);
    visitor("ArenaNoWeaponsBoastCost", value.ArenaNoWeaponsBoastCost);
    visitor("ArenaNoWeaponsBoastReward", value.ArenaNoWeaponsBoastReward);
    visitor("BanditCampNoDamageBoastCost", value.BanditCampNoDamageBoastCost);
    visitor("BanditCampNoDamageBoastReward", value.BanditCampNoDamageBoastReward);
    visitor("BanditCampNoWeaponsBoastCost", value.BanditCampNoWeaponsBoastCost);
    visitor("BanditCampNoWeaponsBoastReward", value.BanditCampNoWeaponsBoastReward);
    visitor("BountyHuntNakedBoastCost", value.BountyHuntNakedBoastCost);
    visitor("BountyHuntNakedBoastReward", value.BountyHuntNakedBoastReward);
    visitor("BountyHuntNoDamageBoastCost", value.BountyHuntNoDamageBoastCost);
    visitor("BountyHuntNoDamageBoastReward", value.BountyHuntNoDamageBoastReward);
    visitor("BreakSiegeNakedBoastCost", value.BreakSiegeNakedBoastCost);
    visitor("BreakSiegeNakedBoastReward", value.BreakSiegeNakedBoastReward);
    visitor("BreakSiegeNoDamageBoastCost", value.BreakSiegeNoDamageBoastCost);
    visitor("BreakSiegeNoDamageBoastReward", value.BreakSiegeNoDamageBoastReward);
    visitor("BreakSiegeNoWeaponsBoastCost", value.BreakSiegeNoWeaponsBoastCost);
    visitor("BreakSiegeNoWeaponsBoastReward", value.BreakSiegeNoWeaponsBoastReward);
    visitor("GraveyardNakedBoastCost", value.GraveyardNakedBoastCost);
    visitor("GraveyardNakedBoastReward", value.GraveyardNakedBoastReward);
    visitor("GraveyardNoDamageBoastCost", value.GraveyardNoDamageBoastCost);
    visitor("GraveyardNoDamageBoastReward", value.GraveyardNoDamageBoastReward);
    visitor("GraveyardNoWeaponsBoastCost", value.GraveyardNoWeaponsBoastCost);
    visitor("GraveyardNoWeaponsBoastReward", value.GraveyardNoWeaponsBoastReward);
    visitor("HobbeCaveNakedBoastCost", value.HobbeCaveNakedBoastCost);
    visitor("HobbeCaveNakedBoastReward", value.HobbeCaveNakedBoastReward);
    visitor("HobbeCaveNoDamageBoastCost", value.HobbeCaveNoDamageBoastCost);
    visitor("HobbeCaveNoDamageBoastReward", value.HobbeCaveNoDamageBoastReward);
    visitor("HobbeCaveNoWeaponsBoastCost", value.HobbeCaveNoWeaponsBoastCost);
    visitor("HobbeCaveNoWeaponsBoastReward", value.HobbeCaveNoWeaponsBoastReward);
    visitor("HTContestNakedBoastCost", value.HTContestNakedBoastCost);
    visitor("HTContestNakedBoastReward", value.HTContestNakedBoastReward);
    visitor("HTContestNoDamageBoastCost", value.HTContestNoDamageBoastCost);
    visitor("HTContestNoDamageBoastReward", value.HTContestNoDamageBoastReward);
    visitor("HTContestNoWeaponsBoastCost", value.HTContestNoWeaponsBoastCost);
    visitor("HTContestNoWeaponsBoastReward", value.HTContestNoWeaponsBoastReward);
    visitor("HTEvilNakedBoastCost", value.HTEvilNakedBoastCost);
    visitor("HTEvilNakedBoastReward", value.HTEvilNakedBoastReward);
    visitor("HTEvilNoDamageBoastCost", value.HTEvilNoDamageBoastCost);
    visitor("HTEvilNoDamageBoastReward", value.HTEvilNoDamageBoastReward);
    visitor("HTEvilNoWeaponsBoastCost", value.HTEvilNoWeaponsBoastCost);
    visitor("HTEvilNoWeaponsBoastReward", value.HTEvilNoWeaponsBoastReward);
    visitor("HTGoodNakedBoastCost", value.HTGoodNakedBoastCost);
    visitor("HTGoodNakedBoastReward", value.HTGoodNakedBoastReward);
    visitor("HTGoodNoDamageBoastCost", value.HTGoodNoDamageBoastCost);
    visitor("HTGoodNoDamageBoastReward", value.HTGoodNoDamageBoastReward);
    visitor("HTGoodNoWeaponsBoastCost", value.HTGoodNoWeaponsBoastCost);
    visitor("HTGoodNoWeaponsBoastReward", value.HTGoodNoWeaponsBoastReward);
    visitor("MCCNakedBoastCost", value.MCCNakedBoastCost);
    visitor("MCCNakedBoastReward", value.MCCNakedBoastReward);
    visitor("MCCNoDamageBoastCost", value.MCCNoDamageBoastCost);
    visitor("MCCNoDamageBoastReward", value.MCCNoDamageBoastReward);
    visitor("MCCNoWeaponsBoastCost", value.MCCNoWeaponsBoastCost);
    visitor("MCCNoWeaponsBoastReward", value.MCCNoWeaponsBoastReward);
    visitor("MinionCampNakedBoastCost", value.MinionCampNakedBoastCost);
    visitor("MinionCampNakedBoastReward", value.MinionCampNakedBoastReward);
    visitor("MinionCampNoDamageBoastCost", value.MinionCampNoDamageBoastCost);
    visitor("MinionCampNoDamageBoastReward", value.MinionCampNoDamageBoastReward);
    visitor("MinionCampNoWeaponsBoastCost", value.MinionCampNoWeaponsBoastCost);
    visitor("MinionCampNoWeaponsBoastReward", value.MinionCampNoWeaponsBoastReward);
    visitor("OFEvilNakedBoastCost", value.OFEvilNakedBoastCost);
    visitor("OFEvilNakedBoastReward", value.OFEvilNakedBoastReward);
    visitor("OFEvilNoDamageBoastCost", value.OFEvilNoDamageBoastCost);
    visitor("OFEvilNoDamageBoastReward", value.OFEvilNoDamageBoastReward);
    visitor("OFEvilNoWeaponsBoastCost", value.OFEvilNoWeaponsBoastCost);
    visitor("OFEvilNoWeaponsBoastReward", value.OFEvilNoWeaponsBoastReward);
    visitor("OFGoodNakedBoastCost", value.OFGoodNakedBoastCost);
    visitor("OFGoodNakedBoastReward", value.OFGoodNakedBoastReward);
    visitor("OFGoodNoDamageBoastCost", value.OFGoodNoDamageBoastCost);
    visitor("OFGoodNoDamageBoastReward", value.OFGoodNoDamageBoastReward);
    visitor("OFGoodNoWeaponsBoastCost", value.OFGoodNoWeaponsBoastCost);
    visitor("OFGoodNoWeaponsBoastReward", value.OFGoodNoWeaponsBoastReward);
    visitor("PrisonNakedBoastCost", value.PrisonNakedBoastCost);
    visitor("PrisonNakedBoastReward", value.PrisonNakedBoastReward);
    visitor("PrisonNoDamageBoastCost", value.PrisonNoDamageBoastCost);
    visitor("PrisonNoDamageBoastReward", value.PrisonNoDamageBoastReward);
    visitor("PrisonNoWeaponsBoastCost", value.PrisonNoWeaponsBoastCost);
    visitor("PrisonNoWeaponsBoastReward", value.PrisonNoWeaponsBoastReward);
    visitor("RansomVictimNakedBoastCost", value.RansomVictimNakedBoastCost);
    visitor("RansomVictimNakedBoastReward", value.RansomVictimNakedBoastReward);
    visitor("RansomVictimNoDamageBoastCost", value.RansomVictimNoDamageBoastCost);
    visitor("RansomVictimNoDamageBoastReward", value.RansomVictimNoDamageBoastReward);
    visitor("RansomVictimNoWeaponsBoastCost", value.RansomVictimNoWeaponsBoastCost);
    visitor("RansomVictimNoWeaponsBoastReward", value.RansomVictimNoWeaponsBoastReward);
    visitor("TraderConflictEvilNakedBoastCost", value.TraderConflictEvilNakedBoastCost);
    visitor("TraderConflictEvilNakedBoastReward", value.TraderConflictEvilNakedBoastReward);
    visitor("TraderConflictEvilNoDamageBoastCost", value.TraderConflictEvilNoDamageBoastCost);
    visitor("TraderConflictEvilNoDamageBoastReward", value.TraderConflictEvilNoDamageBoastReward);
    visitor("TraderConflictEvilNoWeaponsBoastCost", value.TraderConflictEvilNoWeaponsBoastCost);
    visitor("TraderConflictEvilNoWeaponsBoastReward", value.TraderConflictEvilNoWeaponsBoastReward);
    visitor("TraderConflictGoodNakedBoastCost", value.TraderConflictGoodNakedBoastCost);
    visitor("TraderConflictGoodNakedBoastReward", value.TraderConflictGoodNakedBoastReward);
    visitor("TraderConflictGoodNoDamageBoastCost", value.TraderConflictGoodNoDamageBoastCost);
    visitor("TraderConflictGoodNoDamageBoastReward", value.TraderConflictGoodNoDamageBoastReward);
    visitor("TraderConflictGoodNoWeaponsBoastCost", value.TraderConflictGoodNoWeaponsBoastCost);
    visitor("TraderConflictGoodNoWeaponsBoastReward", value.TraderConflictGoodNoWeaponsBoastReward);
    visitor("TraderEscortNakedBoastCost", value.TraderEscortNakedBoastCost);
    visitor("TraderEscortNakedBoastReward", value.TraderEscortNakedBoastReward);
    visitor("TraderEscortNoDamageBoastCost", value.TraderEscortNoDamageBoastCost);
    visitor("TraderEscortNoDamageBoastReward", value.TraderEscortNoDamageBoastReward);
    visitor("TraderEscortNoWeaponsBoastCost", value.TraderEscortNoWeaponsBoastCost);
    visitor("TraderEscortNoWeaponsBoastReward", value.TraderEscortNoWeaponsBoastReward);
    visitor("WhiteBalvNakedBoastCost", value.WhiteBalvNakedBoastCost);
    visitor("WhiteBalvNakedBoastReward", value.WhiteBalvNakedBoastReward);
    visitor("WhiteBalvNoDamageBoastCost", value.WhiteBalvNoDamageBoastCost);
    visitor("WhiteBalvNoDamageBoastReward", value.WhiteBalvNoDamageBoastReward);
    visitor("WhiteBalvNoWeaponsBoastCost", value.WhiteBalvNoWeaponsBoastCost);
    visitor("WhiteBalvNoWeaponsBoastReward", value.WhiteBalvNoWeaponsBoastReward);
    visitor("SummoningShipNakedBoastCost", value.SummoningShipNakedBoastCost);
    visitor("SummoningShipNakedBoastReward", value.SummoningShipNakedBoastReward);
    visitor("SummoningShipNoDamageBoastCost", value.SummoningShipNoDamageBoastCost);
    visitor("SummoningShipNoDamageBoastReward", value.SummoningShipNoDamageBoastReward);
    visitor("TCGKillNoBanditsCost", value.TCGKillNoBanditsCost);
    visitor("TCGKillNoBanditsReward", value.TCGKillNoBanditsReward);
    visitor("TCGMadeTimeLimitCost", value.TCGMadeTimeLimitCost);
    visitor("TCGMadeTimeLimitReward", value.TCGMadeTimeLimitReward);
    visitor("TCEKeepBanditFollowerAliveCost", value.TCEKeepBanditFollowerAliveCost);
    visitor("TCEKeepBanditFollowerAliveReward", value.TCEKeepBanditFollowerAliveReward);
    visitor("TCEMadeTimeLimitCost", value.TCEMadeTimeLimitCost);
    visitor("TCEMadeTimeLimitReward", value.TCEMadeTimeLimitReward);
    visitor("RansomVictimVictimNoDamageCost", value.RansomVictimVictimNoDamageCost);
    visitor("RansomVictimVictimNoDamageReward", value.RansomVictimVictimNoDamageReward);
    visitor("RansomVictimKillKidnapperCost", value.RansomVictimKillKidnapperCost);
    visitor("RansomVictimKillKidnapperReward", value.RansomVictimKillKidnapperReward);
    visitor("RansomVictimSaveVictimCost", value.RansomVictimSaveVictimCost);
    visitor("RansomVictimSaveVictimReward", value.RansomVictimSaveVictimReward);
    visitor("RansomVictimHaveVictimKilledCost", value.RansomVictimHaveVictimKilledCost);
    visitor("RansomVictimHaveVictimKilledReward", value.RansomVictimHaveVictimKilledReward);
    visitor("MinionCampBriarNoDamageCost", value.MinionCampBriarNoDamageCost);
    visitor("MinionCampBriarNoDamageReward", value.MinionCampBriarNoDamageReward);
    visitor("AmbushScamTricksterApproachProximityLow", value.AmbushScamTricksterApproachProximityLow);
    visitor("AmbushScamTricksterApproachProximityHigh", value.AmbushScamTricksterApproachProximityHigh);
    visitor("AmbushScamTricksterHeroProximity", value.AmbushScamTricksterHeroProximity);
    visitor("AmbushScamTricksterRunAwayProximity", value.AmbushScamTricksterRunAwayProximity);
    visitor("AmbushScamAmbushTriggerProximity", value.AmbushScamAmbushTriggerProximity);
    visitor("AmbushScamApproachTriggerProximity", value.AmbushScamApproachTriggerProximity);
    visitor("AmbushScamBuddyWanderMax", value.AmbushScamBuddyWanderMax);
    visitor("AmbushScamTricksterWanderMax", value.AmbushScamTricksterWanderMax);
    visitor("AmbushScamGoldReward", value.AmbushScamGoldReward);
    visitor("AmbushScamBonusObject", value.AmbushScamBonusObject);
    visitor("AmbushScamExperience", value.AmbushScamExperience);
    visitor("AmbushScamRenown", value.AmbushScamRenown);
    visitor("AmbushScamMoralityEvil", value.AmbushScamMoralityEvil);
    visitor("AmbushScamMoralityGood", value.AmbushScamMoralityGood);
    visitor("AmbushScamRandomSpeechTimerMax", value.AmbushScamRandomSpeechTimerMax);
    visitor("AmbushScamRandomSpeechTimerVar", value.AmbushScamRandomSpeechTimerVar);
    visitor("AmbushScamScreamSoundIds", value.AmbushScamScreamSoundIds);
    visitor("AmbushScamScreamProximity", value.AmbushScamScreamProximity);
    visitor("ArcheryCompetitionCost", value.ArcheryCompetitionCost);
    visitor("ArcheryGameTimeLimit", value.ArcheryGameTimeLimit);
    visitor("ArcheryTimesUpDuration", value.ArcheryTimesUpDuration);
    visitor("ArcheryInitialHighScoreMax", value.ArcheryInitialHighScoreMax);
    visitor("ArcheryInitialHighScoreVar", value.ArcheryInitialHighScoreVar);
    visitor("ArcheryScoreForPrize", value.ArcheryScoreForPrize);
    visitor("ArcheryScoreForPrizeHigh", value.ArcheryScoreForPrizeHigh);
    visitor("ArcheryScoreForPrizeMedium", value.ArcheryScoreForPrizeMedium);
    visitor("ArcheryScoreForPrizeLow", value.ArcheryScoreForPrizeLow);
    visitor("ArcheryMaxOutsideRing", value.ArcheryMaxOutsideRing);
    visitor("ArcheryDummySpeedFast", value.ArcheryDummySpeedFast);
    visitor("ArcheryDummySpeedMedium", value.ArcheryDummySpeedMedium);
    visitor("ArcheryDummySpeedSlow", value.ArcheryDummySpeedSlow);
    visitor("ArcheryNumberOfMoves", value.ArcheryNumberOfMoves);
    visitor("ArcheryRewardObject", value.ArcheryRewardObject);
    visitor("ArcheryNormalPrizes", value.ArcheryNormalPrizes);
    visitor("ArcheryNormalPrizesHigh", value.ArcheryNormalPrizesHigh);
    visitor("ArcheryNormalPrizesMedium", value.ArcheryNormalPrizesMedium);
    visitor("ArcheryNormalPrizesLow", value.ArcheryNormalPrizesLow);
    visitor("ArcheryMatchHighScorePrizes", value.ArcheryMatchHighScorePrizes);
    visitor("ArcheryNewHighScorePrizes", value.ArcheryNewHighScorePrizes);
    visitor("ArcheryScorePowerLevels", value.ArcheryScorePowerLevels);
    visitor("ArcheryReallyHighScorePrize", value.ArcheryReallyHighScorePrize);
    visitor("ArcheryReallyHighScore", value.ArcheryReallyHighScore);
    visitor("ArcheryExperience", value.ArcheryExperience);
    visitor("ArcheryRenown", value.ArcheryRenown);
    visitor("ArcheryMoralityCheatUsedMagic", value.ArcheryMoralityCheatUsedMagic);
    visitor("ArcheryMoralityCheatSteppedOut", value.ArcheryMoralityCheatSteppedOut);
    visitor("ArcheryMoralityShotOwner", value.ArcheryMoralityShotOwner);
    visitor("ArcheryScoreDummyStatic", value.ArcheryScoreDummyStatic);
    visitor("ArcheryScoreDummyFront", value.ArcheryScoreDummyFront);
    visitor("ArcheryScoreDummyCentre", value.ArcheryScoreDummyCentre);
    visitor("ArcheryScoreDummyLeft", value.ArcheryScoreDummyLeft);
    visitor("ArcheryScoreDummyRight", value.ArcheryScoreDummyRight);
    visitor("ArcheryRandomSpeechTimerMax", value.ArcheryRandomSpeechTimerMax);
    visitor("ArcheryRandomSpeechTimerVar", value.ArcheryRandomSpeechTimerVar);
    visitor("AssassinActivateRenownLevel", value.AssassinActivateRenownLevel);
    visitor("AssassinTriggerProximity", value.AssassinTriggerProximity);
    visitor("AssassinScreamProximity", value.AssassinScreamProximity);
    visitor("AssassinChestProximity", value.AssassinChestProximity);
    visitor("AssassinCameraOffScreenTime", value.AssassinCameraOffScreenTime);
    visitor("AssassinScreamSoundIds", value.AssassinScreamSoundIds);
    visitor("AssassinReward", value.AssassinReward);
    visitor("AssassinRewardDelay", value.AssassinRewardDelay);
    visitor("AssassinObjects", value.AssassinObjects);
    visitor("AssassinLocationName", value.AssassinLocationName);
    visitor("AssassinRegionKillCount", value.AssassinRegionKillCount);
    visitor("AssassinExperience", value.AssassinExperience);
    visitor("AssassinRenown", value.AssassinRenown);
    visitor("AssassinMorality", value.AssassinMorality);
    visitor("BanditCampPathEavesDropProximity", value.BanditCampPathEavesDropProximity);
    visitor("BanditCampKillNoBanditsBoastCost", value.BanditCampKillNoBanditsBoastCost);
    visitor("BanditCampKillNoBanditsBoastReward", value.BanditCampKillNoBanditsBoastReward);
    visitor("BanditCampKillManyBanditsBoastCost", value.BanditCampKillManyBanditsBoastCost);
    visitor("BanditCampKillManyBanditsBoastReward", value.BanditCampKillManyBanditsBoastReward);
    visitor("KillTraderTriggerDistance", value.KillTraderTriggerDistance);
    visitor("TollBanditCallHeroDistance", value.TollBanditCallHeroDistance);
    visitor("TollBanditWarningOverDistance", value.TollBanditWarningOverDistance);
    visitor("TollBanditHeroLeftDistance", value.TollBanditHeroLeftDistance);
    visitor("TollBanditPayDistance", value.TollBanditPayDistance);
    visitor("TollBanditCallHeroTimer", value.TollBanditCallHeroTimer);
    visitor("TollBanditWarnHeroTimer", value.TollBanditWarnHeroTimer);
    visitor("TollBanditTauntTimer", value.TollBanditTauntTimer);
    visitor("BanditTollSneakDistance", value.BanditTollSneakDistance);
    visitor("BanditTollBanditWanderDistance", value.BanditTollBanditWanderDistance);
    visitor("BanditTollTraderWarnDistance", value.BanditTollTraderWarnDistance);
    visitor("TollBanditFee", value.TollBanditFee);
    visitor("TollBanditFeeWithFollower", value.TollBanditFeeWithFollower);
    visitor("TollBanditCreatureType", value.TollBanditCreatureType);
    visitor("TollBanditCloseToExit", value.TollBanditCloseToExit);
    visitor("TollBanditTimeToFade", value.TollBanditTimeToFade);
    visitor("BanditCountForToll", value.BanditCountForToll);
    visitor("BeardyBaldyWanderDistanceMin", value.BeardyBaldyWanderDistanceMin);
    visitor("BeardyBaldyWanderDistanceMax", value.BeardyBaldyWanderDistanceMax);
    visitor("BeardyBaldyHaircutStyle1", value.BeardyBaldyHaircutStyle1);
    visitor("BeardyBaldyHaircutCard1", value.BeardyBaldyHaircutCard1);
    visitor("BeardyBaldyHaircutStyle2", value.BeardyBaldyHaircutStyle2);
    visitor("BeardyBaldyHaircutCard2", value.BeardyBaldyHaircutCard2);
    visitor("BeardyBaldyBeardStyle1", value.BeardyBaldyBeardStyle1);
    visitor("BeardyBaldyBeardCard1", value.BeardyBaldyBeardCard1);
    visitor("BeardyBaldyBeardStyle2", value.BeardyBaldyBeardStyle2);
    visitor("BeardyBaldyBeardCard2", value.BeardyBaldyBeardCard2);
    visitor("BeardyBaldyTashStyle1", value.BeardyBaldyTashStyle1);
    visitor("BeardyBaldyTashCard1", value.BeardyBaldyTashCard1);
    visitor("BeardyBaldyTashStyle2", value.BeardyBaldyTashStyle2);
    visitor("BeardyBaldyTashCard2", value.BeardyBaldyTashCard2);
    visitor("BeardyBaldyRewardObjectFullComplete", value.BeardyBaldyRewardObjectFullComplete);
    visitor("BeardyBaldyRewardObjectHalfComplete", value.BeardyBaldyRewardObjectHalfComplete);
    visitor("BeardyBaldyExperience", value.BeardyBaldyExperience);
    visitor("BeardyBaldyRenown", value.BeardyBaldyRenown);
    visitor("BeardyBaldyMorality", value.BeardyBaldyMorality);
    visitor("BeardyBaldyRandomSpeechTimerMax", value.BeardyBaldyRandomSpeechTimerMax);
    visitor("BeardyBaldyRandomSpeechTimerVar", value.BeardyBaldyRandomSpeechTimerVar);
    visitor("HenchmanHealthLow", value.HenchmanHealthLow);
    visitor("HenchmanHealthCritical", value.HenchmanHealthCritical);
    visitor("HenchmanPayTimeDelayInSeconds", value.HenchmanPayTimeDelayInSeconds);
    visitor("HenchmanCheapGoldCost", value.HenchmanCheapGoldCost);
    visitor("HenchmanExpensiveGoldCost", value.HenchmanExpensiveGoldCost);
    visitor("BookBoyMarker", value.BookBoyMarker);
    visitor("BookGirlMarker", value.BookGirlMarker);
    visitor("BookReactions", value.BookReactions);
    visitor("BookDefNames", value.BookDefNames);
    visitor("BooksNumberWanted", value.BooksNumberWanted);
    visitor("BooksNumberAccepted", value.BooksNumberAccepted);
    visitor("BooksNumberComment", value.BooksNumberComment);
    visitor("BooksNumberTotal", value.BooksNumberTotal);
    visitor("BadBooksForHat", value.BadBooksForHat);
    visitor("GoodBooksForHat", value.GoodBooksForHat);
    visitor("BooksMoralityReward", value.BooksMoralityReward);
    visitor("BordelloHeroTavernPriceMultiplier", value.BordelloHeroTavernPriceMultiplier);
    visitor("ChapelActivationAge", value.ChapelActivationAge);
    visitor("ChapelAltarProximity", value.ChapelAltarProximity);
    visitor("ChapelAcolyteWanderDistance", value.ChapelAcolyteWanderDistance);
    visitor("ChapelAcolytePrayerTimer", value.ChapelAcolytePrayerTimer);
    visitor("ChapelAcolytePrayerTimerVar", value.ChapelAcolytePrayerTimerVar);
    visitor("ChapelDonationLevel", value.ChapelDonationLevel);
    visitor("ChapelMoralityRewardFraction", value.ChapelMoralityRewardFraction);
    visitor("ChapelDonationRewardObject", value.ChapelDonationRewardObject);
    visitor("ChapelWeaponBrandishProximity", value.ChapelWeaponBrandishProximity);
    visitor("ChapelDonationLevelCoolThing1", value.ChapelDonationLevelCoolThing1);
    visitor("ChapelDonationLevelCoolThing2", value.ChapelDonationLevelCoolThing2);
    visitor("ChapelMoralityChangeCoolThing1", value.ChapelMoralityChangeCoolThing1);
    visitor("ChapelMoralityChangeCoolThing2", value.ChapelMoralityChangeCoolThing2);
    visitor("ChapelDonationRewardAgeMin", value.ChapelDonationRewardAgeMin);
    visitor("ChapelDonationRewardAgeVar", value.ChapelDonationRewardAgeVar);
    visitor("ChapelDonationRewardTitle", value.ChapelDonationRewardTitle);
    visitor("ChapelDonationLevelFiddleSuper", value.ChapelDonationLevelFiddleSuper);
    visitor("ChapelDonationLevelFiddleVery", value.ChapelDonationLevelFiddleVery);
    visitor("ChapelDonationLevelFiddle", value.ChapelDonationLevelFiddle);
    visitor("ChapelExperience", value.ChapelExperience);
    visitor("ChapelRenown", value.ChapelRenown);
    visitor("ChapelMorality", value.ChapelMorality);
    visitor("DealerAttentionDistance", value.DealerAttentionDistance);
    visitor("DealerAttentionTimer", value.DealerAttentionTimer);
    visitor("TraderLeavingDelay", value.TraderLeavingDelay);
    visitor("TraderAttentionDistance", value.TraderAttentionDistance);
    visitor("TraderConversationDelay", value.TraderConversationDelay);
    visitor("TraderStopWalkingTimer", value.TraderStopWalkingTimer);
    visitor("TraderStandStillTimer", value.TraderStandStillTimer);
    visitor("EasyPickingsReward", value.EasyPickingsReward);
    visitor("ExposeMayorEvidenceDef", value.ExposeMayorEvidenceDef);
    visitor("EMMaxFlashDistFromStables", value.EMMaxFlashDistFromStables);
    visitor("F_FishermanHelpDelay", value.F_FishermanHelpDelay);
    visitor("F_FishermanIdleChatDelay", value.F_FishermanIdleChatDelay);
    visitor("F_WaspType", value.F_WaspType);
    visitor("FishermanSaveHimMorality", value.FishermanSaveHimMorality);
    visitor("FishTypesInGame", value.FishTypesInGame);
    visitor("FishingRodsInGame", value.FishingRodsInGame);
    visitor("FishingPrizes", value.FishingPrizes);
    visitor("FishPrizeDefNames", value.FishPrizeDefNames);
    visitor("FishRodDefNames", value.FishRodDefNames);
    visitor("FishDefNames", value.FishDefNames);
    visitor("FishPrizeWeights", value.FishPrizeWeights);
    visitor("FC_FighterDelay", value.FC_FighterDelay);
    visitor("FC_MinimumFighterHealth", value.FC_MinimumFighterHealth);
    visitor("FC_OutsideRingRange", value.FC_OutsideRingRange);
    visitor("FC_RingTimeOut", value.FC_RingTimeOut);
    visitor("FC_BowerstoneAllowedHits", value.FC_BowerstoneAllowedHits);
    visitor("FC_OakvaleAllowedHits", value.FC_OakvaleAllowedHits);
    visitor("FC_BanditCampAllowedHits", value.FC_BanditCampAllowedHits);
    visitor("FC_KnotholeGladeAllowedHits", value.FC_KnotholeGladeAllowedHits);
    visitor("FC_HitsDepletionOverSession", value.FC_HitsDepletionOverSession);
    visitor("FC_BowerstoneFightCount", value.FC_BowerstoneFightCount);
    visitor("FC_OakvaleFightCount", value.FC_OakvaleFightCount);
    visitor("FC_BanditCampFightCount", value.FC_BanditCampFightCount);
    visitor("FC_KnotholeGladeFightCount", value.FC_KnotholeGladeFightCount);
    visitor("FC_StartTimeOfDay", value.FC_StartTimeOfDay);
    visitor("FC_EndTimeOfDay", value.FC_EndTimeOfDay);
    visitor("FC_MoneyAddedPerFight", value.FC_MoneyAddedPerFight);
    visitor("FC_RingRadius", value.FC_RingRadius);
    visitor("FC_InsideRingMessageDelay", value.FC_InsideRingMessageDelay);
    visitor("FC_AttackWarnings", value.FC_AttackWarnings);
    visitor("FC_BowerstoneRewardItem", value.FC_BowerstoneRewardItem);
    visitor("FC_OakvaleRewardItem", value.FC_OakvaleRewardItem);
    visitor("FC_BanditCampRewardItem", value.FC_BanditCampRewardItem);
    visitor("FC_KnotholeGladeRewardItem", value.FC_KnotholeGladeRewardItem);
    visitor("FC_DistanceAwayToTriggerFC", value.FC_DistanceAwayToTriggerFC);
    visitor("FC_MinimumFee", value.FC_MinimumFee);
    visitor("FC_ShoutOutDistance", value.FC_ShoutOutDistance);
    visitor("FC_LostRenownLoss", value.FC_LostRenownLoss);
    visitor("FC_WinRenownGain", value.FC_WinRenownGain);
    visitor("FC_WinExperienceGain", value.FC_WinExperienceGain);
    visitor("GGN_GhostSpawnNearby", value.GGN_GhostSpawnNearby);
    visitor("GGN_GhostSpawnDistance", value.GGN_GhostSpawnDistance);
    visitor("GGN_ReturnNecklaceReward", value.GGN_ReturnNecklaceReward);
    visitor("GGN_GhostClueTime", value.GGN_GhostClueTime);
    visitor("GGN_GhostTalkDistance", value.GGN_GhostTalkDistance);
    visitor("GGN_ReturnNecklaceMoralityGain", value.GGN_ReturnNecklaceMoralityGain);
    visitor("GGN_ReturnNecklaceExperienceGain", value.GGN_ReturnNecklaceExperienceGain);
    visitor("HH_UpperBarLimit", value.HH_UpperBarLimit);
    visitor("HH_BarDecrementAmount", value.HH_BarDecrementAmount);
    visitor("HH_BarDecrementInterval", value.HH_BarDecrementInterval);
    visitor("HH_ClearedHHRenown", value.HH_ClearedHHRenown);
    visitor("HeroDuelDefeatThunderMorality", value.HeroDuelDefeatThunderMorality);
    visitor("HeroDuelDefeatThunderXP", value.HeroDuelDefeatThunderXP);
    visitor("HiddenBootyTreasureFake", value.HiddenBootyTreasureFake);
    visitor("HiddenBootyTreasureReal", value.HiddenBootyTreasureReal);
    visitor("HiddenBootyExperience", value.HiddenBootyExperience);
    visitor("HiddenBootyRenownReal", value.HiddenBootyRenownReal);
    visitor("HiddenBootyRenownFake", value.HiddenBootyRenownFake);
    visitor("HiddenBootyMorality", value.HiddenBootyMorality);
    visitor("TrophiesOpinionDeedTimer", value.TrophiesOpinionDeedTimer);
    visitor("TrophiesBriarRoseTrophyLeft", value.TrophiesBriarRoseTrophyLeft);
    visitor("TrophiesBriarRoseTrophyRight", value.TrophiesBriarRoseTrophyRight);
    visitor("TrophiesBriarRoseProximityNear", value.TrophiesBriarRoseProximityNear);
    visitor("TrophiesBriarRoseProximityFar", value.TrophiesBriarRoseProximityFar);
    visitor("TrophiesStandStillTimer", value.TrophiesStandStillTimer);
    visitor("TrophiesFacingWatcherTimer", value.TrophiesFacingWatcherTimer);
    visitor("TrophiesBriarRoseWalkingSpeed", value.TrophiesBriarRoseWalkingSpeed);
    visitor("TrophiesCrowdWanderDistanceMin", value.TrophiesCrowdWanderDistanceMin);
    visitor("TrophiesCrowdWanderDistanceMax", value.TrophiesCrowdWanderDistanceMax);
    visitor("TrophiesBriarRoseAnimTime", value.TrophiesBriarRoseAnimTime);
    visitor("TrophiesBriarRoseAnimTimeVar", value.TrophiesBriarRoseAnimTimeVar);
    visitor("TrophiesInsideTavernTimer", value.TrophiesInsideTavernTimer);
    visitor("TrophiesOutsideTavernTimer", value.TrophiesOutsideTavernTimer);
    visitor("TrophiesHitCountForBriarAttack", value.TrophiesHitCountForBriarAttack);
    visitor("OuterTraderDistance", value.OuterTraderDistance);
    visitor("MiddleTraderDistance", value.MiddleTraderDistance);
    visitor("InnerTraderDistance", value.InnerTraderDistance);
    visitor("TraderYellForHelpRandomValue", value.TraderYellForHelpRandomValue);
    visitor("TraderYellAtHeroRandomValue", value.TraderYellAtHeroRandomValue);
    visitor("TraderCommentRandomValue", value.TraderCommentRandomValue);
    visitor("BrotherStartDistance", value.BrotherStartDistance);
    visitor("BrotherScreamForHelpRandomValue", value.BrotherScreamForHelpRandomValue);
    visitor("BrotherWaitCommentRandomValue", value.BrotherWaitCommentRandomValue);
    visitor("HeroEvilMoralityValue", value.HeroEvilMoralityValue);
    visitor("HeroGoodMoralityValue", value.HeroGoodMoralityValue);
    visitor("HeroKillsAssassinMorality", value.HeroKillsAssassinMorality);
    visitor("HeroKillsSheriffMorality", value.HeroKillsSheriffMorality);
    visitor("HeroTakesAssassinGoldMorality", value.HeroTakesAssassinGoldMorality);
    visitor("HeroTakesAssassinGoldMoralityKillsAssassin", value.HeroTakesAssassinGoldMoralityKillsAssassin);
    visitor("HeroKillsAssassinRenown", value.HeroKillsAssassinRenown);
    visitor("HeroKillsAssassinProofOfKill", value.HeroKillsAssassinProofOfKill);
    visitor("HeroKillsSheriffRenown", value.HeroKillsSheriffRenown);
    visitor("AssassinThreatDistSq", value.AssassinThreatDistSq);
    visitor("RandomPopulationTraderGoldMinimum", value.RandomPopulationTraderGoldMinimum);
    visitor("RandomPopulationTraderGoldVariation", value.RandomPopulationTraderGoldVariation);
    visitor("RockTrollTriggerProximity", value.RockTrollTriggerProximity);
    visitor("RockTrollReward1", value.RockTrollReward1);
    visitor("RockTrollReward2", value.RockTrollReward2);
    visitor("IngredientObject", value.IngredientObject);
    visitor("ObjectToGiveToMansLover", value.ObjectToGiveToMansLover);
    visitor("ObjectFromMansLover", value.ObjectFromMansLover);
    visitor("EndPotion", value.EndPotion);
    visitor("SickChildSleepingAnim", value.SickChildSleepingAnim);
    visitor("SickChildsSisterAttentionAnims", value.SickChildsSisterAttentionAnims);
    visitor("TellingTheTruth", value.TellingTheTruth);
    visitor("LyingToWomanMorality", value.LyingToWomanMorality);
    visitor("LyingToManAboutWomanNotLikingHer", value.LyingToManAboutWomanNotLikingHer);
    visitor("LyingToManAboutWomanLikingHer", value.LyingToManAboutWomanLikingHer);
    visitor("KilledSomeoneForMushroom", value.KilledSomeoneForMushroom);
    visitor("StoleMushroomInBarrowFields", value.StoleMushroomInBarrowFields);
    visitor("BoughtMushroomInBarrowFields", value.BoughtMushroomInBarrowFields);
    visitor("MadeWomanLaugh", value.MadeWomanLaugh);
    visitor("SingingStonesDoorManWanderMax", value.SingingStonesDoorManWanderMax);
    visitor("SwordInTheStoneWanderDistanceMin", value.SwordInTheStoneWanderDistanceMin);
    visitor("SwordInTheStoneWanderDistanceMax", value.SwordInTheStoneWanderDistanceMax);
    visitor("SwordInTheStoneSuccessExperience", value.SwordInTheStoneSuccessExperience);
    visitor("SwordInTheStoneSuccessRenown", value.SwordInTheStoneSuccessRenown);
    visitor("SwordInTheStoneSuccessMorality", value.SwordInTheStoneSuccessMorality);
    visitor("SwordInTheStonePhysiqueChange", value.SwordInTheStonePhysiqueChange);
    visitor("SwordInTheStoneToughnessChange", value.SwordInTheStoneToughnessChange);
    visitor("SwordInTheStoneHealthChange", value.SwordInTheStoneHealthChange);
    visitor("BardSingingFee", value.BardSingingFee);
    visitor("BardNumVersesPerVisit", value.BardNumVersesPerVisit);
    visitor("BardConsiderEvil", value.BardConsiderEvil);
    visitor("BardConsiderGood", value.BardConsiderGood);
    visitor("BardConsiderRich", value.BardConsiderRich);
    visitor("BardConsiderBoaster", value.BardConsiderBoaster);
    visitor("BardConsiderFailure", value.BardConsiderFailure);
    visitor("BardSpecial", value.BardSpecial);
    visitor("BardGoAwayCost", value.BardGoAwayCost);
    visitor("BardWanderDistanceMin", value.BardWanderDistanceMin);
    visitor("BardWanderDistanceMax", value.BardWanderDistanceMax);
    visitor("BardWanderResetTime", value.BardWanderResetTime);
    visitor("BardFollowDistance", value.BardFollowDistance);
    visitor("BardRandomSpeechTimerMax", value.BardRandomSpeechTimerMax);
    visitor("BardRandomSpeechTimerVar", value.BardRandomSpeechTimerVar);
    visitor("BardHeroAgeHigh", value.BardHeroAgeHigh);
    visitor("BardHeroAgeMedium", value.BardHeroAgeMedium);
    visitor("BardHeroHealthHigh", value.BardHeroHealthHigh);
    visitor("BardHeroHealthLow", value.BardHeroHealthLow);
    visitor("BardHeroWillHigh", value.BardHeroWillHigh);
    visitor("BardHeroWillLow", value.BardHeroWillLow);
    visitor("TempleActivationAge", value.TempleActivationAge);
    visitor("TempleAltarProximity", value.TempleAltarProximity);
    visitor("TempleAcolyteWanderDistance", value.TempleAcolyteWanderDistance);
    visitor("TempleAcolytePrayerTimer", value.TempleAcolytePrayerTimer);
    visitor("TempleAcolytePrayerTimerVar", value.TempleAcolytePrayerTimerVar);
    visitor("TempleDonationLevel", value.TempleDonationLevel);
    visitor("TempleMoralityRewardFraction", value.TempleMoralityRewardFraction);
    visitor("TempleDonationRewardObject", value.TempleDonationRewardObject);
    visitor("TempleDonationLevelCoolThing1", value.TempleDonationLevelCoolThing1);
    visitor("TempleDonationLevelCoolThing2", value.TempleDonationLevelCoolThing2);
    visitor("TempleMoralityChangeCoolThing1", value.TempleMoralityChangeCoolThing1);
    visitor("TempleMoralityChangeCoolThing2", value.TempleMoralityChangeCoolThing2);
    visitor("TempleDonationRewardAgeMin", value.TempleDonationRewardAgeMin);
    visitor("TempleDonationRewardAgeVar", value.TempleDonationRewardAgeVar);
    visitor("TempleDonationRewardTitle", value.TempleDonationRewardTitle);
    visitor("TempleDonationLevelFiddleSuper", value.TempleDonationLevelFiddleSuper);
    visitor("TempleDonationLevelFiddleVery", value.TempleDonationLevelFiddleVery);
    visitor("TempleDonationLevelFiddle", value.TempleDonationLevelFiddle);
    visitor("TempleExperience", value.TempleExperience);
    visitor("TempleRenown", value.TempleRenown);
    visitor("TempleMorality", value.TempleMorality);
    visitor("TemplePeasantWalkSpeed", value.TemplePeasantWalkSpeed);
    visitor("TemplePrayerCounterActivation", value.TemplePrayerCounterActivation);
    visitor("TempleDonationBoxRequirement", value.TempleDonationBoxRequirement);
    visitor("TempleWeaponBrandishProximity", value.TempleWeaponBrandishProximity);
    visitor("TempleTeleportEnableProximity", value.TempleTeleportEnableProximity);
    visitor("TemplePrayerFactorLowest", value.TemplePrayerFactorLowest);
    visitor("TemplePrayerFactorLow", value.TemplePrayerFactorLow);
    visitor("TemplePrayerFactorMiddle", value.TemplePrayerFactorMiddle);
    visitor("TemplePrayerFactorHigh", value.TemplePrayerFactorHigh);
    visitor("TemplePrayerFactorHighest", value.TemplePrayerFactorHighest);
    visitor("TemplePrayerFactorHighest2", value.TemplePrayerFactorHighest2);
    visitor("TemplePrayerFactorBurnChance", value.TemplePrayerFactorBurnChance);
    visitor("TemplePrayerWorshipRewardHealth", value.TemplePrayerWorshipRewardHealth);
    visitor("TemplePrayerWorshipRewardMorality", value.TemplePrayerWorshipRewardMorality);
    visitor("TemplePrayerWorshipRewardGold", value.TemplePrayerWorshipRewardGold);
    visitor("TemplePrayerWorshipRewardDonationModifier", value.TemplePrayerWorshipRewardDonationModifier);
    visitor("TemplePrayerWorshipBurnHealth", value.TemplePrayerWorshipBurnHealth);
    visitor("TemplePrayerWorshipBurnGold", value.TemplePrayerWorshipBurnGold);
    visitor("TourGuideFollowerDistance", value.TourGuideFollowerDistance);
    visitor("TourGuideOverhearDistance", value.TourGuideOverhearDistance);
    visitor("TourGuideTalkDistance", value.TourGuideTalkDistance);
    visitor("TourGuideStandStillTimer", value.TourGuideStandStillTimer);
    visitor("TourGuideTalkToHeroTimer", value.TourGuideTalkToHeroTimer);
    visitor("TourGuideFollowerStandStillTimer", value.TourGuideFollowerStandStillTimer);
    visitor("TourGuideOpeningTime", value.TourGuideOpeningTime);
    visitor("TourGuideClosingTime", value.TourGuideClosingTime);
    visitor("SlumsBeggarGoldAmount", value.SlumsBeggarGoldAmount);
    visitor("OakValeBeggarGoldAmount", value.OakValeBeggarGoldAmount);
    visitor("PickUpLitterTimeInSeconds", value.PickUpLitterTimeInSeconds);
    visitor("NumPiecesOfLitter", value.NumPiecesOfLitter);
    visitor("TBRMoralityGoodSmall", value.TBRMoralityGoodSmall);
    visitor("TBRMoralityGoodMedium", value.TBRMoralityGoodMedium);
    visitor("TBRMoralityGoodLarge", value.TBRMoralityGoodLarge);
    visitor("TBYMoralityBadSmall", value.TBYMoralityBadSmall);
    visitor("TBYMoralityBadMedium", value.TBYMoralityBadMedium);
    visitor("TBYMoralityBadLarge", value.TBYMoralityBadLarge);
    visitor("OakShopKeepGoldAmount", value.OakShopKeepGoldAmount);
    visitor("ArsonGoldAmount", value.ArsonGoldAmount);
    visitor("KGExtortAmount", value.KGExtortAmount);
    visitor("BULLY_WEAPON", value.BULLY_WEAPON);
    visitor("TH_ThunderConv_Length", value.TH_ThunderConv_Length);
    visitor("TH_ThunderConv_Speaker", value.TH_ThunderConv_Speaker);
    visitor("TH_ThunderConv_Dialogue", value.TH_ThunderConv_Dialogue);
    visitor("TH_ThunderConv_Actor", value.TH_ThunderConv_Actor);
    visitor("TH_ThunderConv_Animation", value.TH_ThunderConv_Animation);
    visitor("TH_ThunderConv_AnimDelay", value.TH_ThunderConv_AnimDelay);
    visitor("EscortTraderRewardDownpayment", value.EscortTraderRewardDownpayment);
    visitor("EscortTraderAttackComplaintTime", value.EscortTraderAttackComplaintTime);
    visitor("EscortTraderRewardObjectPerfect", value.EscortTraderRewardObjectPerfect);
    visitor("EscortTraderRegionExitWait", value.EscortTraderRegionExitWait);
    visitor("EscortTraderRegionExitStop", value.EscortTraderRegionExitStop);
    visitor("EscortTraderRegionExitDist", value.EscortTraderRegionExitDist);
    visitor("EscortTraderHitCountHigh", value.EscortTraderHitCountHigh);
    visitor("EscortTraderHitCountMedium", value.EscortTraderHitCountMedium);
    visitor("EscortTraderHealthHigh", value.EscortTraderHealthHigh);
    visitor("EscortTraderHealthMedium", value.EscortTraderHealthMedium);
    visitor("EscortTraderFollowProximity1", value.EscortTraderFollowProximity1);
    visitor("EscortTraderFollowProximity2", value.EscortTraderFollowProximity2);
    visitor("EscortTraderProximityTimer", value.EscortTraderProximityTimer);
    visitor("EscortTraderExpressionDetectTimer", value.EscortTraderExpressionDetectTimer);
    visitor("EscortTraderHitDetectTimer", value.EscortTraderHitDetectTimer);
    visitor("EscortTraderSpawnedBanditType", value.EscortTraderSpawnedBanditType);
    visitor("EscortTraderExperience", value.EscortTraderExperience);
    visitor("EscortTraderRenown", value.EscortTraderRenown);
    visitor("EscortTraderMorality", value.EscortTraderMorality);
    visitor("EscortTraderFailureRenown", value.EscortTraderFailureRenown);
    visitor("TraderRandomSpeechTimerMax", value.TraderRandomSpeechTimerMax);
    visitor("TraderRandomSpeechTimerVar", value.TraderRandomSpeechTimerVar);
    visitor("EscortTraderWanderDistance", value.EscortTraderWanderDistance);
    visitor("EscortTraderQuestDayInterval", value.EscortTraderQuestDayInterval);
    visitor("EscortTraderQuestStartTime", value.EscortTraderQuestStartTime);
    visitor("AmbushTradersBanditHireCost", value.AmbushTradersBanditHireCost);
    visitor("AmbushTradersConvoyExitProximity", value.AmbushTradersConvoyExitProximity);
    visitor("AmbushTradersPhase1GuardWanderDistance", value.AmbushTradersPhase1GuardWanderDistance);
    visitor("AmbushTradersContactWanderDistance", value.AmbushTradersContactWanderDistance);
    visitor("AmbushTradersConvoyTriggerProximity", value.AmbushTradersConvoyTriggerProximity);
    visitor("AmbushTradersSentryWanderDistance", value.AmbushTradersSentryWanderDistance);
    visitor("AmbushTradersSpySentryProximity", value.AmbushTradersSpySentryProximity);
    visitor("AmbushTradersSpyHeroProximity", value.AmbushTradersSpyHeroProximity);
    visitor("AmbushTradersQuestSuccessReward", value.AmbushTradersQuestSuccessReward);
    visitor("AmbushTradersQuestSuccessBonus", value.AmbushTradersQuestSuccessBonus);
    visitor("AmbushTradersBanditWanderMax", value.AmbushTradersBanditWanderMax);
    visitor("AmbushTradersExperience", value.AmbushTradersExperience);
    visitor("AmbushTradersRenown", value.AmbushTradersRenown);
    visitor("AmbushTradersMorality", value.AmbushTradersMorality);
    visitor("AmbushTradersBoastCostKillAllGuards", value.AmbushTradersBoastCostKillAllGuards);
    visitor("AmbushTradersBoastRewardKillAllGuards", value.AmbushTradersBoastRewardKillAllGuards);
    visitor("AmbushTradersBoastCostKillAllTraders", value.AmbushTradersBoastCostKillAllTraders);
    visitor("AmbushTradersBoastRewardKillAllTraders", value.AmbushTradersBoastRewardKillAllTraders);
    visitor("AmbushTradersBoastCostKillNone", value.AmbushTradersBoastCostKillNone);
    visitor("AmbushTradersBoastRewardKillNone", value.AmbushTradersBoastRewardKillNone);
    visitor("AmbushTradersBoastCostUnassisted", value.AmbushTradersBoastCostUnassisted);
    visitor("AmbushTradersBoastRewardUnassisted", value.AmbushTradersBoastRewardUnassisted);
    visitor("AmbushTradersBanditFollowDistance", value.AmbushTradersBanditFollowDistance);
    visitor("AmbushTradersPhase2GuardWanderDistance", value.AmbushTradersPhase2GuardWanderDistance);
    visitor("AmbushTradersSpeechBanditProximity", value.AmbushTradersSpeechBanditProximity);
    visitor("AmbushTradersSpeechSpyProximity", value.AmbushTradersSpeechSpyProximity);
    visitor("AmbushTradersBanditSpeechTimerLong", value.AmbushTradersBanditSpeechTimerLong);
    visitor("AmbushTradersBanditSpeechTimerShort", value.AmbushTradersBanditSpeechTimerShort);
    visitor("AmbushTradersSpySpeechTimerProximity", value.AmbushTradersSpySpeechTimerProximity);
    visitor("AmbushTradersSpySpeechTimerLong", value.AmbushTradersSpySpeechTimerLong);
    visitor("AmbushTradersSpySpeechTimerShort", value.AmbushTradersSpySpeechTimerShort);
    visitor("AmbushTradersSpySpeechTimerSentry", value.AmbushTradersSpySpeechTimerSentry);
    visitor("AmbushTradersInvalidRegionTimerMax", value.AmbushTradersInvalidRegionTimerMax);
    visitor("AmbushTradersInvalidRegionTimerVar", value.AmbushTradersInvalidRegionTimerVar);
    visitor("ArenaCrowdShoutDelay", value.ArenaCrowdShoutDelay);
    visitor("ArenaCrowdShoutDelayRandom", value.ArenaCrowdShoutDelayRandom);
    visitor("RewardObjects", value.RewardObjects);
    visitor("ArenaCountdownDuration", value.ArenaCountdownDuration);
    visitor("ArenaCreatureRound1", value.ArenaCreatureRound1);
    visitor("ArenaNumberOrCreaturesRound1", value.ArenaNumberOrCreaturesRound1);
    visitor("ArenaNumberOfWavesRound1", value.ArenaNumberOfWavesRound1);
    visitor("ArenaCreatureRound2", value.ArenaCreatureRound2);
    visitor("ArenaNumberOrCreaturesRound2", value.ArenaNumberOrCreaturesRound2);
    visitor("ArenaNumberOfWavesRound2", value.ArenaNumberOfWavesRound2);
    visitor("ArenaCreatureRound3", value.ArenaCreatureRound3);
    visitor("ArenaNumberOrCreaturesRound3", value.ArenaNumberOrCreaturesRound3);
    visitor("ArenaNumberOfWavesRound3", value.ArenaNumberOfWavesRound3);
    visitor("ArenaCreatureRound4", value.ArenaCreatureRound4);
    visitor("ArenaNumberOrCreaturesRound4", value.ArenaNumberOrCreaturesRound4);
    visitor("ArenaNumberOfWavesRound4", value.ArenaNumberOfWavesRound4);
    visitor("ArenaCreatureRound5", value.ArenaCreatureRound5);
    visitor("ArenaNumberOrCreaturesRound5", value.ArenaNumberOrCreaturesRound5);
    visitor("ArenaNumberOfWavesRound5", value.ArenaNumberOfWavesRound5);
    visitor("ArenaCreatureRound6", value.ArenaCreatureRound6);
    visitor("ArenaNumberOrCreaturesRound6", value.ArenaNumberOrCreaturesRound6);
    visitor("ArenaNumberOfWavesRound6", value.ArenaNumberOfWavesRound6);
    visitor("ArenaCreatureRound7", value.ArenaCreatureRound7);
    visitor("ArenaNumberOrCreaturesRound7", value.ArenaNumberOrCreaturesRound7);
    visitor("ArenaNumberOfWavesRound7", value.ArenaNumberOfWavesRound7);
    visitor("ArenaCreatureRound8", value.ArenaCreatureRound8);
    visitor("ArenaNumberOrCreaturesRound8", value.ArenaNumberOrCreaturesRound8);
    visitor("ArenaNumberOfWavesRound8", value.ArenaNumberOfWavesRound8);
    visitor("ArenaShowNoMercyBoastAmount", value.ArenaShowNoMercyBoastAmount);
    visitor("ArenaShowMercyBoastAmount", value.ArenaShowMercyBoastAmount);
    visitor("ArenaShowNoMercyBoastReward", value.ArenaShowNoMercyBoastReward);
    visitor("ArenaShowMercyBoastReward", value.ArenaShowMercyBoastReward);
    visitor("ArenaSpareWhisperMorality", value.ArenaSpareWhisperMorality);
    visitor("ArenaKillWhisperMorality", value.ArenaKillWhisperMorality);
    visitor("ATO_DiggingRewards", value.ATO_DiggingRewards);
    visitor("ATO_MaxSpawnedCreatures", value.ATO_MaxSpawnedCreatures);
    visitor("BountyHuntGoldReward", value.BountyHuntGoldReward);
    visitor("BountyHuntVillagerIntroProximity", value.BountyHuntVillagerIntroProximity);
    visitor("BountyHuntVillagerWanderDistanceMin", value.BountyHuntVillagerWanderDistanceMin);
    visitor("BountyHuntVillagerWanderDistanceMax", value.BountyHuntVillagerWanderDistanceMax);
    visitor("BountyHuntVillagerHeroProximity", value.BountyHuntVillagerHeroProximity);
    visitor("BountyHuntVillagerHeroStartConversation", value.BountyHuntVillagerHeroStartConversation);
    visitor("BountyHuntCutsceneTriggerProximity", value.BountyHuntCutsceneTriggerProximity);
    visitor("BountyHuntBanditGruntWanderDistance", value.BountyHuntBanditGruntWanderDistance);
    visitor("BountyHuntBanditLeaderWanderDistance", value.BountyHuntBanditLeaderWanderDistance);
    visitor("BountyHuntHostage2WanderDistance", value.BountyHuntHostage2WanderDistance);
    visitor("BountyHuntBanditDeputy1WanderDistance", value.BountyHuntBanditDeputy1WanderDistance);
    visitor("BountyHuntBanditDeputy1AttackProximity", value.BountyHuntBanditDeputy1AttackProximity);
    visitor("BountyHuntBanditDeputy2WanderDistance", value.BountyHuntBanditDeputy2WanderDistance);
    visitor("BountyHuntBanditDeputy2AttackProximity", value.BountyHuntBanditDeputy2AttackProximity);
    visitor("BountyHuntLeaderHealthDiffStage1", value.BountyHuntLeaderHealthDiffStage1);
    visitor("BountyHuntLeaderHealthDiffStage2", value.BountyHuntLeaderHealthDiffStage2);
    visitor("BountyHuntLeaderHealthDiffStage3", value.BountyHuntLeaderHealthDiffStage3);
    visitor("BountyHuntLeaderHealthDiffStage4", value.BountyHuntLeaderHealthDiffStage4);
    visitor("BountyHuntExperience", value.BountyHuntExperience);
    visitor("BountyHuntRenownHostage1", value.BountyHuntRenownHostage1);
    visitor("BountyHuntRenownHostage2", value.BountyHuntRenownHostage2);
    visitor("BountyHuntMorality", value.BountyHuntMorality);
    visitor("BountyHuntCompletionMorality", value.BountyHuntCompletionMorality);
    visitor("BountyHuntBoastTimeLimit", value.BountyHuntBoastTimeLimit);
    visitor("BountyHuntBoastCostTimeLimit", value.BountyHuntBoastCostTimeLimit);
    visitor("BountyHuntBoastRewardTimeLimit", value.BountyHuntBoastRewardTimeLimit);
    visitor("BountyHuntBoastCostDecapitation", value.BountyHuntBoastCostDecapitation);
    visitor("BountyHuntBoastRewardDecapitation", value.BountyHuntBoastRewardDecapitation);
    visitor("BountyHuntRansomGold", value.BountyHuntRansomGold);
    visitor("BountyHuntRewardObject", value.BountyHuntRewardObject);
    visitor("BowerstoneIntroGuardProximity", value.BowerstoneIntroGuardProximity);
    visitor("BowerstoneIntroPoshGuardProximity", value.BowerstoneIntroPoshGuardProximity);
    visitor("BowerstoneIntroCrowdWanderDistanceMin", value.BowerstoneIntroCrowdWanderDistanceMin);
    visitor("BowerstoneIntroCrowdWanderDistanceMax", value.BowerstoneIntroCrowdWanderDistanceMax);
    visitor("BS_BanditAwarenessRange", value.BS_BanditAwarenessRange);
    visitor("BS_RewardGold", value.BS_RewardGold);
    visitor("BS_NoAssistanceBoastAmount", value.BS_NoAssistanceBoastAmount);
    visitor("BS_KilledLeaderBoastAmount", value.BS_KilledLeaderBoastAmount);
    visitor("BS_NoAssistanceBoastReward", value.BS_NoAssistanceBoastReward);
    visitor("BS_KilledLeaderBoastReward", value.BS_KilledLeaderBoastReward);
    visitor("BS_NoOfWavesInFirstAttack", value.BS_NoOfWavesInFirstAttack);
    visitor("BS_NoOfBanditsInWave", value.BS_NoOfBanditsInWave);
    visitor("BS_BanditTypeInFirstAttack", value.BS_BanditTypeInFirstAttack);
    visitor("BS_EnvTheme", value.BS_EnvTheme);
    visitor("DBDragonMediumHealth", value.DBDragonMediumHealth);
    visitor("DBDragonLowHealth", value.DBDragonLowHealth);
    visitor("DBDragonVeryLowHealth", value.DBDragonVeryLowHealth);
    visitor("DBNumMinionsHighHealth", value.DBNumMinionsHighHealth);
    visitor("DBNumMinionsMediumHealth", value.DBNumMinionsMediumHealth);
    visitor("DBNumMinionsLowHealth", value.DBNumMinionsLowHealth);
    visitor("DBNumMinionsVeryLowHealth", value.DBNumMinionsVeryLowHealth);
    visitor("DBNumSummonersHighHealth", value.DBNumSummonersHighHealth);
    visitor("DBNumSummonersMediumHealth", value.DBNumSummonersMediumHealth);
    visitor("DBNumSummonersLowHealth", value.DBNumSummonersLowHealth);
    visitor("DBNumSummonersVeryLowHealth", value.DBNumSummonersVeryLowHealth);
    visitor("DBNumFlyBysBetweenSummonerSpawns", value.DBNumFlyBysBetweenSummonerSpawns);
    visitor("EndGame_UpperBarLimit", value.EndGame_UpperBarLimit);
    visitor("EndGame_BarDecrementAmount", value.EndGame_BarDecrementAmount);
    visitor("EndGame_BarDecrementInterval", value.EndGame_BarDecrementInterval);
    visitor("EndGame_TeleportEffect", value.EndGame_TeleportEffect);
    visitor("EndGame_TeleportHeroDistance", value.EndGame_TeleportHeroDistance);
    visitor("EndGame_Stage1FX", value.EndGame_Stage1FX);
    visitor("EndGame_Stage2FX", value.EndGame_Stage2FX);
    visitor("EndGame_Stage3FX", value.EndGame_Stage3FX);
    visitor("FHSunPadFX", value.FHSunPadFX);
    visitor("FHMoonPadFX", value.FHMoonPadFX);
    visitor("FHPadFXHeight", value.FHPadFXHeight);
    visitor("FHPadRadius", value.FHPadRadius);
    visitor("FHNumberOfPads", value.FHNumberOfPads);
    visitor("FHNumberOfProphets", value.FHNumberOfProphets);
    visitor("FHNumberOfPatterns", value.FHNumberOfPatterns);
    visitor("FHNumberOfRounds", value.FHNumberOfRounds);
    visitor("FHNumberOfSunsEvil", value.FHNumberOfSunsEvil);
    visitor("FHNumberOfSunsGood", value.FHNumberOfSunsGood);
    visitor("FHPatterns", value.FHPatterns);
    visitor("FHTimeLimit", value.FHTimeLimit);
    visitor("FHWarnTime", value.FHWarnTime);
    visitor("FHNumberOfVillagers", value.FHNumberOfVillagers);
    visitor("FHHealthChangeWhenZapped", value.FHHealthChangeWhenZapped);
    visitor("FHMoralityChangeOnKilled", value.FHMoralityChangeOnKilled);
    visitor("FHMoralityChangeOnFreed", value.FHMoralityChangeOnFreed);
    visitor("FHFreeAllPrisonersMoralityChange", value.FHFreeAllPrisonersMoralityChange);
    visitor("FHFreeAllPrisonersRenownBoost", value.FHFreeAllPrisonersRenownBoost);
    visitor("FHKillAllPrisonersMoralityChange", value.FHKillAllPrisonersMoralityChange);
    visitor("FHKillAllPrisonersRenownBoost", value.FHKillAllPrisonersRenownBoost);
    visitor("GSI_MazeCallsOverDistance", value.GSI_MazeCallsOverDistance);
    visitor("GSI_MazeRegionExitDistance", value.GSI_MazeRegionExitDistance);
    visitor("HS_ScreamerCount", value.HS_ScreamerCount);
    visitor("HS_ScreamersAtOnce", value.HS_ScreamersAtOnce);
    visitor("HS_ScreamerInc", value.HS_ScreamerInc);
    visitor("HS_SecondsBeforeSuperScreamerDamage", value.HS_SecondsBeforeSuperScreamerDamage);
    visitor("HS_SuperScreamerDamagePerSecond", value.HS_SuperScreamerDamagePerSecond);
    visitor("HS_SecondsBeforeNewSuperScreamer", value.HS_SecondsBeforeNewSuperScreamer);
    visitor("HS_TimePerUndeadKilled", value.HS_TimePerUndeadKilled);
    visitor("HS_MaxTimeNostroExposed", value.HS_MaxTimeNostroExposed);
    visitor("HS_GMShortDistance", value.HS_GMShortDistance);
    visitor("HS_GMLongDistance", value.HS_GMLongDistance);
    visitor("HS_GMGuardDistance", value.HS_GMGuardDistance);
    visitor("HS_GoodSoulTaken", value.HS_GoodSoulTaken);
    visitor("HC_SpawnCountPostReleaseGSon", value.HC_SpawnCountPostReleaseGSon);
    visitor("HC_ReduceCountPostSideRoom", value.HC_ReduceCountPostSideRoom);
    visitor("HC_SpawnGenerationRadius", value.HC_SpawnGenerationRadius);
    visitor("HC_BanditTimeBeforeComment", value.HC_BanditTimeBeforeComment);
    visitor("HC_BanditCallsForHelpDelay", value.HC_BanditCallsForHelpDelay);
    visitor("HC_BanditGoldDemand", value.HC_BanditGoldDemand);
    visitor("HC_BanditFadeOut", value.HC_BanditFadeOut);
    visitor("HC_HobbeSpawnDelay", value.HC_HobbeSpawnDelay);
    visitor("HC_BanditCommentTime", value.HC_BanditCommentTime);
    visitor("HC_BanditCloseToMarkerDistance", value.HC_BanditCloseToMarkerDistance);
    visitor("HC_HobbeGruntDelay", value.HC_HobbeGruntDelay);
    visitor("HC_HobbeSpawnCount", value.HC_HobbeSpawnCount);
    visitor("HC_RescueBanditMoralityGain", value.HC_RescueBanditMoralityGain);
    visitor("HC_RescueBanditExperienceGain", value.HC_RescueBanditExperienceGain);
    visitor("HC_SacrificeToNymphExperienceGain", value.HC_SacrificeToNymphExperienceGain);
    visitor("HC_SacrificeBanditMoralityLoss", value.HC_SacrificeBanditMoralityLoss);
    visitor("HC_SacrificeToNymphMoralityLoss", value.HC_SacrificeToNymphMoralityLoss);
    visitor("HC_LetBanditGoAndPayMoralityGain", value.HC_LetBanditGoAndPayMoralityGain);
    visitor("HC_LetBanditGoRenown", value.HC_LetBanditGoRenown);
    visitor("HC_DontPayBanditMoralityLoss", value.HC_DontPayBanditMoralityLoss);
    visitor("HC_KillNymphBoastCost", value.HC_KillNymphBoastCost);
    visitor("HC_KillNymphBoastReward", value.HC_KillNymphBoastReward);
    visitor("HC_SacrificeBoastCost", value.HC_SacrificeBoastCost);
    visitor("HC_SacrificeBoastReward", value.HC_SacrificeBoastReward);
    visitor("HC_BoyNoHarmBoastCost", value.HC_BoyNoHarmBoastCost);
    visitor("HC_BoyNoHarmBoastReward", value.HC_BoyNoHarmBoastReward);
    visitor("HC_HobbeMaxNumberInRegion", value.HC_HobbeMaxNumberInRegion);
    visitor("JackFightDefeatJackXP", value.JackFightDefeatJackXP);
    visitor("MB_MadBomberStartDistance", value.MB_MadBomberStartDistance);
    visitor("MB_MadBomberProximityToBomb", value.MB_MadBomberProximityToBomb);
    visitor("MB_MadBomberHeroCloseDelay", value.MB_MadBomberHeroCloseDelay);
    visitor("MB_MadBomberRunToWalkDistance", value.MB_MadBomberRunToWalkDistance);
    visitor("MB_BlacksmithCallHeroDistance", value.MB_BlacksmithCallHeroDistance);
    visitor("MB_BombInitialTimeToBoom", value.MB_BombInitialTimeToBoom);
    visitor("MB_FailMissionBombCount", value.MB_FailMissionBombCount);
    visitor("MB_BombGoesOffRenownLoss", value.MB_BombGoesOffRenownLoss);
    visitor("MB_BombDefusedExperience", value.MB_BombDefusedExperience);
    visitor("MB_BombDefusedMorality", value.MB_BombDefusedMorality);
    visitor("MCNumHitsBetweenForcePushes", value.MCNumHitsBetweenForcePushes);
    visitor("MCMinMinionSpawnPeriod", value.MCMinMinionSpawnPeriod);
    visitor("MCMaxMinionSpawnPeriod", value.MCMaxMinionSpawnPeriod);
    visitor("MCC_GuardTalkingDistance", value.MCC_GuardTalkingDistance);
    visitor("MCC_MinionRunDistance", value.MCC_MinionRunDistance);
    visitor("MCC_MinionBreakoffDistance", value.MCC_MinionBreakoffDistance);
    visitor("MCC_SavedGuardMoralityGain", value.MCC_SavedGuardMoralityGain);
    visitor("MCC_Timer", value.MCC_Timer);
    visitor("MCC_MinionCounter", value.MCC_MinionCounter);
    visitor("OVBRBanditSightRadius", value.OVBRBanditSightRadius);
    visitor("OVBRBanditSoundRadius", value.OVBRBanditSoundRadius);
    visitor("OVI_MoralityChangePerDeed", value.OVI_MoralityChangePerDeed);
    visitor("ExperienceForDefeatingWhisperInOrchardFarm", value.ExperienceForDefeatingWhisperInOrchardFarm);
    visitor("GuardReinforcementsYourTeam", value.GuardReinforcementsYourTeam);
    visitor("GuardReinforcementsEnemyTeam", value.GuardReinforcementsEnemyTeam);
    visitor("BanditReinforcementDelay", value.BanditReinforcementDelay);
    visitor("OFE_NoHealthPotionBoastCost", value.OFE_NoHealthPotionBoastCost);
    visitor("OFE_NoHealthPotionBoastReward", value.OFE_NoHealthPotionBoastReward);
    visitor("OFE_NoBanditsDieBoastCost", value.OFE_NoBanditsDieBoastCost);
    visitor("OFE_NoBanditsDieBoastReward", value.OFE_NoBanditsDieBoastReward);
    visitor("OFG_NoCratesStolenBoastCost", value.OFG_NoCratesStolenBoastCost);
    visitor("OFG_NoCratesStolenBoastReward", value.OFG_NoCratesStolenBoastReward);
    visitor("OFG_NoGuardsDieBoastCost", value.OFG_NoGuardsDieBoastCost);
    visitor("OFG_NoGuardsDieBoastReward", value.OFG_NoGuardsDieBoastReward);
    visitor("OFEvilCompletionMorality", value.OFEvilCompletionMorality);
    visitor("OFGoodCompletionMorality", value.OFGoodCompletionMorality);
    visitor("PrisonExerciseStartingTimeLimit", value.PrisonExerciseStartingTimeLimit);
    visitor("PrisonWayPointHitRange", value.PrisonWayPointHitRange);
    visitor("PrisonKeyRackDangerRange", value.PrisonKeyRackDangerRange);
    visitor("PrisonClothesChestDangerRange", value.PrisonClothesChestDangerRange);
    visitor("PrisonGuardProximityToAskQuestion", value.PrisonGuardProximityToAskQuestion);
    visitor("PrisonTrainingPartnerHintDistance", value.PrisonTrainingPartnerHintDistance);
    visitor("PrisonTrainingPartnerHintDistance2", value.PrisonTrainingPartnerHintDistance2);
    visitor("PrisonDistanceForGuardsToJeer", value.PrisonDistanceForGuardsToJeer);
    visitor("PrisonRadiusGuardWillAttackHeroWithin", value.PrisonRadiusGuardWillAttackHeroWithin);
    visitor("PrisonGuardMumbleAudioDistance", value.PrisonGuardMumbleAudioDistance);
    visitor("PrisonRacerRunSpeed", value.PrisonRacerRunSpeed);
    visitor("PrisonRacerSprintSpeed", value.PrisonRacerSprintSpeed);
    visitor("PrisonRacerSprintMaxTimeSeconds", value.PrisonRacerSprintMaxTimeSeconds);
    visitor("PrisonRacerSprintRestTimeSeconds", value.PrisonRacerSprintRestTimeSeconds);
    visitor("PrisonWardenGameSneakModifier", value.PrisonWardenGameSneakModifier);
    visitor("PrisonPoetryMaxLines", value.PrisonPoetryMaxLines);
    visitor("PrisonExperienceForKillingKraken", value.PrisonExperienceForKillingKraken);
    visitor("PrisonEscapeExperienceFromMother", value.PrisonEscapeExperienceFromMother);
    visitor("PrisonFreePrisonerMorality", value.PrisonFreePrisonerMorality);
    visitor("MotherFollowingHeroKrakenSafetyDistance", value.MotherFollowingHeroKrakenSafetyDistance);
    visitor("TE_TraderCommentDelay", value.TE_TraderCommentDelay);
    visitor("TE_TraderToBalverineDelay", value.TE_TraderToBalverineDelay);
    visitor("TE_TraderLowInfectionTime", value.TE_TraderLowInfectionTime);
    visitor("TE_TraderMediumInfectionTime", value.TE_TraderMediumInfectionTime);
    visitor("TE_TraderHighInfectionTime", value.TE_TraderHighInfectionTime);
    visitor("TE_AllTradersAliveBoastCost", value.TE_AllTradersAliveBoastCost);
    visitor("TE_AllTradersAliveBoastReward", value.TE_AllTradersAliveBoastReward);
    visitor("TE_BalverineTimeToGoDistance", value.TE_BalverineTimeToGoDistance);
    visitor("TE_TraderCommentDistance", value.TE_TraderCommentDistance);
    visitor("TE_AssassinSpawnDistance", value.TE_AssassinSpawnDistance);
    visitor("TE_AssassinName", value.TE_AssassinName);
    visitor("TE_EarthTrollTriggerDistance", value.TE_EarthTrollTriggerDistance);
    visitor("TE_TrollName", value.TE_TrollName);
    visitor("TE_TraderGoldReward", value.TE_TraderGoldReward);
    visitor("TE_LetTraderFollowMorality", value.TE_LetTraderFollowMorality);
    visitor("TraderEscortLeaveTraderMorality", value.TraderEscortLeaveTraderMorality);
    visitor("WB_MoralityGain", value.WB_MoralityGain);
    visitor("WB_LowHealth", value.WB_LowHealth);
    visitor("WB_GoldReward", value.WB_GoldReward);
    visitor("WB_CreaturesSpawned", value.WB_CreaturesSpawned);
    visitor("WB_WaspBossName", value.WB_WaspBossName);
    visitor("WB_ScreamingVillagerDistance", value.WB_ScreamingVillagerDistance);
    visitor("WB_ScreamingVillagerScreamsDistance", value.WB_ScreamingVillagerScreamsDistance);
    visitor("WB_ScreamingVillagerFadeOutTime", value.WB_ScreamingVillagerFadeOutTime);
    visitor("WB_WaspHelperCallsOutDistance", value.WB_WaspHelperCallsOutDistance);
    visitor("WB_WaspChaseWomanPanicTime", value.WB_WaspChaseWomanPanicTime);
    visitor("WB_ExperienceReward", value.WB_ExperienceReward);
    visitor("BC_EvilMoralityLoss", value.BC_EvilMoralityLoss);
    visitor("BC_GoodMoralityGain", value.BC_GoodMoralityGain);
    visitor("BAC_RenownAward", value.BAC_RenownAward);
    visitor("BAC_HostageMoralityGain", value.BAC_HostageMoralityGain);
    visitor("BAC_SingleHostageMoralityLoss", value.BAC_SingleHostageMoralityLoss);
    visitor("BAC_DoubleHostageMoralityLoss", value.BAC_DoubleHostageMoralityLoss);
    visitor("BAC_AgeAmount", value.BAC_AgeAmount);
    visitor("BAC_EnemiesToKillInFirstArea", value.BAC_EnemiesToKillInFirstArea);
    visitor("BAC_EnemiesToKillInSecondArea", value.BAC_EnemiesToKillInSecondArea);
    visitor("BAC_AssassinGoldAmount", value.BAC_AssassinGoldAmount);
    visitor("BAC_ForgerGoldAmount", value.BAC_ForgerGoldAmount);
    visitor("BAC_BoastBanditKill", value.BAC_BoastBanditKill);
    visitor("BAC_TheresaExperienceGift", value.BAC_TheresaExperienceGift);
    visitor("BAC_TwinBladeKilledMorality", value.BAC_TwinBladeKilledMorality);
    visitor("BAC_TwinBladeSavedMorality", value.BAC_TwinBladeSavedMorality);
    visitor("ENF_TotalTime", value.ENF_TotalTime);
    visitor("ENF_TimeAddition", value.ENF_TimeAddition);
    visitor("ENF_TimeSubtraction", value.ENF_TimeSubtraction);
    visitor("EGJ_MoralityLoss", value.EGJ_MoralityLoss);
    visitor("EGJ_MoralityGain", value.EGJ_MoralityGain);
    visitor("OGSP_KillAllBoastCost", value.OGSP_KillAllBoastCost);
    visitor("OGSP_KillAllBoastReward", value.OGSP_KillAllBoastReward);
    visitor("GUI_MeleeGrades", value.GUI_MeleeGrades);
    visitor("GUI_SkillGrades", value.GUI_SkillGrades);
    visitor("GUI_WillGrades", value.GUI_WillGrades);
    visitor("GUI_MinHealth", value.GUI_MinHealth);
    visitor("GUI_RaceTime", value.GUI_RaceTime);
    visitor("GUI_RaceGold", value.GUI_RaceGold);
    visitor("GUI_RearDummySegements", value.GUI_RearDummySegements);
    visitor("GUI_MiddleDummySegements", value.GUI_MiddleDummySegements);
    visitor("GUI_FrontDummySegements", value.GUI_FrontDummySegements);
    visitor("GUI_RearDummyWorth", value.GUI_RearDummyWorth);
    visitor("GUI_MiddleDummyWorth", value.GUI_MiddleDummyWorth);
    visitor("GUI_FrontDummyWorth", value.GUI_FrontDummyWorth);
    visitor("GUI_GoldPerBird", value.GUI_GoldPerBird);
    visitor("GUI_BirdGoldBonus", value.GUI_BirdGoldBonus);
    visitor("GUI_SkillTimer", value.GUI_SkillTimer);
    visitor("GUI_WillTimer", value.GUI_WillTimer);
    visitor("GUI_DepartureBeetles", value.GUI_DepartureBeetles);
    visitor("GUI_MeleeBeetles", value.GUI_MeleeBeetles);
    visitor("GUI_LampCost", value.GUI_LampCost);
    visitor("GUI_WillDummySpinTimer", value.GUI_WillDummySpinTimer);
    visitor("GUI_EndGuildXP", value.GUI_EndGuildXP);
    visitor("HOB_TotalTime", value.HOB_TotalTime);
    visitor("HOB_BoastHobbeMany", value.HOB_BoastHobbeMany);
    visitor("HOB_RenownAwardSuccess", value.HOB_RenownAwardSuccess);
    visitor("HOB_RenownAwardDraw", value.HOB_RenownAwardDraw);
    visitor("HOB_RenownAwardLost", value.HOB_RenownAwardLost);
    visitor("HOB_KillMoreHobbesBoastCost", value.HOB_KillMoreHobbesBoastCost);
    visitor("HOB_KillMoreHobbesBoastReward", value.HOB_KillMoreHobbesBoastReward);
    visitor("HangingTreeProtectGuardsBoastReward", value.HangingTreeProtectGuardsBoastReward);
    visitor("HangingTreeProtectGuardsBoastAmount", value.HangingTreeProtectGuardsBoastAmount);
    visitor("HTGoodCompletionMorality", value.HTGoodCompletionMorality);
    visitor("HTGoodHeroKilledPrisonerGoldReward", value.HTGoodHeroKilledPrisonerGoldReward);
    visitor("HTGoodHeroKilledPrisonerRenownReward", value.HTGoodHeroKilledPrisonerRenownReward);
    visitor("HangingTreeProtectBanditsBoastReward", value.HangingTreeProtectBanditsBoastReward);
    visitor("HangingTreeProtectBanditsBoastAmount", value.HangingTreeProtectBanditsBoastAmount);
    visitor("HangingTreeEvilMissionTimer", value.HangingTreeEvilMissionTimer);
    visitor("HTEvilCompletionMorality", value.HTEvilCompletionMorality);
    visitor("HOH_MoralityGain", value.HOH_MoralityGain);
    visitor("HOH_MoralityLoss", value.HOH_MoralityLoss);
    visitor("RV_AcceptedGuardLoweredGoldReward", value.RV_AcceptedGuardLoweredGoldReward);
    visitor("RV_AcceptedGuardLoweredRenownReward", value.RV_AcceptedGuardLoweredRenownReward);
    visitor("RV_LowerGoldRewardFromKillingChiefsGuard", value.RV_LowerGoldRewardFromKillingChiefsGuard);
    visitor("RV_HeroEvilMoralityValue", value.RV_HeroEvilMoralityValue);
    visitor("RV_HeroGoodMoralityValue", value.RV_HeroGoodMoralityValue);
    visitor("RV_HeroTellMoralityValue", value.RV_HeroTellMoralityValue);
    visitor("TCE_TimeLimit", value.TCE_TimeLimit);
    visitor("TCG_TimeLimit", value.TCG_TimeLimit);
    visitor("TCE_GuardRangeHighest", value.TCE_GuardRangeHighest);
    visitor("TCE_GuardRangeHigh", value.TCE_GuardRangeHigh);
    visitor("TCE_GuardRangeLow", value.TCE_GuardRangeLow);
    visitor("WBK_NumberHits", value.WBK_NumberHits);
    visitor("WBK_RenownAwardForGates", value.WBK_RenownAwardForGates);
    visitor("WBK_RenownAwardForFirstDefence", value.WBK_RenownAwardForFirstDefence);
    visitor("WBK_RenownAwardForSecondDefence", value.WBK_RenownAwardForSecondDefence);
    visitor("WizardBattleDefeatMazeXP", value.WizardBattleDefeatMazeXP);
    visitor("CHK_LowPrize", value.CHK_LowPrize);
    visitor("CHK_MidPrize", value.CHK_MidPrize);
    visitor("CHK_HighPrize", value.CHK_HighPrize);
    visitor("TeleporterActivationDist", value.TeleporterActivationDist);
    visitor("TimeAdvancePointFadeOutTime", value.TimeAdvancePointFadeOutTime);
    visitor("TimeAdvancePointFadeInTime", value.TimeAdvancePointFadeInTime);
    visitor("TimeAdvancePointFadedOutPauseTime", value.TimeAdvancePointFadedOutPauseTime);
    visitor("TimeAdvancePointTimeToAdvance", value.TimeAdvancePointTimeToAdvance);
    visitor("CoreQuestReminderIntervalSeconds", value.CoreQuestReminderIntervalSeconds);
    visitor("FlourishReminderFrequency", value.FlourishReminderFrequency);
    visitor("PercentageFlourishesTakenLowerBound", value.PercentageFlourishesTakenLowerBound);
    visitor("StopTeachingFlourishesAfterDoneThisMany", value.StopTeachingFlourishesAfterDoneThisMany);
    visitor("CombatMultTooLongUnchanged", value.CombatMultTooLongUnchanged);
    visitor("CombatMultStopBotheringAfter", value.CombatMultStopBotheringAfter);
    visitor("LowHealthWarning", value.LowHealthWarning);
    visitor("VeryLowHealthWarning", value.VeryLowHealthWarning);
    visitor("LowHealthMessageInterval", value.LowHealthMessageInterval);
    visitor("VeryLowHealthMessageInterval", value.VeryLowHealthMessageInterval);
    visitor("HealthMessageLimit", value.HealthMessageLimit);
    visitor("LowWillEnergyWarningLevel", value.LowWillEnergyWarningLevel);
    visitor("LowWillEnergyMessageInterval", value.LowWillEnergyMessageInterval);
    visitor("WillEnergyMessageLimit", value.WillEnergyMessageLimit);
    visitor("SaveMessageLimit", value.SaveMessageLimit);
    visitor("SaveMessageInterval", value.SaveMessageInterval);
    visitor("FishingMaxDistanceFromSpot", value.FishingMaxDistanceFromSpot);
    visitor("DiggingMaxDistanceFromSpot", value.DiggingMaxDistanceFromSpot);
    visitor("PickpocketDurationSeconds", value.PickpocketDurationSeconds);
    visitor("PickpocketSpottedDurationSeconds", value.PickpocketSpottedDurationSeconds);
    visitor("PickpocketSpottedChancePerSecond", value.PickpocketSpottedChancePerSecond);
    visitor("PicklockDurationSeconds", value.PicklockDurationSeconds);
    visitor("PicklockSpottedDurationSeconds", value.PicklockSpottedDurationSeconds);
    visitor("PicklockSpottedChancePerSecond", value.PicklockSpottedChancePerSecond);
    visitor("StealDurationSeconds", value.StealDurationSeconds);
    visitor("StealSpottedDurationSeconds", value.StealSpottedDurationSeconds);
    visitor("StealSpottedChancePerSecond", value.StealSpottedChancePerSecond);
    visitor("NumberOfConcurrentlyAvailableFeats", value.NumberOfConcurrentlyAvailableFeats);
    visitor("Feats", value.Feats);
    visitor("ArenaRounds", value.ArenaRounds);
    visitor("HeroSoulsRounds", value.HeroSoulsRounds);
    visitor("DemonDoor_CombatMult_TriggerLevel", value.DemonDoor_CombatMult_TriggerLevel);
    visitor("DemonDoor_HighRenown_TriggerLevel", value.DemonDoor_HighRenown_TriggerLevel);
    visitor("DemonDoor_ShineLight_TriggerDistance", value.DemonDoor_ShineLight_TriggerDistance);
    visitor("DemonDoor_SummonCreature_NumCreatureTypes", value.DemonDoor_SummonCreature_NumCreatureTypes);
    visitor("DemonDoor_SummonCreature_CreatureType", value.DemonDoor_SummonCreature_CreatureType);
    visitor("DemonDoor_BeEvil_TriggerDistance", value.DemonDoor_BeEvil_TriggerDistance);
    visitor("DemonDoor_BeEvil_EvilnessNeeded", value.DemonDoor_BeEvil_EvilnessNeeded);
    visitor("DemonDoor_DoDamage_DamageLimit", value.DemonDoor_DoDamage_DamageLimit);
    visitor("DemonDoor_EatPies_FatnessLevel", value.DemonDoor_EatPies_FatnessLevel);
}

template <class V>
void visit_fields(CShipDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SightingFrequency", value.SightingFrequency);
    visitor("SightingOffset", value.SightingOffset);
    visitor("DaysVisible", value.DaysVisible);
    visitor("Swell", value.Swell);
    visitor("Pitch", value.Pitch);
    visitor("Roll", value.Roll);
    visitor("WaveLength", value.WaveLength);
}

template <class V>
void visit_fields(CShopItem &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Quantity", value.Quantity);
    visitor("PriceMultiplier", value.PriceMultiplier);
    visitor("MaxStock", value.MaxStock);
    visitor("RestockPeriod", value.RestockPeriod);
    visitor("RestockPeriodRandomness", value.RestockPeriodRandomness);
    visitor("SalesPerDay", value.SalesPerDay);
    visitor("SalesPerDayRandomness", value.SalesPerDayRandomness);
}

template <class V>
void visit_fields(CShopItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("Item", value.Item);
}

template <class V>
void visit_fields(CShopDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Name", value.Name);
    visitor("InitialHawkingPhrase", value.InitialHawkingPhrase);
    visitor("ReplyToRequestPhrase", value.ReplyToRequestPhrase);
    visitor("CustomerFollowPhrase", value.CustomerFollowPhrase);
    visitor("ThankingPhrase", value.ThankingPhrase);
    visitor("ShopkeeperMeshType", value.ShopkeeperMeshType);
    visitor("DefaultStock", value.DefaultStock);
    visitor("StockCategoryDefaults", value.StockCategoryDefaults);
    visitor("BuyPriceMultiplier", value.BuyPriceMultiplier);
    visitor("SellPriceMultiplier", value.SellPriceMultiplier);
    visitor("AttitudeBuyPriceMultiplierDefault", value.AttitudeBuyPriceMultiplierDefault);
    visitor("AttitudeBuyPriceMultiplier", value.AttitudeBuyPriceMultiplier);
    visitor("AttitudeSellPriceMultiplierDefault", value.AttitudeSellPriceMultiplierDefault);
    visitor("AttitudeSellPriceMultiplier", value.AttitudeSellPriceMultiplier);
    visitor("MaxStockChangePerSell", value.MaxStockChangePerSell);
    visitor("MaxStockChangePerBuy", value.MaxStockChangePerBuy);
    visitor("MaxStockRevertPerDay", value.MaxStockRevertPerDay);
    visitor("PriceReductionFractionPerItem", value.PriceReductionFractionPerItem);
    visitor("MaxPriceReductionFractionDueToStockCount", value.MaxPriceReductionFractionDueToStockCount);
    visitor("PreferredShopkeeper", value.PreferredShopkeeper);
    visitor("IsStockDisplayPermanent", value.IsStockDisplayPermanent);
    visitor("IsBarberShop", value.IsBarberShop);
    visitor("IsTattooShop", value.IsTattooShop);
    visitor("IsTitleShop", value.IsTitleShop);
    visitor("AlwaysOpen", value.AlwaysOpen);
    visitor("MinPriceMultiplierForItemToBeWanted", value.MinPriceMultiplierForItemToBeWanted);
    visitor("MaxPriceToDisplay", value.MaxPriceToDisplay);
    visitor("MinTimeToSteal", value.MinTimeToSteal);
    visitor("MaxTimeToSteal", value.MaxTimeToSteal);
    visitor("MinGoldForDonation", value.MinGoldForDonation);
    visitor("MaxGoldForDonation", value.MaxGoldForDonation);
    visitor("MaxMoralityForDonation", value.MaxMoralityForDonation);
    visitor("LogarithmicPower", value.LogarithmicPower);
    visitor("NumDaysToKeepSpecialItem", value.NumDaysToKeepSpecialItem);
    visitor("MinAmountBeforeSpecialItem", value.MinAmountBeforeSpecialItem);
    visitor("ProbabilityOfSpecialItemPerDay", value.ProbabilityOfSpecialItemPerDay);
    visitor("SpecialItemPriceMult", value.SpecialItemPriceMult);
}

template <class V>
void visit_fields(CShoveHaPennyRegionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Start", value.Start);
    visitor("Score", value.Score);
}

template <class V>
void visit_fields(CSimBuildingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Property", value.Property);
    visitor("WorkSpaces", value.WorkSpaces);
    visitor("LiveSpaces", value.LiveSpaces);
}

template <class V>
void visit_fields(CSimVoice &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Voices", value.Voices);
}

template <class V>
void visit_fields(CSimVoicesDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Entry", value.Entry);
}

template <class V>
void visit_fields(CSkeletalMorphDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SkeletalMorphs", value.SkeletalMorphs);
}

template <class V>
void visit_fields(CSkyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SunRadius", value.SunRadius);
    visitor("SunTexture", value.SunTexture);
    visitor("MoonRadius", value.MoonRadius);
    visitor("MoonTexture", value.MoonTexture);
    visitor("SunFlareRadius", value.SunFlareRadius);
    visitor("SunFlareTexture", value.SunFlareTexture);
    visitor("StarTexture", value.StarTexture);
    visitor("StarChartTextureSize", value.StarChartTextureSize);
    visitor("StarSize", value.StarSize);
    visitor("StarChartFilter", value.StarChartFilter);
    visitor("TwinkleInterval", value.TwinkleInterval);
    visitor("TwinkleSpeed", value.TwinkleSpeed);
    visitor("TwinkleMin", value.TwinkleMin);
    visitor("TwinkleMax", value.TwinkleMax);
    visitor("FlareElements", value.FlareElements);
}

template <class V>
void visit_fields(CSmashableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Smashable", value.Smashable);
    visitor("ReplacementObject", value.ReplacementObject);
    visitor("SmashParticleEmitter", value.SmashParticleEmitter);
}

template <class V>
void visit_fields(CSmokeGeneratorDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GenerationDistance", value.GenerationDistance);
    visitor("ParticleEmitter", value.ParticleEmitter);
}

template <class V>
void visit_fields(CSnowTrollDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IcicleAttackIndex", value.IcicleAttackIndex);
    visitor("SecondsIcicleAttackTargetPositionPredictionTime", value.SecondsIcicleAttackTargetPositionPredictionTime);
    visitor("DistancePoundGroundRange", value.DistancePoundGroundRange);
    visitor("SecondsForIcicleAssault", value.SecondsForIcicleAssault);
    visitor("SecondsLeadIntoIcicles", value.SecondsLeadIntoIcicles);
    visitor("SecondsBetweenIcicles", value.SecondsBetweenIcicles);
    visitor("SecondsBetweenWarningFXAndTrigger", value.SecondsBetweenWarningFXAndTrigger);
    visitor("SecondsForTrapToRemain", value.SecondsForTrapToRemain);
    visitor("DistanceRange", value.DistanceRange);
    visitor("DistanceAroundTarget", value.DistanceAroundTarget);
    visitor("DistanceExclusionAroundIcicles", value.DistanceExclusionAroundIcicles);
    visitor("IcicleAssaultWarningFXIndex", value.IcicleAssaultWarningFXIndex);
    visitor("IcicleAssaultTrapIndex", value.IcicleAssaultTrapIndex);
    visitor("IceTrailDistanceLimit", value.IceTrailDistanceLimit);
}

template <class V>
void visit_fields(CSoundAtmospheresDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SoundAtmospheres", value.SoundAtmospheres);
}

template <class V>
void visit_fields(CSoundBankEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ID", value.ID);
    visitor("Filename", value.Filename);
    visitor("SymbolFilename", value.SymbolFilename);
    visitor("LipSyncBankHandle", value.LipSyncBankHandle);
    visitor("DVD", value.DVD);
}

template <class V>
void visit_fields(CSoundDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SoundBankEntries", value.SoundBankEntries);
    visitor("AtmosBankEntries", value.AtmosBankEntries);
    visitor("MusicEntries", value.MusicEntries);
    visitor("MusicSetEntries", value.MusicSetEntries);
    visitor("ReverbEnvironmentEntries", value.ReverbEnvironmentEntries);
    visitor("AtmosListenerDampingMin", value.AtmosListenerDampingMin);
    visitor("AtmosListenerDampingRange", value.AtmosListenerDampingRange);
    visitor("DistanceModelCurve", value.DistanceModelCurve);
}

template <class V>
void visit_fields(CSoundThemeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("EditColour", value.EditColour);
    visitor("AtmosIndex", value.AtmosIndex);
    visitor("MusicSet", value.MusicSet);
    visitor("Gain", value.Gain);
    visitor("Range", value.Range);
    visitor("Granularity", value.Granularity);
    visitor("AtmosGroupMap", value.AtmosGroupMap);
    visitor("PlayOnlyWhenInside", value.PlayOnlyWhenInside);
    visitor("PlayQuietlyWhenInside", value.PlayQuietlyWhenInside);
    visitor("PlayOnlyWhenOutside", value.PlayOnlyWhenOutside);
    visitor("PlayQuietlyWhenOutside", value.PlayQuietlyWhenOutside);
}

template <class V>
void visit_fields(CSoundThemeDef::CAtmosGroup &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GroupName", value.GroupName);
    visitor("GroupID", value.GroupID);
}

template <class V>
void visit_fields(CSpecialAbilitiesAssassinRushDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DistanceAwayFromtargetToAssassinRushTo", value.DistanceAwayFromtargetToAssassinRushTo);
    visitor("SafeCollideRadiusPercentage", value.SafeCollideRadiusPercentage);
    visitor("DistFromFloor", value.DistFromFloor);
    visitor("AssassinStartSeconds", value.AssassinStartSeconds);
    visitor("AssassinEndSeconds", value.AssassinEndSeconds);
    visitor("AssassinThreshold", value.AssassinThreshold);
    visitor("AssassinLength", value.AssassinLength);
    visitor("AssassinIntensity", value.AssassinIntensity);
    visitor("AssassinFade", value.AssassinFade);
    visitor("AssassinRange", value.AssassinRange);
    visitor("AssassinTwist", value.AssassinTwist);
    visitor("RunSpeed", value.RunSpeed);
    visitor("TrailDelaySecs", value.TrailDelaySecs);
    visitor("SlowTimeLength", value.SlowTimeLength);
    visitor("AssassinAlpha", value.AssassinAlpha);
    visitor("SlowTimeMultiplier", value.SlowTimeMultiplier);
    visitor("RushDelay", value.RushDelay);
    visitor("DashDistance", value.DashDistance);
}

template <class V>
void visit_fields(CSpecialAbilitiesBattleChargeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("RunSpeed", value.RunSpeed);
    visitor("DashDistance", value.DashDistance);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
    visitor("BattleChargeDamage", value.BattleChargeDamage);
    visitor("BattleChargeCombatMultiplierFactor", value.BattleChargeCombatMultiplierFactor);
    visitor("BattleChargeRadialStartSeconds", value.BattleChargeRadialStartSeconds);
    visitor("BattleChargeRadialEndSeconds", value.BattleChargeRadialEndSeconds);
    visitor("BattleChargeRadialThreshold", value.BattleChargeRadialThreshold);
    visitor("BattleChargeRadialLength", value.BattleChargeRadialLength);
    visitor("BattleChargeRadialIntensity", value.BattleChargeRadialIntensity);
    visitor("BattleChargeRadialFade", value.BattleChargeRadialFade);
    visitor("BattleChargeRadialRange", value.BattleChargeRadialRange);
    visitor("BattleChargeRadialTwist", value.BattleChargeRadialTwist);
    visitor("PoseDelaySecs", value.PoseDelaySecs);
    visitor("ChargeDelay", value.ChargeDelay);
    visitor("NumPeopleHit", value.NumPeopleHit);
    visitor("DragonExtraGumphRadiusMultiplier", value.DragonExtraGumphRadiusMultiplier);
    visitor("DragonExtraGumphRadius", value.DragonExtraGumphRadius);
}

template <class V>
void visit_fields(CSpecialAbilitiesBerserkDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SpeedMultiplier", value.SpeedMultiplier);
    visitor("FramesToFadeOut", value.FramesToFadeOut);
    visitor("LowerBerserkPercent", value.LowerBerserkPercent);
    visitor("TimeForThemeTransition", value.TimeForThemeTransition);
    visitor("BlurCenterFromFloor", value.BlurCenterFromFloor);
    visitor("BerserkThreshold", value.BerserkThreshold);
    visitor("BerserkLength", value.BerserkLength);
    visitor("BerserkIntensity", value.BerserkIntensity);
    visitor("BerserkFade", value.BerserkFade);
    visitor("BerserkRange", value.BerserkRange);
    visitor("BerserkTwist", value.BerserkTwist);
    visitor("TimeForRadialBlurToFadeOut", value.TimeForRadialBlurToFadeOut);
    visitor("CameraStartDist", value.CameraStartDist);
    visitor("CameraEndDist", value.CameraEndDist);
    visitor("CameraStartDeltaHeight", value.CameraStartDeltaHeight);
    visitor("CameraEndDeltaHeight", value.CameraEndDeltaHeight);
    visitor("BerserkDuration", value.BerserkDuration);
    visitor("BerserkDamageToEnemyMultiplier", value.BerserkDamageToEnemyMultiplier);
    visitor("BerserkDamageToEnemyMultiplierAdditionPerHit", value.BerserkDamageToEnemyMultiplierAdditionPerHit);
    visitor("BerserkDamageToMeMultiplier", value.BerserkDamageToMeMultiplier);
    visitor("MaxIncreaseInStrength", value.MaxIncreaseInStrength);
    visitor("BerserkScale", value.BerserkScale);
    visitor("BerserkBlastRadius", value.BerserkBlastRadius);
    visitor("CamDist", value.CamDist);
    visitor("CamHeight", value.CamHeight);
    visitor("CamFOV", value.CamFOV);
    visitor("CamSwitchTime", value.CamSwitchTime);
    visitor("RumbleLevel", value.RumbleLevel);
    visitor("RumbleSmoothness", value.RumbleSmoothness);
    visitor("RumbleAttack", value.RumbleAttack);
    visitor("RumbleDecay", value.RumbleDecay);
    visitor("RumbleReleaseLevelDelta", value.RumbleReleaseLevelDelta);
    visitor("RumbleReleaseSmoothnessDelta", value.RumbleReleaseSmoothnessDelta);
    visitor("RumbleReleaseAttackDelta", value.RumbleReleaseAttackDelta);
    visitor("RumbleReleaseDecayDelta", value.RumbleReleaseDecayDelta);
    visitor("CrazyActionRadius", value.CrazyActionRadius);
    visitor("CrazyActionDamage", value.CrazyActionDamage);
    visitor("DecapitateMaxProb", value.DecapitateMaxProb);
    visitor("TargetNearestEnemyDistance", value.TargetNearestEnemyDistance);
    visitor("WeaponTrailAttack", value.WeaponTrailAttack);
    visitor("WeaponTrailKnockDown", value.WeaponTrailKnockDown);
    visitor("LevelUpTimings", value.LevelUpTimings);
    visitor("TintColorR", value.TintColorR);
    visitor("TintColorG", value.TintColorG);
    visitor("TintColorB", value.TintColorB);
    visitor("CanBlockWhileBerserked", value.CanBlockWhileBerserked);
    visitor("CrazyIdleIntervalSecs", value.CrazyIdleIntervalSecs);
}

template <class V>
void visit_fields(CSpecialAbilitiesBulletTimeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SlowTimeDuration", value.SlowTimeDuration);
    visitor("BulletTimeSlowdownMultiplier", value.BulletTimeSlowdownMultiplier);
    visitor("LevelUpTimings", value.LevelUpTimings);
    visitor("CamDist", value.CamDist);
    visitor("CamHeight", value.CamHeight);
    visitor("CamFOV", value.CamFOV);
    visitor("CamSwitchTime", value.CamSwitchTime);
    visitor("CastCamDist", value.CastCamDist);
    visitor("CastCamHeight", value.CastCamHeight);
    visitor("CastCamFOV", value.CastCamFOV);
    visitor("CastCamSwitchTime", value.CastCamSwitchTime);
    visitor("ChargeSound", value.ChargeSound);
    visitor("PowerUp1Sound", value.PowerUp1Sound);
    visitor("PowerUp2Sound", value.PowerUp2Sound);
    visitor("PowerUp3Sound", value.PowerUp3Sound);
    visitor("InitSound", value.InitSound);
    visitor("LoopSound", value.LoopSound);
    visitor("EndSound", value.EndSound);
    visitor("RadialBlurThresholdCol", value.RadialBlurThresholdCol);
    visitor("RadialBlurLength", value.RadialBlurLength);
    visitor("RadialBlurIntensityCol", value.RadialBlurIntensityCol);
    visitor("RadialBlurFade", value.RadialBlurFade);
    visitor("RadialBlurRange", value.RadialBlurRange);
    visitor("RadialBlurTwist", value.RadialBlurTwist);
    visitor("RadialBlurFadeTimeSecs", value.RadialBlurFadeTimeSecs);
    visitor("DamageRadialBlurThresholdCol", value.DamageRadialBlurThresholdCol);
    visitor("DamageRadialBlurLength", value.DamageRadialBlurLength);
    visitor("DamageRadialBlurIntensityCol", value.DamageRadialBlurIntensityCol);
    visitor("DamageRadialBlurFade", value.DamageRadialBlurFade);
    visitor("DamageRadialBlurRange", value.DamageRadialBlurRange);
    visitor("DamageRadialBlurTwist", value.DamageRadialBlurTwist);
    visitor("DamageRadialBlurFadeTimeSecs", value.DamageRadialBlurFadeTimeSecs);
    visitor("HeroCombatSlowness", value.HeroCombatSlowness);
    visitor("ScreenFilterAlpha", value.ScreenFilterAlpha);
    visitor("ScreenFilterSaturation", value.ScreenFilterSaturation);
    visitor("ScreenFilterContrast", value.ScreenFilterContrast);
    visitor("ScreenFilterBrightness", value.ScreenFilterBrightness);
    visitor("ScreenFilterTintR", value.ScreenFilterTintR);
    visitor("ScreenFilterTintG", value.ScreenFilterTintG);
    visitor("ScreenFilterTintB", value.ScreenFilterTintB);
    visitor("ScreenFilterFadeInSecs", value.ScreenFilterFadeInSecs);
    visitor("ScreenFilterFadeOutSecs", value.ScreenFilterFadeOutSecs);
}

template <class V>
void visit_fields(CSpecialAbilitiesBurntEffectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FadeInTimeSecs", value.FadeInTimeSecs);
    visitor("FadeOutTimeStartSecs", value.FadeOutTimeStartSecs);
    visitor("FadeOutTimeSecs", value.FadeOutTimeSecs);
    visitor("Color", value.Color);
}

template <class V>
void visit_fields(CSpecialAbilitiesCreatureTintDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FadeInTimeSecs", value.FadeInTimeSecs);
    visitor("FadeOutTimeSecs", value.FadeOutTimeSecs);
    visitor("Color", value.Color);
}

template <class V>
void visit_fields(CSpecialAbilitiesDivineWrathDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StartPowerupSound", value.StartPowerupSound);
    visitor("LoopingPowerupSound", value.LoopingPowerupSound);
    visitor("ReleaseSound", value.ReleaseSound);
    visitor("ReleaseLoopSound", value.ReleaseLoopSound);
    visitor("FinishSound", value.FinishSound);
    visitor("CancelSound", value.CancelSound);
    visitor("RandomFactorForNumSecsBetweenDamageHit", value.RandomFactorForNumSecsBetweenDamageHit);
    visitor("NumSecsBetweenDamageHit", value.NumSecsBetweenDamageHit);
    visitor("DamagePerHit", value.DamagePerHit);
    visitor("DurationOfSpellSecs", value.DurationOfSpellSecs);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
    visitor("EnvChangeSecs", value.EnvChangeSecs);
    visitor("EnvReturnSecs", value.EnvReturnSecs);
    visitor("DoLifting", value.DoLifting);
    visitor("VelocityOfLift", value.VelocityOfLift);
    visitor("DistanceToLift", value.DistanceToLift);
    visitor("StaminaCostPerSec", value.StaminaCostPerSec);
    visitor("DamageTakenMultiplier", value.DamageTakenMultiplier);
    visitor("SmallRingRadius", value.SmallRingRadius);
    visitor("LargeRingRadius", value.LargeRingRadius);
}

template <class V>
void visit_fields(CSpecialAbilitiesDrainLifeDataDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Damage", value.Damage);
}

template <class V>
void visit_fields(CSpecialAbilitiesDrainLifeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AmountOfHealthSucked", value.AmountOfHealthSucked);
    visitor("NumEffects", value.NumEffects);
    visitor("SpellRange", value.SpellRange);
    visitor("ShotName", value.ShotName);
    visitor("SpellDelaySecs", value.SpellDelaySecs);
}

template <class V>
void visit_fields(CSpecialAbilitiesDrunkennessDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DrunkenGraphic", value.DrunkenGraphic);
    visitor("MaxAlpha", value.MaxAlpha);
    visitor("USpeed", value.USpeed);
    visitor("VSpeed", value.VSpeed);
    visitor("RotSpeed", value.RotSpeed);
}

template <class V>
void visit_fields(CSpecialAbilitiesElectrocutedEffectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FadeInTimeSecs", value.FadeInTimeSecs);
    visitor("FadeOutTimeStartSecs", value.FadeOutTimeStartSecs);
    visitor("FadeOutTimeSecs", value.FadeOutTimeSecs);
    visitor("Color", value.Color);
    visitor("HighlightColor", value.HighlightColor);
    visitor("HighlightWidth", value.HighlightWidth);
    visitor("HeightSmall", value.HeightSmall);
    visitor("HeightMedium", value.HeightMedium);
}

template <class V>
void visit_fields(CSpecialAbilitiesEnflameDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CamDist", value.CamDist);
    visitor("CamHeight", value.CamHeight);
    visitor("CamFOV", value.CamFOV);
    visitor("CamSwitchTime", value.CamSwitchTime);
    visitor("LevelUpTimings", value.LevelUpTimings);
    visitor("Damage", value.Damage);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
    visitor("EnflameCombatMultiplierFactor", value.EnflameCombatMultiplierFactor);
}

template <class V>
void visit_fields(CSpecialAbilitiesFireballSpellDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ReleaseDelay", value.ReleaseDelay);
    visitor("Inclination", value.Inclination);
    visitor("StaminaCost", value.StaminaCost);
    visitor("TargettingArcAngle", value.TargettingArcAngle);
}

template <class V>
void visit_fields(CSpecialAbilitiesForcePushDataDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Damage", value.Damage);
}

template <class V>
void visit_fields(CSpecialAbilitiesForcePushDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MaxDistanceToPush", value.MaxDistanceToPush);
    visitor("MinDistanceToPush", value.MinDistanceToPush);
    visitor("DustBoxWidth", value.DustBoxWidth);
    visitor("MaxNumOfExtraEffects", value.MaxNumOfExtraEffects);
    visitor("MinNumOfExtraEffects", value.MinNumOfExtraEffects);
    visitor("LevelUpTimings", value.LevelUpTimings);
    visitor("ReleaseLevel1Delay", value.ReleaseLevel1Delay);
    visitor("ReleaseLevel2Delay", value.ReleaseLevel2Delay);
    visitor("ReleaseLevel3DelayA", value.ReleaseLevel3DelayA);
    visitor("ReleaseLevel3DelayB", value.ReleaseLevel3DelayB);
    visitor("ReleaseLevel4Delay", value.ReleaseLevel4Delay);
    visitor("HeightOffGround", value.HeightOffGround);
    visitor("Level4OffsetMagnitude", value.Level4OffsetMagnitude);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
    visitor("ExtraPush", value.ExtraPush);
    visitor("PushSpeed", value.PushSpeed);
    visitor("ObjectDamage", value.ObjectDamage);
    visitor("TimeDelayBetweenCastsSec", value.TimeDelayBetweenCastsSec);
}

template <class V>
void visit_fields(CSpecialAbilitiesGhostSwordDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TimeOfSummon", value.TimeOfSummon);
    visitor("FadeInTime", value.FadeInTime);
    visitor("FadeOutTime", value.FadeOutTime);
    visitor("CreationDistance", value.CreationDistance);
}

template <class V>
void visit_fields(CSpecialAbilitiesHealLifeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("HealthIncreasePerFrame", value.HealthIncreasePerFrame);
    visitor("TotalHealthIncrease", value.TotalHealthIncrease);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
}

template <class V>
void visit_fields(CSpecialAbilitiesLightningSpellDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("m_faAreaEffect", value.m_faAreaEffect);
    visitor("m_faDamagePerHit", value.m_faDamagePerHit);
    visitor("m_faSubBeamDamagePerHit", value.m_faSubBeamDamagePerHit);
    visitor("m_faActivationTime", value.m_faActivationTime);
    visitor("m_faDeActivationTime", value.m_faDeActivationTime);
    visitor("m_naMinMainBeams", value.m_naMinMainBeams);
    visitor("m_naMaxMainBeams", value.m_naMaxMainBeams);
    visitor("m_naMinSubBeams", value.m_naMinSubBeams);
    visitor("m_naMaxSubBeams", value.m_naMaxSubBeams);
    visitor("m_faTemporaryBeamLife", value.m_faTemporaryBeamLife);
    visitor("m_faTargettingFOV", value.m_faTargettingFOV);
    visitor("m_faStaminaPerSecond", value.m_faStaminaPerSecond);
    visitor("m_fRandomBeamMinAngleX", value.m_fRandomBeamMinAngleX);
    visitor("m_fRandomBeamMaxAngleX", value.m_fRandomBeamMaxAngleX);
    visitor("m_fRandomBeamMinAngleZ", value.m_fRandomBeamMinAngleZ);
    visitor("m_fRandomBeamMaxAngleZ", value.m_fRandomBeamMaxAngleZ);
    visitor("m_faMainBeamDecapitationTime", value.m_faMainBeamDecapitationTime);
    visitor("m_faSubBeamDecapitationTime", value.m_faSubBeamDecapitationTime);
    visitor("m_faSubBeamDelay", value.m_faSubBeamDelay);
    visitor("OnhitEveryNSecs", value.OnhitEveryNSecs);
    visitor("CombatMultiplierIncreaseTime", value.CombatMultiplierIncreaseTime);
    visitor("InitSound", value.InitSound);
    visitor("LoopingSound", value.LoopingSound);
    visitor("EndHeroSound", value.EndHeroSound);
    visitor("EndAttackeeSound", value.EndAttackeeSound);
    visitor("LightningThemeTransitionInSecs", value.LightningThemeTransitionInSecs);
    visitor("LightningThemeTransitionOutSecs", value.LightningThemeTransitionOutSecs);
}

template <class V>
void visit_fields(CSpecialAbilitiesMultiArrowDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumberOfShots", value.NumberOfShots);
    visitor("NumberInVolley", value.NumberInVolley);
    visitor("AmmoEffectCreationDelaySecs", value.AmmoEffectCreationDelaySecs);
    visitor("CastSound", value.CastSound);
    visitor("BowHighlightWidth", value.BowHighlightWidth);
    visitor("BowHighlightColor", value.BowHighlightColor);
    visitor("MultiArrowPerArrowDamageMultiplier", value.MultiArrowPerArrowDamageMultiplier);
}

template <class V>
void visit_fields(CSpecialAbilitiesMultiStrikeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Sounds", value.Sounds);
    visitor("Animations", value.Animations);
    visitor("HighlightWidth", value.HighlightWidth);
    visitor("HighlightColor", value.HighlightColor);
    visitor("DelayTimeSecs", value.DelayTimeSecs);
    visitor("NumHits", value.NumHits);
    visitor("PerHitDamagerMultiplier", value.PerHitDamagerMultiplier);
    visitor("AttachmentPoint", value.AttachmentPoint);
}

template <class V>
void visit_fields(CSpecialAbilitiesPhysicalShieldDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("InitSound", value.InitSound);
    visitor("PowerUpSound", value.PowerUpSound);
    visitor("CastSound", value.CastSound);
    visitor("LoopingSound", value.LoopingSound);
    visitor("EndSound", value.EndSound);
    visitor("Health", value.Health);
    visitor("LevelUpTimings", value.LevelUpTimings);
    visitor("CamDist", value.CamDist);
    visitor("CamHeight", value.CamHeight);
    visitor("CamFOV", value.CamFOV);
    visitor("CamSwitchTime", value.CamSwitchTime);
    visitor("CastCamDist", value.CastCamDist);
    visitor("CastCamHeight", value.CastCamHeight);
    visitor("CastCamFOV", value.CastCamFOV);
    visitor("CastCamSwitchTime", value.CastCamSwitchTime);
    visitor("MinWidth", value.MinWidth);
    visitor("MaxWidth", value.MaxWidth);
    visitor("BeenHitExtraWidth", value.BeenHitExtraWidth);
    visitor("BeenHitFadeOffTimeSecs", value.BeenHitFadeOffTimeSecs);
    visitor("WobbleRate", value.WobbleRate);
    visitor("ColorStartLevel1", value.ColorStartLevel1);
    visitor("ColorStartLevel2", value.ColorStartLevel2);
    visitor("ColorStartLevel3", value.ColorStartLevel3);
    visitor("ColorStartLevel4", value.ColorStartLevel4);
    visitor("ColorEnd", value.ColorEnd);
    visitor("ColorHit", value.ColorHit);
    visitor("FadeOutPhysicalShieldTimeSecs", value.FadeOutPhysicalShieldTimeSecs);
    visitor("ManaDamage", value.ManaDamage);
    visitor("ShieldCoversCarriedItems", value.ShieldCoversCarriedItems);
}

template <class V>
void visit_fields(CSpecialAbilitiesSummonSpellDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TimeOfSummon", value.TimeOfSummon);
    visitor("BaseSummonCreatureName", value.BaseSummonCreatureName);
    visitor("FadeInTime", value.FadeInTime);
    visitor("FadeOutTime", value.FadeOutTime);
    visitor("ResetCreatureManaCost", value.ResetCreatureManaCost);
    visitor("SummonTintColour", value.SummonTintColour);
    visitor("EyeLeftDummyPointName", value.EyeLeftDummyPointName);
    visitor("EyeRightDummyPointName", value.EyeRightDummyPointName);
}

template <class V>
void visit_fields(CSpecialAbilitiesThunderLightningStormDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumBeams", value.NumBeams);
    visitor("StartRadius", value.StartRadius);
    visitor("EndRadius", value.EndRadius);
    visitor("SkyBeamStartTime", value.SkyBeamStartTime);
    visitor("MainBeamsStartTime", value.MainBeamsStartTime);
    visitor("BeamRotationStartSpeed", value.BeamRotationStartSpeed);
    visitor("BeamRotationEndSpeed", value.BeamRotationEndSpeed);
    visitor("TotalLifetime", value.TotalLifetime);
    visitor("Damage", value.Damage);
    visitor("InitSound", value.InitSound);
    visitor("LoopingSound", value.LoopingSound);
}

template <class V>
void visit_fields(CSpecialAbilitiesTurncoatSpellDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StaminaCostPerSec", value.StaminaCostPerSec);
    visitor("ResistanceDamagePerSec", value.ResistanceDamagePerSec);
    visitor("TurncoatTime", value.TurncoatTime);
    visitor("TurncoatRange", value.TurncoatRange);
    visitor("TurncoatXPUpEveryNSecs", value.TurncoatXPUpEveryNSecs);
}

template <class V>
void visit_fields(CSpecialAbilitiesUnholyPowerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("StartPowerupSound", value.StartPowerupSound);
    visitor("LoopingPowerupSound", value.LoopingPowerupSound);
    visitor("ReleaseSound", value.ReleaseSound);
    visitor("ReleaseLoopSound", value.ReleaseLoopSound);
    visitor("FinishSound", value.FinishSound);
    visitor("CancelSound", value.CancelSound);
    visitor("RandomFactorForNumSecsBetweenDamageHit", value.RandomFactorForNumSecsBetweenDamageHit);
    visitor("NumSecsBetweenDamageHit", value.NumSecsBetweenDamageHit);
    visitor("DamagePerHit", value.DamagePerHit);
    visitor("DurationOfSpellSecs", value.DurationOfSpellSecs);
    visitor("RadiusOfEffect", value.RadiusOfEffect);
    visitor("EnvChangeSecs", value.EnvChangeSecs);
    visitor("EnvReturnSecs", value.EnvReturnSecs);
    visitor("DoLifting", value.DoLifting);
    visitor("VelocityOfLift", value.VelocityOfLift);
    visitor("DistanceToLift", value.DistanceToLift);
    visitor("StaminaCostPerSec", value.StaminaCostPerSec);
    visitor("DamageTakenMultiplier", value.DamageTakenMultiplier);
    visitor("SmallRingRadius", value.SmallRingRadius);
    visitor("LargeRingRadius", value.LargeRingRadius);
}

template <class V>
void visit_fields(CSpecialEffectsStringMap &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ContainmentMap", value.ContainmentMap);
}

template <class V>
void visit_fields(CSpecialEffectsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SpecialEffectsMap", value.SpecialEffectsMap);
}

template <class V>
void visit_fields(CSpotLightDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Colour", value.Colour);
    visitor("InnerRadius", value.InnerRadius);
    visitor("OuterRadius", value.OuterRadius);
    visitor("Width", value.Width);
    visitor("Flicker", value.Flicker);
    visitor("FlickerSpeed", value.FlickerSpeed);
    visitor("Angle", value.Angle);
}

template <class V>
void visit_fields(CStealthDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DistanceMovementCanBeHeardFrom", value.DistanceMovementCanBeHeardFrom);
    visitor("RunningNoiseMultiplier", value.RunningNoiseMultiplier);
    visitor("JoggingNoiseMultiplier", value.JoggingNoiseMultiplier);
    visitor("WalkingNoiseMultiplier", value.WalkingNoiseMultiplier);
    visitor("DayTimeLampVisibilityMultiplier", value.DayTimeLampVisibilityMultiplier);
    visitor("NightTimeLampVisibilityMultiplier", value.NightTimeLampVisibilityMultiplier);
    visitor("DangerFeedbackHangtimeSeconds", value.DangerFeedbackHangtimeSeconds);
    visitor("RunningAwayFromCombatHangtimeSeconds", value.RunningAwayFromCombatHangtimeSeconds);
}

template <class V>
void visit_fields(CStockItemDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("DefaultPrice", value.DefaultPrice);
    visitor("DefaultIsStealable", value.DefaultIsStealable);
    visitor("CanOnlyBeSoldToPlayer", value.CanOnlyBeSoldToPlayer);
    visitor("SellersBanter", value.SellersBanter);
    visitor("BuyersPhrase", value.BuyersPhrase);
    visitor("ChanceOfBecomingAvailable", value.ChanceOfBecomingAvailable);
    visitor("CanBeDisplayedInShop", value.CanBeDisplayedInShop);
}

template <class V>
void visit_fields(CSummonDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CreatureToSummon", value.CreatureToSummon);
    visitor("CreatureFamilyToSummon", value.CreatureFamilyToSummon);
    visitor("NumberToSummon", value.NumberToSummon);
    visitor("SummonRadius", value.SummonRadius);
    visitor("MySummonLimit", value.MySummonLimit);
    visitor("MinimumSummon", value.MinimumSummon);
    visitor("TimeBetweenSummons", value.TimeBetweenSummons);
    visitor("SummonEffect", value.SummonEffect);
    visitor("SummoneeEffect", value.SummoneeEffect);
}

template <class V>
void visit_fields(CSummonableCreatureDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Rank", value.Rank);
    visitor("SummonedBrain", value.SummonedBrain);
}

template <class V>
void visit_fields(CSummonerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("LightningRangeForAutomaticUse", value.LightningRangeForAutomaticUse);
    visitor("LightningPercentChanceForRandomUse", value.LightningPercentChanceForRandomUse);
    visitor("LightningPercentChanceForBoastUse", value.LightningPercentChanceForBoastUse);
    visitor("LightningRandomChanceTestSeconds", value.LightningRandomChanceTestSeconds);
    visitor("LightningSecondsToWaitBeforeReuse", value.LightningSecondsToWaitBeforeReuse);
    visitor("ChargeRangeNear", value.ChargeRangeNear);
    visitor("ChargeRangeFar", value.ChargeRangeFar);
    visitor("ChargeRangeAttackInterrupt", value.ChargeRangeAttackInterrupt);
    visitor("ChargeSecondsToWaitBeforeReuse", value.ChargeSecondsToWaitBeforeReuse);
    visitor("SummonerDeathExplosion", value.SummonerDeathExplosion);
    visitor("LightningOrb", value.LightningOrb);
    visitor("LightningOrbAttackFireheart", value.LightningOrbAttackFireheart);
    visitor("LightningOrbLifeSeconds", value.LightningOrbLifeSeconds);
}

template <class V>
void visit_fields(CTCNoiseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("LifeTime", value.LifeTime);
    visitor("DistanceCanBeHeardFrom", value.DistanceCanBeHeardFrom);
}

template <class V>
void visit_fields(CTCVolumeContainmentTrackerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("UpdateEveryFrame", value.UpdateEveryFrame);
    visitor("CalculateBlendedLightingChannel", value.CalculateBlendedLightingChannel);
    visitor("LightingChannelSphereModelRadius", value.LightingChannelSphereModelRadius);
    visitor("LightingChannelSphereModelZOffset", value.LightingChannelSphereModelZOffset);
}

template <class V>
void visit_fields(CTargetingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("UseGlowTargeting", value.UseGlowTargeting);
    visitor("TargetArc", value.TargetArc);
    visitor("TargetArcPCThirdPersonAimingMode", value.TargetArcPCThirdPersonAimingMode);
    visitor("TargetingBasePosFromCamera", value.TargetingBasePosFromCamera);
    visitor("TargetingFacingDirFromCamera", value.TargetingFacingDirFromCamera);
    visitor("TargetingPCFacingDirFromCamera", value.TargetingPCFacingDirFromCamera);
    visitor("RejectTargetsBehindIfRunning", value.RejectTargetsBehindIfRunning);
    visitor("AcceptOnlyEnemiesBehind", value.AcceptOnlyEnemiesBehind);
    visitor("EnemyBehindTargetingRange", value.EnemyBehindTargetingRange);
    visitor("FirstPersonTargetingAngleFalloff", value.FirstPersonTargetingAngleFalloff);
    visitor("FirstPersonTargetingDistanceFalloff", value.FirstPersonTargetingDistanceFalloff);
    visitor("ThirdPersonTargetingAngleFalloff", value.ThirdPersonTargetingAngleFalloff);
    visitor("ThirdPersonTargetingDistanceFalloff", value.ThirdPersonTargetingDistanceFalloff);
    visitor("ZLockTargetSelectAngleFalloff", value.ZLockTargetSelectAngleFalloff);
    visitor("ZLockTargetSelectDistanceFalloff", value.ZLockTargetSelectDistanceFalloff);
    visitor("FirstPersonTargetingOverrideShortRangeFactor", value.FirstPersonTargetingOverrideShortRangeFactor);
    visitor("FirstPersonTargetingOverrideShortRangeFalloff", value.FirstPersonTargetingOverrideShortRangeFalloff);
    visitor("ThirdPersonTargetingOverrideShortRangeFactor", value.ThirdPersonTargetingOverrideShortRangeFactor);
    visitor("ThirdPersonTargetingOverrideShortRangeFalloff", value.ThirdPersonTargetingOverrideShortRangeFalloff);
    visitor("FirstPersonTargetingEnemyPreferenceFactor", value.FirstPersonTargetingEnemyPreferenceFactor);
    visitor("ThirdPersonTargetingEnemyPreferenceFactor", value.ThirdPersonTargetingEnemyPreferenceFactor);
    visitor("TargetingRanges", value.TargetingRanges);
    visitor("PreferPlayerWeight", value.PreferPlayerWeight);
    visitor("PreferNonCombatantsWeight", value.PreferNonCombatantsWeight);
    visitor("PreferLastAttackerWeight", value.PreferLastAttackerWeight);
    visitor("PreferCurrentBestEnemyWeight", value.PreferCurrentBestEnemyWeight);
}

template <class V>
void visit_fields(CTattooDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TattooName", value.TattooName);
    visitor("TextureLayer", value.TextureLayer);
    visitor("ReplacingTextureIndex", value.ReplacingTextureIndex);
    visitor("BankIndex", value.BankIndex);
    visitor("Blend", value.Blend);
    visitor("RemovesAllTattoos", value.RemovesAllTattoos);
    visitor("Permanent", value.Permanent);
    visitor("CoversBodyAreaFlags", value.CoversBodyAreaFlags);
    visitor("VisibilityMultiplier", value.VisibilityMultiplier);
    visitor("SpecificCoversBodyAreaFlags", value.SpecificCoversBodyAreaFlags);
    visitor("CustomFileName", value.CustomFileName);
    visitor("CustomFileNameBase", value.CustomFileNameBase);
    visitor("Attractiveness", value.Attractiveness);
    visitor("Scariness", value.Scariness);
    visitor("Goodstrength", value.Goodstrength);
}

template <class V>
void visit_fields(CTavernDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("BedCost", value.BedCost);
}

template <class V>
void visit_fields(CTavernGameCardBaseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("ControlType", value.ControlType);
    visitor("CardWidth", value.CardWidth);
    visitor("CardHeight", value.CardHeight);
    visitor("ImpulseScaleXbox", value.ImpulseScaleXbox);
    visitor("ImpulseScalePC", value.ImpulseScalePC);
    visitor("PointerModel", value.PointerModel);
    visitor("MoveSpeed", value.MoveSpeed);
    visitor("TurnSpeed", value.TurnSpeed);
    visitor("PointerHeightOffset", value.PointerHeightOffset);
    visitor("PointerParticleEffect", value.PointerParticleEffect);
    visitor("HighlightWidth", value.HighlightWidth);
    visitor("HighlightColourGood", value.HighlightColourGood);
    visitor("HighlightColourBad", value.HighlightColourBad);
    visitor("CardSeparation", value.CardSeparation);
    visitor("TableSeparation", value.TableSeparation);
    visitor("PackOffset", value.PackOffset);
    visitor("CardOrders", value.CardOrders);
    visitor("CardPack", value.CardPack);
}

template <class V>
void visit_fields(CTavernGameCoinBaseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("CoinIDensity", value.CoinIDensity);
    visitor("CoinRadius", value.CoinRadius);
    visitor("CoinEdgeRestitution", value.CoinEdgeRestitution);
    visitor("CoinEdgeFriction", value.CoinEdgeFriction);
    visitor("CoinSurfaceFriction", value.CoinSurfaceFriction);
    visitor("CoinDefIndex", value.CoinDefIndex);
    visitor("ArrowDefIndex", value.ArrowDefIndex);
}

template <class V>
void visit_fields(CTavernGameCoinGolfDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("TargetDistanceScale", value.TargetDistanceScale);
    visitor("JoystickScaleXboxX", value.JoystickScaleXboxX);
    visitor("JoystickScaleXboxY", value.JoystickScaleXboxY);
    visitor("JoystickScaleX", value.JoystickScaleX);
    visitor("JoystickScaleY", value.JoystickScaleY);
    visitor("JoystickMagBelow", value.JoystickMagBelow);
    visitor("AverageJoystickMagAbove", value.AverageJoystickMagAbove);
    visitor("AverageDrift", value.AverageDrift);
    visitor("MaxPower", value.MaxPower);
    visitor("RestSpeed", value.RestSpeed);
    visitor("Courses", value.Courses);
    visitor("GoalRadius", value.GoalRadius);
    visitor("StartingCourse", value.StartingCourse);
}

template <class V>
void visit_fields(CTavernGameDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Banter", value.Banter);
    visitor("Greeting", value.Greeting);
    visitor("OptionsInitial", value.OptionsInitial);
    visitor("OptionsSubsequent", value.OptionsSubsequent);
    visitor("Instructions", value.Instructions);
    visitor("InstructionsPC", value.InstructionsPC);
    visitor("Betting", value.Betting);
    visitor("Play", value.Play);
    visitor("ReactionWin", value.ReactionWin);
    visitor("ReactionLose", value.ReactionLose);
    visitor("ReactionDraw", value.ReactionDraw);
    visitor("ReactionWinNewBestScore", value.ReactionWinNewBestScore);
    visitor("FarewellInitial", value.FarewellInitial);
    visitor("FarewellSubsequent", value.FarewellSubsequent);
    visitor("WinRoundPhrase", value.WinRoundPhrase);
    visitor("OutOfTimePhrase", value.OutOfTimePhrase);
    visitor("NoMoney", value.NoMoney);
    visitor("GreetingForward", value.GreetingForward);
    visitor("OptionsForward", value.OptionsForward);
    visitor("OptionsBack", value.OptionsBack);
    visitor("OptionsAlternative", value.OptionsAlternative);
    visitor("InstructionsBack", value.InstructionsBack);
    visitor("BettingForward", value.BettingForward);
    visitor("BettingBack", value.BettingBack);
    visitor("ReactionForward", value.ReactionForward);
    visitor("FarewellForward", value.FarewellForward);
    visitor("CameraName", value.CameraName);
    visitor("BoxGraphicL", value.BoxGraphicL);
    visitor("BoxGraphicC", value.BoxGraphicC);
    visitor("BoxGraphicR", value.BoxGraphicR);
    visitor("ClickToContinue", value.ClickToContinue);
    visitor("WinPhrase", value.WinPhrase);
    visitor("LosePhrase", value.LosePhrase);
    visitor("DrawPhrase", value.DrawPhrase);
    visitor("NewGame", value.NewGame);
    visitor("BestScore", value.BestScore);
    visitor("CurrentScore", value.CurrentScore);
    visitor("RequiredScore", value.RequiredScore);
    visitor("AdditionalInfo", value.AdditionalInfo);
    visitor("BlackjackBusted", value.BlackjackBusted);
    visitor("BlackjackSplit", value.BlackjackSplit);
    visitor("BlackjackDouble", value.BlackjackDouble);
    visitor("BlackjackHit", value.BlackjackHit);
    visitor("BlackjackStand", value.BlackjackStand);
    visitor("BlackjackDealerTakesCard", value.BlackjackDealerTakesCard);
    visitor("BlackjackSplitGUI", value.BlackjackSplitGUI);
    visitor("BlackjackDoubleGUI", value.BlackjackDoubleGUI);
    visitor("BlackjackHitGUI", value.BlackjackHitGUI);
    visitor("BlackjackStandGUI", value.BlackjackStandGUI);
    visitor("Bet", value.Bet);
    visitor("PlayersMoney", value.PlayersMoney);
    visitor("TotalWinnings", value.TotalWinnings);
    visitor("Continue", value.Continue);
    visitor("Quit", value.Quit);
    visitor("Yes", value.Yes);
    visitor("No", value.No);
    visitor("PrizeGiven", value.PrizeGiven);
    visitor("MoneyBagGraphic", value.MoneyBagGraphic);
    visitor("MinBet", value.MinBet);
    visitor("MaxBet", value.MaxBet);
    visitor("BetIncrement", value.BetIncrement);
    visitor("ScoreFont", value.ScoreFont);
    visitor("TargetFont", value.TargetFont);
    visitor("StatsFont", value.StatsFont);
    visitor("ScoreX", value.ScoreX);
    visitor("ScoreY", value.ScoreY);
    visitor("TargetX", value.TargetX);
    visitor("TargetY", value.TargetY);
    visitor("BestX", value.BestX);
    visitor("BestY", value.BestY);
    visitor("AdditionalX", value.AdditionalX);
    visitor("AdditionalY", value.AdditionalY);
    visitor("BetX", value.BetX);
    visitor("BetY", value.BetY);
    visitor("MoneyX", value.MoneyX);
    visitor("MoneyY", value.MoneyY);
    visitor("WinningsX", value.WinningsX);
    visitor("WinningsY", value.WinningsY);
    visitor("MainBetX", value.MainBetX);
    visitor("MainBetY", value.MainBetY);
    visitor("MainMoneyX", value.MainMoneyX);
    visitor("MainMoneyY", value.MainMoneyY);
    visitor("BestScoreHigh", value.BestScoreHigh);
    visitor("PrizeScores", value.PrizeScores);
    visitor("Prize", value.Prize);
    visitor("PrizeRenown", value.PrizeRenown);
    visitor("MainGameScoreBoxX", value.MainGameScoreBoxX);
    visitor("MainGameScoreBoxY", value.MainGameScoreBoxY);
    visitor("MainGameScoreBoxWidthXbox", value.MainGameScoreBoxWidthXbox);
    visitor("MainGameScoreBoxWidthPC", value.MainGameScoreBoxWidthPC);
    visitor("MainGameScoreBoxHeight", value.MainGameScoreBoxHeight);
    visitor("DisplayErrata", value.DisplayErrata);
    visitor("PointerPhaseSpeed", value.PointerPhaseSpeed);
}

template <class V>
void visit_fields(CTavernGameShoveHaPennyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CTavernGameDef &>(value), visitor);
    visitor("Regions", value.Regions);
    visitor("BoardDefIndex", value.BoardDefIndex);
    visitor("ArrowDefIndex", value.ArrowDefIndex);
    visitor("PositionCoinSpeedXbox", value.PositionCoinSpeedXbox);
    visitor("PositionCoinSpeedPC", value.PositionCoinSpeedPC);
    visitor("PositionCoinMinimum", value.PositionCoinMinimum);
    visitor("PositionCoinMaximum", value.PositionCoinMaximum);
    visitor("PositionCoinDepth", value.PositionCoinDepth);
    visitor("ImpulseScale", value.ImpulseScale);
    visitor("TargetDistanceScale", value.TargetDistanceScale);
    visitor("JoystickScaleXboxX", value.JoystickScaleXboxX);
    visitor("JoystickScaleXboxY", value.JoystickScaleXboxY);
    visitor("JoystickScalePCX", value.JoystickScalePCX);
    visitor("JoystickScalePCY", value.JoystickScalePCY);
    visitor("JoystickTranslateY", value.JoystickTranslateY);
    visitor("JoystickMagBelow", value.JoystickMagBelow);
    visitor("AverageJoystickMagAbove", value.AverageJoystickMagAbove);
    visitor("AverageDrift", value.AverageDrift);
    visitor("MaxPower", value.MaxPower);
    visitor("RestSpeed", value.RestSpeed);
    visitor("Rounds", value.Rounds);
}

template <class V>
void visit_fields(CTavernGameSpotTheAdditionDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Pointer", value.Pointer);
    visitor("Cover", value.Cover);
    visitor("CoverWidth", value.CoverWidth);
    visitor("CoverLength", value.CoverLength);
    visitor("Knicknacks", value.Knicknacks);
    visitor("ItemsPerRound", value.ItemsPerRound);
    visitor("JoystickScale", value.JoystickScale);
    visitor("TimeToThink", value.TimeToThink);
    visitor("MaxCursorHeight", value.MaxCursorHeight);
    visitor("KnickKnackRadius", value.KnickKnackRadius);
    visitor("ImpulseScale", value.ImpulseScale);
    visitor("PointerModel", value.PointerModel);
    visitor("PointerHeightOffset", value.PointerHeightOffset);
    visitor("TimeToMoveCover", value.TimeToMoveCover);
    visitor("TimeToPauseCover", value.TimeToPauseCover);
    visitor("MaxCoverHeight", value.MaxCoverHeight);
    visitor("MinimumKnickKnackGap", value.MinimumKnickKnackGap);
    visitor("LengthMovementScale", value.LengthMovementScale);
    visitor("HeightMovementScale", value.HeightMovementScale);
    visitor("HighlightWidth", value.HighlightWidth);
    visitor("HighlightColour", value.HighlightColour);
    visitor("HighlightColourError", value.HighlightColourError);
}

template <class V>
void visit_fields(CTavernTableDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AddTankards", value.AddTankards);
    visitor("HighQualityTankards", value.HighQualityTankards);
}

template <class V>
void visit_fields(CTeleporterDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Radius", value.Radius);
    visitor("ActiveEffect", value.ActiveEffect);
    visitor("ActiveByProximity", value.ActiveByProximity);
}

template <class V>
void visit_fields(CTextureReplacementDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Entries", value.Entries);
}

template <class V>
void visit_fields(CTextureReplacementDef::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("OriginalTexture", value.OriginalTexture);
    visitor("NewTexture", value.NewTexture);
}

template <class V>
void visit_fields(CThingComponentSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Entries", value.Entries);
    visitor("TrailingU32", value.TrailingU32);
}

template <class V>
void visit_fields(CThingComponentSet::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Name", value.Name);
    visitor("DriverType", value.DriverType);
    visitor("Override", value.Override);
}

template <class V>
void visit_fields(CThingBaseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Components", value.Components);
    visitor("AvailableInEditor", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.AvailableInEditor); },
        [](CThingBaseDef &o, bool v) { o.AvailableInEditor = v; }));
    visitor("AlwaysFrameUpdate", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.AlwaysFrameUpdate); },
        [](CThingBaseDef &o, bool v) { o.AlwaysFrameUpdate = v; }));
    visitor("StartKillLocked", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.StartKillLocked); },
        [](CThingBaseDef &o, bool v) { o.StartKillLocked = v; }));
    visitor("LevelLess", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.LevelLess); },
        [](CThingBaseDef &o, bool v) { o.LevelLess = v; }));
    visitor("IsDrawable", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.IsDrawable); },
        [](CThingBaseDef &o, bool v) { o.IsDrawable = v; }));
    visitor("IsDrawableInEditor", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.IsDrawableInEditor); },
        [](CThingBaseDef &o, bool v) { o.IsDrawableInEditor = v; }));
    visitor("ForceNoSerialise", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.ForceNoSerialise); },
        [](CThingBaseDef &o, bool v) { o.ForceNoSerialise = v; }));
    visitor("InMapSearches", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.InMapSearches); },
        [](CThingBaseDef &o, bool v) { o.InMapSearches = v; }));
    visitor("IsLoadableGlobal", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.IsLoadableGlobal); },
        [](CThingBaseDef &o, bool v) { o.IsLoadableGlobal = v; }));
    visitor("GameStatePersisted", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.GameStatePersisted); },
        [](CThingBaseDef &o, bool v) { o.GameStatePersisted = v; }));
    visitor("CanComeBetweenCameraAndObservedThing", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.CanComeBetweenCameraAndObservedThing); },
        [](CThingBaseDef &o, bool v) { o.CanComeBetweenCameraAndObservedThing = v; }));
    visitor("DrawWeaponTrails", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.DrawWeaponTrails); },
        [](CThingBaseDef &o, bool v) { o.DrawWeaponTrails = v; }));
    visitor("DrawProjectileWeaponTrails", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.DrawProjectileWeaponTrails); },
        [](CThingBaseDef &o, bool v) { o.DrawProjectileWeaponTrails = v; }));
    visitor("IsVulnerableToMelee", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.IsVulnerableToMelee); },
        [](CThingBaseDef &o, bool v) { o.IsVulnerableToMelee = v; }));
    visitor("Damageable", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.Damageable); },
        [](CThingBaseDef &o, bool v) { o.Damageable = v; }));
    visitor("AlwaysIncludeInObjectStrikeScans", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.AlwaysIncludeInObjectStrikeScans); },
        [](CThingBaseDef &o, bool v) { o.AlwaysIncludeInObjectStrikeScans = v; }));
    visitor("DisableOcclusionTesting", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.DisableOcclusionTesting); },
        [](CThingBaseDef &o, bool v) { o.DisableOcclusionTesting = v; }));
    visitor("DrawAfterWater", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.DrawAfterWater); },
        [](CThingBaseDef &o, bool v) { o.DrawAfterWater = v; }));
    visitor("ForceRenderedLastFrameOverride", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.ForceRenderedLastFrameOverride); },
        [](CThingBaseDef &o, bool v) { o.ForceRenderedLastFrameOverride = v; }));
    visitor("AddToComboMultiplierOnHit", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.AddToComboMultiplierOnHit); },
        [](CThingBaseDef &o, bool v) { o.AddToComboMultiplierOnHit = v; }));
    visitor("GiveHeroStatChangesOnBeingHit", detail::bitfield(value,
        [](const CThingBaseDef &o) { return bool(o.GiveHeroStatChangesOnBeingHit); },
        [](CThingBaseDef &o, bool v) { o.GiveHeroStatChangesOnBeingHit = v; }));
    visitor("GroupDef", value.GroupDef);
    visitor("RenderFadeDistance", value.RenderFadeDistance);
    visitor("PersistenceFlags", value.PersistenceFlags);
    visitor("Health", value.Health);
    visitor("MinHealth", value.MinHealth);
    visitor("MeshHeightOffset", value.MeshHeightOffset);
    visitor("MeshHeight", value.MeshHeight);
    visitor("MeshRadius", value.MeshRadius);
    visitor("ApproxMaxMeshHeight", value.ApproxMaxMeshHeight);
    visitor("DefaultScriptName", value.DefaultScriptName);
    visitor("DefaultScriptData", value.DefaultScriptData);
}

template <class V>
void visit_fields(CThingBuildingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("DieOffTimer", value.DieOffTimer);
    visitor("SimBuildingDef", value.SimBuildingDef);
    visitor("Graphic", value.Graphic);
    visitor("WallMaterial", value.WallMaterial);
    visitor("FloorMaterial", value.FloorMaterial);
    visitor("ReverbEnvironmentType", value.ReverbEnvironmentType);
    visitor("ReverbEnvironmentLevel", value.ReverbEnvironmentLevel);
    visitor("OutsideOcclusionLevel", value.OutsideOcclusionLevel);
    visitor("CameraManagerSetIndex", value.CameraManagerSetIndex);
    visitor("UseHighDetailQuadTree", detail::bitfield(value,
        [](const CThingBuildingDef &o) { return bool(o.UseHighDetailQuadTree); },
        [](CThingBuildingDef &o, bool v) { o.UseHighDetailQuadTree = v; }));
    visitor("IsSelfIlluminating", detail::bitfield(value,
        [](const CThingBuildingDef &o) { return bool(o.IsSelfIlluminating); },
        [](CThingBuildingDef &o, bool v) { o.IsSelfIlluminating = v; }));
    visitor("IsPartOfHeroGuild", detail::bitfield(value,
        [](const CThingBuildingDef &o) { return bool(o.IsPartOfHeroGuild); },
        [](CThingBuildingDef &o, bool v) { o.IsPartOfHeroGuild = v; }));
    visitor("HasWeatherMask", detail::bitfield(value,
        [](const CThingBuildingDef &o) { return bool(o.HasWeatherMask); },
        [](CThingBuildingDef &o, bool v) { o.HasWeatherMask = v; }));
}

template <class V>
void visit_fields(CThingCreatureDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("StunTimeMin", value.StunTimeMin);
    visitor("StunTimeMax", value.StunTimeMax);
    visitor("StunHealthThreshold", value.StunHealthThreshold);
    visitor("DazedTimeMin", value.DazedTimeMin);
    visitor("DazedTimeMax", value.DazedTimeMax);
    visitor("DazedHealthThreshold", value.DazedHealthThreshold);
    visitor("NumSecondsToPersistOnDeath", value.NumSecondsToPersistOnDeath);
    visitor("MaxSlowWalkingSpeed", value.MaxSlowWalkingSpeed);
    visitor("MaxWalkingSpeed", value.MaxWalkingSpeed);
    visitor("MaxJoggingSpeed", value.MaxJoggingSpeed);
    visitor("MaxRunningSpeed", value.MaxRunningSpeed);
    visitor("MaxPushingSpeed", value.MaxPushingSpeed);
    visitor("MaxFlyingSpeed", value.MaxFlyingSpeed);
    visitor("MaxHoverSpeedSlow", value.MaxHoverSpeedSlow);
    visitor("MaxHoverSpeedMedium", value.MaxHoverSpeedMedium);
    visitor("MaxHoverSpeedFast", value.MaxHoverSpeedFast);
    visitor("SpeedRandomisationFactor", value.SpeedRandomisationFactor);
    visitor("MaxAcceleration", value.MaxAcceleration);
    visitor("HearingRadius", value.HearingRadius);
    visitor("FOV", value.FOV);
    visitor("Sex", value.Sex);
    visitor("CreatureType", value.CreatureType);
    visitor("NavigatorTypes", value.NavigatorTypes);
    visitor("InitialWeaponDef", value.InitialWeaponDef);
    visitor("PrimaryMeleeWeaponDef", value.PrimaryMeleeWeaponDef);
    visitor("SecondaryMeleeWeaponDef", value.SecondaryMeleeWeaponDef);
    visitor("RangedWeaponDef", value.RangedWeaponDef);
    visitor("InitialSheathedWeapon", value.InitialSheathedWeapon);
    visitor("BrainDef", value.BrainDef);
    visitor("CreatureInteractionType", value.CreatureInteractionType);
    visitor("Property", value.Property);
    visitor("Damage", value.Damage);
    visitor("DamageEffectDef", value.DamageEffectDef);
    visitor("StunnedParticle", value.StunnedParticle);
    visitor("Graphic", value.Graphic);
    visitor("EyeGraphic", value.EyeGraphic);
    visitor("DefaultOwner", value.DefaultOwner);
    visitor("ControlledMovementType", value.ControlledMovementType);
    visitor("CombatTypeDef", value.CombatTypeDef);
    visitor("ResetCombatTypeOnUnsheatheDefaultWeapons", value.ResetCombatTypeOnUnsheatheDefaultWeapons);
    visitor("HeroCombatDef", value.HeroCombatDef);
    visitor("HeroCombatPCDef", value.HeroCombatPCDef);
    visitor("CombatDiameter", value.CombatDiameter);
    visitor("DeferHitsIfBehindHitter", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.DeferHitsIfBehindHitter); },
        [](CThingCreatureDef &o, bool v) { o.DeferHitsIfBehindHitter = v; }));
    visitor("LeaderTypeDef", value.LeaderTypeDef);
    visitor("RecoverEvadeHitsPercent", value.RecoverEvadeHitsPercent);
    visitor("DeadBodyDecayEffect", value.DeadBodyDecayEffect);
    visitor("LeaveDeadCreature", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.LeaveDeadCreature); },
        [](CThingCreatureDef &o, bool v) { o.LeaveDeadCreature = v; }));
    visitor("AbleToStrafe", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.AbleToStrafe); },
        [](CThingCreatureDef &o, bool v) { o.AbleToStrafe = v; }));
    visitor("CreateDummyOnDeath", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.CreateDummyOnDeath); },
        [](CThingCreatureDef &o, bool v) { o.CreateDummyOnDeath = v; }));
    visitor("ImmuneToAttack", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.ImmuneToAttack); },
        [](CThingCreatureDef &o, bool v) { o.ImmuneToAttack = v; }));
    visitor("UseMeleeWeaponAsDefault", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.UseMeleeWeaponAsDefault); },
        [](CThingCreatureDef &o, bool v) { o.UseMeleeWeaponAsDefault = v; }));
    visitor("UnsheatheWeaponsAutomatically", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.UnsheatheWeaponsAutomatically); },
        [](CThingCreatureDef &o, bool v) { o.UnsheatheWeaponsAutomatically = v; }));
    visitor("IsWillable", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.IsWillable); },
        [](CThingCreatureDef &o, bool v) { o.IsWillable = v; }));
    visitor("IsIndestructible", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.IsIndestructible); },
        [](CThingCreatureDef &o, bool v) { o.IsIndestructible = v; }));
    visitor("IsInvulnerableDuringGenericResponse", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.IsInvulnerableDuringGenericResponse); },
        [](CThingCreatureDef &o, bool v) { o.IsInvulnerableDuringGenericResponse = v; }));
    visitor("IsPushedByExplosions", detail::bitfield(value,
        [](const CThingCreatureDef &o) { return bool(o.IsPushedByExplosions); },
        [](CThingCreatureDef &o, bool v) { o.IsPushedByExplosions = v; }));
}

template <class V>
void visit_fields(CThingDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
}

template <class V>
void visit_fields(CThingDrainLifeShotDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MaxDistance", value.MaxDistance);
    visitor("MaxAngle", value.MaxAngle);
    visitor("MaxAngleToTurnDegs", value.MaxAngleToTurnDegs);
    visitor("InitialSpeed", value.InitialSpeed);
    visitor("SpeedRange", value.SpeedRange);
}

template <class V>
void visit_fields(CThingGroupDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Dummy", value.Dummy);
}

template <class V>
void visit_fields(CThingHolySiteDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("Graphic", value.Graphic);
    visitor("PlayerStart", value.PlayerStart);
}

template <class V>
void visit_fields(CThingMarkerDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("Graphic", value.Graphic);
    visitor("TriggerType", value.TriggerType);
    visitor("Properties", value.Properties);
}

template <class V>
void visit_fields(CThingMultiArrowShotDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("AccelerationScalar", value.AccelerationScalar);
    visitor("MaxDistance", value.MaxDistance);
    visitor("MaxAngle", value.MaxAngle);
    visitor("TimeToReachTarget", value.TimeToReachTarget);
    visitor("MaxAngleToTurnDegs", value.MaxAngleToTurnDegs);
    visitor("MultiArrowWeaponTrail", value.MultiArrowWeaponTrail);
    visitor("MultiArrowHitEffect", value.MultiArrowHitEffect);
    visitor("MultiArrowTrailEffect", value.MultiArrowTrailEffect);
    visitor("MultiArrowSpeed", value.MultiArrowSpeed);
    visitor("MultiArrowDeflectionByMelee", value.MultiArrowDeflectionByMelee);
    visitor("MultiArrowHitEffectPercentage", value.MultiArrowHitEffectPercentage);
}

template <class V>
void visit_fields(CThingNoiseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("Noise", value.Noise);
}

template <class V>
void visit_fields(CThingObjectDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("Graphic", value.Graphic);
    visitor("RotationTime", value.RotationTime);
    visitor("Material", value.Material);
    visitor("FloorMaterial", value.FloorMaterial);
    visitor("OnDeathObject", value.OnDeathObject);
    visitor("Property", value.Property);
    visitor("DieOffTimer", value.DieOffTimer);
    visitor("IsFragile", detail::bitfield(value,
        [](const CThingObjectDef &o) { return bool(o.IsFragile); },
        [](CThingObjectDef &o, bool v) { o.IsFragile = v; }));
    visitor("IsClothing", detail::bitfield(value,
        [](const CThingObjectDef &o) { return bool(o.IsClothing); },
        [](CThingObjectDef &o, bool v) { o.IsClothing = v; }));
    visitor("IsSelfIlluminating", detail::bitfield(value,
        [](const CThingObjectDef &o) { return bool(o.IsSelfIlluminating); },
        [](CThingObjectDef &o, bool v) { o.IsSelfIlluminating = v; }));
    visitor("UseHighDetailQuadTree", detail::bitfield(value,
        [](const CThingObjectDef &o) { return bool(o.UseHighDetailQuadTree); },
        [](CThingObjectDef &o, bool v) { o.UseHighDetailQuadTree = v; }));
    visitor("Electrocutable", detail::bitfield(value,
        [](const CThingObjectDef &o) { return bool(o.Electrocutable); },
        [](CThingObjectDef &o, bool v) { o.Electrocutable = v; }));
}

template <class V>
void visit_fields(CThingPhysicalSwitchDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("TriggerType", value.TriggerType);
    visitor("UntriggeredGraphic", value.UntriggeredGraphic);
    visitor("TriggeredGraphic", value.TriggeredGraphic);
    visitor("TriggerSound", value.TriggerSound);
}

template <class V>
void visit_fields(CThingShotDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Speed", value.Speed);
    visitor("PrimaryEffect", value.PrimaryEffect);
    visitor("PrimaryEffectSound", value.PrimaryEffectSound);
    visitor("AttachedContinuousEffect", value.AttachedContinuousEffect);
    visitor("AttachedContinuousSound", value.AttachedContinuousSound);
    visitor("UseEndTimeLineOnContinousEffect", value.UseEndTimeLineOnContinousEffect);
    visitor("HitThingDamage", value.HitThingDamage);
    visitor("HitThingShieldDamage", value.HitThingShieldDamage);
    visitor("CauseDamage", value.CauseDamage);
    visitor("ShotDefinitionClassName", value.ShotDefinitionClassName);
    visitor("ShotOffsetAngleRange", value.ShotOffsetAngleRange);
    visitor("ShotOffsetRadiusRange", value.ShotOffsetRadiusRange);
    visitor("ExplosionDef", value.ExplosionDef);
    visitor("GroundExplosionDef", value.GroundExplosionDef);
    visitor("ObjectDef", value.ObjectDef);
    visitor("LeavesObjectBehind", value.LeavesObjectBehind);
    visitor("IsHittableByMelee", value.IsHittableByMelee);
    visitor("IsDeflectedOnHitByMelee", value.IsDeflectedOnHitByMelee);
    visitor("MeleeHitDeflectionDamageMultiplier", value.MeleeHitDeflectionDamageMultiplier);
    visitor("FaceMovementDirection", value.FaceMovementDirection);
    visitor("AllowDeflections", value.AllowDeflections);
    visitor("BounceOnGround", value.BounceOnGround);
    visitor("Bounciness", value.Bounciness);
    visitor("RotateOnDeflection", value.RotateOnDeflection);
    visitor("IsBlockable", value.IsBlockable);
    visitor("DamageType", value.DamageType);
    visitor("NumSubShots", value.NumSubShots);
    visitor("SubShotType", value.SubShotType);
    visitor("SubShotCreationTime", value.SubShotCreationTime);
    visitor("SubShotMaxAngle", value.SubShotMaxAngle);
    visitor("RemoveMainShotOnSubShotCreation", value.RemoveMainShotOnSubShotCreation);
    visitor("DoesExplosionDamageCaster", value.DoesExplosionDamageCaster);
    visitor("CanHitRollingTargets", value.CanHitRollingTargets);
    visitor("PreventSlidingOnCollision", value.PreventSlidingOnCollision);
}

template <class V>
void visit_fields(CThingSwitchDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("TriggerType", value.TriggerType);
}

template <class V>
void visit_fields(CThingVillageDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visit_fields(static_cast<CThingBaseDef &>(value), visitor);
    visitor("Graphic", value.Graphic);
}

template <class V>
void visit_fields(CThunderBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumGetHitsBeforeStormAttack", value.NumGetHitsBeforeStormAttack);
    visitor("NumMissesBeforeStormAttack", value.NumMissesBeforeStormAttack);
    visitor("LightningDamage", value.LightningDamage);
    visitor("BattleChargeLevel", value.BattleChargeLevel);
}

template <class V>
void visit_fields(CTimeAppearanceFadeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("FadeInStart", value.FadeInStart);
    visitor("FadeInEnd", value.FadeInEnd);
    visitor("FadeOutStart", value.FadeOutStart);
    visitor("FadeOutEnd", value.FadeOutEnd);
}

template <class V>
void visit_fields(CTrapDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Sound", value.Sound);
    visitor("TriggerType", value.TriggerType);
    visitor("TrapType", value.TrapType);
    visitor("TriggerRadius", value.TriggerRadius);
    visitor("TriggerHelperName", value.TriggerHelperName);
    visitor("TrapDamage", value.TrapDamage);
    visitor("TriggeredCollisionMesh", value.TriggeredCollisionMesh);
    visitor("UntriggeredCollisionMesh", value.UntriggeredCollisionMesh);
    visitor("ExplosionDefIndex", value.ExplosionDefIndex);
    visitor("PhysicalObstructionDefIndex", value.PhysicalObstructionDefIndex);
    visitor("NumShotsToFireBeforeIdle", value.NumShotsToFireBeforeIdle);
    visitor("SecondsIdle", value.SecondsIdle);
    visitor("SecondsBetweenShots", value.SecondsBetweenShots);
}

template <class V>
void visit_fields(CTrollBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GroundPoundSpeedMultiplierStart", value.GroundPoundSpeedMultiplierStart);
    visitor("GroundPoundSpeedMultiplierEnd", value.GroundPoundSpeedMultiplierEnd);
    visitor("AttackPosPredictiveness", value.AttackPosPredictiveness);
    visitor("PercentageChanceToUseSecondaryShotType", value.PercentageChanceToUseSecondaryShotType);
    visitor("SecondaryShotType", value.SecondaryShotType);
    visitor("MinLaughDistance", value.MinLaughDistance);
    visitor("MaxTimeBetweenKnockdownAndBoast", value.MaxTimeBetweenKnockdownAndBoast);
    visitor("DeathExplosionIndex", value.DeathExplosionIndex);
    visitor("FallDownExplosionIndex", value.FallDownExplosionIndex);
}

template <class V>
void visit_fields(CTrophyDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("MinigameInitialTimerSecs", value.MinigameInitialTimerSecs);
    visitor("TimeBonusPerWitness", value.TimeBonusPerWitness);
    visitor("RenownGainedPerWitnessAhead", value.RenownGainedPerWitnessAhead);
    visitor("EvilTrophy", value.EvilTrophy);
    visitor("FakeTrophy", value.FakeTrophy);
    visitor("EmoteIconForAvailableWitness", value.EmoteIconForAvailableWitness);
    visitor("ShowTrophyAnimName", value.ShowTrophyAnimName);
    visitor("GameDebugTextScale", value.GameDebugTextScale);
    visitor("SecsDelayPerGainedWitnessSound", value.SecsDelayPerGainedWitnessSound);
    visitor("SoundStartGame", value.SoundStartGame);
    visitor("SoundShowTrophy", value.SoundShowTrophy);
    visitor("SoundGainedWitness", value.SoundGainedWitness);
    visitor("SoundOutOfTime", value.SoundOutOfTime);
    visitor("SoundGotAllWitnesses", value.SoundGotAllWitnesses);
    visitor("SoundEndNewHighScore", value.SoundEndNewHighScore);
    visitor("SoundEndNoNewScore", value.SoundEndNoNewScore);
}

template <class V>
void visit_fields(CTurncoatDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Turncoatable", value.Turncoatable);
    visitor("TurncoatedEffect", value.TurncoatedEffect);
    visitor("TurncoatActivateEffect", value.TurncoatActivateEffect);
    visitor("TurncoatDeactivateEffect", value.TurncoatDeactivateEffect);
    visitor("MinimumResistance", value.MinimumResistance);
    visitor("TimeDelayBeforeResistanceRecoverySecs", value.TimeDelayBeforeResistanceRecoverySecs);
    visitor("ResistanceRecoveryPerSec", value.ResistanceRecoveryPerSec);
    visitor("TurncoatStageEffects", value.TurncoatStageEffects);
    visitor("PercentageOfHealthAsResistance", value.PercentageOfHealthAsResistance);
    visitor("ExtraResistance", value.ExtraResistance);
}

template <class V>
void visit_fields(CVillageDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Tasks", value.Tasks);
    visitor("PatrolTrackName", value.PatrolTrackName);
    visitor("DefaultStanceToHero", value.DefaultStanceToHero);
    visitor("SecondsSecuritySweepLasts", value.SecondsSecuritySweepLasts);
    visitor("WarehouseCrateDef", value.WarehouseCrateDef);
    visitor("StoryStartTimes", value.StoryStartTimes);
    visitor("StoryDuration", value.StoryDuration);
    visitor("SchoolStartTimes", value.SchoolStartTimes);
    visitor("SchoolDuration", value.SchoolDuration);
    visitor("WorkStartTimes", value.WorkStartTimes);
    visitor("WorkDuration", value.WorkDuration);
    visitor("CookStartTimes", value.CookStartTimes);
    visitor("CookDuration", value.CookDuration);
    visitor("EatStartTimes", value.EatStartTimes);
    visitor("EatDuration", value.EatDuration);
    visitor("TavernStartTimes", value.TavernStartTimes);
    visitor("TavernDuration", value.TavernDuration);
    visitor("GamingStartTimes", value.GamingStartTimes);
    visitor("GamingDuration", value.GamingDuration);
    visitor("SleepAdultStartTime", value.SleepAdultStartTime);
    visitor("SleepAdultEndTime", value.SleepAdultEndTime);
    visitor("SleepChildStartTime", value.SleepChildStartTime);
    visitor("SleepChildEndTime", value.SleepChildEndTime);
    visitor("SleepElderlyStartTime", value.SleepElderlyStartTime);
    visitor("SleepElderlyEndTime", value.SleepElderlyEndTime);
    visitor("HouseLightsOnTime", value.HouseLightsOnTime);
    visitor("HouseLightsOffTime", value.HouseLightsOffTime);
    visitor("SociableStartTime", value.SociableStartTime);
    visitor("SociableFinishTime", value.SociableFinishTime);
    visitor("CurfewStartTime", value.CurfewStartTime);
    visitor("CurfewFinishTime", value.CurfewFinishTime);
    visitor("EnableAutoPlacement", value.EnableAutoPlacement);
    visitor("AllowHouseBuying", value.AllowHouseBuying);
    visitor("HearTownCrier", value.HearTownCrier);
    visitor("CurfewStartSound", value.CurfewStartSound);
    visitor("CurfewEndSound", value.CurfewEndSound);
    visitor("WorkStartSound", value.WorkStartSound);
    visitor("WorkEndSound", value.WorkEndSound);
    visitor("BribeCost", value.BribeCost);
    visitor("FramesBetweenGuardSnoop", value.FramesBetweenGuardSnoop);
    visitor("FramesBetweenGuardSnoopNightShops", value.FramesBetweenGuardSnoopNightShops);
    visitor("FramesBeforeGuardSnoop", value.FramesBeforeGuardSnoop);
    visitor("FramesBeforeGuardSnoopNightShops", value.FramesBeforeGuardSnoopNightShops);
}

template <class V>
void visit_fields(CVillageMemberDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NeedsVillageToFunctionCorrectly", value.NeedsVillageToFunctionCorrectly);
}

template <class V>
void visit_fields(CVillagePeopleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Enable", value.Enable);
    visitor("DesiredMaleWeight", value.DesiredMaleWeight);
    visitor("DesiredFemaleWeight", value.DesiredFemaleWeight);
    visitor("DesiredChildWeight", value.DesiredChildWeight);
    visitor("ProbabilityOfHomsexualityPerDialogueEnabledMan", value.ProbabilityOfHomsexualityPerDialogueEnabledMan);
    visitor("MaleTeacher", value.MaleTeacher);
    visitor("FemaleTeacher", value.FemaleTeacher);
    visitor("MaleShopKeeper", value.MaleShopKeeper);
    visitor("FemaleShopKeeper", value.FemaleShopKeeper);
    visitor("MaleMarketTrader", value.MaleMarketTrader);
    visitor("FemaleMarketTrader", value.FemaleMarketTrader);
    visitor("MaleBarman", value.MaleBarman);
    visitor("FemaleBarman", value.FemaleBarman);
    visitor("MaleBarmaid", value.MaleBarmaid);
    visitor("FemaleBarmaid", value.FemaleBarmaid);
    visitor("MaleWarehouseWorker", value.MaleWarehouseWorker);
    visitor("FemaleWarehouseWorker", value.FemaleWarehouseWorker);
    visitor("MaleServant", value.MaleServant);
    visitor("FemaleServant", value.FemaleServant);
    visitor("MaleGoodApprentice", value.MaleGoodApprentice);
    visitor("FemaleGoodApprentice", value.FemaleGoodApprentice);
    visitor("MaleEvilApprentice", value.MaleEvilApprentice);
    visitor("FemaleEvilApprentice", value.FemaleEvilApprentice);
    visitor("MaleUnemployed", value.MaleUnemployed);
    visitor("FemaleUnemployed", value.FemaleUnemployed);
    visitor("MaleChild", value.MaleChild);
    visitor("FemaleChild", value.FemaleChild);
    visitor("GamesMaster", value.GamesMaster);
    visitor("Guard", value.Guard);
    visitor("MaleGhost", value.MaleGhost);
    visitor("FemaleGhost", value.FemaleGhost);
    visitor("NumGuards", value.NumGuards);
    visitor("NumGhosts", value.NumGhosts);
    visitor("GuardRespawnType", value.GuardRespawnType);
    visitor("GuardRespawnQuantity", value.GuardRespawnQuantity);
    visitor("GuardRespawnMaxRenownBasedOffset", value.GuardRespawnMaxRenownBasedOffset);
}

template <class V>
void visit_fields(CVillagerInteractionsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Animation1", value.Animation1);
    visitor("Animation2", value.Animation2);
    visitor("Distance", value.Distance);
}

template <class V>
void visit_fields(CWallMountEffectsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Effect", value.Effect);
    visitor("HealingAmount", value.HealingAmount);
    visitor("SecondsBetweenEffectUsesAllowed", value.SecondsBetweenEffectUsesAllowed);
}

template <class V>
void visit_fields(CWaspQueenBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumFramesPauseBetweenAttacks", value.NumFramesPauseBetweenAttacks);
}

template <class V>
void visit_fields(CWeaponTrailGraphicSet &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Attack", value.Attack);
    visitor("Knockdown", value.Knockdown);
}

template <class V>
void visit_fields(CWeaponDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("Class", value.Class);
    visitor("Property", value.Property);
    visitor("AnimationGroupName", value.AnimationGroupName);
    visitor("SheatheObject", value.SheatheObject);
    visitor("SheatheObjectAlwaysVisible", value.SheatheObjectAlwaysVisible);
    visitor("AnimationSpeedVal", value.AnimationSpeedVal);
    visitor("Damage", value.Damage);
    visitor("DamageType", value.DamageType);
    visitor("Explosion", value.Explosion);
    visitor("FlourishParticles", value.FlourishParticles);
    visitor("FlourishSpecialWeaponTrail", value.FlourishSpecialWeaponTrail);
    visitor("WeaponTrails", value.WeaponTrails);
    visitor("UseAnalogueLoading", value.UseAnalogueLoading);
    visitor("MissileLoadCarrySlot", value.MissileLoadCarrySlot);
    visitor("ProjectileWeaponHelperNameTip", value.ProjectileWeaponHelperNameTip);
    visitor("ProjectileWeaponHelperNameBase", value.ProjectileWeaponHelperNameBase);
    visitor("Shot", value.Shot);
    visitor("Ammo", value.Ammo);
    visitor("TargetingRange", value.TargetingRange);
    visitor("TargetingFOV", value.TargetingFOV);
    visitor("ArrowTrails", value.ArrowTrails);
    visitor("ReloadSpeedRange", value.ReloadSpeedRange);
    visitor("ProjectileWeaponAutoReload", value.ProjectileWeaponAutoReload);
    visitor("ProjectileWeaponStartLoaded", value.ProjectileWeaponStartLoaded);
    visitor("UseExtendedProjectileTrails", value.UseExtendedProjectileTrails);
    visitor("NextWeaponForBoss", value.NextWeaponForBoss);
    visitor("MeleeTargetingArc", value.MeleeTargetingArc);
    visitor("NeedsLoading", value.NeedsLoading);
    visitor("UnsheatheToCombatAnim", value.UnsheatheToCombatAnim);
    visitor("UnsheatheToNormalAnim", value.UnsheatheToNormalAnim);
    visitor("SheatheFromCombatAnim", value.SheatheFromCombatAnim);
    visitor("SheatheFromNormalAnim", value.SheatheFromNormalAnim);
}

template <class V>
void visit_fields(CWhisperBattleDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("NumGetHitsBeforeSommersault", value.NumGetHitsBeforeSommersault);
    visitor("NumPhase2Potions", value.NumPhase2Potions);
}

template <class V>
void visit_fields(CWifeDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Dowry", value.Dowry);
}

template <class V>
void visit_fields(CWillResponseDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IsForcePushable", value.IsForcePushable);
    visitor("IsLifeDrainable", value.IsLifeDrainable);
    visitor("IsLightningable", value.IsLightningable);
    visitor("IsNotBloodyWillable", value.IsNotBloodyWillable);
    visitor("IsAffectedBySlowTime", value.IsAffectedBySlowTime);
    visitor("IsAbleToBeLifted", value.IsAbleToBeLifted);
    visitor("IsEpicPowerExplodable", value.IsEpicPowerExplodable);
    visitor("IsEpicSpellable", value.IsEpicSpellable);
}

template <class V>
void visit_fields(CWoundMorphs &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Morphs", value.Morphs);
    visitor("TrailingU32", value.TrailingU32);
}

template <class V>
void visit_fields(CWoundMorphs::CEntry &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Location", value.Location);
    visitor("ReplacingTextureIndex", value.ReplacingTextureIndex);
    visitor("ReplacingBumpMapIndex", value.ReplacingBumpMapIndex);
    visitor("WoundTextureIndex", value.WoundTextureIndex);
    visitor("WoundBumpMapIndex", value.WoundBumpMapIndex);
    visitor("ScarTextureIndex", value.ScarTextureIndex);
    visitor("ScarBumpMapIndex", value.ScarBumpMapIndex);
    visitor("FadeToScarDuration", value.FadeToScarDuration);
    visitor("TotalDuration", value.TotalDuration);
    visitor("PermanentScarStrength", value.PermanentScarStrength);
}

template <class V>
void visit_fields(NUISystem::CUIStateDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("GraphicIndex", value.GraphicIndex);
    visitor("Position", value.Position);
    visitor("Zoom", value.Zoom);
    visitor("Colour", value.Colour);
    visitor("UpdateTime", value.UpdateTime);
    visitor("StateChangeType", value.StateChangeType);
    visitor("LinearChange", value.LinearChange);
    visitor("StateChangeFlag", value.StateChangeFlag);
    visitor("ChildrenNotAffected", value.ChildrenNotAffected);
}

template <class V>
void visit_fields(NUISystem::CUIDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("Type", value.Type);
    visitor("States", value.States);
    visitor("MeshIndex", value.MeshIndex);
    visitor("Font", value.Font);
    visitor("TextValue", value.TextValue);
    visitor("Height", value.Height);
    visitor("Width", value.Width);
    visitor("ExpansionType", value.ExpansionType);
    visitor("Sprites", value.Sprites);
    visitor("Children", value.Children);
    visitor("HorizontalSeparations", value.HorizontalSeparations);
    visitor("VerticalSeparations", value.VerticalSeparations);
    visitor("NonScrollingChildren", value.NonScrollingChildren);
    visitor("MeshType", value.MeshType);
    visitor("TextWindow", value.TextWindow);
    visitor("Layer", value.Layer);
    visitor("Angle", value.Angle);
    visitor("PositionIsCenter", value.PositionIsCenter);
    visitor("TextLineBreak", value.TextLineBreak);
    visitor("ScaleText", value.ScaleText);
    visitor("Independant", value.Independant);
    visitor("ScrollingSpeed", value.ScrollingSpeed);
    visitor("Action", value.Action);
    visitor("ActionOnBack", value.ActionOnBack);
    visitor("ActionOnSelected", value.ActionOnSelected);
    visitor("ActionOnUnselected", value.ActionOnUnselected);
    visitor("ActionOnDestruction", value.ActionOnDestruction);
    visitor("PreAction", value.PreAction);
    visitor("ActionOnDroppedNowhere", value.ActionOnDroppedNowhere);
    visitor("ActionOnLeftClicked", value.ActionOnLeftClicked);
    visitor("ActionOnLeftUnclicked", value.ActionOnLeftUnclicked);
    visitor("ActionOnLeftHeld", value.ActionOnLeftHeld);
    visitor("ActionOnRightClicked", value.ActionOnRightClicked);
    visitor("ActionOnDropped", value.ActionOnDropped);
    visitor("ActionOnDraggedUp", value.ActionOnDraggedUp);
    visitor("ActionOnDraggedDown", value.ActionOnDraggedDown);
    visitor("ActionOnLeftClickedUnder", value.ActionOnLeftClickedUnder);
    visitor("ActionOnLeftClickedAbove", value.ActionOnLeftClickedAbove);
    visitor("Up", value.Up);
    visitor("Forward", value.Forward);
    visitor("RotationAxis", value.RotationAxis);
    visitor("RotationSpeed", value.RotationSpeed);
    visitor("AnimationIndex", value.AnimationIndex);
    visitor("DownArrow", value.DownArrow);
    visitor("UpArrow", value.UpArrow);
    visitor("UpLimit", value.UpLimit);
    visitor("DownLimit", value.DownLimit);
    visitor("Wrapping", value.Wrapping);
    visitor("Inverted", value.Inverted);
    visitor("PositionOffset", value.PositionOffset);
    visitor("AlphaOffset", value.AlphaOffset);
    visitor("Scrolling", value.Scrolling);
    visitor("ViewArea", value.ViewArea);
    visitor("ComputeOffsetsOnActivate", value.ComputeOffsetsOnActivate);
    visitor("ParentStatePrerequisite", value.ParentStatePrerequisite);
    visitor("Dimensions", value.Dimensions);
    visitor("Min", value.Min);
    visitor("Max", value.Max);
    visitor("Step", value.Step);
    visitor("InputDelay", value.InputDelay);
    visitor("DrawFromViewport", value.DrawFromViewport);
    visitor("SliderLeft", value.SliderLeft);
    visitor("SliderRight", value.SliderRight);
    visitor("TextBankIndex", value.TextBankIndex);
    visitor("ActionText", value.ActionText);
    visitor("KeyText", value.KeyText);
    visitor("UndefinedWarning", value.UndefinedWarning);
    visitor("Redefiner", value.Redefiner);
    visitor("ActionMap", value.ActionMap);
    visitor("ActionMapAliases", value.ActionMapAliases);
    visitor("ActionOrder", value.ActionOrder);
    visitor("EditBoxParentIsButton", value.EditBoxParentIsButton);
    visitor("PasswordBox", value.PasswordBox);
    visitor("EditBoxCharLimit", value.EditBoxCharLimit);
    visitor("EditBoxUsesIME", value.EditBoxUsesIME);
    visitor("DisallowSpaceAsFirstChar", value.DisallowSpaceAsFirstChar);
    visitor("MovieFilename", value.MovieFilename);
    visitor("LayerIndependant", value.LayerIndependant);
    visitor("SwappingStates", value.SwappingStates);
    visitor("SwappingTimes", value.SwappingTimes);
    visitor("BastardChild", value.BastardChild);
    visitor("Alignement", value.Alignement);
    visitor("RandomSwap", value.RandomSwap);
    visitor("HoveredState", value.HoveredState);
    visitor("UseRelativeZoom", value.UseRelativeZoom);
    visitor("UseRelativePosition", value.UseRelativePosition);
    visitor("PartOfListTree", value.PartOfListTree);
    visitor("LeftClickedState", value.LeftClickedState);
    visitor("RightClickedState", value.RightClickedState);
    visitor("ShapeChildren", value.ShapeChildren);
    visitor("UseViewArea", value.UseViewArea);
    visitor("PCStyle", value.PCStyle);
    visitor("Sprite2DFlag", value.Sprite2DFlag);
}

template <class V>
void visit_fields(NUISystem::CUIIconsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("IconFriendRequestReceived", value.IconFriendRequestReceived);
    visitor("IconFriendRequestReceivedOn", value.IconFriendRequestReceivedOn);
    visitor("IconFriendRequestSent", value.IconFriendRequestSent);
    visitor("IconFriendRequestSentOn", value.IconFriendRequestSentOn);
    visitor("IconGameInviteReceived", value.IconGameInviteReceived);
    visitor("IconGameInviteReceivedOn", value.IconGameInviteReceivedOn);
    visitor("IconGameInviteSent", value.IconGameInviteSent);
    visitor("IconGameInviteSentOn", value.IconGameInviteSentOn);
    visitor("IconMute", value.IconMute);
    visitor("IconMuteOn", value.IconMuteOn);
    visitor("IconOnline", value.IconOnline);
    visitor("IconOnlineOn", value.IconOnlineOn);
    visitor("IconPasscodeBlank", value.IconPasscodeBlank);
    visitor("IconPasscodeFilled", value.IconPasscodeFilled);
    visitor("IconTV", value.IconTV);
    visitor("IconTVOn", value.IconTVOn);
    visitor("IconVoice", value.IconVoice);
    visitor("IconVoiceOn", value.IconVoiceOn);
    visitor("IconWait1", value.IconWait1);
    visitor("IconWait2", value.IconWait2);
    visitor("IconWait3", value.IconWait3);
    visitor("IconWait4", value.IconWait4);
    visitor("IconProgress", value.IconProgress);
    visitor("IconProgressOn", value.IconProgressOn);
    visitor("IconA", value.IconA);
    visitor("IconB", value.IconB);
    visitor("IconX", value.IconX);
    visitor("IconY", value.IconY);
    visitor("IconBlank", value.IconBlank);
    visitor("IconUpArrow", value.IconUpArrow);
    visitor("IconDownArrow", value.IconDownArrow);
    visitor("IconListHighlight", value.IconListHighlight);
}

template <class V>
void visit_fields(NUISystem::CUILocaleGraphicsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("WorldMapRegionName", value.WorldMapRegionName);
    visitor("HelpScreenGraphics", value.HelpScreenGraphics);
    visitor("HelpRingPic", value.HelpRingPic);
}

template <class V>
void visit_fields(NUISystem::CUIMiscThingsDef &value, V &&visitor) {
    (void)value;
    (void)visitor;
    visitor("SpaceSeparator", value.SpaceSeparator);
    visitor("CommaSeparator", value.CommaSeparator);
    visitor("ColonSeparator", value.ColonSeparator);
    visitor("NewLineSeparator", value.NewLineSeparator);
    visitor("OpenBracket", value.OpenBracket);
    visitor("CloseBracket", value.CloseBracket);
    visitor("Positive", value.Positive);
    visitor("WeaponValueString", value.WeaponValueString);
    visitor("WeaponAugString", value.WeaponAugString);
    visitor("WeaponAugNone", value.WeaponAugNone);
    visitor("WeaponWeightString", value.WeaponWeightString);
    visitor("WeaponLightString", value.WeaponLightString);
    visitor("WeaponHeavyString", value.WeaponHeavyString);
    visitor("WeaponKillsString", value.WeaponKillsString);
    visitor("WeaponCatMeleeString", value.WeaponCatMeleeString);
    visitor("WeaponCatRangedString", value.WeaponCatRangedString);
    visitor("WeaponDamageString", value.WeaponDamageString);
    visitor("TradeCostString", value.TradeCostString);
    visitor("TradeProfitString", value.TradeProfitString);
    visitor("TradeLossString", value.TradeLossString);
    visitor("TradeAlreadyOwnsString", value.TradeAlreadyOwnsString);
    visitor("TradeNumberInStockString", value.TradeNumberInStockString);
    visitor("TradeDeliveryString", value.TradeDeliveryString);
    visitor("TradeNoDeliveryString", value.TradeNoDeliveryString);
    visitor("TradeDaysString", value.TradeDaysString);
    visitor("TradeBuyString", value.TradeBuyString);
    visitor("TradeSellString", value.TradeSellString);
    visitor("TradeWantedString", value.TradeWantedString);
    visitor("QuestFailedString", value.QuestFailedString);
    visitor("FailedString", value.FailedString);
    visitor("SucceededString", value.SucceededString);
    visitor("Plus", value.Plus);
    visitor("Minus", value.Minus);
    visitor("CoreGraphic", value.CoreGraphic);
    visitor("VignetteGraphic", value.VignetteGraphic);
    visitor("OptionalGraphic", value.OptionalGraphic);
    visitor("FeatGraphic", value.FeatGraphic);
    visitor("ObjectsRewardString", value.ObjectsRewardString);
    visitor("NoneString", value.NoneString);
    visitor("CheckGuildString", value.CheckGuildString);
    visitor("QuestStartingString", value.QuestStartingString);
    visitor("RingCenter", value.RingCenter);
    visitor("PCRingCenter", value.PCRingCenter);
    visitor("WorldMapOffset", value.WorldMapOffset);
    visitor("WorldMapWidth", value.WorldMapWidth);
    visitor("WorldMapHeight", value.WorldMapHeight);
    visitor("YouString", value.YouString);
    visitor("OwnString", value.OwnString);
    visitor("NoString", value.NoString);
    visitor("HousesString", value.HousesString);
    visitor("HouseString", value.HouseString);
    visitor("InString", value.InString);
    visitor("ShopsString", value.ShopsString);
    visitor("ShopString", value.ShopString);
    visitor("ThereString", value.ThereString);
    visitor("AreString", value.AreString);
    visitor("IsString", value.IsString);
    visitor("ForString", value.ForString);
    visitor("SaleString", value.SaleString);
    visitor("GeneralString", value.GeneralString);
    visitor("TitleString", value.TitleString);
    visitor("BarberString", value.BarberString);
    visitor("TatooString", value.TatooString);
    visitor("LevelString", value.LevelString);
    visitor("TotalSpellsInPalettes", value.TotalSpellsInPalettes);
    visitor("TotalSpellsInContainer", value.TotalSpellsInContainer);
    visitor("LogBookBasicsCategoryString", value.LogBookBasicsCategoryString);
    visitor("LogBookObjectsCategoryString", value.LogBookObjectsCategoryString);
    visitor("LogBookTownsCategoryString", value.LogBookTownsCategoryString);
    visitor("LogBookHeroCategoryString", value.LogBookHeroCategoryString);
    visitor("LogBookCombatCategoryString", value.LogBookCombatCategoryString);
    visitor("LogBookQuestCategoryString", value.LogBookQuestCategoryString);
    visitor("LogBookStoryCategoryString", value.LogBookStoryCategoryString);
    visitor("LogBookBasicsCategoryNameString", value.LogBookBasicsCategoryNameString);
    visitor("LogBookObjectsCategoryNameString", value.LogBookObjectsCategoryNameString);
    visitor("LogBookTownsCategoryNameString", value.LogBookTownsCategoryNameString);
    visitor("LogBookHeroCategoryNameString", value.LogBookHeroCategoryNameString);
    visitor("LogBookCombatCategoryNameString", value.LogBookCombatCategoryNameString);
    visitor("LogBookQuestCategoryNameString", value.LogBookQuestCategoryNameString);
    visitor("LogBookStoryCategoryNameString", value.LogBookStoryCategoryNameString);
    visitor("MapPaths", value.MapPaths);
    visitor("SoundUpDown", value.SoundUpDown);
    visitor("SoundSlider", value.SoundSlider);
    visitor("SoundBack", value.SoundBack);
    visitor("SoundForward", value.SoundForward);
    visitor("SoundError", value.SoundError);
    visitor("SoundExit", value.SoundExit);
    visitor("CountUpSound", value.CountUpSound);
    visitor("SoundKeyboardUp", value.SoundKeyboardUp);
    visitor("SoundKeyboardDown", value.SoundKeyboardDown);
    visitor("SoundKeyboardLeft", value.SoundKeyboardLeft);
    visitor("SoundKeyboardRight", value.SoundKeyboardRight);
    visitor("SoundKeyboardEnterCharacter", value.SoundKeyboardEnterCharacter);
    visitor("SoundKeyboardDeleteCharacter", value.SoundKeyboardDeleteCharacter);
    visitor("SoundKeyboardDone", value.SoundKeyboardDone);
    visitor("FrontEndMusic", value.FrontEndMusic);
    visitor("HeroDollTL", value.HeroDollTL);
    visitor("HeroDollBR", value.HeroDollBR);
    visitor("HeroDollSphereRadius", value.HeroDollSphereRadius);
    visitor("HeroDollTL_PC", value.HeroDollTL_PC);
    visitor("HeroDollBR_PC", value.HeroDollBR_PC);
    visitor("HeroDollSphereRadius_PC", value.HeroDollSphereRadius_PC);
    visitor("HeroDollFrameTL_PC", value.HeroDollFrameTL_PC);
    visitor("HeroDollFrameEmulateListOffset", value.HeroDollFrameEmulateListOffset);
    visitor("QuestStartScreenMusic", value.QuestStartScreenMusic);
    visitor("QuestCompleteScreenMusic", value.QuestCompleteScreenMusic);
    visitor("QuestFailureScreenMusic", value.QuestFailureScreenMusic);
    visitor("DeathScreenMusic", value.DeathScreenMusic);
    visitor("DigitCountTime", value.DigitCountTime);
    visitor("SaveHeroGraphicIndex", value.SaveHeroGraphicIndex);
    visitor("MiniMapGraphics", value.MiniMapGraphics);
    visitor("KeyboardSmallKeyGraphic", value.KeyboardSmallKeyGraphic);
    visitor("KeyboardLargeKeyGraphic", value.KeyboardLargeKeyGraphic);
    visitor("TotalAssignableThings", value.TotalAssignableThings);
    visitor("TimeInSecsForFade", value.TimeInSecsForFade);
    visitor("BackBufferFilterSaturation", value.BackBufferFilterSaturation);
    visitor("BackBufferFilterContrast", value.BackBufferFilterContrast);
    visitor("BackBufferFilterBrightness", value.BackBufferFilterBrightness);
    visitor("BackBufferFilterTintR", value.BackBufferFilterTintR);
    visitor("BackBufferFilterTintG", value.BackBufferFilterTintG);
    visitor("BackBufferFilterTintB", value.BackBufferFilterTintB);
    visitor("BackBufferFilterTintScale", value.BackBufferFilterTintScale);
    visitor("BackBufferDiffuseScale", value.BackBufferDiffuseScale);
    visitor("BackBufferAmbientScale", value.BackBufferAmbientScale);
    visitor("MinimumFilterColor", value.MinimumFilterColor);
}

} // namespace defs
} // namespace fable

#endif // FABLE_DEFS_VISIT_HPP
