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

// --- STATE FOR TEXTURE IMPORT POPUP ---
static bool g_ShowTexImportPopup = false;
static std::string g_PendingImportPath = "";
static int g_ImportFormat = 1; // 0: DXT1, 1: DXT3, 2: DXT5, 3: ARGB
static int g_ImportType = 0;   // 0: Graphic, 2: Bumpmap, 5: Flat Sequence
static bool g_ScrollToSelected = false;

static bool g_ShowLeftPanel = true;

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

    // --- TEXTURE IMPORT MODAL ---
    if (g_ShowTexImportPopup) {
        ImGui::OpenPopup("Import Texture Options");
    }

    if (ImGui::BeginPopupModal("Import Texture Options", &g_ShowTexImportPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File: %s", std::filesystem::path(g_PendingImportPath).filename().string().c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Entry Type:");
        ImGui::RadioButton("Graphic Single (0)", &g_ImportType, 0);
        ImGui::RadioButton("Bumpmap (2)", &g_ImportType, 2);
        ImGui::RadioButton("Flat Sequence / Sprite Sheet (5)", &g_ImportType, 5);

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextColored(ImVec4(1, 0, 1, 1), "Compression Format:");
        ImGui::RadioButton("DXT1 (Opaque/1-bit Alpha)", &g_ImportFormat, 0);
        ImGui::RadioButton("DXT3 (Sharp Alpha)", &g_ImportFormat, 1);
        ImGui::RadioButton("DXT5 (Smooth Alpha)", &g_ImportFormat, 2);
        ImGui::RadioButton("ARGB (Uncompressed)", &g_ImportFormat, 3);

        ImGui::Separator();

        if (ImGui::Button("Import", ImVec2(120, 0))) {
            if (g_ActiveBankIndex != -1 && g_ActiveBankIndex < g_OpenBanks.size()) {
                LoadedBank* b = &g_OpenBanks[g_ActiveBankIndex];

                ETextureFormat fmt = ETextureFormat::DXT3;
                if (g_ImportFormat == 0) fmt = ETextureFormat::DXT1;
                else if (g_ImportFormat == 2) fmt = ETextureFormat::DXT5;
                else if (g_ImportFormat == 3) fmt = ETextureFormat::ARGB8888;

                CreateNewTextureEntry(b, g_PendingImportPath, fmt, g_ImportType);
                g_ScrollToSelected = true;
            }
            g_ShowTexImportPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowTexImportPopup = false;
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

                // --- LEFT PANE ---
                if (g_ShowLeftPanel) {
                    ImGui::BeginChild("LeftPane", ImVec2(bankSidebarWidth, 0), true);

                    // Recompile Buttons
                    if (bank.Type == EBankType::Text) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                        if (ImGui::Button("Recompile Text Bank (.BIG)", ImVec2(-FLT_MIN, 30))) SaveBigBank(&bank);
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }
                    else if (bank.Type == EBankType::Dialogue) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.2f, 0.8f, 1.0f));
                        if (ImGui::Button("Recompile Dialogue Bank", ImVec2(-FLT_MIN, 30))) SaveBigBank(&bank);
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }
                    else if (bank.Type == EBankType::Audio) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
                        std::string btnText = bank.LugParserPtr ? "Recompile Script (.LUG + .MET)" : "Recompile Audio Bank (.LUT)";
                        if (ImGui::Button(btnText.c_str(), ImVec2(-FLT_MIN, 30))) SaveAudioBank(&bank);
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }
                    else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.5f, 1.0f));
                        if (ImGui::Button("Recompile Bank (.BIG)", ImVec2(-FLT_MIN, 30))) SaveBigBank(&bank);
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }

                    if (bank.Type != EBankType::Audio && !bank.SubBanks.empty()) {
                        float avail = ImGui::GetContentRegionAvail().x;
                        ImGui::SetNextItemWidth(avail - 30);
                        std::string preview = (bank.ActiveSubBankIndex >= 0) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : "Select Sub-Bank";
                        if (ImGui::BeginCombo("##folder", preview.c_str())) {
                            for (int s = 0; s < (int)bank.SubBanks.size(); s++) {
                                if (ImGui::Selectable((bank.SubBanks[s].Name + " (" + std::to_string(bank.SubBanks[s].EntryCount) + ")").c_str(), bank.ActiveSubBankIndex == s))
                                    LoadSubBankEntries(&bank, s);
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("+", ImVec2(22, 0))) {
                            if (bank.Type == EBankType::Text) { g_ShowAddEntryPopup = true; ImGui::OpenPopup("Add Entry Type"); }
                            else if (bank.Type == EBankType::Dialogue) { CreateNewDialogueEntry(&bank); g_ScrollToSelected = true; }
                            else if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
                                std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
                                if (!path.empty()) {
                                    g_PendingImportPath = path;
                                    g_ShowTexImportPopup = true;
                                }
                            }
                            else g_BankStatus = "Add Entry not implemented for this bank type.";
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Entry");
                        ImGui::Separator();
                    }

                    float searchAvail = ImGui::GetContentRegionAvail().x;
                    bool showFilterBtn = (bank.Type == EBankType::Text || bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects || bank.Type == EBankType::Graphics);

                    if (bank.Type == EBankType::Audio && bank.LugParserPtr) {
                        ImGui::SetNextItemWidth(searchAvail - 35.0f);
                    }
                    else if (showFilterBtn) {
                        ImGui::SetNextItemWidth(searchAvail - 65.0f - ImGui::GetStyle().ItemSpacing.x);
                    }
                    else {
                        ImGui::SetNextItemWidth(-FLT_MIN);
                    }

                    ImGui::InputTextWithHint("##search", "Search...", bank.FilterText, 128);
                    if (ImGui::IsItemEdited()) UpdateFilter(bank);

                    if (bank.Type == EBankType::Audio && bank.LugParserPtr) {
                        ImGui::SameLine();
                        if (ImGui::Button("+", ImVec2(25, 0))) {
                            std::string wavPath = OpenFileDialog("WAV File\0*.wav\0");
                            if (!wavPath.empty()) {
                                if (bank.LugParserPtr->AddEntryFromWav(wavPath)) {
                                    bank.Entries.clear();
                                    bank.FilteredIndices.clear();
                                    for (size_t k = 0; k < bank.LugParserPtr->Entries.size(); k++) {
                                        BankEntry be;
                                        be.ID = bank.LugParserPtr->Entries[k].SoundID;
                                        be.Name = bank.LugParserPtr->Entries[k].Name;
                                        be.FriendlyName = be.Name;
                                        be.Size = bank.LugParserPtr->Entries[k].Length;
                                        be.Offset = bank.LugParserPtr->Entries[k].Offset;
                                        bank.Entries.push_back(be);
                                        bank.FilteredIndices.push_back((int)k);
                                    }
                                    UpdateFilter(bank);
                                    bank.SelectedEntryIndex = (int)bank.Entries.size() - 1;
                                    g_ScrollToSelected = true;
                                    g_SuccessMessage = "Entry created from WAV file.";
                                    g_ShowSuccessPopup = true;
                                }
                            }
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Entry from WAV File");
                    }

                    if (showFilterBtn) {
                        ImGui::SameLine();
                        if (ImGui::Button("Filters", ImVec2(60, 0))) ImGui::OpenPopup("FilterOptionsPopup");
                        if (ImGui::BeginPopup("FilterOptionsPopup")) {

                            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Search Mode:");
                            if (ImGui::RadioButton("Search by Name", bank.FilterMode == EFilterMode::Name)) { bank.FilterMode = EFilterMode::Name; UpdateFilter(bank); }
                            if (ImGui::RadioButton("Search by ID", bank.FilterMode == EFilterMode::ID)) { bank.FilterMode = EFilterMode::ID; UpdateFilter(bank); }

                            if (bank.Type == EBankType::Text) {
                                if (ImGui::RadioButton("Search by Speaker", bank.FilterMode == EFilterMode::Speaker)) { bank.FilterMode = EFilterMode::Speaker; UpdateFilter(bank); }
                            }

                            ImGui::Separator();

                            if (bank.Type == EBankType::Text) {
                                ImGui::TextColored(ImVec4(0, 1, 1, 1), "Entry Type:");
                                if (ImGui::RadioButton("Show All Types", bank.FilterTypeMask == -1)) { bank.FilterTypeMask = -1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Text Entries (Type 0)", bank.FilterTypeMask == 0)) { bank.FilterTypeMask = 0; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Groups (Type 1)", bank.FilterTypeMask == 1)) { bank.FilterTypeMask = 1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Narrator Lists (Type 2)", bank.FilterTypeMask == 2)) { bank.FilterTypeMask = 2; UpdateFilter(bank); }
                            }
                            else if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
                                ImGui::TextColored(ImVec4(0, 1, 1, 1), "Texture Type:");
                                if (ImGui::RadioButton("Show All Types##Tex", bank.FilterTypeMask == -1)) { bank.FilterTypeMask = -1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Graphic Single (Type 0)", bank.FilterTypeMask == 0)) { bank.FilterTypeMask = 0; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Graphic Sequence (Type 1)", bank.FilterTypeMask == 1)) { bank.FilterTypeMask = 1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Bumpmap (Type 2)", bank.FilterTypeMask == 2)) { bank.FilterTypeMask = 2; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Bumpmap Sequence (Type 3)", bank.FilterTypeMask == 3)) { bank.FilterTypeMask = 3; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Volume Texture (Type 4)", bank.FilterTypeMask == 4)) { bank.FilterTypeMask = 4; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Flat Sequence (Type 5)", bank.FilterTypeMask == 5)) { bank.FilterTypeMask = 5; UpdateFilter(bank); }

                                ImGui::Separator();
                                ImGui::TextColored(ImVec4(1, 0, 1, 1), "Compression Format:");
                                if (ImGui::RadioButton("Show All Formats", bank.FilterTextureFormatMask == -1)) { bank.FilterTextureFormatMask = -1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("DXT1 / Bump DXT1", bank.FilterTextureFormatMask == 0)) { bank.FilterTextureFormatMask = 0; UpdateFilter(bank); }
                                if (ImGui::RadioButton("DXT3", bank.FilterTextureFormatMask == 1)) { bank.FilterTextureFormatMask = 1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("DXT5 / Bump DXT5", bank.FilterTextureFormatMask == 2)) { bank.FilterTextureFormatMask = 2; UpdateFilter(bank); }
                                if (ImGui::RadioButton("ARGB8888", bank.FilterTextureFormatMask == 3)) { bank.FilterTextureFormatMask = 3; UpdateFilter(bank); }
                            }
                            // --- NEW: GRAPHICS BANK FILTERS ---
                            else if (bank.Type == EBankType::Graphics) {
                                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Mesh Types:");
                                if (ImGui::RadioButton("Show All Types##Gfx", bank.FilterTypeMask == -1)) { bank.FilterTypeMask = -1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Static Mesh", bank.FilterTypeMask == 1)) { bank.FilterTypeMask = 1; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Repeated Mesh", bank.FilterTypeMask == 2)) { bank.FilterTypeMask = 2; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Physics Mesh", bank.FilterTypeMask == 3)) { bank.FilterTypeMask = 3; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Particle Mesh", bank.FilterTypeMask == 4)) { bank.FilterTypeMask = 4; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Animated Mesh", bank.FilterTypeMask == 5)) { bank.FilterTypeMask = 5; UpdateFilter(bank); }

                                ImGui::Separator();
                                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Animation Types:");
                                if (ImGui::RadioButton("Animation", bank.FilterTypeMask == 6)) { bank.FilterTypeMask = 6; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Delta Animation", bank.FilterTypeMask == 7)) { bank.FilterTypeMask = 7; UpdateFilter(bank); }
                                //if (ImGui::RadioButton("Lipsync Animation", bank.FilterTypeMask == 8)) { bank.FilterTypeMask = 8; UpdateFilter(bank); }
                                if (ImGui::RadioButton("Partial Animation", bank.FilterTypeMask == 9)) { bank.FilterTypeMask = 9; UpdateFilter(bank); }
                                //if (ImGui::RadioButton("Relative Animation", bank.FilterTypeMask == 10)) { bank.FilterTypeMask = 10; UpdateFilter(bank); }
                            }

                            ImGui::EndPopup();
                        }
                    }

                    if (ImGui::BeginPopupModal("Add Entry Type", &g_ShowAddEntryPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Select Entry Type:");
                        ImGui::Separator();

                        if (ImGui::Button("Text Entry (Type 0)", ImVec2(200, 0))) {
                            CreateNewTextEntry(&bank, 0);
                            g_ScrollToSelected = true;
                            g_ShowAddEntryPopup = false;
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::Button("Group Entry (Type 1)", ImVec2(200, 0))) {
                            CreateNewTextEntry(&bank, 1);
                            g_ScrollToSelected = true;
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
                                g_ScrollToSelected = true; // Trigger scroll on arrow keys
                            }
                        }

                        for (int idx : bank.FilteredIndices) {
                            const auto& e = bank.Entries[idx];

                            ImGui::PushID(idx);

                            if (ImGui::Selectable(e.FriendlyName.c_str(), bank.SelectedEntryIndex == idx)) {
                                SelectEntry(&bank, idx);
                                // Intentionally DO NOT set g_ScrollToSelected here so clicking doesn't jerk the list
                            }

                            if (ImGui::BeginPopupContextItem()) {
                                if (ImGui::MenuItem("Duplicate Entry")) {
                                    DuplicateBankEntry(&bank, idx);
                                    g_ScrollToSelected = true; // Trigger scroll on duplicate
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
                                if (g_ScrollToSelected) {
                                    ImGui::SetScrollHereY(0.5f); // Snaps item to middle of view
                                    g_ScrollToSelected = false;
                                }
                            }

                            ImGui::PopID();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();
                    ImGui::InvisibleButton("vsplitter", ImVec2(4.0f, -1.0f));
                    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    if (ImGui::IsItemActive()) bankSidebarWidth += ImGui::GetIO().MouseDelta.x;
                    ImGui::SameLine();
                }

                // --- RIGHT PANE ---
                ImGui::BeginChild("RightPane", ImVec2(0, 0), true);

                // Top Toolbar for toggling the Left Panel
                if (ImGui::Button(g_ShowLeftPanel ? "<<##LeftToggle" : ">>##LeftToggle", ImVec2(28, 24))) {
                    g_ShowLeftPanel = !g_ShowLeftPanel;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ShowLeftPanel ? "Collapse Entry List" : "Expand Entry List");
                ImGui::SameLine();

                if (bank.SelectedEntryIndex != -1) {
                    const auto& e = bank.Entries[bank.SelectedEntryIndex];

                    std::string typeName = "Unknown";
                    if (bank.Type == EBankType::Text) {
                        if (e.Type == 0) typeName = "Type 0 - Text Entry";
                        else if (e.Type == 1) typeName = "Type 1 - Text Group";
                        else if (e.Type == 2) typeName = "Type 2 - Narrator List";
                        else typeName = "Type " + std::to_string(e.Type);
                    }
                    else if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
                        if (e.Type == 0x0) typeName = "Type 0 - Graphic Single";
                        else if (e.Type == 0x1) typeName = "Type 1 - Graphic Sequence";
                        else if (e.Type == 0x2) typeName = "Type 2 - Bumpmap";
                        else if (e.Type == 0x3) typeName = "Type 3 - Bumpmap Sequence";
                        else if (e.Type == 0x4) typeName = "Type 4 - Volume Texture";
                        else if (e.Type == 0x5) typeName = "Type 5 - Flat Sequence";
                        else typeName = "Type " + std::to_string(e.Type) + " - Texture";
                    }
                    else if (bank.Type == EBankType::Graphics) {
                        switch (e.Type) {
                        case 1: typeName = "Type 1 - Static Mesh"; break;
                        case 2: typeName = "Type 2 - Repeated Mesh"; break;
                        case 3: typeName = "Type 3 - Physics Mesh (BBM)"; break;
                        case 4: typeName = "Type 4 - Particle Mesh"; break;
                        case 5: typeName = "Type 5 - Animated Mesh"; break;
                        case 6: typeName = "Type 6 - Animation"; break;
                        case 7: typeName = "Type 7 - Delta Animation"; break;
                        case 8: typeName = "Type 8 - Lipsync Animation"; break;
                        case 9: typeName = "Type 9 - Partial Animation"; break;
                        case 10: typeName = "Type 10 - Relative Animation"; break;
                        default: typeName = "Type " + std::to_string(e.Type) + " - Unknown Mesh"; break;
                        }
                    }
                    else if (bank.Type == EBankType::Audio) typeName = "Audio Clip";
                    else typeName = "Type " + std::to_string(e.Type);

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("ID: %d | %s | Size: %d bytes", e.ID, typeName.c_str(), e.Size);

                    ImGui::SameLine();
                    float availRight = ImGui::GetContentRegionAvail().x;
                    float buttonsWidth = 140.0f;
                    if (availRight > buttonsWidth) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availRight - buttonsWidth);
                    }

                    if (ImGui::Button("Save", ImVec2(60, 0))) {
                        SaveEntryChanges(&bank);
                    }

                    ImGui::SameLine();

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

                    ImGui::Separator();

                    if (bank.Type == EBankType::Audio) {
                        if (bank.LugParserPtr) DrawLugAudioProperties(&bank);
                        else DrawAudioProperties(&bank);
                    }
                    else if (IsSupportedMesh(e.Type) || e.Type == TYPE_STATIC_PHYSICS_MESH) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Name:");
                        ImGui::SameLine();

                        static char nameBuf[256];
                        static int lastEditID = -1;
                        static int lastBankID = -1;
                        if (lastEditID != e.ID || lastBankID != g_ActiveBankIndex) {
                            strncpy_s(nameBuf, sizeof(nameBuf), e.Name.c_str(), _TRUNCATE);
                            lastEditID = e.ID;
                            lastBankID = g_ActiveBankIndex;
                        }

                        ImGui::SetNextItemWidth(500);
                        if (ImGui::InputText("##meshNameEdit", nameBuf, 256)) {
                            bank.Entries[bank.SelectedEntryIndex].Name = nameBuf;
                            bank.Entries[bank.SelectedEntryIndex].FriendlyName = nameBuf;
                            if (IsSupportedMesh(e.Type)) g_ActiveMeshContent.MeshName = nameBuf;
                        }

                        if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
                            ImGui::SameLine();
                            ImGui::Text("LOD:");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            std::string lodPreview = std::to_string(bank.SelectedLOD);
                            if (ImGui::BeginCombo("##lod", lodPreview.c_str())) {
                                for (uint32_t l = 0; l < g_ActiveMeshContent.EntryMeta.LODCount; l++) {
                                    if (ImGui::Selectable(std::to_string(l).c_str(), bank.SelectedLOD == l)) {
                                        bank.SelectedLOD = l;
                                        ParseSelectedLOD(&bank);
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }
                    else {
                        if (e.Name != e.FriendlyName) {
                            ImGui::TextDisabled("Internal File Name: %s", e.Name.c_str());
                        }
                    }

                    ImGui::Separator();

                    if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) DrawTextureProperties();
                    else if (bank.Type == EBankType::Text) DrawTextProperties(&bank, [&]() { SaveEntryChanges(&bank); }, [&](std::string target, uint32_t id, std::string hint) { JumpToBankEntry(target, id, hint); });
                    else if (bank.Type == EBankType::Dialogue) DrawLipSyncProperties(&bank, [&]() { SaveEntryChanges(&bank); }, nullptr);
                    else if (IsSupportedMesh(e.Type) || e.Type == TYPE_STATIC_PHYSICS_MESH) DrawMeshProperties([&]() { SaveEntryChanges(&bank); });
                    else if (e.Type == 6 || e.Type == 7 || e.Type == 9) {
                        DrawAnimProperties(bank.Entries[bank.SelectedEntryIndex].Name, e.ID, g_AnimParser, g_AnimUIState, bank.CurrentEntryRawData);
                    }
}
                ImGui::EndChild();

                if (g_ShowDeleteBankEntryPopup) ImGui::OpenPopup("Delete Bank Entry?");

                if (ImGui::BeginPopupModal("Delete Bank Entry?", &g_ShowDeleteBankEntryPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Are you sure you want to delete this entry?");
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "This action cannot be undone.");
                    ImGui::Separator();

                    static bool dontShowAgain = false;
                    ImGui::Checkbox("Don't show this again", &dontShowAgain);

                    if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                        if (dontShowAgain) { g_AppConfig.ShowBankDeleteConfirm = false; SaveConfig(); }
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

        if (ImGui::TabItemButton("+ Load Bank (.BIG / .LUT / .LUG)", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
            std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut;*.lug\0All Files\0*.*\0");
            if (!path.empty()) LoadBank(path);
        }

        ImGui::EndTabBar();
    }
}