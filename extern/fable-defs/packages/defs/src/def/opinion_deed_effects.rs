use crate::DefStruct;
use crate::def::{
    OpinionDeedReactionDef,
    enums::{CrimeSeverity, DialogueLayer, TutorialCategory},
    values::OpinionTransientOffsetList,
};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionDeedEffectsDef {
    #[def("KnowledgeLifeInSeconds")]
    pub knowledge_life_in_seconds: i32,
    #[def("MinimumSecondsBetweenPostings")]
    pub minimum_seconds_between_postings: i32,
    #[def("MinimumSecondsBetweenEffectsForIndividual")]
    pub minimum_seconds_between_effects_for_individual: i32,
    #[def("PreReactionDelayIgnoreOnDelay")]
    pub pre_reaction_delay_ignore_on_delay: f32,
    #[def("Effects")]
    pub effects: OpinionTransientOffsetList,
    #[def("ReactionPriority")]
    pub reaction_priority: i32,
    #[def("Reaction")]
    pub reaction: Vec<OpinionDeedReactionDef>,
    #[def("ReactionDialogueLayer", default = DialogueLayer::MIDGROUND)]
    pub reaction_dialogue_layer: DialogueLayer,
    #[def("WarningText")]
    pub warning_text: Vec<String>,
    #[def("CrimeSeverity")]
    pub crime_severity: CrimeSeverity,
    #[def("CrimeFineAmount")]
    pub crime_fine_amount: i32,
    #[def("Tutorial")]
    pub tutorial: TutorialCategory,
    #[def("CanBeHeard")]
    pub can_be_heard: bool,
    #[def("ReactionRestrictToTarget")]
    pub reaction_restrict_to_target: bool,
    #[def("AllowSimultaneousReactionDialogue")]
    pub allow_simultaneous_reaction_dialogue: bool,
    #[def("CanApologise")]
    pub can_apologise: bool,
    #[def("MustNotRunWhenWarned")]
    pub must_not_run_when_warned: bool,
    #[def("OnlyGetFinedForOneInstance")]
    pub only_get_fined_for_one_instance: bool,
    #[def("IsFlirting")]
    pub is_flirting: bool,
    #[def("OnReactSetGreeted")]
    pub on_react_set_greeted: bool,
    #[def("AllowSpeechOnNonPureAISpeaker")]
    pub allow_speech_on_non_pure_ai_speaker: bool,
    // Retail order (verified by decoding retail game.bin): AllowIndirectReact… then
    // BlockWhileFollowingPlayer. `AllowWhileCarrying` is NOT a control of this def
    // (it's the OPINION_REACTION_MANAGER indexed VecMap).
    #[def("AllowIndirectReactWhileCarrying")]
    pub allow_indirect_react_while_carrying: bool,
    #[def("BlockWhileFollowingPlayer")]
    pub block_while_following_player: bool,
}
