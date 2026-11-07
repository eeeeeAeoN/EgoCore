//! Def decompiler, binary diff engine, and `.def` text emitter for Fable.

pub mod diff;
pub mod emit;
pub mod unlower;

pub use diff::{
    BinaryDiffResult, DecompileDiffOutput, build_crc_to_str, build_parent_map, diff_binary,
    run_diff,
};
pub use emit::{format_definition, format_expr, format_path, format_statement};
pub use unlower::{
    SCALAR_DEF_INDEX_FIELDS, SemVal, UnlowerCtx, is_scalar_def_index, unlower_def, unlower_generic,
};
