//! Generate a cleaned C++ header from Fable's decompiled def classes.
//!
//! The input is a `decomp-cli tree` directory: one header per original source
//! file, with the PDB's class declarations. The output keeps the original
//! class, base and member names, and removes methods, compiler spellings, and
//! runtime bookkeeping.

pub mod clean;
pub mod emit;
pub mod tree;

use std::fmt;
use std::path::Path;

pub use emit::{Stats, generate};
pub use tree::{Tree, parse_tree};

/// A diagnostic from parsing or emitting the tree.
#[derive(Debug)]
pub struct Error {
    message: String,
}

impl Error {
    pub fn new(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.message)
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;

/// Parse a `decomp-cli tree` directory and render the cleaned header.
pub fn generate_from_tree(root: &Path) -> Result<(String, Stats)> {
    let tree = parse_tree(root)?;
    generate(&tree)
}
