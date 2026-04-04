#pragma once
#include "imgui.h"
#include <vector>
#include <string>
#include <algorithm>

// Include our newly consolidated backend and compiler
#include "ModManagerBackend.h"
#include "ModManagerCompiler.h"

// Forward declarations for global app state (assuming these are in your main app file)
enum class EAppState;
extern EAppState g_CurrentAppState;
extern std::string g_SuccessMessage;
extern bool g_ShowSuccessPopup;

// ==========================================================================================
// UI STATE GLOBALS
// ==========================================================================================
inline int g_ModToDeleteIndex = -1;
inline bool g_ShowModPackageWindow = false;
inline std::vector<StagedModPackageEntry> g_ModPackageEntries;
inline char g_ModNameBuffer[128] = "";

// ==========================================================================================
// 1. MAIN MOD MANAGER WINDOW
// ==========================================================================================
inline void DrawModManagerWindow() {
    bool triggerDeletePopup = false;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    if (ImGui::Begin("Mods Manager UI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {

        ImGui::SetWindowFocus();

        if (ImGui::Button("Return to Main Menu", ImVec2(200, 40))) {
            g_CurrentAppState = (EAppState)1;
        }

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();

        ImGui::AlignTextToFramePadding();

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::TextWrapped("Drag and drop any row to adjust the mod load order. Right-click a row for mod options. DLL Mods require FableLauncher.exe to inject.");
        ImGui::Dummy(ImVec2(0, 5));

        if (ImGui::BeginTable("ModsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Mod Name", ImGuiTableColumnFlags_WidthFixed, 300);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Details & Settings", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (int i = 0; i < (int)ModManagerBackend::g_LoadedMods.size(); i++) {
                auto& mod = ModManagerBackend::g_LoadedMods[i];
                ImGui::PushID(i);
                ImGui::TableNextRow();

                // 1. Order (Full Row Drag & Context Menu)
                ImGui::TableSetColumnIndex(0);

                char rowLabel[32];
                snprintf(rowLabel, sizeof(rowLabel), "##row%d", i);

                ImGui::SetNextItemAllowOverlap();
                ImGui::Selectable(rowLabel, false, ImGuiSelectableFlags_SpanAllColumns);

                // --- Drag and Drop Source ---
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("MOD_ORDER_PAYLOAD", &i, sizeof(int));
                    ImGui::Text("Moving: %s", mod.Name.c_str());
                    ImGui::EndDragDropSource();
                }

                // --- Drag and Drop Target ---
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MOD_ORDER_PAYLOAD")) {
                        int sourceIdx = *(const int*)payload->Data;
                        if (sourceIdx != i) {
                            auto temp = ModManagerBackend::g_LoadedMods[sourceIdx];
                            ModManagerBackend::g_LoadedMods.erase(ModManagerBackend::g_LoadedMods.begin() + sourceIdx);
                            ModManagerBackend::g_LoadedMods.insert(ModManagerBackend::g_LoadedMods.begin() + i, temp);

                            ModManagerBackend::SaveLoadOrder();
                            ModManagerBackend::UpdateGlobalModsIni();

                            g_AppConfig.ModSystemDirty = true;
                            SaveConfig();
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // --- Right-Click Context Menu ---
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Mod Options");
                    ImGui::Separator();

                    if (mod.IsCoreMod) {
                        ImGui::TextDisabled("Cannot delete Core Mods");
                    }
                    else {
                        if (ImGui::MenuItem("Delete Mod Permanently")) {
                            g_ModToDeleteIndex = i;
                            triggerDeletePopup = true;
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::SameLine();
                ImGui::Text("  %d", i + 1);

                // 2. Enabled Toggle
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Checkbox("##enabled", &mod.IsEnabled)) {
                    ModManagerBackend::UpdateGlobalModsIni();
                    ModManagerBackend::SaveLoadOrder();
                    g_AppConfig.ModSystemDirty = true;
                    SaveConfig();
                }

                // 3. Name
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", mod.Name.c_str());

                // 4. Type
                ImGui::TableSetColumnIndex(3);
                if (mod.IsCoreMod) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Core .DLL");
                }
                else if (mod.HasDll) {
                    ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), ".DLL Plugin");
                }
                else if (mod.IsAssetMod) {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Asset Mod");
                }
                else {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Data Mod");
                }

                // 5. Expandable Details 
                ImGui::TableSetColumnIndex(4);
                bool treeNodeOpen = ImGui::TreeNodeEx("View Details & Settings", ImGuiTreeNodeFlags_SpanFullWidth);

                if (treeNodeOpen) {
                    ImGui::Dummy(ImVec2(0, 5));

                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Description:");
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                    ImGui::BeginChild("desc", ImVec2(0, 80), true);
                    ImGui::TextWrapped("%s", mod.Description.c_str());
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    if (!mod.SettingsLines.empty()) {
                        ImGui::Dummy(ImVec2(0, 5));
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Settings:");

                        ImGui::BeginChild("settings", ImVec2(0, 150), true);
                        bool settingsChanged = false;
                        for (size_t sIdx = 0; sIdx < mod.SettingsLines.size(); sIdx++) {
                            auto& line = mod.SettingsLines[sIdx];

                            if (line.IsKeyValue) {
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", line.Key.c_str());
                                ImGui::SameLine(250);

                                ImGui::PushID((int)sIdx);
                                ImGui::SetNextItemWidth(200);

                                char valBuf[128];
                                strncpy_s(valBuf, sizeof(valBuf), line.Value.c_str(), _TRUNCATE);

                                if (ImGui::InputText("##val", valBuf, 128)) {
                                    line.Value = valBuf;
                                    settingsChanged = true;
                                }
                                ImGui::PopID();
                            }
                            else if (!line.Raw.empty() && line.Raw[0] == ';') {
                                ImGui::TextDisabled("%s", line.Raw.c_str());
                            }
                        }
                        ImGui::EndChild();

                        if (settingsChanged) ModManagerBackend::SaveModSettings(mod);
                    }
                    else if (fs::exists(mod.ModFolderPath + "\\" + mod.Name + ".ini")) {
                        ImGui::TextDisabled("Settings file found but is empty or malformed.");
                    }
                    else {
                        ImGui::TextDisabled("No configuration file (.ini) provided with this mod.");
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // --- Delete Confirmation Popup ---
        if (triggerDeletePopup) ImGui::OpenPopup("Delete Mod?");

        if (ImGui::BeginPopupModal("Delete Mod?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to permanently delete this mod?");
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "This will erase the folder from your hard drive.");
            ImGui::Separator();

            if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                ModManagerBackend::DeleteMod(g_ModToDeleteIndex);
                g_AppConfig.ModSystemDirty = true;
                SaveConfig();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

// ==========================================================================================
// 2. MOD PACKAGE CREATION WINDOW
// ==========================================================================================
inline void DrawModPackageWindow() {
    if (!g_ShowModPackageWindow) return;

    ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Create Mod Package", &g_ShowModPackageWindow)) {

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Mod Name:");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##modname", g_ModNameBuffer, 128);
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        float btnWidth = (ImGui::GetContentRegionAvail().x / 2.0f) - 4.0f;

        if (ImGui::Button("Auto-Add Changed Entries", ImVec2(btnWidth, 30))) {
            for (const auto& bank : g_OpenBanks) {
                for (size_t i = 0; i < bank.Entries.size(); ++i) {
                    if (bank.StagedEntries.count(i) || bank.ModifiedEntryData.count(i)) {

                        std::string currentSubBank = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                        bool exists = false;
                        for (const auto& existing : g_ModPackageEntries) {
                            if (existing.EntryID == bank.Entries[i].ID && existing.BankName == bank.FileName && existing.SubBankName == currentSubBank) {
                                exists = true; break;
                            }
                        }
                        if (!exists) {
                            StagedModPackageEntry staged;
                            staged.EntryID = bank.Entries[i].ID;
                            staged.EntryName = bank.Entries[i].Name;
                            staged.EntryType = bank.Entries[i].Type;
                            staged.TypeName = GetEntryTypeName(bank.Type, bank.Entries[i].Type, bank.FileName);
                            staged.BankName = bank.FileName;
                            staged.SourceFullPath = bank.FullPath;
                            staged.SubBankName = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                            g_ModPackageEntries.push_back(staged);
                        }
                    }
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Auto-Add Marked Entries", ImVec2(btnWidth, 30))) {
            for (const auto& tracked : ModPackageTracker::g_MarkedEntries) {
                bool exists = false;
                for (const auto& existing : g_ModPackageEntries) {
                    if (existing.EntryName == tracked.EntryName && existing.BankName == tracked.BankName) {
                        exists = true; break;
                    }
                }
                if (!exists) g_ModPackageEntries.push_back(tracked);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginTable("ModPackageTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0, -50))) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 130);
            ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Sub-Bank", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < g_ModPackageEntries.size(); i++) {
                auto& e = g_ModPackageEntries[i];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0); ImGui::Text("%u", e.EntryID);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", e.EntryName.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::Text("%s", e.TypeName.c_str());
                ImGui::TableSetColumnIndex(3); ImGui::Text("%s", e.BankName.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::Text("%s", e.SubBankName.c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::PushID((int)i);
                if (ImGui::Button("X")) {
                    g_ModPackageEntries.erase(g_ModPackageEntries.begin() + i);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // --- Drag and Drop Logic for Adding Individual Entries ---
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BANK_ENTRY_PAYLOAD")) {

                // 0. COMPILER TRIGGER: Force all unsaved entries to recompile to update definitions before pulling
                for (auto& b : g_OpenBanks) {
                    if (!b.StagedEntries.empty() || !b.ModifiedEntryData.empty()) {
                        if (b.Type == EBankType::Audio) SaveAudioBank(&b);
                        else SaveBigBank(&b);
                    }
                }

                int* data = (int*)payload->Data;
                int bIdx = data[0];
                int eIdx = data[1];

                if (bIdx >= 0 && bIdx < (int)g_OpenBanks.size()) {
                    auto& bank = g_OpenBanks[bIdx];
                    if (eIdx >= 0 && eIdx < (int)bank.Entries.size()) {
                        auto& entry = bank.Entries[eIdx];

                        std::string currentSubBank = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : std::string("N/A");

                        // 1. THE LOGIC GATES: Prevent RAW Dependency Injection
                        std::string lowerBank = bank.FileName;
                        std::transform(lowerBank.begin(), lowerBank.end(), lowerBank.begin(), ::tolower);
                        std::string lowerSubBank = currentSubBank;
                        std::transform(lowerSubBank.begin(), lowerSubBank.end(), lowerSubBank.begin(), ::tolower);

                        // Gate A: Audio/LipSync Lock
                        bool isDisallowedNewAudio = false;
                        if (lowerBank == "dialogue.lut" || (lowerSubBank.find("lipsync_") != std::string::npos && lowerSubBank.find("_main") != std::string::npos && lowerSubBank.find("_2") == std::string::npos)) {
                            if (entry.ID > 12134) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "dialogue2.lut" || lowerSubBank.find("_main_2") != std::string::npos) {
                            if (entry.ID > 1) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "scriptdialogue.lut" || (lowerSubBank.find("_script") != std::string::npos && lowerSubBank.find("_2") == std::string::npos)) {
                            if (entry.ID > 5310) isDisallowedNewAudio = true;
                        }
                        else if (lowerBank == "scriptdialogue2.lut" || lowerSubBank.find("_script_2") != std::string::npos) {
                            if (entry.ID > 3060) isDisallowedNewAudio = true;
                        }

                        if (isDisallowedNewAudio) {
                            g_SuccessMessage = "Directly adding new audio/lipsync entries is locked!\nDrag the matching Text.big entry instead to auto-link dependencies.";
                            g_ShowSuccessPopup = true;
                            ImGui::EndDragDropTarget();
                            return;
                        }

                        // Gate B: Streaming Font GlyphData Lock
                        if (bank.Type == EBankType::Fonts && entry.Type == 2) {
                            g_SuccessMessage = "GlyphData cannot be added manually!\nDrag a Streaming Font entry instead to auto-link its GlyphData.";
                            g_ShowSuccessPopup = true;
                            ImGui::EndDragDropTarget();
                            return;
                        }

                        // Helper lambda to add entries to avoid duplicate code
                        auto addEntryToPackage = [&](uint32_t id, const std::string& name, int32_t type, EBankType bType, const std::string& bName, const std::string& sbName, const std::string& fullPath) {
                            bool exists = false;
                            for (const auto& existing : g_ModPackageEntries) {
                                if (existing.EntryID == id && existing.BankName == bName && existing.SubBankName == sbName) {
                                    exists = true; break;
                                }
                            }
                            if (!exists) {
                                StagedModPackageEntry staged;
                                staged.EntryID = id; staged.EntryName = name; staged.EntryType = type;
                                staged.BankType = bType; staged.BankName = bName; staged.SubBankName = sbName;
                                staged.SourceFullPath = fullPath;
                                staged.TypeName = GetEntryTypeName(bType, type, bName);
                                g_ModPackageEntries.push_back(staged);
                            }
                            };

                        // Add the primary dragged entry
                        addEntryToPackage(entry.ID, entry.Name, entry.Type, bank.Type, bank.FileName, currentSubBank, bank.FullPath);

                        // 2. THE AUDIO CASCADE: Auto-Pull Linked Audio using Text Entry's SpeechBank
                        if (bank.Type == EBankType::Text && entry.Type == 0) {
                            std::string expectedSND = "SND_" + entry.Name;
                            std::transform(expectedSND.begin(), expectedSND.end(), expectedSND.begin(), ::toupper);

                            // --- Fetch the SpeechBank directly from the Text Entry Data ---
                            std::string speechBank = "";
                            if (bank.ModifiedEntryData.count(eIdx)) {
                                CTextParser p; p.Parse(bank.ModifiedEntryData[eIdx], 0);
                                if (p.IsParsed) speechBank = p.TextData.SpeechBank;
                            }
                            else {
                                bank.Stream->clear();
                                bank.Stream->seekg(entry.Offset, std::ios::beg);
                                std::vector<uint8_t> buf(entry.Size);
                                bank.Stream->read((char*)buf.data(), entry.Size);
                                CTextParser p; p.Parse(buf, 0);
                                if (p.IsParsed) speechBank = p.TextData.SpeechBank;
                            }

                            std::transform(speechBank.begin(), speechBank.end(), speechBank.begin(), ::tolower);

                            if (!speechBank.empty()) {
                                std::string targetHeader = "";
                                std::string targetLutName = "";
                                std::string targetSubBankKeyword = "";

                                // Map the specific .lug declaration to the true engine files
                                if (speechBank == "dialogue.lug") { targetHeader = "dialoguesnds.h"; targetLutName = "dialogue.lut"; targetSubBankKeyword = "_main"; }
                                else if (speechBank == "dialogue2.lug") { targetHeader = "dialoguesnds2.h"; targetLutName = "dialogue2.lut"; targetSubBankKeyword = "_main_2"; }
                                else if (speechBank == "scriptdialogue.lug") { targetHeader = "scriptdialoguesnds.h"; targetLutName = "scriptdialogue.lut"; targetSubBankKeyword = "_script"; }
                                else if (speechBank == "scriptdialogue2.lug") { targetHeader = "scriptdialoguesnds2.h"; targetLutName = "scriptdialogue2.lut"; targetSubBankKeyword = "_script_2"; }

                                if (!targetHeader.empty()) {
                                    uint32_t targetAudioID = 0;
                                    std::regex idRegex(expectedSND + R"(\s*=\s*(\d+))");

                                    // Fast targeted scan - we only look inside the specific header now
                                    for (const auto& enumEntry : g_DefWorkspace.AllEnums) {
                                        if (fs::path(enumEntry.FilePath).filename().string() == targetHeader) {
                                            std::smatch match;
                                            if (std::regex_search(enumEntry.FullContent, match, idRegex)) {
                                                targetAudioID = std::stoul(match[1].str());
                                            }
                                            break;
                                        }
                                    }

                                    if (targetAudioID > 0) {
                                        auto loadMissingMediaBank = [&](const std::string& bName) {
                                            bool found = false;
                                            for (auto& b : g_OpenBanks) {
                                                std::string lowerB = b.FileName;
                                                std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
                                                if (lowerB == bName) { found = true; break; }
                                            }
                                            if (!found) {
                                                const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
                                                for (const char* l : langs) {
                                                    std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\" + bName;
                                                    if (fs::exists(p)) { LoadBank(p); break; }
                                                }
                                            }
                                            };

                                        loadMissingMediaBank(targetLutName);
                                        loadMissingMediaBank("dialogue.big");

                                        for (auto& openBank : g_OpenBanks) {
                                            // Stage the .lut Audio Entry
                                            if (openBank.Type == EBankType::Audio && openBank.FileName.find(".lut") != std::string::npos) {
                                                std::string lowerB = openBank.FileName;
                                                std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
                                                if (lowerB == targetLutName) {
                                                    for (auto& depEntry : openBank.Entries) {
                                                        if (depEntry.ID == targetAudioID) {
                                                            addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, openBank.Type, openBank.FileName, "N/A", openBank.FullPath);
                                                        }
                                                    }
                                                }
                                            }
                                            // Stage the dialogue.big LipSync Entry
                                            else if (openBank.Type == EBankType::Dialogue) {
                                                int targetSbIdx = -1;
                                                for (int s = 0; s < openBank.SubBanks.size(); s++) {
                                                    std::string lowerSub = openBank.SubBanks[s].Name;
                                                    std::transform(lowerSub.begin(), lowerSub.end(), lowerSub.begin(), ::tolower);

                                                    // Ensure strict matching so _MAIN doesn't accidentally trigger _MAIN_2
                                                    if (targetSubBankKeyword == "_main" && lowerSub.find("_main_2") != std::string::npos) continue;
                                                    if (targetSubBankKeyword == "_script" && lowerSub.find("_script_2") != std::string::npos) continue;

                                                    if (lowerSub.find(targetSubBankKeyword) != std::string::npos) {
                                                        targetSbIdx = s; break;
                                                    }
                                                }

                                                if (targetSbIdx != -1) {
                                                    // CRITICAL: Force load the correct sub-bank into RAM before we scan it for packaging
                                                    if (openBank.ActiveSubBankIndex != targetSbIdx) {
                                                        LoadSubBankEntries(&openBank, targetSbIdx);
                                                    }

                                                    std::string depSubBank = openBank.SubBanks[openBank.ActiveSubBankIndex].Name;
                                                    for (auto& depEntry : openBank.Entries) {
                                                        if (depEntry.ID == targetAudioID) {
                                                            addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, openBank.Type, openBank.FileName, depSubBank, openBank.FullPath);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // 3. THE FONT CASCADE: Auto-Pull GlyphData for Streaming Fonts
                        if (bank.Type == EBankType::Fonts && lowerSubBank.find("streaming") != std::string::npos && entry.Type != 2) {
                            // Find the matching GlyphData (Type 2) in this exact subbank
                            for (auto& depEntry : bank.Entries) {
                                if (depEntry.Type == 2) {
                                    addEntryToPackage(depEntry.ID, depEntry.Name, depEntry.Type, bank.Type, bank.FileName, currentSubBank, bank.FullPath);
                                }
                            }
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 65);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Warning: Building the package will automatically compile and save all staged entries to your active banks.");

        bool canBuild = !g_ModPackageEntries.empty() && strlen(g_ModNameBuffer) > 0;

        ImGui::BeginDisabled(!canBuild);
        if (ImGui::Button("Build Mod Package", ImVec2(-1, 30))) {

            ModManagerCompiler::BuildPackageStructure(g_ModNameBuffer, g_ModPackageEntries);

            g_SuccessMessage = "Mod folder structure & files created successfully!";
            g_ShowSuccessPopup = true;
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}