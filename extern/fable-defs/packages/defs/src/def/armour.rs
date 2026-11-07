use crate::DefStruct;
use crate::wire::DefIndex;
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ArmourDef {
    #[def("AugmentationResponse")]
    pub augmentation_response: BTreeMap<i32, f32>,
    #[def("DamageTypeResponse")]
    pub damage_type_response: BTreeMap<i32, f32>,
    #[def("ArmourThresholds")]
    pub armour_thresholds: BTreeMap<i32, f32>,
    #[def("AllHitsKnockdown")]
    pub all_hits_knockdown: bool,
    #[def("AllHitsNegated")]
    pub all_hits_negated: bool,
    #[def("AllHitsCauseRecoil")]
    pub all_hits_cause_recoil: bool,
    #[def("DamageMaterial")]
    pub damage_material: DefIndex,
    #[def("BlockMaterial")]
    pub block_material: DefIndex,
}
