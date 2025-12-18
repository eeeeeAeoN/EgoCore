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
static int g_ViewChannel = 0; // 0=RGB, 1=Alpha, 2=Red, 3=Green, 4=Blue

static ID3D11ShaderResourceView* g_BackgroundSRV = nullptr;

// [CHANGED] Now creates a flat solid background (R:16, G:16, B:16)
inline void CreateBackgroundTexture() {
    if (g_BackgroundSRV) return;
    const int W = 32, H = 32;
    uint32_t pixelData[W * H];

    // Fill with solid color: R=16(0x10), G=16(0x10), B=16(0x10), A=255(0xFF)
    // Format B8G8R8A8_UNORM -> 0xAARRGGBB in hex
    const uint32_t flatColor = 0xFF101010;

    for (int i = 0; i < W * H; i++) {
        pixelData[i] = flatColor;
    }

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

        uint32_t width = parser.Header.FrameWidth;
        uint32_t height = parser.Header.FrameHeight;

        // Safety: width/height must be > 0
        if (width == 0 || height == 0) return false;

        // Calc Offset
        uint32_t singleSliceSize = parser.GetMipSize(width, height, 1);
        size_t finalOffset = ((size_t)parser.TrueFrameStride * frameIdx) + (sliceIdx * singleSliceSize);
        if (finalOffset + singleSliceSize > parser.DecodedPixels.size()) return false;

        const uint8_t* rawData = parser.DecodedPixels.data() + finalOffset;

        // --- PATH A: Direct Upload (Fast, RGB mode only) ---
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
        // --- PATH B: Software Decode & Swizzle (Channel Viewing) ---
        else {
            std::vector<Color32> rgbaPixels(width * height);

            if (blockWidth == 4) {
                // Decompress DXT to RGBA
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

                        // Copy 4x4 block to main buffer
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
                // Already ARGB8888
                memcpy(rgbaPixels.data(), rawData, width * height * 4);
            }

            // Apply Channel Filter
            for (auto& px : rgbaPixels) {
                uint8_t val = 0;
                switch (channelMode) {
                case 1: val = px.a; break; // Alpha
                case 2: val = px.r; break; // Red
                case 3: val = px.g; break; // Green
                case 4: val = px.b; break; // Blue
                }
                // Convert to Opaque Grayscale
                px.r = val; px.g = val; px.b = val; px.a = 255;
            }

            // Upload RGBA Uncompressed
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Force RGBA
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

inline std::string BytesToHexString(const uint8_t* data, size_t size, size_t bytesPerLine = 16) {
    std::stringstream ss;
    for (size_t i = 0; i < size; i++) {
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
        if ((i + 1) % bytesPerLine == 0 && i != size - 1) ss << "\n";
    }
    return ss.str();
}

inline void DrawTextureProperties() {
    CreateBackgroundTexture();

    if (!g_TextureParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Parse Failed: %s", g_TextureParser.DebugLog.c_str());
        g_TexViewport.Release();
        return;
    }

    const auto& entry = g_CurrentBank.Entries[g_SelectedEntryIndex];
    bool isVolume = (g_TextureParser.Header.Depth > 1);
    bool isBump = (entry.Type == 0x2 || entry.Type == 0x3);

    // --- CONTROLS ROW ---
    ImGui::BeginGroup();

    // 1. Frame Selector
    int maxFrames = (std::max)(1, (int)g_TextureParser.Header.FrameCount);
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

    // 3. Channel Selector (Available for ALL textures now)
    ImGui::PushItemWidth(100);
    const char* viewModes[] = { "RGB", "Alpha", "Red", "Green", "Blue" };
    ImGui::Combo("##channel", &g_ViewChannel, viewModes, IM_ARRAYSIZE(viewModes));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Channel View");
    ImGui::PopItemWidth();

    ImGui::EndGroup();

    // Update Texture 
    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID, g_SelectedFrame, g_SelectedSlice, g_ViewChannel);

    if (g_TexViewport.SRV) {
        float availWidth = ImGui::GetContentRegionAvail().x;
        float aspect = (float)g_TextureParser.Header.FrameHeight / (float)g_TextureParser.Header.FrameWidth;
        float displayH = (std::min)(400.0f, availWidth * aspect);
        float displayW = displayH / aspect;

        ImGui::BeginChild("TexView", ImVec2(0, displayH + 20), true);
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + displayW, p_min.y + displayH);

        // Draw Flat Background (To see transparency in RGB mode)
        // [CHANGED] Variable name updated
        if (g_BackgroundSRV) {
            ImGui::GetWindowDrawList()->AddImage((void*)g_BackgroundSRV, p_min, p_max,
                ImVec2(0, 0), ImVec2(displayW / 32.0f, displayH / 32.0f));
        }

        // Draw Texture
        ImGui::GetWindowDrawList()->AddImage((void*)g_TexViewport.SRV, p_min, p_max);
        ImGui::EndChild();
    }

    // ... [Metadata Text] ...
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "--- TEXTURE DATA ---");
    ImGui::Text("Format: %s", g_TextureParser.GetFormatString().c_str());
    ImGui::Text("Dims: %d x %d x %d", g_TextureParser.Header.FrameWidth, g_TextureParser.Header.FrameHeight, (int)g_TextureParser.Header.Depth);
}