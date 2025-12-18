#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "MeshRenderer.h"
#include <functional>
#include <d3d11.h>
#include <map>

extern ID3D11Device* g_pd3dDevice;

static MeshRenderer g_MeshRenderer;
static bool g_ShowWireframe = false;
static bool g_ShowHelpers = false; // Toggle for Helper/Dummy overlay

// Cache to prevent re-parsing the same texture multiple times per session
static std::map<int, ID3D11ShaderResourceView*> g_MeshTextureCache;

// Helper to load a texture by ID strictly from Textures bank
inline ID3D11ShaderResourceView* LoadTextureForMesh(int textureID) {
    if (textureID == -1) return nullptr;
    if (g_MeshTextureCache.count(textureID)) return g_MeshTextureCache[textureID];

    for (auto& bank : g_OpenBanks) {
        // STRICTLY look in Textures bank only (as requested)
        if (bank.Type == EBankType::Textures) {
            for (int i = 0; i < bank.Entries.size(); ++i) {
                if (bank.Entries[i].ID == (uint32_t)textureID) {
                    bank.Stream->clear();
                    bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);

                    size_t fileSize = bank.Entries[i].Size;
                    std::vector<uint8_t> tempData(fileSize + 64);
                    bank.Stream->read((char*)tempData.data(), fileSize);

                    // Use the global parser temporarily
                    g_TextureParser.Parse(bank.SubheaderCache[i], tempData, bank.Entries[i].Type);

                    if (g_TextureParser.IsParsed && !g_TextureParser.DecodedPixels.empty()) {
                        ID3D11ShaderResourceView* srv = nullptr;

                        DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                        uint32_t blockWidth = 1;
                        switch (g_TextureParser.DecodedFormat) {
                        case ETextureFormat::DXT1: dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
                        case ETextureFormat::DXT3: dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
                        case ETextureFormat::DXT5: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
                        case ETextureFormat::NormalMap_DXT1: dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
                        case ETextureFormat::NormalMap_DXT5: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
                        case ETextureFormat::ARGB8888: dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
                        }

                        D3D11_TEXTURE2D_DESC desc = {};
                        desc.Width = g_TextureParser.Header.Width ? g_TextureParser.Header.Width : g_TextureParser.Header.FrameWidth;
                        desc.Height = g_TextureParser.Header.Height ? g_TextureParser.Header.Height : g_TextureParser.Header.FrameHeight;
                        desc.MipLevels = 1; desc.ArraySize = 1;
                        desc.Format = dxFormat; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                        D3D11_SUBRESOURCE_DATA subData = {};
                        subData.pSysMem = g_TextureParser.DecodedPixels.data();
                        if (blockWidth == 4) subData.SysMemPitch = ((desc.Width + 3) / 4) * ((dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16);
                        else subData.SysMemPitch = desc.Width * 4;

                        ID3D11Texture2D* tex = nullptr;
                        if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &subData, &tex))) {
                            g_pd3dDevice->CreateShaderResourceView(tex, nullptr, &srv);
                            tex->Release();
                        }

                        if (srv) {
                            g_MeshTextureCache[textureID] = srv;
                            return srv;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

inline void CheckMeshUpload(ID3D11Device* device) {
    if (g_MeshUploadNeeded) {
        g_MeshRenderer.Initialize(device);

        if (g_BBMParser.IsParsed) {
            g_MeshRenderer.UploadBBM(device, g_BBMParser);
        }
        else if (g_ActiveMeshContent.IsParsed) {
            g_MeshRenderer.UploadMesh(device, g_ActiveMeshContent);

            // Resolve Textures
            std::vector<ID3D11ShaderResourceView*> textures;
            int maxMat = 0;
            for (const auto& m : g_ActiveMeshContent.Materials) if (m.ID > maxMat) maxMat = m.ID;
            textures.resize(maxMat + 1, nullptr);

            for (const auto& m : g_ActiveMeshContent.Materials) {
                if (m.DiffuseMapID > 0) {
                    ID3D11ShaderResourceView* tex = LoadTextureForMesh(m.DiffuseMapID);
                    if (tex) textures[m.ID] = tex;
                }
            }
            g_MeshRenderer.SetMaterialTextures(textures);
        }
        g_MeshUploadNeeded = false;
    }
}

// Helpers for Inspector UI
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
    CheckMeshUpload(g_pd3dDevice);

    // Texture Bank Check
    bool hasTextureBank = false;
    for (const auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures) {
            hasTextureBank = true;
            break;
        }
    }

    if (g_BBMParser.IsParsed || g_ActiveMeshContent.IsParsed) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), g_BBMParser.IsParsed ? "PHYSICS MESH (BBM)" : "STANDARD MESH");

        if (!g_BBMParser.IsParsed && !hasTextureBank) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[!] Textures not loaded.");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open 'textures.big' (GBANK_MAIN_PC) in a new tab.");
        }

        ImGui::Separator();

        // Viewport Layout
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float height = 400.0f;

        g_MeshRenderer.Resize(g_pd3dDevice, avail.x, height);

        ID3D11DeviceContext* ctx;
        g_pd3dDevice->GetImmediateContext(&ctx);
        ID3D11ShaderResourceView* tex = g_MeshRenderer.Render(ctx, avail.x, height, g_ShowWireframe, g_BBMParser.IsParsed);
        ctx->Release();

        // Determine screen coordinates for overlay
        ImVec2 pMin = ImGui::GetCursorScreenPos();
        ImVec2 pMax = ImVec2(pMin.x + avail.x, pMin.y + height);

        if (tex) ImGui::Image((void*)tex, ImVec2(avail.x, height));

        // --- HELPER POINT OVERLAY ---
        if (g_ShowHelpers && g_ActiveMeshContent.IsParsed) {
            ImDrawList* dl = ImGui::GetWindowDrawList();

            // [FIX] CLIP DRAWING TO VIEWPORT RECT
            dl->PushClipRect(pMin, pMax, true);

            ImVec2 mousePos = ImGui::GetMousePos();

            // 1. Helpers (HPNT)
            for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
                const auto& h = g_ActiveMeshContent.Helpers[i];
                ImVec2 scrPos;
                if (g_MeshRenderer.ProjectToScreen(XMFLOAT3(h.Pos[0], h.Pos[1], h.Pos[2]), scrPos, avail.x, height)) {
                    scrPos.x += pMin.x; scrPos.y += pMin.y;

                    // Dot
                    dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(0, 255, 255, 200)); // Cyan

                    // Hover
                    float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                    if (dist < 8.0f) {
                        dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                        std::string name = (i < g_ActiveMeshContent.HelperNameStrings.size()) ? g_ActiveMeshContent.HelperNameStrings[i] : "Helper " + std::to_string(i);
                        ImGui::SetTooltip("%s", name.c_str());
                    }
                }
            }

            // 2. Dummies (HDMY)
            for (int i = 0; i < g_ActiveMeshContent.Dummies.size(); i++) {
                const auto& d = g_ActiveMeshContent.Dummies[i];
                // Fable Dummy transform is [Right, Up, Look, Pos]
                XMFLOAT3 pos = XMFLOAT3(d.Transform[9], d.Transform[10], d.Transform[11]);

                ImVec2 scrPos;
                if (g_MeshRenderer.ProjectToScreen(pos, scrPos, avail.x, height)) {
                    scrPos.x += pMin.x; scrPos.y += pMin.y;

                    // Dot
                    dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(255, 0, 255, 200)); // Magenta

                    // Hover
                    float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                    if (dist < 8.0f) {
                        dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                        std::string name = (i < g_ActiveMeshContent.DummyNameStrings.size()) ? g_ActiveMeshContent.DummyNameStrings[i] : "Dummy " + std::to_string(i);
                        ImGui::SetTooltip("%s", name.c_str());
                    }
                }
            }

            dl->PopClipRect();
        }

        ImGui::Checkbox("Show Wireframe", &g_ShowWireframe);
        ImGui::SameLine();
        ImGui::Checkbox("Show Helpers", &g_ShowHelpers);
        ImGui::Separator();
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Mesh Parsed.");
        if (!g_ActiveMeshContent.DebugStatus.empty()) {
            ImGui::Text("Status: %s", g_ActiveMeshContent.DebugStatus.c_str());
        }
        return;
    }

    // --- INSPECTORS ---
    if (g_BBMParser.IsParsed) {
        if (ImGui::BeginTabBar("BBMTabs")) {
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
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", i);
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(mat.Name.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d", mat.Index);
                        ImGui::TableSetColumnIndex(3); if (mat.TwoSided) ImGui::TextColored(ImVec4(0, 1, 0, 1), "2-Side"); else ImGui::TextDisabled("-");
                        ImGui::TableSetColumnIndex(4); ImVec4 colAmb = UnpackBGRA(mat.Ambient); ImGui::ColorButton(("Amb##" + std::to_string(i)).c_str(), colAmb, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(5); ImVec4 colDif = UnpackBGRA(mat.Diffuse); ImGui::ColorButton(("Dif##" + std::to_string(i)).c_str(), colDif, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(6); ImGui::Text("%.2f / %.2f / %.2f", mat.Shiny, mat.ShinyStrength, mat.Transparency);
                        ImGui::TableSetColumnIndex(7);
                        if (mat.Maps.empty()) ImGui::TextDisabled("None");
                        else {
                            if (ImGui::TreeNode((void*)(intptr_t)i, "%zu Maps", mat.Maps.size())) {
                                for (const auto& map : mat.Maps) {
                                    ImGui::TextColored(ImVec4(0.5f, 1, 1, 1), "[%d] %s", map.Type, map.Filename.c_str());
                                }
                                ImGui::TreePop();
                            }
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bones")) {
                if (ImGui::BeginTable("BonesTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Parent", ImGuiTableColumnFlags_WidthFixed, 50.0f); ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, 100.0f); ImGui::TableSetupColumn("Transform"); ImGui::TableHeadersRow();
                    for (const auto& bone : g_BBMParser.Bones) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", bone.Index);
                        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(bone.Name.c_str());
                        ImGui::TableSetColumnIndex(2); if (bone.ParentIndex == -1) ImGui::TextDisabled("Root"); else ImGui::Text("%d", bone.ParentIndex);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d / %d", bone.FirstChildIndex, bone.NextSiblingIndex);
                        ImGui::TableSetColumnIndex(4); DrawMatrix4x3(bone.LocalTransform, ("Matrix##Bone" + std::to_string(bone.Index)).c_str());
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Helpers")) {
                if (ImGui::BeginTable("BBMHelpers", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("SubMesh", ImGuiTableColumnFlags_WidthFixed, 60.0f); ImGui::TableSetupColumn("Position"); ImGui::TableHeadersRow();
                    int id = 0;
                    for (const auto& h : g_BBMParser.Helpers) {
                        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", id++); ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(h.Name.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d", h.BoneIndex); ImGui::TableSetColumnIndex(3); if (h.SubMeshIndex == -1) ImGui::TextDisabled("-"); else ImGui::Text("%d", h.SubMeshIndex);
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f, %.3f, %.3f", h.Position.x, h.Position.y, h.Position.z);
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    else {
        if (ImGui::BeginTabBar("MeshTabs")) {
            if (ImGui::BeginTabItem("TOC Metadata")) {
                if (g_ActiveMeshContent.EntryMeta.HasData) {
                    CMeshEntryMetadata& meta = g_ActiveMeshContent.EntryMeta;
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Header Info");
                    ImGui::Text("Physics Index: %d", meta.PhysicsIndex); ImGui::Text("Safe Radius: %.3f", meta.SafeBoundingRadius);
                    ImGui::Separator();
                    ImGui::Text("Sphere: (%.1f, %.1f, %.1f) R: %.1f", meta.BoundingSphereCenter[0], meta.BoundingSphereCenter[1], meta.BoundingSphereCenter[2], meta.BoundingSphereRadius);
                    if (ImGui::BeginTable("LODTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Level"); ImGui::TableSetupColumn("Data Size"); ImGui::TableSetupColumn("Error Threshold"); ImGui::TableHeadersRow();
                        for (uint32_t i = 0; i < meta.LODCount; i++) {
                            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("LOD %d", i);
                            ImGui::TableSetColumnIndex(1); if (i < meta.LODSizes.size()) ImGui::Text("%d", meta.LODSizes[i]);
                            ImGui::TableSetColumnIndex(2); if (i < meta.LODErrors.size()) ImGui::Text("%.4f", meta.LODErrors[i]); else ImGui::TextDisabled("-");
                        }
                        ImGui::EndTable();
                    }
                }
                else ImGui::TextDisabled("No subheader metadata available.");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Overview")) {
                ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "MESH: %s", g_ActiveMeshContent.MeshName.c_str());
                ImGui::Separator();
                ImGui::Text("Animated: %s", g_ActiveMeshContent.AnimatedFlag ? "YES" : "NO");
                ImGui::Text("Has Cloth: %s", g_ActiveMeshContent.ClothFlag ? "YES" : "NO");
                ImGui::Separator();
                ImGui::Text("Materials: %d", g_ActiveMeshContent.MaterialCount);
                ImGui::Text("Primitives: %d", g_ActiveMeshContent.PrimitiveCount);
                ImGui::Text("Bones: %d", g_ActiveMeshContent.BoneCount);
                ImGui::EndTabItem();
            }
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
                if (ImGui::Button("Auto-Calculate From Mesh")) { g_ActiveMeshContent.AutoCalculateBounds(); changed = true; }
                if (changed && saveCallback) {}
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Helpers/Gen")) {
                if (ImGui::CollapsingHeader("Helper Points", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTable("HelpersTbl", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                        ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name/CRC"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("Position"); ImGui::TableHeadersRow();
                        for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
                            const auto& h = g_ActiveMeshContent.Helpers[i];
                            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                            ImGui::TableSetColumnIndex(1); if (i < g_ActiveMeshContent.HelperNameStrings.size()) ImGui::Text("%s", g_ActiveMeshContent.HelperNameStrings[i].c_str()); else ImGui::Text("CRC: %08X", h.NameCRC);
                            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", h.BoneIndex); ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f, %.2f, %.2f", h.Pos[0], h.Pos[1], h.Pos[2]);
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Bones")) {
                if (ImGui::BeginTable("BoneHier", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Parent"); ImGui::TableSetupColumn("Children"); ImGui::TableHeadersRow();
                    for (int i = 0; i < g_ActiveMeshContent.Bones.size(); i++) {
                        const auto& b = g_ActiveMeshContent.Bones[i];
                        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                        ImGui::TableSetColumnIndex(1); std::string name = (i < g_ActiveMeshContent.BoneNames.size() ? g_ActiveMeshContent.BoneNames[i] : "???"); ImGui::Text("%s", name.c_str());
                        ImGui::TableSetColumnIndex(2); if (b.ParentIndex == -1) ImGui::TextDisabled("ROOT (-1)"); else ImGui::Text("%d", b.ParentIndex);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d", b.OriginalNoChildren);
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Materials")) {
                if (ImGui::BeginTable("MatsAll", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX)) {
                    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30); ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120); ImGui::TableSetupColumn("Maps (D/B/R/I/Dc)", ImGuiTableColumnFlags_WidthFixed, 100); ImGui::TableSetupColumn("MapFlags", ImGuiTableColumnFlags_WidthFixed, 60); ImGui::TableSetupColumn("Illum", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableSetupColumn("2Side", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Trans", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Alpha", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Degen", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed, 35); ImGui::TableSetupColumn("Raw", ImGuiTableColumnFlags_WidthFixed, 40);
                    ImGui::TableHeadersRow();
                    for (const auto& m : g_ActiveMeshContent.Materials) {
                        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", m.ID); ImGui::TableSetColumnIndex(1); ImGui::Text("%s", m.Name.c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d/%d/%d/%d/%d", m.DiffuseMapID, m.BumpMapID, m.ReflectionMapID, m.IlluminationMapID, m.DecalID);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%08X", m.MapFlags); ImGui::TableSetColumnIndex(4); ImGui::Text("%d", m.SelfIllumination);
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
            ImGui::EndTabBar();
        }
    }
}