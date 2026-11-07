use crate::DefStruct;
use crate::enums::{ExpressionInventoryType, GameEventType, OpinionDeedType};
use crate::wire::{DefIndex, DefString};

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExpressionDef {
    #[def("Animation")]
    pub animation: DefString,
    #[def("AnimationInto")]
    pub animation_into: DefString,
    #[def("AnimationLoop")]
    pub animation_loop: DefString,
    #[def("AnimationOutOf")]
    pub animation_out_of: DefString,
    #[def("AnimationToOther")]
    pub animation_to_other: DefString,
    #[def("AnimationToOtherInto")]
    pub animation_to_other_into: DefString,
    #[def("AnimationToOtherLoop")]
    pub animation_to_other_loop: DefString,
    #[def("AnimationToOtherOutOf")]
    pub animation_to_other_out_of: DefString,
    #[def("SpriteDuration")]
    pub sprite_duration: f32,
    #[def("SpriteGraphic")]
    pub sprite_graphic: i32,
    #[def("SpriteOverrideText")]
    pub sprite_override_text: DefString,
    #[def("SoundCriteria")]
    pub sound_criteria: DefString,
    #[def("LoopSoundCriteria")]
    pub loop_sound_criteria: DefString,
    #[def("DummyObject")]
    pub dummy_object: DefIndex,
    #[def("GameEventType")]
    pub game_event_type: GameEventType,
    #[def("ScriptName")]
    pub script_name: DefString,
    #[def("ScriptLoadResources", default = true)]
    pub script_load_resources: bool,
    #[def("DeactivateScriptAtEnd", default = true)]
    pub deactivate_script_at_end: bool,
    #[def("PerformAfterScript")]
    pub perform_after_script: bool,
    #[def("OpinionDeedType")]
    pub opinion_deed_type: OpinionDeedType,
    #[def("Extendable")]
    pub extendable: bool,
    #[def("SheatheWeapons")]
    pub sheathe_weapons: bool,
    #[def("ReplaceHeldItemWhenDone", default = true)]
    pub replace_held_item_when_done: bool,
    #[def("TextTag")]
    pub text_tag: u32,
    #[def("ExpressionType")]
    pub expression_type: ExpressionInventoryType,
}
