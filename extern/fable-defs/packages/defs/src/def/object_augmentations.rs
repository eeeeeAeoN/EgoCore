use crate::DefStruct;
use crate::def::{
    enums::ObjectAugmentationType,
    values::ObjectAugmentationParticleSet,
    wire::{DefString, VecMap},
};
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ObjectAugmentationsDef {
    #[def("NumberOfSlots")]
    pub number_of_slots: i32,
    #[def("MaximumNumberOfSlots")]
    pub maximum_number_of_slots: i32,
    #[def("AugmentationParticles")]
    pub augmentation_particles: VecMap<ObjectAugmentationType, ObjectAugmentationParticleSet>,
    #[def("EmptySlotName")]
    pub empty_slot_name: u32,
    #[def("InitialAugmentations")]
    pub initial_augmentations: Vec<DefIndex>,
    #[def("SlotUpgradeCosts")]
    pub slot_upgrade_costs: Vec<i32>,
    #[def("InitSounds")]
    pub init_sounds: VecMap<ObjectAugmentationType, DefString>,
    #[def("LoopingSounds")]
    pub looping_sounds: VecMap<ObjectAugmentationType, DefString>,
    #[def("FireProjectileTrailEffect")]
    pub fire_projectile_trail_effect: i32,
    #[def("LightningProjectileTrailEffect")]
    pub lightning_projectile_trail_effect: i32,
    #[def("FireAndLightningProjectileTrailEffect")]
    pub fire_and_lightning_projectile_trail_effect: i32,
    #[def("FireProjectileOnHitEffect")]
    pub fire_projectile_on_hit_effect: i32,
    #[def("LightningProjectileOnHitEffect")]
    pub lightning_projectile_on_hit_effect: i32,
    #[def("FireAndLightningProjectileOnHitEffect")]
    pub fire_and_lightning_projectile_on_hit_effect: i32,
    #[def("WeaponTrailAttachmentPoint")]
    pub weapon_trail_attachment_point: String,
}
