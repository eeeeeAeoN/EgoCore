#pragma once
#include "DefBackend.h"
#include "BankBackend.h"
#include "BinaryParser.h"
#include <windows.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <string>
#include "NativeDefCompiler.h"
#include "AnimationEventCompiler.h"

namespace fs = std::filesystem;

// Full compiler output from the last native compile, for the def error popup.
inline std::string g_DefCompileLog = "";

static std::atomic<bool> g_IsCompiling(false);
inline bool g_TriggerCompileSuccess = false;
inline bool g_DefCompileSuccess = false;
inline bool g_PendingGameLaunch = false;
static std::string g_CompileStatus = "";

// Drop cached banks and def state so the reload after compiling sees the new
// binaries.
static void ResetWorkspaceForCompile() {
    if (!g_OpenBanks.empty()) {
        g_OpenBanks.clear();
        g_ActiveBankIndex = -1;
    }

    for (auto& ctx : g_DefWorkspace.Contexts) {
        ctx.IsLoaded = false;
    }
    g_DefWorkspace.IsLoaded = false;

    g_DefWorkspace.CategorizedDefs.clear();
    g_DefWorkspace.AllEnums.clear();
    g_DefWorkspace.SelectedEntryIndex = -1;
    g_DefWorkspace.SelectedType = "";
    g_DefWorkspace.SelectedEnumIndex = -1;
}

// Rebuild the four .bin files and reload the workspace. Defs are compiled
// in-process by the native compiler (see NativeDefCompiler.h), so this needs no
// ego_r.exe, no ini patching and no error-dialog watchdog.
static void CompileAllDefs_Native() {
    g_IsCompiling = true;

    std::thread([]() {
        // 1. Sound binaries first, while g_DefWorkspace is still populated —
        //    CompileSoundBinaries reads CategorizedDefs/AllEnums to figure out
        //    what to compile, so this must happen before the reset below.
        g_CompileStatus = "Compiling Sound Binaries...";
        std::string soundLog;
        BinaryParser::CompileSoundBinaries(g_AppConfig.GameRootPath + "\\Data\\Defs", soundLog);

        g_DefCompileLog.clear();
        g_DefCompileLog += "--- Sound Binaries ---\n" + soundLog;

        // 2. Now safe to clear workspace state ahead of the def recompile.
        ResetWorkspaceForCompile();

        // 3. Def compilation via the native (Rust) compiler.
        g_CompileStatus = "Compiling Definitions...";
        std::string defLog;
        const bool ok = NativeDefs::CompileAllDefs(g_AppConfig.GameRootPath, defLog);
        g_DefCompileLog += "\n--- Definitions ---\n" + defLog;

        // Trust the compiler's own status, but confirm the artifacts landed.
        const std::string compiledDefsDir = g_AppConfig.GameRootPath + "\\Data\\CompiledDefs\\";
        const bool allFound = fs::exists(compiledDefsDir + "frontend.bin") &&
            fs::exists(compiledDefsDir + "game.bin") &&
            fs::exists(compiledDefsDir + "names.bin") &&
            fs::exists(compiledDefsDir + "script.bin");

        g_DefCompileSuccess = ok && allFound;

        if (g_DefCompileSuccess) {
            g_CompileStatus = "Success! Definitions & Binaries Compiled.";
        }
        else {
            const std::string first = NativeDefs::FirstError(defLog);
            g_CompileStatus = first.empty() ? "Error during def compilation." : first;
        }

        LoadDefsFromFolder(g_AppConfig.GameRootPath, true);

        NAnimationEvents::CBinaryCompiler eventCompiler;
        eventCompiler.CompileAnimationEvents(g_DefWorkspace.RootPath);

        g_IsCompiling = false;
        }).detach();
}