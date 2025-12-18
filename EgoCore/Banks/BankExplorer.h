#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "TextProperties.h"
#include "GltfExporter.h"
#include <windows.h>
#include <algorithm>

static void SelectEntry(LoadedBank* bank, int idx) {
    if (!bank || idx < 0 || idx >= (int)bank->Entries.size()) return;

    // 1. Reset Global Parsers
    g_TextureParser.DecodedPixels.clear(); g_TextureParser.IsParsed = false;
    std::vector<uint8_t>().swap(g_TextureParser.DecodedPixels);
    g_BBMParser.IsParsed = false; g_ActiveMeshContent = C3DMeshContent(); g_AnimParseSuccess = false;

    // Reset Text Parser (New Structs)
    g_TextParser.IsParsed = false;
    g_TextParser.TextData = CTextEntry();
    g_TextParser.GroupData = CTextGroup();

    // 2. Set Selection
    bank->SelectedEntryIndex = idx;
    bank->SelectedLOD = 0;
    const auto& e = bank->Entries[idx];

    // 3. Read Data
    bank->Stream->clear();
    bank->Stream->seekg(e.Offset, std::ios::beg);
    size_t fileSize = (e.Size > 50000000) ? 50000000 : e.Size;
    size_t paddedSize = fileSize + 64;
    bank->CurrentEntryRawData.clear();
    bank->CurrentEntryRawData.resize(paddedSize, 0);
    bank->Stream->read((char*)bank->CurrentEntryRawData.data(), fileSize);

    // 4. Parse
    if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend) {
        if (bank->SubheaderCache.count(idx)) {
            g_TextureParser.Parse(bank->SubheaderCache[idx], bank->CurrentEntryRawData, e.Type);
        }
    }
    else if (bank->Type == EBankType::Text || bank->Type == EBankType::Dialogue) {
        g_TextParser.Parse(bank->CurrentEntryRawData, e.Type);

        // [FIX] If it's a group, resolve the names/content NOW (Once)
        if (g_TextParser.IsGroup) {
            ResolveGroupMetadata(bank);
        }
    }
    else {
        // ... (Mesh/Anim logic remains the same) ...
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
    g_MeshUploadNeeded = true; // Signal GPU
}

inline void SaveEntryChanges(LoadedBank* bank) {
    if (!bank || bank->SelectedEntryIndex == -1) return;
    BankEntry& e = bank->Entries[bank->SelectedEntryIndex];
    std::vector<uint8_t> newBytes;

    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) newBytes = g_ActiveAnim.Serialize();
    else return; // Only Animation saving supported currently

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

static void DrawBankExplorer() {
    // 1. Toolbar
    if (ImGui::Button("Open .BIG File")) {
        std::string path = OpenFileDialog();
        if (!path.empty()) LoadBank(path);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", g_BankStatus.c_str());
    ImGui::Separator();

    if (g_OpenBanks.empty()) {
        ImGui::Text("No banks open.");
        return;
    }

    // 2. Tabs
    if (ImGui::BeginTabBar("BankTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        for (int i = 0; i < (int)g_OpenBanks.size(); ) {
            LoadedBank& bank = g_OpenBanks[i];
            bool keepOpen = true;

            // Tab Item
            if (ImGui::BeginTabItem(bank.FileName.c_str(), &keepOpen)) {

                // Context Switch logic: If we change tabs, restore that tab's selection
                if (g_ActiveBankIndex != i) {
                    g_ActiveBankIndex = i;
                    if (bank.SelectedEntryIndex != -1) {
                        SelectEntry(&bank, bank.SelectedEntryIndex);
                    }
                    else {
                        // Clear viewers if nothing selected
                        g_TextureParser.IsParsed = false;
                        g_ActiveMeshContent.IsParsed = false;
                        g_BBMParser.IsParsed = false;
                        g_TextParser.IsParsed = false;
                    }
                }

                // Internal Folder/Sub-Bank Selector
                if (!bank.SubBanks.empty()) {
                    std::string preview = "Select Folder";
                    if (bank.ActiveSubBankIndex >= 0) preview = bank.SubBanks[bank.ActiveSubBankIndex].Name;
                    ImGui::SetNextItemWidth(250);
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

                // --- LEFT PANE (LIST) ---
                ImGui::BeginChild("LeftPane", ImVec2(300, 0), true);
                ImGui::Text("Search:"); ImGui::SameLine();
                if (ImGui::InputText("##filter", bank.FilterText, 128)) UpdateFilter(bank);

                if (!bank.Entries.empty()) {
                    // [FIX] Scroll Logic
                    bool forceScroll = false;

                    // Keyboard Navigation
                    int currentFilteredIdx = -1;
                    for (int f = 0; f < (int)bank.FilteredIndices.size(); f++) {
                        if (bank.FilteredIndices[f] == bank.SelectedEntryIndex) { currentFilteredIdx = f; break; }
                    }

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentFilteredIdx > 0) {
                            SelectEntry(&bank, bank.FilteredIndices[currentFilteredIdx - 1]);
                            forceScroll = true; // Only force scroll on key press
                        }
                        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentFilteredIdx < (int)bank.FilteredIndices.size() - 1) {
                            SelectEntry(&bank, bank.FilteredIndices[currentFilteredIdx + 1]);
                            forceScroll = true; // Only force scroll on key press
                        }
                    }

                    // Render List
                    ImGui::BeginChild("EntryListScroll", ImVec2(0, 0), false);
                    for (int idx : bank.FilteredIndices) {
                        const auto& e = bank.Entries[idx];
                        if (ImGui::Selectable(e.Name.c_str(), bank.SelectedEntryIndex == idx)) {
                            SelectEntry(&bank, idx);
                        }

                        // [FIX] Only snap scroll if requested via keyboard
                        if (forceScroll && bank.SelectedEntryIndex == idx) {
                            ImGui::SetScrollHereY();
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                // --- RIGHT PANE (INSPECTOR) ---
                ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
                if (bank.SelectedEntryIndex != -1) {
                    const auto& e = bank.Entries[bank.SelectedEntryIndex];
                    ImGui::Text("Entry: %s (ID: %d)", e.Name.c_str(), e.ID);
                    ImGui::Text("Type: %d | Size: %d", e.Type, e.Size);

                    // LOD Selector (Only for Meshes)
                    if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
                        ImGui::Separator();
                        std::string preview = "LOD " + std::to_string(bank.SelectedLOD) + " (" + std::to_string(g_ActiveMeshContent.EntryMeta.LODSizes[bank.SelectedLOD]) + " bytes)";
                        if (ImGui::BeginCombo("Select LOD", preview.c_str())) {
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

                    // Global Export Button
                    if (g_ActiveMeshContent.IsParsed || g_BBMParser.IsParsed) {
                        if (ImGui::Button("EXPORT TO GLTF")) {
                            OPENFILENAMEA ofn; char szFile[260] = { 0 }; ZeroMemory(&ofn, sizeof(ofn));
                            std::string def = (g_BBMParser.IsParsed ? "phys" : g_ActiveMeshContent.MeshName) + ".gltf";
                            strcpy_s(szFile, def.c_str()); ofn.lStructSize = sizeof(ofn); ofn.lpstrFile = szFile; ofn.nMaxFile = 260; ofn.lpstrFilter = "glTF\0*.gltf\0"; ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
                            if (GetSaveFileNameA(&ofn) == TRUE) {
                                bool ok = (g_BBMParser.IsParsed) ? GltfExporter::ExportBBM(g_BBMParser, ofn.lpstrFile) : GltfExporter::Export(g_ActiveMeshContent, ofn.lpstrFile);
                                g_BankStatus = ok ? "Exported." : "Export Failed.";
                            }
                        }
                        ImGui::Separator();
                    }

                    // --- VIEWPORT ROUTING ---
                    if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend) {
                        DrawTextureProperties();
                    }
                    else if (bank.Type == EBankType::Text || bank.Type == EBankType::Dialogue) {
                        DrawTextProperties(); // Text & Groups
                    }
                    else if (IsSupportedMesh(e.Type)) {
                        DrawMeshProperties([&]() { SaveEntryChanges(&bank); });
                    }
                    else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
                        DrawAnimProperties(g_ActiveAnim, g_AnimParseSuccess, g_AnimUIState, [&]() { SaveEntryChanges(&bank); });
                    }
                    else {
                        ImGui::TextDisabled("Preview not available for this type.");
                    }
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // Tab Closing Logic (Safe erase)
            if (!keepOpen) {
                g_OpenBanks.erase(g_OpenBanks.begin() + i);
                if (g_OpenBanks.empty()) g_ActiveBankIndex = -1;
                else if (g_ActiveBankIndex >= i) g_ActiveBankIndex = (std::max)(0, g_ActiveBankIndex - 1);
            }
            else {
                i++;
            }
        }
        ImGui::EndTabBar();
    }
}