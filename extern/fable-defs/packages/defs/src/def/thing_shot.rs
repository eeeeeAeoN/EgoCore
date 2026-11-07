use crate::DefStruct;
use crate::def::{enums::DamageAttribute, values::FloatRange, wire::DefIndex, wire::DefString};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ThingShotDef {
    #[def("Speed")]
    pub speed: f32,
    #[def("PrimaryEffect")]
    pub primary_effect: i32,
    #[def("PrimaryEffectSound")]
    pub primary_effect_sound: i32,
    #[def("AttachedContinuousEffect")]
    pub attached_continuous_effect: i32,
    #[def("AttachedContinuousSound")]
    pub attached_continuous_sound: i32,
    #[def("UseEndTimeLineOnContinousEffect")]
    pub use_end_time_line_on_continous_effect: bool,
    #[def("HitThingDamage")]
    pub hit_thing_damage: f32,
    #[def("HitThingShieldDamage")]
    pub hit_thing_shield_damage: f32,
    #[def("CauseDamage", default = true)]
    pub cause_damage: bool,
    #[def("ShotDefinitionClassName")]
    pub shot_definition_class_name: DefString,
    #[def("ShotOffsetAngleRange")]
    pub shot_offset_angle_range: FloatRange,
    #[def("ShotOffsetRadiusRange")]
    pub shot_offset_radius_range: FloatRange,
    #[def("ExplosionDef", default = DefIndex(-1))]
    pub explosion_def: DefIndex,
    #[def("GroundExplosionDef", default = DefIndex(-1))]
    pub ground_explosion_def: DefIndex,
    #[def("ObjectDef")]
    pub object_def: DefIndex,
    #[def("LeavesObjectBehind")]
    pub leaves_object_behind: bool,
    #[def("IsHittableByMelee")]
    pub is_hittable_by_melee: bool,
    #[def("IsDeflectedOnHitByMelee")]
    pub is_deflected_on_hit_by_melee: bool,
    #[def("MeleeHitDeflectionDamageMultiplier", default = 1.0)]
    pub melee_hit_deflection_damage_multiplier: f32,
    #[def("FaceMovementDirection", default = true)]
    pub face_movement_direction: bool,
    #[def("AllowDeflections")]
    pub allow_deflections: bool,
    #[def("BounceOnGround")]
    pub bounce_on_ground: bool,
    #[def("Bounciness", default = 1.0)]
    pub bounciness: f32,
    #[def("RotateOnDeflection", default = true)]
    pub rotate_on_deflection: bool,
    #[def("IsBlockable", default = true)]
    pub is_blockable: bool,
    #[def("DamageType", default = DamageAttribute::PROJECTILE)]
    pub damage_type: DamageAttribute,
    #[def("NumSubShots")]
    pub num_sub_shots: i32,
    #[def("SubShotType", default = DefIndex(-1))]
    pub sub_shot_type: DefIndex,
    #[def("SubShotCreationTime", default = 1.0)]
    pub sub_shot_creation_time: f32,
    #[def("SubShotMaxAngle", default = 0.125)]
    pub sub_shot_max_angle: f32,
    #[def("RemoveMainShotOnSubShotCreation", default = true)]
    pub remove_main_shot_on_sub_shot_creation: bool,
    #[def("DoesExplosionDamageCaster", default = true)]
    pub does_explosion_damage_caster: bool,
    #[def("CanHitRollingTargets")]
    pub can_hit_rolling_targets: bool,
    #[def("PreventSlidingOnCollision")]
    pub prevent_sliding_on_collision: bool,
}
