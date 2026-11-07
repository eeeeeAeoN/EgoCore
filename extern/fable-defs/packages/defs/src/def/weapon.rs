use crate::DefStruct;
use crate::def::{
    enums::DamageAttribute, enums::IdleStateGroup, enums::ObjectAugmentationType,
    enums::WeaponClass, enums::WeaponType, values::FloatRange,
    values::ObjectAugmentationParticleSet, values::WeaponTrailGraphicSet, wire::DefIndex,
    wire::VecMap,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WeaponDef {
    #[def("Type")]
    pub type_: WeaponType,
    #[def("Class")]
    pub class: WeaponClass,
    #[def("Property")]
    pub property: DefIndex,
    #[def("AnimationGroup", default = String::from("SWORD"))]
    pub animation_group: String,
    // Transfer<long>, retail default -1 (none); a def reference — bows set it to
    // OBJECT_QUIVER (others set literal 0).
    #[def("SheatheObject", default = DefIndex(-1))]
    pub sheathe_object: DefIndex,
    #[def("SheatheObjectAlwaysVisible")]
    pub sheathe_object_always_visible: bool,
    #[def("AnimationSpeedVal")]
    pub animation_speed_val: f32,
    #[def("Damage")]
    pub damage: f32,
    #[def("DamageType", default = DamageAttribute::MELEE)]
    pub damage_type: DamageAttribute,
    #[def("Explosion")]
    pub explosion: DefIndex,
    #[def("FlourishParticles")]
    pub flourish_particles: ObjectAugmentationParticleSet,
    #[def("FlourishSpecialWeaponTrail")]
    pub flourish_special_weapon_trail: WeaponTrailGraphicSet,
    #[def("WeaponTrails")]
    pub weapon_trails: VecMap<WeaponTrailGraphicSet, ObjectAugmentationType>,
    #[def("UseAnalogueLoading")]
    pub use_analogue_loading: bool,
    // Transfer<long>, retail default -1; text sets a `CARRY_SLOT_*` value, each of
    // which is a def in carry_slots.def, so this is a def reference (default none).
    #[def("MissileLoadCarrySlot", default = DefIndex(-1))]
    pub missile_load_carry_slot: DefIndex,
    #[def("ProjectileWeaponHelperNameTip", default = String::from("weapon_pos_a"))]
    pub projectile_weapon_helper_name_tip: String,
    #[def("ProjectileWeaponHelperNameBase", default = String::from("weapon_pos_b"))]
    pub projectile_weapon_helper_name_base: String,
    #[def("Shot")]
    pub shot: DefIndex,
    #[def("Ammo")]
    pub ammo: DefIndex,
    #[def("TargetingRange")]
    pub targeting_range: DefIndex,
    #[def("TargetingFOV")]
    pub targeting_fov: f32,
    #[def("ArrowTrails")]
    pub arrow_trails: VecMap<i32, IdleStateGroup>,
    #[def("ReloadSpeedRange")]
    pub reload_speed_range: FloatRange,
    #[def("ProjectileWeaponAutoReload")]
    pub projectile_weapon_auto_reload: bool,
    #[def("ProjectileWeaponStartLoaded")]
    pub projectile_weapon_start_loaded: bool,
    #[def("UseExtendedProjectileTrails")]
    pub use_extended_projectile_trails: bool,
    #[def("NextWeaponForBoss")]
    pub next_weapon_for_boss: DefIndex,
    #[def("MeleeTargetingArc", default = 60.0)]
    pub melee_targeting_arc: f32,
    #[def("NeedsLoading", default = true)]
    pub needs_loading: bool,
    #[def("UnsheatheToCombatAnim")]
    pub unsheathe_to_combat_anim: String,
    #[def("UnsheatheToNormalAnim")]
    pub unsheathe_to_normal_anim: String,
    #[def("SheatheFromCombatAnim")]
    pub sheathe_from_combat_anim: String,
    #[def("SheatheFromNormalAnim")]
    pub sheathe_from_normal_anim: String,
}
