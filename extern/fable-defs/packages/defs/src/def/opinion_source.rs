use crate::DefStruct;
use crate::def::{
    enums::{Opinion, OpinionDeedType},
    wire::DefIndex,
};
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionSourceDef {
    #[def("ProducedOpinion")]
    pub produced_opinion: BTreeMap<Opinion, f32>,
    #[def("AlwaysAwareOf")]
    pub always_aware_of: bool,
    #[def("UnknownThreatSource")]
    pub unknown_threat_source: bool,
    #[def("ReactNonstop")]
    pub react_nonstop: bool,
    #[def("ReactToSelfPermitted")]
    pub react_to_self_permitted: bool,
    #[def("ReactionFlagDefault")]
    pub reaction_flag_default: bool,
    #[def("ReactionFlag")]
    pub reaction_flag: BTreeMap<OpinionDeedType, bool>,
    #[def("UseNavLineOfSight", default = true)]
    pub use_nav_line_of_sight: bool,
    #[def("WitnessesWillNotAttack")]
    pub witnesses_will_not_attack: bool,
    #[def("WitnessesWillOverrideFactionEnemy")]
    pub witnesses_will_override_faction_enemy: bool,
    #[def("BypassesPersonality")]
    pub bypasses_personality: bool,
    #[def("RelativeEffect")]
    pub relative_effect: bool,
    #[def("RestrictToFactionAndAllies")]
    pub restrict_to_faction_and_allies: DefIndex,
    #[def("RestrictToFactionIsReversed")]
    pub restrict_to_faction_is_reversed: bool,
    #[def("BinaryReaction")]
    pub binary_reaction: bool,
    #[def("BinaryReaction")]
    pub binary_reaction2: bool,
    #[def("BinaryReaction")]
    pub binary_reaction3: bool,
    #[def("BinaryReaction")]
    pub binary_reaction4: bool,
    #[def("BinaryReaction")]
    pub binary_reaction5: bool,
    #[def("BinaryReaction")]
    pub binary_reaction6: bool,
    #[def("BinaryReaction")]
    pub binary_reaction7: bool,
    #[def("BinaryReaction")]
    pub binary_reaction8: bool,
    #[def("BinaryReaction")]
    pub binary_reaction9: bool,
    #[def("BinaryReaction")]
    pub binary_reaction10: bool,
    #[def("BinaryReaction")]
    pub binary_reaction11: bool,
    #[def("BinaryReaction")]
    pub binary_reaction12: bool,
    #[def("BinaryReaction")]
    pub binary_reaction13: bool,
    #[def("BinaryReaction")]
    pub binary_reaction14: bool,
    #[def("BinaryReaction")]
    pub binary_reaction15: bool,
    #[def("BinaryReaction")]
    pub binary_reaction16: bool,
    #[def("BinaryReaction")]
    pub binary_reaction17: bool,
    #[def("BinaryReaction")]
    pub binary_reaction18: bool,
    #[def("BinaryReaction")]
    pub binary_reaction19: bool,
    #[def("BinaryReaction")]
    pub binary_reaction20: bool,
    #[def("BinaryReaction")]
    pub binary_reaction21: bool,
    #[def("BinaryReaction")]
    pub binary_reaction22: bool,
    #[def("BinaryReaction")]
    pub binary_reaction23: bool,
    #[def("BinaryReaction")]
    pub binary_reaction24: bool,
    #[def("BinaryReaction")]
    pub binary_reaction25: bool,
    #[def("BinaryReaction")]
    pub binary_reaction26: bool,
    #[def("BinaryReaction")]
    pub binary_reaction27: bool,
    #[def("BinaryReaction")]
    pub binary_reaction28: bool,
    #[def("BinaryReaction")]
    pub binary_reaction29: bool,
    #[def("BinaryReaction")]
    pub binary_reaction30: bool,
    #[def("BinaryReaction")]
    pub binary_reaction31: bool,
    #[def("BinaryReaction")]
    pub binary_reaction32: bool,
    #[def("BinaryReaction")]
    pub binary_reaction33: bool,
    #[def("BinaryReaction")]
    pub binary_reaction34: bool,
    #[def("BinaryReaction")]
    pub binary_reaction35: bool,
    #[def("BinaryReaction")]
    pub binary_reaction36: bool,
    #[def("BinaryReaction")]
    pub binary_reaction37: bool,
    #[def("BinaryReaction")]
    pub binary_reaction38: bool,
    #[def("BinaryReaction")]
    pub binary_reaction39: bool,
    #[def("BinaryReaction")]
    pub binary_reaction40: bool,
    #[def("BinaryReaction")]
    pub binary_reaction41: bool,
    #[def("BinaryReaction")]
    pub binary_reaction42: bool,
    #[def("BinaryReaction")]
    pub binary_reaction43: bool,
    #[def("BinaryReaction")]
    pub binary_reaction44: bool,
    #[def("BinaryReaction")]
    pub binary_reaction45: bool,
    #[def("BinaryReaction")]
    pub binary_reaction46: bool,
    #[def("BinaryReaction")]
    pub binary_reaction47: bool,
    #[def("BinaryReaction")]
    pub binary_reaction48: bool,
    #[def("BinaryReaction")]
    pub binary_reaction49: bool,
    #[def("BinaryReaction")]
    pub binary_reaction50: bool,
    #[def("BinaryReaction")]
    pub binary_reaction51: bool,
    #[def("BinaryReaction")]
    pub binary_reaction52: bool,
    #[def("BinaryReaction")]
    pub binary_reaction53: bool,
    #[def("BinaryReaction")]
    pub binary_reaction54: bool,
    #[def("BinaryReaction")]
    pub binary_reaction55: bool,
    #[def("BinaryReaction")]
    pub binary_reaction56: bool,
    #[def("BinaryReaction")]
    pub binary_reaction57: bool,
    #[def("BinaryReaction")]
    pub binary_reaction58: bool,
    #[def("BinaryReaction")]
    pub binary_reaction59: bool,
    #[def("BinaryReaction")]
    pub binary_reaction60: bool,
    #[def("BinaryReaction")]
    pub binary_reaction61: bool,
    #[def("BinaryReaction")]
    pub binary_reaction62: bool,
    #[def("BinaryReaction")]
    pub binary_reaction63: bool,
    #[def("BinaryReaction")]
    pub binary_reaction64: bool,
    #[def("BinaryReaction")]
    pub binary_reaction65: bool,
    #[def("BinaryReaction")]
    pub binary_reaction66: bool,
    #[def("BinaryReaction")]
    pub binary_reaction67: bool,
    #[def("BinaryReaction")]
    pub binary_reaction68: bool,
    #[def("BinaryReaction")]
    pub binary_reaction69: bool,
    #[def("BinaryReaction")]
    pub binary_reaction70: bool,
    #[def("BinaryReaction")]
    pub binary_reaction71: bool,
    #[def("BinaryReaction")]
    pub binary_reaction72: bool,
    #[def("BinaryReaction")]
    pub binary_reaction73: bool,
    #[def("BinaryReaction")]
    pub binary_reaction74: bool,
    #[def("BinaryReaction")]
    pub binary_reaction75: bool,
    #[def("BinaryReaction")]
    pub binary_reaction76: bool,
    #[def("BinaryReaction")]
    pub binary_reaction77: bool,
    #[def("BinaryReaction")]
    pub binary_reaction78: bool,
    #[def("BinaryReaction")]
    pub binary_reaction79: bool,
    #[def("BinaryOpinion")]
    pub binary_opinion: f32,
    #[def("BinaryOpinion")]
    pub binary_opinion2: f32,
    #[def("BinaryOpinion")]
    pub binary_opinion3: f32,
    #[def("BinaryOpinion")]
    pub binary_opinion4: f32,
    #[def("BinaryOpinion")]
    pub binary_opinion5: f32,
}

impl OpinionSourceDef {
    pub fn derive_binary_reactions(&mut self) {
        let flags = [
            &mut self.binary_reaction,
            &mut self.binary_reaction2,
            &mut self.binary_reaction3,
            &mut self.binary_reaction4,
            &mut self.binary_reaction5,
            &mut self.binary_reaction6,
            &mut self.binary_reaction7,
            &mut self.binary_reaction8,
            &mut self.binary_reaction9,
            &mut self.binary_reaction10,
            &mut self.binary_reaction11,
            &mut self.binary_reaction12,
            &mut self.binary_reaction13,
            &mut self.binary_reaction14,
            &mut self.binary_reaction15,
            &mut self.binary_reaction16,
            &mut self.binary_reaction17,
            &mut self.binary_reaction18,
            &mut self.binary_reaction19,
            &mut self.binary_reaction20,
            &mut self.binary_reaction21,
            &mut self.binary_reaction22,
            &mut self.binary_reaction23,
            &mut self.binary_reaction24,
            &mut self.binary_reaction25,
            &mut self.binary_reaction26,
            &mut self.binary_reaction27,
            &mut self.binary_reaction28,
            &mut self.binary_reaction29,
            &mut self.binary_reaction30,
            &mut self.binary_reaction31,
            &mut self.binary_reaction32,
            &mut self.binary_reaction33,
            &mut self.binary_reaction34,
            &mut self.binary_reaction35,
            &mut self.binary_reaction36,
            &mut self.binary_reaction37,
            &mut self.binary_reaction38,
            &mut self.binary_reaction39,
            &mut self.binary_reaction40,
            &mut self.binary_reaction41,
            &mut self.binary_reaction42,
            &mut self.binary_reaction43,
            &mut self.binary_reaction44,
            &mut self.binary_reaction45,
            &mut self.binary_reaction46,
            &mut self.binary_reaction47,
            &mut self.binary_reaction48,
            &mut self.binary_reaction49,
            &mut self.binary_reaction50,
            &mut self.binary_reaction51,
            &mut self.binary_reaction52,
            &mut self.binary_reaction53,
            &mut self.binary_reaction54,
            &mut self.binary_reaction55,
            &mut self.binary_reaction56,
            &mut self.binary_reaction57,
            &mut self.binary_reaction58,
            &mut self.binary_reaction59,
            &mut self.binary_reaction60,
            &mut self.binary_reaction61,
            &mut self.binary_reaction62,
            &mut self.binary_reaction63,
            &mut self.binary_reaction64,
            &mut self.binary_reaction65,
            &mut self.binary_reaction66,
            &mut self.binary_reaction67,
            &mut self.binary_reaction68,
            &mut self.binary_reaction69,
            &mut self.binary_reaction70,
            &mut self.binary_reaction71,
            &mut self.binary_reaction72,
            &mut self.binary_reaction73,
            &mut self.binary_reaction74,
            &mut self.binary_reaction75,
            &mut self.binary_reaction76,
            &mut self.binary_reaction77,
            &mut self.binary_reaction78,
            &mut self.binary_reaction79,
        ];
        for (i, flag) in flags.into_iter().enumerate() {
            *flag = if i >= 18 {
                self.reaction_flag
                    .get(&OpinionDeedType(i as i32))
                    .copied()
                    .unwrap_or(self.reaction_flag_default)
            } else {
                false
            };
        }
    }

    pub fn derive_binary_opinions(&mut self) {
        let opinions = [
            &mut self.binary_opinion,
            &mut self.binary_opinion2,
            &mut self.binary_opinion3,
            &mut self.binary_opinion4,
            &mut self.binary_opinion5,
        ];
        for (i, op) in opinions.into_iter().enumerate() {
            *op = self
                .produced_opinion
                .get(&Opinion(i as i32))
                .copied()
                .unwrap_or(0.0);
        }
    }
}
