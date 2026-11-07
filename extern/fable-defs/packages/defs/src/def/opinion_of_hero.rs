use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct OpinionOfHeroDef {
    #[def("ThreatenGift")]
    pub threaten_gift: DefIndex,
    #[def("WillAttackCriminals")]
    pub will_attack_criminals: bool,
    #[def("Personality")]
    pub personality: DefIndex,
    #[def("ReactionMask")]
    pub reaction_mask: DefIndex,
    #[def("DeedMask")]
    pub deed_mask: DefIndex,
}
