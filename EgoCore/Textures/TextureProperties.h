#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "TextureParser.h"
#include "TextureExporter.h"
#include <d3d11.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

extern ID3D11Device* g_pd3dDevice;

// UI States
static int g_SelectedFrame = 0;
static int g_SelectedSlice = 0;
static int g_ViewChannel = 0;
static ID3D11ShaderResourceView* g_BackgroundSRV = nullptr;

inline void CreateBackgroundTexture() {
    if (g_BackgroundSRV) return;
    const int W = 32, H = 32;
    uint32_t pixelData[W * H];
    // Solid Dark Grey Background (R:16, G:16, B:16)
    const uint32_t flatColor = 0xFF101010;
    for (int i = 0; i < W * H; i++) pixelData[i] = flatColor;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = W; desc.Height = H; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixelData; initData.SysMemPitch = W * 4;

    ID3D11Texture2D* tex = nullptr;
    if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &initData, &tex))) {
        g_pd3dDevice->CreateShaderResourceView(tex, nullptr, &g_BackgroundSRV);
        tex->Release();
    }
}

// --- DXT DECOMPRESSION HELPERS (For Channel Viewing) ---
struct Color32 { uint8_t r, g, b, a; };

inline void GetColorBlockColors(Color32* colors, const uint8_t* block) {
    uint16_t c0 = *(uint16_t*)(block);
    uint16_t c1 = *(uint16_t*)(block + 2);

    colors[0].r = (c0 >> 11) * 255 / 31;
    colors[0].g = ((c0 >> 5) & 0x3F) * 255 / 63;
    colors[0].b = (c0 & 0x1F) * 255 / 31;
    colors[0].a = 255;

    colors[1].r = (c1 >> 11) * 255 / 31;
    colors[1].g = ((c1 >> 5) & 0x3F) * 255 / 63;
    colors[1].b = (c1 & 0x1F) * 255 / 31;
    colors[1].a = 255;

    if (c0 > c1) {
        colors[2].r = (2 * colors[0].r + colors[1].r) / 3;
        colors[2].g = (2 * colors[0].g + colors[1].g) / 3;
        colors[2].b = (2 * colors[0].b + colors[1].b) / 3;
        colors[2].a = 255;

        colors[3].r = (colors[0].r + 2 * colors[1].r) / 3;
        colors[3].g = (colors[0].g + 2 * colors[1].g) / 3;
        colors[3].b = (colors[0].b + 2 * colors[1].b) / 3;
        colors[3].a = 255;
    }
    else {
        colors[2].r = (colors[0].r + colors[1].r) / 2;
        colors[2].g = (colors[0].g + colors[1].g) / 2;
        colors[2].b = (colors[0].b + colors[1].b) / 2;
        colors[2].a = 255;

        colors[3].r = 0; colors[3].g = 0; colors[3].b = 0; colors[3].a = 0;
    }
}

inline void DecompressDXT1Block(const uint8_t* block, Color32* output, uint32_t stride) {
    Color32 colors[4];
    GetColorBlockColors(colors, block);
    uint32_t indices = *(uint32_t*)(block + 4);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            uint8_t idx = (indices >> (2 * (4 * y + x))) & 0x03;
            output[y * stride + x] = colors[idx];
        }
    }
}

inline void DecompressDXT3Block(const uint8_t* block, Color32* output, uint32_t stride) {
    DecompressDXT1Block(block + 8, output, stride);
    for (int y = 0; y < 4; y++) {
        uint16_t alphaRow = *(uint16_t*)(block + 2 * y);
        for (int x = 0; x < 4; x++) {
            uint8_t alpha = (alphaRow >> (4 * x)) & 0x0F;
            output[y * stride + x].a = (alpha * 255) / 15;
        }
    }
}

inline void DecompressDXT5Block(const uint8_t* block, Color32* output, uint32_t stride) {
    DecompressDXT1Block(block + 8, output, stride);
    uint8_t a0 = block[0];
    uint8_t a1 = block[1];
    uint64_t alphaMask = (*(uint64_t*)(block)) >> 16;

    uint8_t alphas[8];
    alphas[0] = a0;
    alphas[1] = a1;
    if (a0 > a1) {
        for (int i = 2; i < 8; i++) alphas[i] = ((8 - i) * a0 + (i - 1) * a1) / 7;
    }
    else {
        for (int i = 2; i < 6; i++) alphas[i] = ((6 - i) * a0 + (i - 1) * a1) / 5;
        alphas[6] = 0;
        alphas[7] = 255;
    }

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            uint8_t idx = (uint8_t)((alphaMask >> (3 * (4 * y + x))) & 0x07);
            output[y * stride + x].a = alphas[idx];
        }
    }
}
// ----------------------------------------------------

struct TextureViewport {
    ID3D11ShaderResourceView* SRV = nullptr;
    uint32_t CurrentEntryID = 0xFFFFFFFF;
    int CurrentFrame = -1;
    int CurrentSlice = -1;
    int CurrentChannel = -1;

    void Release() {
        if (SRV) { SRV->Release(); SRV = nullptr; }
        CurrentEntryID = 0xFFFFFFFF;
        CurrentFrame = -1;
        CurrentSlice = -1;
        CurrentChannel = -1;
    }

    bool Update(ID3D11Device* device, CTextureParser& parser, uint32_t entryID, int frameIdx, int sliceIdx, int channelMode) {
        if (entryID == CurrentEntryID && frameIdx == CurrentFrame && sliceIdx == CurrentSlice && channelMode == CurrentChannel && SRV) return true;
        Release();

        if (!parser.IsParsed || parser.DecodedPixels.empty() || !device) return false;

        bool needsSoftwareDecode = (channelMode != 0);

        DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t blockWidth = 1;
        ETextureFormat fmt = parser.DecodedFormat;

        switch (fmt) {
        case ETextureFormat::DXT1:
        case ETextureFormat::NormalMap_DXT1: dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT3:      dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT5:
        case ETextureFormat::NormalMap_DXT5: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
        case ETextureFormat::ARGB8888:  dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        default: return false;
        }

        uint32_t width = parser.Header.Width;
        uint32_t height = parser.Header.Height;
        if (width == 0) width = parser.Header.FrameWidth;
        if (height == 0) height = parser.Header.FrameHeight;

        if (width == 0 || height == 0) return false;

        uint32_t singleSliceSize = parser.GetMipSize(width, height, 1);
        size_t finalOffset = ((size_t)parser.TrueFrameStride * frameIdx) + (sliceIdx * singleSliceSize);

        if (finalOffset + singleSliceSize > parser.DecodedPixels.size()) return false;

        const uint8_t* rawData = parser.DecodedPixels.data() + finalOffset;

        if (!needsSoftwareDecode) {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = dxFormat; desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subData = {};
            subData.pSysMem = rawData;

            if (blockWidth == 4) subData.SysMemPitch = ((width + 3) / 4) * ((dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16);
            else subData.SysMemPitch = width * 4;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &subData, &tex))) {
                device->CreateShaderResourceView(tex, nullptr, &SRV);
                tex->Release();
            }
        }
        else {
            std::vector<Color32> rgbaPixels(width * height);

            if (blockWidth == 4) {
                // Decompress DXT
                uint32_t blocksX = (width + 3) / 4;
                uint32_t blocksY = (height + 3) / 4;
                uint32_t blockSize = (dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
                const uint8_t* blockSrc = rawData;

                for (uint32_t y = 0; y < blocksY; y++) {
                    for (uint32_t x = 0; x < blocksX; x++) {
                        Color32 blockOut[16];
                        if (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1)
                            DecompressDXT1Block(blockSrc, blockOut, 4);
                        else if (fmt == ETextureFormat::DXT3)
                            DecompressDXT3Block(blockSrc, blockOut, 4);
                        else
                            DecompressDXT5Block(blockSrc, blockOut, 4);

                        for (int py = 0; py < 4; py++) {
                            for (int px = 0; px < 4; px++) {
                                uint32_t globalX = x * 4 + px;
                                uint32_t globalY = y * 4 + py;
                                if (globalX < width && globalY < height) {
                                    rgbaPixels[globalY * width + globalX] = blockOut[py * 4 + px];
                                }
                            }
                        }
                        blockSrc += blockSize;
                    }
                }
            }
            else {
                memcpy(rgbaPixels.data(), rawData, width * height * 4);
            }

            for (auto& px : rgbaPixels) {
                uint8_t val = 0;
                switch (channelMode) {
                case 1: val = px.a; break;
                case 2: val = px.r; break;
                case 3: val = px.g; break;
                case 4: val = px.b; break;
                }
                px.r = val; px.g = val; px.b = val; px.a = 255;
            }

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subData = {};
            subData.pSysMem = rgbaPixels.data();
            subData.SysMemPitch = width * 4;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &subData, &tex))) {
                device->CreateShaderResourceView(tex, nullptr, &SRV);
                tex->Release();
            }
        }

        CurrentEntryID = entryID;
        CurrentFrame = frameIdx;
        CurrentSlice = sliceIdx;
        CurrentChannel = channelMode;
        return true;
    }
};

static TextureViewport g_TexViewport;

inline void DrawTextureProperties() {
    CreateBackgroundTexture();

    if (!g_TextureParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Parse Failed: %s", g_TextureParser.DebugLog.c_str());
        g_TexViewport.Release();
        return;
    }

    const auto& entry = g_CurrentBank.Entries[g_SelectedEntryIndex];
    bool isVolume = (g_TextureParser.Header.Depth > 1);
    bool isFlatSeq = (entry.Type == 0x5);

    int physW = g_TextureParser.Header.Width;
    int physH = g_TextureParser.Header.Height;
    if (physW == 0) physW = g_TextureParser.Header.FrameWidth;
    if (physH == 0) physH = g_TextureParser.Header.FrameHeight;

    int logW = g_TextureParser.Header.FrameWidth;
    int logH = g_TextureParser.Header.FrameHeight;
    if (logW == 0) logW = physW;
    if (logH == 0) logH = physH;

    int cols = (logW > 0) ? physW / logW : 1;
    int rows = (logH > 0) ? physH / logH : 1;
    int flatFrameCount = cols * rows;

    ImGui::BeginGroup();

    // 1. Frame Selector
    int maxFrames = (std::max)(1, (int)g_TextureParser.Header.FrameCount);
    if (isFlatSeq) maxFrames = flatFrameCount;

    if (maxFrames > 1) {
        ImGui::PushItemWidth(100);
        if (g_SelectedFrame >= maxFrames) g_SelectedFrame = 0;
        ImGui::SliderInt("Frame", &g_SelectedFrame, 0, maxFrames - 1);
        ImGui::PopItemWidth();
        ImGui::SameLine();
    }
    else { g_SelectedFrame = 0; }

    // 2. Depth Slice Selector
    if (isVolume) {
        int maxDepth = (std::max)(1, (int)g_TextureParser.Header.Depth);
        ImGui::PushItemWidth(100);
        if (g_SelectedSlice >= maxDepth) g_SelectedSlice = 0;
        ImGui::SliderInt("Z-Slice", &g_SelectedSlice, 0, maxDepth - 1);
        ImGui::PopItemWidth();
        ImGui::SameLine();
    }
    else { g_SelectedSlice = 0; }

    // 3. Channel Selector
    ImGui::PushItemWidth(100);
    const char* viewModes[] = { "RGB", "Alpha", "Red", "Green", "Blue" };
    ImGui::Combo("##channel", &g_ViewChannel, viewModes, IM_ARRAYSIZE(viewModes));
    ImGui::PopItemWidth();

    ImGui::EndGroup();

    int uploadFrameIdx = isFlatSeq ? 0 : g_SelectedFrame;
    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID, uploadFrameIdx, g_SelectedSlice, g_ViewChannel);

    if (g_TexViewport.SRV) {
        float availWidth = ImGui::GetContentRegionAvail().x;

        ImVec2 uv0(0, 0);
        ImVec2 uv1(1, 1);
        float displayAspect = (float)physH / (float)physW;

        if (isFlatSeq && cols > 0 && rows > 0) {
            int c = g_SelectedFrame % cols;
            int r = g_SelectedFrame / cols;
            uv0.x = (float)(c * logW) / (float)physW;
            uv0.y = (float)(r * logH) / (float)physH;
            uv1.x = (float)((c + 1) * logW) / (float)physW;
            uv1.y = (float)((r + 1) * logH) / (float)physH;
            displayAspect = (float)logH / (float)logW;
        }

        float displayH = (std::min)(400.0f, availWidth * displayAspect);
        float displayW = displayH / displayAspect;

        ImGui::BeginChild("TexView", ImVec2(0, displayH + 20), true);
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + displayW, p_min.y + displayH);

        if (g_BackgroundSRV) {
            ImGui::GetWindowDrawList()->AddImage((void*)g_BackgroundSRV, p_min, p_max, ImVec2(0, 0), ImVec2(1, 1));
        }

        ImGui::GetWindowDrawList()->AddImage((void*)g_TexViewport.SRV, p_min, p_max, uv0, uv1);

        // [FIXED] Red Rectangle Logic
        // Draw if NOT a Flat Sequence (Type 5 handles cropping via UVs)
        // AND if logical dimensions differ from physical dimensions (Width OR Height)
        bool hasPadding = (logW < physW) || (logH < physH);

        if (!isFlatSeq && hasPadding) {
            float scaleX = displayW / physW;
            float scaleY = displayH / physH;
            float boxW = logW * scaleX;
            float boxH = logH * scaleY;
            ImGui::GetWindowDrawList()->AddRect(
                p_min,
                ImVec2(p_min.x + boxW, p_min.y + boxH),
                0xFF0000FF // Red
            );
        }

        ImGui::EndChild();
    }

    // Export Button (Hidden for Volumes)
    if (!isVolume) {
        if (ImGui::Button("Export Frame (.DDS)")) {
            OPENFILENAMEA ofn;
            char szFile[260] = { 0 };
            std::string defaultName = entry.Name + "_F" + std::to_string(g_SelectedFrame) + ".dds";
            strcpy_s(szFile, defaultName.c_str());

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "DirectDraw Surface\0*.dds\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

            if (GetSaveFileNameA(&ofn) == TRUE) {
                int exportIdx = isFlatSeq ? 0 : g_SelectedFrame;
                bool success = TextureExporter::ExportDDS(g_TextureParser, ofn.lpstrFile, exportIdx);
                if (!success) MessageBoxA(NULL, "Export Failed", "Error", MB_OK | MB_ICONERROR);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Format: %s", g_TextureParser.GetFormatString().c_str());
    ImGui::Text("Phys: %d x %d", physW, physH);
    ImGui::Text("Log:  %d x %d", logW, logH);
    if (isFlatSeq) ImGui::Text("Grid: %d x %d", cols, rows);
}