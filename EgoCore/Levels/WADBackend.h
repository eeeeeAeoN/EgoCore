#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <algorithm>
#include "ConfigBackend.h"
#include "imgui.h"

namespace fs = std::filesystem;
extern ImFont* g_TitleFont;

namespace WADBackend {
    inline bool g_ShowWadPrompt = false;
    inline bool g_TriggerManualWadModal = false;
    inline std::atomic<bool> g_IsUnpacking{ false };
    inline bool g_UnpackFinished = false;
    inline bool g_IsManualUnpack = false;
    inline std::string g_UnpackStatus = "";
    inline std::string g_TargetGameRoot = "";

    inline bool RequiresUnpack(const std::string& gameRoot) {
        fs::path levelsDir = fs::path(gameRoot) / "Data" / "Levels";
        fs::path finalAlbionDir = levelsDir / "FinalAlbion";
        fs::path wadPath = levelsDir / "FinalAlbion.wad";

        return !fs::exists(finalAlbionDir) && fs::exists(wadPath);
    }

    inline void TriggerPrompt(const std::string& gameRoot) {
        if (!g_AppConfig.DisableWadPrompt && RequiresUnpack(gameRoot)) {
            g_TargetGameRoot = gameRoot;
            g_ShowWadPrompt = true;
            g_IsManualUnpack = false;
        }
    }

    inline void UnpackRoutine(std::string wadPathStr, bool isSystemSetup, std::string gameRoot) {
        fs::path wadPath = wadPathStr;
        fs::path outBaseDir = wadPath.parent_path();

        std::ifstream file(wadPath, std::ios::binary);
        if (!file.is_open()) {
            g_UnpackStatus = "Failed to open WAD!";
            g_IsUnpacking = false;
            g_UnpackFinished = true;
            return;
        }

        file.seekg(20, std::ios::beg);
        uint32_t entryCount = 0;
        file.read((char*)&entryCount, 4);

        file.seekg(28, std::ios::beg);
        uint32_t footOff = 0;
        file.read((char*)&footOff, 4);

        file.seekg(footOff, std::ios::beg);

        uint32_t statsCount = 0;
        file.read((char*)&statsCount, 4);
        file.seekg(statsCount * 8, std::ios::cur);

        for (uint32_t i = 0; i < entryCount; i++) {
            uint32_t magicE, id, type, eSize, eOffset, crc;
            file.read((char*)&magicE, 4);

            if (magicE != 42) {
                g_UnpackStatus = "Warning: Alignment lost at entry " + std::to_string(i);
                break;
            }

            file.read((char*)&id, 4);
            file.read((char*)&type, 4);
            file.read((char*)&eSize, 4);
            file.read((char*)&eOffset, 4);
            file.read((char*)&crc, 4);

            uint32_t nameLen = 0;
            file.read((char*)&nameLen, 4);
            std::string name;
            if (nameLen > 0) {
                name.resize(nameLen);
                file.read(&name[0], nameLen);
                name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
            }

            uint32_t timestamp, depCount;
            file.read((char*)&timestamp, 4);
            file.read((char*)&depCount, 4);
            for (uint32_t d = 0; d < depCount; d++) {
                uint32_t dLen = 0;
                file.read((char*)&dLen, 4);
                if (dLen > 0) file.seekg(dLen, std::ios::cur);
            }

            uint32_t infoSize = 0;
            file.read((char*)&infoSize, 4);
            if (infoSize > 0) file.seekg(infoSize, std::ios::cur);

            if (eSize > 0 && !name.empty()) {
                g_UnpackStatus = "Extracting: " + name;

                std::string cleanName = name;
                std::replace(cleanName.begin(), cleanName.end(), '/', '\\');

                size_t dlPos = cleanName.find("Data\\Levels\\");
                if (dlPos != std::string::npos) cleanName = cleanName.substr(dlPos + 12);

                fs::path outPath = outBaseDir / cleanName;
                fs::create_directories(outPath.parent_path());

                size_t tempPos = file.tellg();

                std::vector<uint8_t> buffer(eSize);
                file.seekg(eOffset, std::ios::beg);
                file.read((char*)buffer.data(), eSize);

                std::ofstream outFile(outPath, std::ios::binary);
                if (outFile.is_open()) {
                    outFile.write((char*)buffer.data(), eSize);
                }

                file.seekg(tempPos, std::ios::beg);
            }
        }

        file.close();

        if (isSystemSetup) {
            g_UnpackStatus = "Cleaning up WAD and patching userst.ini...";
            std::error_code ec;
            fs::remove(wadPath, ec);

            fs::path iniPath = fs::path(gameRoot) / "userst.ini";
            std::string iniContent;
            bool found = false;

            std::ifstream iniIn(iniPath);
            if (iniIn.is_open()) {
                std::string line;
                while (std::getline(iniIn, line)) {
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    if (line.find("UseLevelWAD") != std::string::npos) {
                        found = true;
                        iniContent += "UseLevelWAD FALSE;\n";
                    }
                    else {
                        iniContent += line + "\n";
                    }
                }
                iniIn.close();
            }

            if (!found) iniContent += "UseLevelWAD FALSE;\n";

            std::ofstream iniOut(iniPath);
            if (iniOut.is_open()) iniOut << iniContent;
        }

        g_IsUnpacking = false;
        g_UnpackFinished = true;
    }

    inline void StartSystemUnpack(const std::string& gameRoot) {
        g_IsUnpacking = true;
        g_UnpackFinished = false;
        g_UnpackStatus = "Reading Table of Contents...";
        fs::path wadPath = fs::path(gameRoot) / "Data" / "Levels" / "FinalAlbion.wad";
        std::thread(UnpackRoutine, wadPath.string(), true, gameRoot).detach();
    }

    inline void StartManualUnpack(const std::string& wadPath) {
        g_IsManualUnpack = true;
        g_IsUnpacking = true;
        g_UnpackFinished = false;
        g_UnpackStatus = "Reading Table of Contents...";
        g_TriggerManualWadModal = true;
        std::thread(UnpackRoutine, wadPath, false, "").detach();
    }

    inline void DrawWADModal() {
        if (g_ShowWadPrompt || g_TriggerManualWadModal) {
            ImGui::OpenPopup("Decompile WAD");
            g_ShowWadPrompt = false;
            g_TriggerManualWadModal = false;
        }

        // Modal Styling (Matches EgoCore Suite Theme)
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.70f, 0.30f, 0.35f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));

        ImGui::SetNextWindowSize(ImVec2(540, 270), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Decompile WAD", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
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

            // Custom Header Title
            if (g_TitleFont) ImGui::PushFont(g_TitleFont);
            ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Decompile WAD Archive");
            if (g_TitleFont) ImGui::PopFont();

            ImGui::Dummy(ImVec2(0, 4));

            // Separator Line with Gold Fade
            drawList->AddLine(
                ImVec2(winPos.x + 20.0f, ImGui::GetCursorScreenPos().y),
                ImVec2(winMax.x - 20.0f, ImGui::GetCursorScreenPos().y),
                IM_COL32(242, 193, 78, 110), 1.2f
            );
            ImGui::Dummy(ImVec2(0, 12));

            // --- STATE 1: DECOMPILE PROMPT ---
            if (!g_IsUnpacking && !g_UnpackFinished && !g_IsManualUnpack) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
                ImGui::TextWrapped("The FinalAlbion level folder is missing from your game installation.");
                ImGui::TextWrapped("EgoCore needs to decompile 'FinalAlbion.wad' into raw .lev and .tng components.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "This will delete the redundant WAD file and automatically patch userst.ini.");
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 8));

                static bool dontAskAgain = false;
                ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::Checkbox("Don't ask me again", &dontAskAgain);
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 12));

                // Upgraded Primary Action Button (Gold)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Decompile Now", ImVec2(140, 28))) {
                    if (dontAskAgain) {
                        g_AppConfig.DisableWadPrompt = true;
                        SaveConfig();
                    }
                    StartSystemUnpack(g_TargetGameRoot);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                ImGui::SameLine(0, 12.0f);

                // Upgraded Secondary Action Button (Slate Dark)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.24f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.32f, 0.42f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.14f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("Skip", ImVec2(100, 28))) {
                    if (dontAskAgain) {
                        g_AppConfig.DisableWadPrompt = true;
                        SaveConfig();
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);
            }
            // --- STATE 2: IN PROGRESS ---
            else if (g_IsUnpacking) {
                ImGui::TextColored(ImVec4(0.95f, 0.82f, 0.45f, 1.0f), "Decompiling WAD Archive...");
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "%s", g_UnpackStatus.c_str());
                ImGui::Dummy(ImVec2(0, 10));

                static int dots = 0;
                if (ImGui::GetFrameCount() % 20 == 0) dots = (dots + 1) % 4;
                std::string spinner = "Extracting levels";
                for (int i = 0; i < dots; i++) spinner += ".";
                ImGui::TextDisabled("%s", spinner.c_str());
            }
            // --- STATE 3: COMPLETE ---
            else if (g_UnpackFinished) {
                ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.50f, 1.0f), "Decompilation Complete!");
                ImGui::Dummy(ImVec2(0, 4));

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.95f, 0.95f));
                if (!g_IsManualUnpack) {
                    ImGui::TextWrapped("Original WAD removed and userst.ini successfully patched.");
                }
                else {
                    ImGui::TextWrapped("Files extracted successfully to the WAD's target directory.");
                }
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 16));

                float closeBtnWidth = 120.0f;
                ImGui::SetCursorPosX((winSize.x - closeBtnWidth) * 0.5f);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.68f, 0.25f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.78f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.55f, 0.18f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                if (ImGui::Button("OK", ImVec2(closeBtnWidth, 28))) {
                    g_UnpackFinished = false;
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