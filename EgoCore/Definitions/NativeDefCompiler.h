#pragma once
//
// Native def compilation via the fable-defs compiler (ThirdParty/fable-defs).
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
#include "def_compiler.h"

#include <string>
#include <vector>

namespace NativeDefs {

    // Compile <fableRoot>\Data\Defs into <fableRoot>\Data\CompiledDefs,
    // producing game.bin, frontend.bin, script.bin and names.bin.
    //
    // Returns true on success. outLog always receives the compiler's output:
    // one "severity: path:line:col: message" line per diagnostic, then the
    // outcome. On failure it explains why.
    inline bool CompileAllDefs(const std::string& fableRoot, std::string& outLog) {
        const std::string defsDir = fableRoot + "\\Data\\Defs";
        const std::string outDir = fableRoot + "\\Data\\CompiledDefs";

        // One call, one build. defc_build compiles whether or not a log buffer is
        // supplied, so asking for the length first and calling again would compile
        // twice. The compiler caps itself at 100 rendered diagnostics, so this is
        // ~10x the largest log it can produce.
        std::vector<char> log(256 * 1024, '\0');
        size_t needed = 0;
        const int32_t status =
            defc_build(defsDir.c_str(), outDir.c_str(), log.data(), log.size(), &needed);

        outLog.assign(log.data());
        if (needed >= log.size()) {
            outLog += "\n[log truncated]\n";
        }

        switch (status) {
        case DEFC_OK:
            return true;
        case DEFC_ERROR_ARGS:
            outLog += "\nThe Fable path could not be used. Non-ASCII install paths "
                      "are not supported; try a path like C:\\Fable.";
            return false;
        case DEFC_ERROR_PANIC:
            outLog += "\nThis is a def compiler bug - please report it with the "
                      "message above.";
            return false;
        default:
            return false;
        }
    }

    // First line of the log that looks like an error, for a one-line status
    // label. Returns an empty string when there is nothing to report.
    inline std::string FirstError(const std::string& log) {
        size_t pos = log.find("error:");
        if (pos == std::string::npos) return "";
        const size_t end = log.find('\n', pos);
        return log.substr(pos, end == std::string::npos ? end : end - pos);
    }

} // namespace NativeDefs
