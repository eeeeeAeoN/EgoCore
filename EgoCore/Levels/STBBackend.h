#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <shellapi.h>
#include "ConfigBackend.h"
#include "FileDialogs.h"
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
    inline char g_WldInputBuffer[512] = "FinalAlbion.wld";

    // Detected worlds in game root
    inline std::vector<std::string> g_DetectedWorlds;
    inline int g_SelectedDetectedWorld = -1;

    // Quality parameters (textureScale is hardcoded to 1 to prevent in-game crashes)
    inline int g_CurrentPreset = 0; // 0: Retail Default, 1: High Quality, 2: Ultra Visuals, 3: Fast Draft, 4: Custom
    inline float g_MeshDetail = 2.0f;
    inline float g_FgDistance = 74.0f;
    inline float g_LodDistanceScale = 1.0f;
    inline int g_MaxLodBand = 7;
    inline bool g_ShareLodBands = true;
    inline bool g_EnableRangeCompression = true;
    inline bool g_OutputIsm = true;
    inline bool g_HighQualityDxt = false;
    inline bool g_GenerateMipmaps = false;

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

    static void HelpMarker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    inline void ApplyPreset(int preset) {
        g_CurrentPreset = preset;
        switch (preset) {
        case 0: // Retail Default (Original Game Accuracy)
            g_MeshDetail = 2.0f;
            g_FgDistance = 74.0f;
            g_LodDistanceScale = 1.0f;
            g_MaxLodBand = 7;
            g_ShareLodBands = true;
            g_EnableRangeCompression = true;
            g_OutputIsm = true;
            g_HighQualityDxt = false;
            g_GenerateMipmaps = false;
            break;
        case 1: // High Quality (Modern PC / Denser Meshes)
            g_MeshDetail = 3.5f;
            g_FgDistance = 120.0f;
            g_LodDistanceScale = 1.5f;
            g_MaxLodBand = 7;
            g_ShareLodBands = false;
            g_EnableRangeCompression = true;
            g_OutputIsm = true;
            g_HighQualityDxt = true;
            g_GenerateMipmaps = false;
            break;
        case 2: // Ultra Visuals
            g_MeshDetail = 5.0f;
            g_FgDistance = 180.0f;
            g_LodDistanceScale = 2.0f;
            g_MaxLodBand = 7;
            g_ShareLodBands = false;
            g_EnableRangeCompression = true;
            g_OutputIsm = true;
            g_HighQualityDxt = true;
            g_GenerateMipmaps = true;
            break;
        case 3: // Fast / Draft (Quick Compilation Test)
            g_MeshDetail = 1.0f;
            g_FgDistance = 50.0f;
            g_LodDistanceScale = 1.0f;
            g_MaxLodBand = 6;
            g_ShareLodBands = true;
            g_EnableRangeCompression = true;
            g_OutputIsm = false;
            g_HighQualityDxt = false;
            g_GenerateMipmaps = false;
            break;
        default:
            break;
        }
    }

    inline void ScanDetectedWorlds() {
        g_DetectedWorlds.clear();
        g_SelectedDetectedWorld = -1;

        std::string searchRoot = g_TargetGameRoot;
        if (searchRoot.empty()) {
            searchRoot = g_AppConfig.GameRootPath;
        }
        if (searchRoot.empty()) {
            searchRoot = egostb::StbCompiler::findGameRootFromIni();
        }

        if (searchRoot.empty()) return;

        fs::path levelsDir = fs::path(searchRoot) / "Data" / "Levels";
        if (fs::exists(levelsDir) && fs::is_directory(levelsDir)) {
            try {
                for (const auto& entry : fs::directory_iterator(levelsDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".wld") {
                        g_DetectedWorlds.push_back(entry.path().filename().string());
                    }
                }
            } catch (...) {}
        }
        std::sort(g_DetectedWorlds.begin(), g_DetectedWorlds.end());

        // Find match in detected worlds
        std::string currentStem = fs::path(g_WldInputBuffer).stem().string();
        for (int i = 0; i < (int)g_DetectedWorlds.size(); ++i) {
            if (fs::path(g_DetectedWorlds[i]).stem().string() == currentStem) {
                g_SelectedDetectedWorld = i;
                break;
            }
        }
        if (g_SelectedDetectedWorld == -1 && !g_DetectedWorlds.empty()) {
            for (int i = 0; i < (int)g_DetectedWorlds.size(); ++i) {
                if (g_DetectedWorlds[i] == "FinalAlbion.wld") {
                    g_SelectedDetectedWorld = i;
                    strncpy_s(g_WldInputBuffer, g_DetectedWorlds[i].c_str(), sizeof(g_WldInputBuffer) - 1);
                    break;
                }
            }
            if (g_SelectedDetectedWorld == -1) {
                g_SelectedDetectedWorld = 0;
                strncpy_s(g_WldInputBuffer, g_DetectedWorlds[0].c_str(), sizeof(g_WldInputBuffer) - 1);
            }
        }
    }

    inline void OpenRecompilePrompt(const std::string& gameRoot, const std::string& wldName = "") {
        g_TargetGameRoot = gameRoot;
        if (g_TargetGameRoot.empty()) {
            g_TargetGameRoot = g_AppConfig.GameRootPath;
        }
        if (g_TargetGameRoot.empty()) {
            g_TargetGameRoot = egostb::StbCompiler::findGameRootFromIni();
        }

        if (!wldName.empty()) {
            strncpy_s(g_WldInputBuffer, wldName.c_str(), sizeof(g_WldInputBuffer) - 1);
        }

        ScanDetectedWorlds();

        // Default to Retail Default preset
        ApplyPreset(0);

        // If an STBconfig.ini exists in Data\Levels, pre-populate parameters from it
        if (!g_TargetGameRoot.empty()) {
            std::string iniCandidate = g_TargetGameRoot + "\\Data\\Levels\\STBconfig.ini";
            if (fs::exists(iniCandidate)) {
                egostb::CompilerConfig cfg;
                cfg.meshDetail = g_MeshDetail;
                cfg.fgDistanceThreshold = g_FgDistance;
                cfg.lodDistanceScale = g_LodDistanceScale;
                cfg.maxLodBand = g_MaxLodBand;
                cfg.shareLodBands = g_ShareLodBands;
                cfg.enableRangeCompression = g_EnableRangeCompression;
                cfg.outputIsm = g_OutputIsm;
                cfg.highQualityDxt = g_HighQualityDxt;
                cfg.generateMipmaps = g_GenerateMipmaps;
                if (egostb::StbCompiler::loadConfigFile(iniCandidate, cfg)) {
                    g_MeshDetail = cfg.meshDetail;
                    g_FgDistance = cfg.fgDistanceThreshold;
                    g_LodDistanceScale = cfg.lodDistanceScale;
                    g_MaxLodBand = cfg.maxLodBand;
                    g_ShareLodBands = cfg.shareLodBands;
                    g_EnableRangeCompression = cfg.enableRangeCompression;
                    g_OutputIsm = cfg.outputIsm;
                    g_HighQualityDxt = cfg.highQualityDxt;
                    g_GenerateMipmaps = cfg.generateMipmaps;
                    g_CurrentPreset = 4; // Custom
                }
            }
        }

        g_Finished = false;
        g_IsCompiling = false;
        g_TriggerSTBModal = true;
    }

    inline void StartRecompilation(const std::string& gameRoot, const std::string& wldName) {
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
            config.outputIsm = g_OutputIsm;
            config.verbose = true;
            config.cancelToken = &g_CancelRequested;

            // Apply selected tuning parameters
            config.meshDetail = g_MeshDetail;
            config.fgDistanceThreshold = g_FgDistance;
            config.lodDistanceScale = g_LodDistanceScale;
            config.textureScale = 1; // Locked to default 1x to avoid crashing the game
            config.maxLodBand = g_MaxLodBand;
            config.shareLodBands = g_ShareLodBands;
            config.enableRangeCompression = g_EnableRangeCompression;
            config.highQualityDxt = g_HighQualityDxt;
            config.generateMipmaps = g_GenerateMipmaps;

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

        // Modal Styling matching EgoCore Arcane Grey Theme
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.26f, 0.26f, 0.32f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.90f, 0.75f, 0.35f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.70f, 0.58f, 0.28f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.95f, 0.80f, 0.38f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.20f, 0.26f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.32f, 0.28f, 0.20f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.11f, 0.14f, 0.98f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 18.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        // Generous width to completely prevent clipping
        ImVec2 modalSize = (!g_IsCompiling && !g_Finished) ? ImVec2(760, 540) : ImVec2(620, 320);
        ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);
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
                ImVec2(winPos.x + 22.0f, ImGui::GetCursorScreenPos().y),
                ImVec2(winMax.x - 22.0f, ImGui::GetCursorScreenPos().y),
                IM_COL32(242, 193, 78, 110), 1.2f
            );
            ImGui::Dummy(ImVec2(0, 12));

            // --- STATE 1: WORLD SELECTION & PARAMETERS MODAL ---
            if (!g_IsCompiling && !g_Finished) {
                // World Selection Section
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.88f, 0.65f, 1.0f));
                ImGui::TextUnformatted("1. World File (.wld)");
                ImGui::PopStyleColor();

                if (!g_DetectedWorlds.empty()) {
                    ImGui::TextColored(ImVec4(0.70f, 0.74f, 0.82f, 1.0f), "Detected World:");
                    ImGui::SameLine();
                    HelpMarker("World files detected in Data\\Levels. Select to quickly set the target world.");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(340.0f);
                    const char* preview = (g_SelectedDetectedWorld >= 0 && g_SelectedDetectedWorld < (int)g_DetectedWorlds.size())
                        ? g_DetectedWorlds[g_SelectedDetectedWorld].c_str() : "Choose map...";
                    if (ImGui::BeginCombo("##DetectedWldCombo", preview)) {
                        for (int i = 0; i < (int)g_DetectedWorlds.size(); ++i) {
                            const bool isSelected = (g_SelectedDetectedWorld == i);
                            if (ImGui::Selectable(g_DetectedWorlds[i].c_str(), isSelected)) {
                                g_SelectedDetectedWorld = i;
                                strncpy_s(g_WldInputBuffer, g_DetectedWorlds[i].c_str(), sizeof(g_WldInputBuffer) - 1);
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }

                // World file path input + Browse button
                ImGui::TextColored(ImVec4(0.70f, 0.74f, 0.82f, 1.0f), "World File:    ");
                ImGui::SameLine();
                HelpMarker("The .wld world file to compile into the Static Terrain Bank archive.");
                ImGui::SameLine();
                const float browseBtnWidth = 90.0f;
                float availW = ImGui::GetContentRegionAvail().x;
                ImGui::SetNextItemWidth(availW - browseBtnWidth - ImGui::GetStyle().ItemSpacing.x);
                ImGui::InputText("##WldInput", g_WldInputBuffer, sizeof(g_WldInputBuffer));

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.95f, 1.0f));
                if (ImGui::Button("Browse...", ImVec2(browseBtnWidth, 0))) {
                    std::string picked = OpenFileDialog("Fable World Files (*.wld)\0*.wld\0All Files (*.*)\0*.*\0");
                    if (!picked.empty()) {
                        strncpy_s(g_WldInputBuffer, picked.c_str(), sizeof(g_WldInputBuffer) - 1);
                        fs::path p(picked);
                        fs::path parent = p.parent_path();
                        if (parent.filename() == "Levels" && parent.parent_path().filename() == "Data") {
                            g_TargetGameRoot = parent.parent_path().parent_path().string();
                            ScanDetectedWorlds();
                        }
                    }
                }
                ImGui::PopStyleColor(4);

                std::string rootDisplay = g_TargetGameRoot.empty() ? "(Auto-detect from egocore_config.ini)" : g_TargetGameRoot;
                ImGui::TextColored(ImVec4(0.55f, 0.60f, 0.70f, 1.0f), "Game Root: %s", rootDisplay.c_str());

                ImGui::Dummy(ImVec2(0, 8));

                // Quality Presets Section
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.88f, 0.65f, 1.0f));
                ImGui::TextUnformatted("2. Quality & Tuning Parameters");
                ImGui::PopStyleColor();

                const char* presets[] = {
                    "Retail Default (Original Game Accuracy)",
                    "High Quality (Modern PC / Denser Meshes)",
                    "Ultra Visuals (Max Distance & Meshes)",
                    "Fast / Draft (Quick Compilation Test)",
                    "Custom (Manual)"
                };
                ImGui::TextColored(ImVec4(0.70f, 0.74f, 0.82f, 1.0f), "Quality Preset:");
                ImGui::SameLine();
                HelpMarker("Convenient presets for retail accuracy, enhanced graphics, or faster builds.");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(360.0f);
                if (ImGui::Combo("##PresetCombo", &g_CurrentPreset, presets, IM_ARRAYSIZE(presets))) {
                    ApplyPreset(g_CurrentPreset);
                }

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.95f, 1.0f));
                if (ImGui::Button("Reset Defaults", ImVec2(120, 0))) {
                    ApplyPreset(0);
                }
                ImGui::PopStyleColor(4);

                ImGui::Dummy(ImVec2(0, 4));

                // Sliders in 2 columns
                ImGui::Columns(2, "ParamCols", false);
                ImGui::SetColumnWidth(0, 360.0f);

                ImGui::Text("Mesh Detail Error Factor:");
                ImGui::SameLine();
                HelpMarker("LOD mesh geometric error factor (default: 2.0). Higher values produce denser, higher-polygon terrain meshes.");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##MeshDetail", &g_MeshDetail, 0.2f, 6.0f, "%.2f")) {
                    g_CurrentPreset = 4;
                }

                ImGui::Text("Foreground Distance (meters):");
                ImGui::SameLine();
                HelpMarker("Foreground patch culling threshold (default: 74.0 m). Higher values keep multi-textured terrain farther into the distance.");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##FgDist", &g_FgDistance, 0.0f, 250.0f, "%.1f m")) {
                    g_CurrentPreset = 4;
                }

                ImGui::NextColumn();

                ImGui::Text("LOD Distance Scale:");
                ImGui::SameLine();
                HelpMarker("Multiplier for background LOD band distance thresholds (default: 1.0). 1.5x or 2.0x pushes transitions further into the distance to prevent pop-in.");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##LodScale", &g_LodDistanceScale, 0.2f, 3.0f, "%.2fx")) {
                    g_CurrentPreset = 4;
                }

                ImGui::Text("Max LOD Band:");
                ImGui::SameLine();
                HelpMarker("Maximum quadtree simplification band to generate (1 to 7, default: 7).");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderInt("##MaxLodBand", &g_MaxLodBand, 1, 7)) {
                    g_CurrentPreset = 4;
                }

                ImGui::Columns(1);
                ImGui::Dummy(ImVec2(0, 6));

                // Feature Checkboxes in 2 columns
                ImGui::Columns(2, "CheckCols", false);
                ImGui::SetColumnWidth(0, 360.0f);

                if (ImGui::Checkbox("Generate companion <wld>_RT.ism", &g_OutputIsm)) {
                    g_CurrentPreset = 4;
                }
                ImGui::SameLine();
                HelpMarker("Generates companion .ism file containing sea/water meshes and instanced trees/rocks.");

                if (ImGui::Checkbox("Share LOD bands 4-6 (Retail style)", &g_ShareLodBands)) {
                    g_CurrentPreset = 4;
                }
                ImGui::SameLine();
                HelpMarker("When enabled, bands 4-6 share geometry, producing smaller STBs and matching the original retail game.");

                if (ImGui::Checkbox("Enable VB/IB range compression", &g_EnableRangeCompression)) {
                    g_CurrentPreset = 4;
                }
                ImGui::SameLine();
                HelpMarker("Compresses vertex and index streams into retail format.");

                ImGui::NextColumn();

                if (ImGui::Checkbox("High Quality DXT1 compression", &g_HighQualityDxt)) {
                    g_CurrentPreset = 4;
                }
                ImGui::SameLine();
                HelpMarker("Uses optimal perceptual cluster-fit compression instead of fast bounding-box.");

                if (ImGui::Checkbox("Generate complete mipmap chains", &g_GenerateMipmaps)) {
                    g_CurrentPreset = 4;
                }
                ImGui::SameLine();
                HelpMarker("Generates complete mipmaps for baked patch CTextures to eliminate distant shimmering.");

                ImGui::Columns(1);
                ImGui::Dummy(ImVec2(0, 16));

                // Bottom Buttons: OK (Recompile) & Cancel
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

                if (ImGui::Button("OK", ImVec2(130, 30))) {
                    StartRecompilation(g_TargetGameRoot, g_WldInputBuffer);
                }
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 14.0f);

                // Secondary Action Button (Slate Dark)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));

                if (ImGui::Button("Cancel", ImVec2(100, 30))) {
                    ImGui::CloseCurrentPopup();
                }
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
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
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

                if (ImGui::Button("Cancel Compilation", ImVec2(160, 26))) {
                    g_CancelRequested = true;
                }
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

                // Open Output Folder Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.90f, 0.95f, 1.0f));
                if (ImGui::Button("Open in File Explorer", ImVec2(160, 28))) {
                    if (fs::exists(g_CompileResult.outputPath)) {
                        std::string param = "/select,\"" + g_CompileResult.outputPath + "\"";
                        ShellExecuteA(NULL, "open", "explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
                    }
                }
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 12.0f);

                // OK Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

                if (ImGui::Button("OK", ImVec2(100, 28))) {
                    g_Finished = false;
                    ImGui::CloseCurrentPopup();
                }
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
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.22f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.38f, 0.20f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));

                if (ImGui::Button("Copy Error", ImVec2(120, 28))) {
                    ImGui::SetClipboardText(g_ErrorMessage.c_str());
                }
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 12.0f);

                // Close Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

                if (ImGui::Button("Close", ImVec2(100, 28))) {
                    g_Finished = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(4);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(6);
        ImGui::PopStyleColor(13);
    }
}
