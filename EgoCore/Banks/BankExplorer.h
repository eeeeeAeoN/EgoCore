#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "BankBackend.h" 
#include "DefExplorer.h"
#include "ConfigBackend.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "TextProperties.h"
#include "GltfExporter.h"
#include "LipSyncProperties.h"
#include "BinaryParser.h"
#include "AudioExplorer.h" // <--- NEW: Using the dedicated Audio UI file
#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem> 

static bool g_HasInitialized = false;
static std::vector<BinaryParser> g_LoadedBinaries;

// --- INITIALIZATION ---

static void LoadSystemBinaries(const std::string& gameRoot) {
    namespace fs = std::filesystem;
    g_LoadedBinaries.clear();

    std::vector<std::string> targetFiles = {
        "gamesnds.bin",
        "dialoguesnds.bin",         "dialoguesnds.h",
        "dialoguesnds2.bin",        "dialoguesnds2.h",
        "scriptdialoguesnds.bin",   "scriptdialoguesnds.h",
        "scriptdialoguesnds2.bin",  "scriptdialoguesnds2.h"
    };

    fs::path defsPath = fs::path(gameRoot) / "Data" / "Defs";

    for (const auto& fname : targetFiles) {
        fs::path fullPath = defsPath / fname;
        if (fs::exists(fullPath)) {
            bool alreadyLoaded = false;
            for (const auto& loaded : g_LoadedBinaries) {
                if (loaded.Data.FileName == fname) alreadyLoaded = true;
            }
            if (alreadyLoaded) continue;

            BinaryParser parser;
            parser.Parse(fullPath.string());
            if (parser.Data.IsParsed) {
                g_LoadedBinaries.push_back(std::move(parser));
            }
        }
    }
}

static void SelectEntry(LoadedBank* bank, int idx) {
    if (!bank || idx < 0 || idx >= (int)bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->Player.Reset();
    }

    g_TextureParser.DecodedPixels.clear();
    g_TextureParser.IsParsed = false;
    std::vector<uint8_t>().swap(g_TextureParser.DecodedPixels);

    g_BBMParser.IsParsed = false;
    g_ActiveMeshContent = C3DMeshContent();
    g_AnimParseSuccess = false;

    g_TextParser.IsParsed = false;
    g_TextParser.TextData = CTextEntry();
    g_TextParser.GroupData = CTextGroup();
    g_LipSyncParser.Data = CLipSyncData();

    bank->SelectedEntryIndex = idx;
    bank->SelectedLOD = 0;
    const auto& e = bank->Entries[idx];

    if (bank->Type != EBankType::Audio) {
        bank->Stream->clear();
        bank->Stream->seekg(e.Offset, std::ios::beg);
        size_t fileSize = (e.Size > 50000000) ? 50000000 : e.Size;
        size_t paddedSize = fileSize + 64;

        bank->CurrentEntryRawData.clear();
        bank->CurrentEntryRawData.resize(paddedSize, 0);
        bank->Stream->read((char*)bank->CurrentEntryRawData.data(), fileSize);
    }

    if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend) {
        if (bank->SubheaderCache.count(idx)) {
            g_TextureParser.Parse(bank->SubheaderCache[idx], bank->CurrentEntryRawData, e.Type);
        }
    }
    else if (bank->Type == EBankType::Text || bank->Type == EBankType::Dialogue) {
        if (bank->Type == EBankType::Dialogue) {
            g_LipSyncParser.Parse(bank->CurrentEntryRawData, bank->SubheaderCache[idx]);
        }
        g_TextParser.Parse(bank->CurrentEntryRawData, e.Type);
        if (g_TextParser.IsGroup) ResolveGroupMetadata(bank);
    }
    else if (bank->Type != EBankType::Audio) {
        if (e.Type == TYPE_STATIC_PHYSICS_MESH) {
            g_BBMParser.Parse(bank->CurrentEntryRawData);
            g_MeshUploadNeeded = true;
        }
        else if (IsSupportedMesh(e.Type)) {
            if (bank->SubheaderCache.count(idx)) g_ActiveMeshContent.ParseEntryMetadata(bank->SubheaderCache[idx]);
            if (!bank->CurrentEntryRawData.empty()) {
                g_ActiveMeshContent.Parse(bank->CurrentEntryRawData);
                g_MeshUploadNeeded = true;
            }
        }
        else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
            if (bank->SubheaderCache.count(idx)) g_AnimParseSuccess = g_ActiveAnim.Deserialize(bank->SubheaderCache[idx]);
        }
    }
}

inline void ParseSelectedLOD(LoadedBank* bank) {
    if (!bank || bank->CurrentEntryRawData.empty()) return;
    size_t offset = 0; size_t size = bank->CurrentEntryRawData.size();

    if (g_ActiveMeshContent.EntryMeta.HasData && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
        if (bank->SelectedLOD >= g_ActiveMeshContent.EntryMeta.LODCount) bank->SelectedLOD = 0;
        size_t currentOffset = 0;
        for (int i = 0; i < bank->SelectedLOD; i++) currentOffset += g_ActiveMeshContent.EntryMeta.LODSizes[i];
        size_t currentSize = g_ActiveMeshContent.EntryMeta.LODSizes[bank->SelectedLOD];
        if (currentOffset + currentSize <= bank->CurrentEntryRawData.size()) { offset = currentOffset; size = currentSize; }
    }
    std::vector<uint8_t> slice;
    if (size > 0) { slice.resize(size); memcpy(slice.data(), bank->CurrentEntryRawData.data() + offset, size); }

    g_ActiveMeshContent.Parse(slice);
    g_MeshUploadNeeded = true;
}

inline void SaveEntryChanges(LoadedBank* bank) {
    if (!bank || bank->SelectedEntryIndex == -1) return;
    BankEntry& e = bank->Entries[bank->SelectedEntryIndex];
    std::vector<uint8_t> newBytes;

    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) newBytes = g_ActiveAnim.Serialize();
    else return;

    if (newBytes.size() <= e.InfoSize) {
        bank->Stream->clear();
        bank->Stream->seekp(e.SubheaderFileOffset, std::ios::beg);
        bank->Stream->write((char*)newBytes.data(), newBytes.size());
        bank->SubheaderCache[bank->SelectedEntryIndex] = newBytes;
        g_BankStatus = "Saved."; bank->Stream->flush();
    }
    else {
        g_BankStatus = "Error: New data exceeds allocated space!";
    }
}

// --- DRAWING FUNCTIONS ---

static void DrawBinaryTab() {
    static bool isCompilingBins = false;
    static std::string compileBinStatus = "";
    static bool showBinResult = false;

    // --- TOOLBAR ---
    if (ImGui::Button("Load Binaries from Data/Defs")) {
        LoadSystemBinaries(g_AppConfig.GameRootPath);
    }

    ImGui::SameLine();

    // NEW COMPILE BUTTON
    if (ImGui::Button("Compile Sound Binaries")) {
        compileBinStatus = "Starting compilation...";
        isCompilingBins = true;
        ImGui::OpenPopup("Compiling Binaries");

        // Run in thread to avoid freezing UI
        std::thread([&]() {
            std::string log = "";
            std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
            BinaryParser::CompileSoundBinaries(defsPath, log);
            compileBinStatus = log;
            isCompilingBins = false;
            showBinResult = true;
            }).detach();
    }

    // --- LOADING POPUP ---
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

    // --- RESULT POPUP ---
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
            // Reload binaries to see changes
            LoadSystemBinaries(g_AppConfig.GameRootPath);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (g_LoadedBinaries.empty()) {
        ImGui::TextDisabled("No binary definitions loaded.");
        return;
    }

    // ... (Rest of the existing DrawBinaryTab logic: CRC Checker, Tabs, etc.) ...
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Manual CRC Checker");
    static char buf[128] = "";
    ImGui::InputText("Sound Name", buf, 128);
    if (buf[0] != 0) {
        // Use the Fable CRC for checking now
        uint32_t hash = BinaryParser::CalculateCRC32_Fable(buf);
        ImGui::SameLine();
        ImGui::TextDisabled("Fable CRC: %08X", hash);

        // Also show Standard CRC just in case
        // uint32_t stdHash = BinaryParser::CalculateCRC32_Standard(buf);
        // ImGui::TextDisabled("Standard CRC: %08X", stdHash);

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

    // ... (Existing Table Drawing Loop) ...
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

    // --- MAIN BANK TAB BAR ---
    if (ImGui::BeginTabBar("BankFiles", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

        // 1. Iterate Open Banks
        for (int i = 0; i < (int)g_OpenBanks.size(); ) {
            LoadedBank& bank = g_OpenBanks[i];
            bool keepOpen = true;
            std::string tabLabel = bank.FileName + "##" + std::to_string(i);

            if (ImGui::BeginTabItem(tabLabel.c_str(), &keepOpen)) {
                if (g_ActiveBankIndex != i) {
                    g_ActiveBankIndex = i;
                    if (bank.SelectedEntryIndex != -1) SelectEntry(&bank, bank.SelectedEntryIndex);
                }

                // --- LEFT PANE: LIST ---
                ImGui::BeginChild("LeftPane", ImVec2(bankSidebarWidth, 0), true);

                // Sub-Bank Selector (Only for .BIG files)
                if (bank.Type != EBankType::Audio && !bank.SubBanks.empty()) {
                    std::string preview = "Select Sub-Bank";
                    if (bank.ActiveSubBankIndex >= 0) preview = bank.SubBanks[bank.ActiveSubBankIndex].Name;
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::BeginCombo("##folder", preview.c_str())) {
                        for (int s = 0; s < (int)bank.SubBanks.size(); s++) {
                            bool is_sel = (bank.ActiveSubBankIndex == s);
                            if (ImGui::Selectable((bank.SubBanks[s].Name + " (" + std::to_string(bank.SubBanks[s].EntryCount) + ")").c_str(), is_sel)) {
                                LoadSubBankEntries(bank, s);
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::Separator();
                }

                // Search Bar
                ImGui::InputText("Search", bank.FilterText, 128);
                if (ImGui::IsItemEdited()) UpdateFilter(bank);

                // Entry List
                if (!bank.Entries.empty()) {
                    ImGui::BeginChild("ListScroll", ImVec2(0, 0), false);

                    // Keyboard Navigation
                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && !bank.FilteredIndices.empty()) {
                        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                            int direction = ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? 1 : -1;
                            int currentPos = -1;
                            for (int k = 0; k < bank.FilteredIndices.size(); k++) {
                                if (bank.FilteredIndices[k] == bank.SelectedEntryIndex) { currentPos = k; break; }
                            }
                            int newPos = std::clamp(currentPos + direction, 0, (int)bank.FilteredIndices.size() - 1);
                            SelectEntry(&bank, bank.FilteredIndices[newPos]);
                        }
                    }

                    // Render List Items
                    for (int idx : bank.FilteredIndices) {
                        const auto& e = bank.Entries[idx];
                        if (ImGui::Selectable(e.Name.c_str(), bank.SelectedEntryIndex == idx)) {
                            SelectEntry(&bank, idx);
                        }
                        if (bank.SelectedEntryIndex == idx && ImGui::IsWindowFocused()) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();

                // --- SPLITTER ---
                ImGui::SameLine();
                ImGui::InvisibleButton("vsplitter", ImVec2(4.0f, -1.0f));
                if (ImGui::IsItemActive()) bankSidebarWidth += ImGui::GetIO().MouseDelta.x;
                ImGui::SameLine();

                // --- RIGHT PANE: PROPERTIES ---
                ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
                if (bank.SelectedEntryIndex != -1) {
                    const auto& e = bank.Entries[bank.SelectedEntryIndex];
                    ImGui::Text("ID: %d | Type: %d | Size: %d bytes", e.ID, e.Type, e.Size);

                    // ---------------------------
                    // NEW AUDIO HANDLING
                    // ---------------------------
                    if (bank.Type == EBankType::Audio) {
                        DrawAudioProperties(&bank);
                    }
                    // ---------------------------
                    // EXISTING LOD / MESH LOGIC
                    // ---------------------------
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

                    // Render Specific Editors
                    if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) DrawTextureProperties();
                    else if (bank.Type == EBankType::Text) DrawTextProperties(g_LoadedBinaries);
                    else if (bank.Type == EBankType::Dialogue) DrawLipSyncProperties(); // Legacy BIG dialogue
                    else if (IsSupportedMesh(e.Type)) DrawMeshProperties([&]() { SaveEntryChanges(&bank); });
                    else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION)
                        DrawAnimProperties(g_ActiveAnim, g_AnimParseSuccess, g_AnimUIState, [&]() { SaveEntryChanges(&bank); });
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (!keepOpen) {
                g_OpenBanks.erase(g_OpenBanks.begin() + i);
                if (g_OpenBanks.empty()) g_ActiveBankIndex = -1;
                else if (g_ActiveBankIndex >= i) g_ActiveBankIndex = (std::max)(0, g_ActiveBankIndex - 1);
            }
            else { i++; }
        }

        // 2. Trailing "Load" Button (Right Side of Tabs)
        if (ImGui::TabItemButton("+ Load Bank (.BIG / .LUT)", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
            std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut\0All Files\0*.*\0");
            if (!path.empty()) LoadBank(path);
        }

        ImGui::EndTabBar();
    }

    // Handle "Empty State" if no tabs are open
    if (g_OpenBanks.empty()) {
        // Just show a centered message or instruction
        // The "+ Load Bank" button is visible in the tab bar even if empty
    }
}

static void DrawBankExplorer() {
    if (!g_HasInitialized) {
        LoadConfig();
        if (g_AppConfig.IsConfigured) {
            PerformAutoLoad();
            LoadSystemBinaries(g_AppConfig.GameRootPath);
        }
        g_HasInitialized = true;
    }

    if (!g_AppConfig.IsConfigured) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 200));
        ImGui::SetNextWindowFocus();

        if (ImGui::Begin("Welcome to EgoCore", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("Welcome to EgoCore!");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextWrapped("To get started, please select your main Fable game folder (The folder containing Fable.exe and Data).");
            ImGui::Dummy(ImVec2(0, 20));

            if (ImGui::Button("Select Game Folder", ImVec2(-1, 40))) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    InitializeSetup(root);
                    LoadSystemBinaries(root);
                }
            }
        }
        ImGui::End();
        return;
    }

    // --- MAIN MENU BAR ---
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Bank File (.BIG / .LUT)")) {
                std::string path = OpenFileDialog("Fable Banks\0*.big;*.lut\0All Files\0*.*\0");
                if (!path.empty()) LoadBank(path);
            }

            if (ImGui::MenuItem("Open Binary Definition (.bin)")) {
                std::string path = OpenFileDialog("Binary Files\0*.bin;*.h\0All Files\0*.*\0");
                if (!path.empty()) {
                    BinaryParser parser;
                    parser.Parse(path);
                    if (parser.Data.IsParsed) g_LoadedBinaries.push_back(std::move(parser));
                }
            }

            if (ImGui::MenuItem("Change Game Folder")) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) {
                    InitializeSetup(root);
                    LoadSystemBinaries(root);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                if (g_DefWorkspace.IsDirty() && g_AppConfig.ShowUnsavedChangesWarning) {
                    g_DefWorkspace.PendingNav = { DefAction::ExitProgram, "", -1 };
                    g_DefWorkspace.TriggerUnsavedPopup = true;
                }
                else {
                    exit(0);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::SameLine();

        if (!g_LoadedBinaries.empty()) {
            ImGui::TextDisabled("| Loaded %zu Binaries", g_LoadedBinaries.size());
        }
        else {
            ImGui::TextDisabled("| %s", g_BankStatus.c_str());
        }

        ImGui::EndMainMenuBar();
    }

    // --- GLOBAL TABS ---
    if (ImGui::BeginTabBar("ModeTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Bank Archives")) {
            DrawBankTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Game Definitions")) {
            DrawDefTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Binary Definitions")) {
            DrawBinaryTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // --- UNSAVED CHANGES POPUP ---
    if (g_DefWorkspace.TriggerUnsavedPopup) {
        ImGui::OpenPopup("UnsavedChangesGlobal");
        g_DefWorkspace.TriggerUnsavedPopup = false;
    }

    if (ImGui::BeginPopupModal("UnsavedChangesGlobal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes.");
        ImGui::Text("What would you like to do?");
        ImGui::Separator();

        static bool dontShowUnsaved = false;
        ImGui::Checkbox("Don't show this message again", &dontShowUnsaved);
        ImGui::Dummy(ImVec2(0, 10));

        if (ImGui::Button("Save & Continue", ImVec2(140, 0))) {
            if (g_DefWorkspace.ShowDefsMode) {
                if (!g_DefWorkspace.SelectedType.empty() && g_DefWorkspace.SelectedEntryIndex != -1)
                    SaveDefEntry(g_DefWorkspace.CategorizedDefs[g_DefWorkspace.SelectedType][g_DefWorkspace.SelectedEntryIndex]);
            }
            else {
                if (g_DefWorkspace.SelectedEnumIndex != -1)
                    SaveHeaderEntry(g_DefWorkspace.AllEnums[g_DefWorkspace.SelectedEnumIndex]);
            }

            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) {
                exit(0);
            }
            // Add navigation logic here if needed
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Discard & Continue", ImVec2(140, 0))) {
            if (dontShowUnsaved) {
                g_AppConfig.ShowUnsavedChangesWarning = false;
                SaveConfig();
            }

            if (g_DefWorkspace.PendingNav.Action == DefAction::ExitProgram) {
                exit(0);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}