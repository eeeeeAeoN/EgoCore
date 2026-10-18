#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <sstream>
#include "ConfigBackend.h"
#include "imgui.h"
#include "egostb/stb_compiler.hpp"

namespace fs = std::filesystem;
extern ImFont* g_TitleFont;

namespace STBBackend {
    inline bool g_TriggerSTBModal = false;
    inline std::atomic<bool> g_IsCompiling{ false };
    inline std::atomic<bool> g_Finished{ false };
    inline std::atomic<bool> g_CancelRequested{ false };
    inline bool g_Success = false;
    inline egostb::CompileResult g_CompileResult;

    inline std::mutex g_StatusMutex;
    inline std::string g_StatusText = "";
    inline float g_Progress = 0.0f;
    inline std::string g_ErrorMessage = "";
    inline std::string g_TargetWldName = "FinalAlbion.wld";
    inline std::string g_TargetGameRoot = "";

    inline void SetStatus(float progress, const std::string& status) {
        std::lock_guard<std::mutex> lock(g_StatusMutex);
        g_Progress = progress;
        g_StatusText = status;
    }

    inline void GetStatus(float& outProgress, std::string& outStatus) {
        std::lock_guard<std::mutex> lock(g_StatusMutex);
        outProgress = g_Progress;
        outStatus = g_StatusText;
    }

    inline void OpenRecompilePrompt(const std::string& gameRoot, const std::string& wldName = "FinalAlbion.wld") {
        g_TargetGameRoot = gameRoot;
        g_TargetWldName = wldName;
        g_Finished = false;
        g_IsCompiling = false;
        g_TriggerSTBModal = true;
    }

    inline void StartRecompilation(const std::string& gameRoot, const std::string& wldName = "FinalAlbion.wld") {
        if (g_IsCompiling) return;

        g_TargetGameRoot = gameRoot;
        g_TargetWldName = wldName;
        g_IsCompiling = true;
        g_Finished = false;
        g_Success = false;
        g_CancelRequested = false;
        g_ErrorMessage = "";
        SetStatus(0.0f, "Preparing to recompile " + wldName + "...");

        std::thread([]() {
            egostb::CompilerConfig config;
            config.fableRoot = g_TargetGameRoot;
            config.wldName = g_TargetWldName;
            config.retailMode = true;
            config.outputIsm = true;
            config.verbose = true;
            config.cancelToken = &g_CancelRequested;

            config.progressCallback = [](float progress, const std::string& status) {
                SetStatus(progress, status);
            };

            auto result = egostb::StbCompiler::compileWorld(config);

            g_CompileResult = result;
            g_Success = result.success;
            if (!result.success) {
                g_ErrorMessage = result.errorMessage;
            }
            g_IsCompiling = false;
            g_Finished = true;
        }).detach();
    }

    inline void DrawSTBModal() {
        if (g_TriggerSTBModal) {
            ImGui::OpenPopup("Recompile Static Terrain Bank");
            g_TriggerSTBModal = false;
        }

        // Modal Styling matching EgoCore Arcane Theme
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

        ImGui::SetNextWindowSize(ImVec2(560, 320), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Recompile Static Terrain Bank", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            ImVec2 winMax = ImVec2(winPos.x + winSize.x, winPos.y + winSize.y);

            // 1. Outer Gold Glow Aura
            for (int i = 3; i >= 1; i--) {
                float expand = (float)i * 1.8f;
                float glowAlpha = (1.0f - (float)i / 4.0f) * 0.25f;
                ImU32 glowCol = IM_COL32(242, 193, 78, (uint32_t)(glowAlpha * 255.0f));
                drawList->AddRect(
                    ImVec2(winPos.x - expand, winPos.y - expand),
                    ImVec2(winMax.x + expand, winMax.y + expand),
                    glowCol, 10.0f + expand, 0, 1.2f
                );
            }

            // 2. Arcane Header Gradient Banner
            ImVec2 headerMin = winPos;
            ImVec2 headerMax = ImVec2(winMax.x, winPos.y + 58.0f);
            drawList->AddRectFilledMultiColor(
                headerMin, headerMax,
                IM_COL32(242, 193, 78, 30),  // Top-Left Gold
                IM_COL32(110, 140, 175, 15), // Top-Right Slate Blue
                IM_COL32(0, 0, 0, 0),        // Bottom-Right Clear
                IM_COL32(0, 0, 0, 0)         // Bottom-Left Clear
            );

            // Header Title
            if (g_TitleFont) ImGui::PushFont(g_TitleFont);
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Recompile Static Terrain Bank");
            if (g_TitleFont) ImGui::PopFont();

            ImGui::Dummy(ImVec2(0, 4));

            // Gold fade separator line
            drawList->AddLine(
                ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
                ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
                IM_COL32(242, 193, 78, 110), 1.2f
            );
            ImGui::Dummy(ImVec2(0, 12));

            // --- STATE 1: RECOMPILE CONFIRMATION PROMPT ---
            if (!g_IsCompiling && !g_Finished) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
                ImGui::TextWrapped("Ready to recompile the Static Terrain Bank archive from %s.", g_TargetWldName.c_str());
                ImGui::TextWrapped("This will compile all maps, LOD quadtrees, and ISM water data directly into retail format.");
                ImGui::Dummy(ImVec2(0, 6));

                std::string rootDisplay = g_TargetGameRoot.empty() ? "(Auto-detect from egocore_config.ini)" : g_TargetGameRoot;
                ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.80f, 1.0f), "Game Root: %s", rootDisplay.c_str());
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 16));

                // Primary Action Button (Gold)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Recompile Now", ImVec2(150, 28))) {
                    StartRecompilation(g_TargetGameRoot, g_TargetWldName);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 12.0f);

                // Secondary Action Button (Slate Dark)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Cancel", ImVec2(100, 28))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
            }
            // --- STATE 2: IN PROGRESS WITH PROGRESS BAR & % ---
            else if (g_IsCompiling) {
                float curProgress = 0.0f;
                std::string curStatus = "";
                GetStatus(curProgress, curStatus);

                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Recompiling Static Terrain Bank...");
                ImGui::Dummy(ImVec2(0, 4));

                // Current task / status line
                ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "%s", curStatus.c_str());
                ImGui::Dummy(ImVec2(0, 10));

                // Progress Bar with Percentage Overlay
                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%.1f%%", curProgress * 100.0f);

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.92f, 0.74f, 0.24f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.14f, 0.18f, 1.0f));
                ImGui::ProgressBar(curProgress, ImVec2(-1.0f, 26.0f), overlay);
                ImGui::PopStyleColor(2);

                ImGui::Dummy(ImVec2(0, 10));

                static int dots = 0;
                if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
                std::string spinner = "Generating meshes, textures and quadtrees";
                for (int i = 0; i < dots; i++) spinner += ".";
                ImGui::TextDisabled("%s", spinner.c_str());

                ImGui::Dummy(ImVec2(0, 10));

                // Cancel Button during compilation
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.14f, 0.14f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.18f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.10f, 0.10f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.70f, 0.70f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Cancel Compilation", ImVec2(160, 26))) {
                    g_CancelRequested = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
            }
            // --- STATE 3: COMPLETE - SUCCESS ---
            else if (g_Finished && g_Success) {
                ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "Recompilation Succeeded!");
                ImGui::Dummy(ImVec2(0, 4));

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
                ImGui::TextWrapped("Static Terrain Bank archive was generated successfully.");

                ImGui::Dummy(ImVec2(0, 4));
                ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.85f, 1.0f), "Output File: %s", g_CompileResult.outputPath.c_str());
                ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.85f, 1.0f), "Maps: %d | BG Nodes: %d | Patches: %d",
                    g_CompileResult.mapsCompiled, g_CompileResult.totalNodes, g_CompileResult.totalPatches);

                double mb = static_cast<double>(g_CompileResult.fileSize) / (1024.0 * 1024.0);
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << mb << " MB in " << std::setprecision(1) << g_CompileResult.durationSeconds << "s";
                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Size: %s", ss.str().c_str());
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 14));

                float closeBtnWidth = 120.0f;
                ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("OK", ImVec2(closeBtnWidth, 28))) {
                    g_Finished = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
            }
            // --- STATE 4: COMPLETE - ERROR ---
            else if (g_Finished && !g_Success) {
                ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Recompilation Failed!");
                ImGui::Dummy(ImVec2(0, 4));

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
                ImGui::TextWrapped("The Static Terrain Bank compiler reported the following error:");
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 4));

                // Scrollable Error Frame
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.06f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.25f, 0.25f, 0.50f));
                ImGui::BeginChild("STBErrorBox", ImVec2(-1, 80), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", g_ErrorMessage.c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor(2);

                ImGui::Dummy(ImVec2(0, 10));

                // Copy Error Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Copy Error", ImVec2(120, 28))) {
                    ImGui::SetClipboardText(g_ErrorMessage.c_str());
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 12.0f);

                // Close Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Close", ImVec2(100, 28))) {
                    g_Finished = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
    }
}
