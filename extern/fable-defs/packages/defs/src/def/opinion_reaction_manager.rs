use crate::DefStruct;
use crate::def::{
    BlurredAttitudesDef,
    enums::{OpinionAttitudeType, OpinionReactionType, OpinionTargetingConditionType},
    values::{ReactionFrequencyTraitsArray, ReactionMatchList},
    wire::VecMap,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionReactionManagerDef {
    #[def("AttitudeBlur")]
    pub attitude_blur: Vec<BlurredAttitudesDef>,
    #[def("AttitudeCondition")]
    pub attitude_condition: VecMap<OpinionAttitudeType, OpinionReactionType>,
    #[def("TargetingCondition")]
    pub targeting_condition: VecMap<OpinionTargetingConditionType, OpinionReactionType>,
    #[def("PreReactionDelay")]
    pub pre_reaction_delay: VecMap<OpinionReactionType, f32>,
    #[def("ToleranceToBeingHit")]
    pub tolerance_to_being_hit: VecMap<OpinionAttitudeType, f32>,
    #[def("BlockFurtherReactions")]
    pub block_further_reactions: VecMap<OpinionReactionType, bool>,
    #[def("AllowSpeechOnNonPureAISpeaker")]
    pub allow_speech_on_non_pure_ai_speaker: VecMap<OpinionReactionType, bool>,
    #[def("AllowWhileCarrying")]
    pub allow_while_carrying: VecMap<OpinionReactionType, bool>,
    #[def("AllowWhileFollowingPlayer")]
    pub allow_while_following_player: VecMap<OpinionReactionType, bool>,
    #[def("Matches")]
    pub matches: ReactionMatchList,
    #[def("Frequency")]
    pub frequency: ReactionFrequencyTraitsArray,
}
