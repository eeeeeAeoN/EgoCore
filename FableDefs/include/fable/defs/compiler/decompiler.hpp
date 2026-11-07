// Def decompiler foundation: binary -> typed value tree -> .def text.
#pragma once

#include <string>
#include <vector>

namespace fable::defs::compiler {

struct DecompileStats {
    std::size_t entries = 0;
    std::size_t definitions = 0;
    std::size_t modified = 0;
    std::size_t added = 0;
    std::vector<std::string> errors;
};

/// Decompile one binary directory (`names.bin` + `<binary>.bin`) to .def text.
bool decompile_binary_to_text(const std::string &directory, const std::string &binary,
                              std::string &out, DecompileStats &stats,
                              std::string &error);

/// Compare two compiled binary directories and emit modified/added definitions.
bool diff_binaries_to_text(const std::string &original_directory,
                           const std::string &modified_directory,
                           std::string &out, DecompileStats &stats,
                           std::string &error);

} // namespace fable::defs::compiler
