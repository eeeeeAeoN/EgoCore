// High-level directory build API: text corpus -> retail binaries.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "fable/defs/compiler/build_pipeline.hpp"
#include "fable/defs/compiler/symbols.hpp"

namespace fable::defs::compiler {

struct DirectoryBuild {
    bool ok = false;
    std::string log;
    BuildResult result;
};

/// Parse every `.def` / `.tpl` / `.h` under `input`, evaluate header symbols,
/// and write game.bin, frontend.bin, script.bin and names.bin into `output`.
///
/// `log` is a rendered, human-readable summary plus one line per diagnostic.
/// This is the entry point used by EgoCore; it does not print to stdout or
/// stderr.
DirectoryBuild build_directory(const std::filesystem::path &input,
                               const std::filesystem::path &output);

/// Parse all embedded non-retail headers not already present in `disk_files`
/// and evaluate their symbols into `symbols`. Returns parse/symbol error count.
std::size_t inject_embedded_headers(
    SymbolTable &symbols, const std::vector<std::filesystem::path> &disk_files,
    std::vector<Redefinition> &redefined, std::uint32_t &next_file,
    std::string &error_log);

} // namespace fable::defs::compiler
