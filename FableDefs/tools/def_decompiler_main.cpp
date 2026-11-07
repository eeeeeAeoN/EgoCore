// fable-def-decompiler — compiled def binary reader and .def emitter.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "fable/defs/compiler/decompiler.hpp"

namespace fs = std::filesystem;

namespace {

void usage(std::ostream &out) {
    out << "Usage: fable-def-decompiler <command> [options]\n"
           "\n"
           "Commands:\n"
           "  dump <bin-dir> <defs-dir> <out.def> [--binary game|frontend|script]\n"
           "                  Decompile one compiled binary to .def text\n"
           "  diff <orig-dir> <mod-dir> <defs-dir> <out.def>\n"
           "                  Emit definitions that differ between two builds\n"
           "  patch <orig-dir> <mod-dir> <defs-dir>\n"
           "                  Patch changed definitions into the .def sources\n"
           "\n"
           "A bin-dir contains names.bin and game.bin/frontend.bin/script.bin.\n";
}

bool write_text(const fs::path &path, const std::string &text, std::string &error) {
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec) {
            error = "cannot create " + path.parent_path().string() + ": " + ec.message();
            return false;
        }
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error = "cannot open " + path.string();
        return false;
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv) {
    using namespace fable::defs::compiler;

    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "--help" || args[0] == "help") {
        usage(std::cout);
        return 0;
    }

    std::string out_path;
    std::string binary = "game";
    std::string original_dir;
    std::string modified_dir;
    std::string defs_dir;

    const std::string command = args[0];
    if (command == "dump") {
        if (args.size() < 4) {
            usage(std::cerr);
            return 2;
        }
        original_dir = args[1];
        defs_dir = args[2];
        out_path = args[3];
        for (std::size_t i = 4; i < args.size(); ++i) {
            if (args[i] == "--binary" && i + 1 < args.size()) {
                binary = args[++i];
            } else {
                std::cerr << "fable-def-decompiler: unknown option '" << args[i] << "'\n";
                return 2;
            }
        }
    } else if (command == "diff") {
        if (args.size() != 5) {
            usage(std::cerr);
            return 2;
        }
        original_dir = args[1];
        modified_dir = args[2];
        defs_dir = args[3];
        out_path = args[4];
    } else if (command == "patch") {
        if (args.size() != 4) {
            usage(std::cerr);
            return 2;
        }
        original_dir = args[1];
        modified_dir = args[2];
        defs_dir = args[3];
    } else {
        usage(std::cerr);
        return 2;
    }

    std::string text;
    std::string error;
    DecompileStats stats;
    bool ok = false;
    if (command == "dump") {
        ok = decompile_binary_to_text(original_dir, binary, defs_dir, text, stats, error);
    } else if (command == "diff") {
        ok = diff_binaries_to_text(original_dir, modified_dir, defs_dir, text, stats,
                                   error);
    } else {
        ok = patch_binaries(original_dir, modified_dir, defs_dir, stats, error);
    }
    if (!ok) {
        std::cerr << "fable-def-decompiler: " << error << "\n";
        return 1;
    }
    if (command != "patch" && !write_text(fs::path(out_path), text, error)) {
        std::cerr << "fable-def-decompiler: " << error << "\n";
        return 1;
    }

    std::cerr << "entries: " << stats.entries << ", definitions: " << stats.definitions
              << ", modified: " << stats.modified << ", added: " << stats.added
              << "\n";
    for (const std::string &message : stats.errors) {
        std::cerr << "warning: " << message << "\n";
    }
    return 0;
}
