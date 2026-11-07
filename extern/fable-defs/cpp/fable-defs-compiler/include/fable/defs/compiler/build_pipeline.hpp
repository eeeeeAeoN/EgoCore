// Binary build pipeline: corpus -> game.bin / frontend.bin / script.bin / names.bin.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/manifest.hpp"
#include "fable/defs/compiler/names.hpp"
#include "fable/defs/compiler/symbols.hpp"

namespace fable::defs::compiler {

struct ParsedFile {
    std::string path; // normalized to forward slashes
    SourceAst ast;
};

struct BinBuildStats {
    std::string label;
    std::string file_name;
    std::size_t entries = 0;
    std::size_t null_defs = 0;
    std::size_t named = 0;
    std::size_t lowered = 0;
    std::size_t unsupported = 0;
    std::vector<std::string> errors;
};

struct BuildResult {
    bool ok = false;
    std::vector<BinBuildStats> bins;
    std::size_t names = 0;
    std::string error;
};

/// Build one or more binaries and the shared names.bin.
///
/// `files` must stay alive for the duration of the call; the pipeline points
/// into their ASTs. Names are interned in build order (game, frontend, script),
/// matching the Rust compiler's single shared table.
BuildResult build_binaries(const std::vector<ParsedFile> &files,
                           const SymbolTable &symbols,
                           const std::filesystem::path &out_dir,
                           const std::vector<BinaryKind> &kinds);

} // namespace fable::defs::compiler
