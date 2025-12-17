#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "GltfExporter.h"
#include <windows.h>

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
    ImGui::TextDisabled("%s", g_BankStatus.c_str());

    ImGui::Separator();

    // --- LEFT PANE ---
    ImGui::BeginChild("LeftPane", ImVec2(300, 0), true);
    ImGui::Text("Search:"); ImGui::SameLine();
    if (ImGui::InputText("##filter", g_FilterText, 128)) UpdateFilter();

    for (int idx : g_CurrentBank.FilteredIndices) {
        const auto& e = g_CurrentBank.Entries[idx];

        if (ImGui::Selectable(e.Name.c_str(), g_SelectedEntryIndex == idx)) {
            g_SelectedEntryIndex = idx;
            g_SelectedLOD = 0;

            g_BankStream.clear();
            g_BankStream.seekg(e.Offset, std::ios::beg);
            size_t bytesToRead = (e.Size > 20000000) ? 20000000 : e.Size;
            g_CurrentEntryRawData.resize(bytesToRead);
            g_BankStream.read((char*)g_CurrentEntryRawData.data(), bytesToRead);

            g_ActiveMeshContent = C3DMeshContent();
            g_BBMParser.IsParsed = false;
            g_AnimParseSuccess = false; // Reset Anim state

            if (e.Type == TYPE_STATIC_PHYSICS_MESH) {
                g_BBMParser.Parse(g_CurrentEntryRawData);
            }
            else if (IsSupportedMesh(e.Type)) {
                if (g_SubheaderCache.count(idx)) {
                    g_ActiveMeshContent.ParseEntryMetadata(g_SubheaderCache[idx]);
                }
                ParseSelectedLOD();
            }
            else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
                if (g_SubheaderCache.count(idx)) {
                    g_AnimParseSuccess = g_ActiveAnim.Deserialize(g_SubheaderCache[idx]);
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- RIGHT PANE ---
    ImGui::BeginChild("RightPane", ImVec2(0, 0), true);
    if (g_SelectedEntryIndex != -1) {
        const auto& e = g_CurrentBank.Entries[g_SelectedEntryIndex];
        ImGui::Text("Entry: %s (ID: %d)", e.Name.c_str(), e.ID);
        ImGui::Text("Type: %d | Total Size: %d", e.Type, e.Size);

        // LOD SELECTOR
        if (IsSupportedMesh(e.Type) && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "LOD MANAGEMENT");

            uint32_t totalLODSize = 0;
            for (uint32_t s : g_ActiveMeshContent.EntryMeta.LODSizes) totalLODSize += s;
            int32_t containerSize = (int32_t)e.Size - (int32_t)totalLODSize;

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

            if (containerSize > 0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[+] Dummy Container Detected");
                ImGui::SameLine();
                ImGui::TextDisabled("(%d extra bytes)", containerSize);
            }
        }

        ImGui::Separator();

        // EXPORT TO GLTF BUTTON (SUPPORTS BOTH TYPES)
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
                    bool success = false;

                    if (g_BBMParser.IsParsed) {
                        success = GltfExporter::ExportBBM(g_BBMParser, ofn.lpstrFile);
                    }
                    else {
                        success = GltfExporter::Export(g_ActiveMeshContent, ofn.lpstrFile);
                    }

                    if (success) g_BankStatus = "Exported: " + std::string(ofn.lpstrFile);
                    else g_BankStatus = "Error: Export Failed";
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Exports Geometry + Rig)");
            ImGui::Separator();
        }

        // PROPERTIES
        if (IsSupportedMesh(e.Type)) {
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