#pragma once
//
// Native def compilation via the embedded FableDefs C++ compiler.
//
// This replaces the "stealth" compile in CompilerBackend.h, which patched
// dbugst.ini, launched ego_r.exe twice (once with a patched instruction), and
// ran a watchdog thread to close the game's error dialogs. The native compiler
// reads Data\Defs and writes the four binaries directly, so none of that is
// needed: no ego_r.exe, no ini patching, no hidden game process.
//
// It also reports real diagnostics — file, line, column and message for every
// bad def — where the stealth path could only suppress crash popups.
//
#include <cstddef>
#include <string>

#include "fable/defs/compiler/build.hpp"
#include "fable/defs/compiler/decompiler.hpp"

namespace NativeDefs {

    // Compile <fableRoot>\Data\Defs into <fableRoot>\Data\CompiledDefs,
    // producing game.bin, frontend.bin, script.bin and names.bin.
    //
    // Returns true on success. outLog always receives the compiler's rendered
    // output: diagnostics followed by the outcome.
    inline bool CompileAllDefs(const std::string& fableRoot, std::string& outLog) {
        const std::string defsDir = fableRoot + "\\Data\\Defs";
        const std::string outDir = fableRoot + "\\Data\\CompiledDefs";

        const auto report = fable::defs::compiler::build_directory(defsDir, outDir);
        outLog = report.log;
        return report.ok;
    }

    // First line of the log that looks like an error, for a one-line status
    // label. Returns an empty string when there is nothing to report.
    inline std::string FirstError(const std::string& log) {
        std::size_t pos = log.find("error:");
        if (pos == std::string::npos) return "";
        const std::size_t end = log.find('\n', pos);
        return log.substr(pos, end == std::string::npos ? end : end - pos);
    }

    // Decompile one compiled binary directory (names.bin + <binary>.bin) to .def text.
    inline bool DecompileBinary(const std::string& binDir,
                                const std::string& binaryName,
                                const std::string& defsDir,
                                std::string& outText,
                                fable::defs::compiler::DecompileStats& outStats,
                                std::string& outError) {
        return fable::defs::compiler::decompile_binary_to_text(
            binDir, binaryName, defsDir, outText, outStats, outError);
    }

    // Decompile compiled definitions (`all` or one of `game` / `frontend` / `script`)
    // into heuristic .tpl / .def files under outDir matching vanilla layout.
    inline bool DecompileToFolder(const std::string& binDir,
                                  const std::string& binaryName,
                                  const std::string& headersDir,
                                  const std::string& outDir,
                                  fable::defs::compiler::FolderDecompileStats& outStats,
                                  std::string& outError) {
        if (binaryName.empty() || binaryName == "all") {
            return fable::defs::compiler::decompile_directory_to_folder(
                binDir, headersDir, outDir, outStats, outError);
        }
        return fable::defs::compiler::decompile_binary_to_folder(
            binDir, binaryName, headersDir, outDir, outStats, outError);
    }

    // Compare two compiled binary directories and emit modified/added definitions.
    inline bool DiffBinaries(const std::string& origDir,
                             const std::string& modDir,
                             const std::string& defsDir,
                             std::string& outText,
                             fable::defs::compiler::DecompileStats& outStats,
                             std::string& outError) {
        return fable::defs::compiler::diff_binaries_to_text(
            origDir, modDir, defsDir, outText, outStats, outError);
    }

    // Patch modified definitions into existing .def files and write newly added
    // definitions to z_mod_decompiled.def.
    inline bool PatchBinaries(const std::string& origDir,
                              const std::string& modDir,
                              const std::string& defsDir,
                              fable::defs::compiler::DecompileStats& outStats,
                              std::string& outError) {
        return fable::defs::compiler::patch_binaries(
            origDir, modDir, defsDir, outStats, outError);
    }

} // namespace NativeDefs
