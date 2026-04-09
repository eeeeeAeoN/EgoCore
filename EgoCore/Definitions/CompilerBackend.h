#pragma once
#include "DefBackend.h"
#include "BankBackend.h"
#include "BinaryParser.h"
#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
//#include "DefCompiler.h"

namespace fs = std::filesystem;

static std::atomic<bool> g_IsCompiling(false);
static std::atomic<bool> g_StopWatchdog(false);
inline bool g_TriggerCompileSuccess = false;
inline bool g_PendingGameLaunch = false;
static std::string g_CompileStatus = "";
static std::string g_TargetIniPath = "";
static std::string g_OriginalIniContent = "";
static bool g_IniWasPatched = false;
static const uintptr_t IDA_TARGET_ADDRESS = 0x00C90613;
static const uintptr_t IDA_DEFAULT_BASE = 0x00400000;
static const uintptr_t INSTRUCTION_BYTE_OFFSET = 6;

struct PatchRule {
    std::string Keyword;
    std::string Replacement;
    bool Found = false; 
};

static std::vector<PatchRule> GetPatchRules() {
    return {
        { "SetFullscreen",           "SetFullscreen(FALSE);" },
        { "ShowDevFrontEnd",         "ShowDevFrontEnd FALSE;" },
        { "SetSkipFrontend",         "SetSkipFrontend(true);" },
        { "AllowDataGeneration",     "AllowDataGeneration TRUE;" },
        { "SetLevel",                "SetLevel(\"dummy_level.wld\");" },
        { "BuildRetailStaticMaps",   "BuildRetailStaticMaps TRUE;" },
        { "UseCompiledDefs",         "UseCompiledDefs TRUE;" },
        { "UseRetailBanks",          "UseRetailBanks TRUE;" },
        { "AllowBackgroundProcessing","AllowBackgroundProcessing true;" }
    };
}

static bool ContainsCaseInsensitive(const std::string& str, const std::string& sub) {
    auto it = std::search(
        str.begin(), str.end(),
        sub.begin(), sub.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return (it != str.end());
}

static bool PatchIniFile(const std::string& rootPath) {
    g_TargetIniPath = "";
    g_OriginalIniContent = "";
    g_IniWasPatched = false;

    std::string dbugPath = rootPath + "\\dbugst.ini";
    std::string userPath = rootPath + "\\userst.ini";

    if (fs::exists(dbugPath)) g_TargetIniPath = dbugPath;
    else if (fs::exists(userPath)) g_TargetIniPath = userPath;
    else {
        MessageBoxA(NULL, "Configuration files not found!\n(Missing userst.ini or dbugst.ini)", "EgoCore Error", MB_ICONERROR);
        return false;
    }

    std::ifstream inFile(g_TargetIniPath);
    if (!inFile.is_open()) return false;

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    g_OriginalIniContent = buffer.str();
    inFile.close();

    std::istringstream linesStream(g_OriginalIniContent);
    std::vector<std::string> newLines;
    std::string line;
    auto rules = GetPatchRules();

    while (std::getline(linesStream, line)) {
        size_t commentPos = line.find("//");

        if (commentPos == 0) {
            newLines.push_back(line);
            continue;
        }

        bool matchedRule = false;
        for (auto& rule : rules) {
            std::string contentPart = line.substr(0, commentPos);

            if (ContainsCaseInsensitive(contentPart, rule.Keyword)) {
                newLines.push_back(rule.Replacement);
                rule.Found = true;
                matchedRule = true;
                break;
            }
        }

        if (!matchedRule) {
            newLines.push_back(line);
        }
    }

    for (const auto& rule : rules) {
        if (!rule.Found) {
            newLines.push_back(rule.Replacement);
        }
    }

    std::ofstream outFile(g_TargetIniPath, std::ios::trunc);
    if (!outFile.is_open()) return false;

    for (const auto& l : newLines) {
        outFile << l << "\n";
    }
    outFile.close();

    g_IniWasPatched = true;
    return true;
}

static void RestoreIniFile() {
    if (g_IniWasPatched && !g_TargetIniPath.empty() && !g_OriginalIniContent.empty()) {
        std::ofstream outFile(g_TargetIniPath, std::ios::trunc);
        if (outFile.is_open()) {
            outFile << g_OriginalIniContent;
            outFile.close();
        }
        g_IniWasPatched = false;
    }
}

static void ErrorWatchdogThread() {
    while (!g_StopWatchdog) {
        HWND hError = FindWindowA(NULL, "Error!");
        if (hError) PostMessageA(hError, WM_CLOSE, 0, 0);

        HWND hAppError = FindWindowA(NULL, "Application Error");
        if (hAppError) PostMessageA(hAppError, WM_CLOSE, 0, 0);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

static uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

static bool RunHiddenFable(const std::string& exePath, bool shouldPatchCode) {

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWMINNOACTIVE;

    std::string cmdStr = "\"" + exePath + "\" -build_retail_static_maps";
    std::vector<char> cmdLine(cmdStr.begin(), cmdStr.end());
    cmdLine.push_back('\0');

    DWORD creationFlags = (shouldPatchCode ? CREATE_SUSPENDED : 0);
    std::string workingDir = fs::path(exePath).parent_path().string();

    if (!CreateProcessA(NULL, cmdLine.data(), NULL, NULL, FALSE, creationFlags, NULL, workingDir.c_str(), &si, &pi)) {
        g_CompileStatus = "Error: CreateProcess failed.";
        return false;
    }

    if (shouldPatchCode) {
        Sleep(200);
        uintptr_t realBase = GetModuleBaseAddress(pi.dwProcessId, L"ego_r.exe");
        if (realBase == 0) realBase = 0x00400000;

        uintptr_t relativeOffset = IDA_TARGET_ADDRESS - IDA_DEFAULT_BASE;
        LPVOID targetAddress = (LPVOID)(realBase + relativeOffset + INSTRUCTION_BYTE_OFFSET);

        DWORD oldProtect;
        if (VirtualProtectEx(pi.hProcess, targetAddress, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            BYTE patchValue = 0x00;
            SIZE_T bytesWritten = 0;
            WriteProcessMemory(pi.hProcess, targetAddress, &patchValue, 1, &bytesWritten);
            VirtualProtectEx(pi.hProcess, targetAddress, 1, oldProtect, &oldProtect);
        }
        ResumeThread(pi.hThread);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

/*
static void NativeCompileFrontend() {
    g_CompileStatus = "Native Compiling... Check compiler.log in Fable folder.";

    FableCompiler::CompileFrontendNative(g_AppConfig.GameRootPath);
}
*/

static void CompileAllDefs_Stealth() {
    if (!g_OpenBanks.empty()) {
        g_OpenBanks.clear();
        g_ActiveBankIndex = -1;
    }

    g_IsCompiling = true;
    g_StopWatchdog = false;

    std::thread watchdog(ErrorWatchdogThread);

    std::thread([watchdog = std::move(watchdog)]() mutable {

        g_CompileStatus = "Compiling Sound Binaries...";
        std::string log;

        std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
        BinaryParser::CompileSoundBinaries(defsPath, log);

        std::string exePath = g_AppConfig.GameRootPath + "\\ego_r.exe";
        if (!fs::exists(exePath)) {
            g_CompileStatus = "Sound Binaries compiled, but ego_r.exe not found! Game defs skipped.";
            g_StopWatchdog = true;
            if (watchdog.joinable()) watchdog.join();
            g_IsCompiling = false;
            return;
        }

        if (!PatchIniFile(g_AppConfig.GameRootPath)) {
            g_CompileStatus = "Sound Binaries compiled, but INI patch failed! Game defs skipped.";
            g_StopWatchdog = true;
            if (watchdog.joinable()) watchdog.join();
            g_IsCompiling = false;
            return;
        }

        g_CompileStatus = "Compiling Frontend (1/2)...";
        RunHiddenFable(exePath, false);

        g_CompileStatus = "Compiling Game (2/2)...";
        RunHiddenFable(exePath, true);

        g_StopWatchdog = true;
        if (watchdog.joinable()) watchdog.join();

        RestoreIniFile();

        g_CompileStatus = "Success! Definitions & Binaries Compiled.";
        g_IsCompiling = false;
        }).detach();
}

static void RunStealthCompilerSync() {
    std::string exePath = g_AppConfig.GameRootPath + "\\ego_r.exe";
    if (!fs::exists(exePath)) return;

    if (!PatchIniFile(g_AppConfig.GameRootPath)) return;

    RunHiddenFable(exePath, false);
    RunHiddenFable(exePath, true);

    RestoreIniFile();
}