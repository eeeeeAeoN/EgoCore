//!
//! Its `Transfer` body is empty: no fields are serialized (verified against the
//! spec and byte-exact against the compiled bins).

use crate::binary::{ParseControlError, SerializeControlError};
use crate::visit::{DefDefault, FieldVisitor, VisitFields};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct EngineThemeGroupDef;

impl DefDefault for EngineThemeGroupDef {
    fn def_default() -> Self {
        Self
    }
}

impl VisitFields for EngineThemeGroupDef {
    fn visit_fields<V: FieldVisitor>(&mut self, _visitor: &mut V) {}
}

impl EngineThemeGroupDef {
    pub(crate) fn parse(_cur: &mut &[u8]) -> Result<Self, ParseControlError> {
        Ok(Self)
    }

    pub(crate) fn serialize(&self, _out: &mut &mut [u8]) -> Result<(), SerializeControlError> {
        Ok(())
    }

    pub(crate) fn byte_size(&self) -> usize {
        0
    }
}
