//! Compound wire value types (`wire_struct!` / `def_variant!`).

use crate::enums::{
    ControllerType, DamageAttribute, EngineGraphicType, GameAction, HeroExperienceStatCategory,
    InputKey, MouseButtonControl, ReactionSpeechType, XboxControllerButton,
};
use crate::wire::{DefIndex, DefString, PString, VecMap};
use crate::{DefVariant, WireStruct};
use std::collections::BTreeMap;

/// C++ `CRGBColour`. The default ctor is opaque black
/// (`0xFF000000` — alpha 0xFF, rgb 0), which is what retail NULLDEFs store.
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RGBColour {
    #[def(default = 0xFF000000)]
    pub int_value: u32,
}

/// C++ `CEngineGraphic` — OG retail layout (the Anniversary build reordered
/// it to bank…alpha,type; the spec documents the newer order). Retail
/// bytes only fit type-first: `Type` is a 4-byte enum value and
/// `AdditiveAlpha` a 1-byte bool.
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct EngineGraphic {
    pub type_: EngineGraphicType,
    pub bank_index: i32,
    pub anim_step: f32,
    #[def(default = 1.0)]
    pub render_size_x: f32,
    pub additive_alpha: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SoundMapF0EntryValue {
    pub first_sound: u32,
    pub last_sound: u32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SoundMapF0Entry {
    pub key: u32,
    pub value: SoundMapF0EntryValue,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SoundMapF1Entry {
    pub key: u32,
    // A names.bin offset (the `AddCriteria` string), so it is a `DefString`, not
    // a bare int — lets the differ resolve it to the string across index spaces.
    pub value: DefString,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SoundMap {
    pub f0: Vec<SoundMapF0Entry>,
    pub f1: Vec<SoundMapF1Entry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct WoundMorphsMorphsEntry {
    pub body_location: i32,
    pub data: [u8; 40],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct WoundMorphs {
    pub morphs: Vec<WoundMorphsMorphsEntry>,
    #[def(default = 28_011_758)]
    pub trailing_u32: u32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts0MeshesEntryTextureMorphsEntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts0MeshesEntryTextureMorphs {
    pub entries: Vec<RandomAppearanceMorphBodyParts0MeshesEntryTextureMorphsEntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts0MeshesEntry {
    pub mesh_id: i32,
    pub texture_id: i32,
    pub texture_morphs: RandomAppearanceMorphBodyParts0MeshesEntryTextureMorphs,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts0 {
    pub meshes: Vec<RandomAppearanceMorphBodyParts0MeshesEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts1MeshesEntryTextureMorphsEntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts1MeshesEntryTextureMorphs {
    pub entries: Vec<RandomAppearanceMorphBodyParts1MeshesEntryTextureMorphsEntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts1MeshesEntry {
    pub mesh_id: i32,
    pub texture_id: i32,
    pub texture_morphs: RandomAppearanceMorphBodyParts1MeshesEntryTextureMorphs,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts1 {
    pub meshes: Vec<RandomAppearanceMorphBodyParts1MeshesEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts2MeshesEntryTextureMorphsEntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts2MeshesEntryTextureMorphs {
    pub entries: Vec<RandomAppearanceMorphBodyParts2MeshesEntryTextureMorphsEntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts2MeshesEntry {
    pub mesh_id: i32,
    pub texture_id: i32,
    pub texture_morphs: RandomAppearanceMorphBodyParts2MeshesEntryTextureMorphs,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyParts2 {
    pub meshes: Vec<RandomAppearanceMorphBodyParts2MeshesEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs0EntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs0 {
    pub entries: Vec<RandomAppearanceMorphBodyPartTextureMorphs0EntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs1EntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs1 {
    pub entries: Vec<RandomAppearanceMorphBodyPartTextureMorphs1EntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs2EntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphBodyPartTextureMorphs2 {
    pub entries: Vec<RandomAppearanceMorphBodyPartTextureMorphs2EntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphSkeletalMorphsEntry {
    pub key: i32,
    pub morph_id: i32,
    pub bone_or_morph_index: i32,
    pub allow_variation: bool,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphGeneralTextureMorphsEntriesEntry {
    pub key: u32,
    pub morph: [u8; 8],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorphGeneralTextureMorphs {
    pub entries: Vec<RandomAppearanceMorphGeneralTextureMorphsEntriesEntry>,
    pub trailing: u32,
    pub dirty: u8,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct RandomAppearanceMorph {
    #[def(default = 3)]
    pub num_body_parts: u32,
    pub body_parts0: RandomAppearanceMorphBodyParts0,
    pub body_parts1: RandomAppearanceMorphBodyParts1,
    pub body_parts2: RandomAppearanceMorphBodyParts2,
    pub body_part_texture_morphs0: RandomAppearanceMorphBodyPartTextureMorphs0,
    pub body_part_texture_morphs1: RandomAppearanceMorphBodyPartTextureMorphs1,
    pub body_part_texture_morphs2: RandomAppearanceMorphBodyPartTextureMorphs2,
    pub skeletal_morphs: Vec<RandomAppearanceMorphSkeletalMorphsEntry>,
    pub general_texture_morphs: RandomAppearanceMorphGeneralTextureMorphs,
    pub final_trailing: u32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ExpressionSetExpressionsEntry {
    pub type_: i32,
    pub data1: u32,
    pub data2: u32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ExpressionSet {
    pub expressions: Vec<ExpressionSetExpressionsEntry>,
    #[def(default = 28_011_826)]
    pub trailing_u32: u32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct FloatRange {
    pub low_value: f32,
    pub high_value: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ThingComponentSetEntriesEntry {
    // `CThingComponentSet::CEntry` (thing_component_set.hpp:867, packed 9 bytes):
    // CDefString Name(0x0), unsigned long DriverType(0x4), bool Override(0x8).
    // Typed (not a raw `[u8;9]`) so the differ resolves `name` by string value
    // instead of comparing raw names.bin offsets across two tables.
    pub name: DefString,
    pub driver_type: u32,
    pub over: bool,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ThingComponentSet {
    pub entries: Vec<ThingComponentSetEntriesEntry>,
    pub trailing_u32: u32,
}

/// C++ `COpinionTransientOffset` — 6 consecutive 4-byte values (24 bytes).
/// Layout confirmed against `tc_opinion_of_hero.hpp` (member offsets 0..0x14)
/// and retail bytes.
///
/// The text form `Effects.Add(opinion, peak, run_in_secs, run_out_secs,
/// persist)` supplies human-readable *seconds*; the constructor
/// (`tc_opinion_of_hero.cpp:4438`) converts them to per-frame rates and
/// frame counts at compile time (see `apply_opinion_transient_offset` in
/// the compiler). This struct stores the *computed* result, not the raw
/// text args — so a straight positional copy is wrong. The seconds→frames
/// factor is 15 (opinion tick rate).
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct OpinionTransientOffset {
    /// `EOpinion` axis (offset 0x0).
    pub opinion: i32,
    /// `OffsetPerFrameRunIn` = peak / max(run_in_frames, 1) (offset 0x4).
    pub offset_per_frame_run_in: f32,
    /// `FramesToRunIn` = max(run_in_secs*15, 1) (offset 0x8).
    pub frames_to_run_in: i32,
    /// `FramesOfCappedPeak` — always 0 from the text ctor (offset 0xc).
    pub frames_of_capped_peak: i32,
    /// `OffsetPerFrameRunOut` = (persist-peak)/max(run_out_frames,1) (offset 0x10).
    pub offset_per_frame_run_out: f32,
    /// `FramesToRunOut` = max(run_out_secs*15, 1) (offset 0x14).
    pub frames_to_run_out: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct OpinionTransientOffsetList {
    pub f0: Vec<OpinionTransientOffset>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct CombatAbilityData {
    #[def(default = 100)]
    pub percentage_chance: i32,
    #[def(default = -1.0)]
    pub seconds_duration: f32,
    pub seconds_to_wait_before_repeat: f32,
    pub anim: DefString,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct Vector3D {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct Vector2D {
    pub x: f32,
    pub y: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MiniMapGraphics {
    pub f0: VecMap<String, i32>,
}

/// Layout (tc decomp): `ParticleIndex`(long), `AttachmentObjectName`(CDefString),
/// `GenerationCutoffDistance`(float), `DummyObject`(bool). The text ctor is
/// `CParticleAttachmentInfo("<attach_point>", <PARTICLE>, dist, dummy)` — its
/// first two args are SWAPPED relative to this wire order (see the
/// `ParticleAttachmentInfo` arm in `apply_struct_from_expr`).
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ParticleAttachmentInfo {
    pub particle_index: i32,
    pub attachment_object_name: DefString,
    pub weight: f32,
    pub dummy_object: bool,
}

const fn personality_traits_default() -> [u8; 180] {
    let mut blob = [0u8; 180];
    let mut trait_idx = 0;
    while trait_idx < 5 {
        let base = trait_idx * 36;
        let mut field = 0;
        while field < 8 {
            let off = base + 4 + field * 4;
            let bytes = 1.0f32.to_le_bytes();
            blob[off] = bytes[0];
            blob[off + 1] = bytes[1];
            blob[off + 2] = bytes[2];
            blob[off + 3] = bytes[3];
            field += 1;
        }
        trait_idx += 1;
    }
    blob
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct OpinionPersonalityTraitsPtr {
    #[def(default = personality_traits_default())]
    pub f0: [u8; 180],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct TextureReplacementEntry {
    pub original_texture: i32,
    pub new_texture: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MapPathsF0Entry {
    pub id: u32,
    pub graphic: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MapPaths {
    pub f0: Vec<MapPathsF0Entry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ComboMultiplierData {
    pub experience_multiplier_num_seconds_for_attenuation: f32,
    pub experience_multiplier_param_decay_delta_per_second: f32,
    // `std::map<long,float>` (tcd_experience_orb.cpp) — key-sorted, not insertion
    // order, so a `BTreeMap` (DamageAttribute variants ascend by i32 value).
    pub combo_multiplier: BTreeMap<DamageAttribute, f32>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct HeroStatIncreaseData {
    // `std::map<EHeroExperienceStatCategory,float>` (tc_hero_experience.cpp) — key-sorted.
    pub stat_increases: BTreeMap<HeroExperienceStatCategory, f32>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AttackHistoryComboAttackComboEntry {
    // pair<EAttackHistoryType, EDamageAttribute> (tc_hero_experience.hpp:899); both
    // signed enums — a missing attribute is DAMAGE_NULL = -1 (0xFFFFFFFF).
    pub type_: i32,
    pub attribute: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AttackHistoryCombo {
    pub attack_combo: Vec<AttackHistoryComboAttackComboEntry>,
    pub multiplier: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ObjectFamilyEntry {
    pub object: DefIndex,
    pub probability: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ObjectAugmentationParticleSet {
    pub period_secs: f32,
    pub particle_effect: i32,
    pub particle_effect_to_blend_to: i32,
    pub attack_particle_effect: i32,
    pub attack_particle_effect_to_blend_to: i32,
    pub start_point: Vec<DefString>,
    pub end_point: Vec<DefString>,
}

#[derive(Debug, Clone, PartialEq, DefVariant)]
pub enum PhysicalPrimitiveInitType {
    #[def(0)]
    Tag0 {},
    #[def(1)]
    Tag1 { base_name: DefString, radius: f32 },
    #[def(2)]
    Tag2 {
        base_name: DefString,
        radius: f32,
        height: f32,
    },
    #[def(3)]
    Tag3 {},
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct PhysicalPrimitiveInit {
    pub type_: PhysicalPrimitiveInitType,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct WeaponTrailGraphicSet {
    #[def(default = -1)]
    pub attack: i32,
    #[def(default = -1)]
    pub knockdown: i32,
}

#[derive(Debug, Clone, PartialEq, DefVariant)]
pub enum AnimationEntryComponentsEntry {
    #[def(0)]
    Tag0 { flags: i32 },
    #[def(1)]
    Tag1 { transition_in_time: i32 },
    #[def(2)]
    Tag2 { delay: i32 },
    #[def(3)]
    Tag3 {
        start_handedness: i32,
        end_handedness: i32,
    },
    #[def(4)]
    Tag4 { combo_stage: i32, combo_id: i32 },
    #[def(5)]
    Tag5 { recoil_anim_index: i32 },
    #[def(6)]
    Tag6 {
        melee_flourish: bool,
        melee_knockdown: bool,
    },
    #[def(7)]
    Tag7 {
        target_offset: bool,
        target_offset_vector_x: f32,
        target_offset_vector_y: f32,
    },
    #[def(8)]
    Tag8 { next_anim_name: DefString },
    #[def(9)]
    Tag9 { animation_speed_multiplier: f32 },
    #[def(10)]
    Tag10 { mode: i32 },
    #[def(11)]
    Tag11 { next_filter: DefString },
    #[def(12)]
    Tag12 { response_anim_name: PString },
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AnimationEntry {
    pub bank_index: i32,
    // anim_name/group_name are names.bin offsets (the animation-key and group
    // strings), so they are `DefString`, not bare ints — the differ resolves them
    // to strings across index spaces. bank_index is a real symbol id.
    pub anim_name: DefString,
    pub group_name: DefString,
    pub components: Vec<AnimationEntryComponentsEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AnimationSetAnimsEntry {
    pub key: u32,
    pub entry: AnimationEntry,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AnimationSet {
    pub anims: Vec<AnimationSetAnimsEntry>,
    pub default_flags: u32,
    pub default_transition_time: u32,
    pub default_delay: u32,
    #[def(default = -1)]
    pub default_group: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SpecialEffectsStringMap {
    pub f0: VecMap<u32, i32>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct FireEffectCreationSet {
    pub containment_map: VecMap<DefString, i32>,
}

#[derive(Debug, Clone, PartialEq, DefVariant)]
pub enum ReactionMatchListElementsEntry {
    #[def(0)]
    Tag0 {
        reaction_type: i32,
        axis: i32,
        lower_bound_on_axis: f32,
        inv_interval_on_axis: f32,
        m_shift_zero: f32,
        m_shift_weight: f32,
        r_shift_zero: f32,
        r_shift_weight: f32,
    },
    #[def(1)]
    Tag1 {
        reaction_type: i32,
        axis: i32,
        centre_on_axis: f32,
        radius_on_axis: f32,
        m_shift_zero: f32,
        m_shift_weight: f32,
        r_shift_zero: f32,
        r_shift_weight: f32,
    },
    #[def(2)]
    Tag2 {
        reaction_type: i32,
        x_axis_opinion_type: i32,
        y_axis_opinion_type: i32,
        x_centre: f32,
        neg_inv_x_radius: f32,
        pos_inv_x_radius: f32,
        y_centre: f32,
        neg_inv_y_radius: f32,
        pos_inv_y_radius: f32,
    },
    #[def(3)]
    Tag3 {
        reaction_type: i32,
        x_axis_opinion_type: i32,
        y_axis_opinion_type: i32,
        x_centre: f32,
        neg_inv_x_radius: f32,
        pos_inv_x_radius: f32,
        y_centre: f32,
        neg_inv_y_radius: f32,
        pos_inv_y_radius: f32,
        scariness_shift: f32,
        agreeableness_shift: f32,
        attractiveness_shift: f32,
    },
}

/// C++ `std::list<CReactionMatch *,std::allocator<CReactionMatch *>>`.
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ReactionMatchList {
    pub elements: Vec<ReactionMatchListElementsEntry>,
}

#[derive(Debug, Clone, PartialEq, DefVariant)]
pub enum ReactionFrequencyTraitsArrayTraitsEntry {
    #[def(0)]
    Tag0 {},
    #[def(1)]
    Tag1 { min_wait: f32, wait_range: f32 },
    #[def(2)]
    Tag2 {
        low_freq_per_min: f32,
        high_low_freq_interval: f32,
        frames_per_minute: u32,
        ramp_up_time_frames: u32,
        cutoff_interval_frames: u32,
    },
    #[def(3)]
    Tag3 {
        allow_individual_repeats: bool,
        min_gap_frames: u32,
        count_recharge_per_frame: f32,
        current_available_count: f32,
        max_count: f32,
        inv_max_count: f32,
    },
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ReactionFrequencyTraitsArray {
    pub traits: [ReactionFrequencyTraitsArrayTraitsEntry; 158],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SimVoiceVoicesEntry {
    pub speech_type: ReactionSpeechType,
    pub voice_id: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SimVoice {
    pub voices: Vec<SimVoiceVoicesEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AppearanceModifierGraphicsGraphicsEntry {
    pub data: [u8; 24],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AppearanceModifierGraphics {
    pub graphics: Vec<AppearanceModifierGraphicsGraphicsEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct BrainUpdateZone {
    pub distance: i32,
    pub update_frequency: i32,
    pub interruption_frequency: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct BrainBehaviour {
    pub name: DefString,
    pub priority: i32,
    pub script_ai_priority: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct BlendedParticleEffectSet {
    pub particle1_index: i32,
    pub particle2_index: i32,
    pub particle1_value: f32,
    pub particle2_value: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct TextureMorphsMorphsEntry {
    // `CVectorMap<EHeroMorphType, CTextureMorphEntry>` pair: 4-byte key +
    // 20-byte value (tc_hero_morph_def.hpp). The value's fields
    // (MorphType/TextureLayer/ReplacingTextureIndex/BankIndex/Blend) were a raw
    // `[u8;20]` blob — typed here so the differ can see them.
    pub morph_type: i32,
    pub value_morph_type: i32,
    pub texture_layer: i32,
    pub replacing_texture_index: i32,
    pub bank_index: i32,
    pub blend: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct TextureMorphs {
    pub morphs: Vec<TextureMorphsMorphsEntry>,
    #[def(default = 28_011_707)]
    pub trailing_u32: u32,
}

/// `CSkeletalMorphs::CEntry` for `CHeroMorphDef` (NOT the creature
/// `CSkeletalMorphDef`, which is a plain `Vec<DefString>`). Verified by decoding
/// retail game.bin: the text form is
/// `SkeletalMorphs.Add(MorphType, "file.bncfg", BoneIndex, Mirror)` — the morph
/// *type* (a header constant), the bone-config *filename* (a def-string), a bone
/// index, and a mirror flag, in that order.
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SkeletalMorphsMorphsEntry {
    pub morph_type: u32,
    pub morph_name: DefString,
    pub bone_index: u16,
    pub mirror_over: bool,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SkeletalMorphs {
    pub morphs: Vec<SkeletalMorphsMorphsEntry>,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ParticleMorphsMorphsEntry {
    pub morph_type: i32,
    pub data: [u8; 36],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ParticleMorphs {
    pub morphs: Vec<ParticleMorphsMorphsEntry>,
    #[def(default = 28_011_708)]
    pub trailing_u32: u32,
}

/// C++ `CExplosionRing`. The serialized wire order is
/// (NumExplosions, RingRadius, AngleOffset, Seconds) — verified against
/// retail — but the text ctor is `CExplosionRing(radius, count, …)`, so its
/// first two args map SWAPPED (see the arm in `apply_struct_from_expr`).
#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ExplosionRing {
    pub num_explosions: i32,
    pub ring_radius: f32,
    pub angle_offset: f32,
    pub seconds_before_spawn_new_ring: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct SoundBankEntry {
    pub id: u32,
    pub filename: DefString,
    pub symbol_filename: DefString,
    pub lip_sync_bank_handle: DefString,
    pub dvd: bool,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct AtmosBankEntry {
    pub id: u32,
    pub filename: DefString,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MusicEntry {
    pub id: u32,
    pub filename: DefString,
    pub gain: f32,
    pub cache_to_hd: bool,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MusicSetEntry {
    pub normal_music_id: i32,
    pub danger_music_id: i32,
    pub loop_count: i32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ReverbEnvironmentEntry {
    pub param0: f32,
    pub param1: f32,
    pub param2: f32,
    pub param3: f32,
    pub param4: f32,
    pub param5: f32,
    pub param6: f32,
    pub param7: f32,
    pub param8: f32,
    pub param9: f32,
    pub param10: f32,
    pub param11: f32,
    pub param12: f32,
    pub param13: f32,
    pub param14: f32,
    pub param15: f32,
    pub param16: f32,
    pub param17: f32,
    pub param18: f32,
    pub param19: f32,
    pub param20: f32,
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct ActionInputControl {
    pub game_action: GameAction,
    pub controller_type: ControllerType,
    pub keyboard_key: InputKey,
    pub xbox_button: XboxControllerButton,
    pub mouse_button: MouseButtonControl,
    pub control_direction: [f32; 2],
}

#[derive(Debug, Clone, PartialEq, WireStruct)]
pub struct MapPathEntry {
    pub id: u32,
    pub graphic: i32,
}
