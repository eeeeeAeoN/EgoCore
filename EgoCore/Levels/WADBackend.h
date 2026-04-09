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

        if (ImGui::BeginPopupModal("Decompile WAD", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            if (!g_IsUnpacking && !g_UnpackFinished && !g_IsManualUnpack) {
                ImGui::Text("The FinalAlbion level folder is missing.");
                ImGui::Text("EgoCore needs to decompile 'FinalAlbion.wad' into its raw .lev and .tng components.");
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "This will also delete the original WAD (you don't need it) and patch userst.ini.");
                ImGui::Dummy(ImVec2(0, 5));

                static bool dontAskAgain = false;
                ImGui::Checkbox("Don't ask me again", &dontAskAgain);
                ImGui::Dummy(ImVec2(0, 10));

                if (ImGui::Button("Decompile Now", ImVec2(120, 0))) {
                    if (dontAskAgain) {
                        g_AppConfig.DisableWadPrompt = true;
                        SaveConfig();
                    }
                    StartSystemUnpack(g_TargetGameRoot);
                }
                ImGui::SameLine();
                if (ImGui::Button("Skip", ImVec2(120, 0))) {
                    if (dontAskAgain) {
                        g_AppConfig.DisableWadPrompt = true;
                        SaveConfig();
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (g_IsUnpacking) {
                ImGui::Text("Decompiling...");
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", g_UnpackStatus.c_str());
            }
            else if (g_UnpackFinished) {
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Decompilation Complete!");

                if (!g_IsManualUnpack) {
                    ImGui::Text("Original WAD removed and userst.ini patched.");
                }
                else {
                    ImGui::Text("Files extracted successfully to the WAD's directory.");
                }

                ImGui::Dummy(ImVec2(0, 10));
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    g_UnpackFinished = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }
}