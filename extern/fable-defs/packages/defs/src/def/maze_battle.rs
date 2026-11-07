use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MazeBattleDef {
    #[def("ShieldUpTime")]
    pub shield_up_time: f32,
    #[def("LightningDuration")]
    pub lightning_duration: f32,
    #[def("MinThrowShotRange")]
    pub min_throw_shot_range: f32,
    #[def("EnflameRange")]
    pub enflame_range: f32,
    #[def("LightningRange")]
    pub lightning_range: f32,
    #[def("TutorialNumGetHits")]
    pub tutorial_num_get_hits: i32,
    #[def("MinTimeBetweenHeals", default = 5.0)]
    pub min_time_between_heals: f32,
    #[def("HealSpellLevel", default = 1)]
    pub heal_spell_level: i32,
    #[def("TutorialTeleportTime", default = 9.0)]
    pub tutorial_teleport_time: f32,
}
