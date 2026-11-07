use crate::DefStruct;
use crate::enums::HeroTitle;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct HeroTitleDef {
    #[def("Title")]
    pub title: HeroTitle,
    #[def("HeroTitleGuiTag")]
    pub hero_title_gui_tag: i32,
    #[def("HeroTitleGreetToHeroTag")]
    pub hero_title_greet_to_hero_tag: i32,
    #[def("HeroTitleCommentAtHeroTag")]
    pub hero_title_comment_at_hero_tag: i32,
    #[def("HeroTitleCommentToSelfTag")]
    pub hero_title_comment_to_self_tag: i32,
    #[def("RequiredRenownLevel")]
    pub required_renown_level: i32,
    #[def("RequiredStrengthExpLevel")]
    pub required_strength_exp_level: i32,
    #[def("RequiredSkillExpLevel")]
    pub required_skill_exp_level: i32,
    #[def("RequiredWillExpLevel")]
    pub required_will_exp_level: i32,
    #[def("IsBuyable")]
    pub is_buyable: bool,
    #[def("IsAutomatic", default = true)]
    pub is_automatic: bool,
}
