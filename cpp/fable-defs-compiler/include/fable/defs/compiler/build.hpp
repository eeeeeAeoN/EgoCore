// High-level directory build API: text corpus -> retail binaries.
#pragma once

#include <filesystem>
#include <string>

#include "fable/defs/compiler/build_pipeline.hpp"

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

} // namespace fable::defs::compiler
