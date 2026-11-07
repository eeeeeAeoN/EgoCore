// Fable def classes, compound values and their dependencies.
//
// Recovered from FableWin.pdb with fable-decomp-engine (`decomp-cli tree`).
// Field order is the original declaration order; this is a data model, not
// the on-disk wire layout.
#ifndef FABLE_DEFS_CLASSES_HPP
#define FABLE_DEFS_CLASSES_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fable/defs/base.hpp>
#include <fable/defs/enums.hpp>
#include <fable/defs/fwd.hpp>

namespace fable {
namespace defs {

class C2DBoxF
{
public:
	float TLX;
	float TLY;
	float BRX;
	float BRY;
};

class C2DBoxI
{
public:
	std::int32_t TLX;
	std::int32_t TLY;
	std::int32_t BRX;
	std::int32_t BRY;
};

class C2DVector
{
public:
	float X;
	float Y;
};

class C3DVector
{
public:
	float X;
	float Y;
	float Z;
};

class CAICreatureWillPowerIndicatorDef
{
public:
	float RechargeTime;
};

class CAIScratchpadDef
{
public:
	std::uint32_t ThankingPhrase;
	std::uint32_t IgnoringPhrase;
	C3DVector WanderCentrePoint;
	float WanderMinDistance;
	float WanderMaxDistance;
	std::int32_t GossipCounter;
	std::int32_t MaxGossipPhrase;
	std::uint32_t WarningPhrase;
	std::uint32_t BeerRequestPhrase;
	EScriptingStateGroups ScriptingStateGroup;
	float MaxHeroReactionDistance;
	std::int32_t ActionFrequency;
	float ActionFrequencyVariation;
	std::string Action;
	bool FaceHeroForAction;
	std::string TargetName;
	float FollowDistance;
	bool AttackHeroOnSight;
	std::int32_t TimeToSpendHarassingHero;
	float CombatNearbyEnemyFleeingBreakOffRange;
	float CombatNearbyBreakOffRange;
	bool StealStealableItems;
	bool RecoverStealableItems;
	bool TakeStealableItemToRandomDestination;
	bool KillSelfAndStealableItemAfterReachingDestination;
	bool AllowedToFollow;
	std::string TableName;
	std::string SeatName;
	bool DisableHeadLooking;
	bool IsPushableByHero;
	bool LookForFiniteTime;
	bool AvoidRegionExits;
	float TargetingDistanceOffset;
};

class CAbilityButtonGuiDef
{
public:
	EControllerType ControllerType;
	EXboxControllerButton ControllerButton;
	EMouseButtonControl MouseButton;
	std::int32_t ButtonGraphic;
	C2DVector Position;
	C2DVector Offset;
};

class CAbilityDef
{
public:
	EHeroAbility Ability;
};

class CAbilityTrackLevelDef
{
public:
	std::int32_t ExperienceCost;
	EHeroAbility AbilityUnlocked;
	std::int32_t Description;
};

class CAbilityTrackDef
{
public:
	EHeroExperienceStatCategory ExperienceStat;
	std::vector<CAbilityTrackLevelDef> Abilities;
	std::int32_t MaxedOutDescription;
};

class CActionInputControl
{
public:
	enum EActionDirection : std::int32_t
	{
		DIRECTION_NONE = 0,
		DIRECTION_UP = 1,
		DIRECTION_DOWN = 2,
		DIRECTION_LEFT = 3,
		DIRECTION_RIGHT = 4,
	};

	EGameAction GameAction;
	EControllerType ControllerType;
	EInputKey KeyboardKey;
	EXboxControllerButton XboxButton;
	EMouseButtonControl MouseButton;
	C2DVector ControlDirection;
};

class CActionUseDef
{
public:
	CDefString AnimationGroup;
	bool SnapToActionPoint;
	bool IgnoreActionPoints;
	ETutorialCategory TutorialCategory;
};

class CActivateQuestDef
{
public:
	CDefString ScriptName;
	bool LoadResources;
};

class CAnimComponentBase
{
public:
};

class CAnimComponentMap
{
public:
	std::vector<std::pair<EAnimComponent,std::shared_ptr<CAnimComponentBase>>> Map;
};

class CAnimationComponent
{
public:
	std::uint32_t Tag = 0;
	std::int32_t Flags;
	std::int32_t TransitionInTime;
	std::int32_t Delay;
	std::int32_t StartHandedness;
	std::int32_t EndHandedness;
	std::int32_t ComboStage;
	std::int32_t ComboId;
	std::int32_t RecoilAnimIndex;
	bool MeleeFlourish;
	bool MeleeKnockdown;
	bool TargetOffset;
	float TargetOffsetVectorX;
	float TargetOffsetVectorY;
	float AnimationSpeedMultiplier;
	std::int32_t Mode;
	CDefString NextAnimName;
	CDefString NextFilter;
	std::string ResponseAnimName;
};

class CAnimationEntry
{
public:
	std::int32_t BankIndex;
	CDefString AnimName;
	CDefString GroupName;
	std::vector<CAnimationComponent> Components;
};

class CAnimationSet
{
public:
	std::vector<std::pair<std::uint32_t,CAnimationEntry>> Anims;
	std::int32_t DefaultFlags;
	std::int32_t DefaultTransitionTime;
	std::int32_t DefaultDelay;
	CDefString DefaultGroup;
};

class CAnimatingObjectDef
{
public:
	CAnimationSet Animation;
};

class CEngineGraphic
{
public:
	EEngineGraphicType Type;
	std::int32_t BankIndex;
	float AnimStep;
	float RenderSizeX = 1.0f;
	std::uint8_t AdditiveAlpha;
};

class CAppearanceDef
{
public:
	CEngineGraphic Graphic;
	CAnimationSet Animation;
	std::int32_t OutlineEffectAlphaRef;
	std::int32_t OutlineEffectAlphaBias;
	bool OverrideLightingChannel;
	ELightingChannel LightingChannel;
	CEngineGraphic FadeOutWhenCloseGraphic;
	bool HasFadeOutWhenCloseGraphic;
	float FadeOutGraphicRenderFadeDistance;
	float FadeOutGraphicNearRenderFadeDistance;
	float FadeOutGraphicNearRenderClipDistance;
};

class CAppearanceModifierGraphics
{
public:
	class CEntry
	{
	public:
		EEngineGraphicType Type;
		EHeroMorphType MorphType;
		std::int32_t BankIndex;
		float RenderSizeX;
		float StartMorphStrength;
		float EndMorphStrength;
	};

	std::vector<CAppearanceModifierGraphics::CEntry> Graphics;
};

class CAppearanceModifierDef
{
public:
	std::vector<std::string> HideMaterials;
	CAppearanceModifierGraphics MorphGraphics;
	std::int32_t CoversBodyAreaFlags;
	bool IsRemoveable;
	std::int32_t HeroSuit;
	EClothingSuitPart SuitPart;
	EHeroAttachableAppearanceModifierType AppearanceType;
	float SoundRadiusMultiplier;
	float VisibilityMultiplier;
	std::int32_t ArmourDefIndex;
	float OveralProtectionWeighting;
	float Scariness;
	float Attractiveness;
	float Goodstrength;
};

class CAppearanceModifierScalingDef
{
public:
	float Attractiveness;
	float Scariness;
	float Goodstrength;
};

class CAreaOfEffectAttackDef
{
public:
	std::int32_t TrapIndex;
	std::int32_t ExplosionIndex;
	float ObstructionRadius;
	float ObstructionLifeTime;
};

class CArenaCreatureDef
{
public:
	std::string CreatureType;
	std::int32_t NumCreatures;
	std::string HUDType;
	std::int32_t DeathScore;
};

class CArenaWaveDef
{
public:
	std::int32_t NumWaveCreatures;
	std::vector<CArenaCreatureDef> Creatures;
	bool ShortWave;
};

class CArenaRoundDef
{
public:
	std::int32_t NumWaves;
	std::vector<CArenaWaveDef> Waves;
};

class CArmourDef
{
public:
	std::map<EObjectAugmentationType,float> AugmentationResponse;
	std::map<EDamageAttribute,float> DamageTypeResponse;
	std::map<EDamageAttribute,float> ArmourThresholds;
	bool AllHitsKnockdown;
	bool AllHitsNegated;
	bool AllHitsCauseRecoil;
	std::int32_t DamageMaterial;
	std::int32_t BlockMaterial;
};

class CAtmosBankEntry
{
public:
	std::uint32_t ID;
	CDefString Filename;
};

class CAttackHistoryCombo
{
public:
	std::vector<std::pair<EAttackHistoryType,EDamageAttribute>> AttackCombo;
	float Multiplier;
};

class CAttackPatternDef
{
public:
	std::vector<std::int32_t> AttackPattern;
};

class CAugmentationDef
{
public:
	std::uint32_t Name;
	EObjectAugmentationType Type;
	std::uint32_t InventoryGraphic;
	float DamageMultiplier;
	float ExperienceMultiplier;
	float FrequencyToAddHealthInSeconds;
	std::int32_t HealthToAddPerIncrease;
	float FrequencyToAddStaminaInSeconds;
	std::int32_t StaminaToAddPerIncrease;
};

class CBalverineBattleDef
{
public:
	float SecondsBetweenLunges;
	float LungeAttackDamage;
	float TimeInTrees;
};

class CBedDef
{
public:
	CDefString GetInBedAnimName;
	CDefString SleepInBedAnimName;
	CDefString GetUpFromBedAnimName;
	CDefString GetUpFromBedFastAnimName;
	bool Animating;
};

class CBettingDef
{
public:
	float BetX;
	float BetY;
	float MoneyX;
	float MoneyY;
	std::int32_t MinBet;
	std::int32_t MaxBet;
	std::int32_t BetIncrement;
	std::uint32_t Bet;
	std::uint32_t PlayersMoney;
	std::uint32_t Question;
	std::uint32_t Continue;
	std::uint32_t Cancel;
};

class CBlendedParticleEffectSet
{
public:
	std::int32_t Particle1Index;
	std::int32_t Particle2Index;
	float Particle1Value;
	float Particle2Value;
};

class CBlurredAttitudesDef
{
public:
	std::vector<EOpinionAttitudeType> Attitudes;
};

class CBoastingPodiumDef
{
public:
	float HeroOnPodiumRadius;
};

class CBonusItemDef
{
public:
	float HealthModifier;
	bool AddsFatness;
	std::int32_t StaminaModifier;
	float SecondsToApplyHealthIncreaseOver;
	std::int32_t WillExperience;
	std::int32_t SkillExperience;
	std::int32_t StrengthExperience;
	std::int32_t MoralityChange;
	float AgreeablenessChange;
	float AttractivenessChange;
	float ScarinessChange;
	float MaxHealthIncrease;
	std::int32_t MaxStaminaIncrease;
	bool ChangeTimeOfDayToDay;
	bool ChangeTimeOfDayToNight;
	float SpeedMultiplier;
	float SecondsForSpeedMultiplierToLast;
	std::int32_t ParticleEffect;
	bool AddsDrunkenness;
	float AlcoholicStrength;
};

class CBossDef
{
public:
	std::vector<std::int32_t> PhaseHealthPercent;
	std::vector<std::int32_t> Shots;
	std::vector<std::int32_t> TwinBladeMissesBeforeLunge;
	std::int32_t TwinBladeStuckLoops;
	float TwinBladeTimeBeforeLunge;
};

class CBrainBehaviour
{
public:
	CDefString Name;
	std::int32_t Priority;
	std::int32_t ScriptAIPriority;
};

class CBrainUpdateZone
{
public:
	std::int32_t Distance;
	std::int32_t UpdateFrequency;
	std::int32_t InterruptionFrequency;
};

class CBrainDef
{
public:
	std::vector<CBrainBehaviour> Behaviours;
	std::vector<CDefString> DisabledBehaviours;
	std::vector<CBrainUpdateZone> UpdateZones;
};

class CBriarRoseDef
{
public:
	float SecondsToTeleport;
	float SecondsToBeIntangible;
	float SecondsToChargeDrainLife;
	std::int32_t GuranteedImpostersHitBeforeAllowedToHitBriar;
	std::int32_t NumberOfHitsToKillImposters;
	std::int32_t EnvironmentThemeForIntangibleMode;
	float SecondsForIntangibleEnvironmentThemeTransitionIn;
	float SecondsForIntangibleEnvironmentThemeTransitionOut;
	float SecondsBetweenWillUse;
	float SecondsBetweenTeleportAway;
	float DistanceForcePushThreshold;
};

class CBuyHouseDef
{
public:
	bool Main;
};

class CBuyableHouseDef
{
public:
	bool IsBuyable;
	bool InitiallyEmpty;
	std::vector<std::int32_t> Price;
	std::vector<std::int32_t> Rent;
	std::int32_t DaysBetweenRentPayments;
	std::int32_t MaxRentBags;
	bool IsScripted;
	bool IsBuyableWithoutSignpost;
};

class CCameraCollisionDef
{
public:
	std::int32_t CameraCollisionMesh;
};

class CCameraManagerDef
{
public:
	std::vector<std::int32_t> CameraList;
	std::vector<std::int32_t> SpecialityCameraList;
	std::int32_t InitialMode;
	std::int32_t ZTargetMode;
	std::int32_t ProjectileWeaponMode;
	float BaseHeightToKeepAboveFlatLandscape;
	float BaseHeightToKeepAboveWater;
	float MaxHeightRelativeToGround;
	float MaxHeightStepForPointAcceptance;
	float AngleBetweenGroundNormalAndVerticalForLandscapeToBeConsideredNotFlat;
};

class CCameraManagerSetDef
{
public:
	std::int32_t CameraManagerMain;
	std::int32_t CameraManagerCombat;
	std::int32_t CameraManagerPCMain;
	std::int32_t CameraManagerPCCombat;
};

class CCameraModeDef
{
public:
	std::int32_t CameraMode;
	float StringLengthXY;
	float CageRadius;
	float HeightOffsetZ;
	float LookOffsetZ;
	float FOV;
	float RollAngle;
	std::int32_t TransitionInFrames;
	float LazyLookAtPosWeight;
	bool RaiseUpWhenClose;
	bool StretchAroundHero;
	float ZTargetBaseOffset;
	float ZTargetLookAtTargetOffset;
	float ZTargetStringLengthToObservedThing;
	float ZTargetStringLengthPerpendicularToObservedThing;
	bool PerformExtendedOcclusionTests;
	float XAxisDisplacementCoupling;
	float XAxisVelocityDamping;
	float XAxisAttractorCoupling;
	float XAxisAttractorFallOff;
	float YAxisDisplacementCoupling;
	float YAxisVelocityDamping;
	float YAxisAttractorCoupling;
	float YAxisAttractorFallOff;
	bool ProjectileCameraFOVZoom;
	bool ProjectileCameraZoomToggle;
	float ProjectileCameraDefaultTranslationalZoomOffset;
	float ProjectileCameraInitialPhiOffsetForCamera;
	float ProjectileCameraInitialHeightParamForCamera;
	float PhiRotationCouple;
	float ThetaRotationCouple;
	float DollyLength;
	std::int32_t NumberOfFramesToAverageInputsOver;
	float DollyLengthBlendResistance;
	float ThetaAngleLimitUp;
	float ThetaAngleLimitDown;
	float SecondsToReset;
	float ViewHeroXYOffsetMultiplier;
	float ViewHeroXYRotationMaxVelocity;
	float ViewHeroHeightParamMaxVelocity;
	float ViewHeroGroundHeightOffset;
};

class CCardDef
{
public:
	std::int32_t CardName;
	std::int32_t CardVal;
};

class CCardPositionsDef
{
public:
	std::vector<C2DVector> Offset;
};

class CCarriedReadableDef
{
public:
	CDefString TitleTextTag;
	CDefString BodyTextTag;
};

class CCarrySlotDef
{
public:
	CDefString DummyPosName;
	std::int32_t DummyPosIndex;
	bool PrimarySlot;
	CDefString SecondaryDummyPosName;
};

class CCarryableDef
{
public:
	std::int32_t ActiveCarrySlot;
	std::int32_t SecondaryActiveCarrySlot;
	std::int32_t PassiveCarrySlot;
	std::int32_t OnKillFX;
	C3DVector OffsetCoordRelativeToAttachToDummy;
	float OffsetAmountForThingsOnTopOfMe;
	float PassiveCarrySlotScale;
	bool IncludeInThingScansWhenCarried;
};

class CCarryingDef
{
public:
	std::vector<std::int32_t> AvailableCarrySlots;
	CDefString OverriddenDummyObject;
};

class CChestDef
{
public:
	bool PersistOnOpening;
	std::int32_t OpenParticleEffect;
	bool DisplayMessageOnEmpty;
	std::string OpenAnimationForCreature;
	std::int32_t OpenerObject;
	std::int32_t OpenersRequired;
};

class CClockDef
{
public:
	std::map<std::string,std::int32_t> Sound;
	EClockHandType HandType;
};

class CCoinGameObstacleDef
{
public:
	float Radius;
};

class CCombatAbilityData
{
public:
	std::int32_t PercentageChance = 100;
	float SecondsDuration = -1.0f;
	float SecondsToWaitBeforeRepeat;
	CDefString Anim;
};

class CCombatAbilityMeleeAttackData : public CCombatAbilityData
{
public:
	bool Blockable;
	bool Knockdown;
	float Damage;
};

class CCombatAbilityAttackBase
{
public:
	CCombatAbilityMeleeAttackData CombatAbility;
};

class CCombatAbilityBlockCounterAttackDef : public CCombatAbilityAttackBase
{
public:
};

class CCombatAbilityBlockingData : public CCombatAbilityData
{
public:
	float BlockAngle;
	bool BlockCanBeBroken;
	bool ForceRecoilWhenBlocking;
	std::set<EWeaponClass> ValidBlockWeaponTypes;
	EWeaponClass WeaponClass;
};

class CCombatAbilityBlockDefBase
{
public:
	CCombatAbilityBlockingData CombatAbility;
};

class CCombatAbilityBlockHeavyWeaponAttackDef : public CCombatAbilityBlockDefBase
{
public:
};

class CCombatAbilityBlockLightWeaponAttackDef : public CCombatAbilityBlockDefBase
{
public:
};

class CCombatAbilityBlockProjectileWeaponAttackDef : public CCombatAbilityBlockDefBase
{
public:
};

class CCombatAbilityBlockUnarmedAttackDef : public CCombatAbilityBlockDefBase
{
public:
};

class CCombatAbilityFlourishCounterAttackDef : public CCombatAbilityAttackBase
{
public:
};

class CCombatAbilityGetHitCounterAttackDef : public CCombatAbilityAttackBase
{
public:
};

class CCombatAbilityProjectileAttackData : public CCombatAbilityData
{
public:
	bool AbleToStrafeAndShoot;
	std::int32_t ShotAccuracyPercentage;
};

class CCombatAbilityStrafingData : public CCombatAbilityData
{
public:
	float RotationSpeed;
};

class CCombatAbilityStrafeDef
{
public:
	CCombatAbilityStrafingData CombatAbility;
};

class CCombatAbilityUseProjectileWeaponDef
{
public:
	CCombatAbilityProjectileAttackData CombatAbility;
};

class CCombatDialogueDef
{
public:
	float MinTimeBetweenSpeech;
	float MinTimeBetweenSpeechRepeat;
	float MinDelayAfterBattleCry;
	std::int32_t EnemySighted;
	std::int32_t RunForCover;
	std::int32_t Hiding;
	std::int32_t Ambushing;
	std::int32_t WillSeenUsed;
	std::int32_t Threat;
	std::int32_t Encourage;
	std::int32_t FoeKnockedDownByColleague;
	std::int32_t MenLosing;
	std::int32_t EnemyKnockedDown;
	std::int32_t RangedCombatAiming;
	std::int32_t RangedCombatAimingBlocked;
	std::int32_t Injured;
	std::int32_t ReactionToDecapitation;
	std::int32_t ColleagueInjured;
	std::int32_t Scared;
	std::int32_t Flee;
};

class CCombatSequenceDef
{
public:
	std::int32_t ValidCombatZones;
	ECombatSequenceType Type;
	ECombatSequenceInterruptionType InterruptionType;
	ECombatSequenceIsValidType IsValidType;
	ECombatSequenceOnStartModuleType OnStartType;
	ECombatSequenceOnStopModuleType OnStopType;
	std::vector<EActionRegisteredType> ActionList;
	float SecondsToPredictTargetPosition;
	bool PreferIfValid;
};

class CCombatTypeDef
{
public:
	std::vector<std::int32_t> DefenderCombatRingOccupationNumbers;
	std::int32_t DefenderWheelOuterRingStartIndex;
	bool DefenderWheelAllowAttackerStrafing;
	std::vector<std::string> CombatSequence;
	std::vector<std::int32_t> CombatSequenceDef;
	bool PreferOuterRing;
	float FrontFarLimit;
	float FrontMiddleLimit;
	float FrontNearLimit;
	float SideLimit;
	float RearLimit;
	float PreferredCombatDistance;
	std::map<ECombatCreatureType,std::int32_t> AttackPatterns;
	ECombatCreatureType CombatCreatureType;
	bool AlwaysAttackIfNearest;
	bool KnockdownCausesStun;
	ECombatStrikeRecoilStyle RecoilStyle;
	bool CanBeKnockedDown;
	float ComboContinueSpeed;
	float CombatFOV;
	bool StartsAsReadyForMelee;
};

class CComboMultiplierData
{
public:
	float ExperienceMultiplierNumSecondsForAttenuation;
	float ExperienceMultiplierParamDecayDeltaPerSecond;
	std::map<EDamageAttribute,float> ComboMultiplier;
};

class CConfigOptionsDefaultsDef
{
public:
	std::uint32_t Antialiasing;
	std::int32_t ResolutionWidth;
	std::int32_t ResolutionHeight;
	std::int32_t BitDepth;
	std::int32_t MinResolutionWidth;
	std::int32_t MinResolutionHeight;
	float TextureDetail;
	float MaxTextureDetail;
	float ShadowDetail;
	float MaxShadowDetail;
	float MeshDetail;
	float MaxMeshDetail;
	float EffectsDetail;
	float MaxEffectsDetail;
};

class CContainerRewardHeroDef
{
public:
	std::vector<CDefIndex> ObjectFamilies;
};

class CContextSensitiveItemDef
{
public:
	EContextSensitiveType Type;
	std::int32_t Priority;
	std::int32_t SlotIndex;
	std::int32_t ExtraData1;
	std::int32_t ExtraData2;
};

class CControlsDef
{
public:
	std::vector<CActionInputControl> Controls;
	bool ToggleZTarget;
	bool ToggleSpells;
	bool ToggleSneak;
	bool ToggleExpressionMenu;
	bool ToggleExpressionShift;
	bool FlourishNeedsAttackButtonHeld;
};

class CConversation
{
public:
	std::int32_t Lines;
	std::vector<std::string> Speaker;
	std::vector<std::string> Dialogue;
	std::vector<std::string> Animation;
	std::vector<std::string> AnimLoop;
};

class CCoopSpiritDef
{
public:
	CDefString OnHitParticle;
	CDefString OnStrikeParticle;
	CDefString DefaultParticle;
	CDefString AttackTargetParticle;
	float MeleeTargetRange;
};

class CCrateStackDef
{
public:
	std::int32_t StackType;
	std::int32_t Priority;
};

class CCreatureAbilityDef
{
public:
	ECreatureAbility Type;
};

class CFloatRange
{
public:
	float HighValue;
	float LowValue;
};

class CHeroAbilityStatWeight
{
public:
	std::map<EHeroTrainableStatType,float> StatWeights;
};

class CCreatureDef
{
public:
	std::shared_ptr<CWoundMorphs> PWoundMorphs;
	std::shared_ptr<CRandomAppearanceMorph> PRandomAppearanceMorph;
	std::shared_ptr<CExpressionSet> PExpressions;
	std::shared_ptr<CCreatureStatsDef> PStats;
	std::vector<std::int32_t> InitialActions;
	std::vector<std::int32_t> InitialAppearanceModifiers;
	std::map<std::string,std::int32_t> PhonemeAnim;
	std::vector<CDefString> DialogueVoices;
	std::int32_t Inventory;
	std::int32_t ClothingInventory;
	std::int32_t WeaponsInventory;
	std::int32_t HeroAbilitiesScreenInventory;
	std::int32_t QuestCardScreenInventory;
	std::int32_t MapScreenInventory;
	std::int32_t StatsScreenInventory;
	std::int32_t MagicScreenInventory;
	std::int32_t ExperienceScreenInventory;
	std::int32_t TradeScreenInventory;
	std::int32_t AugmentScreenInventory;
	std::int32_t QuestsScreenInventory;
	std::int32_t InGameMenuScreen;
	std::uint32_t CreatureGroup;
	CDefString BattleCrySound;
	std::int32_t OpinionSourceDef;
	std::int32_t RespawnWaitInDaysMin;
	std::int32_t RespawnWaitInDaysMax;
	std::int32_t RespawnWaitInFramesMin;
	std::int32_t RespawnWaitInFramesMax;
	bool Short;
	bool FlashOnHit;
	bool PickPocketable;
	bool ChildThatCanRegionFollow;
	bool AllowedInProtectedTowns;
	bool UseActualHitPosForHitEffects;
	CHeroAbilityStatWeight RadiusWeight;
	CFloatRange RadiusSpread;
	bool AGhostIsWhatIBe;
};

class CCreatureGenerationFamilyDef
{
public:
	std::vector<std::int32_t> Creatures;
	std::int32_t DifficultyLevel;
	std::int32_t GeneratorObject;
	std::vector<ECreatureGeneratorGenerateType> GenerationTypes;
};

class CCreatureGeneratorDef
{
public:
	std::vector<std::int32_t> RandomEffects;
	CDefString RandomSoundCriteria;
	std::int32_t SecondsBetweenEffects;
	std::int32_t SecondsBetweenSoundEffects;
};

class CCreatureModeDef
{
public:
	std::int32_t DefaultCreatureMode;
	std::int32_t InitialCreatureMode;
	std::int32_t DefaultWeaponCreatureMode;
	std::vector<std::int32_t> AvailableIdleModes;
};

class CCreatureNavigationDef
{
public:
	float MinPathCost;
	float MaxPathCost;
	float Radius;
	bool AvoidDynamicObstacles;
};

class CCreatureStatsDef
{
public:
	std::int32_t MoralityForKill;
	std::int32_t MoralityForPunch;
	std::int32_t MoralityForWeaponHit;
	std::int32_t ExperienceWorth;
	std::int32_t RenownWorth;
	float Speed;
	float Dexterity;
	float StrengthDamageMultiplier;
};

class CCutsceneDef
{
public:
	std::vector<std::string> Macro;
	std::vector<std::string> SkipCond;
	std::vector<std::string> Lights;
	std::vector<std::string> LightScene;
	std::vector<std::string> SetupCond;
	std::vector<std::string> Sound;
	std::vector<std::string> Answer0;
	std::vector<std::string> Answer1;
};

class CDecapitationDef
{
public:
	std::int32_t BodyParticleEffect;
	std::int32_t Head;
};

class CDegradableInfo
{
public:
	float HealthPercentage;
	std::int32_t BankIndex;
	float AnimStep;
	float RenderSizeX = 1.0f;
	std::uint8_t AdditiveAlpha;
	std::uint8_t Type;
	std::int32_t SmashParticleEmitter;
	bool BlocksNavigation;
	std::uint8_t Skip[4];
};

class CDegradableDef
{
public:
	class CIsHealth1GTHealth2
	{
	public:
	};

	class CSetEngineGraphicType
	{
	public:
		EEngineGraphicType GraphicType;
	};

	bool Degradable;
	EEngineGraphicType GraphicType;
	bool InitiallyBlocksNavigation;
	std::vector<CDegradableInfo> Degradations;
};

class CDoorDef
{
public:
	EDoorTriggerType TriggerType;
	float TriggerRadius;
	std::int32_t OpenCollisionMesh;
	std::int32_t ClosedCollisionMesh;
	bool PostVandalismCrimeIfDestroyed;
	bool LockPickable;
	bool PreventLockingByOwner;
};

class CDragonActionHoverDef
{
public:
	float WindSpeed;
};

class CDragonActionNapalmDef
{
public:
	float ChargeUpTime;
	float WindSpeed;
	float TimeBeforeAddOfMouthEffect;
	float TimeBeforeAddExplosion;
};

class CDragonActionSwoopDef
{
public:
	float IntoDelay;
	float OutOfDelay;
	float Distance;
	float MidPointsControlOffset;
	float StartHeight;
	float MidHeight;
	float EndHeight;
	float FlightSpeed;
	float MaxSplineProgressForSplineAdjustment;
	float MaxSplineProgressForBurninationEffect;
	float MinSplineProgressForWingFlap;
	float MaxExplosionRange;
	float ExplosionLineProjectionAngle;
	std::int32_t FramesBetweenExplosions;
	float WindSpeed;
};

class CDrunkennessDef
{
public:
	float DrunkennessThresholdMult;
};

class CEnemyDef
{
public:
	std::int32_t Faction;
};

class CEngineDef
{
public:
	float LODErrorTolerance;
	float CharacterLODErrorTolerance;
	float LODErrorFactor;
	float SeaHeight;
	float GamePrimitiveDefaultFadeStart;
	float GamePrimitiveDefaultFadeRangeRatio;
	float LocalDetailDefaultFadeStart;
	float LocalDetailDefaultFadeRangeRatio;
	float FOV_2D;
	std::int32_t LocalDetailBooleanAlphaDefaultAlphaRef;
	std::int32_t DefaultPrimitiveAlphaRef;
	std::int32_t TestStaticMesh;
	std::int32_t TestAnimatedMesh;
	std::int32_t TestAnim;
	std::int32_t TestGraphic;
	std::int32_t InvalidTextureStandin;
	std::int32_t InvalidThemeStandin;
};

class CEngineLocalDetailObjectDef
{
public:
	std::int32_t Mesh;
	std::int32_t ShadowMesh;
	float Probability;
	float ThemeBlendThreshold;
	float Scale;
	float ScaleRandomElement;
	float FadeStart;
	float FadeEnd;
	std::int32_t ZSpriteMesh;
	float ZSpriteFadeStart;
	float ZSpriteFadeEnd;
	float SlopeFadeStart;
	float SlopeFadeEnd;
	std::int32_t AlphaRef;
	float AlphaMipBias;
	float WindSkewConstantFactor;
	float WindSkewRandomFactor;
	float WindSkewSpeedFactor;
	bool CastShadows : 1;
	bool ReceiveShadows : 1;
	bool IsRepeatedMesh : 1;
	bool IsZSprite : 1;
	bool AlphaIsBoolean : 1;
	bool HasLandscapeNormalLighting : 1;
	bool TiltToSlope : 1;
	bool HasWindSkew : 1;
};

class CEngineLocalDetailLayerDef
{
public:
	std::vector<float> SpacingFromLayer;
	std::vector<CEngineLocalDetailObjectDef> Objects;
};

class CEngineLocalDetailGeneratorDef
{
public:
	std::vector<CEngineLocalDetailLayerDef> Layers;
};

class CEngineThemeDef
{
public:
	std::int32_t BaseTexture;
	std::int32_t BackgroundTexture;
	std::int32_t CliffBaseTexture;
	std::int32_t CliffBackgroundTexture;
	std::int32_t BaseBumpMap;
	std::int32_t CliffBumpMap;
	std::int32_t BaseTextureMaxSize;
	std::int32_t CliffTextureMaxSize;
	std::int32_t BaseBumpMapMaxSize;
	std::int32_t CliffBumpMapMaxSize;
	float BaseTextureSelfIllumination;
	float CliffTextureSelfIllumination;
	float WaterHeight;
	EWaterType WaterType;
	std::int32_t NoWaterThemeDef;
	std::int32_t LocalDetailGeneratorDef;
	std::int32_t GroupDef;
	float Height;
	float MinCameraHeight;
	float CoverValue;
	std::int32_t MaterialDef;
	EMinimapThemeType MinimapTheme;
	std::int32_t DestructionEffectDef;
	std::int32_t DestructionThemeDef;
	float Friction;
	bool Passable : 1;
};

class CEngineThemeGroupDef
{
public:
	bool Dummy;
};

class CEngineVideoOptionsDef
{
public:
	std::int32_t HiresTextureMemory;
	float LODErrorTolerance;
	float CharacterLODErrorTolerance;
	float DrawDistanceMultiplier;
	float DrawDistanceMinimum;
	float DrawDistanceMaximum;
	float RepeatedMeshDrawDistanceFactor;
	float MinimumZSpriteAsMeshDistance;
	float MaximumZSpriteAsMeshDistance;
	float ZSpriteDrawDistanceMultiplier;
	std::int32_t ShadowBufferSize;
	float ShadowDistanceScale;
	bool Enable2DDisplacement;
	bool Enable3DDisplacement;
	bool EnableGlow;
	bool EnableRadialBlur;
	bool EnableWaterReflection;
	bool EnableWeatherEffects;
	bool EnableColourFilter;
	bool EnableRepeatedMeshes;
	float WeatherDensity;
};

class CSoundPair
{
public:
	std::int32_t FirstSound;
	std::int32_t LastSound;
};

class CSoundMap
{
public:
	std::vector<std::pair<std::uint32_t,CSoundPair>> SoundMap;
	std::vector<std::pair<std::uint32_t,CDefString>> CriteriaMap;
};

class CEntitySoundDef
{
public:
	CSoundMap SoundMap;
	std::string SoundIdentifier;
	float VoicePitchOverride;
	std::int32_t AnimCriteriaClipDistance;
};

class CEnvironmentDef
{
public:
	std::string ColourLookupTexture;
	std::int32_t DiffuseLookupRow;
	std::int32_t AmbientLookupRow;
	std::int32_t BacklightLookupRow;
	std::int32_t ReflectionLookupRow;
	std::int32_t MistEffectColourLookupRow;
	std::int32_t FogColourLookupRow;
	std::int32_t FogAlphaLookupRow;
	std::int32_t SunColourLookupRow;
	std::int32_t CloudColourLookupRow;
	std::int32_t MoonColourLookupRow;
	std::int32_t StarsColourLookupRow;
	std::int32_t SunFlareColourLookupRow;
	std::int32_t LensFlareColourLookupRow;
	std::int32_t SkyGradientTopLookupRow;
	std::int32_t SkyGradientTopAlphaLookupRow;
	std::int32_t SkyGradientBottomLookupRow;
	std::int32_t SkyGradientBottomAlphaLookupRow;
	std::int32_t WaterColourLookupRow;
	std::int32_t SeaColourLookupRow;
	std::int32_t GlowThresholdColourLookupRow;
	std::int32_t GlowBloomColourLookupRow;
	std::int32_t SunlightAttenuatorColourLookupRow;
	float DiffuseClampAngle;
	float SunlightAttenuatorAngleFadeStart;
	float SunlightAttenuatorAngleFadeEnd;
	std::int32_t SeaTexture;
	std::int32_t SeaRadius;
	std::int32_t SeaFlatSectionStart;
	std::int32_t SeaFlatSectionEnd;
	float DayStartTime;
	float DaySpeed;
	std::int32_t WaterBumpMapPC;
	std::int32_t SeaBumpMapPC;
	std::int32_t IceBumpMapPC;
	std::int32_t IceBumpMap;
	std::int32_t IceTexture;
	std::int32_t WaterEdgeAlphaMap;
	std::int32_t WaterSurfMap;
	std::int32_t WaterBumpMap;
	std::int32_t WaterBumpMap2;
	std::int32_t SeaBumpMap;
	std::int32_t SeaBumpMap2;
	std::int32_t WaterEnvMapOverlayTexture;
	std::int32_t MistAlphaGraphic;
	std::int32_t MistAlphaGraphicPC;
	std::int32_t RainTexture;
	std::int32_t RainTexturePC;
	std::int32_t SnowTexture;
	std::int32_t RainSplashParticle;
	float LightningFadeInDuration;
	float LightningFadeOutDuration;
	float LightningFlashDuration;
	float LightningRainThreshold;
	std::int32_t LightningTheme;
	float CloudSpeedMultiplier;
	float CloudMaxSpeed;
	float CloudTextureCoordMultiplier;
	float CloudTextureCoordOffset;
	float CloudHeightOffset;
	float WaterLakeMinimumFlowSpeed;
	float WaterLakeMaximumFlowSpeed;
};

class CEnvironmentThemeDef
{
public:
	float TimeOfDay;
	bool MoonLit;
	float FogStartZ;
	float FogEndZ;
	std::int32_t SkyTexture0;
	std::int32_t SkyTexture1;
	float SkyTexture1Blend;
	std::int32_t CloudLowerLayerTexture0;
	std::int32_t CloudLowerLayerTexture1;
	float CloudLowerLayerTexture1Blend;
	float CloudLowerLayerSpeedMultiplier;
	std::int32_t CloudUpperLayerTexture0;
	std::int32_t CloudUpperLayerTexture1;
	float CloudUpperLayerTexture1Blend;
	float CloudUpperLayerSpeedMultiplier;
	float WaterColourToReflectionBlend;
	float WaterAlphaFactor;
	float WaterSpecularHightlightFactor;
	float RainStrength;
	float SnowStrength;
	float MistMaxAlpha;
	float LightningFrequency;
	float GlowThresholdScale;
	float GlowBloomScale;
	float GlowMotionBlur;
	float ShadowFactor;
	float FadedShadowFactor;
	float WaterRefractionBlendStart;
	float WaterRefractionBlendEnd;
	float WaterFlowSpeedFactor;
	float WaterOscilationSpeed;
	float WaterSwellFactor;
	float WaterShoreSwellFactor;
	float WaterReflectionOffset;
};

class CRGBColour
{
public:
	// Packed ARGB; retail's default ctor is opaque black (0xFF000000).
	std::uint32_t IntValue = 0xFF000000;
};

class CEnvironmentThemeDaySetDef
{
public:
	std::vector<CEnvironmentThemeDef> TimeTheme;
	float SunTilt;
	float SunRotate;
	float SunHeight;
	float MoonTilt;
	float MoonRotate;
	float MoonHeight;
	std::int32_t ColourLookupColumn;
	CRGBColour EditorColour;
	float FishWeightMult;
};

class CExperienceDef
{
public:
	std::int32_t OrbCombiningParticleEffect;
	std::map<std::int32_t,float> ParticleScalesPerExperiencePoints;
	std::map<std::int32_t,std::int32_t> PickupParticleEffectsPerExperiencePoints;
	std::map<std::int32_t,float> LifeTimeForDistanceThresholds;
};

class CExplodingObjectDef
{
public:
	bool TriggeredOnCreatureProximity;
	float TriggerRadius;
	std::int32_t ExplosionDef;
	std::int32_t ProximityParticleEffect;
};

class CExplosionRing
{
public:
	std::int32_t NumExplosions;
	float RingRadius;
	float AngleOffset;
	float SecondsBeforeSpawnNewRing;
};

class CExplosionDef
{
public:
	float Damage;
	float ExplosionRadius;
	std::int32_t FireDamage;
	float ExplosionLifeTime;
	bool DamageFalloffWithRadius;
	std::int32_t ExplosionBlendSmall;
	std::int32_t ExplosionBlendLarge;
	std::int32_t ExplosionReplacementBlendSmall;
	std::int32_t ExplosionReplacementBlendLarge;
	float SecondsBeforeEffectReplacement;
	float ExplosionRadiusSmall;
	float ExplosionRadiusLarge;
	std::vector<CExplosionRing> ExplosionRings;
	bool ExplosionShouldKnockdown;
	bool CauseFireEffectOnHitCreatures;
	bool CausesRumble;
	EQuakeStrength RumbleStrength;
	EQuakeLength RumbleLength;
};

class CExplosiveTrailDef
{
public:
	std::int32_t TrapDefIndex;
	float SecondsBetweenTraps;
	float DistanceBetweenTraps;
	float SecondsForTrapPersistance;
	float SecondsTrailLifetime;
	float DistanceForLateralDeviation;
	float DegreesMaxTurnTowardsTargetPerTrap;
	float FractionOfLifetimeToTurnTowardsTarget;
	float SecondsIcicleAttackTargetPositionPredictionTime;
};

class CExpressionDef
{
public:
	CDefString Animation;
	CDefString AnimationInto;
	CDefString AnimationLoop;
	CDefString AnimationOutOf;
	CDefString AnimationToOther;
	CDefString AnimationToOtherInto;
	CDefString AnimationToOtherLoop;
	CDefString AnimationToOtherOutOf;
	float SpriteDuration;
	std::int32_t SpriteGraphic;
	CDefString SpriteOverrideText;
	CDefString SoundCriteria;
	CDefString LoopSoundCriteria;
	CDefIndex DummyObject;
	EGameEventType GameEventType;
	CDefString ScriptName;
	bool ScriptLoadResources;
	bool DeactivateScriptAtEnd;
	bool PerformAfterScript;
	EOpinionDeedType OpinionDeedType;
	bool Extendable;
	bool SheatheWeapons;
	bool ReplaceHeldItemWhenDone;
	std::uint32_t TextTag;
	EExpressionInventoryType ExpressionType;
};

class CExpressionSet
{
public:
	class CEntry
	{
	public:
		EExpressionType Type;
		std::int32_t AnimIndex;
	};

	std::vector<std::pair<EExpressionType,CExpressionSet::CEntry>> Expressions;
	std::uint32_t TrailingU32 = 28011826;
};

class CExpressionSubDef
{
public:
	std::int32_t ExpressionDef;
};

class CFactionDef
{
public:
	std::vector<std::string> EnemyFactions;
	std::vector<std::string> AlliedFactions;
};

class CFeatDef
{
public:
	CDefString FeatName;
	CDefString Verb;
	float TimeLimit;
	std::int32_t TargetNumber;
	std::int32_t GoldReward;
	std::int32_t XPReward;
	CDefString ItemReward;
	bool NoBlocking;
	EFeatAttackType KN_AttackType;
	bool KN_Perfect;
	std::int32_t KN_CreatureType;
	CDefString GF_FromRegion;
	CDefString GF_ToRegion;
	bool GF_NoTeleporting;
	CDefString CO_ItemName;
};

class CFireEffectCreationSet
{
public:
	std::map<CDefString,std::int32_t> ContainmentMap;
};

class CFireHeartPatternDef
{
public:
	std::vector<bool> PadActive;
};

class CFireballSpellLevelDef
{
public:
	std::int32_t SpellLevel;
};

class CFireheartMinigameDef
{
public:
	std::string MoonFX;
	std::string SunFX;
	std::uint32_t QuitText;
	std::uint32_t YesText;
	std::uint32_t NoText;
};

class CFishDef
{
public:
	float MinLength;
	float MaxLength;
	float MinWeight;
	float MaxWeight;
	float MinSpeed;
	float MaxSpeed;
	float BasePullSeconds;
	float MaxRandomPullSeconds;
	float BasePullWaitSeconds;
	float MaxRandomPullWaitSeconds;
	std::int32_t Level;
	std::int32_t InfoText;
};

class CFishingDef
{
public:
	std::vector<std::int32_t> FishTypes;
	float FishingSpotProximity;
	float FishingSpotMinDepth;
	float FishingSpotMinHeightDiff;
	float FishingSpotMaxHeightDiff;
	std::int32_t PointsPerFishingLevel;
	float SlowFishingSpeed;
	float NormalFishingSpeed;
	float FastFishingSpeed;
	float FastestFishingSpeed;
	float FishWeightRainMult;
	float FishWeightSnowMult;
	float FishWeightMistMult;
	float BaseWaitPeriodSeconds;
	float MaxRandomWaitPeriodSeconds;
	float HookWindowSeconds;
	std::int32_t BaseHookPresses;
	std::int32_t MaxRandomHookPresses;
	float StrainIncreasePerPress;
};

class CFishingRodDef
{
public:
	float StrainReductionPerSecond;
	float DistanceDecreasePerPress;
	float PullingDistanceDecreasePerPress;
	float MaxStrain;
	float MaxLengthMetres;
};

class CFlammableDef
{
public:
	std::int32_t FireResistance;
	float MaxDamageCaused;
	float FireDamagePerCycle;
	float FireDamagePeriod;
	CFireEffectCreationSet EffectCreationSet;
};

class CFrontEndDef
{
public:
	std::vector<std::string> vAttractModeMovie;
	std::int32_t ErrorMessageBackgroundGraphic;
	std::int32_t ButtonABigGraphic;
	std::int32_t ButtonBBigGraphic;
};

class CGiftDef
{
public:
	EGiftType Type;
	bool IsWeddingRing;
};

namespace NSpeechGainManager {
class CDialogueLayerDef
{
public:
	float ResponseTimeSecsAttack;
	float ResponseTimeSecsRelease;
	float GainLevelHigh;
	float GainLevelMid;
	float GainLevelLow;
	float FocusShiftDistanceThreshold;
};
}

class CGlobalDef
{
public:
	std::int32_t ControlSchemePC;
	std::int32_t ControlSchemePCAggressive;
	std::int32_t ControlSchemeXBOX;
	std::int32_t ControlSchemeXBOXAggressive;
	std::int32_t PhysicsTexture;
	std::int32_t InvalidThemeStandin;
	std::int32_t DefaultEngineThemeDef;
	float StaticMapGenerationEstimatedMaxCameraHeightAboveLandscape;
	std::int32_t GraphicWeaponTrail;
	std::int32_t GraphicWeaponTrail2;
	std::int32_t GraphicWeaponTrailArrow;
	std::int32_t WeaponTrailAlpha;
	float WeaponTrailAlphaFadeOffTime;
	std::int32_t WeaponTrailArrowAlpha;
	float WeaponTrailArrowAlphaFadeOffTime;
	std::int32_t WeaponTrialNumFramesToFadeOut;
	std::int32_t HealthPickupValue;
	std::int32_t RespawnDelay;
	std::int32_t ScrabbleAngle;
	std::int32_t SlideAngle;
	std::int32_t MaxFollowers;
	std::string PickupGoldSoundCriteria;
	std::string PickupItemSoundCriteria;
	std::string OnClickSoundCriteria;
	std::int32_t EditorSpangleCursor;
	std::int32_t EditorCircleSprite;
	std::int32_t EditorSecondaryCircleSprite;
	std::int32_t TestSpeech;
	std::string MimeSpeechIndex;
	std::int32_t AdultSleepTimeStart;
	std::int32_t AdultSleepTimeEnd;
	std::int32_t ChildSleepTimeStart;
	std::int32_t ChildSleepTimeEnd;
	std::int32_t ElderlySleepTimeStart;
	std::int32_t ElderlySleepTimeEnd;
	float CreatureGeneratorRegenDelaySeconds;
	std::int32_t Music;
	float AgeIncreasePerLevelUp;
	std::int32_t E3BodgeFireflyGraphic;
	CEngineGraphic SwitchGraphic;
	CEngineGraphic TrackGraphic;
	std::map<std::int32_t,std::string> ConversationCameras;
	std::vector<std::string> PermittedAINarrators;
	std::vector<NSpeechGainManager::CDialogueLayerDef> DialogueLayers;
	std::int32_t OpinionDebugIndicatorIcon;
	std::int32_t WifeLoveIndicatorIcon;
	std::int32_t WifeEngagedIndicatorIcon;
	std::int32_t WifeMarriedIndicatorIcon;
	std::int32_t ExpressionIndicatorIcon;
	std::int32_t FollowerIndicatorIcon;
	float EmoteIconDrawDistance;
	float AngleToBeConsideredVertical;
	float SoundGainLowerValueWhenInsideOutside;
	std::int32_t TeleportOutParticleEffect;
	float TeleportFadeOutTimeInSeconds;
	std::int32_t TeleportInParticleEffect;
	float TeleportFadeInTimeInSeconds;
	float DamageForMaximumBlood;
};

class CGoldDef
{
public:
	std::int32_t Gold;
};

class CGuardDef
{
public:
	std::int32_t BribeCostWeaponOut;
	std::int32_t BribeCostTrespass;
	std::int32_t BribeCostVandalism;
	std::int32_t BribeCostShoplifter;
	std::int32_t BribeCostPickPocket;
	std::int32_t BribeCostAssault;
	std::int32_t BribeCostGuardAssault;
	std::int32_t BribeCostGBH;
	std::int32_t BribeCostGuardGBH;
	std::int32_t BribeCostMurder;
};

class CGuiGraphicDef
{
public:
	std::int32_t Outer;
	std::int32_t InnerLeft;
	std::int32_t InnerMiddle;
	std::int32_t InnerRight;
	std::int32_t InnerBackLeft;
	std::int32_t InnerBackMiddle;
	std::int32_t InnerBackRight;
	CRGBColour TextColour;
};

class CGuildMasterDef
{
public:
	float MinSecondsBetweenTeleport;
	float MinSecondsBetweenWillUse;
};

class CHairCardDef
{
public:
	std::int32_t HairObject;
};

class CHasNameDef
{
public:
	std::uint32_t DefaultNameTag;
	std::uint32_t Home;
	std::uint32_t Occupation;
};

class CHeroAbilityDef
{
public:
	EHeroAbility Ability;
	EHeroExperienceStatCategory Stat;
	std::int32_t IconGraphicIndex;
	std::int32_t IconEffectIndex;
	std::int32_t IconActiveEffectIndex;
	C2DVector IconEffectOffset;
	std::int32_t Name;
	std::int32_t Description;
	std::vector<std::int32_t> LevDescription;
	std::int32_t MaxedOutDescription;
	std::vector<std::int32_t> StaminaCost;
	std::int32_t StatExperienceGainedOnUse;
	std::vector<std::int32_t> ExperienceCostsToUpgrade;
	std::int32_t ABXYPriority;
	float MoralityCostFactor;
	bool Aggressive;
	CDefIndex DummyObject;
};

class CHeroAbilityStatPackage
{
public:
	std::map<EHeroTrainableStatType,std::int32_t> StatThresholds;
};

class CHeroCentreDef
{
public:
};

class CHeroCombatDef
{
public:
	CFloatRange RangedAccuracySecondsForMaximumWithSkill;
	CFloatRange RangedAccuracyPercentFractionWithBuildup;
	CFloatRange RangedAccuracyPercentWithStats;
	CHeroAbilityStatWeight RangedAccuracyWeighting;
	CFloatRange RangedAccuracyShakeyCursor;
	CHeroAbilityStatWeight RangedAccuracyShakeyCursorWeighting;
	float RangedAccuracyShakeFractionOfRadius;
	CHeroAbilityStatWeight RangedDamageMultiplierWeighting;
	float RangedDamageMultiplierSpeedWithStatParameter;
	float RangedDamageMultiplierSpeedWithDistance;
	float RangedDamageMultiplierSpeedNominalBuildup;
	float RangedDamageMultiplierScaleNominalBuildup;
	float RangedDamageMultiplierSpeedSuperBuildup;
	float RangedDamageMultiplierScaleSuperBuildup;
	CHeroAbilityStatWeight RangedWeaponReloadSpeedWeighting;
	CFloatRange MeleeDamageMultiplierWithStats;
	CHeroAbilityStatWeight MeleeDamageWeighting;
	CFloatRange HitDamageMultiplierWithStats;
	CHeroAbilityStatWeight HitDamageWeighting;
	bool MeleeFlourishValidOnMultiplierLevelIncrease;
	std::int32_t MeleeFlourishNumMultiplierLevelsPerFlourish;
	bool MeleeFlourishAllowBreakingOfRotation;
	float MeleeFlourishSecondsForAvailibility;
	bool MeleeFlourishOneShot;
	bool MeleeFlourishUsable;
	bool MeleeCombatPlayStrikeSounds;
	float TargetingPressureFractionThresholdForToggle;
	float MaxAngleToTurnToShootAtTargetInThirdPersonBowMode;
	std::int32_t MeleeStrikeNumFramesPauseOnHeroStrike;
	CFloatRange MeleeStrikeAnimationSpeedFractionWithStats;
	CHeroAbilityStatWeight MeleeStrikeAnimationSpeedWeighting;
	float AutoBlockSkillThreshold;
	float AutoBlockStrengthThreshold;
	float MeleeRecoilMaintainHandednessSkillThreshold;
	float MeleeRecoilMaintainHandednessStrengthThreshold;
	float MeleeRecoilMaintainComboChainSkillThreshold;
	float MeleeRecoilMaintainComboChainStrengthThreshold;
	std::vector<std::int32_t> DefaultCombatAbilities;
	std::vector<float> ProjectileWeaponFirstPersonModeMaxFOVValues;
	bool ProjectileTargetingAcquireTargetTag;
	bool ProjectileTargetingAcquireTargetsAuto;
	bool ProjectileTargetingUseAnalogueZoom;
	float ProjectileWeaponAnalogueZoomControlWeight;
	CHeroAbilityStatPackage HeavyWeapons;
};

class CHeroDef
{
public:
	std::int32_t DefaultTitle;
	EHeroTrainingStatus DefaultHeroTrainingStatus;
};

class CHeroStatIncreaseData
{
public:
	std::map<EHeroExperienceStatCategory,float> StatIncreases;
};

class CHeroExperienceDef
{
public:
	std::vector<std::int32_t> ExperienceLevelThresholds;
	std::vector<std::int32_t> HeroLevelTitles;
	std::vector<std::int32_t> FallbackMultipliers;
	std::map<std::int32_t,CDefString> SoundIntensityThresholds;
	std::map<std::int32_t,CComboMultiplierData> ComboMultiplierMap;
	float ExperienceMultiplierParamPerUnitComboMultiplier;
	float ExperienceMultiplierRegionChangeMultiplier;
	std::map<std::int32_t,CDefString> ComboMultiplierIncrease;
	std::map<std::int32_t,CDefString> ComboMultiplierLock;
	std::map<std::int32_t,CDefString> ComboMultiplierLost;
	std::map<std::int32_t,CDefString> ComboMultiplierRunningHitsIncrease;
	std::map<EDamageAttribute,CHeroStatIncreaseData> StatIncreasePerHitType;
	std::map<EDamageAttribute,CHeroStatIncreaseData> StatIncreasePerHitType2;
	std::vector<CAttackHistoryCombo> AttackHistoryCombos;
};

class CHeroMarriageDef
{
public:
	float WeddingTimeForScreenToFadeOut;
	float WeddingTimeForScreenToFadeIn;
	float WeddingAfterFrescoTimeOfDayFastForwardTo;
	float SecondsJustMarriedDuration;
	float HeroStatRenownIncreasePerMarriage;
	std::int32_t SecondsBetweenCheckingPlayersAppearance;
	std::int32_t SecondsMaxTimeWithinNoticingCanReact;
	float FatnessChangeThresholdForComment;
	std::int32_t SecondsBetweenReceivingGiftOpinionReactionCanHappen;
	std::int32_t SecondsThatGiftsReceivedWillBeRemembered;
	std::int32_t FramesBetweenCullingOldGifts;
	std::int32_t SecondsBetweenEvaluatingGiftOpinion;
	std::int32_t SecondsBetweenGiftGivingOpportunities;
	float ChanceThatGiftWillBeGiven;
	float AmountOfGoldPerHourOfMarriageToIncreaseGiftGivingPriceValuePerHourOfPlay;
	float MaxNumHoursOfMarriageToIncreaseGiftGivingPriceValue;
	float MultiplierForGiftGivingValueEveryOpportunity;
	float MaxDistanceFromMaxStatsThatGiftsCanBeGiven;
	std::int32_t SecondsBetweenEvaluatingLoveAttitude;
	float SecondsOutOfTheRegionForLoveAttitudeToDecayOneToZero;
	float ThresholdForLoveAttitudeOverride;
	std::string FallenInLoveSoundCriteria;
	float FallenInLoveSoundGain;
	std::int32_t FianceeOpinionPersonalityDef;
	float ProbabilityOfCourtingAdviceOnGreet;
	float SecondsTimeSinceSeeingYouMinThreshold;
	float SecondsRunOutWifeOnViolenceHateSpike;
	float PeakEffectWifeOnViolenceHateSpike;
	float PersistEffectWifeOnViolenceHateSpike;
	float SecsInHateBeforeDivorceOccurs;
	float SecsInHateBetweenDivorceWarnings;
	std::string TextGuiWifeDivorceOccurred;
	std::string TextGuiWifeDivorceRenting;
	float HeroStatMoralityInOpinionFormChangePerDivorce;
	float SecsInLoveAndHusbandPresentBeforeSexOffered;
	float SecsInLoveAndHusbandPresentBetweenSexOffers;
	std::string TextGuiWifeSexOffered;
	float SecsMinSoundDurationInSexCutscene;
	float SecsWaitAfterSoundInSexCutscene;
	std::string DialogueGroupForSexCutscene;
};

class CParticleMorphs
{
public:
	class CEntry
	{
	public:
		EHeroMorphType MorphType;
		bool Blending;
		EMeshLocationPoint Location;
		CDefString LocationName;
		std::int32_t LocationIndex;
		std::int32_t ParticleIndex;
		std::int32_t SecondParticleIndex;
		float StartAtMorphStrength;
		float EndAtMorphStrength;
	};

	std::vector<std::pair<EHeroMorphType,CParticleMorphs::CEntry>> Morphs;
	std::uint32_t TrailingU32 = 28011708;
};

class CSkeletalMorphs
{
public:
	class CEntry
	{
	public:
		CDefString MorphName;
		std::int16_t Priority;
		bool SupressesLowerMorphs;
	};

	std::vector<std::pair<std::uint32_t,CSkeletalMorphs::CEntry>> Morphs;
};

class CTextureMorphEntry
{
public:
	EHeroMorphType MorphType;
	std::int32_t TextureLayer;
	std::int32_t ReplacingTextureIndex;
	std::int32_t BankIndex;
	ECompositeBlendType Blend;
};

class CTextureMorphs
{
public:
	std::vector<std::pair<EHeroMorphType,CTextureMorphEntry>> Morphs;
	std::uint32_t TrailingU32 = 28011707;
};

class CHeroMorphDef
{
public:
	CTextureMorphs TextureMorphs;
	CSkeletalMorphs SkeletalMorphs;
	CParticleMorphs ParticleMorphs;
	CParticleMorphs IdleParticleMorphs;
};

class CHeroPostcardGeneratorDef
{
public:
	C2DVector MeshAreaTLPos;
	C2DVector MeshAreaBRPos;
	C3DVector DollCentreOffset;
	float DollBoundingSphereRadius;
};

class CHeroSoulsCreatureDef
{
public:
	std::string CreatureType;
	std::string Location;
	std::int32_t NumCreatures;
};

class CHeroSoulsWaveDef
{
public:
	std::int32_t NumWaveCreatures;
	std::vector<CHeroSoulsCreatureDef> Creatures;
};

class CHeroSoulsRoundDef
{
public:
	std::int32_t NumWaves;
	std::vector<CHeroSoulsWaveDef> Waves;
};

class CHeroSpecialMovementDef
{
public:
	float TotalTimeToSprintSeconds;
	float MinimumNumberOfSecondsLeftToBeAllowedToSprint;
	float TotalTimeToFullyRestoreSprintBarSeconds;
	float NoSecondsToRunBeforeSprintStarts;
};

class CHeroStatsDef
{
public:
	std::int32_t MaximumStamina;
	float NumberOfSecondsAfterUseStaminaStartsRefilling;
	float NumberOfSecondsAfterChangeHealthStartsRefilling;
	float NumberOfSecondsBetweenHealthRefills;
	std::int32_t StaminaToRegainPerSecond;
	std::int32_t HealthToRegainPerRefill;
	std::int32_t RenownLossOnDeath;
	std::int32_t MoneyLossOnDeath;
	std::int32_t MaximumMoney;
	std::int32_t MaximumExpToSpend;
	std::int32_t ExpToSpendAtWhichToDisplayTutorial;
	float MinAge;
	float MaxAge;
	std::vector<CAppearanceModifierScalingDef> AppearanceModifierScaling;
	float AmountFatnessIncreasesPerPointOfOverEating;
	float NumberOfMinutesBetweenFatnessReductions;
	float AmountFatnessDecreasesPerReduction;
	float DrinkAffectMultiplierMultiplier;
	std::int32_t MinVomitTime;
	std::int32_t MaxVomitTime;
	std::int32_t MinTimeBeforeVomiting;
	std::int32_t MaxTimeBeforeVomiting;
	float PintsOfSobrietyPerSecondOfPuke;
	float PintsToDrunk;
	float PintsToVomit;
	float AlcoholAbsorbtionPerSecond;
	std::vector<std::int32_t> RenownLevelCosts;
	std::vector<std::int32_t> RenownLevelMaxFollowers;
	std::vector<std::int32_t> RenownLevelMaxBoastingCrowd;
	std::int32_t MaxMorality;
	std::int32_t MaxMoralityAsChild;
	float MaxExtraMaxHitPoints;
	std::int32_t MaxExtraMaxStaminaPoints;
	std::int32_t MoralityChangeOnTheftFromHome;
	std::int32_t MoralityChangeOnTheftFromShop;
	std::int32_t MoralityChangeOnPicklock;
	std::int32_t MoralityChangeOnVandalism;
	std::vector<float> TradeSkillForGuileLevel;
	std::vector<CAbilityTrackDef> StatLevels;
	std::vector<float> MaxHitPointsPerHealthLevel;
	std::vector<std::int32_t> MaxStaminaPointsPerMagicPowerLevel;
	std::vector<float> SoundDistanceMultiplierPerStealthLevel;
	std::vector<float> VisibilityMultiplierPerStealthLevel;
};

class CHeroSuitDef
{
public:
	std::map<EClothingSuitPart,std::int32_t> SuitParts;
};

class CHeroTitleDef
{
public:
	EHeroTitle Title;
	std::int32_t HeroTitleGuiTag;
	std::int32_t HeroTitleGreetToHeroTag;
	std::int32_t HeroTitleCommentAtHeroTag;
	std::int32_t HeroTitleCommentToSelfTag;
	std::int32_t RequiredRenownLevel;
	std::int32_t RequiredStrengthExpLevel;
	std::int32_t RequiredSkillExpLevel;
	std::int32_t RequiredWillExpLevel;
	bool IsBuyable;
	bool IsAutomatic;
};

class CHighlightItemDef
{
public:
	std::int32_t ParticleEmitter;
	std::int32_t PickupParticleEmitter;
	float RotationSpeed;
	float DistanceFromGround;
	float BobDistance;
	float BobTime;
	bool StartAsRunning;
	float StartAsRunningDelay;
};

namespace NHitLocations {
class CPhysicalPrimitiveBase
{
public:
};
}

class CPhysicalPrimitiveInit
{
public:
	// Wire variant: tag 0/3 have no payload, 1 adds BaseName+Radius, 2 adds Height.
	std::uint32_t Type = 0;
	CDefString BaseName;
	float Radius = 0.0f;
	float Height = 0.0f;
};

class CHitLocationDef
{
public:
	CPhysicalPrimitiveInit PhysicalPrimitive;
	std::int32_t Armour;
	EActionRegisteredType HitResponseAction;
	EActionRegisteredType HitResponseToEpicSpellsAction;
	EActionRegisteredType HitKnockdownAction;
	EActionRegisteredType HitKnockdownDeathAction;
	EActionRegisteredType BlockResponseAction;
	std::int32_t HitDamageEffect;
	std::int32_t HitBlockedEffect;
	std::int32_t VulnerableEffect;
	EHitLocationFlags Flags;
	bool PlayGetHitResponse;
	bool DecapitateOnHit;
	bool DefendableAgainstShots;
	bool DefendableAgainstMelee;
	std::int32_t Priority;
	bool Default;
	bool EnableOnCreate;
	EHitLocationCodelet HitLocationCodelet;
};

class CHitLocationsDef
{
public:
	std::vector<std::int32_t> HitLocations;
};

class CIdleSchedulerDef
{
public:
	std::map<EIdleStateGroup,std::int32_t> Period;
};

class CInterestingToVillagersDef
{
public:
	float Interest;
};

class CInventoryCategoryDef
{
public:
	std::int32_t Inventory;
	std::int32_t NumberOfSlots;
	bool DrawItemSlots;
	bool SelectEmptySlots;
	bool WrapHighlightCursor;
	std::uint32_t CategoryName;
	bool AllowItemsToFillMoreThanOneSlot;
	NInventory::EInventoryCategoryIdentifier CategoryIdentifier;
	bool AddCategoryOnCreate;
};

class CInventoryDef
{
public:
	float InitialCategoryMovementRepeatDelay;
	float ContinuedCategoryMovementRepeatDelay;
	float InitialItemMovementRepeatDelay;
	float ContinuedItemMovementRepeatDelay;
	std::int32_t MovementSound;
	std::int32_t NoMovementSound;
	std::int32_t InventoryOpenSound;
	bool TrackSelectedItems;
	std::vector<std::int32_t> InitialInventoryClothingItems;
	std::vector<std::int32_t> InitialInventoryWeaponsItems;
	std::vector<std::int32_t> InitialInventoryStatsItems;
	std::vector<std::int32_t> InitialInventoryStandardItems;
	C2DVector ClothingShadedCircleTLPos;
	C2DVector ClothingShadedCircleExtents;
	C2DVector CategoriesTLPos;
	C2DVector MeshAreaTLPos;
	C2DVector MeshAreaBRPos;
	C2DVector NextCategoryOffset;
	C2DVector FirstItemOffsetFromCategory;
	C2DVector NextItemOffset;
	CDefString GenericFont;
	CRGBColour BackGroundColour;
	std::int32_t ClothingDoll;
	C3DVector DollCentreOffset;
	float DollBoundingSphereRadius;
	C2DVector TotalExperienceTextPos;
	std::uint32_t TotalExperienceText;
	C2DVector SpendingExperienceTextPos;
	std::uint32_t SpendingExperienceText;
	C2DVector StrengthLevelTextPos;
	CRGBColour StrengthLevelColour;
	C2DVector SkillLevelTextPos;
	CRGBColour SkillLevelColour;
	C2DVector WillLevelTextPos;
	CRGBColour WillLevelColour;
	C2DVector RenownLevelTextPos;
	C2DVector HeroLevelTextPos;
	C2DVector StatLevelsFirstLineTextPos;
	C2DVector StatLevelsNextLineTextOffset;
	C2DVector ButtonAssignmentTextPos;
	C2DVector ButtonAssignmentTextOffset;
	std::vector<std::uint32_t> StatLevelsTexts;
	std::vector<CRGBColour> StatLevelsColours;
	std::int32_t WeaponsTextSeparator;
	std::int32_t WeaponsTextNumberOfSlots;
	std::int32_t WeaponsTextSlot;
	std::int32_t WeaponsTextSuccessfulAugmentation;
	std::int32_t WeaponsTextNoFreeSlots;
	std::int32_t WeaponsTextReadyToAugment;
	std::int32_t WeaponsTextNotReadyToAugment;
	C2DVector WeaponsDescriptionBoxTLPos;
	C2DVector WeaponsDescriptionBoxExtents;
	C2DVector WeaponsActiveMeleeIconPos;
	C2DVector WeaponsActiveRangeIconPos;
	C2DVector WeaponAugmentationTextStartPos;
	C2DVector WeaponAugmentationTextNewLineOffset;
	C2DVector AugmentationStatusTextAreaTLPos;
	C2DVector AugmentationStatusTextAreaBRPos;
	float TimeToDisplayWarningMessageInSeconds;
	C2DVector WeaponsShadedCircleTLPos;
	C2DVector WeaponsShadedCircleExtents;
	float NumberOfSecondsForObjectRotation;
	std::int32_t MaxNumberOfItemsToDisplayPerCategory;
	std::int32_t MaxNumberOfCategoriesToDisplay;
	std::int32_t MaxNumberOfScreenNamesToDisplay;
	std::uint32_t InventoryName;
	NInventory::EInventoryIdentifier InventoryIdentifier;
	C2DVector MapScreenMapDisplayBoxTLPos;
	C2DVector MapScreenMapDisplayBoxBRPos;
	C2DVector MapScreenRegionNamePos;
	std::int32_t MapScreenWorldMapGraphic;
	C2DVector MapScreenWorldMapPos;
	std::int32_t MapScreenRegionUnknownGraphic;
	std::int32_t MapScreenRegionUnknownHighlightGraphic;
	std::int32_t MapScreenRegionHeroGraphic;
	std::int32_t MapScreenRegionQuestGraphic;
	std::int32_t MapScreenRegionTeleportGraphic;
	std::int32_t MapScreenRegionHouseGraphic;
	C2DVector StatsTitleTLPos;
	C2DVector StatsShadedCircleTLPos;
	C2DVector StatsShadedCircleExtents;
	CEngineGraphic StatsProgBarL;
	CEngineGraphic StatsProgBarC;
	CEngineGraphic StatsProgBarR;
	CEngineGraphic StatsProgBarValue;
	C2DVector StatsProgBarValueOffset;
	C2DVector StatsPersonalityHeadingTLPos;
	C2DVector StatsPersonalityHeadingBRPos;
	std::uint32_t StatsPersonalityRenownLowerLabel;
	std::uint32_t StatsPersonalityRenownUpperLabel;
	std::uint32_t StatsPersonalityGoodnessLowerLabel;
	std::uint32_t StatsPersonalityGoodnessUpperLabel;
	std::uint32_t StatsPersonalityAttractivenessLowerLabel;
	std::uint32_t StatsPersonalityAttractivenessUpperLabel;
	std::uint32_t StatsPersonalityScarinessLowerLabel;
	std::uint32_t StatsPersonalityScarinessUpperLabel;
	std::uint32_t StatsPersonalityNicenessLowerLabel;
	std::uint32_t StatsPersonalityNicenessUpperLabel;
	C2DVector StatsPersonalityLabelTextOffset;
	C2DVector StatsPersonalityLowerLabelTLPos;
	float StatsPersonalityLowerLabelWidth;
	float StatsPersonalityLowerLabelHeight;
	float StatsPersonalitySliderWidth;
	float StatsPersonalityUpperLabelWidth;
	std::int32_t StatsPersonalityDoll;
	C3DVector StatsPersonalityDollCentreOffset;
	float StatsPersonalityDollBoundingSphereRadius;
	C2DVector StatsStatisticsLabelTLPos;
	float StatsStatisticsLabelWidth;
	float StatsStatisticsLabelHeight;
	C2DVector StatsStatisticsLabelTextOffset;
	C2DVector StatsStatisticsDataTextOffset;
	C2DVector StatsStatisticsHeadingTLPos;
	C2DVector StatsStatisticsHeadingBRPos;
	std::uint32_t StatsStatisticsPeopleKilledLabel;
	std::uint32_t StatsStatisticsMonstersKilledLabel;
	std::uint32_t StatsStatisticsMaxMultiplierLabel;
	std::uint32_t StatsStatisticsPreferredFoodLabel;
	std::uint32_t StatsStatisticsDietaryTypeLabel;
	std::uint32_t StatsStatisticsGameTimeElapsedLabel;
	std::uint32_t StatsStatisticsNumberOfWivesLabel;
	std::uint32_t StatsStatisticsQuestsActiveLabel;
	std::uint32_t StatsStatisticsQuestsFinishedLabel;
	std::uint32_t StatsStatisticsBoastsAchievedLabel;
	C2DVector StatsExperienceStrengthTextOffset;
	C2DVector StatsExperienceWillTextOffset;
	C2DVector StatsExperienceSkillTextOffset;
	std::uint32_t StatsExperienceStrengthText;
	std::uint32_t StatsExperienceWillText;
	std::uint32_t StatsExperienceSkillText;
	C2DVector StatsExperiencePoolGeneralTLPos;
	C2DVector StatsExperiencePoolGeneralTextOffset;
	C2DVector StatsExperiencePoolStrengthGroupTLPos;
	C2DVector StatsExperiencePoolSkillGroupTLPos;
	C2DVector StatsExperiencePoolWillGroupTLPos;
	C2DVector StatsExperiencePoolLabelTLPos;
	C2DVector StatsExperiencePoolLabelTextOffset;
	float StatsExperiencePoolLabelWidth;
	float StatsExperiencePoolLabelHeight;
	float StatsExperiencePoolSliderWidth;
	std::uint32_t StatsExperiencePoolStrengthLabel;
	std::uint32_t StatsExperiencePoolSkillLabel;
	std::uint32_t StatsExperiencePoolWillLabel;
	std::uint32_t StatsExperiencePoolPhysiqueLabel;
	std::uint32_t StatsExperiencePoolHealthLabel;
	std::uint32_t StatsExperiencePoolToughnessLabel;
	std::uint32_t StatsExperiencePoolSpeedLabel;
	std::uint32_t StatsExperiencePoolAccuracyLabel;
	std::uint32_t StatsExperiencePoolStealthLabel;
	std::uint32_t StatsExperiencePoolWeaponMagicLabel;
	std::uint32_t StatsExperiencePoolAbilityMagicLabel;
	std::uint32_t StatsExperiencePoolPureMagicLabel;
	float StatsExperienceStrengthBarValue;
	float StatsExperienceSkillBarValue;
	float StatsExperienceWillBarValue;
	C2DVector ExperienceGeneralPoolLabelTLPos;
	C2DVector ExperienceGeneralPoolLabelTextOffset;
	float ExperienceGeneralPoolWidth;
	float ExperienceGeneralPoolSliderWidth;
	std::uint32_t ExperienceGeneralPoolBarValue;
	std::uint32_t ExperienceGeneralPoolLabel;
	C2DVector ExperienceGeneralPoolSpendTextOffset;
	C2DVector ExperienceStrengthPoolTLPos;
	C2DVector ExperienceSkillPoolTLPos;
	C2DVector ExperienceWillPoolTLPos;
	float ExperiencePoolWidth;
	float ExperiencePoolHeight;
	std::uint32_t ExperienceTypeStrengthLabel;
	std::uint32_t ExperienceTypeSkillLabel;
	std::uint32_t ExperienceTypeWillLabel;
	std::uint32_t ExperienceTypePureMagicLabel;
	std::uint32_t ExperienceTypeAbilityMagicLabel;
	std::uint32_t ExperienceTypeWeaponMagicLabel;
	C2DVector ExperienceTypeTLPos;
	C2DVector ExperienceTypeTLPosOffset;
	C2DVector ExperienceTypeTextOffset;
	C2DVector ExperienceTypeSpendTextOffset;
	float ExperienceTypeLabelWidth;
	float ExperienceTypeLabelHeight;
	float ExperienceTypeSliderWidth;
	std::uint32_t ExperienceStrengthPhysiqueLabel;
	std::uint32_t ExperienceStrengthHealthLabel;
	std::uint32_t ExperienceStrengthToughnessLabel;
	std::uint32_t ExperienceSkillSpeedLabel;
	std::uint32_t ExperienceSkillAccuracyLabel;
	std::uint32_t ExperienceSkillStealthLabel;
	std::uint32_t ExperienceWeaponMagicBerserkLabel;
	std::uint32_t ExperienceWeaponMagicFlamingBladeLabel;
	std::uint32_t ExperienceWeaponMagicDoubleStrikeLabel;
	std::uint32_t ExperienceWeaponMagicLightningBladeLabel;
	std::uint32_t ExperienceAbilityMagicBattleChargeLabel;
	std::uint32_t ExperienceAbilityMagicAssassinRushLabel;
	std::uint32_t ExperienceAbilityMagicTimeSpellLabel;
	std::uint32_t ExperiencePureMagicBurningHandsLabel;
	std::uint32_t ExperiencePureMagicPhysicalShieldLabel;
	std::uint32_t ExperiencePureMagicForcePushLabel;
	std::uint32_t ExperiencePureMagicEnflameSpellLabel;
	std::uint32_t ExperiencePureMagicDrainLifeLabel;
	std::uint32_t ExperiencePureMagicHealLifeLabel;
	std::uint32_t ExperiencePureMagicGhostSwordLabel;
	float ExperienceStrengthBarValue;
	float ExperienceSkillBarValue;
	float ExperienceWillBarValue;
	float ExperienceStrengthPhysiqueBarValue;
	float ExperienceStrengthHealthBarValue;
	float ExperienceStrengthToughnessBarValue;
	float ExperienceSkillSpeedBarValue;
	float ExperienceSkillAccuracyBarValue;
	float ExperienceSkillHealthBarValue;
	float ExperienceWeaponMagicBerserkBarValue;
	float ExperienceWeaponMagicFlamingBladeBarValue;
	float ExperienceWeaponMagicDoubleStrikeBarValue;
	float ExperienceAbilityMagicBattleChargeBarValue;
	float ExperienceAbilityMagicAssassinRushBarValue;
	float ExperienceAbilityMagicTimeSpellBarValue;
	float ExperiencePureMagicBurningHandsBarValue;
	float ExperiencePureMagicPhysicalShieldBarValue;
	float ExperiencePureMagicForcePushBarValue;
	CEngineGraphic ExperienceProgBarL;
	CEngineGraphic ExperienceProgBarC;
	CEngineGraphic ExperienceProgBarR;
	CEngineGraphic ExperienceProgBarValue;
	C2DVector ExperienceProgBarValueOffset;
	std::int32_t ExperienceSpendTimeOffset;
	std::int32_t ExperienceSpendRate;
	std::vector<std::int32_t> ExperienceItemLabelStat;
	std::vector<std::int32_t> ExperienceItemLabelAbility;
	CEngineGraphic ExperienceSpendGreen;
	CEngineGraphic ExperienceSpendGreenOutline;
	CEngineGraphic ExperienceSpendGreenEnd;
	CEngineGraphic ExperienceSpendRed;
	CEngineGraphic ExperienceSpendRedOutline;
	CEngineGraphic ExperienceSpendRedEnd;
	C2DVector ExperienceDescriptionTextTLPos;
	C2DVector ExperienceDescriptionTextOffset;
	std::uint32_t QuestCardInfoQuestNameLabel;
	std::uint32_t QuestCardInfoMoralityLabel;
	std::uint32_t QuestCardInfoCoreLabel;
	std::uint32_t QuestCardInfoDetailsLabel;
	std::uint32_t QuestCardInfoRegionLabel;
	std::uint32_t QuestCardInfoOriginatorLabel;
	std::uint32_t QuestCardInfoDescriptionLabel;
	std::uint32_t QuestCardInfoSuccessLabel;
	std::uint32_t QuestCardInfoFailureLabel;
	std::uint32_t QuestCardInfoRewardsLabel;
	std::uint32_t QuestCardInfoRenownLabel;
	std::uint32_t QuestCardInfoTrophyLabel;
	std::uint32_t QuestCardInfoBoastsLabel;
	std::uint32_t QuestCardInfoStatusLabel;
	std::uint32_t QuestCardInfoQuestNameText;
	std::uint32_t QuestCardInfoMoralityText;
	std::uint32_t QuestCardInfoCoreText;
	std::uint32_t QuestCardInfoDetailsText;
	std::uint32_t QuestCardInfoRegionText;
	std::uint32_t QuestCardInfoOriginatorText;
	std::uint32_t QuestCardInfoDescriptionText;
	std::uint32_t QuestCardInfoSuccessText;
	std::uint32_t QuestCardInfoFailureText;
	std::uint32_t QuestCardInfoRewardsText;
	std::uint32_t QuestCardInfoRenownText;
	std::uint32_t QuestCardInfoTrophyText;
	std::uint32_t QuestCardInfoBoastsText;
	std::uint32_t QuestCardInfoStatusText;
	C2DVector TradeEdgedButtonCostTLPos;
	C2DVector TradeEdgedButtonOwnedTLPos;
	C2DVector TradeEdgedButtonProfitTLPos;
	C2DVector TradeEdgedButtonTitleTLPos;
	C2DVector TradeEdgedButtonCashTLPos;
	C2DVector TradeEdgedButtonExtents;
	C2DVector TradeAnimImageBoxTLPos;
	C2DVector TradeAnimImageBoxExtents;
	float TradeAnimImageXRotationTime;
	float TradeAnimImageYRotationTime;
	float TradeAnimImageZRotationTime;
	C2DVector TradeEquippedItemTLPos;
	C2DVector TradeEquippedItemExtents;
	CEngineGraphic TradeEquippedItemGraphic;
	C2DVector TradeDescriptionBoxTLPos;
	C2DVector TradeDescriptionBoxExtents;
	std::uint32_t TradeTextOwned;
	std::uint32_t TradeTextDivider;
	std::uint32_t TradeTextCost;
	std::uint32_t TradeTextGold;
	std::uint32_t TradeTextProfit;
	C2DVector TradeShadedCircleTLPos;
	C2DVector TradeShadedCircleExtents;
	C2DVector AugmentationMeshAreaTLPos;
	C2DVector AugmentationMeshAreaBRPos;
	C2DVector AugmentDescriptionBoxTLPos;
	C2DVector AugmentDescriptionBoxExtents;
	C2DVector AugmentAugmentationInfoTLPos;
	C2DVector AugmentAugmentationLabelTLPos;
	C2DVector AugmentAugmentationInfoTextOffset;
	C2DVector AugmentAugmentationSlotTLPos;
	C2DVector AugmentAugmentationSlotTextOffset;
	C2DVector AugmentAugmentationWeaponTLPos;
	C2DVector AugmentAugmentationWeaponTextOffset;
	C2DVector AugmentTitleTLPos;
	C2DVector AugmentTitleTextOffset;
	std::uint32_t AugmentTextMaxSlots;
	std::uint32_t AugmentTextNumSlots;
	std::uint32_t AugmentTextUpgradeCost;
	std::uint32_t AugmentTextGold;
	std::uint32_t AugmentTextDamageMultiplier;
	std::uint32_t AugmentTextFireStrength;
	std::uint32_t AugmentTextSlot;
	std::uint32_t AugmentTextAugmentations;
	std::uint32_t AugmentTextWeapons;
	std::uint32_t AugmentTextDivider;
	C2DVector AugmentShadedCircleTLPos;
	C2DVector AugmentShadedCircleExtents;
	float InventoryAnimImageXRotationTime;
	float InventoryAnimImageYRotationTime;
	float InventoryAnimImageZRotationTime;
	CRGBColour QuestCardFeatColour;
	CRGBColour QuestCardCoreColour;
	CRGBColour QuestCardExclusiveColour;
	C2DVector QuestCardInfoTLPos;
	C2DVector QuestCardInfo2TLPos;
	C2DVector QuestCardNoQuestsTLPos;
	std::uint32_t QuestCardNoQuestsText;
	C2DVector QuestCardInfoTextOffset;
	C2DVector QuestCardEdgedButtonTitleTLPos;
	C2DVector QuestCardEdgedButtonExtents;
	std::vector<std::int32_t> ExpSpendItemLabelStat;
	std::vector<std::int32_t> ExpSpendItemLabelAbility;
	std::int32_t ExpSpendMaxItemsToDisplay;
	C2DVector ExpSpendButtonTLPos;
	C2DVector ExpSpendButtonTLPosOffset;
	C2DVector ExpSpendButtonTextOffset;
	CEngineGraphic ExpSpendButtonSelL;
	CEngineGraphic ExpSpendButtonSelC;
	CEngineGraphic ExpSpendButtonSelR;
	CEngineGraphic ExpSpendButtonL;
	CEngineGraphic ExpSpendButtonC;
	CEngineGraphic ExpSpendButtonR;
	C2DVector ExpSpendDescriptionBoxTLPos;
	C2DVector ExpSpendDescriptionBoxExtents;
	C2DVector ExpSpendArrowUpTLPos;
	C2DVector ExpSpendArrowDnTLPos;
	C2DVector ExpSpendGeneralPoolLabelTLPos;
	C2DVector ExpSpendGeneralPoolLabelTextOffset;
	C2DVector ExpSpendGeneralPoolSpendTextOffset;
	float ExpSpendGeneralPoolWidth;
	float ExpSpendGeneralPoolSliderWidth;
	C2DVector ExpSpendStrengthPoolTLPos;
	C2DVector ExpSpendSkillPoolTLPos;
	C2DVector ExpSpendWillPoolTLPos;
	float ExpSpendPoolWidth;
	float ExpSpendPoolHeight;
	CEngineGraphic ExpSpendProgBarL;
	CEngineGraphic ExpSpendProgBarC;
	CEngineGraphic ExpSpendProgBarR;
	CEngineGraphic ExpSpendProgBarValue;
	C2DVector ExpSpendProgBarValueOffset;
	C2DVector ExpSpendTypeTextOffset;
	std::int32_t ExpSpendGeneralPoolBarValue;
	std::int32_t ExpSpendGeneralPoolLabel;
	std::int32_t ExpSpendTypeStrengthLabel;
	std::int32_t ExpSpendTypeSkillLabel;
	std::int32_t ExpSpendTypeWillLabel;
	float ExpSpendStrengthBarValue;
	float ExpSpendSkillBarValue;
	float ExpSpendWillBarValue;
	std::int32_t ExpSpendRate;
	std::int32_t ExpSpendTimeOffset;
	CEngineGraphic ExpSpendGreen;
	CEngineGraphic ExpSpendGreenOutline;
	CEngineGraphic ExpSpendGreenEnd;
	CEngineGraphic ExpSpendRed;
	CEngineGraphic ExpSpendRedOutline;
	CEngineGraphic ExpSpendRedEnd;
	C2DVector ExpSpendDescriptionTextTLPos;
	C2DVector ExpSpendDescriptionTextOffset;
	C2DVector MainEdgedButtonTitleTLPos;
	C2DVector MainEdgedButtonOwnedTLPos;
	C2DVector MainEdgedButtonPriceTLPos;
	C2DVector MainEdgedButtonCostTLPos;
	C2DVector MainEdgedButtonExtents;
	CEngineGraphic MainDescriptionBoxC;
	CEngineGraphic MainDescriptionBoxTL;
	CEngineGraphic MainDescriptionBoxTR;
	CEngineGraphic MainDescriptionBoxBL;
	CEngineGraphic MainDescriptionBoxBR;
	C2DVector MainDescriptionBoxTLPos;
	C2DVector MainDescriptionBoxExtents;
	C2DVector MainShadedCircleTLPos;
	C2DVector MainShadedCircleExtents;
	C2DVector MainAnimBoxTLPos;
	C2DVector MainAnimBoxExtents;
	float MainQuickAccessItemFlashTime;
};

class CInventoryItemDef
{
public:
	CEngineGraphic Graphic;
	std::uint32_t ItemDescription;
	std::uint32_t ItemDetails;
	std::int32_t InventoryCategory;
	std::int32_t MaxNumberItems;
	std::int32_t SlotIndex;
	float ActivationTime;
	EInventoryActionType UseButtonAction;
	ETCInterfaceType InventoryType;
	EInventoryObjectOrientation Orientation;
	std::int32_t HeroAbilityDef;
	bool IsSellable;
	bool IsBuyable;
	bool IsConfiscatable;
	bool DoNotPersistUntilQuestCompleted;
	bool DoNotAutoPickUp;
	bool AutoPickUpAfterFirstPickUp;
	std::int32_t ItemToSelectUponRemoval;
	ETutorialCategory TutorialCategory;
	EUIInventoryCategory UIInventoryCategory;
};

class CJackDragonDef
{
public:
	float MinTimeBetweenSlams;
	std::int32_t MaxHitsBeforeFlyAway;
	float MaxFirebeamDistance;
	float MinSnapDistance;
	float MaxSnapDistance;
	float MaxTimeAtLongRangeBeforeFlyAway;
	float MaxSlamDistance;
	std::int32_t MaxHoverLoops;
	float PercentChanceOfFakeLandingSwoop;
	float MaxTimeOnGround;
	float DragonGroundHeightOffsetBodge;
	float HoverBackOffOffset;
	std::int32_t FramesBetweenBurninateUpdate;
	std::int32_t FramesBetweenFireBeamUpdate;
	float HoverObstructionRadius;
	float StrafeTriggerDistance;
	float GetHitSnapBuildupTime;
	float NotHitSnapBuildupTime;
	float MaxTimeBeforeSnap;
	float MinRoarDistance;
};

class CJackOfBladesBattleDef
{
public:
	float IdealShootDistance;
	std::int32_t NumGetHitsBeforeRush;
	float TimeBetweenShots;
	float ScreamInAirTime;
	float TimeBetweenPillars;
	float BattleFloatHeight;
	float BattleScale;
	float ScaleUpTime;
	float ScaleDownTime;
	float DeathScreamTime;
	float DeathHeight;
	float BurnDamage;
	float FlashDuration;
	float FlashBuildUpTime;
	float RushTriggerDistance;
	float RushToDistance;
	float RushAwayDistance;
};

class CKickableDef
{
public:
	float BaseDistanceToTravel;
	float StrengthScaleRange;
	float SpeedScaleRange;
	float BaseSpeedScaleRange;
	float RandomBase;
	float RandomRange;
	std::int32_t ShotToAttach;
	EOpinionDeedType OpinionDeedType;
	std::int32_t OpinionDeedFaction;
	EOpinionDeedType OpinionDeedTypeToEnemies;
	bool IgnoreNavigationTest;
};

class CKrakenDef
{
public:
	float HeightOffset;
	float PoolRadius;
	float HeadRiseDelay;
	float HeadIdleTime;
	std::int32_t LowStrikesBeforeHigh;
	std::int32_t LowStrikeChargeLoops;
	std::int32_t LowStrikeAttackLoops;
	std::int32_t HighStrikeAttackLoops;
	std::int32_t RoarLoops;
	float DelayBeforeLowBeam;
	float DelayBeforeHighBeam;
	float BeamExplosionRadius;
	float BeamTrackSpeed;
	float TargetingTrackSpeed;
	float BeamMouthDistLow;
	float BeamMouthDistHigh;
	std::int32_t TicksBetweenBeamExplosions;
	std::vector<std::int32_t> TentacleDefIndex;
	float TentacleMaxRiseDelay;
};

class CKrakenTentacleDef
{
public:
	float TentacleSmackDownTime;
	float TentacleTurnSpeed;
	float TentacleMoveSpeed;
	float TentacleStrikeAngleTolerence;
	float TentacleStrikeDistTolerence;
	float TentacleHeightOffset;
	float TentacleTimeBeforeExplosion;
	float TentacleTimeBetweenAttacks;
	float TentacleTimeBeforeObstructionRemoval;
	float TentacleObstructionRadius;
	float TentacleExplosionBodgeAngle;
	float TentacleExplosionRadius;
	float TentacleObstructionRadiusMin;
	float TentacleObstructionRadiusMax;
};

class CLensFlareElementDef
{
public:
	float Radius;
	CRGBColour Colour;
	float Position;
	std::int32_t Texture;
};

class CLightDef
{
public:
	CRGBColour Colour;
	std::int32_t HaloGraphic;
	float InnerRadius;
	float OuterRadius;
	float Flicker;
	float FlickerSpeed;
	bool StartActive;
	bool SunlightAttenuator;
	bool DaylightAttenuator;
	float DaylightAttenuatorFadeInStart;
	float DaylightAttenuatorFadeInEnd;
	float DaylightAttenuatorFadeOutStart;
	float DaylightAttenuatorFadeOutEnd;
	EPointLightChannelEffect EffectChannel;
	bool Inverted;
};

class CLightningDef
{
public:
	float Range;
	float LifeTime;
	std::int32_t MaxNumberOfBranches;
	bool NeedLOSToBranch;
	float DamagePeriod;
	float DamagePerCycle;
	std::int32_t NodeDefIndex;
};

class CLightningOrbDef
{
public:
	std::int32_t LightningDefIndex;
	float LifeTime;
	float Speed;
	float ExplosionRange;
	std::int32_t ExplosionDefIndex;
};

class CLookDef
{
public:
	float EyesTurnRangeXY;
	float EyesTurnRangeYZ;
	float HeadTurnRangeXY;
	float HeadTurnRangeYZUp;
	float HeadTurnRangeYZDown;
	float MaxTurnSpeed;
	bool AllowTurningViaAnimation;
	float AnimationTurnSpeedMultiplier;
	float CombatMaxTurnSpeed;
	bool CombatAllowTurningViaAnimation;
	bool AllowOversteer;
	bool AllowUndersteer;
	bool DrawEyePrimitives;
	float CutOffNoLooking;
	bool AlwaysUpdateHeadPos;
};

class CMaterialDef
{
public:
	std::int32_t FirstHitSound;
	std::int32_t LastHitSound;
	float OnHitRumbleLevel;
	float OnHitRumbleFalloff;
	bool DeflectAllHits;
	bool IsDiggable;
	std::vector<std::int32_t> SlideFootParticleEffect;
	std::vector<std::int32_t> SwordHitParticleEffect;
	std::vector<std::int32_t> KnockdownHitFloorParticleEffect;
	std::vector<std::int32_t> WindBlowDustEffect;
	std::vector<std::int32_t> BerserkedSwordHitParticleEffect;
	std::map<std::int32_t,CBlendedParticleEffectSet> BlendedSwordHitEffects;
};

class CMazeBattleDef
{
public:
	float ShieldUpTime;
	float LightningDuration;
	float MinThrowShotRange;
	float LightningRange;
	float EnflameRange;
	std::int32_t TutorialNumGetHits;
	float MinTimeBetweenHeals;
	std::int32_t HealSpellLevel;
	float TutorialTeleportTime;
};

class CMeleeCombatAbilityDef
{
public:
	CDefString FlourishAnimName;
	CDefString FlourishResponseAnimName;
	float DamageMultiplier;
	bool Flourish;
	bool BlockCounter;
	std::int32_t AbilityButtonGraphicOverride;
	std::vector<EWeaponClass> ValidWeaponClasses;
	bool Decapitate;
	std::int32_t KnockDownEffects;
	bool EvadeAllHits;
	bool CauseRecoil;
	CHeroAbilityStatPackage StatsNeeded;
	std::int32_t ComboMultiplierLowerBound;
	std::int32_t ComboMultiplierUpperBound;
};

class CMeleeCombatKnockdownEffects
{
public:
	std::int32_t BlendInFrames;
	std::int32_t BlendOutFrames;
	std::int32_t AttackerLightingChannel;
	std::int32_t TargetLightingChannel;
	std::int32_t EnvironmentTheme;
	bool CameraFOVChange;
	bool CameraPan;
	float AttackerAndTargetSpeedMultiplier;
	bool PauseEntities;
	bool NonAttackingEntities;
	bool AttackerInvulnerable;
};

class CMessageEventDef
{
public:
	std::int32_t MaxSeenRadius;
	std::int32_t MaxHeardRadius;
	std::int32_t Lifespan;
	EReactionSpeechType ReactionSpeechType;
	EMessageEventType Type;
};

class CMultiStaticMeshEntryDef
{
public:
	std::int32_t Mesh;
	bool OverrideLightingChannel;
	bool OverrideRenderFadeDistance;
	float RenderFadeDistance;
	ELightingChannel LightingChannel;
};

class CMultiStaticMeshDef
{
public:
	std::vector<CMultiStaticMeshEntryDef> Meshes;
};

class CMusicEntry
{
public:
	std::uint32_t ID;
	CDefString Filename;
	float Gain;
	bool CacheToHD;
};

class CMusicSetEntry
{
public:
	std::int32_t NormalMusicID;
	std::int32_t DangerMusicID;
	std::int32_t LoopCount;
};

class CNymphDef
{
public:
	float MinAttackRange;
	float MaxHeight;
	float MinHeight;
	float CircleRange;
	std::int32_t FlyAroundTime;
	std::int32_t ShotTrapType;
	std::int32_t ShotsBeforeIdle;
	float IdleTimeBetweenShots;
	float FadeInTime;
	float FadeOutTime;
};

class CObjectAugmentationParticleSet
{
public:
	float PeriodSecs;
	std::int32_t ParticleEffect;
	std::int32_t ParticleEffectToBlendTo;
	std::int32_t AttackParticleEffect;
	std::int32_t AttackParticleEffectToBlendTo;
	std::vector<CDefString> StartPoint;
	std::vector<CDefString> EndPoint;
};

class CObjectAugmentationsDef
{
public:
	std::int32_t NumberOfSlots;
	std::int32_t MaximumNumberOfSlots;
	std::map<EObjectAugmentationType,CObjectAugmentationParticleSet> AugmentationParticles;
	std::uint32_t EmptySlotName;
	std::vector<std::int32_t> InitialAugmentations;
	std::vector<std::int32_t> SlotUpgradeCosts;
	std::map<EObjectAugmentationType,CDefString> InitSounds;
	std::map<EObjectAugmentationType,CDefString> LoopingSounds;
	std::int32_t FireProjectileTrailEffect;
	std::int32_t LightningProjectileTrailEffect;
	std::int32_t FireAndLightningProjectileTrailEffect;
	std::int32_t FireProjectileOnHitEffect;
	std::int32_t LightningProjectileOnHitEffect;
	std::int32_t FireAndLightningProjectileOnHitEffect;
	std::string WeaponTrailAttachmentPoint;
};

class CObjectFamilyEntry
{
public:
	CDefIndex Object;
	float Probability;
};

class CObjectFamilyDef
{
public:
	std::vector<CObjectFamilyEntry> Objects;
};

class COccupiableDef
{
public:
	std::uint32_t TypeFlags;
};

class COpinionDeedReactionDef
{
public:
	std::vector<EOpinionAttitudeType> Attitudes;
	CDefString Animation;
	std::uint32_t DialogueTag;
};

class COpinionTransientOffset
{
public:
	EOpinion Opinion;
	float OffsetPerFrameRunIn;
	std::int32_t FramesToRunIn;
	std::int32_t FramesOfCappedPeak;
	float OffsetPerFrameRunOut;
	std::int32_t FramesToRunOut;
};

class COpinionDeedEffectsDef
{
public:
	std::int32_t KnowledgeLifeInSeconds;
	std::int32_t MinimumSecondsBetweenPostings;
	std::int32_t MinimumSecondsBetweenEffectsForIndividual;
	float PreReactionDelayIgnoreOnDelay;
	std::list<COpinionTransientOffset> Effects;
	std::int32_t ReactionPriority;
	std::vector<COpinionDeedReactionDef> Reaction;
	NSpeechGainManager::EDialogueLayer ReactionDialogueLayer;
	std::vector<std::string> WarningText;
	ECrimeSeverity CrimeSeverity;
	std::int32_t CrimeFineAmount;
	ETutorialCategory Tutorial;
	bool CanBeHeard : 1;
	bool ReactionRestrictToTarget : 1;
	bool AllowSimultaneousReactionDialogue : 1;
	bool CanApologise : 1;
	bool MustNotRunWhenWarned : 1;
	bool OnlyGetFinedForOneInstance : 1;
	bool IsFlirting : 1;
	bool OnReactSetGreeted : 1;
	bool AllowSpeechOnNonPureAISpeaker : 1;
	bool AllowIndirectReactWhileCarrying : 1;
	bool BlockWhileFollowingPlayer : 1;
};

class COpinionDeedMaskDef
{
public:
	bool DeedEnabledDefault;
	std::map<EOpinionDeedType,bool> DeedEnabled;
};

class COpinionOfHeroDef
{
public:
	CDefIndex ThreatenGift;
	bool WillAttackCriminals;
	std::int32_t Personality;
	std::int32_t ReactionMask;
	std::int32_t DeedMask;
};

class COpinionPersonalityTraits
{
public:
	// Opaque 180-byte wire blob: 5 traits x (u32 + 8 floats).
	std::uint8_t Data[180];
};
class COpinionPersonalityDef
{
public:
	COpinionPersonalityTraits PersonalityTraits;
	bool AttitudeEnabledDefaultForNotNone;
	std::map<EOpinionAttitudeType,bool> AttitudeEnabled;
	float ToleranceToBeingHit;
};



class CReactionFrequencyTraits
{
public:
	std::uint32_t Type = 0;
	float MinWait;
	float WaitRange;
	float LowFreqPerMin;
	float HighLowFreqInterval;
	std::uint32_t FramesPerMinute;
	std::uint32_t RampUpTimeFrames;
	std::uint32_t CutoffIntervalFrames;
	bool AllowIndividualRepeats;
	std::uint32_t MinGapFrames;
	float CountRechargePerFrame;
	float CurrentAvailableCount;
	float MaxCount;
	float InvMaxCount;
};

class CReactionFrequencyTraitsArray
{
public:
	CReactionFrequencyTraits Traits[158];
};

class CReactionMatchEntry
{
public:
	std::uint32_t Tag = 0;
	std::int32_t ReactionType;
	std::int32_t Axis;
	std::int32_t XAxisOpinionType;
	std::int32_t YAxisOpinionType;
	float LowerBoundOnAxis;
	float InvIntervalOnAxis;
	float CentreOnAxis;
	float RadiusOnAxis;
	float XCentre;
	float NegInvXRadius;
	float PosInvXRadius;
	float YCentre;
	float NegInvYRadius;
	float PosInvYRadius;
	float MShiftZero;
	float MShiftWeight;
	float RShiftZero;
	float RShiftWeight;
	float ScarinessShift;
	float AgreeablenessShift;
	float AttractivenessShift;
};

class CReactionMatchList
{
public:
	std::vector<CReactionMatchEntry> Elements;
};

class COpinionReactionManagerDef
{
public:
	std::vector<CBlurredAttitudesDef> AttitudeBlur;
	std::map<EOpinionReactionType,EOpinionAttitudeType> AttitudeCondition;
	std::map<EOpinionReactionType,EOpinionTargetingConditionType> TargetingCondition;
	std::map<EOpinionReactionType,float> PreReactionDelay;
	std::map<EOpinionAttitudeType,float> ToleranceToBeingHit;
	std::map<EOpinionReactionType,bool> BlockFurtherReactions;
	std::map<EOpinionReactionType,bool> AllowSpeechOnNonPureAISpeaker;
	std::map<EOpinionReactionType,bool> AllowWhileCarrying;
	std::map<EOpinionReactionType,bool> AllowWhileFollowingPlayer;
	CReactionMatchList Matches;
	CReactionFrequencyTraitsArray FrequencyTraitsArray;
	CReactionFrequencyTraits** FrequencyTraits;
	CReactionFrequencyTraits** FrequencyTraitsHigh;
};

class COpinionReactionMaskDef
{
public:
	bool ReactionEnabledDefault;
	std::map<EOpinionReactionType,bool> ReactionEnabled;
};

class COpinionSet
{
public:
	std::vector<float> OpinionValues;
};

class COpinionSourceDef
{
public:
	std::map<EOpinion,float> ProducedOpinion;
	bool AlwaysAwareOf;
	bool UnknownThreatSource;
	bool ReactNonstop;
	bool ReactToSelfPermitted;
	bool ReactionFlagDefault;
	std::map<EOpinionReactionType,bool> ReactionFlag;
	bool UseNavLineOfSight;
	bool WitnessesWillNotAttack;
	bool WitnessesWillOverrideFactionEnemy;
	bool BypassesPersonality;
	bool RelativeEffect;
	std::int32_t RestrictToFactionAndAllies;
	bool RestrictToFactionIsReversed;
	COpinionSet OpinionProduced;
	bool ReactionFlags[79];
};

class COracleMinigameDef
{
public:
	float WaitBetweenRoundsSeconds;
	float PauseBetweenOutputSeconds;
	float PauseBetweenOutputsLevelMultiplier;
	std::int32_t MaxLevel;
	std::uint32_t TintRed;
	std::uint32_t TintGreen;
	std::uint32_t TintBlue;
	std::uint32_t TextLose;
	std::uint32_t TextWin;
	std::uint32_t TextWatch;
	std::uint32_t TextRepeat;
	std::uint32_t TextOfferGame;
	std::uint32_t TextWonRound;
};

class COverheadDisplayDef
{
public:
	bool DisplayDamage;
	bool DisplayRepair;
	bool DisplayRenown;
};

namespace NParticleAttachment {
class CParticleAttachmentInfo
{
public:
	std::int32_t ParticleIndex;
	CDefString AttachmentObjectName;
	float GenerationCutoffDistance;
	bool DummyObject;
};
}

class CParticleAttacherDef
{
public:
	std::vector<NParticleAttachment::CParticleAttachmentInfo> ParticlesToAttach;
};

class CPerceivedThingDef
{
public:
	float FieldOfView;
	float GiveUpChaseRadius;
	float SightRadius;
	float ExtendedSightRadius;
	float SoundRadius;
	float SmellRadius;
	EPerceivedThingType Type;
	float AwarenessTimer;
	float DayTimeVisibilityMultiplier;
	float NightTimeVisibilityMultiplier;
};

class CPhysicsDef
{
public:
	float Diameter;
	float Friction;
	float Elasticity;
	float AirResistance;
	float Mass;
	std::int32_t InteractionFlags;
	bool PushableByHero;
};

class CPlayerDef
{
public:
	std::int32_t CharacterDef;
	CRGBColour Colour;
};

class CQuickAccessItemButtonGuiDef
{
public:
	std::int32_t ButtonGraphic;
	C2DVector Position;
	C2DVector Offset;
};

class CPlayerGuiDef
{
public:
	std::int32_t InventoryNoMovementSound;
	float SecondsToFadeWholeGui;
	float AlphaValueToFadeWholeGui;
	CRGBColour AmbientColour;
	CRGBColour DiffuseColour;
	CRGBColour BacklightColour;
	C3DVector DiffuseVector;
	C2DVector GuildSealPos;
	std::int32_t GuildSealFlashCycles;
	float GuildSealFlashCycleSeconds;
	float GuildSealReminderTimeSeconds;
	C2DVector HealthBarPos;
	std::int32_t HealthBarLeftGraphic;
	C2DVector HealthBarMiddleOffset;
	float HealthBarMiddleLength;
	std::int32_t HealthBarMiddleGraphic;
	std::int32_t HealthBarRightGraphic;
	C2DVector HealthBarHealthOffset;
	std::int32_t HealthBarHealthGraphic;
	std::int32_t HealthBarHealthEvilGraphic;
	std::int32_t HealthBarHealthGoodGraphic;
	C2DVector HealthBarHealthEffectOffset;
	C2DVector HealthBarHealthIncreaseEffectOffset;
	std::int32_t HealthBarHealthDefaultEffect;
	std::int32_t HealthBarHealthIncreaseEffect;
	std::int32_t HealthBarHealthLevelUpEffect;
	std::int32_t HealthBarHealthAdrenalineEffect;
	std::int32_t HealthBarHealthBerserkEffect;
	std::int32_t HealthBarHealthLowHealthEffect;
	float HealthBarHealthLowPercentage;
	C2DVector HealthBarHealthInnerOffset;
	std::int32_t HealthBarHealthInnerGraphic;
	std::int32_t HealthBarHealthEndGraphic;
	C2DVector HealthBarResurrectionOffset;
	CRGBColour HealthBarResurrectionColour;
	C2DVector HealthBarStaminaBackOffset;
	C2DVector HealthBarStaminaOffset;
	std::int32_t HealthBarStaminaGraphic;
	C2DVector HealthBarStaminaInnerOffset;
	std::int32_t HealthBarStaminaInnerGraphic;
	std::int32_t HealthBarStaminaEndGraphic;
	C2DVector HealthBarStaminaEffectOffset;
	C2DVector HealthBarStaminaIncreaseEffectOffset;
	float HealthBarStaminaIncreaseEffectWidth;
	float HealthBarStaminaIncreaseEffectSegmentWidth;
	std::int32_t HealthBarStaminaDefaultEffect;
	std::int32_t HealthBarStaminaIncreaseEffect;
	std::int32_t HealthBarStaminaIncreaseSegmentEffect;
	C2DVector StealthEyePos;
	std::int32_t StealthEyeFriendGraphic;
	std::int32_t StealthEyeNeutralGraphic;
	std::int32_t StealthEyeEnemyGraphic;
	std::int32_t StealthEyeQuestionMarkGraphic;
	std::vector<std::int32_t> StealthEyeGraphicFrames;
	C2DVector StealthEyeTextOffset;
	float StealthEyeSecondsToOpen;
	float StealthEyeSecondsToThrob;
	float StealthEyeThrobScale;
	std::int32_t DPadRingsGraphic;
	C2DVector DPadRingsPos;
	std::int32_t DPadRingsTopGraphic;
	C2DVector DPadRingsTopOffset;
	std::int32_t DPadRingsExtendGraphic;
	std::vector<std::int32_t> DPadRingsChargeGraphics;
	std::vector<C2DVector> DPadRingsChargeOffsets;
	std::vector<float> DPadRingsChargeAngles;
	std::int32_t DPadRingsFavouriteGraphic;
	std::int32_t DPadRingsTopFavouriteGraphic;
	std::int32_t DPadRingsTopAggressiveGraphic;
	std::int32_t DPadGuildSealLevelUpGraphic;
	C2DVector DPadGuildSealTooltipOffset;
	C2DVector DPadMiddleOffset;
	std::int32_t DPadArrowUpGraphic;
	std::int32_t DPadArrowUpDepGraphic;
	std::int32_t DPadArrowRightGraphic;
	std::int32_t DPadArrowRightDepGraphic;
	std::int32_t DPadArrowDownGraphic;
	std::int32_t DPadArrowDownDepGraphic;
	std::int32_t DPadArrowLeftGraphic;
	std::int32_t DPadArrowLeftDepGraphic;
	std::int32_t DPadGuildSealRecallGraphic;
	std::int32_t GuildSealLevelUpGraphic;
	std::int32_t GuildSealQuestGraphic;
	std::int32_t GuildSealRecallGraphic;
	C2DVector GuildSealLevelUpOffset;
	C2DVector GuildSealMainLevelUpOffset;
	std::int32_t ABXYRingsGraphic;
	C2DVector ABXYRingsPos;
	std::int32_t ABXYRingsAlpha;
	std::int32_t ABXYRingsTopGraphic;
	C2DVector ABXYRingsTopOffset;
	std::int32_t ABXYMiddleGraphic;
	C2DVector ABXYMiddleOffset;
	std::int32_t ABXYMiddleUpGraphic;
	std::int32_t ABXYMiddleRightGraphic;
	std::int32_t ABXYMiddleDownGraphic;
	std::int32_t ABXYMiddleLeftGraphic;
	std::int32_t ABXYRingsFavouriteGraphic;
	std::int32_t ABXYRingsTopFavouriteGraphic;
	std::int32_t WhiteButtonGraphic;
	C2DVector WhiteButtonPos;
	C2DVector WhiteButtonIconOffset;
	C2DVector WhiteButtonTooltipOffset;
	std::int32_t BlackButtonGraphic;
	C2DVector BlackButtonPos;
	C2DVector BlackButtonIconOffset;
	C2DVector BlackButtonTooltipOffset;
	std::int32_t TextBoxTLGraphic;
	std::int32_t TextBoxTMGraphic;
	std::int32_t TextBoxTRGraphic;
	std::int32_t TextBoxMLGraphic;
	std::int32_t TextBoxMRGraphic;
	std::int32_t TextBoxBLGraphic;
	std::int32_t TextBoxBMGraphic;
	std::int32_t TextBoxBRGraphic;
	std::int32_t TextBoxBackGraphic;
	float TextBoxMaxWidth;
	float TextBoxMinWidth;
	float TextBoxMaxHeight;
	float TextBoxMinHeight;
	float TextBoxBorderWidth;
	float TextBoxBorderHeight;
	float TextBoxCutsceneSecondsBeforeSkip;
	float StatBoxMaxWidth;
	std::int32_t StatBoxMiddleGraphic;
	C2DVector StatBoxLeftOffset;
	C2DVector StatBoxRightOffset;
	std::int32_t StatBoxStartGraphic;
	std::int32_t StatBoxEndGraphic;
	std::int32_t StatBoxGoldGraphic;
	std::int32_t StatBoxGoldGainEffect;
	std::int32_t StatBoxGoldLossEffect;
	C2DVector ClockPos;
	std::int32_t ClockFaceGraphic;
	std::int32_t ClockHandGraphic;
	C2DVector ClockRingPos;
	std::int32_t ClockRingGraphic;
	C2DVector ModePos;
	C2DVector ModeOffset;
	C2DVector ModeInnerOffset;
	std::int32_t ModeBorderGraphic;
	std::int32_t ModeLockOffGraphic;
	std::int32_t ModeLockOnGraphic;
	std::int32_t ModeSneakOffGraphic;
	std::int32_t ModeSneakOnGraphic;
	std::int32_t ModeSafetyOffGraphic;
	std::int32_t ModeSafetyOnGraphic;
	C2DVector ModeTextOffset;
	C2DVector CombatPos;
	C2DVector CombatLockedOffset;
	std::vector<std::int32_t> CombatLockedGraphics;
	C2DVector CombatRingOffset;
	std::int32_t CombatRingGraphic;
	C2DVector CombatCurrentOffset;
	C2DVector CombatCurrentTextOffset;
	std::vector<C2DVector> CombatCountOffsets;
	std::int32_t CombatCountEmptyGraphic;
	std::vector<std::int32_t> CombatCountGraphics;
	std::vector<std::int32_t> CombatRingGraphics;
	std::int32_t CombatRingBlackGraphic;
	std::int32_t CombatRingWhiteGraphic;
	std::int32_t CombatCrossGraphic;
	C2DVector CombatCrossOffset;
	C2DVector CombatMiddleOffset;
	float CombatMiddleMinWidth;
	std::int32_t CombatMiddleGraphic;
	std::int32_t CombatEndGraphic;
	C2DVector ExperiencePos;
	std::int32_t ExperienceGraphic;
	std::int32_t ExperienceGeneralGraphic;
	std::int32_t ExperienceSkillGraphic;
	std::int32_t ExperienceStrengthGraphic;
	std::int32_t ExperienceWillGraphic;
	C2DVector ExperienceTextOffset;
	CRGBColour ExperienceTextColour;
	C2DVector ExperienceMiddleOffset;
	float ExperienceMiddleMinWidth;
	std::int32_t ExperienceMiddleGraphic;
	std::int32_t ExperienceEndGraphic;
	C2DVector CrimePos;
	C2DVector CrimeTextOffset;
	CRGBColour CrimeTextColour;
	CRGBColour CrimeTextAlertColour;
	std::int32_t CrimeWarningGraphic;
	std::int32_t CrimeCriminalGraphic;
	std::int32_t CrimeBarGraphic;
	C2DVector CrimeBarIconOffset;
	C2DVector CrimeBarBorderOffset;
	std::vector<C2DVector> CrosshairOffsets;
	std::vector<std::int32_t> CrosshairGraphics;
	std::vector<std::int32_t> CrosshairGreenGraphics;
	std::int32_t MiniGameFishingLeftGraphic;
	std::int32_t MiniGameFishingRightGraphic;
	std::int32_t MiniGameBettingGraphic;
	std::int32_t MiniGameDiggingGraphic;
	std::int32_t MiniGamePickpocketGraphic;
	std::int32_t MiniGamePicklockGraphic;
	std::int32_t MiniGameStealGraphic;
	std::int32_t MiniGameTrophyGotGraphic;
	std::int32_t MiniGameTrophyLeftGraphic;
	std::int32_t SmackEffect;
	std::int32_t SmackAlphaFadePerFrame;
	std::int32_t SmackAlphaStartValue;
	std::int32_t SmackAlphaEndValue;
	float SmackScaleFactorPerFrame;
	float SmackTimeToRotateSeconds;
	C2DVector MoneyPos;
	C2DVector MoneyTextOffset;
	C2DVector MoneyTextRightOffset;
	float MoneyWidth;
	CRGBColour MoneyTextColour;
	CRGBColour MoneyIncreaseTextColour;
	CRGBColour MoneyDecreaseTextColour;
	C2DVector MoneyChangeStartOffset;
	C2DVector MoneyChangeEndOffset;
	float MoneyChangeTextSpeed;
	std::string StatBarsFontName;
	float SecondsForStatsBarsToAppear;
	float SecondsBeforeStatsBackBarMoves;
	float SecondsToShowStatsBars;
	float SecondsForStatsBarsToDisappear;
	C2DVector StatUpdatePos;
	C2DVector StatUpdateMoralityGoodPos;
	C2DVector StatUpdateMoralityEvilPos;
	C2DVector StatUpdateRenownPos;
	C2DVector StatUpdateTextOffset;
	CRGBColour StatUpdateTextColour;
	float StatUpdateDisplayDuration;
	float StatUpdateWaitingDuration;
	C2DVector StatUpdateEffectOffset;
	std::int32_t StatUpdateMoralityGoodGraphic;
	std::int32_t StatUpdateMoralityEvilGraphic;
	std::int32_t StatUpdateMoralityGoodEffect;
	std::int32_t StatUpdateMoralityEvilEffect;
	std::vector<CRGBColour> StatUpdateColours;
	std::vector<std::int32_t> StatUpdateGraphics;
	std::vector<std::int32_t> StatUpdateEffects;
	C2DVector AutoPickupPos;
	C2DVector QuestInfoPos;
	float QuestInfoBarOffsetX;
	std::int32_t QuestInfoGraphicWidth;
	std::int32_t QuestInfoGraphicHeight;
	float QuestInfoGraphicMinHeight;
	float QuestInfoSecondsToScroll;
	float QuestInfoSecondsToAppear;
	CRGBColour QuestInfoTurncoatColour;
	std::int32_t QuestInfoTickGraphic;
	C2DVector OracleSymbolPos;
	C2DVector OracleSymbolOffset;
	C2DVector TextBoxTextAreaTLPos;
	C2DVector TextBoxTextAreaBRPos;
	CRGBColour TextBoxTextColour;
	CRGBColour TextBoxTextGameInfoColour;
	CRGBColour TextBoxTextGameActionColour;
	C2DVector TextBoxButtonGraphicOffset;
	float TextBoxButtonOffsetY;
	float TextBoxButtonInBoxOffsetY;
	float TextBoxButtonGapWidth;
	std::string TextBoxButtonTextFontName;
	std::int32_t TextBoxConfirmGraphic;
	std::int32_t TextBoxCancelGraphic;
	std::int32_t TextBoxThirdGraphic;
	std::int32_t TextBoxSkipCutsceneGraphic;
	std::string TextBoxButtonASoundCriteria;
	std::string TextBoxButtonBSoundCriteria;
	std::int32_t TextBoxHugeButtonGraphic;
	std::int32_t TextBoxHugeMouseButtonGraphic;
	std::int32_t TextBoxHugeMouseButtonLeftGraphic;
	C2DVector TextBoxHugeButtonOffset;
	C2DVector TextBoxHugeButtonLeftOffset;
	C2DVector TextBoxFishingReelTextPos;
	C2DVector TextBoxFishingReelGraphicOffset;
	C2DVector TextBoxBettingGraphicPos;
	C2DVector TextBoxBettingUpOffset;
	C2DVector TextBoxBettingDownOffset;
	C2DVector TextBoxBettingLeftOffset;
	C2DVector TextBoxBettingRightOffset;
	std::string ReceiveItemsSoundCriteria;
	std::int32_t ControllerAGraphic;
	std::int32_t ControllerBGraphic;
	std::int32_t ControllerXGraphic;
	std::int32_t ControllerYGraphic;
	std::int32_t ControllerDPadGraphic;
	std::int32_t ControllerDPadDownGraphic;
	std::int32_t ControllerDPadLeftGraphic;
	std::int32_t ControllerDPadRightGraphic;
	std::int32_t ControllerDPadUpGraphic;
	std::int32_t ControllerThumbLeftGraphic;
	std::int32_t ControllerThumbRightGraphic;
	std::int32_t ControllerThumbClickLeftGraphic;
	std::int32_t ControllerThumbClickRightGraphic;
	std::int32_t ControllerBackGraphic;
	std::int32_t ControllerStartGraphic;
	std::int32_t ControllerTriggerLeftGraphic;
	std::int32_t ControllerTriggerRightGraphic;
	std::int32_t ControllerTriggerBlackGraphic;
	std::int32_t ControllerTriggerWhiteGraphic;
	C2DVector ControllerGraphicOffset;
	C2DVector ControllerGraphicTextOffset;
	C2DVector ControllerClickGraphicTextOffset;
	C2DVector ControllerTriggerGraphicTextOffset;
	C2DVector MenuConfirmPos;
	C2DVector MenuConfirmTextPos;
	std::int32_t MenuConfirmGraphic;
	C2DVector MenuCancelPos;
	C2DVector MenuCancelTextPos;
	std::int32_t MenuCancelGraphic;
	std::int32_t QuickAccessDPadGraphic;
	std::int32_t QuickAccessMenuGraphic;
	std::int32_t QuickAccessLampEffect;
	std::int32_t QuickAccessGuildSealEffect;
	std::int32_t QuickAccessMainGuildSealEffect;
	C2DVector QuickAccessEffectOffset;
	C2DVector QuickAccessAbilityCrossOffset;
	std::int32_t HotBarSingleGraphic;
	std::int32_t HotBarMainGraphic;
	std::vector<std::int32_t> HotBarEndGraphics;
	C2DVector HotBarOffset;
	C2DVector HotBarEndOffset;
	std::int32_t HotBarItemWidth;
	std::vector<CQuickAccessItemButtonGuiDef> QuickAccessItemButtons;
	std::vector<CQuickAccessItemButtonGuiDef> ContextSensitiveItemButtons;
	std::int32_t TargetingCursorSectionEnemyEmptyGraphic;
	std::int32_t CompassGraphic;
	C2DVector CompassPos;
	std::int32_t CompassShadowGraphic;
	C2DVector CompassShadowOffset;
	std::int32_t AbilityButtonTalkGraphic;
	std::int32_t AbilityButtonActivateGraphic;
	std::int32_t AbilityButtonBlockGraphic;
	std::int32_t AbilityButtonUnarmedGraphic;
	std::int32_t AbilityButtonMeleeGraphic;
	std::int32_t AbilityButtonRangedGraphic;
	std::int32_t AbilityButtonSheatheAxeGraphic;
	std::int32_t AbilityButtonSheatheBowGraphic;
	std::int32_t AbilityButtonSheatheMaceGraphic;
	std::int32_t AbilityButtonSheatheStickGraphic;
	std::int32_t AbilityButtonSheatheSwordGraphic;
	C2DVector AbilityButtonFlourishEffectPos;
	std::int32_t AbilityButtonFlourishGraphic;
	C2DVector AbilityButtonFlourishGraphicOffset;
	std::int32_t ABXYButtonMiddleGraphic;
	C2DVector ABXYAbilityLevelInitialOffset;
	C2DVector ABXYAbilityLevelOffset;
	std::int32_t ABXYAbilityUnavailableGraphic;
	std::int32_t ABXYAbilityUncastableGraphic;
	std::int32_t ABXYAbilitySwitchGraphic;
	std::int32_t ABXYAbilityShieldOffGraphic;
	std::int32_t ABXYRunGraphic;
	std::int32_t ABXYTargetingEnterGraphic;
	std::int32_t ABXYTargetingCancelGraphic;
	float ABXYButtonFadeDurationSeconds;
	std::int32_t TargetingBorderLeftGraphic;
	C2DVector TargetingBorderLeftPos;
	std::int32_t TargetingBorderRightGraphic;
	C2DVector TargetingBorderRightPos;
	std::int32_t InteractButtonBorderGraphic;
	C2DVector InteractButtonBorderOffset;
	C2DVector InteractButtonTooltipOffset;
	std::vector<CAbilityButtonGuiDef> AbilityButtons;
	std::vector<CAbilityButtonGuiDef> SpecialAbilityButtons;
	C2DVector DigitPos;
	C2DVector DigitHeroPos;
	C2DVector DigitHeroOffset;
	std::vector<std::int32_t> DigitGraphics;
	C2DVector RegionDisplayPos;
	C2DVector InfoDisplayPos;
	std::int32_t HeroSprintBarLeftGraphic;
	std::int32_t HeroSprintBarLeftGlowGraphic;
	std::int32_t HeroSprintBarMiddleGraphic;
	std::int32_t HeroSprintBarMiddleGlowGraphic;
	std::int32_t HeroSprintBarRightGraphic;
	std::int32_t HeroSprintBarRightGlowGraphic;
	std::int32_t HeroSprintBarInnerGraphic;
	C2DVector HeroSprintBarInnerGraphicOffset;
	std::vector<CRGBColour> HighlightStartColour;
	std::vector<CRGBColour> HighlightEndColour;
	std::vector<float> HighlightStartWidthWorldSpace;
	std::vector<float> HighlightEndWidthWorldSpace;
	std::vector<float> HighlightStartWidthScreenSpace;
	std::vector<float> HighlightEndWidthScreenSpace;
	std::vector<std::int32_t> HighlightFadeSteps;
	float NumberOfSecondsForContainerItemToReachFullSize;
	C2DVector ContainerItemDisplayAreaTLPos;
	C2DVector ContainerItemDisplayAreaBRPos;
	float NumberOfSecondsForContainerItemToRotate;
	float NumberOfSecondsForContainerItemToStayAtFullSize;
	CRGBColour ContainerItemDisplayBackgroundColour;
	C2DVector ScreenMessageTextPos;
	CRGBColour ScreenMessageTextColour;
	C2DVector ScreenMessageTextDropShadowOffset;
	CRGBColour ScreenMessageTextDropShadowColour;
	std::string ScreenMessageFont;
	C2DVector ScreenMessageNextMessageTextOffset;
	std::int32_t MaximumNumberOfScreenMessages;
	float NumberOfSecondsScreenMessageStaysOnScreen;
	std::int32_t NumberOfCharactersScreenMessageAddsPerFrame;
	std::vector<std::string> TutorialText;
	std::vector<std::string> TutorialLogBookText;
	std::vector<std::int32_t> TutorialGuildSealGraphics;
	std::map<std::string,std::int32_t> ScriptSprites;
	std::map<std::string,EGameAction> GameActionValues;
	std::map<std::string,std::int32_t> MiniMapGraphics;
	std::vector<std::int32_t> MiniMapMarkerGraphics;
	C2DVector MiniMapScreenOrigin;
	C2DVector MiniMapScreenSize;
	std::int32_t MiniMapScreenRadius;
	std::int32_t MiniMapTextureWidth;
	std::int32_t MiniMapMarkerTextureWidth;
	float MiniMapWorldResolution;
	CRGBColour MiniMapPassableColour;
	CRGBColour MiniMapImpassableColour;
	CRGBColour MiniMapWaterColour;
	CRGBColour MiniMapMaplessColour;
	CRGBColour MiniMapImpassableWaterColour;
	std::uint32_t MaxMinimapZoneColours;
	std::uint32_t MaxMinimapThemeColours;
	std::vector<CRGBColour> MinimapZoneColours;
	std::vector<CRGBColour> MinimapThemeColours;
	C2DVector RegionChangeTLOffset;
	C2DVector RegionChangeTMOffset;
	C2DVector RegionChangeTROffset;
	C2DVector RegionChangeBLOffset;
	C2DVector RegionChangeBMOffset;
	C2DVector RegionChangeBROffset;
	std::int32_t RegionChangeTLGraphic;
	std::int32_t RegionChangeTMGraphic;
	std::int32_t RegionChangeTRGraphic;
	std::int32_t RegionChangeBLGraphic;
	std::int32_t RegionChangeBMGraphic;
	std::int32_t RegionChangeBRGraphic;
	C2DVector RegionChangeBorderLeftOffset;
	C2DVector RegionChangeBorderRightOffset;
	std::int32_t RegionChangeBorderLeftGraphic;
	std::int32_t RegionChangeBorderRightGraphic;
	std::int32_t RegionChangeGuildSealBorderGraphic;
	C2DVector RegionChangeGuildSealBorderOffset;
	std::vector<std::int32_t> RegionChangeGuildSealGraphics;
	std::int32_t LoadProgressBarLeftGraphic;
	std::int32_t LoadProgressBarMidGraphic;
	std::int32_t LoadProgressBarRightGraphic;
	std::int32_t LoadProgressBarInnerGraphic;
	std::vector<std::int32_t> LoadProgressBackdropGraphics;
	C2DVector TavernGameBlockTLOffset;
	C2DVector TavernGameBlockTMOffset;
	C2DVector TavernGameBlockTROffset;
	C2DVector TavernGameBlockBLOffset;
	C2DVector TavernGameBlockBMOffset;
	C2DVector TavernGameBlockBROffset;
	std::int32_t TavernGameBlockTLGraphic;
	std::int32_t TavernGameBlockTMGraphic;
	std::int32_t TavernGameBlockTRGraphic;
	std::int32_t TavernGameBlockBLGraphic;
	std::int32_t TavernGameBlockBMGraphic;
	std::int32_t TavernGameBlockBRGraphic;
	std::string RegionGraphFileName;
	float MouseCursorScaleX;
	float MouseCursorScaleY;
	std::int32_t MouseCursorGraphic;
	std::int32_t MouseLMBGraphic;
	std::int32_t MouseMMBGraphic;
	std::int32_t MouseRMBGraphic;
	std::int32_t MouseOverlayGraphic;
};

class CPlayerInventoryDef
{
public:
	std::vector<std::int32_t> Slot;
	std::vector<std::int32_t> MaxInSlot;
	std::vector<std::int32_t> InitialAmount;
	std::vector<bool> IsUsable;
	std::int32_t InitialSelectedSlot;
};

class CPlayerMovementDef
{
public:
	float JoystickMovementStationaryBand;
	float JoystickMovementSlowBand;
	float JoystickMovementWalkingBand;
	float JoystickMovementJoggingBand;
	float MinRequiredAngleFor180;
	float SecondsToStoreLastMovementFor;
	std::int32_t NoMovementFramesToCheckFor180;
	std::int32_t NoMovementFramesToCheckFor180PriorStraightLine;
	float MinNoRunSecondsForSkidHaltMove;
	std::int32_t NoRunningFramesToPreventSkidHalt;
	float MaxAllowedAngleChangeForSkidHalt;
	float MaxTurnSpeed;
	float MaxTurnSpeedHeavyWeapon;
	float LeanAngleTurnFactor;
	float LeanAngleLimitAngle;
	float LeanAngleStepPerFrame;
};

class CPrizeScoreDef
{
public:
	float Score;
	std::int32_t Mult;
};

class CQuestCardDef
{
public:
	std::int32_t QuestName;
	std::int32_t QuestSummary;
	std::int32_t QuestObjective;
	std::int32_t SuccessSummary;
	CDefString RegionName;
	CDefString TeleporterRegionName;
	std::int32_t InventoryCategory;
	std::int32_t RenownReward;
	std::int32_t GoldReward;
	std::vector<std::int32_t> RewardObjects;
	bool IsCore;
	bool IsVignette;
	bool IsExclusive;
	bool MakeVignetteRouteAppearOnMinimap;
	std::map<EQuestCardPrerequisite,std::int32_t> Prerequisites;
	std::int32_t NumBoasts;
	bool CanPlayerCancel;
	std::uint32_t QuestEpilogue;
};

class CRGBFloatColour
{
public:
	float R;
	float G;
	float B;
	float A;
};

class CRandomAppearanceMorph
{
public:
	class CTextureMorphEntry
	{
	public:
		std::uint8_t Morph[8];
	};

	class CTextureMorphTable
	{
	public:
		std::vector<std::pair<std::uint32_t,CRandomAppearanceMorph::CTextureMorphEntry>> Entries;
		std::uint32_t Trailing = 0;
		std::uint8_t Dirty = 0;
	};

	class CBodyPart0Mesh
	{
	public:
		std::int32_t MeshId;
		std::int32_t TextureId;
		CRandomAppearanceMorph::CTextureMorphTable TextureMorphs;
	};

	class CBodyPart1Mesh
	{
	public:
		std::int32_t MeshId;
		std::int32_t TextureId;
		CRandomAppearanceMorph::CTextureMorphTable TextureMorphs;
	};

	class CBodyPart2Mesh
	{
	public:
		std::int32_t MeshId;
		std::int32_t TextureId;
		CRandomAppearanceMorph::CTextureMorphTable TextureMorphs;
	};

	class CBodyPart0
	{
	public:
		std::vector<CRandomAppearanceMorph::CBodyPart0Mesh> Meshes;
	};

	class CBodyPart1
	{
	public:
		std::vector<CRandomAppearanceMorph::CBodyPart1Mesh> Meshes;
	};

	class CBodyPart2
	{
	public:
		std::vector<CRandomAppearanceMorph::CBodyPart2Mesh> Meshes;
	};

	class CSkeletalMorphEntry
	{
	public:
		std::int32_t Key;
		std::int32_t MorphId;
		std::int32_t BoneOrMorphIndex;
		bool AllowVariation;
	};

	std::uint32_t NumBodyParts = 3;
	CRandomAppearanceMorph::CBodyPart0 BodyParts0;
	CRandomAppearanceMorph::CBodyPart1 BodyParts1;
	CRandomAppearanceMorph::CBodyPart2 BodyParts2;
	CRandomAppearanceMorph::CTextureMorphTable BodyPartTextureMorphs0;
	CRandomAppearanceMorph::CTextureMorphTable BodyPartTextureMorphs1;
	CRandomAppearanceMorph::CTextureMorphTable BodyPartTextureMorphs2;
	std::vector<CRandomAppearanceMorph::CSkeletalMorphEntry> SkeletalMorphs;
	CRandomAppearanceMorph::CTextureMorphTable GeneralTextureMorphs;
	std::uint32_t FinalTrailing;
};


class CReadableDef
{
public:
	bool UseInfoDisplay;
};

class CRegionDef
{
public:
	std::int32_t EnvironmentTheme;
	std::int32_t InternalEnvironmentTheme;
	std::int32_t InternalLightingChannelCount;
	std::int32_t CameraManagerSetIndex;
	std::int32_t CameraManagerSetBuildingIndex;
	CSoundMap SoundMap;
	bool PreventHeroWeaponUse;
	bool PreventHeroWillUse;
	CDefString WanderingPopulationScriptDefName;
	std::int32_t Music;
	EReverbEnvironmentType ReverbEnvironmentType;
	float ReverbEnvironmentLevel;
	float InBuildingOcclusionLevel;
	bool UseVillageTime;
	bool AllowHenchmen;
	std::int32_t AttachToCameraFX;
	bool EnableSpotlightShadowMode;
	bool CheckAllThingsOnMapForCombatCollision;
	std::int32_t HeroBreathParticle;
	std::int32_t MaxNumAttackersOnHero;
	bool WeaponOutCrime;
	bool UseCrimes;
	bool DisplayCrimes;
	bool PermitCrimes;
	std::uint32_t RegionDescription;
	bool HasSea;
	std::int32_t LockToFrameRate;
	C2DVector WorldMapOffset;
	C2DVector NameGraphicOffset;
	EWorldMapNameGraphic NameGraphic;
	float FishWeightMult;
};

class CRegionScriptDef
{
public:
	std::int32_t RandomVillagerMax;
	std::int32_t RandomGuardMax;
	std::int32_t RandomBanditMax;
	std::int32_t RegionDangerLevel;
};

namespace NGraphicAppearance {
class CReplaceableMeshEntry
{
public:
	std::int32_t BankIndex;
	float AnimStep;
	float RenderSizeX = 1.0f;
	std::uint8_t AdditiveAlpha;
	std::uint8_t GraphicType;
};

class CReplaceableMeshes
{
public:
	std::vector<CReplaceableMeshEntry> Vector;
};
}

class CReplaceableMeshDef
{
public:
	NGraphicAppearance::CReplaceableMeshes Meshes;
};

class CResurrectionItemDef
{
public:
	std::int32_t OnUseParticleEffect;
};

class CReverbEnvironmentEntry
{
public:
	float Params[21];
};

class CRumbleDef
{
public:
	std::map<EQuakeStrength,float> QuakeIntensities;
	std::map<EQuakeLength,float> QuakeDurations;
};

class CScorpionKingBattleDef
{
public:
	std::int32_t NumChargesBeforeTailStrike;
	float UnderGroundTime;
	float TailPlungeTime;
	float SpikeTime;
	float SpikeDecisionTime;
	float TailStrikePrepareTime;
	float DamageToCauseTailStrike;
	float ChargeDamage;
	float ClawObstructionRadius;
};

class CScriptDef
{
public:
	std::int32_t EXPERIENCE_GRANT_SMALL;
	std::int32_t EXPERIENCE_GRANT_MEDIUM;
	std::int32_t EXPERIENCE_GRANT_LARGE;
	std::int32_t EXPERIENCE_GRANT_XLARGE;
	std::int32_t EXPERIENCE_GRANT_HEROIC;
	std::int32_t NPC_COMMENT_FREQUENCY;
	std::int32_t NPC_COMMENT_DISTANCE;
	std::int32_t AmbushTradersNakedBoastCost;
	std::int32_t AmbushTradersNakedBoastReward;
	std::int32_t AmbushTradersNoDamageBoastCost;
	std::int32_t AmbushTradersNoDamageBoastReward;
	std::int32_t AmbushTradersNoWeaponsBoastCost;
	std::int32_t AmbushTradersNoWeaponsBoastReward;
	std::int32_t ArenaNakedBoastCost;
	std::int32_t ArenaNakedBoastReward;
	std::int32_t ArenaNoDamageBoastCost;
	std::int32_t ArenaNoDamageBoastReward;
	std::int32_t ArenaNoWeaponsBoastCost;
	std::int32_t ArenaNoWeaponsBoastReward;
	std::int32_t BanditCampNoDamageBoastCost;
	std::int32_t BanditCampNoDamageBoastReward;
	std::int32_t BanditCampNoWeaponsBoastCost;
	std::int32_t BanditCampNoWeaponsBoastReward;
	std::int32_t BountyHuntNakedBoastCost;
	std::int32_t BountyHuntNakedBoastReward;
	std::int32_t BountyHuntNoDamageBoastCost;
	std::int32_t BountyHuntNoDamageBoastReward;
	std::int32_t BreakSiegeNakedBoastCost;
	std::int32_t BreakSiegeNakedBoastReward;
	std::int32_t BreakSiegeNoDamageBoastCost;
	std::int32_t BreakSiegeNoDamageBoastReward;
	std::int32_t BreakSiegeNoWeaponsBoastCost;
	std::int32_t BreakSiegeNoWeaponsBoastReward;
	std::int32_t GraveyardNakedBoastCost;
	std::int32_t GraveyardNakedBoastReward;
	std::int32_t GraveyardNoDamageBoastCost;
	std::int32_t GraveyardNoDamageBoastReward;
	std::int32_t GraveyardNoWeaponsBoastCost;
	std::int32_t GraveyardNoWeaponsBoastReward;
	std::int32_t HobbeCaveNakedBoastCost;
	std::int32_t HobbeCaveNakedBoastReward;
	std::int32_t HobbeCaveNoDamageBoastCost;
	std::int32_t HobbeCaveNoDamageBoastReward;
	std::int32_t HobbeCaveNoWeaponsBoastCost;
	std::int32_t HobbeCaveNoWeaponsBoastReward;
	std::int32_t HTContestNakedBoastCost;
	std::int32_t HTContestNakedBoastReward;
	std::int32_t HTContestNoDamageBoastCost;
	std::int32_t HTContestNoDamageBoastReward;
	std::int32_t HTContestNoWeaponsBoastCost;
	std::int32_t HTContestNoWeaponsBoastReward;
	std::int32_t HTEvilNakedBoastCost;
	std::int32_t HTEvilNakedBoastReward;
	std::int32_t HTEvilNoDamageBoastCost;
	std::int32_t HTEvilNoDamageBoastReward;
	std::int32_t HTEvilNoWeaponsBoastCost;
	std::int32_t HTEvilNoWeaponsBoastReward;
	std::int32_t HTGoodNakedBoastCost;
	std::int32_t HTGoodNakedBoastReward;
	std::int32_t HTGoodNoDamageBoastCost;
	std::int32_t HTGoodNoDamageBoastReward;
	std::int32_t HTGoodNoWeaponsBoastCost;
	std::int32_t HTGoodNoWeaponsBoastReward;
	std::int32_t MCCNakedBoastCost;
	std::int32_t MCCNakedBoastReward;
	std::int32_t MCCNoDamageBoastCost;
	std::int32_t MCCNoDamageBoastReward;
	std::int32_t MCCNoWeaponsBoastCost;
	std::int32_t MCCNoWeaponsBoastReward;
	std::int32_t MinionCampNakedBoastCost;
	std::int32_t MinionCampNakedBoastReward;
	std::int32_t MinionCampNoDamageBoastCost;
	std::int32_t MinionCampNoDamageBoastReward;
	std::int32_t MinionCampNoWeaponsBoastCost;
	std::int32_t MinionCampNoWeaponsBoastReward;
	std::int32_t OFEvilNakedBoastCost;
	std::int32_t OFEvilNakedBoastReward;
	std::int32_t OFEvilNoDamageBoastCost;
	std::int32_t OFEvilNoDamageBoastReward;
	std::int32_t OFEvilNoWeaponsBoastCost;
	std::int32_t OFEvilNoWeaponsBoastReward;
	std::int32_t OFGoodNakedBoastCost;
	std::int32_t OFGoodNakedBoastReward;
	std::int32_t OFGoodNoDamageBoastCost;
	std::int32_t OFGoodNoDamageBoastReward;
	std::int32_t OFGoodNoWeaponsBoastCost;
	std::int32_t OFGoodNoWeaponsBoastReward;
	std::int32_t PrisonNakedBoastCost;
	std::int32_t PrisonNakedBoastReward;
	std::int32_t PrisonNoDamageBoastCost;
	std::int32_t PrisonNoDamageBoastReward;
	std::int32_t PrisonNoWeaponsBoastCost;
	std::int32_t PrisonNoWeaponsBoastReward;
	std::int32_t RansomVictimNakedBoastCost;
	std::int32_t RansomVictimNakedBoastReward;
	std::int32_t RansomVictimNoDamageBoastCost;
	std::int32_t RansomVictimNoDamageBoastReward;
	std::int32_t RansomVictimNoWeaponsBoastCost;
	std::int32_t RansomVictimNoWeaponsBoastReward;
	std::int32_t TraderConflictEvilNakedBoastCost;
	std::int32_t TraderConflictEvilNakedBoastReward;
	std::int32_t TraderConflictEvilNoDamageBoastCost;
	std::int32_t TraderConflictEvilNoDamageBoastReward;
	std::int32_t TraderConflictEvilNoWeaponsBoastCost;
	std::int32_t TraderConflictEvilNoWeaponsBoastReward;
	std::int32_t TraderConflictGoodNakedBoastCost;
	std::int32_t TraderConflictGoodNakedBoastReward;
	std::int32_t TraderConflictGoodNoDamageBoastCost;
	std::int32_t TraderConflictGoodNoDamageBoastReward;
	std::int32_t TraderConflictGoodNoWeaponsBoastCost;
	std::int32_t TraderConflictGoodNoWeaponsBoastReward;
	std::int32_t TraderEscortNakedBoastCost;
	std::int32_t TraderEscortNakedBoastReward;
	std::int32_t TraderEscortNoDamageBoastCost;
	std::int32_t TraderEscortNoDamageBoastReward;
	std::int32_t TraderEscortNoWeaponsBoastCost;
	std::int32_t TraderEscortNoWeaponsBoastReward;
	std::int32_t WhiteBalvNakedBoastCost;
	std::int32_t WhiteBalvNakedBoastReward;
	std::int32_t WhiteBalvNoDamageBoastCost;
	std::int32_t WhiteBalvNoDamageBoastReward;
	std::int32_t WhiteBalvNoWeaponsBoastCost;
	std::int32_t WhiteBalvNoWeaponsBoastReward;
	std::int32_t SummoningShipNakedBoastCost;
	std::int32_t SummoningShipNakedBoastReward;
	std::int32_t SummoningShipNoDamageBoastCost;
	std::int32_t SummoningShipNoDamageBoastReward;
	std::int32_t TCGKillNoBanditsCost;
	std::int32_t TCGKillNoBanditsReward;
	std::int32_t TCGMadeTimeLimitCost;
	std::int32_t TCGMadeTimeLimitReward;
	std::int32_t TCEKeepBanditFollowerAliveCost;
	std::int32_t TCEKeepBanditFollowerAliveReward;
	std::int32_t TCEMadeTimeLimitCost;
	std::int32_t TCEMadeTimeLimitReward;
	std::int32_t RansomVictimVictimNoDamageCost;
	std::int32_t RansomVictimVictimNoDamageReward;
	std::int32_t RansomVictimKillKidnapperCost;
	std::int32_t RansomVictimKillKidnapperReward;
	std::int32_t RansomVictimSaveVictimCost;
	std::int32_t RansomVictimSaveVictimReward;
	std::int32_t RansomVictimHaveVictimKilledCost;
	std::int32_t RansomVictimHaveVictimKilledReward;
	std::int32_t MinionCampBriarNoDamageCost;
	std::int32_t MinionCampBriarNoDamageReward;
	float AmbushScamTricksterApproachProximityLow;
	float AmbushScamTricksterApproachProximityHigh;
	float AmbushScamTricksterHeroProximity;
	float AmbushScamTricksterRunAwayProximity;
	float AmbushScamAmbushTriggerProximity;
	float AmbushScamApproachTriggerProximity;
	float AmbushScamBuddyWanderMax;
	float AmbushScamTricksterWanderMax;
	std::int32_t AmbushScamGoldReward;
	CDefString AmbushScamBonusObject;
	std::int32_t AmbushScamExperience;
	std::int32_t AmbushScamRenown;
	float AmbushScamMoralityEvil;
	float AmbushScamMoralityGood;
	std::int32_t AmbushScamRandomSpeechTimerMax;
	std::int32_t AmbushScamRandomSpeechTimerVar;
	std::vector<CDefString> AmbushScamScreamSoundIds;
	float AmbushScamScreamProximity;
	std::int32_t ArcheryCompetitionCost;
	std::int32_t ArcheryGameTimeLimit;
	float ArcheryTimesUpDuration;
	std::int32_t ArcheryInitialHighScoreMax;
	std::int32_t ArcheryInitialHighScoreVar;
	float ArcheryScoreForPrize;
	float ArcheryScoreForPrizeHigh;
	float ArcheryScoreForPrizeMedium;
	float ArcheryScoreForPrizeLow;
	float ArcheryMaxOutsideRing;
	std::int32_t ArcheryDummySpeedFast;
	std::int32_t ArcheryDummySpeedMedium;
	std::int32_t ArcheryDummySpeedSlow;
	std::int32_t ArcheryNumberOfMoves;
	CDefString ArcheryRewardObject;
	std::vector<CDefString> ArcheryNormalPrizes;
	std::vector<CDefString> ArcheryNormalPrizesHigh;
	std::vector<CDefString> ArcheryNormalPrizesMedium;
	std::vector<CDefString> ArcheryNormalPrizesLow;
	std::vector<CDefString> ArcheryMatchHighScorePrizes;
	std::vector<CDefString> ArcheryNewHighScorePrizes;
	std::vector<float> ArcheryScorePowerLevels;
	CDefString ArcheryReallyHighScorePrize;
	std::int32_t ArcheryReallyHighScore;
	std::int32_t ArcheryExperience;
	std::int32_t ArcheryRenown;
	float ArcheryMoralityCheatUsedMagic;
	float ArcheryMoralityCheatSteppedOut;
	float ArcheryMoralityShotOwner;
	std::int32_t ArcheryScoreDummyStatic;
	std::int32_t ArcheryScoreDummyFront;
	std::int32_t ArcheryScoreDummyCentre;
	std::int32_t ArcheryScoreDummyLeft;
	std::int32_t ArcheryScoreDummyRight;
	std::int32_t ArcheryRandomSpeechTimerMax;
	std::int32_t ArcheryRandomSpeechTimerVar;
	std::int32_t AssassinActivateRenownLevel;
	std::int32_t AssassinTriggerProximity;
	std::int32_t AssassinScreamProximity;
	std::int32_t AssassinChestProximity;
	std::int32_t AssassinCameraOffScreenTime;
	std::vector<CDefString> AssassinScreamSoundIds;
	std::int32_t AssassinReward;
	float AssassinRewardDelay;
	std::vector<std::string> AssassinObjects;
	std::vector<std::string> AssassinLocationName;
	std::vector<std::int32_t> AssassinRegionKillCount;
	std::int32_t AssassinExperience;
	std::int32_t AssassinRenown;
	float AssassinMorality;
	std::int32_t BanditCampPathEavesDropProximity;
	std::int32_t BanditCampKillNoBanditsBoastCost;
	std::int32_t BanditCampKillNoBanditsBoastReward;
	std::int32_t BanditCampKillManyBanditsBoastCost;
	std::int32_t BanditCampKillManyBanditsBoastReward;
	std::int32_t KillTraderTriggerDistance;
	std::int32_t TollBanditCallHeroDistance;
	std::int32_t TollBanditWarningOverDistance;
	std::int32_t TollBanditHeroLeftDistance;
	std::int32_t TollBanditPayDistance;
	std::int32_t TollBanditCallHeroTimer;
	std::int32_t TollBanditWarnHeroTimer;
	std::int32_t TollBanditTauntTimer;
	std::int32_t BanditTollSneakDistance;
	std::int32_t BanditTollBanditWanderDistance;
	std::int32_t BanditTollTraderWarnDistance;
	std::int32_t TollBanditFee;
	std::int32_t TollBanditFeeWithFollower;
	CDefString TollBanditCreatureType;
	float TollBanditCloseToExit;
	float TollBanditTimeToFade;
	std::int32_t BanditCountForToll;
	float BeardyBaldyWanderDistanceMin;
	float BeardyBaldyWanderDistanceMax;
	CDefString BeardyBaldyHaircutStyle1;
	CDefString BeardyBaldyHaircutCard1;
	CDefString BeardyBaldyHaircutStyle2;
	CDefString BeardyBaldyHaircutCard2;
	CDefString BeardyBaldyBeardStyle1;
	CDefString BeardyBaldyBeardCard1;
	CDefString BeardyBaldyBeardStyle2;
	CDefString BeardyBaldyBeardCard2;
	CDefString BeardyBaldyTashStyle1;
	CDefString BeardyBaldyTashCard1;
	CDefString BeardyBaldyTashStyle2;
	CDefString BeardyBaldyTashCard2;
	CDefString BeardyBaldyRewardObjectFullComplete;
	CDefString BeardyBaldyRewardObjectHalfComplete;
	std::int32_t BeardyBaldyExperience;
	std::int32_t BeardyBaldyRenown;
	float BeardyBaldyMorality;
	std::int32_t BeardyBaldyRandomSpeechTimerMax;
	std::int32_t BeardyBaldyRandomSpeechTimerVar;
	std::int32_t HenchmanHealthLow;
	std::int32_t HenchmanHealthCritical;
	std::int32_t HenchmanPayTimeDelayInSeconds;
	std::int32_t HenchmanCheapGoldCost;
	std::int32_t HenchmanExpensiveGoldCost;
	std::vector<std::string> BookBoyMarker;
	std::vector<std::string> BookGirlMarker;
	std::vector<CConversation> BookReactions;
	std::vector<std::string> BookDefNames;
	std::int32_t BooksNumberWanted;
	std::int32_t BooksNumberAccepted;
	std::int32_t BooksNumberComment;
	std::int32_t BooksNumberTotal;
	std::int32_t BadBooksForHat;
	std::int32_t GoodBooksForHat;
	float BooksMoralityReward;
	float BordelloHeroTavernPriceMultiplier;
	std::int32_t ChapelActivationAge;
	float ChapelAltarProximity;
	float ChapelAcolyteWanderDistance;
	std::int32_t ChapelAcolytePrayerTimer;
	std::int32_t ChapelAcolytePrayerTimerVar;
	std::vector<float> ChapelDonationLevel;
	std::vector<float> ChapelMoralityRewardFraction;
	std::string ChapelDonationRewardObject;
	float ChapelWeaponBrandishProximity;
	float ChapelDonationLevelCoolThing1;
	float ChapelDonationLevelCoolThing2;
	float ChapelMoralityChangeCoolThing1;
	float ChapelMoralityChangeCoolThing2;
	std::int32_t ChapelDonationRewardAgeMin;
	std::int32_t ChapelDonationRewardAgeVar;
	std::string ChapelDonationRewardTitle;
	float ChapelDonationLevelFiddleSuper;
	float ChapelDonationLevelFiddleVery;
	float ChapelDonationLevelFiddle;
	std::int32_t ChapelExperience;
	std::int32_t ChapelRenown;
	float ChapelMorality;
	std::int32_t DealerAttentionDistance;
	std::int32_t DealerAttentionTimer;
	std::int32_t TraderLeavingDelay;
	std::int32_t TraderAttentionDistance;
	std::int32_t TraderConversationDelay;
	std::int32_t TraderStopWalkingTimer;
	std::int32_t TraderStandStillTimer;
	std::int32_t EasyPickingsReward;
	std::string ExposeMayorEvidenceDef;
	float EMMaxFlashDistFromStables;
	std::int32_t F_FishermanHelpDelay;
	std::int32_t F_FishermanIdleChatDelay;
	CDefString F_WaspType;
	float FishermanSaveHimMorality;
	std::int32_t FishTypesInGame;
	std::int32_t FishingRodsInGame;
	std::int32_t FishingPrizes;
	std::vector<std::string> FishPrizeDefNames;
	std::vector<std::string> FishRodDefNames;
	std::vector<std::string> FishDefNames;
	std::vector<float> FishPrizeWeights;
	std::int32_t FC_FighterDelay;
	std::int32_t FC_MinimumFighterHealth;
	float FC_OutsideRingRange;
	float FC_RingTimeOut;
	std::int32_t FC_BowerstoneAllowedHits;
	std::int32_t FC_OakvaleAllowedHits;
	std::int32_t FC_BanditCampAllowedHits;
	std::int32_t FC_KnotholeGladeAllowedHits;
	std::int32_t FC_HitsDepletionOverSession;
	std::int32_t FC_BowerstoneFightCount;
	std::int32_t FC_OakvaleFightCount;
	std::int32_t FC_BanditCampFightCount;
	std::int32_t FC_KnotholeGladeFightCount;
	std::int32_t FC_StartTimeOfDay;
	std::int32_t FC_EndTimeOfDay;
	std::int32_t FC_MoneyAddedPerFight;
	float FC_RingRadius;
	std::int32_t FC_InsideRingMessageDelay;
	std::int32_t FC_AttackWarnings;
	CDefString FC_BowerstoneRewardItem;
	CDefString FC_OakvaleRewardItem;
	CDefString FC_BanditCampRewardItem;
	CDefString FC_KnotholeGladeRewardItem;
	float FC_DistanceAwayToTriggerFC;
	std::int32_t FC_MinimumFee;
	float FC_ShoutOutDistance;
	std::int32_t FC_LostRenownLoss;
	std::int32_t FC_WinRenownGain;
	std::int32_t FC_WinExperienceGain;
	std::int32_t GGN_GhostSpawnNearby;
	std::int32_t GGN_GhostSpawnDistance;
	std::int32_t GGN_ReturnNecklaceReward;
	std::int32_t GGN_GhostClueTime;
	std::int32_t GGN_GhostTalkDistance;
	float GGN_ReturnNecklaceMoralityGain;
	std::int32_t GGN_ReturnNecklaceExperienceGain;
	float HH_UpperBarLimit;
	float HH_BarDecrementAmount;
	float HH_BarDecrementInterval;
	std::int32_t HH_ClearedHHRenown;
	float HeroDuelDefeatThunderMorality;
	std::int32_t HeroDuelDefeatThunderXP;
	CDefString HiddenBootyTreasureFake;
	CDefString HiddenBootyTreasureReal;
	std::int32_t HiddenBootyExperience;
	std::int32_t HiddenBootyRenownReal;
	std::int32_t HiddenBootyRenownFake;
	float HiddenBootyMorality;
	std::int32_t TrophiesOpinionDeedTimer;
	CDefString TrophiesBriarRoseTrophyLeft;
	CDefString TrophiesBriarRoseTrophyRight;
	float TrophiesBriarRoseProximityNear;
	float TrophiesBriarRoseProximityFar;
	float TrophiesStandStillTimer;
	float TrophiesFacingWatcherTimer;
	float TrophiesBriarRoseWalkingSpeed;
	float TrophiesCrowdWanderDistanceMin;
	float TrophiesCrowdWanderDistanceMax;
	std::int32_t TrophiesBriarRoseAnimTime;
	std::int32_t TrophiesBriarRoseAnimTimeVar;
	std::int32_t TrophiesInsideTavernTimer;
	std::int32_t TrophiesOutsideTavernTimer;
	std::int32_t TrophiesHitCountForBriarAttack;
	float OuterTraderDistance;
	float MiddleTraderDistance;
	float InnerTraderDistance;
	std::int32_t TraderYellForHelpRandomValue;
	std::int32_t TraderYellAtHeroRandomValue;
	std::int32_t TraderCommentRandomValue;
	float BrotherStartDistance;
	std::int32_t BrotherScreamForHelpRandomValue;
	std::int32_t BrotherWaitCommentRandomValue;
	float HeroEvilMoralityValue;
	float HeroGoodMoralityValue;
	float HeroKillsAssassinMorality;
	float HeroKillsSheriffMorality;
	float HeroTakesAssassinGoldMorality;
	float HeroTakesAssassinGoldMoralityKillsAssassin;
	std::int32_t HeroKillsAssassinRenown;
	CDefString HeroKillsAssassinProofOfKill;
	std::int32_t HeroKillsSheriffRenown;
	float AssassinThreatDistSq;
	std::int32_t RandomPopulationTraderGoldMinimum;
	std::int32_t RandomPopulationTraderGoldVariation;
	float RockTrollTriggerProximity;
	CDefString RockTrollReward1;
	CDefString RockTrollReward2;
	CDefString IngredientObject;
	CDefString ObjectToGiveToMansLover;
	CDefString ObjectFromMansLover;
	CDefString EndPotion;
	CDefString SickChildSleepingAnim;
	std::vector<CDefString> SickChildsSisterAttentionAnims;
	float TellingTheTruth;
	float LyingToWomanMorality;
	float LyingToManAboutWomanNotLikingHer;
	float LyingToManAboutWomanLikingHer;
	float KilledSomeoneForMushroom;
	float StoleMushroomInBarrowFields;
	float BoughtMushroomInBarrowFields;
	float MadeWomanLaugh;
	float SingingStonesDoorManWanderMax;
	float SwordInTheStoneWanderDistanceMin;
	float SwordInTheStoneWanderDistanceMax;
	std::int32_t SwordInTheStoneSuccessExperience;
	std::int32_t SwordInTheStoneSuccessRenown;
	float SwordInTheStoneSuccessMorality;
	std::int32_t SwordInTheStonePhysiqueChange;
	std::int32_t SwordInTheStoneToughnessChange;
	std::int32_t SwordInTheStoneHealthChange;
	std::int32_t BardSingingFee;
	std::int32_t BardNumVersesPerVisit;
	float BardConsiderEvil;
	float BardConsiderGood;
	std::int32_t BardConsiderRich;
	std::int32_t BardConsiderBoaster;
	std::int32_t BardConsiderFailure;
	std::int32_t BardSpecial;
	std::int32_t BardGoAwayCost;
	float BardWanderDistanceMin;
	float BardWanderDistanceMax;
	std::int32_t BardWanderResetTime;
	float BardFollowDistance;
	std::int32_t BardRandomSpeechTimerMax;
	std::int32_t BardRandomSpeechTimerVar;
	std::int32_t BardHeroAgeHigh;
	std::int32_t BardHeroAgeMedium;
	float BardHeroHealthHigh;
	float BardHeroHealthLow;
	float BardHeroWillHigh;
	float BardHeroWillLow;
	std::int32_t TempleActivationAge;
	float TempleAltarProximity;
	float TempleAcolyteWanderDistance;
	std::int32_t TempleAcolytePrayerTimer;
	std::int32_t TempleAcolytePrayerTimerVar;
	std::vector<float> TempleDonationLevel;
	std::vector<float> TempleMoralityRewardFraction;
	std::string TempleDonationRewardObject;
	float TempleDonationLevelCoolThing1;
	float TempleDonationLevelCoolThing2;
	float TempleMoralityChangeCoolThing1;
	float TempleMoralityChangeCoolThing2;
	std::int32_t TempleDonationRewardAgeMin;
	std::int32_t TempleDonationRewardAgeVar;
	std::string TempleDonationRewardTitle;
	float TempleDonationLevelFiddleSuper;
	float TempleDonationLevelFiddleVery;
	float TempleDonationLevelFiddle;
	std::int32_t TempleExperience;
	std::int32_t TempleRenown;
	float TempleMorality;
	float TemplePeasantWalkSpeed;
	std::int32_t TemplePrayerCounterActivation;
	std::int32_t TempleDonationBoxRequirement;
	float TempleWeaponBrandishProximity;
	float TempleTeleportEnableProximity;
	std::int32_t TemplePrayerFactorLowest;
	std::int32_t TemplePrayerFactorLow;
	std::int32_t TemplePrayerFactorMiddle;
	std::int32_t TemplePrayerFactorHigh;
	std::int32_t TemplePrayerFactorHighest;
	std::int32_t TemplePrayerFactorHighest2;
	std::vector<std::int32_t> TemplePrayerFactorBurnChance;
	std::vector<float> TemplePrayerWorshipRewardHealth;
	std::vector<float> TemplePrayerWorshipRewardMorality;
	std::vector<std::int32_t> TemplePrayerWorshipRewardGold;
	std::vector<float> TemplePrayerWorshipRewardDonationModifier;
	std::vector<float> TemplePrayerWorshipBurnHealth;
	std::vector<std::int32_t> TemplePrayerWorshipBurnGold;
	float TourGuideFollowerDistance;
	float TourGuideOverhearDistance;
	float TourGuideTalkDistance;
	std::int32_t TourGuideStandStillTimer;
	std::int32_t TourGuideTalkToHeroTimer;
	std::int32_t TourGuideFollowerStandStillTimer;
	std::int32_t TourGuideOpeningTime;
	std::int32_t TourGuideClosingTime;
	std::int32_t SlumsBeggarGoldAmount;
	std::int32_t OakValeBeggarGoldAmount;
	std::int32_t PickUpLitterTimeInSeconds;
	std::int32_t NumPiecesOfLitter;
	float TBRMoralityGoodSmall;
	float TBRMoralityGoodMedium;
	float TBRMoralityGoodLarge;
	float TBYMoralityBadSmall;
	float TBYMoralityBadMedium;
	float TBYMoralityBadLarge;
	std::int32_t OakShopKeepGoldAmount;
	std::int32_t ArsonGoldAmount;
	std::int32_t KGExtortAmount;
	CDefString BULLY_WEAPON;
	std::int32_t TH_ThunderConv_Length;
	std::vector<std::string> TH_ThunderConv_Speaker;
	std::vector<std::string> TH_ThunderConv_Dialogue;
	std::vector<std::string> TH_ThunderConv_Actor;
	std::vector<std::string> TH_ThunderConv_Animation;
	std::vector<float> TH_ThunderConv_AnimDelay;
	std::int32_t EscortTraderRewardDownpayment;
	std::int32_t EscortTraderAttackComplaintTime;
	std::vector<CDefString> EscortTraderRewardObjectPerfect;
	float EscortTraderRegionExitWait;
	float EscortTraderRegionExitStop;
	float EscortTraderRegionExitDist;
	std::int32_t EscortTraderHitCountHigh;
	std::int32_t EscortTraderHitCountMedium;
	float EscortTraderHealthHigh;
	float EscortTraderHealthMedium;
	float EscortTraderFollowProximity1;
	float EscortTraderFollowProximity2;
	std::int32_t EscortTraderProximityTimer;
	std::int32_t EscortTraderExpressionDetectTimer;
	std::int32_t EscortTraderHitDetectTimer;
	std::vector<std::string> EscortTraderSpawnedBanditType;
	std::int32_t EscortTraderExperience;
	std::int32_t EscortTraderRenown;
	float EscortTraderMorality;
	std::int32_t EscortTraderFailureRenown;
	std::int32_t TraderRandomSpeechTimerMax;
	std::int32_t TraderRandomSpeechTimerVar;
	float EscortTraderWanderDistance;
	std::int32_t EscortTraderQuestDayInterval;
	std::int32_t EscortTraderQuestStartTime;
	std::int32_t AmbushTradersBanditHireCost;
	float AmbushTradersConvoyExitProximity;
	float AmbushTradersPhase1GuardWanderDistance;
	float AmbushTradersContactWanderDistance;
	float AmbushTradersConvoyTriggerProximity;
	float AmbushTradersSentryWanderDistance;
	float AmbushTradersSpySentryProximity;
	float AmbushTradersSpyHeroProximity;
	float AmbushTradersQuestSuccessReward;
	CDefString AmbushTradersQuestSuccessBonus;
	float AmbushTradersBanditWanderMax;
	std::int32_t AmbushTradersExperience;
	std::int32_t AmbushTradersRenown;
	float AmbushTradersMorality;
	std::int32_t AmbushTradersBoastCostKillAllGuards;
	std::int32_t AmbushTradersBoastRewardKillAllGuards;
	std::int32_t AmbushTradersBoastCostKillAllTraders;
	std::int32_t AmbushTradersBoastRewardKillAllTraders;
	std::int32_t AmbushTradersBoastCostKillNone;
	std::int32_t AmbushTradersBoastRewardKillNone;
	std::int32_t AmbushTradersBoastCostUnassisted;
	std::int32_t AmbushTradersBoastRewardUnassisted;
	float AmbushTradersBanditFollowDistance;
	float AmbushTradersPhase2GuardWanderDistance;
	float AmbushTradersSpeechBanditProximity;
	float AmbushTradersSpeechSpyProximity;
	std::int32_t AmbushTradersBanditSpeechTimerLong;
	std::int32_t AmbushTradersBanditSpeechTimerShort;
	std::int32_t AmbushTradersSpySpeechTimerProximity;
	std::int32_t AmbushTradersSpySpeechTimerLong;
	std::int32_t AmbushTradersSpySpeechTimerShort;
	std::int32_t AmbushTradersSpySpeechTimerSentry;
	std::int32_t AmbushTradersInvalidRegionTimerMax;
	std::int32_t AmbushTradersInvalidRegionTimerVar;
	std::int32_t ArenaCrowdShoutDelay;
	std::int32_t ArenaCrowdShoutDelayRandom;
	std::vector<CDefString> RewardObjects;
	std::int32_t ArenaCountdownDuration;
	std::string ArenaCreatureRound1;
	std::int32_t ArenaNumberOrCreaturesRound1;
	std::int32_t ArenaNumberOfWavesRound1;
	std::string ArenaCreatureRound2;
	std::int32_t ArenaNumberOrCreaturesRound2;
	std::int32_t ArenaNumberOfWavesRound2;
	std::string ArenaCreatureRound3;
	std::int32_t ArenaNumberOrCreaturesRound3;
	std::int32_t ArenaNumberOfWavesRound3;
	std::string ArenaCreatureRound4;
	std::int32_t ArenaNumberOrCreaturesRound4;
	std::int32_t ArenaNumberOfWavesRound4;
	std::string ArenaCreatureRound5;
	std::int32_t ArenaNumberOrCreaturesRound5;
	std::int32_t ArenaNumberOfWavesRound5;
	std::string ArenaCreatureRound6;
	std::int32_t ArenaNumberOrCreaturesRound6;
	std::int32_t ArenaNumberOfWavesRound6;
	std::string ArenaCreatureRound7;
	std::int32_t ArenaNumberOrCreaturesRound7;
	std::int32_t ArenaNumberOfWavesRound7;
	std::string ArenaCreatureRound8;
	std::int32_t ArenaNumberOrCreaturesRound8;
	std::int32_t ArenaNumberOfWavesRound8;
	std::int32_t ArenaShowNoMercyBoastAmount;
	std::int32_t ArenaShowMercyBoastAmount;
	std::int32_t ArenaShowNoMercyBoastReward;
	std::int32_t ArenaShowMercyBoastReward;
	float ArenaSpareWhisperMorality;
	float ArenaKillWhisperMorality;
	std::vector<CDefString> ATO_DiggingRewards;
	std::int32_t ATO_MaxSpawnedCreatures;
	std::int32_t BountyHuntGoldReward;
	float BountyHuntVillagerIntroProximity;
	float BountyHuntVillagerWanderDistanceMin;
	float BountyHuntVillagerWanderDistanceMax;
	float BountyHuntVillagerHeroProximity;
	float BountyHuntVillagerHeroStartConversation;
	float BountyHuntCutsceneTriggerProximity;
	float BountyHuntBanditGruntWanderDistance;
	float BountyHuntBanditLeaderWanderDistance;
	float BountyHuntHostage2WanderDistance;
	float BountyHuntBanditDeputy1WanderDistance;
	float BountyHuntBanditDeputy1AttackProximity;
	float BountyHuntBanditDeputy2WanderDistance;
	float BountyHuntBanditDeputy2AttackProximity;
	float BountyHuntLeaderHealthDiffStage1;
	float BountyHuntLeaderHealthDiffStage2;
	float BountyHuntLeaderHealthDiffStage3;
	float BountyHuntLeaderHealthDiffStage4;
	std::int32_t BountyHuntExperience;
	std::int32_t BountyHuntRenownHostage1;
	std::int32_t BountyHuntRenownHostage2;
	float BountyHuntMorality;
	float BountyHuntCompletionMorality;
	std::int32_t BountyHuntBoastTimeLimit;
	std::int32_t BountyHuntBoastCostTimeLimit;
	std::int32_t BountyHuntBoastRewardTimeLimit;
	std::int32_t BountyHuntBoastCostDecapitation;
	std::int32_t BountyHuntBoastRewardDecapitation;
	std::int32_t BountyHuntRansomGold;
	CDefString BountyHuntRewardObject;
	float BowerstoneIntroGuardProximity;
	float BowerstoneIntroPoshGuardProximity;
	float BowerstoneIntroCrowdWanderDistanceMin;
	float BowerstoneIntroCrowdWanderDistanceMax;
	float BS_BanditAwarenessRange;
	std::int32_t BS_RewardGold;
	std::int32_t BS_NoAssistanceBoastAmount;
	std::int32_t BS_KilledLeaderBoastAmount;
	std::int32_t BS_NoAssistanceBoastReward;
	std::int32_t BS_KilledLeaderBoastReward;
	std::int32_t BS_NoOfWavesInFirstAttack;
	std::int32_t BS_NoOfBanditsInWave;
	CDefString BS_BanditTypeInFirstAttack;
	CDefString BS_EnvTheme;
	float DBDragonMediumHealth;
	float DBDragonLowHealth;
	float DBDragonVeryLowHealth;
	std::int32_t DBNumMinionsHighHealth;
	std::int32_t DBNumMinionsMediumHealth;
	std::int32_t DBNumMinionsLowHealth;
	std::int32_t DBNumMinionsVeryLowHealth;
	std::int32_t DBNumSummonersHighHealth;
	std::int32_t DBNumSummonersMediumHealth;
	std::int32_t DBNumSummonersLowHealth;
	std::int32_t DBNumSummonersVeryLowHealth;
	std::int32_t DBNumFlyBysBetweenSummonerSpawns;
	float EndGame_UpperBarLimit;
	float EndGame_BarDecrementAmount;
	float EndGame_BarDecrementInterval;
	CDefString EndGame_TeleportEffect;
	std::int32_t EndGame_TeleportHeroDistance;
	float EndGame_Stage1FX;
	float EndGame_Stage2FX;
	float EndGame_Stage3FX;
	std::string FHSunPadFX;
	std::string FHMoonPadFX;
	float FHPadFXHeight;
	float FHPadRadius;
	std::int32_t FHNumberOfPads;
	std::int32_t FHNumberOfProphets;
	std::int32_t FHNumberOfPatterns;
	std::int32_t FHNumberOfRounds;
	std::int32_t FHNumberOfSunsEvil;
	std::int32_t FHNumberOfSunsGood;
	std::vector<CFireHeartPatternDef> FHPatterns;
	std::vector<std::int32_t> FHTimeLimit;
	std::int32_t FHWarnTime;
	std::int32_t FHNumberOfVillagers;
	float FHHealthChangeWhenZapped;
	float FHMoralityChangeOnKilled;
	float FHMoralityChangeOnFreed;
	float FHFreeAllPrisonersMoralityChange;
	std::int32_t FHFreeAllPrisonersRenownBoost;
	float FHKillAllPrisonersMoralityChange;
	std::int32_t FHKillAllPrisonersRenownBoost;
	std::int32_t GSI_MazeCallsOverDistance;
	std::int32_t GSI_MazeRegionExitDistance;
	float HS_ScreamerCount;
	std::int32_t HS_ScreamersAtOnce;
	float HS_ScreamerInc;
	std::int32_t HS_SecondsBeforeSuperScreamerDamage;
	float HS_SuperScreamerDamagePerSecond;
	std::int32_t HS_SecondsBeforeNewSuperScreamer;
	std::int32_t HS_TimePerUndeadKilled;
	std::int32_t HS_MaxTimeNostroExposed;
	std::int32_t HS_GMShortDistance;
	std::int32_t HS_GMLongDistance;
	std::int32_t HS_GMGuardDistance;
	float HS_GoodSoulTaken;
	std::int32_t HC_SpawnCountPostReleaseGSon;
	std::int32_t HC_ReduceCountPostSideRoom;
	float HC_SpawnGenerationRadius;
	std::int32_t HC_BanditTimeBeforeComment;
	std::int32_t HC_BanditCallsForHelpDelay;
	std::int32_t HC_BanditGoldDemand;
	float HC_BanditFadeOut;
	std::int32_t HC_HobbeSpawnDelay;
	std::int32_t HC_BanditCommentTime;
	std::int32_t HC_BanditCloseToMarkerDistance;
	std::int32_t HC_HobbeGruntDelay;
	std::int32_t HC_HobbeSpawnCount;
	float HC_RescueBanditMoralityGain;
	std::int32_t HC_RescueBanditExperienceGain;
	std::int32_t HC_SacrificeToNymphExperienceGain;
	float HC_SacrificeBanditMoralityLoss;
	float HC_SacrificeToNymphMoralityLoss;
	float HC_LetBanditGoAndPayMoralityGain;
	std::int32_t HC_LetBanditGoRenown;
	float HC_DontPayBanditMoralityLoss;
	std::int32_t HC_KillNymphBoastCost;
	std::int32_t HC_KillNymphBoastReward;
	std::int32_t HC_SacrificeBoastCost;
	std::int32_t HC_SacrificeBoastReward;
	std::int32_t HC_BoyNoHarmBoastCost;
	std::int32_t HC_BoyNoHarmBoastReward;
	std::int32_t HC_HobbeMaxNumberInRegion;
	std::int32_t JackFightDefeatJackXP;
	std::int32_t MB_MadBomberStartDistance;
	std::int32_t MB_MadBomberProximityToBomb;
	std::int32_t MB_MadBomberHeroCloseDelay;
	std::int32_t MB_MadBomberRunToWalkDistance;
	std::int32_t MB_BlacksmithCallHeroDistance;
	std::int32_t MB_BombInitialTimeToBoom;
	std::int32_t MB_FailMissionBombCount;
	std::int32_t MB_BombGoesOffRenownLoss;
	std::int32_t MB_BombDefusedExperience;
	float MB_BombDefusedMorality;
	std::int32_t MCNumHitsBetweenForcePushes;
	float MCMinMinionSpawnPeriod;
	float MCMaxMinionSpawnPeriod;
	std::int32_t MCC_GuardTalkingDistance;
	std::int32_t MCC_MinionRunDistance;
	float MCC_MinionBreakoffDistance;
	float MCC_SavedGuardMoralityGain;
	std::int32_t MCC_Timer;
	std::int32_t MCC_MinionCounter;
	float OVBRBanditSightRadius;
	float OVBRBanditSoundRadius;
	float OVI_MoralityChangePerDeed;
	std::int32_t ExperienceForDefeatingWhisperInOrchardFarm;
	std::int32_t GuardReinforcementsYourTeam;
	std::int32_t GuardReinforcementsEnemyTeam;
	std::int32_t BanditReinforcementDelay;
	std::int32_t OFE_NoHealthPotionBoastCost;
	std::int32_t OFE_NoHealthPotionBoastReward;
	std::int32_t OFE_NoBanditsDieBoastCost;
	std::int32_t OFE_NoBanditsDieBoastReward;
	std::int32_t OFG_NoCratesStolenBoastCost;
	std::int32_t OFG_NoCratesStolenBoastReward;
	std::int32_t OFG_NoGuardsDieBoastCost;
	std::int32_t OFG_NoGuardsDieBoastReward;
	float OFEvilCompletionMorality;
	float OFGoodCompletionMorality;
	std::int32_t PrisonExerciseStartingTimeLimit;
	std::int32_t PrisonWayPointHitRange;
	std::int32_t PrisonKeyRackDangerRange;
	std::int32_t PrisonClothesChestDangerRange;
	std::int32_t PrisonGuardProximityToAskQuestion;
	std::int32_t PrisonTrainingPartnerHintDistance;
	std::int32_t PrisonTrainingPartnerHintDistance2;
	std::int32_t PrisonDistanceForGuardsToJeer;
	std::int32_t PrisonRadiusGuardWillAttackHeroWithin;
	std::int32_t PrisonGuardMumbleAudioDistance;
	float PrisonRacerRunSpeed;
	float PrisonRacerSprintSpeed;
	std::int32_t PrisonRacerSprintMaxTimeSeconds;
	std::int32_t PrisonRacerSprintRestTimeSeconds;
	float PrisonWardenGameSneakModifier;
	std::int32_t PrisonPoetryMaxLines;
	std::int32_t PrisonExperienceForKillingKraken;
	std::int32_t PrisonEscapeExperienceFromMother;
	float PrisonFreePrisonerMorality;
	std::int32_t MotherFollowingHeroKrakenSafetyDistance;
	std::int32_t TE_TraderCommentDelay;
	std::int32_t TE_TraderToBalverineDelay;
	std::int32_t TE_TraderLowInfectionTime;
	std::int32_t TE_TraderMediumInfectionTime;
	std::int32_t TE_TraderHighInfectionTime;
	std::int32_t TE_AllTradersAliveBoastCost;
	std::int32_t TE_AllTradersAliveBoastReward;
	std::int32_t TE_BalverineTimeToGoDistance;
	std::int32_t TE_TraderCommentDistance;
	std::int32_t TE_AssassinSpawnDistance;
	CDefString TE_AssassinName;
	std::int32_t TE_EarthTrollTriggerDistance;
	CDefString TE_TrollName;
	std::int32_t TE_TraderGoldReward;
	float TE_LetTraderFollowMorality;
	float TraderEscortLeaveTraderMorality;
	float WB_MoralityGain;
	std::int32_t WB_LowHealth;
	std::int32_t WB_GoldReward;
	CDefString WB_CreaturesSpawned;
	CDefString WB_WaspBossName;
	std::int32_t WB_ScreamingVillagerDistance;
	std::int32_t WB_ScreamingVillagerScreamsDistance;
	float WB_ScreamingVillagerFadeOutTime;
	std::int32_t WB_WaspHelperCallsOutDistance;
	std::int32_t WB_WaspChaseWomanPanicTime;
	std::int32_t WB_ExperienceReward;
	float BC_EvilMoralityLoss;
	float BC_GoodMoralityGain;
	float BAC_RenownAward;
	float BAC_HostageMoralityGain;
	float BAC_SingleHostageMoralityLoss;
	float BAC_DoubleHostageMoralityLoss;
	float BAC_AgeAmount;
	float BAC_EnemiesToKillInFirstArea;
	float BAC_EnemiesToKillInSecondArea;
	float BAC_AssassinGoldAmount;
	float BAC_ForgerGoldAmount;
	float BAC_BoastBanditKill;
	float BAC_TheresaExperienceGift;
	float BAC_TwinBladeKilledMorality;
	float BAC_TwinBladeSavedMorality;
	float ENF_TotalTime;
	float ENF_TimeAddition;
	float ENF_TimeSubtraction;
	float EGJ_MoralityLoss;
	float EGJ_MoralityGain;
	std::int32_t OGSP_KillAllBoastCost;
	std::int32_t OGSP_KillAllBoastReward;
	std::vector<float> GUI_MeleeGrades;
	std::vector<float> GUI_SkillGrades;
	std::vector<float> GUI_WillGrades;
	float GUI_MinHealth;
	float GUI_RaceTime;
	float GUI_RaceGold;
	float GUI_RearDummySegements;
	float GUI_MiddleDummySegements;
	float GUI_FrontDummySegements;
	float GUI_RearDummyWorth;
	float GUI_MiddleDummyWorth;
	float GUI_FrontDummyWorth;
	float GUI_GoldPerBird;
	float GUI_BirdGoldBonus;
	float GUI_SkillTimer;
	float GUI_WillTimer;
	float GUI_DepartureBeetles;
	float GUI_MeleeBeetles;
	float GUI_LampCost;
	float GUI_WillDummySpinTimer;
	float GUI_EndGuildXP;
	float HOB_TotalTime;
	float HOB_BoastHobbeMany;
	float HOB_RenownAwardSuccess;
	float HOB_RenownAwardDraw;
	float HOB_RenownAwardLost;
	std::int32_t HOB_KillMoreHobbesBoastCost;
	std::int32_t HOB_KillMoreHobbesBoastReward;
	std::int32_t HangingTreeProtectGuardsBoastReward;
	std::int32_t HangingTreeProtectGuardsBoastAmount;
	float HTGoodCompletionMorality;
	std::int32_t HTGoodHeroKilledPrisonerGoldReward;
	std::int32_t HTGoodHeroKilledPrisonerRenownReward;
	std::int32_t HangingTreeProtectBanditsBoastReward;
	std::int32_t HangingTreeProtectBanditsBoastAmount;
	std::int32_t HangingTreeEvilMissionTimer;
	float HTEvilCompletionMorality;
	float HOH_MoralityGain;
	float HOH_MoralityLoss;
	std::int32_t RV_AcceptedGuardLoweredGoldReward;
	std::int32_t RV_AcceptedGuardLoweredRenownReward;
	std::int32_t RV_LowerGoldRewardFromKillingChiefsGuard;
	float RV_HeroEvilMoralityValue;
	float RV_HeroGoodMoralityValue;
	float RV_HeroTellMoralityValue;
	std::int32_t TCE_TimeLimit;
	std::int32_t TCG_TimeLimit;
	std::int32_t TCE_GuardRangeHighest;
	std::int32_t TCE_GuardRangeHigh;
	std::int32_t TCE_GuardRangeLow;
	float WBK_NumberHits;
	float WBK_RenownAwardForGates;
	float WBK_RenownAwardForFirstDefence;
	float WBK_RenownAwardForSecondDefence;
	std::int32_t WizardBattleDefeatMazeXP;
	float CHK_LowPrize;
	float CHK_MidPrize;
	float CHK_HighPrize;
	std::int32_t TeleporterActivationDist;
	float TimeAdvancePointFadeOutTime;
	float TimeAdvancePointFadeInTime;
	float TimeAdvancePointFadedOutPauseTime;
	float TimeAdvancePointTimeToAdvance;
	std::int32_t CoreQuestReminderIntervalSeconds;
	std::int32_t FlourishReminderFrequency;
	float PercentageFlourishesTakenLowerBound;
	std::int32_t StopTeachingFlourishesAfterDoneThisMany;
	std::int32_t CombatMultTooLongUnchanged;
	std::int32_t CombatMultStopBotheringAfter;
	std::int32_t LowHealthWarning;
	std::int32_t VeryLowHealthWarning;
	std::int32_t LowHealthMessageInterval;
	std::int32_t VeryLowHealthMessageInterval;
	std::int32_t HealthMessageLimit;
	float LowWillEnergyWarningLevel;
	std::int32_t LowWillEnergyMessageInterval;
	std::int32_t WillEnergyMessageLimit;
	std::int32_t SaveMessageLimit;
	std::int32_t SaveMessageInterval;
	float FishingMaxDistanceFromSpot;
	float DiggingMaxDistanceFromSpot;
	float PickpocketDurationSeconds;
	float PickpocketSpottedDurationSeconds;
	std::int32_t PickpocketSpottedChancePerSecond;
	float PicklockDurationSeconds;
	float PicklockSpottedDurationSeconds;
	std::int32_t PicklockSpottedChancePerSecond;
	float StealDurationSeconds;
	float StealSpottedDurationSeconds;
	std::int32_t StealSpottedChancePerSecond;
	std::int32_t NumberOfConcurrentlyAvailableFeats;
	std::vector<CFeatDef> Feats;
	std::vector<CArenaRoundDef> ArenaRounds;
	std::vector<CHeroSoulsRoundDef> HeroSoulsRounds;
	std::int32_t DemonDoor_CombatMult_TriggerLevel;
	float DemonDoor_HighRenown_TriggerLevel;
	float DemonDoor_ShineLight_TriggerDistance;
	std::int32_t DemonDoor_SummonCreature_NumCreatureTypes;
	std::vector<std::string> DemonDoor_SummonCreature_CreatureType;
	std::int32_t DemonDoor_BeEvil_TriggerDistance;
	float DemonDoor_BeEvil_EvilnessNeeded;
	float DemonDoor_DoDamage_DamageLimit;
	float DemonDoor_EatPies_FatnessLevel;
};

class CShipDef
{
public:
	std::int32_t SightingFrequency;
	std::int32_t SightingOffset;
	std::int32_t DaysVisible;
	float Swell;
	float Pitch;
	float Roll;
	float WaveLength;
};

class CShopItem
{
public:
	float Quantity;
	float PriceMultiplier;
	float MaxStock;
	std::int32_t RestockPeriod;
	float RestockPeriodRandomness;
	float SalesPerDay;
	float SalesPerDayRandomness;
};

class CShopItemDef
{
public:
	std::int32_t Type;
	CShopItem Item;
};

class CShopDef
{
public:
	std::uint32_t Name;
	std::uint32_t InitialHawkingPhrase;
	std::uint32_t ReplyToRequestPhrase;
	std::uint32_t CustomerFollowPhrase;
	std::uint32_t ThankingPhrase;
	std::string ShopkeeperMeshType;
	std::vector<CShopItemDef> DefaultStock;
	std::vector<CShopItemDef> StockCategoryDefaults;
	float BuyPriceMultiplier;
	float SellPriceMultiplier;
	float AttitudeBuyPriceMultiplierDefault;
	std::map<EOpinionAttitudeType,float> AttitudeBuyPriceMultiplier;
	float AttitudeSellPriceMultiplierDefault;
	std::map<EOpinionAttitudeType,float> AttitudeSellPriceMultiplier;
	float MaxStockChangePerSell;
	float MaxStockChangePerBuy;
	float MaxStockRevertPerDay;
	float PriceReductionFractionPerItem;
	float MaxPriceReductionFractionDueToStockCount;
	std::uint32_t PreferredShopkeeper;
	bool IsStockDisplayPermanent;
	bool IsBarberShop;
	bool IsTattooShop;
	bool IsTitleShop;
	bool AlwaysOpen;
	float MinPriceMultiplierForItemToBeWanted;
	float MaxPriceToDisplay;
	float MinTimeToSteal;
	float MaxTimeToSteal;
	std::int32_t MinGoldForDonation;
	std::int32_t MaxGoldForDonation;
	float MaxMoralityForDonation;
	float LogarithmicPower;
	std::int32_t NumDaysToKeepSpecialItem;
	std::int32_t MinAmountBeforeSpecialItem;
	float ProbabilityOfSpecialItemPerDay;
	float SpecialItemPriceMult;
};

class CShoveHaPennyRegionDef
{
public:
	float Start;
	float Score;
};

class CSimBuildingDef
{
public:
	std::int32_t Property;
	std::int32_t WorkSpaces;
	std::int32_t LiveSpaces;
};

class CSimVoice
{
public:
	std::vector<std::pair<EReactionSpeechType,std::int32_t>> Voices;
};

class CSimVoicesDef
{
public:
	CSimVoice Entry;
};

class CSkeletalMorphDef
{
public:
	std::vector<CDefString> SkeletalMorphs;
};

class CSkyDef
{
public:
	std::int32_t SunRadius;
	std::int32_t SunTexture;
	std::int32_t MoonRadius;
	std::int32_t MoonTexture;
	std::int32_t SunFlareRadius;
	std::int32_t SunFlareTexture;
	std::int32_t StarTexture;
	std::int32_t StarChartTextureSize;
	float StarSize;
	std::int32_t StarChartFilter;
	std::int32_t TwinkleInterval;
	float TwinkleSpeed;
	float TwinkleMin;
	float TwinkleMax;
	std::vector<CLensFlareElementDef> FlareElements;
};

class CSmashableDef
{
public:
	bool Smashable;
	CDefIndex ReplacementObject;
	std::int32_t SmashParticleEmitter;
};

class CSmokeGeneratorDef
{
public:
	float GenerationDistance;
	std::int32_t ParticleEmitter;
};

class CSnowTrollDef
{
public:
	std::int32_t IcicleAttackIndex;
	float SecondsIcicleAttackTargetPositionPredictionTime;
	float DistancePoundGroundRange;
	float SecondsForIcicleAssault;
	float SecondsLeadIntoIcicles;
	float SecondsBetweenIcicles;
	float SecondsBetweenWarningFXAndTrigger;
	float SecondsForTrapToRemain;
	float DistanceRange;
	float DistanceAroundTarget;
	float DistanceExclusionAroundIcicles;
	std::int32_t IcicleAssaultWarningFXIndex;
	std::int32_t IcicleAssaultTrapIndex;
	float IceTrailDistanceLimit;
};

class CSoundAtmospheresDef
{
public:
	std::vector<std::string> SoundAtmospheres;
};

class CSoundBankEntry
{
public:
	std::uint32_t ID;
	CDefString Filename;
	CDefString SymbolFilename;
	CDefString LipSyncBankHandle;
	bool DVD;
};

class CSoundDef
{
public:
	std::vector<CSoundBankEntry> SoundBankEntries;
	std::vector<CAtmosBankEntry> AtmosBankEntries;
	std::vector<CMusicEntry> MusicEntries;
	std::vector<CMusicSetEntry> MusicSetEntries;
	std::vector<CReverbEnvironmentEntry> ReverbEnvironmentEntries;
	float AtmosListenerDampingMin;
	float AtmosListenerDampingRange;
	float DistanceModelCurve;
};

class CSoundThemeDef
{
public:
	class CAtmosGroup
	{
	public:
		std::string GroupName;
		std::int32_t GroupID;
	};

	CRGBColour EditColour;
	std::int32_t AtmosIndex;
	std::int32_t MusicSet;
	float Gain;
	float Range;
	float Granularity;
	std::multimap<std::string,std::int32_t> AtmosGroupMap;
	bool PlayOnlyWhenInside;
	bool PlayQuietlyWhenInside;
	bool PlayOnlyWhenOutside;
	bool PlayQuietlyWhenOutside;
};

class CSpecialAbilitiesAssassinRushDef
{
public:
	float DistanceAwayFromtargetToAssassinRushTo;
	float SafeCollideRadiusPercentage;
	float DistFromFloor;
	float AssassinStartSeconds;
	float AssassinEndSeconds;
	float AssassinThreshold;
	float AssassinLength;
	float AssassinIntensity;
	float AssassinFade;
	float AssassinRange;
	float AssassinTwist;
	float RunSpeed;
	float TrailDelaySecs;
	float SlowTimeLength;
	std::int32_t AssassinAlpha;
	std::int32_t SlowTimeMultiplier;
	float RushDelay;
	std::vector<float> DashDistance;
};

class CSpecialAbilitiesBattleChargeDef
{
public:
	float RunSpeed;
	std::vector<float> DashDistance;
	std::vector<float> RadiusOfEffect;
	std::vector<float> BattleChargeDamage;
	float BattleChargeCombatMultiplierFactor;
	float BattleChargeRadialStartSeconds;
	float BattleChargeRadialEndSeconds;
	float BattleChargeRadialThreshold;
	float BattleChargeRadialLength;
	float BattleChargeRadialIntensity;
	float BattleChargeRadialFade;
	float BattleChargeRadialRange;
	float BattleChargeRadialTwist;
	float PoseDelaySecs;
	float ChargeDelay;
	std::vector<std::int32_t> NumPeopleHit;
	float DragonExtraGumphRadiusMultiplier;
	float DragonExtraGumphRadius;
};

class CSpecialAbilitiesBerserkDef
{
public:
	float SpeedMultiplier;
	std::int32_t FramesToFadeOut;
	float LowerBerserkPercent;
	float TimeForThemeTransition;
	float BlurCenterFromFloor;
	float BerserkThreshold;
	float BerserkLength;
	float BerserkIntensity;
	float BerserkFade;
	float BerserkRange;
	float BerserkTwist;
	float TimeForRadialBlurToFadeOut;
	float CameraStartDist;
	float CameraEndDist;
	float CameraStartDeltaHeight;
	float CameraEndDeltaHeight;
	std::vector<float> BerserkDuration;
	std::vector<float> BerserkDamageToEnemyMultiplier;
	float BerserkDamageToEnemyMultiplierAdditionPerHit;
	std::vector<float> BerserkDamageToMeMultiplier;
	std::vector<float> MaxIncreaseInStrength;
	std::vector<float> BerserkScale;
	float BerserkBlastRadius;
	std::vector<float> CamDist;
	std::vector<float> CamHeight;
	std::vector<float> CamFOV;
	std::vector<float> CamSwitchTime;
	std::vector<float> RumbleLevel;
	std::vector<float> RumbleSmoothness;
	std::vector<float> RumbleAttack;
	std::vector<float> RumbleDecay;
	float RumbleReleaseLevelDelta;
	float RumbleReleaseSmoothnessDelta;
	float RumbleReleaseAttackDelta;
	float RumbleReleaseDecayDelta;
	float CrazyActionRadius;
	float CrazyActionDamage;
	std::vector<float> DecapitateMaxProb;
	float TargetNearestEnemyDistance;
	std::int32_t WeaponTrailAttack;
	std::int32_t WeaponTrailKnockDown;
	std::vector<float> LevelUpTimings;
	std::vector<std::int32_t> TintColorR;
	std::vector<std::int32_t> TintColorG;
	std::vector<std::int32_t> TintColorB;
	bool CanBlockWhileBerserked;
	float CrazyIdleIntervalSecs;
};

class CSpecialAbilitiesBulletTimeDef
{
public:
	float SlowTimeDuration[4];
	std::int32_t BulletTimeSlowdownMultiplier[4];
	float LevelUpTimings[3];
	float CamDist[4];
	float CamHeight[4];
	float CamFOV[4];
	float CamSwitchTime[4];
	float CastCamDist;
	float CastCamHeight;
	float CastCamFOV;
	float CastCamSwitchTime;
	CDefString ChargeSound;
	CDefString PowerUp1Sound;
	CDefString PowerUp2Sound;
	CDefString PowerUp3Sound;
	CDefString InitSound;
	CDefString LoopSound;
	CDefString EndSound;
	C3DVector RadialBlurThresholdCol;
	float RadialBlurLength;
	C3DVector RadialBlurIntensityCol;
	float RadialBlurFade;
	float RadialBlurRange;
	float RadialBlurTwist;
	float RadialBlurFadeTimeSecs;
	C3DVector DamageRadialBlurThresholdCol;
	float DamageRadialBlurLength;
	C3DVector DamageRadialBlurIntensityCol;
	float DamageRadialBlurFade;
	float DamageRadialBlurRange;
	float DamageRadialBlurTwist;
	float DamageRadialBlurFadeTimeSecs;
	float HeroCombatSlowness;
	std::vector<float> ScreenFilterAlpha;
	std::vector<float> ScreenFilterSaturation;
	std::vector<float> ScreenFilterContrast;
	std::vector<float> ScreenFilterBrightness;
	std::vector<float> ScreenFilterTintR;
	std::vector<float> ScreenFilterTintG;
	std::vector<float> ScreenFilterTintB;
	float ScreenFilterFadeInSecs;
	float ScreenFilterFadeOutSecs;
};

class CSpecialAbilitiesBurntEffectDef
{
public:
	float FadeInTimeSecs;
	float FadeOutTimeStartSecs;
	float FadeOutTimeSecs;
	std::vector<std::int32_t> Color;
};

class CSpecialAbilitiesCreatureTintDef
{
public:
	float FadeInTimeSecs;
	float FadeOutTimeSecs;
	std::vector<std::int32_t> Color;
};

class CSpecialAbilitiesDivineWrathDef
{
public:
	std::vector<CDefString> StartPowerupSound;
	std::vector<CDefString> LoopingPowerupSound;
	std::vector<CDefString> ReleaseSound;
	std::vector<CDefString> ReleaseLoopSound;
	std::vector<CDefString> FinishSound;
	std::vector<CDefString> CancelSound;
	float RandomFactorForNumSecsBetweenDamageHit;
	std::vector<float> NumSecsBetweenDamageHit;
	std::vector<float> DamagePerHit;
	std::vector<float> DurationOfSpellSecs;
	std::vector<float> RadiusOfEffect;
	std::vector<float> EnvChangeSecs;
	std::vector<float> EnvReturnSecs;
	bool DoLifting;
	float VelocityOfLift;
	float DistanceToLift;
	std::vector<float> StaminaCostPerSec;
	std::vector<float> DamageTakenMultiplier;
	float SmallRingRadius;
	float LargeRingRadius;
};

class CSpecialAbilitiesDrainLifeDataDef
{
public:
	float Damage;
};

class CSpecialAbilitiesDrainLifeDef
{
public:
	std::vector<float> AmountOfHealthSucked;
	std::vector<std::int32_t> NumEffects;
	std::vector<float> SpellRange;
	std::vector<std::string> ShotName;
	float SpellDelaySecs;
};

class CSpecialAbilitiesDrunkennessDef
{
public:
	std::int32_t DrunkenGraphic;
	std::int32_t MaxAlpha;
	float USpeed;
	float VSpeed;
	float RotSpeed;
};

class CSpecialAbilitiesElectrocutedEffectDef
{
public:
	float FadeInTimeSecs;
	float FadeOutTimeStartSecs;
	float FadeOutTimeSecs;
	std::vector<std::int32_t> Color;
	std::vector<std::int32_t> HighlightColor;
	float HighlightWidth;
	float HeightSmall;
	float HeightMedium;
};

class CSpecialAbilitiesEnflameDef
{
public:
	float CamDist[4];
	float CamHeight[4];
	float CamFOV[4];
	float CamSwitchTime[4];
	float LevelUpTimings[3];
	float Damage[4];
	std::vector<float> RadiusOfEffect;
	float EnflameCombatMultiplierFactor;
};

class CSpecialAbilitiesFireballSpellDef
{
public:
	float ReleaseDelay;
	float Inclination;
	std::vector<float> StaminaCost;
	float TargettingArcAngle;
};

class CSpecialAbilitiesForcePushDataDef
{
public:
	float Damage;
};

class CSpecialAbilitiesForcePushDef
{
public:
	float MaxDistanceToPush;
	float MinDistanceToPush;
	float DustBoxWidth;
	std::int32_t MaxNumOfExtraEffects;
	std::int32_t MinNumOfExtraEffects;
	float LevelUpTimings[3];
	float ReleaseLevel1Delay;
	float ReleaseLevel2Delay;
	float ReleaseLevel3DelayA;
	float ReleaseLevel3DelayB;
	float ReleaseLevel4Delay;
	float HeightOffGround;
	float Level4OffsetMagnitude;
	std::vector<float> RadiusOfEffect;
	float ExtraPush;
	float PushSpeed;
	std::vector<float> ObjectDamage;
	std::vector<float> TimeDelayBetweenCastsSec;
};

class CSpecialAbilitiesGhostSwordDef
{
public:
	float TimeOfSummon[4];
	float FadeInTime;
	float FadeOutTime;
	float CreationDistance;
};

class CSpecialAbilitiesHealLifeDef
{
public:
	std::vector<float> HealthIncreasePerFrame;
	std::vector<float> TotalHealthIncrease;
	std::vector<float> RadiusOfEffect;
};

class CSpecialAbilitiesLightningSpellDef
{
public:
	std::vector<float> m_faAreaEffect;
	std::vector<float> m_faDamagePerHit;
	std::vector<float> m_faSubBeamDamagePerHit;
	std::vector<float> m_faActivationTime;
	std::vector<float> m_faDeActivationTime;
	std::vector<std::int32_t> m_naMinMainBeams;
	std::vector<std::int32_t> m_naMaxMainBeams;
	std::vector<std::int32_t> m_naMinSubBeams;
	std::vector<std::int32_t> m_naMaxSubBeams;
	std::vector<float> m_faTemporaryBeamLife;
	std::vector<float> m_faTargettingFOV;
	std::vector<float> m_faStaminaPerSecond;
	float m_fRandomBeamMinAngleX;
	float m_fRandomBeamMaxAngleX;
	float m_fRandomBeamMinAngleZ;
	float m_fRandomBeamMaxAngleZ;
	std::vector<float> m_faMainBeamDecapitationTime;
	std::vector<float> m_faSubBeamDecapitationTime;
	std::vector<float> m_faSubBeamDelay;
	std::vector<float> OnhitEveryNSecs;
	float CombatMultiplierIncreaseTime;
	CDefString InitSound;
	CDefString LoopingSound;
	CDefString EndHeroSound;
	CDefString EndAttackeeSound;
	float LightningThemeTransitionInSecs;
	float LightningThemeTransitionOutSecs;
};

class CSpecialAbilitiesMultiArrowDef
{
public:
	std::int32_t NumberOfShots[4];
	std::int32_t NumberInVolley[4];
	float AmmoEffectCreationDelaySecs;
	CDefString CastSound;
	float BowHighlightWidth;
	std::vector<std::int32_t> BowHighlightColor;
	std::vector<float> MultiArrowPerArrowDamageMultiplier;
};

class CSpecialAbilitiesMultiStrikeDef
{
public:
	CDefString Sounds[4];
	CDefString Animations[4];
	float HighlightWidth;
	std::vector<std::int32_t> HighlightColor;
	std::vector<float> DelayTimeSecs;
	std::vector<std::int32_t> NumHits;
	std::vector<float> PerHitDamagerMultiplier;
	std::vector<std::string> AttachmentPoint;
};

class CSpecialAbilitiesPhysicalShieldDef
{
public:
	CDefString InitSound;
	CDefString PowerUpSound[3];
	CDefString CastSound;
	CDefString LoopingSound[4];
	CDefString EndSound;
	float Health[4];
	float LevelUpTimings[3];
	float CamDist[4];
	float CamHeight[4];
	float CamFOV[4];
	float CamSwitchTime[4];
	float CastCamDist;
	float CastCamHeight;
	float CastCamFOV;
	float CastCamSwitchTime;
	float MinWidth;
	std::vector<float> MaxWidth;
	float BeenHitExtraWidth;
	float BeenHitFadeOffTimeSecs;
	float WobbleRate;
	std::vector<float> ColorStartLevel1;
	std::vector<float> ColorStartLevel2;
	std::vector<float> ColorStartLevel3;
	std::vector<float> ColorStartLevel4;
	std::vector<float> ColorEnd;
	std::vector<float> ColorHit;
	float FadeOutPhysicalShieldTimeSecs;
	std::vector<float> ManaDamage;
	bool ShieldCoversCarriedItems;
};

class CSpecialAbilitiesSummonSpellDef
{
public:
	std::vector<float> TimeOfSummon;
	CDefString BaseSummonCreatureName;
	float FadeInTime;
	float FadeOutTime;
	std::int32_t ResetCreatureManaCost;
	std::vector<std::int32_t> SummonTintColour;
	std::string EyeLeftDummyPointName;
	std::string EyeRightDummyPointName;
};

class CSpecialAbilitiesThunderLightningStormDef
{
public:
	std::int32_t NumBeams;
	float StartRadius;
	float EndRadius;
	float SkyBeamStartTime;
	float MainBeamsStartTime;
	float BeamRotationStartSpeed;
	float BeamRotationEndSpeed;
	float TotalLifetime;
	float Damage;
	CDefString InitSound;
	CDefString LoopingSound;
};

class CSpecialAbilitiesTurncoatSpellDef
{
public:
	std::vector<float> StaminaCostPerSec;
	std::vector<float> ResistanceDamagePerSec;
	std::vector<float> TurncoatTime;
	std::vector<float> TurncoatRange;
	std::vector<float> TurncoatXPUpEveryNSecs;
};

class CSpecialAbilitiesUnholyPowerDef
{
public:
	std::vector<CDefString> StartPowerupSound;
	std::vector<CDefString> LoopingPowerupSound;
	std::vector<CDefString> ReleaseSound;
	std::vector<CDefString> ReleaseLoopSound;
	std::vector<CDefString> FinishSound;
	std::vector<CDefString> CancelSound;
	float RandomFactorForNumSecsBetweenDamageHit;
	std::vector<float> NumSecsBetweenDamageHit;
	std::vector<float> DamagePerHit;
	std::vector<float> DurationOfSpellSecs;
	std::vector<float> RadiusOfEffect;
	std::vector<float> EnvChangeSecs;
	std::vector<float> EnvReturnSecs;
	bool DoLifting;
	float VelocityOfLift;
	float DistanceToLift;
	std::vector<float> StaminaCostPerSec;
	std::vector<float> DamageTakenMultiplier;
	float SmallRingRadius;
	float LargeRingRadius;
};

class CSpecialEffectsStringMap
{
public:
	std::map<std::uint32_t,std::int32_t> ContainmentMap;
};

class CSpecialEffectsDef
{
public:
	CSpecialEffectsStringMap SpecialEffectsMap;
};

class CSpotLightDef
{
public:
	CRGBColour Colour;
	float InnerRadius;
	float OuterRadius;
	float Width;
	float Flicker;
	float FlickerSpeed;
	float Angle;
};

class CStealthDef
{
public:
	float DistanceMovementCanBeHeardFrom;
	float RunningNoiseMultiplier;
	float JoggingNoiseMultiplier;
	float WalkingNoiseMultiplier;
	float DayTimeLampVisibilityMultiplier;
	float NightTimeLampVisibilityMultiplier;
	float DangerFeedbackHangtimeSeconds;
	float RunningAwayFromCombatHangtimeSeconds;
};

class CStockItemDef
{
public:
	std::int32_t DefaultPrice;
	bool DefaultIsStealable;
	bool CanOnlyBeSoldToPlayer;
	std::uint32_t SellersBanter;
	std::uint32_t BuyersPhrase;
	float ChanceOfBecomingAvailable;
	bool CanBeDisplayedInShop;
};

class CSummonDef
{
public:
	std::int32_t CreatureToSummon;
	std::int32_t CreatureFamilyToSummon;
	std::int32_t NumberToSummon;
	float SummonRadius;
	std::int32_t MySummonLimit;
	std::int32_t MinimumSummon;
	float TimeBetweenSummons;
	std::int32_t SummonEffect;
	std::int32_t SummoneeEffect;
};

class CSummonableCreatureDef
{
public:
	float Rank;
	CDefString SummonedBrain;
};

class CSummonerDef
{
public:
	float LightningRangeForAutomaticUse;
	float LightningPercentChanceForRandomUse;
	float LightningPercentChanceForBoastUse;
	float LightningRandomChanceTestSeconds;
	float LightningSecondsToWaitBeforeReuse;
	float ChargeRangeNear;
	float ChargeRangeFar;
	float ChargeRangeAttackInterrupt;
	float ChargeSecondsToWaitBeforeReuse;
	std::int32_t SummonerDeathExplosion;
	std::int32_t LightningOrb;
	std::int32_t LightningOrbAttackFireheart;
	float LightningOrbLifeSeconds;
};

class CTCNoiseDef
{
public:
	ENoiseType Type;
	float LifeTime;
	float DistanceCanBeHeardFrom;
};

class CTCVolumeContainmentTrackerDef
{
public:
	bool UpdateEveryFrame;
	bool CalculateBlendedLightingChannel;
	float LightingChannelSphereModelRadius;
	float LightingChannelSphereModelZOffset;
};

class CTargetingDef
{
public:
	std::uint32_t Type;
	bool UseGlowTargeting;
	float TargetArc;
	float TargetArcPCThirdPersonAimingMode;
	bool TargetingBasePosFromCamera;
	bool TargetingFacingDirFromCamera;
	bool TargetingPCFacingDirFromCamera;
	bool RejectTargetsBehindIfRunning;
	bool AcceptOnlyEnemiesBehind;
	float EnemyBehindTargetingRange;
	float FirstPersonTargetingAngleFalloff;
	float FirstPersonTargetingDistanceFalloff;
	float ThirdPersonTargetingAngleFalloff;
	float ThirdPersonTargetingDistanceFalloff;
	float ZLockTargetSelectAngleFalloff;
	float ZLockTargetSelectDistanceFalloff;
	float FirstPersonTargetingOverrideShortRangeFactor;
	float FirstPersonTargetingOverrideShortRangeFalloff;
	float ThirdPersonTargetingOverrideShortRangeFactor;
	float ThirdPersonTargetingOverrideShortRangeFalloff;
	float FirstPersonTargetingEnemyPreferenceFactor;
	float ThirdPersonTargetingEnemyPreferenceFactor;
	std::map<ETargetingType,float> TargetingRanges;
	float PreferPlayerWeight;
	float PreferNonCombatantsWeight;
	float PreferLastAttackerWeight;
	float PreferCurrentBestEnemyWeight;
};

class CTattooDef
{
public:
	std::string TattooName;
	std::int32_t TextureLayer;
	std::int32_t ReplacingTextureIndex;
	std::int32_t BankIndex;
	ECompositeBlendType Blend;
	bool RemovesAllTattoos;
	bool Permanent;
	std::int32_t CoversBodyAreaFlags;
	float VisibilityMultiplier;
	std::int32_t SpecificCoversBodyAreaFlags;
	std::string CustomFileName;
	std::string CustomFileNameBase;
	float Attractiveness;
	float Scariness;
	float Goodstrength;
};

class CTavernDef
{
public:
	std::int32_t BedCost;
};

class CTavernGameCardBaseDef
{
public:
	ETavernGameControlType ControlType;
	float CardWidth;
	float CardHeight;
	float ImpulseScaleXbox;
	float ImpulseScalePC;
	std::int32_t PointerModel;
	float MoveSpeed;
	float TurnSpeed;
	float PointerHeightOffset;
	std::int32_t PointerParticleEffect;
	float HighlightWidth;
	CRGBColour HighlightColourGood;
	CRGBColour HighlightColourBad;
	float CardSeparation;
	float TableSeparation;
	C2DVector PackOffset;
	std::vector<CCardPositionsDef> CardOrders;
	std::vector<CCardDef> CardPack;
};

class CTavernGameCoinBaseDef
{
public:
	float CoinIDensity;
	float CoinRadius;
	float CoinEdgeRestitution;
	float CoinEdgeFriction;
	float CoinSurfaceFriction;
	std::int32_t CoinDefIndex;
	std::int32_t ArrowDefIndex;
};

class CTavernGameCoinGolfDef
{
public:
	float TargetDistanceScale;
	float JoystickScaleXboxX;
	float JoystickScaleXboxY;
	float JoystickScaleX;
	float JoystickScaleY;
	float JoystickMagBelow;
	float AverageJoystickMagAbove;
	float AverageDrift;
	float MaxPower;
	float RestSpeed;
	std::int32_t Courses;
	float GoalRadius;
	std::int32_t StartingCourse;
};

class CTavernGameDef
{
public:
	std::uint32_t Banter;
	std::uint32_t Greeting;
	std::uint32_t OptionsInitial;
	std::uint32_t OptionsSubsequent;
	std::uint32_t Instructions;
	std::uint32_t InstructionsPC;
	std::uint32_t Betting;
	std::uint32_t Play;
	std::uint32_t ReactionWin;
	std::uint32_t ReactionLose;
	std::uint32_t ReactionDraw;
	std::uint32_t ReactionWinNewBestScore;
	std::uint32_t FarewellInitial;
	std::uint32_t FarewellSubsequent;
	std::uint32_t WinRoundPhrase;
	std::uint32_t OutOfTimePhrase;
	std::uint32_t NoMoney;
	std::uint32_t GreetingForward;
	std::uint32_t OptionsForward;
	std::uint32_t OptionsBack;
	std::uint32_t OptionsAlternative;
	std::uint32_t InstructionsBack;
	std::uint32_t BettingForward;
	std::uint32_t BettingBack;
	std::uint32_t ReactionForward;
	std::uint32_t FarewellForward;
	CDefString CameraName;
	std::uint32_t BoxGraphicL;
	std::uint32_t BoxGraphicC;
	std::uint32_t BoxGraphicR;
	std::uint32_t ClickToContinue;
	std::uint32_t WinPhrase;
	std::uint32_t LosePhrase;
	std::uint32_t DrawPhrase;
	std::uint32_t NewGame;
	std::uint32_t BestScore;
	std::uint32_t CurrentScore;
	std::uint32_t RequiredScore;
	std::uint32_t AdditionalInfo;
	std::uint32_t BlackjackBusted;
	std::uint32_t BlackjackSplit;
	std::uint32_t BlackjackDouble;
	std::uint32_t BlackjackHit;
	std::uint32_t BlackjackStand;
	std::uint32_t BlackjackDealerTakesCard;
	std::uint32_t BlackjackSplitGUI;
	std::uint32_t BlackjackDoubleGUI;
	std::uint32_t BlackjackHitGUI;
	std::uint32_t BlackjackStandGUI;
	std::uint32_t Bet;
	std::uint32_t PlayersMoney;
	std::uint32_t TotalWinnings;
	std::uint32_t Continue;
	std::uint32_t Quit;
	std::uint32_t Yes;
	std::uint32_t No;
	std::uint32_t PrizeGiven;
	std::uint32_t MoneyBagGraphic;
	std::int32_t MinBet;
	std::int32_t MaxBet;
	std::int32_t BetIncrement;
	CDefString ScoreFont;
	CDefString TargetFont;
	CDefString StatsFont;
	float ScoreX;
	float ScoreY;
	float TargetX;
	float TargetY;
	float BestX;
	float BestY;
	float AdditionalX;
	float AdditionalY;
	float BetX;
	float BetY;
	float MoneyX;
	float MoneyY;
	float WinningsX;
	float WinningsY;
	float MainBetX;
	float MainBetY;
	float MainMoneyX;
	float MainMoneyY;
	bool BestScoreHigh;
	std::vector<CPrizeScoreDef> PrizeScores;
	std::int32_t Prize;
	std::int32_t PrizeRenown;
	float MainGameScoreBoxX;
	float MainGameScoreBoxY;
	float MainGameScoreBoxWidthXbox;
	float MainGameScoreBoxWidthPC;
	float MainGameScoreBoxHeight;
	bool DisplayErrata;
	float PointerPhaseSpeed;
};

class CTavernGameShoveHaPennyDef : public CTavernGameDef
{
public:
	std::vector<CShoveHaPennyRegionDef> Regions;
	std::int32_t BoardDefIndex;
	std::int32_t ArrowDefIndex;
	float PositionCoinSpeedXbox;
	float PositionCoinSpeedPC;
	float PositionCoinMinimum;
	float PositionCoinMaximum;
	float PositionCoinDepth;
	float ImpulseScale;
	float TargetDistanceScale;
	float JoystickScaleXboxX;
	float JoystickScaleXboxY;
	float JoystickScalePCX;
	float JoystickScalePCY;
	float JoystickTranslateY;
	float JoystickMagBelow;
	float AverageJoystickMagAbove;
	float AverageDrift;
	float MaxPower;
	float RestSpeed;
	std::int32_t Rounds;
};

class CTavernGameSpotTheAdditionDef
{
public:
	std::int32_t Pointer;
	std::int32_t Cover;
	float CoverWidth;
	float CoverLength;
	std::vector<std::int32_t> Knicknacks;
	std::vector<std::int32_t> ItemsPerRound;
	float JoystickScale;
	float TimeToThink;
	float MaxCursorHeight;
	float KnickKnackRadius;
	float ImpulseScale;
	std::int32_t PointerModel;
	float PointerHeightOffset;
	float TimeToMoveCover;
	float TimeToPauseCover;
	float MaxCoverHeight;
	float MinimumKnickKnackGap;
	float LengthMovementScale;
	float HeightMovementScale;
	float HighlightWidth;
	CRGBColour HighlightColour;
	CRGBColour HighlightColourError;
};

class CTavernTableDef
{
public:
	bool AddTankards;
	bool HighQualityTankards;
};

class CTeleporterDef
{
public:
	float Radius;
	std::int32_t ActiveEffect;
	bool ActiveByProximity;
};

class CTextureReplacementDef
{
public:
	class CEntry
	{
	public:
		std::int32_t OriginalTexture;
		std::int32_t NewTexture;
	};

	std::vector<CTextureReplacementDef::CEntry> Entries;
};

class CThingComponentSet
{
public:
	class CEntry
	{
	public:
		CDefString Name;
		std::uint32_t DriverType;
		bool Override;
	};

	std::vector<CThingComponentSet::CEntry> Entries;
	std::uint32_t TrailingU32 = 28011726;
};

class CThingBaseDef
{
public:
	CThingComponentSet Components;
	bool AvailableInEditor : 1;
	bool AlwaysFrameUpdate : 1;
	bool StartKillLocked : 1;
	bool LevelLess : 1;
	bool IsDrawable : 1;
	bool IsDrawableInEditor : 1;
	bool ForceNoSerialise : 1;
	bool InMapSearches : 1;
	bool IsLoadableGlobal : 1;
	bool GameStatePersisted : 1;
	bool CanComeBetweenCameraAndObservedThing : 1;
	bool DrawWeaponTrails : 1;
	bool DrawProjectileWeaponTrails : 1;
	bool IsVulnerableToMelee : 1;
	bool Damageable : 1;
	bool AlwaysIncludeInObjectStrikeScans : 1;
	bool DisableOcclusionTesting : 1;
	bool DrawAfterWater : 1;
	bool ForceRenderedLastFrameOverride : 1;
	bool AddToComboMultiplierOnHit : 1;
	bool GiveHeroStatChangesOnBeingHit : 1;
	std::int32_t GroupDef;
	float RenderFadeDistance;
	std::int32_t PersistenceFlags;
	float Health;
	float MinHealth;
	float MeshHeightOffset;
	float MeshHeight;
	float MeshRadius;
	float ApproxMaxMeshHeight;
	CDefString DefaultScriptName;
	CDefString DefaultScriptData;
};

class CThingBuildingDef : public CThingBaseDef
{
public:
	std::int32_t DieOffTimer;
	std::uint32_t SimBuildingDef;
	CEngineGraphic Graphic;
	std::int32_t WallMaterial;
	std::int32_t FloorMaterial;
	EReverbEnvironmentType ReverbEnvironmentType;
	float ReverbEnvironmentLevel;
	float OutsideOcclusionLevel;
	std::int32_t CameraManagerSetIndex;
	bool UseHighDetailQuadTree : 1;
	bool IsSelfIlluminating : 1;
	bool IsPartOfHeroGuild : 1;
	bool HasWeatherMask : 1;
};

class CThingCreatureDef : public CThingBaseDef
{
public:
	float StunTimeMin;
	float StunTimeMax;
	float StunHealthThreshold;
	float DazedTimeMin;
	float DazedTimeMax;
	std::int32_t DazedHealthThreshold;
	float NumSecondsToPersistOnDeath;
	float MaxSlowWalkingSpeed;
	float MaxWalkingSpeed;
	float MaxJoggingSpeed;
	float MaxRunningSpeed;
	float MaxPushingSpeed;
	float MaxFlyingSpeed;
	float MaxHoverSpeedSlow;
	float MaxHoverSpeedMedium;
	float MaxHoverSpeedFast;
	float SpeedRandomisationFactor;
	float MaxAcceleration;
	float HearingRadius;
	float FOV;
	ESex Sex;
	ECreatureType CreatureType;
	std::vector<ENavigatorType> NavigatorTypes;
	std::int32_t InitialWeaponDef;
	std::int32_t PrimaryMeleeWeaponDef;
	std::int32_t SecondaryMeleeWeaponDef;
	std::int32_t RangedWeaponDef;
	std::int32_t InitialSheathedWeapon;
	std::int32_t BrainDef;
	ECreatureInteractionType CreatureInteractionType;
	EThingCreatureProperty Property;
	float Damage;
	std::int32_t DamageEffectDef;
	std::int32_t StunnedParticle;
	CEngineGraphic Graphic;
	CEngineGraphic EyeGraphic;
	std::int32_t DefaultOwner;
	EControlledMovementType ControlledMovementType;
	std::int32_t CombatTypeDef;
	bool ResetCombatTypeOnUnsheatheDefaultWeapons;
	std::int32_t HeroCombatDef;
	std::int32_t HeroCombatPCDef;
	float CombatDiameter;
	bool DeferHitsIfBehindHitter : 1;
	std::int32_t LeaderTypeDef;
	float RecoverEvadeHitsPercent;
	std::int32_t DeadBodyDecayEffect;
	bool LeaveDeadCreature : 1;
	bool AbleToStrafe : 1;
	bool CreateDummyOnDeath : 1;
	bool ImmuneToAttack : 1;
	bool UseMeleeWeaponAsDefault : 1;
	bool UnsheatheWeaponsAutomatically : 1;
	bool IsWillable : 1;
	bool IsIndestructible : 1;
	bool IsInvulnerableDuringGenericResponse : 1;
	bool IsPushedByExplosions : 1;
};

class CThingDef : public CThingBaseDef
{
public:
};

class CThingDrainLifeShotDef
{
public:
	float MaxDistance;
	float MaxAngle;
	float MaxAngleToTurnDegs;
	float InitialSpeed;
	float SpeedRange;
};

class CThingGroupDef
{
public:
	bool Dummy;
};

class CThingHolySiteDef : public CThingBaseDef
{
public:
	CEngineGraphic Graphic;
	bool PlayerStart;
};

class CThingMarkerDef : public CThingBaseDef
{
public:
	CEngineGraphic Graphic;
	ESwitchTriggerType TriggerType;
	std::uint32_t Properties;
};

class CThingMultiArrowShotDef
{
public:
	float AccelerationScalar;
	float MaxDistance;
	float MaxAngle;
	float TimeToReachTarget;
	float MaxAngleToTurnDegs;
	std::int32_t MultiArrowWeaponTrail;
	std::int32_t MultiArrowHitEffect;
	std::int32_t MultiArrowTrailEffect;
	float MultiArrowSpeed;
	bool MultiArrowDeflectionByMelee;
	float MultiArrowHitEffectPercentage;
};

class CThingNoiseDef : public CThingBaseDef
{
public:
	CTCNoiseDef Noise;
};

class CThingObjectDef : public CThingBaseDef
{
public:
	CEngineGraphic Graphic;
	float RotationTime;
	std::int32_t Material;
	std::int32_t FloorMaterial;
	std::int32_t OnDeathObject;
	std::uint32_t Property;
	std::int32_t DieOffTimer;
	bool IsFragile : 1;
	bool IsClothing : 1;
	bool IsSelfIlluminating : 1;
	bool UseHighDetailQuadTree : 1;
	bool Electrocutable : 1;
};

class CThingPhysicalSwitchDef : public CThingBaseDef
{
public:
	EPSwitchTriggerType TriggerType;
	CEngineGraphic UntriggeredGraphic;
	CEngineGraphic TriggeredGraphic;
	std::int32_t TriggerSound;
};

class CThingShotDef
{
public:
	float Speed;
	std::int32_t PrimaryEffect;
	std::int32_t PrimaryEffectSound;
	std::int32_t AttachedContinuousEffect;
	std::int32_t AttachedContinuousSound;
	bool UseEndTimeLineOnContinousEffect;
	float HitThingDamage;
	float HitThingShieldDamage;
	bool CauseDamage;
	CDefString ShotDefinitionClassName;
	CFloatRange ShotOffsetAngleRange;
	CFloatRange ShotOffsetRadiusRange;
	CDefIndex ExplosionDef;
	CDefIndex GroundExplosionDef;
	CDefIndex ObjectDef;
	bool LeavesObjectBehind;
	bool IsHittableByMelee;
	bool IsDeflectedOnHitByMelee;
	float MeleeHitDeflectionDamageMultiplier;
	bool FaceMovementDirection;
	bool AllowDeflections;
	bool BounceOnGround;
	float Bounciness;
	bool RotateOnDeflection;
	bool IsBlockable;
	EDamageAttribute DamageType;
	std::int32_t NumSubShots;
	std::int32_t SubShotType;
	float SubShotCreationTime;
	float SubShotMaxAngle;
	bool RemoveMainShotOnSubShotCreation;
	bool DoesExplosionDamageCaster;
	bool CanHitRollingTargets;
	bool PreventSlidingOnCollision;
};

class CThingSwitchDef : public CThingBaseDef
{
public:
	ESwitchTriggerType TriggerType;
};

class CThingVillageDef : public CThingBaseDef
{
public:
	CEngineGraphic Graphic;
};

class CThunderBattleDef
{
public:
	std::int32_t NumGetHitsBeforeStormAttack;
	std::int32_t NumMissesBeforeStormAttack;
	float LightningDamage;
	std::int32_t BattleChargeLevel;
};

class CTimeAppearanceFadeDef
{
public:
	float FadeInStart;
	float FadeInEnd;
	float FadeOutStart;
	float FadeOutEnd;
};

class CTrapDef
{
public:
	std::map<std::string,std::int32_t> Sound;
	ETrapTriggerType TriggerType;
	ETrapType TrapType;
	float TriggerRadius;
	CDefString TriggerHelperName;
	float TrapDamage;
	std::int32_t TriggeredCollisionMesh;
	std::int32_t UntriggeredCollisionMesh;
	std::int32_t ExplosionDefIndex;
	std::int32_t PhysicalObstructionDefIndex;
	std::int32_t NumShotsToFireBeforeIdle;
	float SecondsIdle;
	float SecondsBetweenShots;
};

class CTrollBattleDef
{
public:
	float GroundPoundSpeedMultiplierStart;
	float GroundPoundSpeedMultiplierEnd;
	float AttackPosPredictiveness;
	float PercentageChanceToUseSecondaryShotType;
	std::int32_t SecondaryShotType;
	float MinLaughDistance;
	float MaxTimeBetweenKnockdownAndBoast;
	std::int32_t DeathExplosionIndex;
	std::int32_t FallDownExplosionIndex;
};

class CTrophyDef
{
public:
	float MinigameInitialTimerSecs;
	float TimeBonusPerWitness;
	std::int32_t RenownGainedPerWitnessAhead;
	bool EvilTrophy;
	bool FakeTrophy;
	std::int32_t EmoteIconForAvailableWitness;
	CDefString ShowTrophyAnimName;
	float GameDebugTextScale;
	float SecsDelayPerGainedWitnessSound;
	std::string SoundStartGame;
	std::string SoundShowTrophy;
	std::string SoundGainedWitness;
	std::string SoundOutOfTime;
	std::string SoundGotAllWitnesses;
	std::string SoundEndNewHighScore;
	std::string SoundEndNoNewScore;
};

class CTurncoatDef
{
public:
	bool Turncoatable;
	std::int32_t TurncoatedEffect;
	std::int32_t TurncoatActivateEffect;
	std::int32_t TurncoatDeactivateEffect;
	float MinimumResistance;
	float TimeDelayBeforeResistanceRecoverySecs;
	float ResistanceRecoveryPerSec;
	std::vector<std::int32_t> TurncoatStageEffects;
	float PercentageOfHealthAsResistance;
	float ExtraResistance;
};

class CVillageDef
{
public:
	std::vector<EVillageTaskType> Tasks;
	CDefString PatrolTrackName;
	EVillageStanceToHero DefaultStanceToHero;
	std::int32_t SecondsSecuritySweepLasts;
	std::int32_t WarehouseCrateDef;
	std::vector<std::int32_t> StoryStartTimes;
	std::int32_t StoryDuration;
	std::vector<std::int32_t> SchoolStartTimes;
	std::int32_t SchoolDuration;
	std::vector<std::int32_t> WorkStartTimes;
	std::int32_t WorkDuration;
	std::vector<std::int32_t> CookStartTimes;
	std::int32_t CookDuration;
	std::vector<std::int32_t> EatStartTimes;
	std::int32_t EatDuration;
	std::vector<std::int32_t> TavernStartTimes;
	std::int32_t TavernDuration;
	std::vector<std::int32_t> GamingStartTimes;
	std::int32_t GamingDuration;
	std::int32_t SleepAdultStartTime;
	std::int32_t SleepAdultEndTime;
	std::int32_t SleepChildStartTime;
	std::int32_t SleepChildEndTime;
	std::int32_t SleepElderlyStartTime;
	std::int32_t SleepElderlyEndTime;
	std::int32_t HouseLightsOnTime;
	std::int32_t HouseLightsOffTime;
	std::int32_t SociableStartTime;
	std::int32_t SociableFinishTime;
	std::int32_t CurfewStartTime;
	std::int32_t CurfewFinishTime;
	bool EnableAutoPlacement;
	bool AllowHouseBuying;
	bool HearTownCrier;
	std::string CurfewStartSound;
	std::string CurfewEndSound;
	std::string WorkStartSound;
	std::string WorkEndSound;
	std::int32_t BribeCost;
	std::int32_t FramesBetweenGuardSnoop;
	std::int32_t FramesBetweenGuardSnoopNightShops;
	std::int32_t FramesBeforeGuardSnoop;
	std::int32_t FramesBeforeGuardSnoopNightShops;
};

class CVillageMemberDef
{
public:
	bool NeedsVillageToFunctionCorrectly;
};

class CVillagePeopleDef
{
public:
	bool Enable;
	float DesiredMaleWeight;
	float DesiredFemaleWeight;
	float DesiredChildWeight;
	float ProbabilityOfHomsexualityPerDialogueEnabledMan;
	std::int32_t MaleTeacher;
	std::int32_t FemaleTeacher;
	std::int32_t MaleShopKeeper;
	std::int32_t FemaleShopKeeper;
	std::int32_t MaleMarketTrader;
	std::int32_t FemaleMarketTrader;
	std::int32_t MaleBarman;
	std::int32_t FemaleBarman;
	std::int32_t MaleBarmaid;
	std::int32_t FemaleBarmaid;
	std::int32_t MaleWarehouseWorker;
	std::int32_t FemaleWarehouseWorker;
	std::int32_t MaleServant;
	std::int32_t FemaleServant;
	std::int32_t MaleGoodApprentice;
	std::int32_t FemaleGoodApprentice;
	std::int32_t MaleEvilApprentice;
	std::int32_t FemaleEvilApprentice;
	std::int32_t MaleUnemployed;
	std::int32_t FemaleUnemployed;
	std::int32_t MaleChild;
	std::int32_t FemaleChild;
	std::int32_t GamesMaster;
	std::int32_t Guard;
	std::int32_t MaleGhost;
	std::int32_t FemaleGhost;
	std::int32_t NumGuards;
	std::int32_t NumGhosts;
	std::vector<std::int32_t> GuardRespawnType;
	std::vector<std::int32_t> GuardRespawnQuantity;
	std::int32_t GuardRespawnMaxRenownBasedOffset;
};

class CVillagerInteractionsDef
{
public:
	CDefString Animation1;
	CDefString Animation2;
	float Distance;
};

class CWallMountEffectsDef
{
public:
	EWallMountEffects Effect;
	float HealingAmount;
	std::int32_t SecondsBetweenEffectUsesAllowed;
};

class CWaspQueenBattleDef
{
public:
	std::int32_t NumFramesPauseBetweenAttacks;
};

class CWeaponTrailGraphicSet
{
public:
	std::int32_t Attack;
	std::int32_t Knockdown;
};

class CWeaponDef
{
public:
	EWeaponType Type;
	EWeaponClass Class;
	std::uint32_t Property;
	std::string AnimationGroupName;
	std::int32_t SheatheObject;
	bool SheatheObjectAlwaysVisible;
	float AnimationSpeedVal;
	float Damage;
	EDamageAttribute DamageType;
	std::int32_t Explosion;
	CObjectAugmentationParticleSet FlourishParticles;
	CWeaponTrailGraphicSet FlourishSpecialWeaponTrail;
	std::map<EObjectAugmentationType,CWeaponTrailGraphicSet> WeaponTrails;
	bool UseAnalogueLoading;
	std::int32_t MissileLoadCarrySlot;
	std::string ProjectileWeaponHelperNameTip;
	std::string ProjectileWeaponHelperNameBase;
	std::int32_t Shot;
	std::int32_t Ammo;
	std::int32_t TargetingRange;
	float TargetingFOV;
	std::map<EObjectAugmentationType,std::int32_t> ArrowTrails;
	CFloatRange ReloadSpeedRange;
	bool ProjectileWeaponAutoReload;
	bool ProjectileWeaponStartLoaded;
	bool UseExtendedProjectileTrails;
	std::int32_t NextWeaponForBoss;
	float MeleeTargetingArc;
	bool NeedsLoading;
	std::string UnsheatheToCombatAnim;
	std::string UnsheatheToNormalAnim;
	std::string SheatheFromCombatAnim;
	std::string SheatheFromNormalAnim;
};

class CWhisperBattleDef
{
public:
	std::int32_t NumGetHitsBeforeSommersault;
	std::int32_t NumPhase2Potions;
};

class CWifeDef
{
public:
	std::int32_t Dowry;
};

class CWillResponseDef
{
public:
	bool IsForcePushable;
	bool IsLifeDrainable;
	bool IsLightningable;
	bool IsNotBloodyWillable;
	bool IsAffectedBySlowTime;
	bool IsAbleToBeLifted;
	bool IsEpicPowerExplodable;
	bool IsEpicSpellable;
};

class CWoundMorphs
{
public:
	class CEntry
	{
	public:
		EBodyLocation Location;
		std::int32_t ReplacingTextureIndex;
		std::int32_t ReplacingBumpMapIndex;
		std::int32_t WoundTextureIndex;
		std::int32_t WoundBumpMapIndex;
		std::int32_t ScarTextureIndex;
		std::int32_t ScarBumpMapIndex;
		std::int32_t FadeToScarDuration;
		std::int32_t TotalDuration;
		float PermanentScarStrength;
	};

	std::vector<std::pair<EBodyLocation,CWoundMorphs::CEntry>> Morphs;
	std::uint32_t TrailingU32 = 28011758;
};

namespace NUISystem {
class CUIStateDef
{
public:
	std::uint32_t GraphicIndex;
	C2DVector Position;
	C2DVector Zoom;
	CRGBFloatColour Colour;
	float UpdateTime;
	NUISystem::EStateChangeType StateChangeType;
	bool LinearChange;
	std::uint32_t StateChangeFlag;
	std::vector<std::uint32_t> ChildrenNotAffected;
};
}

namespace NUISystem {
class CUIDef
{
public:
	NUISystem::EType Type;
	std::vector<NUISystem::CUIStateDef> States;
	std::uint32_t MeshIndex;
	CDefString Font;
	std::u16string TextValue;
	float Height;
	float Width;
	NUISystem::ETableExpansionTypes ExpansionType;
	std::map<NUISystem::ETableSprites,std::int32_t> Sprites;
	std::vector<std::int32_t> Children;
	std::vector<std::uint32_t> HorizontalSeparations;
	std::vector<std::uint32_t> VerticalSeparations;
	std::vector<std::uint32_t> NonScrollingChildren;
	EEngineGraphicType MeshType;
	C2DBoxF TextWindow;
	std::int32_t Layer;
	float Angle;
	bool PositionIsCenter;
	bool TextLineBreak;
	bool ScaleText;
	bool Independant;
	float ScrollingSpeed;
	NUISystem::EActionType Action;
	NUISystem::EActionType ActionOnBack;
	NUISystem::EActionType ActionOnSelected;
	NUISystem::EActionType ActionOnUnselected;
	NUISystem::EActionType ActionOnDestruction;
	NUISystem::EActionType PreAction;
	NUISystem::EActionType ActionOnDroppedNowhere;
	NUISystem::EActionType ActionOnLeftClicked;
	NUISystem::EActionType ActionOnLeftUnclicked;
	NUISystem::EActionType ActionOnLeftHeld;
	NUISystem::EActionType ActionOnRightClicked;
	NUISystem::EActionType ActionOnDropped;
	NUISystem::EActionType ActionOnDraggedUp;
	NUISystem::EActionType ActionOnDraggedDown;
	NUISystem::EActionType ActionOnLeftClickedUnder;
	NUISystem::EActionType ActionOnLeftClickedAbove;
	C3DVector Up;
	C3DVector Forward;
	C3DVector RotationAxis;
	float RotationSpeed;
	std::uint32_t AnimationIndex;
	std::int32_t DownArrow;
	std::int32_t UpArrow;
	std::int32_t UpLimit;
	std::int32_t DownLimit;
	bool Wrapping;
	bool Inverted;
	C2DVector PositionOffset;
	std::uint32_t AlphaOffset;
	bool Scrolling;
	C2DBoxI ViewArea;
	bool ComputeOffsetsOnActivate;
	NUISystem::EState ParentStatePrerequisite;
	C2DVector Dimensions;
	C2DVector Min;
	C2DVector Max;
	C2DVector Step;
	float InputDelay;
	bool DrawFromViewport;
	std::int32_t SliderLeft;
	std::int32_t SliderRight;
	std::uint32_t TextBankIndex;
	std::int32_t ActionText;
	std::int32_t KeyText;
	std::int32_t UndefinedWarning;
	std::int32_t Redefiner;
	std::map<std::uint32_t,std::string> ActionMap;
	std::map<std::uint32_t,std::uint32_t> ActionMapAliases;
	std::vector<std::uint32_t> ActionOrder;
	bool EditBoxParentIsButton;
	bool PasswordBox;
	std::int32_t EditBoxCharLimit;
	bool EditBoxUsesIME;
	bool DisallowSpaceAsFirstChar;
	std::u16string MovieFilename;
	bool LayerIndependant;
	std::vector<std::uint32_t> SwappingStates;
	std::vector<float> SwappingTimes;
	bool BastardChild;
	NUISystem::ETextAlignement Alignement;
	bool RandomSwap;
	std::int32_t HoveredState;
	bool UseRelativeZoom;
	bool UseRelativePosition;
	bool PartOfListTree;
	std::int32_t LeftClickedState;
	std::int32_t RightClickedState;
	std::vector<std::uint32_t> ShapeChildren;
	bool UseViewArea;
	bool PCStyle;
	EEngineSprite2DFlag Sprite2DFlag;
};
}

namespace NUISystem {
class CUIIconsDef
{
public:
	std::uint32_t IconFriendRequestReceived;
	std::uint32_t IconFriendRequestReceivedOn;
	std::uint32_t IconFriendRequestSent;
	std::uint32_t IconFriendRequestSentOn;
	std::uint32_t IconGameInviteReceived;
	std::uint32_t IconGameInviteReceivedOn;
	std::uint32_t IconGameInviteSent;
	std::uint32_t IconGameInviteSentOn;
	std::uint32_t IconMute;
	std::uint32_t IconMuteOn;
	std::uint32_t IconOnline;
	std::uint32_t IconOnlineOn;
	std::uint32_t IconPasscodeBlank;
	std::uint32_t IconPasscodeFilled;
	std::uint32_t IconTV;
	std::uint32_t IconTVOn;
	std::uint32_t IconVoice;
	std::uint32_t IconVoiceOn;
	std::uint32_t IconWait1;
	std::uint32_t IconWait2;
	std::uint32_t IconWait3;
	std::uint32_t IconWait4;
	std::uint32_t IconProgress;
	std::uint32_t IconProgressOn;
	std::uint32_t IconA;
	std::uint32_t IconB;
	std::uint32_t IconX;
	std::uint32_t IconY;
	std::uint32_t IconBlank;
	std::uint32_t IconUpArrow;
	std::uint32_t IconDownArrow;
	std::uint32_t IconListHighlight;
};
}

namespace NUISystem {
class CUILocaleGraphicsDef
{
public:
	std::map<EWorldMapNameGraphic,std::uint32_t> WorldMapRegionName;
	std::vector<std::uint32_t> HelpScreenGraphics;
	std::uint32_t HelpRingPic;
};
}

namespace NUISystem {
class CUIMiscThingsDef
{
public:
	std::u16string SpaceSeparator;
	std::u16string CommaSeparator;
	std::u16string ColonSeparator;
	std::u16string NewLineSeparator;
	std::u16string OpenBracket;
	std::u16string CloseBracket;
	std::u16string Positive;
	std::u16string WeaponValueString;
	std::u16string WeaponAugString;
	std::u16string WeaponAugNone;
	std::u16string WeaponWeightString;
	std::u16string WeaponLightString;
	std::u16string WeaponHeavyString;
	std::u16string WeaponKillsString;
	std::u16string WeaponCatMeleeString;
	std::u16string WeaponCatRangedString;
	std::u16string WeaponDamageString;
	std::u16string TradeCostString;
	std::u16string TradeProfitString;
	std::u16string TradeLossString;
	std::u16string TradeAlreadyOwnsString;
	std::u16string TradeNumberInStockString;
	std::u16string TradeDeliveryString;
	std::u16string TradeNoDeliveryString;
	std::u16string TradeDaysString;
	std::u16string TradeBuyString;
	std::u16string TradeSellString;
	std::u16string TradeWantedString;
	std::u16string QuestFailedString;
	std::u16string FailedString;
	std::u16string SucceededString;
	std::u16string Plus;
	std::u16string Minus;
	std::uint32_t CoreGraphic;
	std::uint32_t VignetteGraphic;
	std::uint32_t OptionalGraphic;
	std::uint32_t FeatGraphic;
	std::u16string ObjectsRewardString;
	std::u16string NoneString;
	std::u16string CheckGuildString;
	std::u16string QuestStartingString;
	C2DVector RingCenter;
	C2DVector PCRingCenter;
	C2DVector WorldMapOffset;
	float WorldMapWidth;
	float WorldMapHeight;
	std::u16string YouString;
	std::u16string OwnString;
	std::u16string NoString;
	std::u16string HousesString;
	std::u16string HouseString;
	std::u16string InString;
	std::u16string ShopsString;
	std::u16string ShopString;
	std::u16string ThereString;
	std::u16string AreString;
	std::u16string IsString;
	std::u16string ForString;
	std::u16string SaleString;
	std::u16string GeneralString;
	std::u16string TitleString;
	std::u16string BarberString;
	std::u16string TatooString;
	std::u16string LevelString;
	std::uint32_t TotalSpellsInPalettes;
	std::uint32_t TotalSpellsInContainer;
	std::u16string LogBookBasicsCategoryString;
	std::u16string LogBookObjectsCategoryString;
	std::u16string LogBookTownsCategoryString;
	std::u16string LogBookHeroCategoryString;
	std::u16string LogBookCombatCategoryString;
	std::u16string LogBookQuestCategoryString;
	std::u16string LogBookStoryCategoryString;
	std::u16string LogBookBasicsCategoryNameString;
	std::u16string LogBookObjectsCategoryNameString;
	std::u16string LogBookTownsCategoryNameString;
	std::u16string LogBookHeroCategoryNameString;
	std::u16string LogBookCombatCategoryNameString;
	std::u16string LogBookQuestCategoryNameString;
	std::u16string LogBookStoryCategoryNameString;
	std::map<std::uint32_t,std::uint32_t> MapPaths;
	std::string SoundUpDown;
	std::string SoundSlider;
	std::string SoundBack;
	std::string SoundForward;
	std::string SoundError;
	std::string SoundExit;
	std::string CountUpSound;
	std::string SoundKeyboardUp;
	std::string SoundKeyboardDown;
	std::string SoundKeyboardLeft;
	std::string SoundKeyboardRight;
	std::string SoundKeyboardEnterCharacter;
	std::string SoundKeyboardDeleteCharacter;
	std::string SoundKeyboardDone;
	std::u16string FrontEndMusic;
	C2DVector HeroDollTL;
	C2DVector HeroDollBR;
	float HeroDollSphereRadius;
	C2DVector HeroDollTL_PC;
	C2DVector HeroDollBR_PC;
	float HeroDollSphereRadius_PC;
	C2DVector HeroDollFrameTL_PC;
	float HeroDollFrameEmulateListOffset;
	EMusicSetType QuestStartScreenMusic;
	EMusicSetType QuestCompleteScreenMusic;
	EMusicSetType QuestFailureScreenMusic;
	EMusicSetType DeathScreenMusic;
	float DigitCountTime;
	std::uint32_t SaveHeroGraphicIndex;
	std::map<std::string,std::int32_t> MiniMapGraphics;
	std::int32_t KeyboardSmallKeyGraphic;
	std::int32_t KeyboardLargeKeyGraphic;
	std::uint32_t TotalAssignableThings;
	float TimeInSecsForFade;
	float BackBufferFilterSaturation;
	float BackBufferFilterContrast;
	float BackBufferFilterBrightness;
	float BackBufferFilterTintR;
	float BackBufferFilterTintG;
	float BackBufferFilterTintB;
	float BackBufferFilterTintScale;
	float BackBufferDiffuseScale;
	float BackBufferAmbientScale;
	float MinimumFilterColor;
};
}

} // namespace defs
} // namespace fable

#endif // FABLE_DEFS_CLASSES_HPP
