use crate::DefStruct;
use crate::def::values::SimVoice;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct SimVoicesDef {
    #[def("Entry")]
    pub entry: SimVoice,
}
