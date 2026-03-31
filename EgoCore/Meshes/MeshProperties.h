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
#include "MeshCompiler.h"
#include "TextureExporter.h"
#include "GltfMeshImporter.h"
#include "BigBankCompiler.h"
#include "TextureBuilder.h"

extern ID3D11Device* g_pd3dDevice;

static MeshRenderer g_MeshRenderer;
static bool g_ShowWireframe = false;
static bool g_ShowHelpers = false;
static bool g_ShowBounds = false;
static bool g_ShowRightPanel = true;
static bool g_ShowSkeleton = false;
static MeshRenderer g_PhysicsOverlayRenderer;
static CBBMParser g_OverlayBBMParser;
static bool g_ShowPhysicsOverlay = false;
static int g_LoadedOverlayID = -1;
static bool g_TriggerPhysicsPopup = false;
static bool g_ShowPhysicsSelectPopup = false;
static char g_PhysicsSearchBuf[128] = "";
static int g_SelectedPhysicsID = -1;
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
static int g_SelectedAnimType = -1;
static AnimParser g_PreviewAnimParser;
static std::vector<XMMATRIX> g_PreviewBoneTransforms;
static std::vector<XMMATRIX> g_PreviewGlobalTransforms;
static char g_AnimSearchBuf[128] = "";
static bool g_TriggerBonePopup = false;
static bool g_ShowBoneSelectPopup = false;
static int g_EditingTargetType = -1;
static int g_EditingTargetIndex = -1;
static char g_BoneSearchBuf[128] = "";
static bool g_PreserveCamera = false;
static bool g_ShowCloth = false;

struct VolumeBoxState {
    bool IsBoxMode = false;
    float Center[3] = { 0.0f, 0.0f, 0.0f };
    float Size[3] = { 2.0f, 2.0f, 2.0f };
    float Rotation[3] = { 0.0f, 0.0f, 0.0f };
};

static std::map<std::string, VolumeBoxState> g_VolumeStates;

// --- XBOX TEXTURE CACHING SYSTEM ---
struct XboxTexMeta {
    std::string Name;
    uint32_t Type;
    uint32_t Offset;
    uint32_t Size;
    std::vector<uint8_t> Info;
};
static std::map<int, std::map<uint32_t, XboxTexMeta>> g_XboxTexCache;

inline void EnsureXboxTextureCache(int bankIndex) {
    if (g_XboxTexCache.count(bankIndex)) return;
    LoadedBank& bank = g_OpenBanks[bankIndex];

    auto pos = bank.Stream->tellg(); // Save current file position
    for (const auto& sb : bank.SubBanks) {
        if (StartsWith(sb.Name, "GBANK")) {
            bank.Stream->clear();
            bank.Stream->seekg(sb.Offset, std::ios::beg);

            uint32_t statsCount = 0;
            bank.Stream->read((char*)&statsCount, 4);
            if (statsCount < 1000) bank.Stream->seekg(statsCount * 8, std::ios::cur);
            else bank.Stream->seekg(-4, std::ios::cur);

            for (uint32_t i = 0; i < sb.EntryCount; i++) {
                uint32_t magicE, id, type, size, offset, crc, timestamp, depCount, infoSize;
                bank.Stream->read((char*)&magicE, 4);
                bank.Stream->read((char*)&id, 4);
                bank.Stream->read((char*)&type, 4);
                bank.Stream->read((char*)&size, 4);
                bank.Stream->read((char*)&offset, 4);
                bank.Stream->read((char*)&crc, 4);

                std::string name = ReadBankString(*bank.Stream);

                bank.Stream->read((char*)&timestamp, 4);
                bank.Stream->read((char*)&depCount, 4);
                for (uint32_t d = 0; d < depCount; d++) ReadBankString(*bank.Stream);

                bank.Stream->read((char*)&infoSize, 4);
                std::vector<uint8_t> infoBuf(infoSize);
                if (infoSize > 0) bank.Stream->read((char*)infoBuf.data(), infoSize);

                if (magicE == 42 || (size > 0 && id > 0)) {
                    g_XboxTexCache[bankIndex][id] = { name, type, offset, size, infoBuf };
                }
            }
        }
    }
    bank.Stream->clear();
    bank.Stream->seekg(pos, std::ios::beg); // Restore original file position
}
// -----------------------------------

inline std::string GetTextureNameForMesh(int textureID) {
    if (textureID <= 0) return "";

    // 1. Check local Xbox bank first
    if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics) {
        EnsureXboxTextureCache(g_ActiveBankIndex);
        if (g_XboxTexCache[g_ActiveBankIndex].count(textureID)) {
            return g_XboxTexCache[g_ActiveBankIndex][textureID].Name;
        }
    }

    // 2. Fallback to global PC texture banks
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

    // 1. Process local Xbox bank first
    if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics) {
        EnsureXboxTextureCache(g_ActiveBankIndex);
        auto& cache = g_XboxTexCache[g_ActiveBankIndex];

        if (cache.count(textureID)) {
            auto& meta = cache[textureID];
            LoadedBank& bank = g_OpenBanks[g_ActiveBankIndex];

            std::vector<uint8_t> tempData;
            bank.Stream->clear();
            bank.Stream->seekg(meta.Offset, std::ios::beg);
            tempData.resize(meta.Size + 64);
            bank.Stream->read((char*)tempData.data(), meta.Size);

            g_TextureParser.Parse(meta.Info, tempData, meta.Type);

            if (g_TextureParser.IsParsed && !g_TextureParser.DecodedPixels.empty()) {
                ID3D11ShaderResourceView* srv = nullptr;
                DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                uint32_t blockWidth = 1;

                switch (g_TextureParser.DecodedFormat) {
                case ETextureFormat::DXT1: case ETextureFormat::NormalMap_DXT1: dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
                case ETextureFormat::DXT3: dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
                case ETextureFormat::DXT5: case ETextureFormat::NormalMap_DXT5: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
                case ETextureFormat::ARGB8888: dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
                }

                uint32_t w = g_TextureParser.Header.Width ? g_TextureParser.Header.Width : g_TextureParser.Header.FrameWidth;
                uint32_t h = g_TextureParser.Header.Height ? g_TextureParser.Header.Height : g_TextureParser.Header.FrameHeight;

                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
                desc.Format = dxFormat; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA subData = {};
                subData.pSysMem = g_TextureParser.DecodedPixels.data();
                if (blockWidth == 4) subData.SysMemPitch = ((w + 3) / 4) * ((dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16);
                else subData.SysMemPitch = w * 4;

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

    // 2. Process global PC texture banks
    for (auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
            for (int i = 0; i < bank.Entries.size(); ++i) {
                if (bank.Entries[i].ID == (uint32_t)textureID) {

                    if (bank.StagedEntries.count(i) && bank.StagedEntries[i].Texture) {
                        auto& tex = bank.StagedEntries[i].Texture;
                        if (tex->RawFrames.empty()) return nullptr;

                        uint32_t w = tex->Header.Width ? tex->Header.Width : tex->Header.FrameWidth;
                        uint32_t h = tex->Header.Height ? tex->Header.Height : tex->Header.FrameHeight;
                        if (w == 0 || h == 0) return nullptr;

                        D3D11_TEXTURE2D_DESC desc = {};
                        desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
                        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                        D3D11_SUBRESOURCE_DATA subData = {};
                        subData.pSysMem = tex->RawFrames[0].data();
                        subData.SysMemPitch = w * 4;

                        ID3D11Texture2D* d3dTex = nullptr;
                        ID3D11ShaderResourceView* srv = nullptr;
                        if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &subData, &d3dTex))) {
                            g_pd3dDevice->CreateShaderResourceView(d3dTex, nullptr, &srv);
                            d3dTex->Release();
                            g_MeshTextureCache[textureID] = srv;
                            return srv;
                        }
                    }

                    std::vector<uint8_t> tempData;
                    if (bank.ModifiedEntryData.count(i)) {
                        tempData = bank.ModifiedEntryData[i];
                    }
                    else {
                        bank.Stream->clear();
                        bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);
                        tempData.resize(bank.Entries[i].Size + 64);
                        bank.Stream->read((char*)tempData.data(), bank.Entries[i].Size);
                    }

                    g_TextureParser.Parse(bank.SubheaderCache[i], tempData, bank.Entries[i].Type);

                    if (g_TextureParser.IsParsed && !g_TextureParser.DecodedPixels.empty()) {
                        ID3D11ShaderResourceView* srv = nullptr;
                        DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                        uint32_t blockWidth = 1;

                        switch (g_TextureParser.DecodedFormat) {
                        case ETextureFormat::DXT1: case ETextureFormat::NormalMap_DXT1: dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
                        case ETextureFormat::DXT3: dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
                        case ETextureFormat::DXT5: case ETextureFormat::NormalMap_DXT5: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
                        case ETextureFormat::ARGB8888: dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
                        }

                        uint32_t w = g_TextureParser.Header.Width ? g_TextureParser.Header.Width : g_TextureParser.Header.FrameWidth;
                        uint32_t h = g_TextureParser.Header.Height ? g_TextureParser.Header.Height : g_TextureParser.Header.FrameHeight;

                        D3D11_TEXTURE2D_DESC desc = {};
                        desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
                        desc.Format = dxFormat; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                        D3D11_SUBRESOURCE_DATA subData = {};
                        subData.pSysMem = g_TextureParser.DecodedPixels.data();
                        if (blockWidth == 4) subData.SysMemPitch = ((w + 3) / 4) * ((dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16);
                        else subData.SysMemPitch = w * 4;

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

inline std::string ExtractTextureForGltf(int textureID, const std::string& exportDir) {
    if (textureID <= 0) return "";

    // 1. Process local Xbox bank first
    if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics) {
        EnsureXboxTextureCache(g_ActiveBankIndex);
        auto& cache = g_XboxTexCache[g_ActiveBankIndex];

        if (cache.count(textureID)) {
            auto& meta = cache[textureID];
            LoadedBank& bank = g_OpenBanks[g_ActiveBankIndex];

            std::string fname = "tex_" + std::to_string(textureID) + ".dds";
            std::string fullPath = exportDir + fname;

            std::vector<uint8_t> tempData;
            bank.Stream->clear();
            bank.Stream->seekg(meta.Offset, std::ios::beg);
            tempData.resize(meta.Size + 64);
            bank.Stream->read((char*)tempData.data(), meta.Size);

            CTextureParser parser;
            parser.Parse(meta.Info, tempData, meta.Type);

            if (parser.IsParsed && !parser.DecodedPixels.empty()) {
                if (TextureExporter::ExportDDS(parser, fullPath, 0)) {
                    return fname;
                }
            }
        }
    }

    // 2. Process global PC texture banks
    for (auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
            for (int i = 0; i < bank.Entries.size(); ++i) {
                if (bank.Entries[i].ID == (uint32_t)textureID) {

                    std::string fname = "tex_" + std::to_string(textureID) + ".dds";
                    std::string fullPath = exportDir + fname;

                    if (bank.StagedEntries.count(i) && bank.StagedEntries[i].Texture) {
                        auto& texInfo = bank.StagedEntries[i].Texture;
                        if (texInfo->RawFrames.empty()) return "";
                        TextureBuilder::ImportOptions opts;
                        opts.Format = texInfo->TargetFormat;
                        opts.GenerateMipmaps = true;

                        int w = texInfo->Header.Width ? texInfo->Header.Width : texInfo->Header.FrameWidth;
                        int h = texInfo->Header.Height ? texInfo->Header.Height : texInfo->Header.FrameHeight;

                        auto compiled = TextureBuilder::CompileFromRGBA(texInfo->RawFrames[0], w, h, opts);

                        CTextureParser tempParser;
                        tempParser.Header = texInfo->Header;
                        tempParser.Header.MipmapLevels = ((CGraphicHeader*)compiled.HeaderInfo.data())->MipmapLevels;
                        tempParser.DecodedFormat = texInfo->TargetFormat;
                        tempParser.DecodedPixels = compiled.FullData;
                        tempParser.IsParsed = true;

                        if (TextureExporter::ExportDDS(tempParser, fullPath, 0)) return fname;
                    }

                    std::vector<uint8_t> tempData;
                    if (bank.ModifiedEntryData.count(i)) {
                        tempData = bank.ModifiedEntryData[i];
                    }
                    else {
                        bank.Stream->clear();
                        bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);
                        tempData.resize(bank.Entries[i].Size + 64);
                        bank.Stream->read((char*)tempData.data(), bank.Entries[i].Size);
                    }

                    CTextureParser parser;
                    parser.Parse(bank.SubheaderCache[i], tempData, bank.Entries[i].Type);

                    if (parser.IsParsed && !parser.DecodedPixels.empty()) {
                        if (TextureExporter::ExportDDS(parser, fullPath, 0)) {
                            return fname;
                        }
                    }
                }
            }
        }
    }
    return "";
}

inline void CheckMeshUpload(ID3D11Device* device) {
    if (g_MeshUploadNeeded) {
        g_MeshRenderer.Initialize(device);

        g_PreviewAnimPlaying = false;
        g_PreviewAnimTime = 0.0f;
        g_PreviewBoneTransforms.clear();
        g_PreviewGlobalTransforms.clear();

        bool resetCam = !g_PreserveCamera;

        if (g_BBMParser.IsParsed) {
            g_MeshRenderer.UploadBBM(device, g_BBMParser, resetCam);

            // Added BBM Material Support!
            std::vector<MeshRenderer::RenderMaterial> materials;
            int maxMat = 0;
            for (const auto& m : g_BBMParser.ParsedMaterials) if (m.Index > maxMat) maxMat = m.Index;
            materials.resize(maxMat + 1);

            for (const auto& m : g_BBMParser.ParsedMaterials) {
                if (m.DiffuseBank > 0) materials[m.Index].Diffuse = LoadTextureForMesh(m.DiffuseBank);
                if (m.BumpBank > 0) materials[m.Index].Bump = LoadTextureForMesh(m.BumpBank);
                if (m.ReflectBank > 0) materials[m.Index].Specular = LoadTextureForMesh(m.ReflectBank);
            }
            g_MeshRenderer.SetMaterials(materials);
        }
        else if (g_ActiveMeshContent.IsParsed) {
            g_MeshRenderer.UploadMesh(device, g_ActiveMeshContent, resetCam);

            std::vector<MeshRenderer::RenderMaterial> materials;
            int maxMat = 0;
            for (const auto& m : g_ActiveMeshContent.Materials) if (m.ID > maxMat) maxMat = m.ID;
            materials.resize(maxMat + 1);

            // PASS 1: Load Degenerate/Dummy materials first (as fallbacks)
            for (const auto& m : g_ActiveMeshContent.Materials) {
                if (!m.DegenerateTriangles) continue;

                if (m.DiffuseMapID > 0) materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                if (m.BumpMapID > 0) materials[m.ID].Bump = LoadTextureForMesh(m.BumpMapID);
                if (m.ReflectionMapID > 0) materials[m.ID].Specular = LoadTextureForMesh(m.ReflectionMapID);
                materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;
            }

            // PASS 2: Load REAL materials (These will overwrite the dummy data!)
            for (const auto& m : g_ActiveMeshContent.Materials) {
                if (m.DegenerateTriangles) continue;

                if (m.DiffuseMapID > 0) materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                if (m.BumpMapID > 0) materials[m.ID].Bump = LoadTextureForMesh(m.BumpMapID);
                if (m.ReflectionMapID > 0) materials[m.ID].Specular = LoadTextureForMesh(m.ReflectionMapID);
                materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;
            }

            g_MeshRenderer.SetMaterials(materials);
        }
        g_MeshUploadNeeded = false;
        g_PreserveCamera = false;
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

    for (int i = 0; i < boneCount; i++) {
        if ((i + 1) * 64 <= g_ActiveMeshContent.BoneTransformsRaw.size()) {
            float* raw = (float*)(g_ActiveMeshContent.BoneTransformsRaw.data() + i * 64);
            XMMATRIX rawMatrix = XMMATRIX(raw);

            rawMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            XMMATRIX dxIBM = XMMatrixTranspose(rawMatrix);

            ibm[i] = dxIBM;
            bindGlobal[i] = XMMatrixInverse(nullptr, dxIBM);
        }
        else {
            ibm[i] = XMMatrixIdentity();
            bindGlobal[i] = XMMatrixIdentity();
        }
    }

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

    if (!isAnimLoaded) {
        for (int i = 0; i < boneCount; i++) {
            g_PreviewBoneTransforms[i] = XMMatrixIdentity();
            g_PreviewGlobalTransforms[i] = bindGlobal[i];
        }
        return;
    }

    std::vector<XMMATRIX> localTransforms(boneCount);

    auto cleanName = [](const std::string& str) {
        std::string res;
        for (char c : str) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) res += (char)tolower(c);
        }
        return res;
        };

    for (int i = 0; i < boneCount; i++) {
        bool hasAnim = false;

        std::string targetBoneName = i < g_ActiveMeshContent.BoneNames.size() ? g_ActiveMeshContent.BoneNames[i] : "";
        std::string cleanTarget = cleanName(targetBoneName);

        if (!cleanTarget.empty()) {
            for (const auto& track : g_PreviewAnimParser.Data.Tracks) {
                std::string cleanTrack = cleanName(track.BoneName);

                if (cleanTarget == cleanTrack) {
                    if (track.FrameCount > 0 && track.SamplesPerSecond > 0) {
                        int frame = (int)(g_PreviewAnimTime * track.SamplesPerSecond) % track.FrameCount;
                        if (frame < 0) frame += track.FrameCount;

                        if (g_SelectedAnimType == 7) {
                            Vec3 p = { 0.0f, 0.0f, 0.0f };
                            Vec4 q = { 0.0f, 0.0f, 0.0f, 1.0f };

                            track.EvaluateFrame(frame, p, q);

                            XMVECTOR vPos = XMVectorSet(p.x, p.y, p.z, 1.0f);
                            XMVECTOR vRot = XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w));
                            vRot = XMQuaternionConjugate(vRot);

                            XMMATRIX trackMat = XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vPos);
                            localTransforms[i] = XMMatrixMultiply(trackMat, bindLocal[i]);
                        }
                        else {
                            XMVECTOR s_b, r_b, t_b;
                            XMMatrixDecompose(&s_b, &r_b, &t_b, bindLocal[i]);
                            r_b = XMQuaternionConjugate(r_b);

                            Vec3 p = { XMVectorGetX(t_b), XMVectorGetY(t_b), XMVectorGetZ(t_b) };
                            Vec4 q = { XMVectorGetX(r_b), XMVectorGetY(r_b), XMVectorGetZ(r_b), XMVectorGetW(r_b) };

                            track.EvaluateFrame(frame, p, q);

                            XMVECTOR vPos = XMVectorSet(p.x, p.y, p.z, 1.0f);
                            XMVECTOR vRot = XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w));
                            vRot = XMQuaternionConjugate(vRot);

                            localTransforms[i] = XMMatrixScalingFromVector(s_b) * XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vPos);
                        }

                        hasAnim = true;
                    }
                    break;
                }
            }
        }

        if (!hasAnim) {
            localTransforms[i] = bindLocal[i];
        }
    }

    std::vector<bool> computed(boneCount, false);
    std::function<void(int)> ComputeGlobal = [&](int idx) {
        if (computed[idx]) return;

        int p = g_ActiveMeshContent.Bones[idx].ParentIndex;
        if (p != -1 && p < boneCount) {
            ComputeGlobal(p);
            g_PreviewGlobalTransforms[idx] = XMMatrixMultiply(localTransforms[idx], g_PreviewGlobalTransforms[p]);
        }
        else {
            g_PreviewGlobalTransforms[idx] = localTransforms[idx];
        }
        computed[idx] = true;
        };

    for (int i = 0; i < boneCount; i++) {
        ComputeGlobal(i);
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

    if (g_ActiveMeshContent.ClothFlag) {
        ImGui::SameLine();
        ImGui::Checkbox("Cloth", &g_ShowCloth);
    }

    if (g_ActiveMeshContent.BoneCount > 0) {
        ImGui::SameLine();
        ImGui::Checkbox("Skeleton", &g_ShowSkeleton);
    }

    if (g_ActiveMeshContent.IsParsed) {
        if (g_ActiveMeshContent.EntryMeta.PhysicsIndex <= 0) {
            g_ShowPhysicsOverlay = false;
            g_LoadedOverlayID = -1;
        }
        else {
            ImGui::SameLine();
            ImGui::Checkbox("Physics", &g_ShowPhysicsOverlay);

            if (g_ShowPhysicsOverlay && g_LoadedOverlayID != g_ActiveMeshContent.EntryMeta.PhysicsIndex) {
                g_LoadedOverlayID = g_ActiveMeshContent.EntryMeta.PhysicsIndex;
                bool loaded = false;

                bool isXboxActive = (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics);

                auto LoadOverlayFromBank = [&](LoadedBank& bank) {
                    for (int i = 0; i < bank.Entries.size(); ++i) {
                        if (bank.Entries[i].ID == g_LoadedOverlayID) {
                            std::vector<uint8_t> rawData;
                            if (bank.ModifiedEntryData.count(i)) rawData = bank.ModifiedEntryData[i];
                            else {
                                bank.Stream->clear();
                                bank.Stream->seekg(bank.Entries[i].Offset, std::ios::beg);
                                rawData.resize(bank.Entries[i].Size);
                                bank.Stream->read((char*)rawData.data(), bank.Entries[i].Size);
                            }
                            g_OverlayBBMParser.Parse(rawData);
                            g_PhysicsOverlayRenderer.Initialize(g_pd3dDevice);
                            g_PhysicsOverlayRenderer.UploadBBM(g_pd3dDevice, g_OverlayBBMParser);
                            return true;
                        }
                    }
                    return false;
                    };

                if (isXboxActive) {
                    loaded = LoadOverlayFromBank(g_OpenBanks[g_ActiveBankIndex]);
                }
                else {
                    for (auto& bank : g_OpenBanks) {
                        if (bank.Type == EBankType::Graphics) {
                            if (LoadOverlayFromBank(bank)) {
                                loaded = true;
                                break;
                            }
                        }
                    }
                }

                if (!loaded) g_ShowPhysicsOverlay = false;
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export to glTF")) {
        std::string savePath = SaveFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
        if (!savePath.empty()) {
            for (auto& b : g_OpenBanks) FlushStagedEntries(&b);
            if (savePath.length() < 5 || savePath.substr(savePath.length() - 5) != ".gltf") savePath += ".gltf";
            std::string expDir = savePath.substr(0, savePath.find_last_of("\\/") + 1);
            auto extFunc = [expDir](int id) { return ExtractTextureForGltf(id, expDir); };

            if (g_BBMParser.IsParsed) GltfExporter::ExportBBM(g_BBMParser, savePath);
            else if (g_ActiveMeshContent.IsParsed) GltfExporter::Export(g_ActiveMeshContent, savePath, nullptr, 6, extFunc);
        }
    }

    if (g_ActiveMeshContent.IsParsed && g_PreviewAnimParser.Data.IsParsed) {
        ImGui::SameLine();
        if (ImGui::Button("Export Mesh + Anim (glTF)")) {
            std::string savePath = SaveFileDialog("glTF Files\0*.gltf\0All Files\0*.*\0");
            if (!savePath.empty()) {
                for (auto& b : g_OpenBanks) FlushStagedEntries(&b);
                if (savePath.length() < 5 || savePath.substr(savePath.length() - 5) != ".gltf") savePath += ".gltf";
                GltfExporter::Export(g_ActiveMeshContent, savePath, &g_PreviewAnimParser, g_SelectedAnimType);
            }
        }
        if (ImGui::IsItemHovered()) {
            std::string tooltip = "Exports the mesh along with the currently playing animation.";
            if (g_SelectedAnimType == 7) tooltip += "\nDelta Animation: Baked to Bind Pose.";
            if (g_SelectedAnimType == 9) tooltip += "\nPartial Animation: Only masked bones are exported.";
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export Uncompressed Binary")) {
        std::string savePath = SaveFileDialog("Binary Files\0*.bin\0All Files\0*.*\0");
        if (!savePath.empty()) {
            if (g_BBMParser.IsParsed) {
                std::vector<uint8_t> outData = MeshCompiler::CompilePhysicsUncompressed(g_BBMParser);
                std::ofstream out(savePath, std::ios::binary);
                out.write((char*)outData.data(), outData.size());
                g_BankStatus = "Exported Uncompressed Physics Mesh (BBM)!";
                g_ShowSuccessPopup = true;
                g_SuccessMessage = "Physics Binary dumped correctly!";
            }
            else if (g_ActiveMeshContent.IsParsed) {
                std::vector<uint8_t> outData = MeshCompiler::CompileForExport(g_ActiveMeshContent, true);
                std::ofstream out(savePath, std::ios::binary);
                out.write((char*)outData.data(), outData.size());
                g_BankStatus = "Exported Uncompressed Mesh Binary (LZO Bypassed)!";
                g_ShowSuccessPopup = true;
                g_SuccessMessage = "Binary dumped correctly (LZO Bypassed)!";
            }
            else {
                g_BankStatus = "Error: No active parsed mesh available to export.";
            }
        }
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

    ImGui::BeginChild("MeshViewportChild", ImVec2(viewportWidth, avail.y), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    g_MeshRenderer.Resize(g_pd3dDevice, viewportWidth, avail.y);
    if (g_ShowPhysicsOverlay) g_PhysicsOverlayRenderer.Resize(g_pd3dDevice, viewportWidth, avail.y);

    ID3D11DeviceContext* ctx;
    g_pd3dDevice->GetImmediateContext(&ctx);

    ID3D11ShaderResourceView* tex = g_MeshRenderer.Render(ctx, viewportWidth, avail.y, g_ShowWireframe, g_BBMParser.IsParsed, &g_PreviewBoneTransforms, true, 1.0f, nullptr, nullptr, g_ShowCloth);

    if (g_ShowBounds) {
        if (g_BBMParser.IsParsed) {
            for (const auto& v : g_BBMParser.Volumes) {
                std::vector<CPlane> pCnv;
                for (const auto& pl : v.Planes) pCnv.push_back({ {pl.Normal[0], pl.Normal[1], pl.Normal[2]}, pl.D });
                g_MeshRenderer.RenderVolumes(ctx, viewportWidth, avail.y, pCnv, true);
            }
        }
        else if (g_ActiveMeshContent.IsParsed) {
            g_MeshRenderer.RenderBounds(ctx, viewportWidth, avail.y, g_ActiveMeshContent.BoundingBoxMin, g_ActiveMeshContent.BoundingBoxMax, g_ActiveMeshContent.BoundingSphereCenter, g_ActiveMeshContent.BoundingSphereRadius);
            for (const auto& v : g_ActiveMeshContent.Volumes) {
                g_MeshRenderer.RenderVolumes(ctx, viewportWidth, avail.y, v.Planes, false);
            }
        }
    }

    if (g_ShowPhysicsOverlay && g_OverlayBBMParser.IsParsed && !g_BBMParser.IsParsed) {
        g_PhysicsOverlayRenderer.CamRotX = g_MeshRenderer.CamRotX;
        g_PhysicsOverlayRenderer.CamRotY = g_MeshRenderer.CamRotY;
        g_PhysicsOverlayRenderer.CamDist = g_MeshRenderer.CamDist;
        g_PhysicsOverlayRenderer.CamPan = g_MeshRenderer.CamPan;

        g_PhysicsOverlayRenderer.Render(ctx, viewportWidth, avail.y, true, false, &g_PreviewBoneTransforms, false, 0.5f, g_MeshRenderer.GetRTV(), g_MeshRenderer.GetDSV(), false);
    }

    ctx->Release();

    ImVec2 pMin = ImGui::GetCursorScreenPos();
    ImVec2 pMax = ImVec2(pMin.x + viewportWidth, pMin.y + avail.y);

    if (tex) ImGui::Image((void*)tex, ImVec2(viewportWidth, avail.y));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(pMin, pMax, true);

    if (g_ShowSkeleton && !g_PreviewGlobalTransforms.empty()) {
        for (int i = 0; i < g_ActiveMeshContent.BoneCount; i++) {
            XMMATRIX globalMat = g_PreviewGlobalTransforms[i];
            XMFLOAT3 pos(XMVectorGetX(globalMat.r[3]), XMVectorGetY(globalMat.r[3]), XMVectorGetZ(globalMat.r[3]));

            ImVec2 scrPos;
            if (g_MeshRenderer.ProjectToScreen(pos, scrPos, viewportWidth, avail.y)) {
                scrPos.x += pMin.x; scrPos.y += pMin.y;
                dl->AddCircleFilled(scrPos, 3.0f, IM_COL32(0, 255, 0, 255));

                int parentIdx = g_ActiveMeshContent.Bones[i].ParentIndex;
                if (parentIdx != -1 && parentIdx < g_ActiveMeshContent.BoneCount) {
                    XMMATRIX pMat = g_PreviewGlobalTransforms[parentIdx];
                    XMFLOAT3 pPos(XMVectorGetX(pMat.r[3]), XMVectorGetY(pMat.r[3]), XMVectorGetZ(pMat.r[3]));
                    ImVec2 pScrPos;
                    if (g_MeshRenderer.ProjectToScreen(pPos, pScrPos, viewportWidth, avail.y)) {
                        pScrPos.x += pMin.x; pScrPos.y += pMin.y;
                        dl->AddLine(scrPos, pScrPos, IM_COL32(255, 255, 0, 255), 2.0f);
                    }
                }
            }
        }
    }

    if (g_ShowHelpers) {
        ImVec2 mousePos = ImGui::GetMousePos();

        if (g_BBMParser.IsParsed) {
            for (int i = 0; i < g_BBMParser.Helpers.size(); i++) {
                const auto& h = g_BBMParser.Helpers[i];
                ImVec2 scrPos;
                if (g_MeshRenderer.ProjectToScreen(XMFLOAT3(h.Position.x, h.Position.y, h.Position.z), scrPos, viewportWidth, avail.y, true)) {
                    scrPos.x += pMin.x; scrPos.y += pMin.y;
                    dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(0, 255, 255, 200));

                    float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                    if (dist < 8.0f) {
                        dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                        ImGui::SetTooltip("%s", h.Name.c_str());
                    }
                }
            }
            for (int i = 0; i < g_BBMParser.Dummies.size(); i++) {
                const auto& d = g_BBMParser.Dummies[i];
                ImVec2 scrPos;
                if (g_MeshRenderer.ProjectToScreen(XMFLOAT3(d.Transform[9], d.Transform[10], d.Transform[11]), scrPos, viewportWidth, avail.y, true)) {
                    scrPos.x += pMin.x; scrPos.y += pMin.y;
                    dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(255, 0, 255, 200));

                    float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                    if (dist < 8.0f) {
                        dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                        ImGui::SetTooltip("%s", d.Name.c_str());
                    }
                }
            }
        }

        for (int i = 0; i < g_ActiveMeshContent.Generators.size(); i++) {
            const auto& gen = g_ActiveMeshContent.Generators[i];
            XMFLOAT3 pos = XMFLOAT3(gen.Transform[9], gen.Transform[10], gen.Transform[11]);

            ImVec2 scrPos;
            if (g_MeshRenderer.ProjectToScreen(pos, scrPos, viewportWidth, avail.y)) {
                scrPos.x += pMin.x; scrPos.y += pMin.y;

                dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(255, 0, 0, 200));

                float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                if (dist < 8.0f) {
                    dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                    ImGui::SetTooltip("%s", gen.ObjectName.c_str());
                }
            }
        }

        if (g_ActiveMeshContent.IsParsed) {
            for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
                const auto& h = g_ActiveMeshContent.Helpers[i];
                ImVec2 scrPos;
                if (g_MeshRenderer.ProjectToScreen(XMFLOAT3(h.Pos[0], h.Pos[1], h.Pos[2]), scrPos, viewportWidth, avail.y)) {
                    scrPos.x += pMin.x; scrPos.y += pMin.y;
                    dl->AddCircleFilled(scrPos, 4.0f, IM_COL32(0, 255, 255, 200));

                    float dist = sqrtf((mousePos.x - scrPos.x) * (mousePos.x - scrPos.x) + (mousePos.y - scrPos.y) * (mousePos.y - scrPos.y));
                    if (dist < 8.0f) {
                        dl->AddCircle(scrPos, 6.0f, IM_COL32(255, 255, 0, 255));
                        std::string name = g_ActiveMeshContent.GetNameFromCRC(h.NameCRC);
                        if (name.empty()) name = "CRC: " + std::to_string(h.NameCRC);
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
                        std::string name = g_ActiveMeshContent.GetNameFromCRC(d.NameCRC);
                        if (name.empty()) name = "CRC: " + std::to_string(d.NameCRC);
                        ImGui::SetTooltip("%s", name.c_str());
                    }
                }
            }
        }
    }
    dl->PopClipRect();
    ImGui::EndChild();

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

        if (ImGui::BeginTabBar("MeshTabs")) {
            if (ImGui::BeginTabItem("Overview")) {
                if (g_BBMParser.IsParsed) {
                    ImGui::Text("Version: %u", g_BBMParser.FileVersion);
                    ImGui::Text("Comment: %s", g_BBMParser.FileComment.c_str());
                    ImGui::Separator();
                    ImGui::Text("Vertices: %zu", g_BBMParser.ParsedVertices.size());
                    ImGui::Text("Indices:  %zu", g_BBMParser.ParsedIndices.size());
                }
                else {
                    ImGui::Text("Materials: %d", g_ActiveMeshContent.MaterialCount);
                    ImGui::Text("Primitives: %d", g_ActiveMeshContent.PrimitiveCount);
                    ImGui::Text("Bones: %d", g_ActiveMeshContent.BoneCount);

                    ImGui::Dummy(ImVec2(0, 5));
                    if (g_ActiveMeshContent.ClothFlag) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "Cloth: YES");
                        ImGui::SameLine();
                        if (ImGui::Button("Export Raw Cloth Data")) {
                            std::string savePath = SaveFileDialog("Binary Data\0*.bin\0All Files\0*.*\0");
                            if (!savePath.empty()) {
                                std::ofstream out(savePath, std::ios::binary);
                                for (const auto& p : g_ActiveMeshContent.Primitives) {
                                    for (const auto& cp : p.ClothPrimitives) {
                                        // Write a tiny header for the hex editor: PrimID, MatID, Size
                                        uint32_t pIdx = cp.PrimitiveIndex;
                                        uint32_t mIdx = cp.MaterialIndex;
                                        uint32_t sz = (uint32_t)cp.ParticleProgramData.size();
                                        out.write((char*)&pIdx, 4);
                                        out.write((char*)&mIdx, 4);
                                        out.write((char*)&sz, 4);
                                        if (sz > 0) {
                                            out.write((char*)cp.ParticleProgramData.data(), sz);
                                        }
                                    }
                                }
                                g_BankStatus = "Exported raw cloth data!";
                            }
                        }
                    }
                    else {
                        ImGui::TextDisabled("Cloth: NO");
                    }
                    ImGui::Dummy(ImVec2(0, 5));

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

                        bool tocChanged = false;

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Physics Index");
                        ImGui::SameLine(130);
                        ImGui::SetNextItemWidth(80);
                        if (ImGui::InputInt("##phys_idx", &g_ActiveMeshContent.EntryMeta.PhysicsIndex, 0, 0)) tocChanged = true;
                        ImGui::SameLine();
                        if (ImGui::Button("+##phys_btn", ImVec2(24, 0))) {
                            g_SelectedPhysicsID = g_ActiveMeshContent.EntryMeta.PhysicsIndex;
                            g_PhysicsSearchBuf[0] = '\0';
                            g_TriggerPhysicsPopup = true;
                        }

                        //ImGui::AlignTextToFramePadding();
                        //ImGui::Text("Safe Radius");
                        //ImGui::SameLine(130);
                        //ImGui::SetNextItemWidth(80);
                        //if (ImGui::InputFloat("##safe_rad", &g_ActiveMeshContent.EntryMeta.SafeBoundingRadius, 0.0f, 0.0f, "%.3f")) tocChanged = true;

                        if (g_ActiveMeshContent.EntryMeta.LODCount > 0) {
                            ImGui::Dummy(ImVec2(0, 5));
                            if (ImGui::BeginTable("LODTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60);
                                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
                                ImGui::TableSetupColumn("Error");
                                ImGui::TableHeadersRow();
                                for (uint32_t i = 0; i < g_ActiveMeshContent.EntryMeta.LODCount; i++) {
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0); ImGui::Text("LOD %d", i);
                                    ImGui::TableSetColumnIndex(1);
                                    if (i < g_ActiveMeshContent.EntryMeta.LODSizes.size()) ImGui::Text("%d", g_ActiveMeshContent.EntryMeta.LODSizes[i]);

                                    ImGui::TableSetColumnIndex(2);
                                    if (i < g_ActiveMeshContent.EntryMeta.LODErrors.size()) {
                                        ImGui::PushID(i);
                                        ImGui::SetNextItemWidth(-1);
                                        if (ImGui::InputFloat("##lod_err", &g_ActiveMeshContent.EntryMeta.LODErrors[i], 0.0f, 0.0f, "%.4f")) tocChanged = true;
                                        ImGui::PopID();
                                    }
                                    else {
                                        ImGui::TextDisabled("-");
                                    }
                                }
                                ImGui::EndTable();
                            }
                        }
                        if (tocChanged && saveCallback) saveCallback();
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
                }
                ImGui::EndTabItem();
            }

            if (g_ActiveMeshContent.AnimatedFlag && ImGui::BeginTabItem("Animations")) {
                struct FoundAnim { int BankIdx; int EntryIdx; std::string Name; uint32_t ID; int Type; };
                std::vector<FoundAnim> anims;

                // --- ISOLATED ANIMATION SCANNER ---
                bool isXboxActive = (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics);

                if (isXboxActive) {
                    // XBOX: Only scan the local active bank
                    int i = g_ActiveBankIndex;
                    for (int j = 0; j < g_OpenBanks[i].Entries.size(); j++) {
                        int t = g_OpenBanks[i].Entries[j].Type;
                        if (t == 6 || t == 7 || t == 9) {
                            anims.push_back({ i, j, g_OpenBanks[i].Entries[j].FriendlyName, g_OpenBanks[i].Entries[j].ID, t });
                        }
                    }
                }
                else {
                    // PC: Scan all standard graphics banks
                    for (int i = 0; i < g_OpenBanks.size(); i++) {
                        if (g_OpenBanks[i].Type == EBankType::Graphics) {
                            for (int j = 0; j < g_OpenBanks[i].Entries.size(); j++) {
                                int t = g_OpenBanks[i].Entries[j].Type;
                                if (t == 6 || t == 7 || t == 9) {
                                    anims.push_back({ i, j, g_OpenBanks[i].Entries[j].FriendlyName, g_OpenBanks[i].Entries[j].ID, t });
                                }
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

                    float fps = g_PreviewAnimParser.Data.Tracks.empty() ? 30.0f : g_PreviewAnimParser.Data.Tracks[0].SamplesPerSecond;
                    int maxFrames = (int)(duration * fps);
                    if (maxFrames < 1) maxFrames = 1;
                    int currentFrame = (int)(g_PreviewAnimTime * fps);

                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderInt("##FrameScrub", &currentFrame, 0, maxFrames, "%d")) {
                        g_PreviewAnimTime = (float)currentFrame / fps;
                        g_PreviewAnimPlaying = false;
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("FPS: %.1f", fps);
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

            // --- CLOTH PHYSICS TAB ---
            if (g_ActiveMeshContent.ClothFlag && ImGui::BeginTabItem("Cloth Physics")) {
                int cGlobalIdx = 0;
                for (size_t pIdx = 0; pIdx < g_ActiveMeshContent.Primitives.size(); pIdx++) {
                    auto& prim = g_ActiveMeshContent.Primitives[pIdx];
                    for (size_t cIdx = 0; cIdx < prim.ClothPrimitives.size(); cIdx++) {
                        auto& cp = prim.ClothPrimitives[cIdx];
                        if (!cp.Program.IsParsed) continue;

                        auto& sim = cp.Program.InitialSimulation;
                        auto& prog = cp.Program;

                        int springCount = 0;
                        for (const auto& inst : cp.Program.ParsedInstructions) {
                            if (inst.Opcode == 2) springCount += inst.Count;
                        }

                        std::string headerName = "Cloth Block " + std::to_string(cGlobalIdx) + " (Primitive " + std::to_string(pIdx) + ")";
                        if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::PushID(cGlobalIdx);

                            ImGui::TextDisabled("Particles: %d | Springs: %d", sim.Size, springCount);
                            ImGui::Dummy(ImVec2(0, 5));

                            bool changed = false;

                            // --- SECTION: CORE SIMULATION ---
                            if (ImGui::TreeNodeEx("Simulation Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Timestep", &sim.Timestep, 0.0001f, 0.0f, 1.0f, "%.4f")) changed = true;

                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Timestep Multiplier", &sim.TimestepMultiplier, 0.01f, 0.0f, 10.0f)) changed = true;

                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Gravity Strength", &sim.GravityStrength, 0.01f)) changed = true;

                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Wind Strength", &sim.WindStrength, 0.01f)) changed = true;

                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Global Damping", &sim.GlobalDamping, 0.001f, 0.0f, 1.0f)) changed = true;

                                ImGui::TreePop();
                            }

                            // --- SECTION: DRAG & ACCELERATION ---
                            if (ImGui::TreeNodeEx("Drag & Interaction", ImGuiTreeNodeFlags_DefaultOpen)) {
                                bool dragEn = sim.DraggingEnable != 0;
                                if (ImGui::Checkbox("Enable Drag", &dragEn)) { sim.DraggingEnable = dragEn ? 1 : 0; changed = true; }

                                ImGui::SameLine();
                                bool dragRot = sim.DraggingRotational != 0;
                                if (ImGui::Checkbox("Rotational Drag", &dragRot)) { sim.DraggingRotational = dragRot ? 1 : 0; changed = true; }

                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Drag Strength", &sim.DraggingStrength, 0.01f)) changed = true;

                                bool accelEn = sim.AccelerationEnable != 0;
                                if (ImGui::Checkbox("Enable Acceleration", &accelEn)) { sim.AccelerationEnable = accelEn ? 1 : 0; changed = true; }

                                ImGui::TreePop();
                            }

                            // --- SECTION: RENDERING ---
                            if (ImGui::TreeNodeEx("Rendering Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::SetNextItemWidth(150);
                                if (ImGui::DragFloat("Avg Patch Size", &prog.AveragePatchSize, 0.01f)) changed = true;

                                bool bezierEn = prog.BezierEnable != 0;
                                if (ImGui::Checkbox("Enable Bezier (Curved Cloth)", &bezierEn)) { prog.BezierEnable = bezierEn ? 1 : 0; changed = true; }

                                ImGui::TreePop();
                            }

                            if (changed && saveCallback) saveCallback();

                            ImGui::PopID();
                        }
                        cGlobalIdx++;
                    }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Extras")) {
                auto GetGraphString = [&](uint32_t crc) -> std::string {
                    std::string name = g_ActiveMeshContent.GetNameFromCRC(crc);
                    if (name.empty() && crc != 0) return "CRC_" + std::to_string(crc);
                    return name;
                    };
                auto SortAndRepackGraphExtras = [&]() {
                    std::sort(g_ActiveMeshContent.Helpers.begin(), g_ActiveMeshContent.Helpers.end(), [](const CHelperPoint& a, const CHelperPoint& b) { return a.NameCRC < b.NameCRC; });
                    std::sort(g_ActiveMeshContent.Dummies.begin(), g_ActiveMeshContent.Dummies.end(), [](const CDummyObject& a, const CDummyObject& b) { return a.NameCRC < b.NameCRC; });
                    std::sort(g_ActiveMeshContent.Generators.begin(), g_ActiveMeshContent.Generators.end(), [](const CMeshGenerator& a, const CMeshGenerator& b) { return CalculateFableCRC(a.ObjectName) < CalculateFableCRC(b.ObjectName); });

                    std::vector<std::string> hNames, dNames;
                    for (auto& h : g_ActiveMeshContent.Helpers) hNames.push_back(GetGraphString(h.NameCRC));
                    for (auto& d : g_ActiveMeshContent.Dummies) dNames.push_back(GetGraphString(d.NameCRC));

                    g_ActiveMeshContent.PackNames(hNames, dNames);
                    g_ActiveMeshContent.UnpackNames();
                    };

                if (ImGui::CollapsingHeader("Helper Points", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (g_BBMParser.IsParsed) {
                        if (ImGui::Button("+ Add Helper##BBM")) {
                            CBBMParser::HelperPoint h = { "NewHelper", {0,0,0}, -1, 0 };
                            g_BBMParser.Helpers.push_back(h);
                            if (saveCallback) saveCallback();
                        }
                        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Physics Mesh)");

                        for (int i = 0; i < g_BBMParser.Helpers.size(); i++) {
                            auto& h = g_BBMParser.Helpers[i];
                            ImGui::PushID(("BBM_H_" + std::to_string(i)).c_str());

                            char nameBuf[128]; strncpy_s(nameBuf, h.Name.c_str(), 127);
                            ImGui::SetNextItemWidth(150);
                            if (ImGui::InputText("##Name", nameBuf, 128)) { h.Name = nameBuf; }
                            if (ImGui::IsItemDeactivatedAfterEdit()) { if (saveCallback) saveCallback(); }

                            ImGui::SameLine(); ImGui::SetNextItemWidth(200);
                            if (ImGui::DragFloat3("##Pos", &h.Position.x, 0.05f)) { g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }

                            ImGui::SameLine();
                            if (ImGui::Button((h.BoneIndex >= 0 ? "Bone: " + std::to_string(h.BoneIndex) : "No Bone").c_str(), ImVec2(80, 0))) {
                                g_EditingTargetType = 0; g_EditingTargetIndex = i; g_TriggerBonePopup = true;
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("X")) { g_BBMParser.Helpers.erase(g_BBMParser.Helpers.begin() + i); i--; g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }
                            ImGui::PopID();
                        }
                    }
                    else if (g_ActiveMeshContent.IsParsed) {
                        if (ImGui::Button("+ Add Helper##Graph")) {
                            CHelperPoint h = { CalculateFableCRC("NewHelper"), {0,0,0}, -1 };
                            g_ActiveMeshContent.CRCNameMap[h.NameCRC] = "NewHelper";
                            g_ActiveMeshContent.Helpers.push_back(h);
                            g_ActiveMeshContent.HelperPointCount = (uint16_t)g_ActiveMeshContent.Helpers.size();
                            SortAndRepackGraphExtras();
                            if (saveCallback) saveCallback();
                        }
                        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Graphics Mesh)");

                        for (int i = 0; i < g_ActiveMeshContent.Helpers.size(); i++) {
                            auto& h = g_ActiveMeshContent.Helpers[i];
                            ImGui::PushID(("Graph_H_" + std::to_string(i)).c_str());

                            std::string sName = GetGraphString(h.NameCRC);
                            char nameBuf[128]; strncpy_s(nameBuf, sName.c_str(), 127);
                            ImGui::SetNextItemWidth(150);

                            if (ImGui::InputText("##Name", nameBuf, 128)) {
                                std::string newStr = nameBuf;
                                h.NameCRC = CalculateFableCRC(newStr);
                                g_ActiveMeshContent.CRCNameMap[h.NameCRC] = newStr;
                            }

                            if (ImGui::IsItemDeactivatedAfterEdit()) {
                                SortAndRepackGraphExtras();
                                if (saveCallback) saveCallback();
                            }

                            ImGui::SameLine(); ImGui::SetNextItemWidth(200);
                            if (ImGui::DragFloat3("##Pos", h.Pos, 0.05f)) { g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }

                            ImGui::SameLine();
                            if (ImGui::Button((h.BoneIndex >= 0 ? "Bone: " + std::to_string(h.BoneIndex) : "No Bone").c_str(), ImVec2(80, 0))) {
                                g_EditingTargetType = 0; g_EditingTargetIndex = i; g_TriggerBonePopup = true;
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("X")) {
                                g_ActiveMeshContent.Helpers.erase(g_ActiveMeshContent.Helpers.begin() + i);
                                g_ActiveMeshContent.HelperPointCount = (uint16_t)g_ActiveMeshContent.Helpers.size();
                                SortAndRepackGraphExtras();
                                i--; g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback();
                            }
                            ImGui::PopID();
                        }
                    }
                }

                if (ImGui::CollapsingHeader("Dummy Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (g_BBMParser.IsParsed) {
                        if (ImGui::Button("+ Add Dummy##BBM")) {
                            CBBMParser::DummyObject d; d.Name = "NewDummy"; d.BoneIndex = -1; d.SubMeshIndex = 0; d.UseLocalOrigin = false;
                            d.Position = { 0,0,0 }; d.Direction = { 0,0,1 };
                            memset(d.Transform, 0, 48); d.Transform[0] = 1; d.Transform[4] = 1; d.Transform[8] = 1;
                            g_BBMParser.Dummies.push_back(d);
                            if (saveCallback) saveCallback();
                        }

                        for (int i = 0; i < g_BBMParser.Dummies.size(); i++) {
                            auto& d = g_BBMParser.Dummies[i];
                            ImGui::PushID(("BBM_D_" + std::to_string(i)).c_str());

                            char nameBuf[128]; strncpy_s(nameBuf, d.Name.c_str(), 127);
                            ImGui::SetNextItemWidth(150);
                            if (ImGui::InputText("##Name", nameBuf, 128)) { d.Name = nameBuf; }
                            if (ImGui::IsItemDeactivatedAfterEdit()) { if (saveCallback) saveCallback(); }

                            ImGui::SameLine(); ImGui::SetNextItemWidth(200);
                            if (ImGui::DragFloat3("##Pos", &d.Transform[9], 0.05f)) { g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }

                            ImGui::SameLine();
                            if (ImGui::Button((d.BoneIndex >= 0 ? "Bone: " + std::to_string(d.BoneIndex) : "No Bone").c_str(), ImVec2(80, 0))) {
                                g_EditingTargetType = 1; g_EditingTargetIndex = i; g_TriggerBonePopup = true;
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("X")) { g_BBMParser.Dummies.erase(g_BBMParser.Dummies.begin() + i); i--; g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }
                            ImGui::PopID();
                        }
                    }
                    else if (g_ActiveMeshContent.IsParsed) {
                        if (ImGui::Button("+ Add Dummy##Graph")) {
                            CDummyObject d; d.NameCRC = CalculateFableCRC("NewDummy"); d.BoneIndex = -1;
                            memset(d.Transform, 0, 48); d.Transform[0] = 1; d.Transform[4] = 1; d.Transform[8] = 1;
                            g_ActiveMeshContent.CRCNameMap[d.NameCRC] = "NewDummy";
                            g_ActiveMeshContent.Dummies.push_back(d);
                            g_ActiveMeshContent.DummyObjectCount = (uint16_t)g_ActiveMeshContent.Dummies.size();
                            SortAndRepackGraphExtras();
                            if (saveCallback) saveCallback();
                        }

                        for (int i = 0; i < g_ActiveMeshContent.Dummies.size(); i++) {
                            auto& d = g_ActiveMeshContent.Dummies[i];
                            ImGui::PushID(("Graph_D_" + std::to_string(i)).c_str());

                            std::string sName = GetGraphString(d.NameCRC);
                            char nameBuf[128]; strncpy_s(nameBuf, sName.c_str(), 127);
                            ImGui::SetNextItemWidth(150);
                            if (ImGui::InputText("##Name", nameBuf, 128)) {
                                std::string newStr = nameBuf;
                                d.NameCRC = CalculateFableCRC(newStr);
                                g_ActiveMeshContent.CRCNameMap[d.NameCRC] = newStr;
                            }

                            if (ImGui::IsItemDeactivatedAfterEdit()) {
                                SortAndRepackGraphExtras();
                                if (saveCallback) saveCallback();
                            }

                            ImGui::SameLine(); ImGui::SetNextItemWidth(200);
                            if (ImGui::DragFloat3("##Pos", &d.Transform[9], 0.05f)) { g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }

                            ImGui::SameLine();
                            if (ImGui::Button((d.BoneIndex >= 0 ? "Bone: " + std::to_string(d.BoneIndex) : "No Bone").c_str(), ImVec2(80, 0))) {
                                g_EditingTargetType = 1; g_EditingTargetIndex = i; g_TriggerBonePopup = true;
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("X")) {
                                g_ActiveMeshContent.Dummies.erase(g_ActiveMeshContent.Dummies.begin() + i);
                                g_ActiveMeshContent.DummyObjectCount = (uint16_t)g_ActiveMeshContent.Dummies.size();
                                SortAndRepackGraphExtras();
                                i--; g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback();
                            }
                            ImGui::PopID();
                        }
                    }
                }

                if (!g_BBMParser.IsParsed && g_ActiveMeshContent.IsParsed && ImGui::CollapsingHeader("Generators", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Button("+ Add Generator")) {
                        CMeshGenerator g; g.ObjectName = "NewGenerator"; g.BoneIndex = -1; g.BankIndex = 0; g.UseLocalOrigin = false;
                        memset(g.Transform, 0, 48); g.Transform[0] = 1; g.Transform[4] = 1; g.Transform[8] = 1;
                        g_ActiveMeshContent.Generators.push_back(g);
                        g_ActiveMeshContent.MeshGeneratorCount = (uint16_t)g_ActiveMeshContent.Generators.size();
                        SortAndRepackGraphExtras();
                        if (saveCallback) saveCallback();
                    }

                    for (int i = 0; i < g_ActiveMeshContent.Generators.size(); i++) {
                        auto& gen = g_ActiveMeshContent.Generators[i];
                        ImGui::PushID(("Graph_G_" + std::to_string(i)).c_str());

                        char nameBuf[128]; strncpy_s(nameBuf, gen.ObjectName.c_str(), 127);
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::InputText("##Name", nameBuf, 128)) { gen.ObjectName = nameBuf; }
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            SortAndRepackGraphExtras();
                            if (saveCallback) saveCallback();
                        }

                        ImGui::SameLine(); ImGui::SetNextItemWidth(160);
                        if (ImGui::DragFloat3("##Pos", &gen.Transform[9], 0.05f)) { g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback(); }

                        ImGui::SameLine(); ImGui::SetNextItemWidth(70);
                        int tempBank = (int)gen.BankIndex;
                        if (ImGui::InputInt("##Bank", &tempBank, 0)) {
                            gen.BankIndex = (uint32_t)(std::max)(0, tempBank);
                            if (saveCallback) saveCallback();
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bank Index");

                        ImGui::SameLine();
                        if (ImGui::Button((gen.BoneIndex >= 0 ? "Bone: " + std::to_string(gen.BoneIndex) : "No Bone").c_str(), ImVec2(80, 0))) {
                            g_EditingTargetType = 2; g_EditingTargetIndex = i; g_TriggerBonePopup = true;
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("X")) {
                            g_ActiveMeshContent.Generators.erase(g_ActiveMeshContent.Generators.begin() + i);
                            g_ActiveMeshContent.MeshGeneratorCount = (uint16_t)g_ActiveMeshContent.Generators.size();
                            SortAndRepackGraphExtras();
                            i--; g_MeshUploadNeeded = true; g_PreserveCamera = true; if (saveCallback) saveCallback();
                        }
                        ImGui::PopID();
                    }
                }



                if (g_BBMParser.IsParsed && ImGui::CollapsingHeader("Physics Volumes (Type 3 Only)", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto GenerateBoxPlanes = [](float* center, float* size, float* rot, std::vector<CBBMParser::BBMPlane>& outPlanes) {
                        XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(XMConvertToRadians(rot[0]), XMConvertToRadians(rot[1]), XMConvertToRadians(rot[2]));
                        XMVECTOR right = rotMat.r[0];
                        XMVECTOR up = rotMat.r[1];
                        XMVECTOR fwd = rotMat.r[2];
                        XMVECTOR c = XMVectorSet(center[0], center[1], center[2], 0);

                        outPlanes.resize(6);
                        auto setPlane = [](CBBMParser::BBMPlane& plane, XMVECTOR normal, XMVECTOR point) {
                            plane.Normal[0] = XMVectorGetX(normal);
                            plane.Normal[1] = XMVectorGetY(normal);
                            plane.Normal[2] = XMVectorGetZ(normal);
                            plane.D = -XMVectorGetX(XMVector3Dot(normal, point));
                            };

                        setPlane(outPlanes[0], right, c + right * (size[0] * 0.5f));
                        setPlane(outPlanes[1], -right, c - right * (size[0] * 0.5f));
                        setPlane(outPlanes[2], up, c + up * (size[1] * 0.5f));
                        setPlane(outPlanes[3], -up, c - up * (size[1] * 0.5f));
                        setPlane(outPlanes[4], fwd, c + fwd * (size[2] * 0.5f));
                        setPlane(outPlanes[5], -fwd, c - fwd * (size[2] * 0.5f));
                        };

                    auto DrawVolumeItem = [&](CBBMParser::Volume& v, int i, const std::string& prefix) {
                        std::string vKey = prefix + "_V_" + std::to_string(i);
                        auto& vState = g_VolumeStates[vKey];
                        ImGui::PushID(vKey.c_str());

                        char nameBuf[128]; strncpy_s(nameBuf, v.Name.c_str(), 127);
                        ImGui::SetNextItemWidth(150);
                        if (ImGui::InputText("##Name", nameBuf, 128)) { v.Name = nameBuf; }
                        if (ImGui::IsItemDeactivatedAfterEdit()) { if (saveCallback) saveCallback(); }

                        ImGui::SameLine(); ImGui::SetNextItemWidth(100);
                        int tempID = (int)v.ID;
                        if (ImGui::InputInt("ID", &tempID, 0)) { v.ID = (uint32_t)(std::max)(0, tempID); if (saveCallback) saveCallback(); }

                        ImGui::SameLine();
                        if (ImGui::Button("X##DelVol")) {
                            ImGui::PopID();
                            return true;
                        }

                        ImGui::Indent(15.0f);
                        if (ImGui::Checkbox("Edit as Box Mode", &vState.IsBoxMode)) {
                            if (vState.IsBoxMode && v.Planes.size() >= 6) {
                                float minX = -1e9f, maxX = 1e9f, minY = -1e9f, maxY = 1e9f, minZ = -1e9f, maxZ = 1e9f;
                                for (auto& p : v.Planes) {
                                    if (p.Normal[0] > 0.9f) maxX = -p.D; else if (p.Normal[0] < -0.9f) minX = p.D;
                                    else if (p.Normal[1] > 0.9f) maxY = -p.D; else if (p.Normal[1] < -0.9f) minY = p.D;
                                    else if (p.Normal[2] > 0.9f) maxZ = -p.D; else if (p.Normal[2] < -0.9f) minZ = p.D;
                                }
                                if (minX != -1e9f && maxX != 1e9f) { vState.Center[0] = (minX + maxX) * 0.5f; vState.Size[0] = maxX - minX; }
                                if (minY != -1e9f && maxY != 1e9f) { vState.Center[1] = (minY + maxY) * 0.5f; vState.Size[1] = maxY - minY; }
                                if (minZ != -1e9f && maxZ != 1e9f) { vState.Center[2] = (minZ + maxZ) * 0.5f; vState.Size[2] = maxZ - minZ; }
                                vState.Rotation[0] = 0; vState.Rotation[1] = 0; vState.Rotation[2] = 0;
                            }
                            if (vState.IsBoxMode) {
                                GenerateBoxPlanes(vState.Center, vState.Size, vState.Rotation, v.Planes);
                                v.PlaneCount = (uint32_t)v.Planes.size();
                                if (saveCallback) saveCallback();
                            }
                        }

                        if (vState.IsBoxMode) {
                            bool changed = false;
                            ImGui::SetNextItemWidth(200); if (ImGui::DragFloat3("Center", vState.Center, 0.05f)) changed = true;
                            ImGui::SetNextItemWidth(200); if (ImGui::DragFloat3("Size", vState.Size, 0.05f)) {
                                vState.Size[0] = (std::max)(0.01f, vState.Size[0]);
                                vState.Size[1] = (std::max)(0.01f, vState.Size[1]);
                                vState.Size[2] = (std::max)(0.01f, vState.Size[2]);
                                changed = true;
                            }
                            ImGui::SetNextItemWidth(200); if (ImGui::DragFloat3("Rotation", vState.Rotation, 1.0f)) changed = true;

                            if (changed) {
                                GenerateBoxPlanes(vState.Center, vState.Size, vState.Rotation, v.Planes);
                                v.PlaneCount = (uint32_t)v.Planes.size();
                                g_PreserveCamera = true;
                                if (saveCallback) saveCallback();
                            }
                        }
                        else {
                            if (ImGui::TreeNode(("Raw Planes (" + std::to_string(v.Planes.size()) + ")###PlanesNode").c_str())) {
                                if (ImGui::Button("+ Add Plane")) {
                                    CBBMParser::BBMPlane p = { {0,1,0}, 0 };
                                    v.Planes.push_back(p);
                                    v.PlaneCount = (uint32_t)v.Planes.size();
                                    if (saveCallback) saveCallback();
                                }
                                for (int pIdx = 0; pIdx < v.Planes.size(); pIdx++) {
                                    auto& plane = v.Planes[pIdx];
                                    ImGui::PushID(pIdx);
                                    ImGui::Text("P%d", pIdx);
                                    ImGui::SameLine(); ImGui::SetNextItemWidth(150);
                                    if (ImGui::DragFloat3("Normal", plane.Normal, 0.05f)) { g_PreserveCamera = true; if (saveCallback) saveCallback(); }
                                    ImGui::SameLine(); ImGui::SetNextItemWidth(80);
                                    if (ImGui::DragFloat("Dist(D)", &plane.D, 0.05f)) { g_PreserveCamera = true; if (saveCallback) saveCallback(); }
                                    ImGui::SameLine();
                                    if (ImGui::Button("X")) {
                                        v.Planes.erase(v.Planes.begin() + pIdx); pIdx--;
                                        v.PlaneCount = (uint32_t)v.Planes.size();
                                        if (saveCallback) saveCallback();
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::TreePop();
                            }
                        }
                        ImGui::Unindent(15.0f);
                        ImGui::Separator();
                        ImGui::PopID();
                        return false;
                        };

                    if (ImGui::Button("+ Add Volume")) {
                        CBBMParser::Volume v; v.Name = "NewVolume"; v.ID = 0; v.PlaneCount = 0;
                        g_BBMParser.Volumes.push_back(v);
                        if (saveCallback) saveCallback();
                    }

                    for (int i = 0; i < g_BBMParser.Volumes.size(); i++) {
                        if (DrawVolumeItem(g_BBMParser.Volumes[i], i, "BBM")) {
                            g_BBMParser.Volumes.erase(g_BBMParser.Volumes.begin() + i);
                            i--;
                            if (saveCallback) saveCallback();
                        }
                    }
                }
            ImGui::EndTabItem();
            }

            if (g_TriggerBonePopup) {
                ImGui::OpenPopup("Assign Bone");
                g_ShowBoneSelectPopup = true;
                g_BoneSearchBuf[0] = '\0';
                g_TriggerBonePopup = false;
            }

            if (ImGui::BeginPopupModal("Assign Bone", &g_ShowBoneSelectPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::InputTextWithHint("##BoneSearch", "Search Bones...", g_BoneSearchBuf, 128);
                ImGui::Separator();

                ImGui::BeginChild("BoneList", ImVec2(300, 300), true);

                std::string filter = g_BoneSearchBuf;
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                if (ImGui::Selectable("-1 - Unassign (Root)", false)) {
                    if (g_BBMParser.IsParsed) {
                        if (g_EditingTargetType == 0) g_BBMParser.Helpers[g_EditingTargetIndex].BoneIndex = -1;
                        else if (g_EditingTargetType == 1) g_BBMParser.Dummies[g_EditingTargetIndex].BoneIndex = -1;
                    }
                    else {
                        if (g_EditingTargetType == 0) g_ActiveMeshContent.Helpers[g_EditingTargetIndex].BoneIndex = -1;
                        else if (g_EditingTargetType == 1) g_ActiveMeshContent.Dummies[g_EditingTargetIndex].BoneIndex = -1;
                        else if (g_EditingTargetType == 2) g_ActiveMeshContent.Generators[g_EditingTargetIndex].BoneIndex = -1;
                    }
                    if (saveCallback) saveCallback();
                    g_ShowBoneSelectPopup = false; ImGui::CloseCurrentPopup();
                }

                int boneCount = g_BBMParser.IsParsed ? (int)g_BBMParser.Bones.size() : g_ActiveMeshContent.BoneCount;

                for (int i = 0; i < boneCount; i++) {
                    std::string boneName = g_BBMParser.IsParsed ? g_BBMParser.Bones[i].Name : (i < g_ActiveMeshContent.BoneNames.size() ? g_ActiveMeshContent.BoneNames[i] : "Unknown");
                    std::string lowerName = boneName;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                    if (filter.empty() || lowerName.find(filter) != std::string::npos || std::to_string(i).find(filter) != std::string::npos) {
                        std::string displayStr = std::to_string(i) + " - " + boneName;
                        if (ImGui::Selectable(displayStr.c_str(), false)) {
                            if (g_BBMParser.IsParsed) {
                                if (g_EditingTargetType == 0) g_BBMParser.Helpers[g_EditingTargetIndex].BoneIndex = i;
                                else if (g_EditingTargetType == 1) g_BBMParser.Dummies[g_EditingTargetIndex].BoneIndex = i;
                            }
                            else {
                                if (g_EditingTargetType == 0) g_ActiveMeshContent.Helpers[g_EditingTargetIndex].BoneIndex = i;
                                else if (g_EditingTargetType == 1) g_ActiveMeshContent.Dummies[g_EditingTargetIndex].BoneIndex = i;
                                else if (g_EditingTargetType == 2) g_ActiveMeshContent.Generators[g_EditingTargetIndex].BoneIndex = i;
                            }
                            g_MeshUploadNeeded = true;
                            g_PreserveCamera = true;
                            if (saveCallback) saveCallback();
                            g_ShowBoneSelectPopup = false; ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::Separator();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    g_ShowBoneSelectPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (!g_BBMParser.IsParsed && ImGui::BeginTabItem("Bones")) {
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
                if (g_BBMParser.IsParsed) {
                    auto PackBGRA = [](const ImVec4& c) -> uint32_t {
                        uint32_t r = (uint32_t)(std::clamp(c.x, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t g = (uint32_t)(std::clamp(c.y, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t b = (uint32_t)(std::clamp(c.z, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        uint32_t a = (uint32_t)(std::clamp(c.w, 0.0f, 1.0f) * 255.0f) & 0xFF;
                        return (a << 24) | (r << 16) | (g << 8) | b;
                        };

                    auto DrawBBMTexRow = [&](const char* label, int& mapID, int matIdx, int type) {
                        ImGui::AlignTextToFramePadding(); ImGui::Text("%s", label); ImGui::SameLine(130);
                        ImGui::SetNextItemWidth(80);
                        std::string idStr = "##bbm_id" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::InputInt(idStr.c_str(), &mapID, 0, 0)) {
                            g_MeshUploadNeeded = true;
                            if (saveCallback) saveCallback();
                        }
                        ImGui::SameLine();
                        std::string btnStr = "+##bbm_btn" + std::to_string(type) + "_" + std::to_string(matIdx);
                        if (ImGui::Button(btnStr.c_str(), ImVec2(24, 0))) {
                            g_EditingMaterialIndex = matIdx; g_EditingTextureType = type; g_SelectedTextureID = mapID;
                            g_TextureSearchBuf[0] = '\0'; g_TriggerTexPopup = true;
                        }
                        if (mapID > 0) {
                            ImGui::SameLine(); ID3D11ShaderResourceView* srv = LoadTextureForMesh(mapID);
                            if (srv) {
                                ImGui::Image((void*)srv, ImVec2(24, 24));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::BeginTooltip(); ImGui::Image((void*)srv, ImVec2(256, 256));
                                    ImGui::PushTextWrapPos(256.0f); ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", GetTextureNameForMesh(mapID).c_str());
                                    ImGui::PopTextWrapPos(); ImGui::EndTooltip();
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
                            if (ImGui::InputText("##Name", nameBuf, 128)) { m.Name = nameBuf; if (saveCallback) saveCallback(); }

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
                            if (ImGui::Checkbox("Two-Sided", &m.TwoSided)) { if (saveCallback) saveCallback(); } ImGui::SameLine();
                            if (ImGui::Checkbox("Transparent", &m.Transparent)) { if (saveCallback) saveCallback(); }
                            if (ImGui::Checkbox("Boolean Alpha", &m.BooleanAlpha)) { if (saveCallback) saveCallback(); } ImGui::SameLine();
                            if (ImGui::Checkbox("Degenerate Tris", &m.DegenerateTriangles)) { if (saveCallback) saveCallback(); }

                            ImGui::PopID();
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                }
                else {
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
                                g_MeshUploadNeeded = true;
                                g_PreserveCamera = true;
                                if (saveCallback) saveCallback();
                            }

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
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::EndChild();
    }

    if (g_TriggerTexPopup) {
        ImGui::OpenPopup("Select Texture");
        g_ShowTextureSelectPopup = true;
        g_TriggerTexPopup = false;
    }

    if (ImGui::BeginPopupModal("Select Texture", &g_ShowTextureSelectPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##Search", "Search Textures by ID or Name...", g_TextureSearchBuf, 128);
        ImGui::Separator();

        ImGui::BeginChild("TexList", ImVec2(350, 300), true);

        std::string filter = g_TextureSearchBuf;
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
        bool foundAny = false;

        bool isXboxActive = (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size() && g_OpenBanks[g_ActiveBankIndex].Type == EBankType::XboxGraphics);

        if (isXboxActive) {
            // 1. Local Xbox Textures ONLY
            EnsureXboxTextureCache(g_ActiveBankIndex);
            for (const auto& [id, meta] : g_XboxTexCache[g_ActiveBankIndex]) {
                foundAny = true;
                std::string nameLower = meta.Name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                std::string idStr = std::to_string(id);

                if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                    bool isSelected = (g_SelectedTextureID == id);
                    if (ImGui::Selectable((idStr + " - " + meta.Name + " [Xbox]").c_str(), isSelected)) {
                        g_SelectedTextureID = id;
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
            }
            if (!foundAny) ImGui::TextDisabled("No textures found in this Xbox bank!");
        }
        else {
            // 2. Global PC Textures ONLY
            for (auto& bank : g_OpenBanks) {
                if (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects) {
                    foundAny = true;
                    for (const auto& entry : bank.Entries) {
                        std::string nameLower = entry.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string idStr = std::to_string(entry.ID);

                        if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                            bool isSelected = (g_SelectedTextureID == entry.ID);

                            std::string label = idStr + " - " + entry.Name;
                            if (bank.Type == EBankType::Frontend) label += " [Frontend]";
                            else if (bank.Type == EBankType::Effects) label += " [Effects]";

                            if (ImGui::Selectable(label.c_str(), isSelected)) {
                                g_SelectedTextureID = entry.ID;
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                    }
                }
            }
            if (!foundAny) ImGui::TextDisabled("No Texture Bank open!\nPlease open 'textures.big' in another tab.");
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

    if (g_TriggerPhysicsPopup) {
        ImGui::OpenPopup("Select Physics Mesh");
        g_ShowPhysicsSelectPopup = true;
        g_TriggerPhysicsPopup = false;
    }

    if (ImGui::BeginPopupModal("Select Physics Mesh", &g_ShowPhysicsSelectPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputTextWithHint("##PhysSearch", "Search Physics by ID or Name...", g_PhysicsSearchBuf, 128);
        ImGui::Separator();

        ImGui::BeginChild("PhysList", ImVec2(400, 300), true);

        std::string filter = g_PhysicsSearchBuf;
        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

        for (auto& bank : g_OpenBanks) {
            if (bank.Type == EBankType::Graphics) {
                for (const auto& entry : bank.Entries) {
                    if (entry.Type == 3) {
                        std::string nameLower = entry.Name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string idStr = std::to_string(entry.ID);

                        if (filter.empty() || nameLower.find(filter) != std::string::npos || idStr.find(filter) != std::string::npos) {
                            bool isSelected = (g_SelectedPhysicsID == entry.ID);
                            if (ImGui::Selectable((idStr + " - " + entry.Name).c_str(), isSelected)) {
                                g_SelectedPhysicsID = entry.ID;
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::Button("Choose", ImVec2(120, 0))) {
            if (g_SelectedPhysicsID >= 0) {
                g_ActiveMeshContent.EntryMeta.PhysicsIndex = g_SelectedPhysicsID;
                if (saveCallback) saveCallback();
            }
            g_ShowPhysicsSelectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowPhysicsSelectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}