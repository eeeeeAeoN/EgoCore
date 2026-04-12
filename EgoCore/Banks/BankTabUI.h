#pragma once
#include "imgui.h"
#include "FileDialogs.h"
#include "AudioExplorer.h"
#include "AnimCompiler.h"
#include "BankEditor.h" 
#include "InputManager.h"
#include "ShaderProperties.h"
#include "FontProperties.h"
#include "ParticleProperties.h"
#include "StreamingFontProperties.h"
#include "ModManagerCompiler.h"
#include <thread>

static int g_ContextEntryIndex = -1;
static bool g_ShowDeleteBankEntryPopup = false;
static bool g_ShowAddEntryPopup = false;
static bool g_ShowTexImportPopup = false;
static std::string g_PendingImportPath = "";
static int g_ImportFormat = 1;
static int g_ImportType = 0;
static float g_ImportBumpFactor = 5.0f;
static bool g_ScrollToSelected = false;
static bool g_ShowAddLODPopup = false;
static bool g_ShowDeleteLODPopup = false;
static bool g_ShowReplaceLODPopup = false;
static int g_PendingLODActionIndex = -1;
static int g_ImportReps = 32;
static bool g_ShowType2SettingsPopup = false;
static std::string g_PendingGltfPath = "";
static int g_PendingLODAction = 0;
static int g_LODImportType = 1;
static bool g_ForceRecalculateBounds = false;
static bool g_ShowAnimImportPopup = false;
static int g_ImportAnimType = 6;
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

    // --- GLOBAL POPUPS ---
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

        static bool g_ImportGenerateBump = true;

        if (g_ImportType == 2) {
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Bumpmap Settings:");
            ImGui::Checkbox("Generate Normal Map from Image", &g_ImportGenerateBump);

            if (g_ImportGenerateBump) {
                ImGui::SliderFloat("Bump Intensity", &g_ImportBumpFactor, 0.1f, 20.0f, "%.1f");
                ImGui::TextDisabled("Auto-converts colors to a Fable normal map.");
            }
            else {
                ImGui::TextDisabled("Imports the image as-is (must already be a normal map).");
            }
            ImGui::Dummy(ImVec2(0, 5));
        }

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
                if (g_ImportFormat == 0) {
                    fmt = (g_ImportType == 2) ? ETextureFormat::NormalMap_DXT1 : ETextureFormat::DXT1;
                }
                else if (g_ImportFormat == 2) {
                    fmt = (g_ImportType == 2) ? ETextureFormat::NormalMap_DXT5 : ETextureFormat::DXT5;
                }
                else if (g_ImportFormat == 3) {
                    fmt = ETextureFormat::ARGB8888;
                }

                bool doGenerate = (g_ImportType == 2 && g_ImportGenerateBump);
                CreateNewTextureEntry(b, g_PendingImportPath, fmt, g_ImportType, doGenerate);
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

    if (g_ShowAnimImportPopup) {
        ImGui::OpenPopup("Import Animation Options");
    }

    if (ImGui::BeginPopupModal("Import Animation Options", &g_ShowAnimImportPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File: %s", std::filesystem::path(g_PendingImportPath).filename().string().c_str());
        ImGui::Separator();

        if (g_ActiveMeshContent.BoneCount == 0) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "WARNING: No Mesh loaded!");
            ImGui::TextWrapped("Fable requires animations to match the mesh skeleton exactly.");
            ImGui::TextWrapped("Importing without an active mesh open may cause game crashes.");
            ImGui::Dummy(ImVec2(0, 10));
        }

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Mesh Types:");
        ImGui::RadioButton("Static Mesh (1)", &g_ImportAnimType, 1);
        ImGui::RadioButton("Repeated Mesh (2)", &g_ImportAnimType, 2);
        ImGui::RadioButton("Physics Mesh (3)", &g_ImportAnimType, 3);
        ImGui::RadioButton("Particle Mesh (4)", &g_ImportAnimType, 4);
        ImGui::RadioButton("Animated Mesh (5)", &g_ImportAnimType, 5);

        if (g_ImportAnimType == 1 || g_ImportAnimType == 5) {
            ImGui::Separator();
            ImGui::Checkbox("Recalculate Bounds & Scale", &g_ForceRecalculateBounds);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Tick this if you have physically enlarged the mesh in Blender.\nLeave unticked for Lossless mode (best for texture/vertex tweaks).");
            }
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Animation Types:");
        ImGui::RadioButton("Normal Animation (6)", &g_ImportAnimType, 6);
        ImGui::RadioButton("Delta Animation (7)", &g_ImportAnimType, 7);
        ImGui::TextDisabled("Note: Partial Animations (9) are auto-detected via bitmasks.");

        ImGui::Separator();

        if (ImGui::Button("Import", ImVec2(120, 0))) {
            if (g_ActiveBankIndex != -1 && g_ActiveBankIndex < g_OpenBanks.size()) {
                if (g_ImportAnimType == 1 || g_ImportAnimType == 2 || g_ImportAnimType == 3 || g_ImportAnimType == 4 || g_ImportAnimType == 5) {
                    if (g_ImportAnimType == 1 || g_ImportAnimType == 3 || g_ImportAnimType == 4 || g_ImportAnimType == 5) {
                        if (CreateNewMeshEntry(&g_OpenBanks[g_ActiveBankIndex], g_PendingImportPath, g_ImportAnimType, 0, g_ForceRecalculateBounds)) {
                            g_BankStatus = "New Mesh Created Successfully!";
                            g_ScrollToSelected = true;
                        }
                    }
                    else {
                        g_PendingGltfPath = g_PendingImportPath;
                        g_PendingLODAction = 2;
                        g_ShowType2SettingsPopup = true;
                    }
                }
                else {
                    CreateNewAnimationEntry(&g_OpenBanks[g_ActiveBankIndex], g_PendingImportPath, g_ImportAnimType);
                    g_ScrollToSelected = true;
                }
            }
            g_ShowAnimImportPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowAnimImportPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    // --- LAYER 2: THE BANK CONTROL HEADER ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::BeginChild("BankHeaderBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);

    if (g_OpenBanks.empty()) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("No banks currently loaded. Use File -> Load Bank.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
    }

    if (g_ActiveBankIndex < 0 || g_ActiveBankIndex >= g_OpenBanks.size()) g_ActiveBankIndex = 0;
    LoadedBank& bank = g_OpenBanks[g_ActiveBankIndex];

    // 1. Bank Dropdown
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Bank:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250);
    if (ImGui::BeginCombo("##BankSelectCombo", bank.FileName.c_str())) {
        for (int i = 0; i < g_OpenBanks.size(); i++) {
            bool isSelected = (g_ActiveBankIndex == i);
            std::string comboLabel = g_OpenBanks[i].FileName + "##Bank" + std::to_string(i);

            if (ImGui::Selectable(comboLabel.c_str(), isSelected)) {
                if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size()) {
                    // Include SubBankIndex!
                    PushBankHistory(g_ActiveBankIndex, g_OpenBanks[g_ActiveBankIndex].ActiveSubBankIndex, g_OpenBanks[g_ActiveBankIndex].SelectedEntryIndex);
                }
                g_ActiveBankIndex = i;
                if (g_OpenBanks[i].SelectedEntryIndex != -1) {
                    SelectEntry(&g_OpenBanks[i], g_OpenBanks[i].SelectedEntryIndex);
                }
            }
        }
        ImGui::EndCombo();
    }

    // Close Bank Button 
    ImGui::SameLine();
    if (ImGui::Button("X##CloseBank", ImVec2(24, 0))) {
        g_OpenBanks.erase(g_OpenBanks.begin() + g_ActiveBankIndex);
        g_ActiveBankIndex = (std::max)(0, g_ActiveBankIndex - 1);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
    }

    if (!bank.SubBanks.empty()) {
        ImGui::SameLine(0, 20);
        ImGui::Text("Sub-Bank:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250);
        std::string sbPreview = (bank.ActiveSubBankIndex >= 0) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : "Select Sub-Bank";
        if (ImGui::BeginCombo("##SubBankSelectCombo", sbPreview.c_str())) {
            for (int s = 0; s < bank.SubBanks.size(); s++) {
                if (ImGui::Selectable((bank.SubBanks[s].Name + " (" + std::to_string(bank.SubBanks[s].EntryCount) + ")").c_str(), bank.ActiveSubBankIndex == s)) {
                    PushBankHistory(g_ActiveBankIndex, bank.ActiveSubBankIndex, bank.SelectedEntryIndex);
                    LoadSubBankEntries(&bank, s);
                }
            }
            ImGui::EndCombo();
        }
    }

    float compileBtnWidth = 180.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - compileBtnWidth - 15.0f);

    if (bank.Type == EBankType::Text) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
    else if (bank.Type == EBankType::Dialogue) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.2f, 0.8f, 1.0f));
    else if (bank.Type == EBankType::Audio) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
    else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.5f, 1.0f));

    std::string compileText = "RECOMPILE";
    if (bank.Type == EBankType::Text) compileText = "RECOMPILE";
    else if (bank.Type == EBankType::Dialogue) compileText = "RECOMPILE";
    else if (bank.Type == EBankType::Audio) compileText = bank.LugParserPtr ? "RECOMPILE" : "RECOMPILE";

    if (ImGui::Button(compileText.c_str(), ImVec2(compileBtnWidth, 0))) {
        if (bank.Type == EBankType::Audio) SaveAudioBank(&bank);
        else SaveBigBank(&bank);
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && bank.SelectedEntryIndex != -1) {
        if (g_Keybinds.DeleteEntry.IsPressed() && bank.Type != EBankType::Shaders && bank.Type != EBankType::Fonts) {
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

    if (g_ShowLeftPanel) {
        ImGui::BeginChild("LeftPane", ImVec2(bankSidebarWidth, 0), true);

        float searchAvail = ImGui::GetContentRegionAvail().x;
        bool showFilterBtn = (bank.Type == EBankType::Text || bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Graphics || bank.Type == EBankType::XboxGraphics);
        bool canAdd = (bank.Type == EBankType::Text || bank.Type == EBankType::Dialogue || bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects || bank.Type == EBankType::Graphics || (bank.Type == EBankType::Audio && bank.LugParserPtr));

        float inputWidth = searchAvail;
        if (showFilterBtn) inputWidth -= (65.0f + ImGui::GetStyle().ItemSpacing.x);
        if (canAdd) inputWidth -= (30.0f + ImGui::GetStyle().ItemSpacing.x);

        ImGui::SetNextItemWidth(inputWidth);
        ImGui::InputTextWithHint("##search", "Search...", bank.FilterText, 128);
        if (ImGui::IsItemEdited()) UpdateFilter(bank);

        if (canAdd) {
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(25, 0))) {
                if (bank.Type == EBankType::Text) { g_ShowAddEntryPopup = true; ImGui::OpenPopup("Add Entry Type"); }
                else if (bank.Type == EBankType::Dialogue) { CreateNewDialogueEntry(&bank); g_ScrollToSelected = true; }
                else if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) {
                    std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
                    if (!path.empty()) {
                        g_PendingImportPath = path;
                        g_ShowTexImportPopup = true;
                    }
                }
                else if (bank.Type == EBankType::Effects) {
                    CreateNewParticleEntry(&bank);
                    g_ScrollToSelected = true;
                }
                else if (bank.Type == EBankType::Graphics) {
                    std::string path = OpenFileDialog("glTF 3D Models\0*.gltf\0All Files\0*.*\0");
                    if (!path.empty()) {
                        g_PendingImportPath = path;
                        g_ShowAnimImportPopup = true;
                        ImGui::OpenPopup("Import Animation Options");
                    }
                }
                else if (bank.Type == EBankType::Audio && bank.LugParserPtr) {
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
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Entry");
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
                if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects || (bank.Type == EBankType::XboxGraphics && IsTextureSubBank(&bank))) {
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
                else if (bank.Type == EBankType::Graphics || (bank.Type == EBankType::XboxGraphics && IsGraphicsSubBank(&bank))) {
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
                    if (ImGui::RadioButton("Partial Animation", bank.FilterTypeMask == 9)) { bank.FilterTypeMask = 9; UpdateFilter(bank); }
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
                    g_ScrollToSelected = true;
                }
            }

            for (int idx : bank.FilteredIndices) {
                const auto& e = bank.Entries[idx];
                ImGui::PushID(idx);

                bool isStaged = bank.StagedEntries.count(idx) > 0;
                bool isModified = bank.ModifiedEntryData.count(idx) > 0;
                bool isMarked = ModPackageTracker::IsMarked(bank.FileName, e.Name);

                std::string displayLabel = e.FriendlyName;
                if (isStaged) displayLabel += " *";
                else if (isModified) displayLabel += " (Mod)";

                if (isMarked) displayLabel += " [M]";

                if (isMarked) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
                else if (isStaged) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                else if (isModified) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));

                if (ImGui::Selectable(displayLabel.c_str(), bank.SelectedEntryIndex == idx)) {
                    PushBankHistory(g_ActiveBankIndex, bank.ActiveSubBankIndex, bank.SelectedEntryIndex);
                    SelectEntry(&bank, idx);
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    int payloadData[2] = { g_ActiveBankIndex, idx };
                    ImGui::SetDragDropPayload("BANK_ENTRY_PAYLOAD", &payloadData, sizeof(int) * 2);

                    ImGui::Text("Add to Mod Package:\n%s (ID: %d)", e.Name.c_str(), e.ID);
                    ImGui::EndDragDropSource();
                }

                if (isMarked || isStaged || isModified) ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem()) {

                    if (ImGui::MenuItem(isMarked ? "Unmark for Packaging" : "Mark for Packaging (Tracker)")) {
                        StagedModPackageEntry staged;
                        staged.EntryID = e.ID;
                        staged.EntryName = e.Name;
                        staged.EntryType = e.Type;
                        staged.BankType = bank.Type;
                        staged.TypeName = GetEntryTypeName(bank.Type, e.Type, bank.FileName); // This is in ModManagerCompiler.h now
                        staged.BankName = bank.FileName;
                        staged.SourceFullPath = bank.FullPath;
                        staged.SubBankName = (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) ? bank.SubBanks[bank.ActiveSubBankIndex].Name : "N/A";

                        ModPackageTracker::ToggleMark(staged); // This is in ModManagerCompiler.h now
                    }
                    ImGui::Separator();

                    if (bank.Type == EBankType::Shaders || bank.Type == EBankType::Fonts) {
                        ImGui::TextDisabled("Duplicate (Locked for %s)", bank.Type == EBankType::Shaders ? "Shaders" : "Fonts");
                        ImGui::TextDisabled("Delete (Locked for %s)", bank.Type == EBankType::Shaders ? "Shaders" : "Fonts");
                    }
                    else {
                        if (ImGui::MenuItem("Duplicate Entry")) {
                            DuplicateBankEntry(&bank, idx);
                            g_ScrollToSelected = true;
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
                    }
                    ImGui::EndPopup();
                }

                if (bank.SelectedEntryIndex == idx) {
                    ImGui::SetItemDefaultFocus();
                    if (g_ScrollToSelected) {
                        ImGui::SetScrollHereY(0.5f);
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

    ImGui::BeginChild("RightPane", ImVec2(0, 0), true);

    if (g_CurrentMode == EAppMode::Banks && !ImGui::GetIO().WantTextInput) {
        if (g_Keybinds.SaveEntry.IsPressed() && bank.SelectedEntryIndex != -1) SaveEntryChanges(&bank);
        if (g_Keybinds.Compile.IsPressed()) {
            if (bank.Type == EBankType::Audio) SaveAudioBank(&bank);
            else SaveBigBank(&bank);
        }

        if (g_Keybinds.ToggleLeftPanel.IsPressed()) {
            g_ShowLeftPanel = !g_ShowLeftPanel;
        }
    }

    if (bank.SelectedEntryIndex != -1) {
        const auto& e = bank.Entries[bank.SelectedEntryIndex];

        std::string typeName = "Unknown";
        ImVec4 typeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

        if (bank.Type == EBankType::Text) {
            if (e.Type == 0) { typeName = "Type 0 - Text Entry"; typeColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); }
            else if (e.Type == 1) { typeName = "Type 1 - Text Group"; typeColor = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); }
            else if (e.Type == 2) { typeName = "Type 2 - Narrator List"; typeColor = ImVec4(0.8f, 0.6f, 1.0f, 1.0f); }
            else typeName = "Type " + std::to_string(e.Type);
        }
        else if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) {
            if (e.Type == 0x0) { typeName = "Type 0 - Graphic Single"; typeColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); }
            else if (e.Type == 0x1) { typeName = "Type 1 - Graphic Sequence"; typeColor = ImVec4(0.6f, 0.6f, 1.0f, 1.0f); }
            else if (e.Type == 0x2) { typeName = "Type 2 - Bumpmap"; typeColor = ImVec4(1.0f, 0.7f, 0.5f, 1.0f); }
            else if (e.Type == 0x3) { typeName = "Type 3 - Bumpmap Sequence"; typeColor = ImVec4(1.0f, 0.5f, 0.6f, 1.0f); }
            else if (e.Type == 0x4) { typeName = "Type 4 - Volume Texture"; typeColor = ImVec4(0.4f, 0.9f, 0.7f, 1.0f); }
            else if (e.Type == 0x5) { typeName = "Type 5 - Flat Sequence"; typeColor = ImVec4(0.9f, 0.9f, 0.5f, 1.0f); }
            else { typeName = "Type " + std::to_string(e.Type) + " - Texture"; typeColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); }
        }
        else if (bank.Type == EBankType::Effects) {
            if (e.Type == 0) { typeName = "Type 0 - Particle Entry"; typeColor = ImVec4(1.0f, 0.9f, 0.6f, 1.0f); }
            else typeName = "Type " + std::to_string(e.Type) + " - Particle";
        }
        else if (bank.Type == EBankType::Graphics) {
            switch (e.Type) {
            case 1: typeName = "Type 1 - Static Mesh"; typeColor = ImVec4(0.7f, 0.9f, 0.7f, 1.0f); break;
            case 2: typeName = "Type 2 - Repeated Mesh"; typeColor = ImVec4(0.5f, 0.8f, 0.5f, 1.0f); break;
            case 3: typeName = "Type 3 - Physics Mesh (BBM)"; typeColor = ImVec4(0.9f, 0.5f, 0.5f, 1.0f); break;
            case 4: typeName = "Type 4 - Particle Mesh"; typeColor = ImVec4(1.0f, 0.9f, 0.6f, 1.0f); break;
            case 5: typeName = "Type 5 - Animated Mesh"; typeColor = ImVec4(0.6f, 0.9f, 1.0f, 1.0f); break;
            case 6: typeName = "Type 6 - Animation"; typeColor = ImVec4(0.8f, 0.7f, 1.0f, 1.0f); break;
            case 7: typeName = "Type 7 - Delta Animation"; typeColor = ImVec4(0.9f, 0.7f, 1.0f, 1.0f); break;
            case 8: typeName = "Type 8 - Lipsync Animation"; typeColor = ImVec4(1.0f, 0.7f, 0.8f, 1.0f); break;
            case 9: typeName = "Type 9 - Partial Animation"; typeColor = ImVec4(0.7f, 0.7f, 0.9f, 1.0f); break;
            case 10: typeName = "Type 10 - Relative Animation"; typeColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
            default: typeName = "Type " + std::to_string(e.Type) + " - Unknown Mesh"; break;
            }
        }
        else if (bank.Type == EBankType::Shaders) {
            if (e.Type == 0) { typeName = "Type 0 - Vertex Shader"; typeColor = ImVec4(1.0f, 0.6f, 0.8f, 1.0f); }
            else if (e.Type == 1) { typeName = "Type 1 - Pixel Shader"; typeColor = ImVec4(0.6f, 1.0f, 0.8f, 1.0f); }
            else typeName = "Type " + std::to_string(e.Type) + " - Shader";
        }
        else if (bank.Type == EBankType::Fonts) {
            if (e.Type == 0) { typeName = "Type 0 - PC Font Entry"; typeColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); }
            else if (e.Type == 1) { typeName = "Type 1 - Xbox Font Entry"; typeColor = ImVec4(0.7f, 0.9f, 0.5f, 1.0f); }
            else if (e.Type == 2) { typeName = "Type 2 - Glyph Data"; typeColor = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); }
            else typeName = "Type " + std::to_string(e.Type) + " - Font";
        }
        else if (bank.Type == EBankType::Dialogue) {
            if (e.Type == 1) { typeName = "Type 1 - Lipsync Entry"; typeColor = ImVec4(0.8f, 0.7f, 1.0f, 1.0f); }
            else typeName = "Type " + std::to_string(e.Type) + " - Dialogue";
        }
        else if (bank.Type == EBankType::Audio) typeName = "Audio Clip";
        else typeName = "Type " + std::to_string(e.Type);

        ImGui::AlignTextToFramePadding();

        bool isCurrentlyStaged = bank.StagedEntries.count(bank.SelectedEntryIndex) > 0;
        if (isCurrentlyStaged && e.Size == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "ID: %d | ", e.ID);
        }
        else {
            ImGui::Text("ID: %d | ", e.ID);
        }

        ImGui::SameLine(0, 0);
        ImGui::TextColored(typeColor, "%s", typeName.c_str());
        ImGui::SameLine(0, 0);
        if (isCurrentlyStaged && e.Size == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " | Size: [Displayed after compilation]");
        }
        else {
            ImGui::Text(" | Size: %d bytes", e.Size);
        }

        ImGui::SameLine();
        float availRight = ImGui::GetContentRegionAvail().x;
        float buttonsWidth = 140.0f;
        if (availRight > buttonsWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availRight - buttonsWidth);
        }

        if (ImGui::Button("Save", ImVec2(60, 0))) {
            SaveEntryChanges(&bank);
        }

        if (bank.Type != EBankType::Shaders && bank.Type != EBankType::Fonts) {
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
        }

        ImGui::Separator();

        if (bank.Type == EBankType::Audio) {
            if (bank.LugParserPtr) DrawLugAudioProperties(&bank);
            else DrawAudioProperties(&bank);
        }
        else if (bank.Type == EBankType::Dialogue || bank.Type == EBankType::Text) {
            if (e.Name != e.FriendlyName) {
                ImGui::TextDisabled("Internal File Name: %s", e.Name.c_str());
            }
        }
        else {
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

            ImGui::SetNextItemWidth(300);
            if (ImGui::InputText("##globalNameEdit", nameBuf, 256)) {
                bank.Entries[bank.SelectedEntryIndex].Name = nameBuf;
                bank.Entries[bank.SelectedEntryIndex].FriendlyName = nameBuf;

                // Sync changes dynamically to whichever internal struct is currently loaded
                if (bank.Type == EBankType::Graphics && IsSupportedMesh(e.Type) && e.Type != 3) g_ActiveMeshContent.MeshName = nameBuf;
                if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
                    g_TextureParser.PendingName = nameBuf;
                }
                if (e.Type == 6 || e.Type == 7 || e.Type == 9) {
                    g_AnimParser.Data.ObjectName = nameBuf;
                }

                if (bank.Type == EBankType::Effects) {
                    g_ActiveParticleEmitter.Name = nameBuf;
                }
            }

            if (bank.Type == EBankType::Graphics && IsSupportedMesh(e.Type)) {
                if (e.Type == 3) {
                    ImGui::SameLine();
                    if (ImGui::Button("Replace Physics Mesh")) {
                        std::string gltfPath = OpenFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
                        if (!gltfPath.empty()) {
                            CBBMParser newBBM;
                            std::string err = GltfMeshImporter::ImportType3(gltfPath, e.Name, newBBM);
                            if (err.empty()) {
                                if (!bank.StagedEntries.count(bank.SelectedEntryIndex)) SaveEntryChanges(&bank);
                                bank.StagedEntries[bank.SelectedEntryIndex].Physics = std::make_shared<CBBMParser>(newBBM);

                                g_BBMParser = newBBM;
                                g_MeshUploadNeeded = true;
                                g_BankStatus = "Physics Mesh Replaced (Staged).";
                            }
                            else {
                                g_BankStatus = "Import Error: " + err;
                            }
                        }
                    }
                }
                else {
                    ImGui::SameLine();
                    ImGui::Text("LOD:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    std::string lodPreview = std::to_string(bank.SelectedLOD);

                    uint32_t displayLODCount = g_ActiveMeshContent.EntryMeta.LODCount > 0 ? g_ActiveMeshContent.EntryMeta.LODCount : 1;

                    if (ImGui::BeginCombo("##lod", lodPreview.c_str())) {
                        for (uint32_t l = 0; l < displayLODCount; l++) {
                            ImGui::PushID(l);
                            float availW = ImGui::GetContentRegionAvail().x;
                            bool is_selected = (bank.SelectedLOD == l);

                            if (ImGui::Selectable((std::to_string(l) + "##sel").c_str(), is_selected, 0, ImVec2(availW - 30, 0))) {
                                bank.SelectedLOD = l;
                                if (g_ActiveMeshContent.EntryMeta.LODCount > 0) ParseSelectedLOD(&bank);
                            }

                            ImGui::SameLine(availW - 20);
                            if (ImGui::Button("X", ImVec2(20, 0))) {
                                g_PendingLODActionIndex = l;
                                g_ShowDeleteLODPopup = true;
                            }
                            ImGui::PopID();
                        }
                        ImGui::Separator();
                        if (ImGui::Selectable("Add LOD...")) {
                            g_LODImportType = e.Type;
                            g_ShowAddLODPopup = true;
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Replace LOD")) {
                        g_PendingLODActionIndex = bank.SelectedLOD;
                        g_LODImportType = e.Type;
                        g_ShowReplaceLODPopup = true;
                    }
                }

                if (g_ShowAddLODPopup) { ImGui::OpenPopup("Add LOD?"); }
                if (g_ShowDeleteLODPopup) { ImGui::OpenPopup("Delete LOD?"); }
                if (g_ShowReplaceLODPopup) { ImGui::OpenPopup("Replace LOD?"); }

                auto getLODOffsetAndSize = [&](int lodIndex, size_t& outOffset, size_t& outSize) {
                    outOffset = 0; outSize = 0;
                    if (lodIndex < 0 || lodIndex >= g_ActiveMeshContent.EntryMeta.LODCount) return;
                    for (int i = 0; i < lodIndex; i++) outOffset += g_ActiveMeshContent.EntryMeta.LODSizes[i];
                    outSize = g_ActiveMeshContent.EntryMeta.LODSizes[lodIndex];
                    };

                if (ImGui::BeginPopupModal("Add LOD?", &g_ShowAddLODPopup, ImGuiWindowFlags_AlwaysAutoResize)) {

                    ImGui::Text("Select import format for the new LOD:");
                    ImGui::RadioButton("Type 1 (Static)", &g_LODImportType, 1);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 2 (Repeated)", &g_LODImportType, 2);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 4 (Particle)", &g_LODImportType, 4);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 5 (Animated)", &g_LODImportType, 5);

                    if (g_LODImportType == 1 || g_LODImportType == 5) {
                        ImGui::Separator();
                        ImGui::Checkbox("Recalculate Bounds & Scale", &g_ForceRecalculateBounds);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick this if you enlarged the mesh. Untick for Lossless.");
                    }

                    if (g_LODImportType == 2) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Type 2 Settings:");
                        const int allowedReps[] = { 4, 8, 16, 32, 64 };
                        int currentIdx = 0;
                        for (int i = 0; i < 5; i++) { if (g_ImportReps == allowedReps[i]) currentIdx = i; }
                        if (ImGui::SliderInt("Repetitions", &currentIdx, 0, 4, std::to_string(allowedReps[currentIdx]).c_str())) {
                            g_ImportReps = allowedReps[currentIdx];
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::Button("Browse & Add", ImVec2(120, 0))) {
                        std::string gltfPath = OpenFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
                        if (!gltfPath.empty()) {
                            C3DMeshContent newMesh;
                            std::string err;

                            if (g_LODImportType == 1) err = GltfMeshImporter::ImportType1(gltfPath, e.Name, newMesh, g_ForceRecalculateBounds);
                            else if (g_LODImportType == 2) err = GltfMeshImporter::ImportType2(gltfPath, e.Name, newMesh, g_ImportReps);
                            else if (g_LODImportType == 4) err = GltfMeshImporter::ImportType4(gltfPath, e.Name, newMesh);
                            else if (g_LODImportType == 5) err = GltfMeshImporter::ImportType5(gltfPath, e.Name, newMesh, g_ForceRecalculateBounds);

                            if (err.empty()) {
                                if (!bank.StagedEntries.count(bank.SelectedEntryIndex)) SaveEntryChanges(&bank);
                                auto& staged = bank.StagedEntries[bank.SelectedEntryIndex];

                                staged.MeshLODs.push_back(std::make_shared<C3DMeshContent>(newMesh));

                                staged.MeshMeta.LODCount = (uint32_t)staged.MeshLODs.size();
                                staged.MeshMeta.LODSizes.push_back(0);
                                staged.MeshMeta.LODErrors.push_back(0.01f);

                                g_ActiveMeshContent.EntryMeta = staged.MeshMeta;

                                g_BankStatus = "LOD Added (Staged for Compilation).";
                                g_ShowAddLODPopup = false;
                                ImGui::CloseCurrentPopup();
                            }
                            else {
                                g_BankStatus = "Import Error: " + err;
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        g_ShowAddLODPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopupModal("Delete LOD?", &g_ShowDeleteLODPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Are you sure you want to delete LOD %d?", g_PendingLODActionIndex);
                    ImGui::Separator();
                    if (ImGui::Button("Yes, Delete", ImVec2(120, 0))) {
                        if (!bank.StagedEntries.count(bank.SelectedEntryIndex)) SaveEntryChanges(&bank);
                        auto& staged = bank.StagedEntries[bank.SelectedEntryIndex];

                        if (staged.MeshLODs.size() > 1 && g_PendingLODActionIndex < staged.MeshLODs.size()) {
                            staged.MeshLODs.erase(staged.MeshLODs.begin() + g_PendingLODActionIndex);

                            staged.MeshMeta.LODSizes.erase(staged.MeshMeta.LODSizes.begin() + g_PendingLODActionIndex);
                            if (!staged.MeshMeta.LODErrors.empty()) {
                                int eIdx = (std::min)((int)g_PendingLODActionIndex, (int)staged.MeshMeta.LODErrors.size() - 1);
                                staged.MeshMeta.LODErrors.erase(staged.MeshMeta.LODErrors.begin() + eIdx);
                            }
                            staged.MeshMeta.LODCount = (uint32_t)staged.MeshLODs.size();

                            bank.SelectedLOD = 0;
                            ParseSelectedLOD(&bank);
                            g_BankStatus = "LOD deleted (Staged for Compilation).";
                        }
                        g_ShowDeleteLODPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        g_ShowDeleteLODPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopupModal("Replace LOD?", &g_ShowReplaceLODPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Replacing an LOD will auto-compile the mesh.");
                    ImGui::Text("Select import format to replace LOD %d:", g_PendingLODActionIndex);
                    ImGui::RadioButton("Type 1 (Static)", &g_LODImportType, 1);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 2 (Repeated)", &g_LODImportType, 2);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 4 (Particle)", &g_LODImportType, 4);
                    ImGui::SameLine();
                    ImGui::RadioButton("Type 5 (Animated)", &g_LODImportType, 5);

                    if (g_LODImportType == 1 || g_LODImportType == 5) {
                        ImGui::Separator();
                        ImGui::Checkbox("Recalculate Bounds & Scale", &g_ForceRecalculateBounds);
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Tick this if you enlarged the mesh. Untick for Lossless.");
                    }

                    if (g_LODImportType == 2) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Type 2 Settings:");
                        const int allowedReps[] = { 4, 8, 16, 32, 64 };
                        int currentIdx = 0;
                        for (int i = 0; i < 5; i++) { if (g_ImportReps == allowedReps[i]) currentIdx = i; }
                        if (ImGui::SliderInt("Repetitions", &currentIdx, 0, 4, std::to_string(allowedReps[currentIdx]).c_str())) {
                            g_ImportReps = allowedReps[currentIdx];
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::Button("Browse & Replace", ImVec2(120, 0))) {
                        std::string gltfPath = OpenFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
                        if (!gltfPath.empty()) {
                            std::string err;

                            if (g_LODImportType == 3) {
                                CBBMParser newBBM;
                                err = GltfMeshImporter::ImportType3(gltfPath, e.Name, newBBM);
                                if (err.empty()) {
                                    if (!bank.StagedEntries.count(bank.SelectedEntryIndex)) SaveEntryChanges(&bank);
                                    bank.StagedEntries[bank.SelectedEntryIndex].Physics = std::make_shared<CBBMParser>(newBBM);

                                    g_BBMParser = newBBM;
                                    g_MeshUploadNeeded = true;
                                    g_BankStatus = "Physics Mesh Replaced (Staged).";
                                    g_ShowReplaceLODPopup = false;
                                    ImGui::CloseCurrentPopup();
                                }
                                else {
                                    g_BankStatus = "Import Error: " + err;
                                }
                            }
                            else {
                                C3DMeshContent newMesh;
                                if (g_LODImportType == 1) err = GltfMeshImporter::ImportType1(gltfPath, e.Name, newMesh, g_ForceRecalculateBounds);
                                else if (g_LODImportType == 2) err = GltfMeshImporter::ImportType2(gltfPath, e.Name, newMesh, g_ImportReps);
                                else if (g_LODImportType == 4) err = GltfMeshImporter::ImportType4(gltfPath, e.Name, newMesh);
                                else if (g_LODImportType == 5) err = GltfMeshImporter::ImportType5(gltfPath, e.Name, newMesh, g_ForceRecalculateBounds);

                                if (err.empty()) {
                                    if (!bank.StagedEntries.count(bank.SelectedEntryIndex)) SaveEntryChanges(&bank);
                                    auto& staged = bank.StagedEntries[bank.SelectedEntryIndex];

                                    if (g_PendingLODActionIndex < staged.MeshLODs.size()) {
                                        staged.MeshLODs[g_PendingLODActionIndex] = std::make_shared<C3DMeshContent>(newMesh);
                                    }

                                    ParseSelectedLOD(&bank);
                                    g_BankStatus = "LOD Replaced (Staged).";
                                    g_ShowReplaceLODPopup = false;
                                    ImGui::CloseCurrentPopup();
                                }
                                else {
                                    g_BankStatus = "Import Error: " + err;
                                }
                            }
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        g_ShowReplaceLODPopup = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }

        ImGui::Separator();

        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || (bank.Type == EBankType::XboxGraphics && IsTextureSubBank(&bank))) DrawTextureProperties();
        else if (bank.Type == EBankType::Effects) { DrawParticleProperties(g_ActiveParticleEmitter); }
        else if (bank.Type == EBankType::Text) DrawTextProperties(&bank, nullptr, [&](std::string target, uint32_t id, std::string hint) { JumpToBankEntry(target, id, hint); });
        else if (bank.Type == EBankType::Dialogue) DrawLipSyncProperties(&bank, nullptr, nullptr);
        else if ((bank.Type == EBankType::Graphics || (bank.Type == EBankType::XboxGraphics && IsGraphicsSubBank(&bank))) && IsSupportedMesh(e.Type)) DrawMeshProperties(nullptr);
        else if (bank.Type == EBankType::Shaders) { DrawShaderProperties(e.ID); }
        else if ((bank.Type == EBankType::Graphics || (bank.Type == EBankType::XboxGraphics && IsGraphicsSubBank(&bank))) && (e.Type == 6 || e.Type == 7 || e.Type == 9)) {
            DrawAnimProperties(bank.Entries[bank.SelectedEntryIndex].Name, e.ID, bank.Entries[bank.SelectedEntryIndex].Type, g_AnimParser, g_AnimUIState, bank.CurrentEntryRawData);
        }
        else if (bank.Type == EBankType::Fonts) {
            std::string subBank = "";
            if (bank.ActiveSubBankIndex >= 0 && bank.ActiveSubBankIndex < bank.SubBanks.size()) {
                subBank = bank.SubBanks[bank.ActiveSubBankIndex].Name;
            }
            std::string upperSubBank = subBank;
            std::transform(upperSubBank.begin(), upperSubBank.end(), upperSubBank.begin(), ::toupper);

            if (upperSubBank.find("STREAMING") != std::string::npos) {
                g_StreamingFontParser.Parse(bank.CurrentEntryRawData, bank.Entries[bank.SelectedEntryIndex].Type);
                DrawStreamingFontProperties(&bank, bank.SelectedEntryIndex);
            }
            else {
                DrawFontProperties(e.ID);
            }
        }
    }
    ImGui::EndChild();

    if (g_ShowShaderErrorPopup) {
        ImGui::OpenPopup("Shader Compilation Error");
        g_ShowShaderErrorPopup = false;
    }

    if (ImGui::BeginPopupModal("Shader Compilation Error", NULL, ImGuiWindowFlags_None)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "D3DAssemble Failed:");
        ImGui::Separator();

        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 150), ImVec2(800, 600));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::InputTextMultiline("##shader_err",
            (char*)g_ShaderCompileError.c_str(),
            g_ShaderCompileError.size(),
            ImVec2(-FLT_MIN, 250),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }


    if (g_ShowType2SettingsPopup) {
        ImGui::OpenPopup("Type 2 Import Settings");
    }

    if (ImGui::BeginPopupModal("Type 2 Import Settings", &g_ShowType2SettingsPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File Selected:");
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", g_PendingGltfPath.c_str());
        ImGui::Separator();

        ImGui::Text("Set Repetitions for this mesh:");
        const int allowedReps[] = { 4, 8, 16, 32, 64 };
        int currentIdx = 0;
        for (int i = 0; i < 5; i++) { if (g_ImportReps == allowedReps[i]) currentIdx = i; }

        if (ImGui::SliderInt("##reps_staging", &currentIdx, 0, 4, std::to_string(allowedReps[currentIdx]).c_str())) {
            g_ImportReps = allowedReps[currentIdx];
        }

        int maxVerts = 65535 / g_ImportReps;
        ImGui::TextDisabled("Max base vertices allowed: %d", maxVerts);

        ImGui::Separator();

        if (ImGui::Button("Finalize Import", ImVec2(120, 0))) {
            if (g_PendingLODAction == 2) {
                if (CreateNewMeshEntry(&g_OpenBanks[g_ActiveBankIndex], g_PendingGltfPath, 2, g_ImportReps)) {
                    g_BankStatus = "New Type 2 Mesh Created Successfully!";
                    g_ScrollToSelected = true;
                    g_ShowType2SettingsPopup = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            else {
                std::string meshName = "UnknownMesh";
                if (bank.SelectedEntryIndex != -1 && bank.SelectedEntryIndex < bank.Entries.size()) {
                    meshName = bank.Entries[bank.SelectedEntryIndex].Name;
                }

                C3DMeshContent newMesh;
                std::string err = GltfMeshImporter::ImportType2(g_PendingGltfPath, meshName, newMesh, g_ImportReps);

                if (err.empty()) {
                    auto originalMeta = g_ActiveMeshContent.EntryMeta;
                    g_ActiveMeshContent = newMesh;
                    g_ActiveMeshContent.EntryMeta = originalMeta;

                    g_MeshUploadNeeded = true;
                    SaveEntryChanges(&bank);
                    g_BankStatus = (g_PendingLODAction == 0) ? "LOD Added and Compiled Successfully!" : "LOD Replaced and Compiled Successfully!";
                    g_ShowType2SettingsPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                else {
                    g_BankStatus = "Import Error: " + err;
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowType2SettingsPopup = false;
            g_PendingGltfPath = "";
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

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
}