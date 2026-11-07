use crate::DefStruct;
use crate::enums::ObjectAugmentationType;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct AugmentationDef {
    #[def("Name")]
    pub name: u32,
    #[def("Type")]
    pub type_: ObjectAugmentationType,
    #[def("InventoryGraphic")]
    pub inventory_graphic: u32,
    #[def("DamageMultiplier", default = 1.0)]
    pub damage_multiplier: f32,
    #[def("ExperienceMultiplier", default = 1.0)]
    pub experience_multiplier: f32,
    #[def("FrequencyToAddHealthInSeconds")]
    pub frequency_to_add_health_in_seconds: f32,
    #[def("HealthToAddPerIncrease")]
    pub health_to_add_per_increase: i32,
    #[def("FrequencyToAddStaminaInSeconds")]
    pub frequency_to_add_stamina_in_seconds: f32,
    #[def("StaminaToAddPerIncrease")]
    pub stamina_to_add_per_increase: i32,
}
