#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "AudioExplorer.h"
#include "BankEditor.h" 
#include <thread>

// --- STATE FOR POPUPS ---
static int g_ContextEntryIndex = -1;
static bool g_ShowDeleteBankEntryPopup = false;
static bool g_ShowAddEntryPopup = false;

static void DrawBinaryTab() {
    static bool isCompilingBins = false;
    static std::string compileBinStatus = "";
    static bool showBinResult = false;

    if (ImGui::Button("Load Binaries from Data/Defs")) {
        LoadSystemBinaries(g_AppConfig.GameRootPath);
    }

    ImGui::SameLine();

    if (ImGui::Button("Compile Sound Binaries")) {
        compileBinStatus = "Starting compilation...";
        isCompilingBins = true;
        ImGui::OpenPopup("Compiling Binaries");

        std::thread([&]() {
            std::string log = "";
            std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
            BinaryParser::CompileSoundBinaries(defsPath, log);
            compileBinStatus = log;
            isCompilingBins = false;
            showBinResult = true;
            }).detach();
    }

    if (ImGui::BeginPopupModal("Compiling Binaries", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (isCompilingBins) {
            ImGui::Text("Compiling binaries...");
            static int dots = 0;
            std::string s = ""; for (int i = 0; i < dots / 10; ++i) s += ".";
            ImGui::Text("%s", s.c_str());
            dots = (dots + 1) % 40;
        }
        else {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (showBinResult) {
        ImGui::OpenPopup("Binary Compile Result");
        showBinResult = false;
    }
    if (ImGui::BeginPopupModal("Binary Compile Result", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Compilation Report:");
        ImGui::Separator();
        ImGui::TextUnformatted(compileBinStatus.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            LoadSystemBinaries(g_AppConfig.GameRootPath);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (g_LoadedBinaries.empty()) {
        ImGui::TextDisabled("No binary definitions loaded.");
        return;
    }

    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Manual CRC Checker");
    static char buf[128] = "";
    ImGui::InputText("Sound Name", buf, 128);
    if (buf[0] != 0) {
        uint32_t hash = BinaryParser::CalculateCRC32_Fable(buf);
        ImGui::SameLine();
        ImGui::TextDisabled("Fable CRC: %08X", hash);

        bool found = false;
        for (const auto& parser : g_LoadedBinaries) {
            for (const auto& e : parser.Data.Entries) {
                if (e.CRC == hash) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "MATCH: %s -> ID %d", parser.Data.FileName.c_str(), e.ID);
                    found = true;
                }
            }
        }
        if (!found) ImGui::TextColored(ImVec4(1, 0, 0, 1), "No match found.");
    }
    ImGui::Separator();

    if (ImGui::BeginTabBar("BinaryFilesTab", ImGuiTabBarFlags_Reorderable)) {
        for (int i = 0; i < (int)g_LoadedBinaries.size(); ++i) {
            auto& parser = g_LoadedBinaries[i];
            if (ImGui::BeginTabItem(parser.Data.FileName.c_str())) {
                if (ImGui::BeginTable("BinTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50);
                    ImGui::TableSetupColumn("CRC32", ImGuiTableColumnFlags_WidthFixed, 100);
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    ImGuiListClipper clipper;
                    clipper.Begin((int)parser.Data.Entries.size());
                    while (clipper.Step()) {
                        for (int k = clipper.DisplayStart; k < clipper.DisplayEnd; k++) {
                            const auto& e = parser.Data.Entries[k];
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", k);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%08X", e.CRC);
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", e.ID);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

static void DrawBankTab() {
    static float bankSidebarWidth = 300.0f;
    if (bankSidebarWidth < 50.0f) bankSidebarWidth = 50.0f;
    if (bankSidebarWidth > ImGui::GetWindowWidth() - 100.0f) bankSidebarWidth = ImGui::GetWindowWidth() - 100.0f;

    // --- SUCCESS MODAL ---
    if (g_ShowSuccessPopup) {
        ImGui::OpenPopup("Success");
        g_ShowSuccessPopup = false;
    }

    if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", g_SuccessMessage.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginTabBar("BankFiles", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

        for (int i = 0; i < (int)g_OpenBanks.size(); ) {
            LoadedBank& bank = g_OpenBanks[i];
            bool keepOpen = true;
            std::string tabLabel = bank.FileName + "##" + std::to_string(i);

            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (g_ForceTabSwitch && g_ActiveBankIndex == i) {
                flags |= ImGuiTabItemFlags_SetSelected;
            }

            if (ImGui::BeginTabItem(tabLabel.c_str(), &keepOpen, flags)) {
                if (g_ActiveBankIndex != i) {
                    g_ActiveBankIndex = i;
                    if (bank.SelectedEntryIndex != -1) SelectEntry(&bank, bank.SelectedEntryIndex);
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && bank.SelectedEntryIndex != -1) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                        if (g_AppConfig.ShowBankDeleteConfirm) {
                            g_ContextEntryIndex = bank.SelectedEntryIndex;
                            g_ShowDeleteBankEntryPopup = true;
                            ImGui::OpenPopup("Delete Bank Entry?");
                        }
                        else {
                            DeleteBankEntry(&bank, bank.SelectedEntryIndex);
                        }
                    }
                }

                ImGui::BeginChild("LeftPane", ImVec2(bankSidebarWidth, 0), true);

                // --- RECOMPILE BUTTONS ---
                if (bank.Type == EBankType::Text) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                    if (ImGui::Button("Recompile Text Bank (.BIG)", ImVec2(-FLT_MIN, 30))) {
                        SaveBigBank(&bank);
                    }
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                }
                else if (bank.Type == EBankType::Dialogue) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.2f, 0.8f, 1.0f));
                    if (ImGui::Button("Recompile Dialogue Bank", ImVec2(-FLT_MIN, 30))) {
                        SaveBigBank(&bank);
                    }
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                }
                else if (bank.Type == EBankType::Audio) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
                    if (ImGui::Button("Recompile Audio Bank (.LUT)", ImVec2(-FLT_MIN, 30))) {
                        SaveLutBank(&bank);
                    }
                    ImGui::PopStyleColor();
                    ImGui::Separator();
                }

                if (bank.Type != EBankType::Audio && !bank.SubBanks.empty()) {
                    float avail = ImGui::GetContentRegionAvail().x;
                    ImGui::SetNextItemWidth(avail - 30);

                    std::string preview = "Select Sub-Bank";
                    if (bank.ActiveSubBankIndex >= 0) preview = bank.SubBanks[bank.ActiveSubBankIndex].Name;

                    if (ImGui::BeginCombo("##folder", preview.c_str())) {
                        for (int s = 0; s < (int)bank.SubBanks.size(); s++) {
                            bool is_sel = (bank.ActiveSubBankIndex == s);
                            std::string itemLabel = bank.SubBanks[s].Name + " (" + std::to_string(bank.SubBanks[s].EntryCount) + ")";
                            if (ImGui::Selectable(itemLabel.c_str(), is_sel)) {
                                LoadSubBankEntries(&bank, s);
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("+", ImVec2(22, 0))) {
                        if (bank.Type == EBankType::Text) {
                            g_ShowAddEntryPopup = true;
                            ImGui::OpenPopup("Add Entry Type");
                        }
                        else if (bank.Type == EBankType::Dialogue) {
                            CreateNewDialogueEntry(&bank);
                        }
                        else {
                            g_BankStatus = "Add Entry not implemented for this bank type.";
                        }
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Entry");

                    ImGui::Separator();
                }
                else if (bank.Type == EBankType::Audio) {
                    if (ImGui::Button("+ Add Audio (WAV)", ImVec2(-FLT_MIN, 0))) {
                        std::string openPath = OpenFileDialog("WAV File\0*.wav\0");
                        if (!openPath.empty()) {
                            uint32_t nextID = GetNextFreeID(&bank);
                            if (bank.AudioParser->AddEntry(nextID, openPath)) {
                                // Rebuild List with Friendly Names
                                std::string headerName = GetHeaderName(bank.FileName);
                                std::map<uint32_t, std::string> friendlyNames = BuildFriendlyNameMap(headerName);

                                bank.Entries.clear();
                                bank.FilteredIndices.clear();
                                for (size_t k = 0; k < bank.AudioParser->Entries.size(); k++) {
                                    const auto& ae = bank.AudioParser->Entries[k];
                                    BankEntry be; be.ID = ae.SoundID; be.Name = "Sound ID " + std::to_string(ae.SoundID);

                                    if (friendlyNames.count(be.ID)) be.FriendlyName = friendlyNames[be.ID];
                                    else be.FriendlyName = be.Name;

                                    be.Size = ae.Length; be.Offset = ae.Offset; be.Type = 999;
                                    bank.Entries.push_back(be);
                                    bank.FilteredIndices.push_back((int)k);
                                }
                                UpdateFilter(bank);
                            }
                        }
                    }
                    ImGui::Separator();
                }

                ImGui::InputText("Search", bank.FilterText, 128);
                if (ImGui::IsItemEdited()) UpdateFilter(bank);

                if (ImGui::BeginPopupModal("Add Entry Type", &g_ShowAddEntryPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Select Entry Type:");
                    ImGui::Separator();

                    if (ImGui::Button("Text Entry (Type 0)", ImVec2(200, 0))) {
                        CreateNewTextEntry(&bank, 0);
                        g_ShowAddEntryPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::Button("Group Entry (Type 1)", ImVec2(200, 0))) {
                        CreateNewTextEntry(&bank, 1);
                        g_ShowAddEntryPopup = false;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("Type 2 (Narrator List) not supported.");

                    if (ImGui::Button("Cancel", ImVec2(200, 0))) {
                        g_ShowAddEntryPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (!bank.Entries.empty()) {
                    ImGui::BeginChild("ListScroll", ImVec2(0, 0), false);

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !bank.FilteredIndices.empty()) {
                        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                            int direction = ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? 1 : -1;
                            int currentPos = -1;
                            for (int k = 0; k < bank.FilteredIndices.size(); k++) {
                                if (bank.FilteredIndices[k] == bank.SelectedEntryIndex) { currentPos = k; break; }
                            }

                            if (currentPos == -1) {
                                currentPos = (direction == 1) ? -1 : (int)bank.FilteredIndices.size();
                            }

                            int newPos = std::clamp(currentPos + direction, 0, (int)bank.FilteredIndices.size() - 1);
                            SelectEntry(&bank, bank.FilteredIndices[newPos]);
                        }
                    }

                    for (int idx : bank.FilteredIndices) {
                        const auto& e = bank.Entries[idx];

                        std::string label = e.FriendlyName + "##" + std::to_string(e.ID);

                        if (ImGui::Selectable(label.c_str(), bank.SelectedEntryIndex == idx)) {
                            SelectEntry(&bank, idx);
                        }

                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Duplicate Entry")) {
                                DuplicateBankEntry(&bank, idx);
                            }
                            if (ImGui::MenuItem("Delete Entry")) {
                                if (g_AppConfig.ShowBankDeleteConfirm) {
                                    g_ContextEntryIndex = idx;
                                    g_ShowDeleteBankEntryPopup = true;
                                    ImGui::OpenPopup("Delete Bank Entry?");
                                }
                                else {
                                    DeleteBankEntry(&bank, idx);
                                }
                            }
                            ImGui::EndPopup();
                        }

                        if (bank.SelectedEntryIndex == idx) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();

                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitter", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                if (ImGui::IsItemActive()) bankSidebarWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                ImGui::BeginChild("RightPane", ImVec2(0, 0), true);

                if (bank.SelectedEntryIndex != -1) {
                    const auto& e = bank.Entries[bank.SelectedEntryIndex];

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("ID: %d | Type: %d | Size: %d bytes", e.ID, e.Type, e.Size);

                    // Right: Actions
                    ImGui::SameLine();
                    float avail = ImGui::GetContentRegionAvail().x;
                    float buttonsWidth = 140.0f; // Save(70) + Delete(60) + Spacing
                    if (avail > buttonsWidth) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - buttonsWidth);
                    }

                    // Save Button
                    if (ImGui::Button("Save", ImVec2(60, 0))) {
                        SaveEntryChanges(&bank);
                    }

                    ImGui::SameLine();

                    // Delete Button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button("Delete", ImVec2(60, 0))) {
                        if (g_AppConfig.ShowBankDeleteConfirm) {
                            g_ContextEntryIndex = bank.SelectedEntryIndex;
                            g_ShowDeleteBankEntryPopup = true;
                            ImGui::OpenPopup("Delete Bank Entry?");
                        }
                        else {
                            DeleteBankEntry(&bank, bank.SelectedEntryIndex);
                        }
                    }
                    ImGui::PopStyleColor();
                    // -------------------------

                    ImGui::Separator();

                    if (e.Name != e.FriendlyName) {
                        ImGui::TextDisabled("Internal File Name: %s", e.Name.c_str());
                    }

                    if (bank.Type == EBankType::Audio) {
                        DrawAudioProperties(&bank);
                    }
                    else if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
                        std::string lodPreview = "LOD " + std::to_string(bank.SelectedLOD);
                        if (ImGui::BeginCombo("##lod", lodPreview.c_str())) {
                            for (uint32_t l = 0; l < g_ActiveMeshContent.EntryMeta.LODCount; l++) {
                                if (ImGui::Selectable(("LOD " + std::to_string(l)).c_str(), bank.SelectedLOD == l)) {
                                    bank.SelectedLOD = l;
                                    ParseSelectedLOD(&bank);
                                }
                            }
                            ImGui::EndCombo();
                        }
                    }

                    ImGui::Separator();

                    if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) {
                        DrawTextureProperties();
                    }
                    else if (bank.Type == EBankType::Text) {
                        DrawTextProperties(&bank,
                            [&]() { SaveEntryChanges(&bank); },
                            [&](std::string target, uint32_t id, std::string hint) { JumpToBankEntry(target, id, hint); }
                        );
                    }
                    else if (bank.Type == EBankType::Dialogue) {
                        DrawLipSyncProperties(&bank,
                            [&]() { SaveEntryChanges(&bank); },
                            nullptr
                        );
                    }
                    else if (IsSupportedMesh(e.Type)) {
                        DrawMeshProperties([&]() { SaveEntryChanges(&bank); });
                    }
                    else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
                        DrawAnimProperties(g_ActiveAnim, g_AnimParseSuccess, g_AnimUIState, [&]() { SaveEntryChanges(&bank); });
                    }
                }
                ImGui::EndChild();

                if (g_ShowDeleteBankEntryPopup) {
                    ImGui::OpenPopup("Delete Bank Entry?");
                }

                if (ImGui::BeginPopupModal("Delete Bank Entry?", &g_ShowDeleteBankEntryPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Are you sure you want to delete this entry?");
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "This action cannot be undone.");
                    ImGui::Separator();

                    static bool dontShowAgain = false;
                    ImGui::Checkbox("Don't show this again", &dontShowAgain);

                    if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                        if (dontShowAgain) {
                            g_AppConfig.ShowBankDeleteConfirm = false;
                            SaveConfig();
                        }
                        DeleteBankEntry(&bank, g_ContextEntryIndex);
                        g_ShowDeleteBankEntryPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        g_ShowDeleteBankEntryPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::EndTabItem();
            }

            if (!keepOpen) {
                g_OpenBanks.erase(g_OpenBanks.begin() + i);
                if (g_OpenBanks.empty()) g_ActiveBankIndex = -1;
                else if (g_ActiveBankIndex >= i) g_ActiveBankIndex = (std::max)(0, g_ActiveBankIndex - 1);
            }
            else {
                i++;
            }
        }

        g_ForceTabSwitch = false;

        if (ImGui::TabItemButton("+ Load Bank (.BIG / .LUT)", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
            std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut\0All Files\0*.*\0");
            if (!path.empty()) LoadBank(path);
        }

        ImGui::EndTabBar();
    }
}