use crate::DefStruct;
use crate::enums::{MessageEventType, ReactionSpeechType};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct MessageEventDef {
    #[def("MaxSeenRadius")]
    pub max_seen_radius: i32,
    #[def("MaxHeardRadius")]
    pub max_heard_radius: i32,
    #[def("Lifespan")]
    pub lifespan: i32,
    #[def("Type")]
    pub type_: MessageEventType,
    #[def("ReactionSpeechType")]
    pub reaction_speech_type: ReactionSpeechType,
}
