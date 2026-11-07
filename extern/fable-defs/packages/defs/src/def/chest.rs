use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ChestDef {
    #[def("OpenParticleEffect")]
    pub open_particle_effect: i32,
    #[def("PersistOnOpening")]
    pub persist_on_opening: bool,
    #[def("DisplayMessageOnEmpty", default = true)]
    pub display_message_on_empty: bool,
    #[def("OpenAnimationForCreature")]
    pub open_animation_for_creature: String,
    #[def("OpenerObject")]
    pub opener_object: DefIndex,
    #[def("OpenersRequired")]
    pub openers_required: i32,
}
