use crate::DefStruct;
use crate::enums::{ClothingSuitPart, HeroAttachableAppearanceModifierType};
use crate::values::AppearanceModifierGraphics;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AppearanceModifierDef {
    #[def("HideMaterials")]
    pub hide_materials: Vec<String>,
    #[def("Graphics")]
    pub graphics: AppearanceModifierGraphics,
    #[def("CoversBodyAreaFlags")]
    pub covers_body_area_flags: i32,
    #[def("IsRemoveable", default = true)]
    pub is_removeable: bool,
    #[def("HeroSuit")]
    pub hero_suit: DefIndex,
    #[def("SuitPart")]
    pub suit_part: ClothingSuitPart,
    #[def("AppearanceType")]
    pub appearance_type: HeroAttachableAppearanceModifierType,
    #[def("Scariness")]
    pub scariness: f32,
    #[def("Attractiveness")]
    pub attractiveness: f32,
    #[def("Goodstrength")]
    pub goodstrength: f32,
    #[def("SoundRadiusMultiplier", default = 1.0)]
    pub sound_radius_multiplier: f32,
    #[def("VisibilityMultiplier", default = 1.0)]
    pub visibility_multiplier: f32,
    #[def("ArmourDefIndex", default = DefIndex(-1))]
    pub armour_def_index: DefIndex,
    #[def("OveralProtectionWeighting")]
    pub overal_protection_weighting: f32,
}
