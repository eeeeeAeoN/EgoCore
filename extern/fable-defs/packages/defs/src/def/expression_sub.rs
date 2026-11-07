use crate::DefStruct;
use crate::wire::DefIndex;

#[derive(Debug, Clone, PartialEq, DefStruct)]
pub struct ExpressionSubDef {
    #[def("ExpressionDef")]
    pub expression_def: DefIndex,
}
