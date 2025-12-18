#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "GltfExporter.h"
#include <windows.h>

static void SelectEntry(int idx) {
    if (idx < 0 || idx >= (int)g_CurrentBank.Entries.size()) return;

    g_TextureParser.DecodedPixels.clear();
    g_TextureParser.IsParsed = false;
    std::vector<uint8_t>().swap(g_TextureParser.DecodedPixels);

    g_SelectedEntryIndex = idx;
    g_SelectedLOD = 0;
    const auto& e = g_CurrentBank.Entries[idx];

    g_BankStream.clear();
    g_BankStream.seekg(e.Offset, std::ios::beg);

    size_t fileSize = (e.Size > 50000000) ? 50000000 : e.Size;
    size_t paddedSize = fileSize + 64;

    g_CurrentEntryRawData.clear();
    g_CurrentEntryRawData.resize(paddedSize, 0);
    g_BankStream.read((char*)g_CurrentEntryRawData.data(), fileSize);

    g_ActiveMeshContent = C3DMeshContent();
    g_BBMParser.IsParsed = false;
    g_AnimParseSuccess = false;

    if (g_CurrentBank.Type == EBankType::Textures) {
        if (g_SubheaderCache.count(idx)) {
            g_TextureParser.Parse(g_SubheaderCache[idx], g_CurrentEntryRawData, e.Type);
        }
    }
    else {
        if (e.Type == TYPE_STATIC_PHYSICS_MESH) {
            g_BBMParser.Parse(g_CurrentEntryRawData);
            g_MeshUploadNeeded = true; // [FIX] Signal GPU upload
        }
        else if (IsSupportedMesh(e.Type)) {
            if (g_SubheaderCache.count(idx)) {
                g_ActiveMeshContent.ParseEntryMetadata(g_SubheaderCache[idx]);
            }
            ParseSelectedLOD();
            g_MeshUploadNeeded = true; // [FIX] Signal GPU upload
        }
        else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
            if (g_SubheaderCache.count(idx)) {
                g_AnimParseSuccess = g_ActiveAnim.Deserialize(g_SubheaderCache[idx]);
            }
        }
    }
}

static std::string OpenFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Big Bank Files\0*.big\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) return std::string(ofn.lpstrFile);
    return "";
}

static void DrawBankExplorer() {
    if (ImGui::Button("Open .BIG File")) {
        std::string path = OpenFileDialog();
        if (!path.empty()) LoadBank(path);
    }
    ImGui::SameLine();
    ImGui::Text("%s", g_CurrentBank.FileName.c_str());

    if (!g_CurrentBank.SubBanks.empty()) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250);

        std::string preview = "Select Folder";
        if (g_CurrentBank.ActiveSubBankIndex >= 0 && g_CurrentBank.ActiveSubBankIndex < (int)g_CurrentBank.SubBanks.size()) {
            preview = g_CurrentBank.SubBanks[g_CurrentBank.ActiveSubBankIndex].Name;
        }

        if (ImGui::BeginCombo("##folder_selector", preview.c_str())) {
            for (int i = 0; i < (int)g_CurrentBank.SubBanks.size(); i++) {
                const bool is_selected = (g_CurrentBank.ActiveSubBankIndex == i);
                std::string label = g_CurrentBank.SubBanks[i].Name + " (" + std::to_string(g_CurrentBank.SubBanks[i].EntryCount) + ")";

                if (ImGui::Selectable(label.c_str(), is_selected)) {
                    LoadSubBankEntries(i);
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::TextDisabled("%s", g_BankStatus.c_str());
    ImGui::Separator();

    ImGui::BeginChild("LeftPane", ImVec2(300, 0), true);
    ImGui::Text("Search:"); ImGui::SameLine();
    if (ImGui::InputText("##filter", g_FilterText, 128)) UpdateFilter();

    if (!g_CurrentBank.Entries.empty()) {

        int currentFilteredIdx = -1;
        for (int i = 0; i < (int)g_CurrentBank.FilteredIndices.size(); i++) {
            if (g_CurrentBank.FilteredIndices[i] == g_SelectedEntryIndex) {
                currentFilteredIdx = i;
                break;
            }
        }

        bool forceScroll = false;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && currentFilteredIdx > 0) {
            SelectEntry(g_CurrentBank.FilteredIndices[currentFilteredIdx - 1]);
            forceScroll = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && currentFilteredIdx < (int)g_CurrentBank.FilteredIndices.size() - 1) {
            SelectEntry(g_CurrentBank.FilteredIndices[currentFilteredIdx + 1]);
            forceScroll = true;
        }

        ImGui::BeginChild("EntryListScroll", ImVec2(0, 0), false);
        for (int idx : g_CurrentBank.FilteredIndices) {
            const auto& e = g_CurrentBank.Entries[idx];

            if (ImGui::Selectable(e.Name.c_str(), g_SelectedEntryIndex == idx)) {
                SelectEntry(idx);
            }

            if (forceScroll && g_SelectedEntryIndex == idx) {
                ImGui::SetScrollHereY();
            }
        }
        ImGui::EndChild();
    }
    else {
        ImGui::TextDisabled("No entries found in this folder.");
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
    if (g_SelectedEntryIndex != -1) {
        const auto& e = g_CurrentBank.Entries[g_SelectedEntryIndex];
        ImGui::Text("Entry: %s (ID: %d)", e.Name.c_str(), e.ID);
        ImGui::Text("Type: %d | Total Size: %d | CRC: %08X", e.Type, e.Size, e.CRC);

        if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "LOD MANAGEMENT");

            std::string preview = "LOD " + std::to_string(g_SelectedLOD) +
                " (" + std::to_string(g_ActiveMeshContent.EntryMeta.LODSizes[g_SelectedLOD]) + " bytes)";

            if (ImGui::BeginCombo("Select LOD", preview.c_str())) {
                for (uint32_t i = 0; i < g_ActiveMeshContent.EntryMeta.LODCount; i++) {
                    std::string label = "LOD " + std::to_string(i) + " (" +
                        std::to_string(g_ActiveMeshContent.EntryMeta.LODSizes[i]) + " bytes)";
                    const bool is_selected = (g_SelectedLOD == i);
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        g_SelectedLOD = i;
                        ParseSelectedLOD();
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        bool canExport = g_ActiveMeshContent.IsParsed || g_BBMParser.IsParsed;
        if (canExport) {
            if (ImGui::Button("EXPORT TO GLTF")) {
                OPENFILENAMEA ofn;
                char szFile[260] = { 0 };
                std::string defaultName = (g_BBMParser.IsParsed ? "physics_export" : g_ActiveMeshContent.MeshName) + ".gltf";
                strcpy_s(szFile, defaultName.c_str());

                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = "glTF 2.0\0*.gltf\0All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

                if (GetSaveFileNameA(&ofn) == TRUE) {
                    bool success = (g_BBMParser.IsParsed) ? GltfExporter::ExportBBM(g_BBMParser, ofn.lpstrFile) : GltfExporter::Export(g_ActiveMeshContent, ofn.lpstrFile);
                    g_BankStatus = success ? "Exported: " + std::string(ofn.lpstrFile) : "Error: Export Failed";
                }
            }
            ImGui::Separator();
        }

        if (g_CurrentBank.Type == EBankType::Textures) {
            DrawTextureProperties();
        }
        else if (IsSupportedMesh(e.Type)) {
            DrawMeshProperties(SaveEntryChanges);
        }
        else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
            DrawAnimProperties(g_ActiveAnim, g_AnimParseSuccess, g_AnimUIState, SaveEntryChanges);
        }
        else {
            ImGui::TextDisabled("Preview not implemented for asset type %d.", e.Type);
        }
    }
    ImGui::EndChild();
}