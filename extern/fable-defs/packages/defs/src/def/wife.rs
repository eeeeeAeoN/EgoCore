use crate::DefStruct;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct WifeDef {
    // A gold amount (Transfer<long>; text values are plain numbers), not a ref.
    #[def("Dowry")]
    pub dowry: i32,
}
