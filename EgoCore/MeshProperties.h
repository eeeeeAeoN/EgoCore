#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "C3DViewports.h"
#include <functional>

static void DrawMatrix4x3(const float* m, const char* label) {
    if (ImGui::TreeNode(label)) {
        ImGui::Text("Right: %.3f, %.3f, %.3f", m[0], m[1], m[2]);
        ImGui::Text("Up:    %.3f, %.3f, %.3f", m[3], m[4], m[5]);
        ImGui::Text("Look:  %.3f, %.3f, %.3f", m[6], m[7], m[8]);
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "Pos:   %.3f, %.3f, %.3f", m[9], m[10], m[11]);
        ImGui::TreePop();
    }
}

static ImVec4 UnpackBGRA(uint32_t bgra) {
    float b = (float)(bgra & 0xFF) / 255.0f;
    float g = (float)((bgra >> 8) & 0xFF) / 255.0f;
    float r = (float)((bgra >> 16) & 0xFF) / 255.0f;
    float a = (float)((bgra >> 24) & 0xFF) / 255.0f;
    return ImVec4(r, g, b, a);
}

inline void DrawMeshProperties(std::function<void()> saveCallback = nullptr) {

    if (g_BBMParser.IsParsed) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "BBM FORMAT (Physics Mesh)");
        ImGui::Separator();

        DrawBBMMeshViewport(g_BBMParser);
        ImGui::Separator();

        if (ImGui::BeginTabBar("BBMTabs")) {

            // TAB: Overview
            if (ImGui::BeginTabItem("Overview")) {
                ImGui::Text("Version: %u", g_BBMParser.FileVersion);
                ImGui::Text("Comment: %s", g_BBMParser.FileComment.c_str());
                ImGui::Separator();
                ImGui::Text("Vertices: %zu", g_BBMParser.ParsedVertices.size());
                ImGui::Text("Indices:  %zu", g_BBMParser.ParsedIndices.size());
                ImGui::Text("Bones:    %zu", g_BBMParser.Bones.size());
                ImGui::Text("Helpers:  %zu", g_BBMParser.Helpers.size());
                ImGui::Text("Dummies:  %zu", g_BBMParser.Dummies.size());
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Materials")) {
                if (g_BBMParser.ParsedMaterials.empty()) {
                    ImGui::TextDisabled("No material chunks found.");
                }
                else {
                    ImGui::Text("Material Count: %zu", g_BBMParser.ParsedMaterials.size());

                    // Table with more columns for the new data
                    if (ImGui::BeginTable("BBMMatTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {
                        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30);
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120);
                        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 40);
                        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableSetupColumn("Ambient", ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableSetupColumn("Diffuse", ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableSetupColumn("Physics (Shiny/Str/Trans)", ImGuiTableColumnFlags_WidthFixed, 150);
                        ImGui::TableSetupColumn("Texture Maps", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        for (size_t i = 0; i < g_BBMParser.ParsedMaterials.size(); i++) {
                            const auto& mat = g_BBMParser.ParsedMaterials[i];
                            ImGui::TableNextRow();

                            // 0: ID
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%zu", i);

                            // 1: Name
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextUnformatted(mat.Name.c_str());

                            // 2: Index
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%d", mat.Index);

                            // 3: Flags
                            ImGui::TableSetColumnIndex(3);
                            if (mat.TwoSided) ImGui::TextColored(ImVec4(0, 1, 0, 1), "2-Side");
                            else ImGui::TextDisabled("-");

                            // 4: Ambient Color (BGRA -> RGBA)
                            ImGui::TableSetColumnIndex(4);
                            ImVec4 colAmb = UnpackBGRA(mat.Ambient);
                            ImGui::ColorButton(("Amb##" + std::to_string(i)).c_str(), colAmb, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(20, 20));
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ambient: R:%.2f G:%.2f B:%.2f A:%.2f", colAmb.x, colAmb.y, colAmb.z, colAmb.w);

                            // 5: Diffuse Color (BGRA -> RGBA)
                            ImGui::TableSetColumnIndex(5);
                            ImVec4 colDif = UnpackBGRA(mat.Diffuse);
                            ImGui::ColorButton(("Dif##" + std::to_string(i)).c_str(), colDif, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(20, 20));
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Diffuse: R:%.2f G:%.2f B:%.2f A:%.2f", colDif.x, colDif.y, colDif.z, colDif.w);

                            // 6: Physics
                            ImGui::TableSetColumnIndex(6);
                            ImGui::Text("%.2f / %.2f / %.2f", mat.Shiny, mat.ShinyStrength, mat.Transparency);

                            // 7: Maps
                            ImGui::TableSetColumnIndex(7);
                            if (mat.Maps.empty()) {
                                ImGui::TextDisabled("None");
                            }
                            else {
                                if (ImGui::TreeNode((void*)(intptr_t)i, "%zu Maps", mat.Maps.size())) {
                                    for (const auto& map : mat.Maps) {
                                        ImGui::TextColored(ImVec4(0.5f, 1, 1, 1), "[%d] %s", map.Type, map.Filename.c_str());
                                        if (!map.Description.empty()) {
                                            ImGui::SameLine();
                                            ImGui::TextDisabled("(%s)", map.Description.c_str());
                                        }
                                    }
                                    ImGui::TreePop();
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            // TAB: Bones
            if (ImGui::BeginTabItem("Bones")) {
                ImGui::Text("Total Bones: %zu", g_BBMParser.Bones.size());

                if (ImGui::BeginTable("BonesTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Parent", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                    ImGui::TableSetupColumn("Tree (Child/Sibl)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Transform");
                    ImGui::TableHeadersRow();

                    for (const auto& bone : g_BBMParser.Bones) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", bone.Index);
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(bone.Name.c_str());

                        ImGui::TableSetColumnIndex(2);
                        if (bone.ParentIndex == -1) ImGui::TextDisabled("Root");
                        else ImGui::Text("%d", bone.ParentIndex);

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%d / %d", bone.FirstChildIndex, bone.NextSiblingIndex);

                        ImGui::TableSetColumnIndex(4);
                        std::string label = "Matrix##Bone" + std::to_string(bone.Index);
                        DrawMatrix4x3(bone.LocalTransform, label.c_str());
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            // TAB: Helpers/Dummies
            if (ImGui::BeginTabItem("Helpers/Dummies")) {
                if (ImGui::CollapsingHeader("Helper Points (HPNT)", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text("Count: %zu", g_BBMParser.Helpers.size());
                    if (ImGui::BeginTable("BBMHelpers", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("SubMesh", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Position");
                        ImGui::TableHeadersRow();

                        int id = 0;
                        for (const auto& h : g_BBMParser.Helpers) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", id++);
                            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(h.Name.c_str());
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", h.BoneIndex);
                            ImGui::TableSetColumnIndex(3);
                            if (h.SubMeshIndex == -1) ImGui::TextDisabled("-"); else ImGui::Text("%d", h.SubMeshIndex);
                            ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f, %.3f, %.3f", h.Position.x, h.Position.y, h.Position.z);
                        }
                        ImGui::EndTable();
                    }
                }

                if (ImGui::CollapsingHeader("Dummy Objects (HDMY)")) {
                    ImGui::Text("Count: %zu", g_BBMParser.Dummies.size());
                    if (ImGui::BeginTable("BBMDummies", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("Pos", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("Dir", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("Matrix");
                        ImGui::TableHeadersRow();

                        int id = 0;
                        for (const auto& d : g_BBMParser.Dummies) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", id++);
                            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(d.Name.c_str());
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", d.BoneIndex);
                            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f, %.2f, %.2f", d.Position.x, d.Position.y, d.Position.z);
                            ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f, %.2f, %.2f", d.Direction.x, d.Direction.y, d.Direction.z);

                            ImGui::TableSetColumnIndex(5);
                            if (d.UseLocalOrigin) ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOC"); else ImGui::TextDisabled("-");

                            ImGui::TableSetColumnIndex(6);
                            std::string label = "Mat##Dmy" + std::to_string(id);
                            DrawMatrix4x3(d.Transform, label.c_str());
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            // TAB: Geometry
            if (ImGui::BeginTabItem("Geometry")) {
                if (!g_BBMParser.ParsedVertices.empty() && ImGui::TreeNode("Vertex Preview")) {
                    int maxShow = std::min<int>(20, g_BBMParser.ParsedVertices.size());
                    if (ImGui::BeginTable("BBMVerts", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Idx"); ImGui::TableSetupColumn("Position"); ImGui::TableSetupColumn("Normal"); ImGui::TableSetupColumn("UV");
                        ImGui::TableHeadersRow();
                        for (int i = 0; i < maxShow; i++) {
                            const auto& v = g_BBMParser.ParsedVertices[i];
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f, %.2f, %.2f", v.Position.x, v.Position.y, v.Position.z);
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f, %.2f, %.2f", v.Normal.x, v.Normal.y, v.Normal.z);
                            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f, %.2f", v.UV.u, v.UV.v);
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }
                ImGui::EndTabItem();
            }

            // TAB: Volumes
            if (ImGui::BeginTabItem("Volumes")) {
                if (g_BBMParser.Volumes.empty()) ImGui::TextDisabled("No volumes");
                else if (ImGui::BeginTable("BBMVolumes", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("ID"); ImGui::TableSetupColumn("Planes"); ImGui::TableHeadersRow();
                    for (const auto& v : g_BBMParser.Volumes) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%s", v.Name.c_str());
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%u", v.ID);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%u", v.PlaneCount);
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            // TAB: Smoothing
            if (ImGui::BeginTabItem("Smoothing")) {
                if (g_BBMParser.SmoothGroups.FaceGroupMasks.empty()) ImGui::TextDisabled("No smoothing groups");
                else {
                    ImGui::Text("Total Face Masks: %zu", g_BBMParser.SmoothGroups.FaceGroupMasks.size());
                    if (ImGui::BeginTable("BBMSmoothing", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {
                        ImGui::TableSetupColumn("Face Index", ImGuiTableColumnFlags_WidthFixed, 80); ImGui::TableSetupColumn("Group Mask (Hex)"); ImGui::TableHeadersRow();
                        for (size_t i = 0; i < g_BBMParser.SmoothGroups.FaceGroupMasks.size(); i++) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
                            ImGui::TableSetColumnIndex(1); ImGui::Text("0x%08X", g_BBMParser.SmoothGroups.FaceGroupMasks[i]);
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            // TAB: Transform (Root)
            if (ImGui::BeginTabItem("Root Transform")) {
                if (!g_BBMParser.HasTransform) ImGui::TextDisabled("No TRFM chunk found (Using Identity)");
                else {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Last Parsed TRFM Matrix");
                    ImGui::Separator();
                    DrawMatrix4x3(g_BBMParser.LastTransform, "Root Matrix");
                }
                ImGui::EndTabItem();
            }

            // TAB: Debug Log
            if (ImGui::BeginTabItem("Debug Log")) {
                if (ImGui::BeginChild("DebugLog", ImVec2(0, 0), true)) {
                    ImGui::TextUnformatted(g_BBMParser.DebugInfo.c_str());
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        return;
    }

    // [COOKED] CHECK FOR STANDARD MESH
    if (!g_ActiveMeshContent.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Parse Failed: %s", g_ActiveMeshContent.DebugStatus.c_str());
        return;
    }

    ImGui::Separator();
    DrawCookedMeshViewport(g_ActiveMeshContent); // <--- CALL THE VIEWPORT
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "%s", g_ActiveMeshContent.DebugStatus.c_str());
    ImGui::Separator();

    if (ImGui::BeginTabBar("MeshTabs")) {

        // TAB 1: TOC METADATA
        if (ImGui::BeginTabItem("TOC Metadata")) {
            if (g_ActiveMeshContent.EntryMeta.HasData) {
                CMeshEntryMetadata& meta = g_ActiveMeshContent.EntryMeta;
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Header Info");
                ImGui::Text("Physics Index: %d", meta.PhysicsIndex);
                ImGui::Text("Safe Radius: %.3f", meta.SafeBoundingRadius);
                ImGui::Separator();
                ImGui::Text("Sphere: (%.1f, %.1f, %.1f) R: %.1f", meta.BoundingSphereCenter[0], meta.BoundingSphereCenter[1], meta.BoundingSphereCenter[2], meta.BoundingSphereRadius);
                ImGui::Text("Box: Min(%.1f, %.1f, %.1f) Max(%.1f, %.1f, %.1f)", meta.BoundingBoxMin[0], meta.BoundingBoxMin[1], meta.BoundingBoxMin[2], meta.BoundingBoxMax[0], meta.BoundingBoxMax[1], meta.BoundingBoxMax[2]);
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "LOD Configuration (%d Levels)", meta.LODCount);
                if (ImGui::BeginTable("LODTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Level"); ImGui::TableSetupColumn("Data Size"); ImGui::TableSetupColumn("Error Threshold"); ImGui::TableHeadersRow();
                    for (uint32_t i = 0; i < meta.LODCount; i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("LOD %d", i);
                        ImGui::TableSetColumnIndex(1); if (i < meta.LODSizes.size()) ImGui::Text("%d", meta.LODSizes[i]);
                        ImGui::TableSetColumnIndex(2); if (i < meta.LODErrors.size()) ImGui::Text("%.4f", meta.LODErrors[i]); else ImGui::TextDisabled("-");
                    }
                    ImGui::EndTable();
                }
                if (!meta.TextureIDs.empty()) {
                    ImGui::Separator();
                    if (ImGui::TreeNode("Texture References")) {
                        for (size_t i = 0; i < meta.TextureIDs.size(); i++) ImGui::Text("ID: %d", meta.TextureIDs[i]);
                        ImGui::TreePop();
                    }
                }
            }
            else ImGui::TextDisabled("No subheader metadata available.");
            ImGui::EndTabItem();
        }

        // TAB 2: OVERVIEW
        if (ImGui::BeginTabItem("Overview")) {
            ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "MESH: %s", g_ActiveMeshContent.MeshName.c_str());
            ImGui::Separator();
            ImGui::Text("Animated: %s", g_ActiveMeshContent.AnimatedFlag ? "YES" : "NO");
            ImGui::Text("Has Cloth: %s", g_ActiveMeshContent.ClothFlag ? "YES" : "NO");
            ImGui::Separator();
            ImGui::Text("Materials: %d", g_ActiveMeshContent.MaterialCount);
            ImGui::Text("Primitives: %d", g_ActiveMeshContent.PrimitiveCount);
            ImGui::Text("Bones: %d", g_ActiveMeshContent.BoneCount);
            ImGui::Separator();
            ImGui::Text("Total Static Blocks: %d", g_ActiveMeshContent.TotalStaticBlocks);
            ImGui::Text("Total Animated Blocks: %d", g_ActiveMeshContent.TotalAnimatedBlocks);
            ImGui::Separator();
            ImGui::Text("Helper Points: %d", g_ActiveMeshContent.HelperPointCount);
            ImGui::Text("Dummy Objects: %d", g_ActiveMeshContent.DummyObjectCount);
            ImGui::Text("Mesh Volumes: %d", g_ActiveMeshContent.MeshVolumeCount);
            ImGui::Text("Mesh Generators: %d", g_ActiveMeshContent.MeshGeneratorCount);
            ImGui::EndTabItem();
        }

        //BOUNDS EDITOR
        if (ImGui::BeginTabItem("Bounds")) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Global Bounding Box");
            bool changed = false;
            changed |= ImGui::InputFloat3("Box Min", g_ActiveMeshContent.BoundingBoxMin);
            changed |= ImGui::InputFloat3("Box Max", g_ActiveMeshContent.BoundingBoxMax);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Global Bounding Sphere");
            changed |= ImGui::InputFloat3("Center", g_ActiveMeshContent.BoundingSphereCenter);
            changed |= ImGui::InputFloat("Radius", &g_ActiveMeshContent.BoundingSphereRadius);

            ImGui::Separator();
            if (ImGui::Button("Auto-Calculate From Mesh")) {
                g_ActiveMeshContent.AutoCalculateBounds();
                changed = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Recalculates tightest box/sphere from all vertices.");

            if (changed && saveCallback) {
            }
            ImGui::EndTabItem();
        }

        // TAB 4: HELPERS/GEN
        if (ImGui::BeginTabItem("Helpers/Gen")) {
            if (ImGui::CollapsingHeader("Helper Points", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (g_ActiveMeshContent.Helpers.empty()) ImGui::TextDisabled("None");
                else if (ImGui::BeginTable("HelpersTbl", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name/CRC"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Position"); ImGui::TableHeadersRow();
                    for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
                        const auto& h = g_ActiveMeshContent.Helpers[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1); if (i < g_ActiveMeshContent.HelperNameStrings.size()) ImGui::Text("%s", g_ActiveMeshContent.HelperNameStrings[i].c_str()); else ImGui::Text("CRC: %08X", h.NameCRC);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d", h.BoneIndex);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f, %.2f, %.2f", h.Pos[0], h.Pos[1], h.Pos[2]);
                    }
                    ImGui::EndTable();
                }
            }
            if (ImGui::CollapsingHeader("Dummy Objects")) {
                if (g_ActiveMeshContent.Dummies.empty()) ImGui::TextDisabled("None");
                else if (ImGui::BeginTable("DummiesTbl", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name/CRC"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Matrix Preview"); ImGui::TableHeadersRow();
                    for (int i = 0; i < g_ActiveMeshContent.Dummies.size(); i++) {
                        const auto& d = g_ActiveMeshContent.Dummies[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1); std::string n = (i < g_ActiveMeshContent.DummyNameStrings.size()) ? g_ActiveMeshContent.DummyNameStrings[i] : ""; if (!n.empty()) ImGui::Text("%s", n.c_str()); else ImGui::Text("CRC: %08X", d.NameCRC);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d", d.BoneIndex);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("[%.2f, %.2f, %.2f...]", d.Transform[0], d.Transform[1], d.Transform[2]);
                    }
                    ImGui::EndTable();
                }
            }
            if (ImGui::CollapsingHeader("Volumes")) {
                if (g_ActiveMeshContent.Volumes.empty()) ImGui::TextDisabled("None");
                else if (ImGui::BeginTable("VolumesTbl", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Planes"); ImGui::TableHeadersRow();
                    for (int i = 0; i < g_ActiveMeshContent.Volumes.size(); i++) {
                        const auto& v = g_ActiveMeshContent.Volumes[i];
                        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", v.ID); ImGui::TableSetColumnIndex(1); ImGui::Text("%s", v.Name.c_str()); ImGui::TableSetColumnIndex(2); ImGui::Text("%zu planes", v.Planes.size());
                    }
                    ImGui::EndTable();
                }
            }
            if (ImGui::CollapsingHeader("Generators")) {
                if (g_ActiveMeshContent.Generators.empty()) ImGui::TextDisabled("None");
                else if (ImGui::BeginTable("GenTbl", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Bank", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Local", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Matrix"); ImGui::TableHeadersRow();
                    for (int i = 0; i < g_ActiveMeshContent.Generators.size(); i++) {
                        const auto& g = g_ActiveMeshContent.Generators[i];
                        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%s", g.ObjectName.c_str()); ImGui::TableSetColumnIndex(1); ImGui::Text("%d", g.BoneIndex); ImGui::TableSetColumnIndex(2); ImGui::Text("%d", g.BankIndex); ImGui::TableSetColumnIndex(3); ImGui::Text("%d", g.UseLocalOrigin); ImGui::TableSetColumnIndex(4); ImGui::Text("[%.2f, %.2f...]", g.Transform[0], g.Transform[1]);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Bones")) {
            if (g_ActiveMeshContent.BoneCount == 0) {
                ImGui::TextDisabled("No bone data.");
            }
            else {
                ImGui::Text("Total Bones: %d", g_ActiveMeshContent.BoneCount);
                ImGui::Separator();

                if (ImGui::BeginTable("BoneHier", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30);
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Parent");
                    ImGui::TableSetupColumn("Children");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < g_ActiveMeshContent.Bones.size(); i++) {
                        const auto& b = g_ActiveMeshContent.Bones[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);

                        ImGui::TableSetColumnIndex(1);
                        std::string name = (i < g_ActiveMeshContent.BoneNames.size() ? g_ActiveMeshContent.BoneNames[i] : "???");
                        ImGui::Text("%s", name.c_str());

                        if (i < g_ActiveMeshContent.BoneIndices.size()) {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1, 1, 0, 1), "(GID: %d)", g_ActiveMeshContent.BoneIndices[i]);
                        }

                        ImGui::TableSetColumnIndex(2);
                        if (b.ParentIndex == -1) ImGui::TextDisabled("ROOT (-1)");
                        else ImGui::Text("%d", b.ParentIndex);

                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d", b.OriginalNoChildren);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndTabItem();
        }

        // TAB 6: MATERIALS
        if (ImGui::BeginTabItem("Materials")) {
            if (ImGui::BeginTable("MatsAll", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30); ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120); ImGui::TableSetupColumn("Maps (D/B/R/I/Dc)", ImGuiTableColumnFlags_WidthFixed, 100); ImGui::TableSetupColumn("MapFlags", ImGuiTableColumnFlags_WidthFixed, 60); ImGui::TableSetupColumn("Illum", ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableSetupColumn("2Side", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Trans", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Alpha", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Degen", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Raw", ImGuiTableColumnFlags_WidthFixed, 40);
                ImGui::TableHeadersRow();
                for (const auto& m : g_ActiveMeshContent.Materials) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", m.ID);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", m.Name.c_str());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d/%d/%d/%d/%d", m.DiffuseMapID, m.BumpMapID, m.ReflectionMapID, m.IlluminationMapID, m.DecalID);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Diffuse: %d\nBump: %d\nReflect: %d\nIllum: %d\nDecal: %d", m.DiffuseMapID, m.BumpMapID, m.ReflectionMapID, m.IlluminationMapID, m.DecalID);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%08X", m.MapFlags);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%d", m.SelfIllumination);
                    ImGui::TableSetColumnIndex(5); if (m.IsTwoSided) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::TextDisabled("-");
                    ImGui::TableSetColumnIndex(6); if (m.IsTransparent) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::TextDisabled("-");
                    ImGui::TableSetColumnIndex(7); if (m.BooleanAlpha) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::TextDisabled("-");
                    ImGui::TableSetColumnIndex(8); if (m.DegenerateTriangles) ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "YES"); else ImGui::TextDisabled("-");
                    ImGui::TableSetColumnIndex(9); if (m.UseFilenames) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::TextDisabled("-");
                    ImGui::TableSetColumnIndex(10); ImGui::TextDisabled("...");
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // TAB 7: PRIMITIVES
        if (ImGui::BeginTabItem("Primitives")) {
            for (int i = 0; i < g_ActiveMeshContent.Primitives.size(); i++) {
                const auto& p = g_ActiveMeshContent.Primitives[i];
                if (ImGui::TreeNode((void*)(intptr_t)i, "Primitive %d (Mat: %d)", i, p.MaterialIndex)) {

                    if (!p.AnimatedBlocks.empty()) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Animated Blocks (Skinning Palettes)");
                        for (size_t b = 0; b < p.AnimatedBlocks.size(); b++) {
                            const auto& block = p.AnimatedBlocks[b];
                            if (ImGui::TreeNode((void*)(intptr_t)(b + 1000), "Block %d (Verts: %d, Palette Size: %d)", b, block.VertexCount, block.Groups.size())) {
                                std::string palStr = "";
                                for (uint8_t g : block.Groups) palStr += std::to_string(g) + " ";
                                ImGui::TextWrapped("Palette: %s", palStr.c_str());
                                ImGui::TreePop();
                            }
                        }
                        ImGui::Separator();
                    }

                    if (p.VertexBuffer.size() > 0 && ImGui::TreeNode("Vertex Preview")) {
                        int maxV = (p.VertexCount > 10) ? 10 : p.VertexCount;
                        bool isAnim = p.AnimatedBlockCount > 0;
                        int cols = isAnim ? 8 : 6;

                        if (ImGui::BeginTable("VertPreview", cols, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Idx");
                            ImGui::TableSetupColumn("Pos"); ImGui::TableSetupColumn("Norm"); ImGui::TableSetupColumn("UV");
                            if (isAnim) { ImGui::TableSetupColumn("Indices"); ImGui::TableSetupColumn("Weights"); }
                            ImGui::TableHeadersRow();

                            for (int v = 0; v < maxV; v++) {
                                size_t offset = v * p.VertexStride;
                                if (offset + 4 <= p.VertexBuffer.size()) {
                                    float px, py, pz;
                                    if (p.IsCompressed) {
                                        uint32_t packed = *(uint32_t*)(p.VertexBuffer.data() + offset);
                                        UnpackPOSPACKED3(packed, p.Compression.Scale, p.Compression.Offset, px, py, pz);
                                    }
                                    else {
                                        const float* raw = (const float*)(p.VertexBuffer.data() + offset);
                                        px = raw[0]; py = raw[1]; pz = raw[2];
                                    }

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", v);
                                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f, %.2f, %.2f", px, py, pz);

                                    float nx = 0, ny = 1, nz = 0;
                                    size_t normOff = isAnim ? (p.IsCompressed ? 12 : 20) : (p.IsCompressed ? 4 : 12);
                                    if (offset + normOff + 4 <= p.VertexBuffer.size()) {
                                        if (p.IsCompressed) {
                                            uint32_t packed = *(uint32_t*)(p.VertexBuffer.data() + offset + normOff);
                                            UnpackNORMPACKED3(packed, nx, ny, nz);
                                        }
                                        else {
                                            const float* raw = (const float*)(p.VertexBuffer.data() + offset + normOff);
                                            nx = raw[0]; ny = raw[1]; nz = raw[2];
                                        }
                                    }
                                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f, %.2f, %.2f", nx, ny, nz);

                                    float u = 0, v_c = 0;
                                    size_t uvOff = isAnim ? (p.IsCompressed ? 16 : 32) : (p.IsCompressed ? 8 : 24);
                                    if (offset + uvOff + 4 <= p.VertexBuffer.size()) {
                                        if (p.IsCompressed) {
                                            int16_t* uvs = (int16_t*)(p.VertexBuffer.data() + offset + uvOff);
                                            u = DecompressUV(uvs[0]); v_c = DecompressUV(uvs[1]);
                                        }
                                        else {
                                            float* uvs = (float*)(p.VertexBuffer.data() + offset + uvOff);
                                            u = uvs[0]; v_c = uvs[1];
                                        }
                                    }
                                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f, %.2f", u, v_c);

                                    if (isAnim) {
                                        size_t iOff = p.IsCompressed ? 4 : 12;
                                        size_t wOff = iOff + 4;
                                        uint8_t* idx = (uint8_t*)(p.VertexBuffer.data() + offset + iOff);
                                        uint8_t* w = (uint8_t*)(p.VertexBuffer.data() + offset + wOff);
                                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d %d %d %d", idx[0], idx[1], idx[2], idx[3]);
                                        ImGui::TableSetColumnIndex(5); ImGui::Text("%d %d %d %d", w[0], w[1], w[2], w[3]);
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}