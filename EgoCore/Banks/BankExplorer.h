#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "DefExplorer.h"
#include "ConfigBackend.h" // New Include
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "TextProperties.h"
#include "GltfExporter.h"
#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>

// --- HELPER: Init wrapper ---
static bool g_HasInitialized = false;

// --- Helper Functions for Bank Management ---
// (Keeping your existing SelectEntry, SaveEntry, etc. unchanged)
static void SelectEntry(LoadedBank* bank, int idx) {
    if (!bank || idx < 0 || idx >= (int)bank->Entries.size()) return;

    g_TextureParser.DecodedPixels.clear();
    g_TextureParser.IsParsed = false;
    std::vector<uint8_t>().swap(g_TextureParser.DecodedPixels);

    g_BBMParser.IsParsed = false;
    g_ActiveMeshContent = C3DMeshContent();
    g_AnimParseSuccess = false;

    g_TextParser.IsParsed = false;
    g_TextParser.TextData = CTextEntry();
    g_TextParser.GroupData = CTextGroup();

    bank->SelectedEntryIndex = idx;
    bank->SelectedLOD = 0;
    const auto& e = bank->Entries[idx];

    bank->Stream->clear();
    bank->Stream->seekg(e.Offset, std::ios::beg);
    size_t fileSize = (e.Size > 50000000) ? 50000000 : e.Size;
    size_t paddedSize = fileSize + 64;

    bank->CurrentEntryRawData.clear();
    bank->CurrentEntryRawData.resize(paddedSize, 0);
    bank->Stream->read((char*)bank->CurrentEntryRawData.data(), fileSize);

    if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend) {
        if (bank->SubheaderCache.count(idx)) {
            g_TextureParser.Parse(bank->SubheaderCache[idx], bank->CurrentEntryRawData, e.Type);
        }
    }
    else if (bank->Type == EBankType::Text || bank->Type == EBankType::Dialogue) {
        g_TextParser.Parse(bank->CurrentEntryRawData, e.Type);
        if (g_TextParser.IsGroup) ResolveGroupMetadata(bank);
    }
    else {
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

static std::string OpenFileDialog() {
    OPENFILENAMEA ofn; char szFile[260] = { 0 }; ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = NULL; ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Big Bank Files\0*.big\0All Files\0*.*\0"; ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) return std::string(ofn.lpstrFile);
    return "";
}

// --- TAB 1: BANK UI CONTENT ---
static void DrawBankTab() {
    static float bankSidebarWidth = 300.0f;
    if (bankSidebarWidth < 50.0f) bankSidebarWidth = 50.0f;
    if (bankSidebarWidth > ImGui::GetWindowWidth() - 100.0f) bankSidebarWidth = ImGui::GetWindowWidth() - 100.0f;

    if (g_OpenBanks.empty()) {
        ImGui::TextDisabled("No bank files loaded.");
        if (ImGui::Button("Manually Load .BIG")) {
            std::string path = OpenFileDialog();
            if (!path.empty()) LoadBank(path);
        }
    }
    else {
        if (ImGui::BeginTabBar("BankFiles", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
            for (int i = 0; i < (int)g_OpenBanks.size(); ) {
                LoadedBank& bank = g_OpenBanks[i];
                bool keepOpen = true;

                std::string tabLabel = bank.FileName + "##" + std::to_string(i);

                if (ImGui::BeginTabItem(tabLabel.c_str(), &keepOpen)) {
                    if (g_ActiveBankIndex != i) {
                        g_ActiveBankIndex = i;
                        if (bank.SelectedEntryIndex != -1) SelectEntry(&bank, bank.SelectedEntryIndex);
                        else {
                            g_TextureParser.IsParsed = false;
                            g_ActiveMeshContent.IsParsed = false;
                            g_BBMParser.IsParsed = false;
                        }
                    }

                    if (!bank.SubBanks.empty()) {
                        std::string preview = "Select Sub-Bank";
                        if (bank.ActiveSubBankIndex >= 0) preview = bank.SubBanks[bank.ActiveSubBankIndex].Name;

                        ImGui::SetNextItemWidth(300);
                        if (ImGui::BeginCombo("##folder", preview.c_str())) {
                            for (int s = 0; s < (int)bank.SubBanks.size(); s++) {
                                bool is_sel = (bank.ActiveSubBankIndex == s);
                                if (ImGui::Selectable((bank.SubBanks[s].Name + " (" + std::to_string(bank.SubBanks[s].EntryCount) + ")").c_str(), is_sel)) {
                                    LoadSubBankEntries(bank, s);
                                }
                            }
                            ImGui::EndCombo();
                        }
                    }
                    ImGui::Separator();

                    ImGui::BeginChild("LeftPane", ImVec2(bankSidebarWidth, 0), true);
                    ImGui::InputText("Search", bank.FilterText, 128);
                    if (ImGui::IsItemEdited()) UpdateFilter(bank);

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

                    if (!bank.Entries.empty()) {
                        ImGui::BeginChild("ListScroll", ImVec2(0, 0), false);
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

                    ImGui::SameLine();
                    ImGui::InvisibleButton("vsplitterBank", ImVec2(4.0f, -1.0f));
                    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    if (ImGui::IsItemActive()) bankSidebarWidth += ImGui::GetIO().MouseDelta.x;
                    ImGui::SameLine();

                    ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
                    if (bank.SelectedEntryIndex != -1) {
                        const auto& e = bank.Entries[bank.SelectedEntryIndex];
                        ImGui::Text("ID: %d | Type: %d | Size: %d bytes", e.ID, e.Type, e.Size);

                        if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
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

                        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) DrawTextureProperties();
                        else if (bank.Type == EBankType::Text || bank.Type == EBankType::Dialogue) DrawTextProperties();
                        else if (IsSupportedMesh(e.Type)) DrawMeshProperties([&]() { SaveEntryChanges(&bank); });
                        else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION)
                            DrawAnimProperties(g_ActiveAnim, g_AnimParseSuccess, g_AnimUIState, [&]() { SaveEntryChanges(&bank); });
                        else ImGui::TextDisabled("No preview available.");
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
            ImGui::EndTabBar();
        }
    }
}

// --- MAIN UI RENDER ---
static void DrawBankExplorer() {

    // --- 1. STARTUP CHECK ---
    if (!g_HasInitialized) {
        LoadConfig();
        if (g_AppConfig.IsConfigured) {
            PerformAutoLoad();
        }
        g_HasInitialized = true;
    }

    // --- 2. SETUP SCREEN (If Config Missing) ---
    if (!g_AppConfig.IsConfigured) {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 200));

        if (ImGui::Begin("Welcome to EgoCore", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("Welcome to EgoCore!");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextWrapped("To get started, please select your main Fable game folder (The folder containing Fable.exe and Data).");

            ImGui::Dummy(ImVec2(0, 20));

            if (ImGui::Button("Select Game Folder", ImVec2(-1, 40))) {
                std::string root = OpenFolderDialog(); // From DefBackend.h
                if (!root.empty()) {
                    InitializeSetup(root);
                }
            }
        }
        ImGui::End();
        return; // Stop rendering main UI until configured
    }

    // --- 3. MAIN APP UI ---

    // Global Menu
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open .BIG File")) {
                std::string path = OpenFileDialog();
                if (!path.empty()) LoadBank(path);
            }
            if (ImGui::MenuItem("Change Game Folder")) {
                std::string root = OpenFolderDialog();
                if (!root.empty()) InitializeSetup(root);
            }
            ImGui::EndMenu();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("| %s", g_BankStatus.c_str());
        ImGui::EndMainMenuBar();
    }

    // Tabs
    if (ImGui::BeginTabBar("ModeTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Bank Archives")) {
            DrawBankTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Game Definitions")) {
            DrawDefTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}