#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "MeshRenderer.h"
#include "AnimParser.h"
#include <functional>
#include <d3d11.h>
#include <map>
#include <string>
#include <algorithm>
#include "GltfExporter.h"
#include "FileDialogs.h"

extern ID3D11Device* g_pd3dDevice;

static MeshRenderer g_MeshRenderer;
static bool g_ShowWireframe = false;
static bool g_ShowHelpers = false;
static bool g_ShowBounds = false;
static bool g_ShowRightPanel = true;

// --- DIAGNOSTIC TOGGLES ---
static bool g_ShowSkeleton = true;

static std::map<int, ID3D11ShaderResourceView*> g_MeshTextureCache;

static bool g_TriggerTexPopup = false;
static bool g_ShowTextureSelectPopup = false;
static int g_EditingMaterialIndex = -1;
static int g_EditingTextureType = 0;
static char g_TextureSearchBuf[128] = "";
static int g_SelectedTextureID = -1;

static bool g_PreviewAnimPlaying = false;
static float g_PreviewAnimTime = 0.0f;
static int g_SelectedAnimBankIndex = -1;
static int g_SelectedAnimEntryIndex = -1;
static int g_SelectedAnimType = -1; // Tracks if it is 6, 7, or 9
static AnimParser g_PreviewAnimParser;
static std::vector<XMMATRIX> g_PreviewBoneTransforms;     // Sent to Shader
static std::vector<XMMATRIX> g_PreviewGlobalTransforms;   // Sent to UI Overlay
static char g_AnimSearchBuf[128] = "";

inline std::string GetTextureNameForMesh(int textureID) {
    if (textureID <= 0) return "";
    for (auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
            for (auto& e : bank.Entries) {
                if (e.ID == (uint32_t)textureID) return e.Name;
            }
        }
    }
    return "Unknown Texture";
}

inline ID3D11ShaderResourceView* LoadTextureForMesh(int textureID) {
    if (textureID <= 0) return nullptr;
    if (g_MeshTextureCache.count(textureID)) return g_MeshTextureCache[textureID];

    for (auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
            for (int i = 0; i < bank.Entries.size(); ++i) {
                if (bank.Entries[i].ID == (uint32_t)textureID) {
                    bank.Stream->clear();
                    bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);

                    size_t fileSize = bank.Entries[i].Size;
                    std::vector<uint8_t> tempData(fileSize + 64);
                    bank.Stream->read((char*)tempData.data(), fileSize);

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

        g_PreviewAnimPlaying = false;
        g_PreviewAnimTime = 0.0f;
        g_PreviewBoneTransforms.clear();
        g_PreviewGlobalTransforms.clear();

        if (g_BBMParser.IsParsed) {
            g_MeshRenderer.UploadBBM(device, g_BBMParser);
        }
        else if (g_ActiveMeshContent.IsParsed) {
            g_MeshRenderer.UploadMesh(device, g_ActiveMeshContent);

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

inline void UpdateAnimationBones() {
    if (!g_ActiveMeshContent.IsParsed || g_ActiveMeshContent.BoneCount == 0) {
        g_PreviewBoneTransforms.clear();
        g_PreviewGlobalTransforms.clear();
        return;
    }

    int boneCount = g_ActiveMeshContent.BoneCount;
    g_PreviewBoneTransforms.resize(boneCount);
    g_PreviewGlobalTransforms.resize(boneCount);

    std::vector<XMMATRIX> ibm(boneCount);
    std::vector<XMMATRIX> bindGlobal(boneCount);
    std::vector<XMMATRIX> bindLocal(boneCount);

    // 1. Extract, Clean, and Transpose Inverse Bind Matrices (IBM)
    for (int i = 0; i < boneCount; i++) {
        if ((i + 1) * 64 <= g_ActiveMeshContent.BoneTransformsRaw.size()) {
            float* raw = (float*)(g_ActiveMeshContent.BoneTransformsRaw.data() + i * 64);
            XMMATRIX rawMatrix = XMMATRIX(raw);

            // Clear Fable's garbage 4th row
            rawMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

            // Transpose Fable's Column-Major matrix to DirectX's Row-Major matrix
            XMMATRIX dxIBM = XMMatrixTranspose(rawMatrix);

            ibm[i] = dxIBM;
            bindGlobal[i] = XMMatrixInverse(nullptr, dxIBM);
        }
        else {
            ibm[i] = XMMatrixIdentity();
            bindGlobal[i] = XMMatrixIdentity();
        }
    }

    // Pre-calculate mathematically perfect local bind matrices
    for (int i = 0; i < boneCount; i++) {
        int p = g_ActiveMeshContent.Bones[i].ParentIndex;
        if (p == -1 || p >= boneCount) {
            bindLocal[i] = bindGlobal[i];
        }
        else {
            bindLocal[i] = XMMatrixMultiply(bindGlobal[i], ibm[p]);
        }
    }

    bool isAnimLoaded = g_PreviewAnimParser.Data.IsParsed;

    // 2. PERFECT BIND POSE BYPASS
    if (!isAnimLoaded) {
        for (int i = 0; i < boneCount; i++) {
            g_PreviewBoneTransforms[i] = XMMatrixIdentity();
            g_PreviewGlobalTransforms[i] = bindGlobal[i];
        }
        return;
    }

    std::vector<XMMATRIX> localTransforms(boneCount);

    // 3. Calculate Local Transforms for this frame
    for (int i = 0; i < boneCount; i++) {
        bool hasAnim = false;
        std::string targetBoneName = i < g_ActiveMeshContent.BoneNames.size() ? g_ActiveMeshContent.BoneNames[i] : "";
        std::transform(targetBoneName.begin(), targetBoneName.end(), targetBoneName.begin(), ::tolower);

        // Iterate through all parsed tracks to find a match
        for (const auto& track : g_PreviewAnimParser.Data.Tracks) {
            std::string tName = track.BoneName;
            std::transform(tName.begin(), tName.end(), tName.begin(), ::tolower);

            bool isMatch = false;

            // IRONCLAD PREFIX MATCHER: 
            // Fable track names contain binary garbage at the end because they aren't properly null-terminated.
            // We check if the track name STARTS with the mesh bone name, and ensure the next character is garbage, not a letter!
            if (targetBoneName.length() > 0 && tName.length() >= targetBoneName.length()) {
                if (tName.compare(0, targetBoneName.length(), targetBoneName) == 0) {
                    if (tName.length() == targetBoneName.length()) {
                        isMatch = true; // Exact match
                    }
                    else {
                        // Check the first character after the match. If it's a letter/number, it's a different bone (e.g. "Arm" vs "Armor")
                        char nextChar = tName[targetBoneName.length()];
                        if ((nextChar < 'a' || nextChar > 'z') && (nextChar < '0' || nextChar > '9')) {
                            isMatch = true; // It's binary garbage padding! Safe match.
                        }
                    }
                }
            }

            if (isMatch) {
                if (track.FrameCount > 0 && track.SamplesPerSecond > 0) {
                    int frame = (int)(g_PreviewAnimTime * track.SamplesPerSecond) % track.FrameCount;
                    if (frame < 0) frame += track.FrameCount;

                    Vec3 p;
                    Vec4 q;
                    track.EvaluateFrame(frame, p, q);

                    XMVECTOR vPos = XMVectorSet(p.x, p.y, p.z, 1.0f);
                    XMVECTOR vRot = XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w));

                    // Conjugate Fable Quaternion
                    vRot = XMQuaternionConjugate(vRot);

                    XMMATRIX trackMat = XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vPos);

                    // DELTA ANIMATION (TYPE 7) OFFSET MATH
                    if (g_SelectedAnimType == 7) {
                        localTransforms[i] = XMMatrixMultiply(trackMat, bindLocal[i]);
                    }
                    else {
                        // STANDARD (TYPE 6) AND PARTIAL (TYPE 9)
                        localTransforms[i] = trackMat;
                    }

                    hasAnim = true;
                }
                break; // Found the track, stop searching! (Prevents noodles)
            }
        }

        // If no animation overrides it, use the TRUE local bind transform (crucial for Partial animations!)
        if (!hasAnim) {
            localTransforms[i] = bindLocal[i];
        }
    }

    // 4. Rebuild Global Transforms for the Current Frame
    for (int i = 0; i < boneCount; i++) {
        int p = g_ActiveMeshContent.Bones[i].ParentIndex;
        if (p != -1 && p < i) {
            g_PreviewGlobalTransforms[i] = XMMatrixMultiply(localTransforms[i], g_PreviewGlobalTransforms[p]);
        }
        else {
            g_PreviewGlobalTransforms[i] = localTransforms[i];
        }

        // 5. Final Skinning Matrix: Clean Row-Major IBM * AnimatedGlobal
        g_PreviewBoneTransforms[i] = XMMatrixMultiply(ibm[i], g_PreviewGlobalTransforms[i]);
    }
}

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

    bool hasTextureBank = false;
    for (const auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures) {
            hasTextureBank = true;
            break;
        }
    }

    if (!g_BBMParser.IsParsed && !g_ActiveMeshContent.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Mesh Parsed.");
        if (!g_ActiveMeshContent.DebugStatus.empty()) {
            ImGui::Text("Status: %s", g_ActiveMeshContent.DebugStatus.c_str());
        }
        return;
    }

    // --- ANIMATION UPDATE LOOP ---
    if (g_PreviewAnimPlaying && g_PreviewAnimParser.Data.IsParsed) {
        g_PreviewAnimTime += ImGui::GetIO().DeltaTime;
        float duration = g_PreviewAnimParser.Data.Duration > 0 ? g_PreviewAnimParser.Data.Duration : 1.0f;

        if (g_PreviewAnimTime >= duration) {
            if (g_PreviewAnimParser.Data.IsCyclic) {
                g_PreviewAnimTime = fmod(g_PreviewAnimTime, duration);
            }
            else {
                g_PreviewAnimTime = duration;
                g_PreviewAnimPlaying = false;
            }
        }
    }

    if (g_ActiveMeshContent.BoneCount > 0) {
        UpdateAnimationBones();
    }

    // --- TOP TOOLBAR ---
    if (!g_BBMParser.IsParsed && !hasTextureBank) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[!] Textures not loaded.");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open 'textures.big' (GBANK_MAIN_PC) in a new tab.");
        ImGui::SameLine();
    }

    ImGui::Checkbox("Wireframe", &g_ShowWireframe);
    ImGui::SameLine();
    ImGui::Checkbox("Helpers", &g_ShowHelpers);
    ImGui::SameLine();
    ImGui::Checkbox("Bounds", &g_ShowBounds);

    if (g_ActiveMeshContent.BoneCount > 0) {
        ImGui::SameLine();
        ImGui::Checkbox("Draw Skeleton", &g_ShowSkeleton);
    }

    ImGui::SameLine();
    if (ImGui::Button("Export to glTF")) {
        std::string savePath = SaveFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
        if (!savePath.empty()) {
            if (savePath.length() < 5 || savePath.substr(savePath.length() - 5) != ".gltf") savePath += ".gltf";

            if (g_BBMParser.IsParsed) GltfExporter::ExportBBM(g_BBMParser, savePath);
            else if (g_ActiveMeshContent.IsParsed) GltfExporter::Export(g_ActiveMeshContent, savePath);
        }
    }

    if (g_ActiveMeshContent.IsParsed && g_PreviewAnimPlaying && g_PreviewAnimParser.Data.IsParsed && g_SelectedAnimType == 6) {
        ImGui::SameLine();
        if (ImGui::Button("Export Mesh + Anim (glTF)")) {
            std::string savePath = SaveFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
            if (!savePath.empty()) {
                if (savePath.length() < 5 || savePath.substr(savePath.length() - 5) != ".gltf") savePath += ".gltf";
                // Pass the animation parser to the exporter
                GltfExporter::Export(g_ActiveMeshContent, savePath, &g_PreviewAnimParser);
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Exports the mesh along with the currently playing animation (Standard Type 6 only).");
    }


    ImGui::SameLine();
    float availToolW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availToolW - 30);
    if (ImGui::Button(g_ShowRightPanel ? ">>##RightToggle" : "<<##RightToggle", ImVec2(28, 24))) {
        g_ShowRightPanel = !g_ShowRightPanel;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ShowRightPanel ? "Collapse Info Panel" : "Expand Info Panel");

    ImGui::Separator();

    static float rightPanelWidth = 450.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float splitterWidth = 4.0f;

    float viewportWidth = avail.x;
    if (g_ShowRightPanel) {
        viewportWidth = avail.x - rightPanelWidth - splitterWidth;
    }

    // ====================================================
    // LEFT PANEL: 3D VIEWPORT
    // ====================================================
    ImGui::BeginChild("MeshViewportChild", ImVec2(viewportWidth, avail.y), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    g_MeshRenderer.Resize(g_pd3dDevice, viewportWidth, avail.y);

    ID3D11DeviceContext* ctx;
    g_pd3dDevice->GetImmediateContext(&ctx);

    ID3D11ShaderResourceView* tex = g_MeshRenderer.Render(ctx, viewportWidth, avail.y, g_ShowWireframe, g_BBMParser.IsParsed, &g_PreviewBoneTransforms);

    if (g_ShowBounds && g_ActiveMeshContent.IsParsed) {
        g_MeshRenderer.RenderBounds(ctx, viewportWidth, avail.y,
            g_ActiveMeshContent.BoundingBoxMin,
            g_ActiveMeshContent.BoundingBoxMax,
            g_ActiveMeshContent.BoundingSphereCenter,
            g_ActiveMeshContent.BoundingSphereRadius);
    }

    ctx->Release();

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImVec2 pMax = ImVec2(pMin.x + viewportWidth, pMin.y + avail.y);

    if (tex) ImGui::Image((void*)tex, ImVec2(viewportWidth, avail.y));

    // Overlays (Helpers / Dummies / SKELETON)
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(pMin, pMax, true);

    if (g_ShowSkeleton && !g_PreviewGlobalTransforms.empty()) {
        for (int i = 0; i < g_ActiveMeshContent.BoneCount; i++) {
            XMMATRIX globalMat = g_PreviewGlobalTransforms[i];
            XMFLOAT3 pos(XMVectorGetX(globalMat.r[3]), XMVectorGetY(globalMat.r[3]), XMVectorGetZ(globalMat.r[3]));

            ImVec2 scrPos;
            if (g_MeshRenderer.ProjectToScreen(pos, scrPos, viewportWidth, avail.y)) {
                scrPos.x += pMin.x; scrPos.y += pMin.y;
                dl->AddCircleFilled(scrPos, 3.0f, IM_COL32(0, 255, 0, 255)); // Green joint

                int parentIdx = g_ActiveMeshContent.Bones[i].ParentIndex;
                if (parentIdx != -1 && parentIdx < g_ActiveMeshContent.BoneCount) {
                    XMMATRIX pMat = g_PreviewGlobalTransforms[parentIdx];
                    XMFLOAT3 pPos(XMVectorGetX(pMat.r[3]), XMVectorGetY(pMat.r[3]), XMVectorGetZ(pMat.r[3]));
                    ImVec2 pScrPos;
                    if (g_MeshRenderer.ProjectToScreen(pPos, pScrPos, viewportWidth, avail.y)) {
                        pScrPos.x += pMin.x; pScrPos.y += pMin.y;
                        dl->AddLine(scrPos, pScrPos, IM_COL32(255, 255, 0, 255), 2.0f); // Yellow bone
                    }
                }
            }
        }
    }

    if (g_ShowHelpers && g_ActiveMeshContent.IsParsed) {
        ImVec2 mousePos = ImGui::GetMousePos();

        for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
            const auto& h = g_ActiveMeshContent.Helpers[i];
            ImVec2 scrPos;
            if (g_MeshRenderer.ProjectToScreen(XMFLOAT3(h.Pos[0], h.Pos[1], h.Pos[2]), scrPos, viewportWidth, avail.y)) {
                scrPos.x += pMin.x; scrPos.y += pMin.y;

                dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(0, 255, 255, 200));

                float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                if (dist < 8.0f) {
                    dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                    std::string name = (i < g_ActiveMeshContent.HelperNameStrings.size()) ? g_ActiveMeshContent.HelperNameStrings[i] : "Helper " + std::to_string(i);
                    ImGui::SetTooltip("%s", name.c_str());
                }
            }
        }

        for (int i = 0; i < g_ActiveMeshContent.Dummies.size(); i++) {
            const auto& d = g_ActiveMeshContent.Dummies[i];
            XMFLOAT3 pos = XMFLOAT3(d.Transform[9], d.Transform[10], d.Transform[11]);

            ImVec2 scrPos;
            if (g_MeshRenderer.ProjectToScreen(pos, scrPos, viewportWidth, avail.y)) {
                scrPos.x += pMin.x; scrPos.y += pMin.y;

                dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(255, 0, 255, 200));

                float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                if (dist < 8.0f) {
                    dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                    std::string name = (i < g_ActiveMeshContent.DummyNameStrings.size()) ? g_ActiveMeshContent.DummyNameStrings[i] : "Dummy " + std::to_string(i);
                    ImGui::SetTooltip("%s", name.c_str());
                }
            }
        }
    }
    dl->PopClipRect();
    ImGui::EndChild();

    // ====================================================
    // RIGHT PANEL: TABS & DATA
    // ====================================================
    if (g_ShowRightPanel) {
        ImGui::SameLine(0, 0);

        ImGui::InvisibleButton("vsplitter", ImVec2(splitterWidth, avail.y));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive()) {
            rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
            if (rightPanelWidth < 250.0f) rightPanelWidth = 250.0f;
            if (rightPanelWidth > avail.x - 200.0f) rightPanelWidth = avail.x - 200.0f;
        }

        ImGui::SameLine(0, 0);

        ImGui::BeginChild("MeshInfoRightPanel", ImVec2(rightPanelWidth, avail.y), false);

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
                    auto PackBGRA = [](const ImVec4& c) -> uint32_t {
                        uint32_t r = (uint32_t)(std::clamp(c.x, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t g = (uint32_t)(std::clamp(c.y, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t b = (uint32_t)(std::clamp(c.z, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t a = (uint32_t)(std::clamp(c.w, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        return (a << 24) | (r << 16) | (g << 8) | b;
                        };

                    auto DrawBBMTexRow = [&](const char* label, int& mapID, int matIdx, int type) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);
                        ImGui::SameLine(130);

                        ImGui::SetNextItemWidth(80);
                        std::string idStr = "##bbm_id" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::InputInt(idStr.c_str(), &mapID, 0, 0)) {
                            g_MeshUploadNeeded = true;
                            if (saveCallback) saveCallback();
                        }

                        ImGui::SameLine();
                        std::string btnStr = "+##bbm_btn" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::Button(btnStr.c_str(), ImVec2(24, 0))) {
                            g_EditingMaterialIndex = matIdx;
                            g_EditingTextureType = type;
                            g_SelectedTextureID = mapID;
                            g_TextureSearchBuf[0] = '\0';
                            g_TriggerTexPopup = true;
                        }

                        if (mapID > 0) {
                            ImGui::SameLine();
                            ID3D11ShaderResourceView* srv = LoadTextureForMesh(mapID);
                            if (srv) {
                                ImGui::Image((void*)srv, ImVec2(24, 24));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::BeginTooltip();
                                    ImGui::Image((void*)srv, ImVec2(256, 256));
                                    ImGui::PushTextWrapPos(256.0f);
                                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", GetTextureNameForMesh(mapID).c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::EndTooltip();
                                }
                            }
                        }
                        };

                    ImGui::BeginChild("BBMMatList", ImVec2(0, 0), true);
                    for (int i = 0; i < g_BBMParser.ParsedMaterials.size(); i++) {
                        auto& m = g_BBMParser.ParsedMaterials[i];
                        if (ImGui::CollapsingHeader(("Material " + std::to_string(m.Index) + " - " + m.Name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::PushID(i);
                            ImGui::AlignTextToFramePadding(); ImGui::Text("Name"); ImGui::SameLine(130);
                            char nameBuf[128]; strncpy_s(nameBuf, m.Name.c_str(), 127); ImGui::SetNextItemWidth(200);
                            if (ImGui::InputText("##Name", nameBuf, 128)) m.Name = nameBuf;

                            DrawBBMTexRow("Diffuse", m.DiffuseBank, i, 0);
                            DrawBBMTexRow("Bump", m.BumpBank, i, 1);
                            DrawBBMTexRow("Specular", m.ReflectBank, i, 2);
                            DrawBBMTexRow("Illumination", m.IllumBank, i, 3);

                            ImGui::Separator();

                            ImGui::AlignTextToFramePadding(); ImGui::Text("Ambient Color"); ImGui::SameLine(130);
                            ImVec4 cAmb = UnpackBGRA(m.Ambient);
                            if (ImGui::ColorEdit4("##Amb", (float*)&cAmb, ImGuiColorEditFlags_NoInputs)) { m.Ambient = PackBGRA(cAmb); if (saveCallback) saveCallback(); }

                            ImGui::AlignTextToFramePadding(); ImGui::Text("Diffuse Color"); ImGui::SameLine(130);
                            ImVec4 cDif = UnpackBGRA(m.Diffuse);
                            if (ImGui::ColorEdit4("##Dif", (float*)&cDif, ImGuiColorEditFlags_NoInputs)) { m.Diffuse = PackBGRA(cDif); if (saveCallback) saveCallback(); }

                            ImGui::AlignTextToFramePadding(); ImGui::Text("Shiny"); ImGui::SameLine(130); ImGui::SetNextItemWidth(120);
                            if (ImGui::DragFloat("##Shi", &m.Shiny, 0.01f)) { if (saveCallback) saveCallback(); }
                            ImGui::AlignTextToFramePadding(); ImGui::Text("Shiny Str"); ImGui::SameLine(130); ImGui::SetNextItemWidth(120);
                            if (ImGui::DragFloat("##ShiStr", &m.ShinyStrength, 0.01f)) { if (saveCallback) saveCallback(); }
                            ImGui::AlignTextToFramePadding(); ImGui::Text("Transparency"); ImGui::SameLine(130); ImGui::SetNextItemWidth(120);
                            if (ImGui::DragFloat("##Trans", &m.Transparency, 0.01f)) { if (saveCallback) saveCallback(); }

                            ImGui::Dummy(ImVec2(0, 5));
                            ImGui::Checkbox("Two-Sided", &m.TwoSided); ImGui::Checkbox("Transparent", &m.Transparent); ImGui::Checkbox("Boolean Alpha", &m.BooleanAlpha); ImGui::Checkbox("Degenerate Tris", &m.DegenerateTriangles);

                            ImGui::PopID();
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Bones")) {
                    if (ImGui::BeginTable("BonesTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Parent", ImGuiTableColumnFlags_WidthFixed, 50.0f); ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, 100.0f); ImGui::TableSetupColumn("Transform"); ImGui::TableHeadersRow();
                        for (const auto& bone : g_BBMParser.Bones) {
                            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", bone.Index); ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(bone.Name.c_str()); ImGui::TableSetColumnIndex(2); if (bone.ParentIndex == -1) ImGui::TextDisabled("Root"); else ImGui::Text("%d", bone.ParentIndex); ImGui::TableSetColumnIndex(3); ImGui::Text("%d / %d", bone.FirstChildIndex, bone.NextSiblingIndex); ImGui::TableSetColumnIndex(4); DrawMatrix4x3(bone.LocalTransform, ("Matrix##Bone" + std::to_string(bone.Index)).c_str());
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Helpers")) {
                    if (ImGui::BeginTable("BBMHelpers", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f); ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Bone", ImGuiTableColumnFlags_WidthFixed, 40.0f); ImGui::TableSetupColumn("SubMesh", ImGuiTableColumnFlags_WidthFixed, 60.0f); ImGui::TableSetupColumn("Position"); ImGui::TableHeadersRow();
                        int id = 0;
                        for (const auto& h : g_BBMParser.Helpers) {
                            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", id++); ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(h.Name.c_str()); ImGui::TableSetColumnIndex(2); ImGui::Text("%d", h.BoneIndex); ImGui::TableSetColumnIndex(3); if (h.SubMeshIndex == -1) ImGui::TextDisabled("-"); else ImGui::Text("%d", h.SubMeshIndex); ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f, %.3f, %.3f", h.Position.x, h.Position.y, h.Position.z);
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
                if (ImGui::BeginTabItem("Overview")) {
                    ImGui::Text("Materials: %d", g_ActiveMeshContent.MaterialCount);
                    ImGui::Text("Primitives: %d", g_ActiveMeshContent.PrimitiveCount);
                    ImGui::Text("Bones: %d", g_ActiveMeshContent.BoneCount);

                    ImGui::Dummy(ImVec2(0, 5));
                    if (ImGui::BeginTable("PrimTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 30); ImGui::TableSetupColumn("MatIdx"); ImGui::TableSetupColumn("Verts"); ImGui::TableSetupColumn("Tris"); ImGui::TableHeadersRow();
                        for (int p = 0; p < g_ActiveMeshContent.Primitives.size(); p++) {
                            ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("%d", p); ImGui::TableSetColumnIndex(1); ImGui::Text("%d", g_ActiveMeshContent.Primitives[p].MaterialIndex); ImGui::TableSetColumnIndex(2); ImGui::Text("%d", g_ActiveMeshContent.Primitives[p].VertexCount); ImGui::TableSetColumnIndex(3); ImGui::Text("%d", g_ActiveMeshContent.Primitives[p].TriangleCount);
                        }
                        ImGui::EndTable();
                    }

                    if (g_ActiveMeshContent.EntryMeta.HasData) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "TOC Metadata");
                        ImGui::Text("Physics Index: %d", g_ActiveMeshContent.EntryMeta.PhysicsIndex);
                        ImGui::Text("Safe Radius: %.3f", g_ActiveMeshContent.EntryMeta.SafeBoundingRadius);
                        if (g_ActiveMeshContent.EntryMeta.LODCount > 0) {
                            ImGui::Dummy(ImVec2(0, 5));
                            if (ImGui::BeginTable("LODTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                ImGui::TableSetupColumn("Level"); ImGui::TableSetupColumn("Size"); ImGui::TableSetupColumn("Error"); ImGui::TableHeadersRow();
                                for (uint32_t i = 0; i < g_ActiveMeshContent.EntryMeta.LODCount; i++) {
                                    ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("LOD %d", i); ImGui::TableSetColumnIndex(1); if (i < g_ActiveMeshContent.EntryMeta.LODSizes.size()) ImGui::Text("%d", g_ActiveMeshContent.EntryMeta.LODSizes[i]); ImGui::TableSetColumnIndex(2); if (i < g_ActiveMeshContent.EntryMeta.LODErrors.size()) ImGui::Text("%.4f", g_ActiveMeshContent.EntryMeta.LODErrors[i]); else ImGui::TextDisabled("-");
                                }
                                ImGui::EndTable();
                            }
                        }
                    }

                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Bounding Box");
                    bool changed = false;
                    changed |= ImGui::InputFloat3("Min", g_ActiveMeshContent.BoundingBoxMin);
                    changed |= ImGui::InputFloat3("Max", g_ActiveMeshContent.BoundingBoxMax);

                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Bounding Sphere");
                    changed |= ImGui::InputFloat3("Center", g_ActiveMeshContent.BoundingSphereCenter);
                    changed |= ImGui::InputFloat("Radius", &g_ActiveMeshContent.BoundingSphereRadius);

                    ImGui::Dummy(ImVec2(0, 5));
                    if (ImGui::Button("Auto-Calculate Bounds From Mesh", ImVec2(-1, 0))) {
                        g_ActiveMeshContent.AutoCalculateBounds();
                        changed = true;
                    }
                    if (changed && saveCallback) saveCallback();

                    ImGui::EndTabItem();
                }

                // --- ANIMATIONS TAB ---
                if (g_ActiveMeshContent.AnimatedFlag && ImGui::BeginTabItem("Animations")) {
                    struct FoundAnim { int BankIdx; int EntryIdx; std::string Name; uint32_t ID; int Type; };
                    std::vector<FoundAnim> anims;

                    for (int i = 0; i < g_OpenBanks.size(); i++) {
                        if (g_OpenBanks[i].Type == EBankType::Graphics) {
                            for (int j = 0; j < g_OpenBanks[i].Entries.size(); j++) {
                                int t = g_OpenBanks[i].Entries[j].Type;
                                if (t == 6 || t == 7 || t == 9) { // Include Standard, Delta, and Partial
                                    anims.push_back({ i, j, g_OpenBanks[i].Entries[j].FriendlyName, g_OpenBanks[i].Entries[j].ID, t });
                                }
                            }
                        }
                    }

                    if (g_PreviewAnimParser.Data.IsParsed) {
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded: %s", anims.empty() ? "Anim" : g_OpenBanks[g_SelectedAnimBankIndex].Entries[g_SelectedAnimEntryIndex].FriendlyName.c_str());
                        float duration = g_PreviewAnimParser.Data.Duration > 0 ? g_PreviewAnimParser.Data.Duration : 1.0f;

                        if (ImGui::Button(g_PreviewAnimPlaying ? "Pause" : "Play", ImVec2(80, 0))) g_PreviewAnimPlaying = !g_PreviewAnimPlaying;
                        ImGui::SameLine();
                        if (ImGui::Button("Stop", ImVec2(80, 0))) {
                            g_PreviewAnimPlaying = false; g_PreviewAnimTime = 0.0f;
                            g_PreviewAnimParser.Data.IsParsed = false; g_SelectedAnimBankIndex = -1; g_SelectedAnimEntryIndex = -1; g_SelectedAnimType = -1;
                        }

                        if (ImGui::SliderFloat("Time", &g_PreviewAnimTime, 0.0f, duration, "%.2f s")) g_PreviewAnimPlaying = false;
                        ImGui::Separator();
                    }
                    else {
                        ImGui::TextDisabled("Select an animation below to load it.");
                        ImGui::Separator();
                    }

                    ImGui::InputTextWithHint("##AnimSearch", "Search Animations...", g_AnimSearchBuf, 128);
                    ImGui::BeginChild("AnimList", ImVec2(0, 0), true);

                    std::string filter = g_AnimSearchBuf;
                    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                    for (const auto& a : anims) {
                        std::string lowerName = a.Name;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                        if (filter.empty() || lowerName.find(filter) != std::string::npos) {
                            bool isSelected = (g_SelectedAnimBankIndex == a.BankIdx && g_SelectedAnimEntryIndex == a.EntryIdx);

                            std::string typeLabel = "";
                            if (a.Type == 7) typeLabel = " [Delta]";
                            else if (a.Type == 9) typeLabel = " [Partial]";
                            std::string displayStr = std::to_string(a.ID) + " - " + a.Name + typeLabel;

                            if (ImGui::Selectable(displayStr.c_str(), isSelected)) {
                                g_SelectedAnimBankIndex = a.BankIdx;
                                g_SelectedAnimEntryIndex = a.EntryIdx;
                                g_SelectedAnimType = a.Type;

                                auto& b = g_OpenBanks[a.BankIdx];
                                std::vector<uint8_t> rawData;
                                if (b.ModifiedEntryData.count(a.EntryIdx)) rawData = b.ModifiedEntryData[a.EntryIdx];
                                else {
                                    b.Stream->clear();
                                    b.Stream->seekg(b.Entries[a.EntryIdx].Offset, std::ios::beg);
                                    rawData.resize(b.Entries[a.EntryIdx].Size);
                                    b.Stream->read((char*)rawData.data(), b.Entries[a.EntryIdx].Size);
                                }

                                g_PreviewAnimParser.Parse(rawData);
                                g_PreviewAnimTime = 0.0f;
                                g_PreviewAnimPlaying = true;
                            }
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Helpers/Gen")) {
                    if (ImGui::CollapsingHeader("Helper Points", ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (ImGui::BeginTable("HelpersTbl", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
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
                    if (ImGui::BeginTable("BoneHier", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
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
                    auto DrawTexRow = [&](const char* label, int& mapID, int matIdx, int type, auto& matObj) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);
                        ImGui::SameLine(130);

                        ImGui::SetNextItemWidth(80);
                        std::string idStr = "##id" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::InputInt(idStr.c_str(), &mapID, 0, 0)) {
                            int flags = 0;
                            if (matObj.DiffuseMapID > 0) flags |= 1;
                            if (matObj.BumpMapID > 0) flags |= 2;
                            if (matObj.ReflectionMapID > 0) flags |= 4;
                            if (matObj.IlluminationMapID > 0) flags |= 8;
                            matObj.MapFlags = (matObj.MapFlags & ~0xF) | flags;

                            g_MeshUploadNeeded = true;
                            if (saveCallback) saveCallback();
                        }

                        ImGui::SameLine();
                        std::string btnStr = "+##btn" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::Button(btnStr.c_str(), ImVec2(24, 0))) {
                            g_EditingMaterialIndex = matIdx;
                            g_EditingTextureType = type;
                            g_SelectedTextureID = mapID;
                            g_TextureSearchBuf[0] = '\0';
                            g_TriggerTexPopup = true;
                        }

                        if (mapID > 0) {
                            ImGui::SameLine();
                            ID3D11ShaderResourceView* srv = LoadTextureForMesh(mapID);
                            if (srv) {
                                ImGui::Image((void*)srv, ImVec2(24, 24));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::BeginTooltip();
                                    ImGui::Image((void*)srv, ImVec2(256, 256));
                                    ImGui::PushTextWrapPos(256.0f);
                                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", GetTextureNameForMesh(mapID).c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::EndTooltip();
                                }
                            }
                        }
                        };

                    ImGui::BeginChild("MatList", ImVec2(0, 0), true);
                    for (int i = 0; i < g_ActiveMeshContent.Materials.size(); i++) {
                        auto& m = g_ActiveMeshContent.Materials[i];

                        if (ImGui::CollapsingHeader(("Material " + std::to_string(m.ID) + " - " + m.Name).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::PushID(i);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Name");
                            ImGui::SameLine(130);
                            char nameBuf[128]; strncpy_s(nameBuf, m.Name.c_str(), 127);
                            ImGui::SetNextItemWidth(200);
                            if (ImGui::InputText("##Name", nameBuf, 128)) m.Name = nameBuf;

                            DrawTexRow("Diffuse", m.DiffuseMapID, i, 0, m);
                            DrawTexRow("Bump", m.BumpMapID, i, 1, m);
                            DrawTexRow("Specular", m.ReflectionMapID, i, 2, m);
                            DrawTexRow("Illumination", m.IlluminationMapID, i, 3, m);
                            DrawTexRow("Decals", m.DecalID, i, 4, m);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Self Illum");
                            ImGui::SameLine(130);
                            ImGui::SetNextItemWidth(120);
                            if (ImGui::SliderInt("##si_slider", &m.SelfIllumination, 0, 255)) {
                                if (saveCallback) saveCallback();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("-##si_sub")) { m.SelfIllumination = (std::max)(0, m.SelfIllumination - 1); if (saveCallback) saveCallback(); }
                            ImGui::SameLine();
                            if (ImGui::Button("+##si_add")) { m.SelfIllumination = (std::min)(255, m.SelfIllumination + 1); if (saveCallback) saveCallback(); }

                            ImGui::Checkbox("Two-Sided", &m.IsTwoSided);
                            ImGui::Checkbox("Transparent", &m.IsTransparent);
                            ImGui::Checkbox("Boolean Alpha", &m.BooleanAlpha);
                            ImGui::Checkbox("Degenerate Triangles", &m.DegenerateTriangles);
                            ImGui::Checkbox("Use Filenames", &m.UseFilenames);

                            ImGui::PopID();
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
    }

    // ==========================================
    // TEXTURE PICKER MODAL (Global Scope)
    // ==========================================
    if (g_TriggerTexPopup) {
        ImGui::OpenPopup("Select Texture");
        g_ShowTextureSelectPopup = true;
        g_TriggerTexPopup = false;
    }

    if (ImGui::BeginPopupModal("Select Texture", &g_ShowTextureSelectPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##Search", "Search Textures by ID or Name...", g_TextureSearchBuf, 128);
        ImGui::Separator();

        ImGui::BeginChild("TexList", ImVec2(350, 300), true);
        LoadedBank* texBank = nullptr;
        for (auto& b : g_OpenBanks) {
            if (b.Type == EBankType::Textures || b.Type == EBankType::Frontend || b.Type == EBankType::Effects) {
                texBank = &b; break;
            }
        }

        if (texBank) {
            std::string filter = g_TextureSearchBuf;
            std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

            for (const auto& entry : texBank->Entries) {
                std::string nameLower = entry.Name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                std::string idStr = std::to_string(entry.ID);

                if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                    bool isSelected = (g_SelectedTextureID == entry.ID);
                    if (ImGui::Selectable((idStr + " - " + entry.Name).c_str(), isSelected)) {
                        g_SelectedTextureID = entry.ID;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
            }
        }
        else {
            ImGui::TextDisabled("No Texture Bank open!\nPlease open 'textures.big' in another tab.");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("TexPreview", ImVec2(256, 300), true);
        if (g_SelectedTextureID > 0) {
            ID3D11ShaderResourceView* srv = LoadTextureForMesh(g_SelectedTextureID);
            if (srv) {
                ImGui::Image((void*)srv, ImVec2(256, 256));
            }
            else {
                ImGui::TextDisabled("Preview Not Available\n(Or Format Unsupported)");
            }
        }
        else {
            ImGui::TextDisabled("No Texture Selected");
        }
        ImGui::EndChild();

        ImGui::Separator();

        if (ImGui::Button("Choose", ImVec2(120, 0))) {
            if (g_EditingMaterialIndex != -1 && g_SelectedTextureID >= 0) {
                if (g_BBMParser.IsParsed) {
                    auto& mat = g_BBMParser.ParsedMaterials[g_EditingMaterialIndex];
                    switch (g_EditingTextureType) {
                    case 0: mat.DiffuseBank = g_SelectedTextureID; break;
                    case 1: mat.BumpBank = g_SelectedTextureID; break;
                    case 2: mat.ReflectBank = g_SelectedTextureID; break;
                    case 3: mat.IllumBank = g_SelectedTextureID; break;
                    }
                }
                else {
                    auto& mat = g_ActiveMeshContent.Materials[g_EditingMaterialIndex];
                    switch (g_EditingTextureType) {
                    case 0: mat.DiffuseMapID = g_SelectedTextureID; break;
                    case 1: mat.BumpMapID = g_SelectedTextureID; break;
                    case 2: mat.ReflectionMapID = g_SelectedTextureID; break;
                    case 3: mat.IlluminationMapID = g_SelectedTextureID; break;
                    case 4: mat.DecalID = g_SelectedTextureID; break;
                    }
                    int flags = 0;
                    if (mat.DiffuseMapID > 0) flags |= 1;
                    if (mat.BumpMapID > 0) flags |= 2;
                    if (mat.ReflectionMapID > 0) flags |= 4;
                    if (mat.IlluminationMapID > 0) flags |= 8;
                    mat.MapFlags = (mat.MapFlags & ~0xF) | flags;
                }

                g_MeshUploadNeeded = true;
                if (saveCallback) saveCallback();
            }
            g_ShowTextureSelectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowTextureSelectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}