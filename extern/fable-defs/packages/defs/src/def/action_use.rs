use crate::DefStruct;
use crate::enums::TutorialCategory;
use crate::wire::DefString;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ActionUseDef {
    #[def("AnimationGroup")]
    pub animation_group: DefString,
    #[def("SnapToActionPoint")]
    pub snap_to_action_point: bool,
    #[def("IgnoreActionPoints", default = true)]
    pub ignore_action_points: bool,
    #[def("TutorialCategory")]
    pub tutorial_category: TutorialCategory,
}
