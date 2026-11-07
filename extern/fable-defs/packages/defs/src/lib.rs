//! Fable def compiler support library: the typed def model, text parsing, and
//! the compiled-def binary container, extracted from OpenAlbion's `fable-data`.
//!
//! The def structs are declared with the [`defs_derive`] proc-macros, which
//! generate code against `crate::def::…` and `crate::bytes::…` paths.

pub mod binary;
pub mod bytes;
pub mod crc32;
pub mod def;
pub mod enums;
pub mod names;
pub mod text;
pub mod values;
pub mod visit;
pub mod wire;

/// Proc-macro derives for the def wire model (see [`wire::Wire`] / [`enums::DefEnum`]).
/// Re-exported at the crate root so the generated `crate::def::…` paths resolve
/// and def modules can `use crate::{DefStruct, WireStruct, …}`.
pub use defs_derive::{DefEnum, DefFlags, DefStruct, DefVariant, WireStruct};
