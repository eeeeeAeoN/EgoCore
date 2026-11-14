#pragma once

#include "imgui.h"
#include "FileDialogs.h"
#include "ConfigBackend.h"
#include "DefBackend.h"
#include "NativeDefCompiler.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

extern ImFont* g_CodeFont;

namespace DefDecompilerUI {

struct State {
    bool TriggerOpen = false;
    std::atomic<bool> IsRunning{ false };
    std::mutex Mutex;

    char DumpBinDir[512] = "";
    char OrigBinDir[512] = "";
    char ModBinDir[512] = "";
    char DefsDir[512] = "";
    int BinaryIndex = 0; // 0=game, 1=frontend, 2=script

    std::string StatusText;
    bool LastOk = false;
    bool HasResult = false;
    std::string OutputText;
    std::string SaveStatus;
    bool PendingReloadDefs = false;
};

inline State g_State;

struct FolderState {
    bool TriggerOpen = false;
    std::atomic<bool> IsRunning{ false };
    std::mutex Mutex;

    char CompiledDefsDir[512] = "";
    char OutputDir[512] = "";
    char HeadersDir[512] = "";
    int BinaryIndex = 0; // 0=all, 1=game, 2=frontend, 3=script

    std::string StatusText;
    std::string SummaryLog;
    bool HasResult = false;
    bool LastOk = false;
};

inline FolderState g_FolderState;

inline void CopyToBuf(char* dst, size_t dstSize, const std::string& src) {
    if (dstSize == 0) return;
    const size_t n = (std::min)(src.size(), dstSize - 1);
    if (n > 0) std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

inline void OpenModal() {
    if (!g_AppConfig.GameRootPath.empty()) {
        const std::string defs = g_AppConfig.GameRootPath + "\\Data\\Defs";
        const std::string compiled = g_AppConfig.GameRootPath + "\\Data\\CompiledDefs";
        if (g_State.DefsDir[0] == '\0') CopyToBuf(g_State.DefsDir, sizeof(g_State.DefsDir), defs);
        if (g_State.DumpBinDir[0] == '\0') CopyToBuf(g_State.DumpBinDir, sizeof(g_State.DumpBinDir), compiled);
        if (g_State.ModBinDir[0] == '\0') CopyToBuf(g_State.ModBinDir, sizeof(g_State.ModBinDir), compiled);
    }
    g_State.TriggerOpen = true;
}

inline void OpenFolderModal() {
    if (!g_AppConfig.GameRootPath.empty()) {
        const std::string compiled = g_AppConfig.GameRootPath + "\\Data\\CompiledDefs";
        const std::string defs = g_AppConfig.GameRootPath + "\\Data\\Defs";
        if (g_FolderState.CompiledDefsDir[0] == '\0') {
            CopyToBuf(g_FolderState.CompiledDefsDir, sizeof(g_FolderState.CompiledDefsDir), compiled);
        }
        if (g_FolderState.HeadersDir[0] == '\0') {
            CopyToBuf(g_FolderState.HeadersDir, sizeof(g_FolderState.HeadersDir), defs);
        }
    }
    g_FolderState.TriggerOpen = true;
}

inline void DrawHelpMarker(const char* tooltip) {
    if (!tooltip || tooltip[0] == '\0') return;
    ImGui::TextColored(ImVec4(0.85f, 0.70f, 0.30f, 0.70f), "(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.10f, 0.14f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.45f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextColored(ImVec4(0.90f, 0.92f, 0.96f, 1.0f), "%s", tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
}

inline bool DrawPathRow(const char* label, const char* inputId, const char* btnId, char* buf, size_t bufSize, const char* dialogTitle, bool disabled, const char* helpText = nullptr, float inputStartX = 190.0f) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    if (helpText && helpText[0] != '\0') {
        ImGui::SameLine(0.0f, 5.0f);
        DrawHelpMarker(helpText);
    }
    ImGui::SameLine(inputStartX);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 92.0f);
    if (disabled) ImGui::BeginDisabled();
    ImGui::InputText(inputId, buf, bufSize);
    ImGui::SameLine(0.0f, 8.0f);
    bool browsed = false;
    if (ImGui::Button(btnId, ImVec2(84.0f, 0.0f))) {
        std::string chosen = OpenFolderDialog(dialogTitle);
        if (!chosen.empty()) {
            CopyToBuf(buf, bufSize, chosen);
            browsed = true;
        }
    }
    if (disabled) ImGui::EndDisabled();
    return browsed;
}

inline std::string FormatStatsSummary(const char* actionLabel, const fable::defs::compiler::DecompileStats& stats, bool ok, const std::string& error) {
    if (!ok) {
        return std::string(actionLabel) + " failed: " + (error.empty() ? "unknown error" : error);
    }
    std::string msg = std::string(actionLabel) + " complete — entries: " + std::to_string(stats.entries) +
        ", definitions: " + std::to_string(stats.definitions) +
        ", modified: " + std::to_string(stats.modified) +
        ", added: " + std::to_string(stats.added);
    if (!stats.errors.empty()) {
        msg += ", warnings/errors: " + std::to_string(stats.errors.size());
    }
    return msg;
}

inline void StartDumpJob() {
    if (g_State.IsRunning) return;
    const std::string binDir = g_State.DumpBinDir;
    const std::string defsDir = g_State.DefsDir;
    static const char* kBinaries[] = { "game", "frontend", "script" };
    const std::string binary = kBinaries[g_State.BinaryIndex];

    if (binDir.empty() || defsDir.empty()) {
        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.HasResult = true;
        g_State.LastOk = false;
        g_State.StatusText = "Please specify both the CompiledDefs folder and the Defs folder.";
        return;
    }

    g_State.IsRunning = true;
    {
        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.StatusText = "Decompiling " + binary + ".bin...";
        g_State.SaveStatus.clear();
        g_State.OutputText.clear();
    }

    std::thread([binDir, binary, defsDir]() {
        std::string outText;
        std::string error;
        fable::defs::compiler::DecompileStats stats;
        const bool ok = NativeDefs::DecompileBinary(binDir, binary, defsDir, outText, stats, error);

        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.HasResult = true;
        g_State.LastOk = ok;
        g_State.StatusText = FormatStatsSummary("Decompile", stats, ok, error);
        if (ok) {
            g_State.OutputText = std::move(outText);
        }
        else if (!stats.errors.empty()) {
            for (const auto& e : stats.errors) {
                g_State.OutputText += e + "\n";
            }
        }
        g_State.IsRunning = false;
    }).detach();
}

inline void StartDiffOrPatchJob(bool patchInPlace) {
    if (g_State.IsRunning) return;
    const std::string origDir = g_State.OrigBinDir;
    const std::string modDir = g_State.ModBinDir;
    const std::string defsDir = g_State.DefsDir;

    if (origDir.empty() || modDir.empty() || defsDir.empty()) {
        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.HasResult = true;
        g_State.LastOk = false;
        g_State.StatusText = "Please specify Original CompiledDefs, Modified CompiledDefs, and Defs folders.";
        return;
    }

    g_State.IsRunning = true;
    {
        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.StatusText = patchInPlace ? "Patching Data\\Defs in-place..." : "Diffing compiled binaries...";
        g_State.SaveStatus.clear();
        g_State.OutputText.clear();
    }

    std::thread([origDir, modDir, defsDir, patchInPlace]() {
        std::string outText;
        std::string error;
        fable::defs::compiler::DecompileStats stats;
        const bool ok = patchInPlace
            ? NativeDefs::PatchBinaries(origDir, modDir, defsDir, stats, error)
            : NativeDefs::DiffBinaries(origDir, modDir, defsDir, outText, stats, error);

        std::lock_guard<std::mutex> lock(g_State.Mutex);
        g_State.HasResult = true;
        g_State.LastOk = ok;
        g_State.StatusText = FormatStatsSummary(patchInPlace ? "Patch" : "Diff", stats, ok, error);
        if (ok) {
            if (patchInPlace) {
                g_State.OutputText = "Patched " + std::to_string(stats.modified) + " modified definition(s) in-place.\n" +
                    "Wrote " + std::to_string(stats.added) + " newly added definition(s) to z_mod_decompiled.def.\n";
                for (const auto& e : stats.errors) {
                    g_State.OutputText += "Warning: " + e + "\n";
                }
                std::error_code ec;
                if (!g_AppConfig.GameRootPath.empty() &&
                    std::filesystem::weakly_canonical(defsDir, ec) ==
                        std::filesystem::weakly_canonical(g_AppConfig.GameRootPath + "\\Data\\Defs", ec)) {
                    g_State.PendingReloadDefs = true;
                }
            }
            else {
                g_State.OutputText = std::move(outText);
            }
        }
        else if (!stats.errors.empty()) {
            for (const auto& e : stats.errors) {
                g_State.OutputText += e + "\n";
            }
        }
        g_State.IsRunning = false;
    }).detach();
}

inline void DrawModal() {
    if (g_State.TriggerOpen) {
        ImGui::OpenPopup("Def Decompiler");
        g_State.TriggerOpen = false;
    }

    if (g_State.PendingReloadDefs && !g_State.IsRunning) {
        g_State.PendingReloadDefs = false;
        if (!g_AppConfig.GameRootPath.empty()) {
            LoadDefsFromFolder(g_AppConfig.GameRootPath, true);
        }
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

    ImGui::SetNextWindowSize(ImVec2(820, 600), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Def Decompiler", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImVec2 winSize = ImGui::GetWindowSize();
        DrawGoldModalHeader("Def Decompiler");

        const bool running = g_State.IsRunning.load();
        static const char* kBinaries[] = { "game", "frontend", "script" };

        if (ImGui::BeginTabBar("##DefDecompilerTabs")) {
            if (ImGui::BeginTabItem("Dump Binary")) {
                ImGui::Dummy(ImVec2(0, 6));
                DrawPathRow("CompiledDefs Folder:", "##DumpBinDir", "Browse##DumpBin", g_State.DumpBinDir, sizeof(g_State.DumpBinDir), "Select CompiledDefs Folder (contains names.bin)", running);
                DrawPathRow("Source Defs Folder:", "##DumpDefsDir", "Browse##DumpDefs", g_State.DefsDir, sizeof(g_State.DefsDir), "Select Data\\Defs Folder", running);

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Binary:");
                ImGui::SameLine(175.0f);
                ImGui::SetNextItemWidth(160.0f);
                if (running) ImGui::BeginDisabled();
                ImGui::Combo("##DumpBinarySelect", &g_State.BinaryIndex, kBinaries, IM_ARRAYSIZE(kBinaries));
                ImGui::SameLine(0, 16.0f);
                if (ImGui::Button("Decompile to .def", ImVec2(160.0f, 0.0f))) {
                    StartDumpJob();
                }
                if (running) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Diff Builds")) {
                ImGui::Dummy(ImVec2(0, 6));
                DrawPathRow("Original CompiledDefs:", "##DiffOrigDir", "Browse##DiffOrig", g_State.OrigBinDir, sizeof(g_State.OrigBinDir), "Select Original CompiledDefs Folder", running);
                DrawPathRow("Modified CompiledDefs:", "##DiffModDir", "Browse##DiffMod", g_State.ModBinDir, sizeof(g_State.ModBinDir), "Select Modified CompiledDefs Folder", running);
                DrawPathRow("Source Defs Folder:", "##DiffDefsDir", "Browse##DiffDefs", g_State.DefsDir, sizeof(g_State.DefsDir), "Select Data\\Defs Folder", running);

                ImGui::Dummy(ImVec2(0, 4));
                if (running) ImGui::BeginDisabled();
                if (ImGui::Button("Diff Binaries", ImVec2(160.0f, 0.0f))) {
                    StartDiffOrPatchJob(false);
                }
                if (running) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Patch Defs")) {
                ImGui::Dummy(ImVec2(0, 6));
                DrawPathRow("Original CompiledDefs:", "##PatchOrigDir", "Browse##PatchOrig", g_State.OrigBinDir, sizeof(g_State.OrigBinDir), "Select Original CompiledDefs Folder", running);
                DrawPathRow("Modified CompiledDefs:", "##PatchModDir", "Browse##PatchMod", g_State.ModBinDir, sizeof(g_State.ModBinDir), "Select Modified CompiledDefs Folder", running);
                DrawPathRow("Target Defs Folder:", "##PatchDefsDir", "Browse##PatchDefs", g_State.DefsDir, sizeof(g_State.DefsDir), "Select Target Data\\Defs Folder", running);

                ImGui::TextDisabled("Modifies existing .def files in-place and writes newly added defs to z_mod_decompiled.def.");
                ImGui::Dummy(ImVec2(0, 4));
                if (running) ImGui::BeginDisabled();
                if (ImGui::Button("Patch Data\\Defs", ImVec2(160.0f, 0.0f))) {
                    StartDiffOrPatchJob(true);
                }
                if (running) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 6));

        std::string statusCopy;
        std::string outputCopy;
        std::string saveStatusCopy;
        bool hasResult = false;
        bool lastOk = false;
        {
            std::lock_guard<std::mutex> lock(g_State.Mutex);
            statusCopy = g_State.StatusText;
            outputCopy = g_State.OutputText;
            saveStatusCopy = g_State.SaveStatus;
            hasResult = g_State.HasResult;
            lastOk = g_State.LastOk;
        }

        if (!statusCopy.empty()) {
            ImVec4 statusCol = running
                ? ImVec4(0.95f, 0.82f, 0.45f, 1.0f)
                : (hasResult && !lastOk ? ImVec4(0.95f, 0.35f, 0.35f, 1.0f) : ImVec4(0.45f, 0.88f, 0.55f, 1.0f));
            ImGui::TextColored(statusCol, "%s", statusCopy.c_str());
        }
        else {
            ImGui::TextDisabled("Select a mode above to decompile, diff, or patch compiled definitions.");
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.05f, 0.07f, 0.80f));
        if (::g_CodeFont) ImGui::PushFont(::g_CodeFont);
        const float previewHeight = ImGui::GetContentRegionAvail().y - 46.0f;
        ImGui::InputTextMultiline("##DefDecompilerOutput",
            outputCopy.empty() ? const_cast<char*>("") : outputCopy.data(),
            outputCopy.size() + 1,
            ImVec2(ImGui::GetContentRegionAvail().x, previewHeight > 120.0f ? previewHeight : 120.0f),
            ImGuiInputTextFlags_ReadOnly);
        if (::g_CodeFont) ImGui::PopFont();
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));

        const bool hasOutput = !outputCopy.empty();
        if (!hasOutput || running) ImGui::BeginDisabled();
        if (ImGui::Button("Copy Output", ImVec2(120.0f, 26.0f))) {
            ImGui::SetClipboardText(outputCopy.c_str());
            std::lock_guard<std::mutex> lock(g_State.Mutex);
            g_State.SaveStatus = "Copied to clipboard.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Save .def...", ImVec2(120.0f, 26.0f))) {
            std::string savePath = SaveFileDialog("Definition Files\0*.def\0All Files\0*.*\0");
            if (!savePath.empty()) {
                std::ofstream out(savePath, std::ios::binary);
                std::lock_guard<std::mutex> lock(g_State.Mutex);
                if (out) {
                    out.write(outputCopy.data(), static_cast<std::streamsize>(outputCopy.size()));
                    g_State.SaveStatus = out ? ("Saved: " + savePath) : ("Failed to write: " + savePath);
                }
                else {
                    g_State.SaveStatus = "Failed to open: " + savePath;
                }
            }
        }
        if (!hasOutput || running) ImGui::EndDisabled();

        if (!saveStatusCopy.empty()) {
            ImGui::SameLine(0, 12.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", saveStatusCopy.c_str());
        }

        const float closeBtnWidth = 110.0f;
        ImGui::SameLine(winSize.x - closeBtnWidth - 20.0f);
        if (running) ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 26.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        if (running) ImGui::EndDisabled();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

inline void StartFolderDecompileJob() {
    if (g_FolderState.IsRunning) return;
    const std::string binDir = g_FolderState.CompiledDefsDir;
    const std::string outDir = g_FolderState.OutputDir;
    const std::string headersDir = g_FolderState.HeadersDir;
    static const char* kFolderBinaries[] = { "all", "game", "frontend", "script" };
    const std::string binary = kFolderBinaries[g_FolderState.BinaryIndex];

    if (binDir.empty() || outDir.empty()) {
        std::lock_guard<std::mutex> lock(g_FolderState.Mutex);
        g_FolderState.HasResult = true;
        g_FolderState.LastOk = false;
        g_FolderState.StatusText = "Please select both the CompiledDefs folder and the destination output folder.";
        return;
    }

    g_FolderState.IsRunning = true;
    {
        std::lock_guard<std::mutex> lock(g_FolderState.Mutex);
        g_FolderState.StatusText = "Decompiling " + binary + " definitions to folder...";
        g_FolderState.SummaryLog.clear();
    }

    std::thread([binDir, binary, headersDir, outDir]() {
        fable::defs::compiler::FolderDecompileStats stats;
        std::string error;
        const bool ok = NativeDefs::DecompileToFolder(binDir, binary, headersDir, outDir, stats, error);

        std::lock_guard<std::mutex> lock(g_FolderState.Mutex);
        g_FolderState.HasResult = true;
        g_FolderState.LastOk = ok;
        if (!ok) {
            g_FolderState.StatusText = "Folder decompilation failed: " + (error.empty() ? "unknown error" : error);
        }
        else {
            g_FolderState.StatusText =
                "Decompiled " + std::to_string(stats.definitions) + " definitions (" +
                std::to_string(stats.templates) + " templates, " +
                std::to_string(stats.specialises_recovered) + " specialises) into " +
                std::to_string(stats.files_written) + " files.";
        }

        std::string log;
        if (ok) {
            log += "Output folder: " + outDir + "\n";
            log += "Entries read: " + std::to_string(stats.entries) + "\n";
            log += "Files written (" + std::to_string(stats.files_written) + "):\n";
            for (const auto& f : stats.written_files) {
                log += "  " + f + "\n";
            }
        }
        for (const auto& e : stats.errors) {
            log += "Warning: " + e + "\n";
        }
        g_FolderState.SummaryLog = std::move(log);
        g_FolderState.IsRunning = false;
    }).detach();
}

inline void DrawFolderModal() {
    if (g_FolderState.TriggerOpen) {
        ImGui::OpenPopup("Decompile CompiledDefs");
        g_FolderState.TriggerOpen = false;
    }

    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.09f, 0.10f, 0.14f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.17f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.20f, 0.26f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.24f, 0.31f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.85f, 0.68f, 0.25f, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.85f, 0.68f, 0.25f, 0.48f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.85f, 0.68f, 0.25f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.85f, 0.68f, 0.25f, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.85f, 0.70f, 0.30f, 0.22f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 18.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

    ImGui::SetNextWindowSizeConstraints(ImVec2(700.0f, 0.0f), ImVec2(700.0f, 520.0f));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Decompile CompiledDefs", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
        ImVec2 winSize = ImGui::GetWindowSize();
        DrawGoldModalHeader("Decompile CompiledDefs");

        const bool running = g_FolderState.IsRunning.load();
        static const char* kBinaryChoices[] = {
            "All (game + frontend + script)",
            "game (.bin)",
            "frontend (.bin)",
            "script (.bin)"
        };

        DrawPathRow("CompiledDefs Folder", "##FolderSrcBinDir", "Browse##FolderSrc",
                    g_FolderState.CompiledDefsDir, sizeof(g_FolderState.CompiledDefsDir),
                    "Select CompiledDefs Folder (contains names.bin)", running,
                    "Folder containing compiled Fable definition binaries (names.bin, game.bin, frontend.bin, script.bin).");
        ImGui::Dummy(ImVec2(0, 2));

        DrawPathRow("Output Folder", "##FolderDestDir", "Browse##FolderDest",
                    g_FolderState.OutputDir, sizeof(g_FolderState.OutputDir),
                    "Select Destination Folder for Decompiled .tpl / .def Files", running,
                    "Destination folder where decompiled definitions are organized into building.tpl, creature.tpl, objects.tpl, heuristic .def files, FrontEndDefs/, and ScriptDefs/.");
        ImGui::Dummy(ImVec2(0, 2));

        DrawPathRow("Bank Headers", "##FolderHdrDir", "Browse##FolderHdr",
                    g_FolderState.HeadersDir, sizeof(g_FolderState.HeadersDir),
                    "Optional: Select Data\\Defs Folder with Bank Headers (RetailHeaders)", running,
                    "Optional Data\\Defs folder used to read RetailHeaders or modded bank enums. Non-retail engine headers & field enums are built in.");
        ImGui::Dummy(ImVec2(0, 2));

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Binary");
        ImGui::SameLine(0.0f, 5.0f);
        DrawHelpMarker("Choose whether to decompile all definition binaries (game, frontend, and script) or a single binary.");
        ImGui::SameLine(190.0f);
        ImGui::SetNextItemWidth(260.0f);
        if (running) ImGui::BeginDisabled();
        ImGui::Combo("##FolderBinarySelect", &g_FolderState.BinaryIndex, kBinaryChoices, IM_ARRAYSIZE(kBinaryChoices));
        if (running) ImGui::EndDisabled();

        std::string statusCopy;
        std::string logCopy;
        bool hasResult = false;
        bool lastOk = false;
        {
            std::lock_guard<std::mutex> lock(g_FolderState.Mutex);
            statusCopy = g_FolderState.StatusText;
            logCopy = g_FolderState.SummaryLog;
            hasResult = g_FolderState.HasResult;
            lastOk = g_FolderState.LastOk;
        }

        if (!statusCopy.empty()) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6));
            ImVec4 statusCol = running
                ? ImVec4(0.95f, 0.82f, 0.45f, 1.0f)
                : (hasResult && !lastOk ? ImVec4(0.95f, 0.38f, 0.38f, 1.0f) : ImVec4(0.45f, 0.88f, 0.55f, 1.0f));
            const float wrapRight = ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - (logCopy.empty() ? 0.0f : 28.0f);
            ImGui::PushTextWrapPos(wrapRight);
            ImGui::TextColored(statusCol, "%s", statusCopy.c_str());
            ImGui::PopTextWrapPos();
            if (!logCopy.empty()) {
                ImGui::SameLine(0.0f, 6.0f);
                DrawHelpMarker(logCopy.c_str());
            }
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        const float decompileBtnWidth = 140.0f;
        const float closeBtnWidth = 100.0f;
        const float totalBtnWidth = decompileBtnWidth + 10.0f + closeBtnWidth;
        ImGui::SetCursorPosX(winSize.x - totalBtnWidth - 22.0f);

        if (running) ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.88f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        if (ImGui::Button("Decompile", ImVec2(decompileBtnWidth, 30.0f))) {
            StartFolderDecompileJob();
        }
        ImGui::PopStyleColor(4);

        ImGui::SameLine(0.0f, 10.0f);
        if (ImGui::Button("Close", ImVec2(closeBtnWidth, 30.0f))) {
            ImGui::CloseCurrentPopup();
        }
        if (running) ImGui::EndDisabled();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(6);
    ImGui::PopStyleColor(15);
}

inline void DrawAllModals() {
    DrawFolderModal();
}

} // namespace DefDecompilerUI
