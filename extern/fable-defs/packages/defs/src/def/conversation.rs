use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct Conversation {
    #[def("Lines")]
    pub lines: i32,
    #[def("Speaker")]
    pub speaker: Vec<String>,
    #[def("Dialogue")]
    pub dialogue: Vec<String>,
    #[def("Animation")]
    pub animation: Vec<String>,
    #[def("AnimLoop")]
    pub anim_loop: Vec<String>,
}
