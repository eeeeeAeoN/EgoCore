use crate::DefStruct;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct CreatureGeneratorDef {
    #[def("RandomEffects")]
    pub random_effects: Vec<i32>,
    #[def("RandomSoundCriteria")]
    pub random_sound_criteria: DefString,
    #[def("SecondsBetweenEffects")]
    pub seconds_between_effects: i32,
    #[def("SecondsBetweenSoundEffects")]
    pub seconds_between_sound_effects: i32,
}
