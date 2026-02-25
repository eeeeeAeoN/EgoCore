#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "TextureParser.h"
#include "TextureExporter.h"
#include "ImageBackend.h" 
#include <d3d11.h>
#include <string>
#include <vector>

// Forward Declarations
void AddTextureFrame(LoadedBank * bank, int entryIdx, const std::string & filePath);
void DeleteTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx);
void RenameTextureEntry(LoadedBank* bank, int entryIdx, const std::string& newName);
void ReplaceTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx, const std::string& filePath);

extern ID3D11Device* g_pd3dDevice;

static int g_SelectedFrame = 0;
static int g_SelectedSlice = 0;
static int g_ViewChannel = 0;
static ID3D11ShaderResourceView* g_BackgroundSRV = nullptr;

// [FIX] Ensure this function is present
inline void CreateBackgroundTexture() {
    if (g_BackgroundSRV) return;
    const int W = 32, H = 32;
    uint32_t pixelData[W * H];
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

        uint32_t width = parser.Header.Width ? parser.Header.Width : parser.Header.FrameWidth;
        uint32_t height = parser.Header.Height ? parser.Header.Height : parser.Header.FrameHeight;

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
                uint32_t blocksX = (width + 3) / 4;
                uint32_t blocksY = (height + 3) / 4;
                uint32_t blockSize = (dxFormat == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
                const uint8_t* blockSrc = rawData;

                for (uint32_t y = 0; y < blocksY; y++) {
                    for (uint32_t x = 0; x < blocksX; x++) {
                        Color32 blockOut[16];
                        if (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1)
                            TextureUtils::DecompressDXT1Block(blockSrc, blockOut, 4);
                        else if (fmt == ETextureFormat::DXT3)
                            TextureUtils::DecompressDXT3Block(blockSrc, blockOut, 4);
                        else
                            TextureUtils::DecompressDXT5Block(blockSrc, blockOut, 4);

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
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Parse Failed or Empty.");
        return;
    }

    if (g_ActiveBankIndex == -1 || g_ActiveBankIndex >= g_OpenBanks.size()) return;
    auto& bank = g_OpenBanks[g_ActiveBankIndex];
    if (bank.SelectedEntryIndex == -1) return;
    auto& entry = bank.Entries[bank.SelectedEntryIndex];

    static char nameBuf[128] = "";
    static int lastEntryID = -1;
    if (lastEntryID != entry.ID) {
        strncpy_s(nameBuf, entry.Name.c_str(), 128);
        lastEntryID = entry.ID;
    }

    if (ImGui::InputText("Name", nameBuf, 128)) {
        g_TextureParser.PendingName = nameBuf;
    }

    ImGui::Separator();

    // --- DIMENSIONS ---
    bool isVolume = (g_TextureParser.Header.Depth > 1);
    bool isFlatSeq = (entry.Type == 0x5);

    int physW = g_TextureParser.Header.Width ? g_TextureParser.Header.Width : g_TextureParser.Header.FrameWidth;
    int physH = g_TextureParser.Header.Height ? g_TextureParser.Header.Height : g_TextureParser.Header.FrameHeight;
    int logW = g_TextureParser.Header.FrameWidth;
    int logH = g_TextureParser.Header.FrameHeight;

    ImGui::Text("Physical: %d x %d", physW, physH);

    bool dimChanged = false;
    // [FIX] Update parser header only. Removed the SubheaderCache memcpy autosave.
    if (ImGui::InputInt("Frame Width", &logW)) {
        if (logW > 0) g_TextureParser.Header.FrameWidth = (uint16_t)logW;
    }
    if (ImGui::InputInt("Frame Height", &logH)) {
        if (logH > 0) g_TextureParser.Header.FrameHeight = (uint16_t)logH;
    }

    if (dimChanged && bank.SubheaderCache.count(bank.SelectedEntryIndex)) {
        memcpy(bank.SubheaderCache[bank.SelectedEntryIndex].data(), &g_TextureParser.Header, sizeof(CGraphicHeader));
    }

    ImGui::Separator();

    int maxFrames = (std::max)(1, (int)g_TextureParser.Header.FrameCount);

    ImGui::BeginGroup();
    ImGui::Text("Frames: %d", maxFrames);

    if (maxFrames > 1) {
        if (g_SelectedFrame >= maxFrames) g_SelectedFrame = 0;
        ImGui::SliderInt("##FrameSlider", &g_SelectedFrame, 0, maxFrames - 1);
        ImGui::SameLine();
        // [FIX] Unique Label
        if (ImGui::Button("Delete Frame")) {
            DeleteTextureFrame(&bank, bank.SelectedEntryIndex, g_SelectedFrame);
            g_TexViewport.Release();
        }
    }
    else { g_SelectedFrame = 0; }

    if (ImGui::Button("Add Frame...")) {
        std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
        if (!path.empty()) {
            AddTextureFrame(&bank, bank.SelectedEntryIndex, path);
            g_TexViewport.Release(); // Force Refresh
        }
    }

    if (isVolume) {
        int maxDepth = (std::max)(1, (int)g_TextureParser.Header.Depth);
        if (g_SelectedSlice >= maxDepth) g_SelectedSlice = 0;
        ImGui::SliderInt("Z-Slice", &g_SelectedSlice, 0, maxDepth - 1);
    }

    const char* viewModes[] = { "RGB", "Alpha", "Red", "Green", "Blue" };
    ImGui::Combo("##channel", &g_ViewChannel, viewModes, IM_ARRAYSIZE(viewModes));
    ImGui::EndGroup();

    // --- VIEWPORT ---
    int uploadFrameIdx = isFlatSeq ? 0 : g_SelectedFrame;
    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID, uploadFrameIdx, g_SelectedSlice, g_ViewChannel);

    if (g_TexViewport.SRV) {
        float availW = ImGui::GetContentRegionAvail().x;
        float ratio = (float)physH / (float)physW;
        if (physW == 0) ratio = 1.0f;
        float h = (std::min)(400.0f, availW * ratio);
        float w = h / ratio;

        ImGui::BeginChild("TexView", ImVec2(0, h + 20), true);
        ImVec2 p = ImGui::GetCursorScreenPos();

        if (g_BackgroundSRV) ImGui::GetWindowDrawList()->AddImage((void*)g_BackgroundSRV, p, ImVec2(p.x + w, p.y + h));
        ImGui::GetWindowDrawList()->AddImage((void*)g_TexViewport.SRV, p, ImVec2(p.x + w, p.y + h));

        if (logW > 0 && logH > 0 && (logW != physW || logH != physH)) {
            float sx = w / physW;
            float sy = h / physH;
            ImGui::GetWindowDrawList()->AddRect(p, ImVec2(p.x + logW * sx, p.y + logH * sy), 0xFF00FFFF, 0.0f, 0, 2.0f);
        }
        ImGui::EndChild();
    }

    // --- EXPORT / REPLACE ---
    if (ImGui::Button("Export Frame...")) {
        std::string path = SaveFileDialog("PNG Image\0*.png\0TGA Image\0*.tga\0DDS Texture\0*.dds\0");
        if (!path.empty()) {
            std::string ext = std::filesystem::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext.empty()) { path += ".png"; ext = ".png"; }

            int expIdx = isFlatSeq ? 0 : g_SelectedFrame;

            if (ext == ".dds") {
                TextureExporter::ExportDDS(g_TextureParser, path, expIdx);
            }
            else if (g_TextureParser.IsParsed && !g_TextureParser.DecodedPixels.empty()) {
                uint32_t stride = g_TextureParser.TrueFrameStride;
                uint32_t frameOffset = stride * expIdx;
                std::vector<Color32> rgba(physW * physH);

                if (frameOffset + stride <= g_TextureParser.DecodedPixels.size()) {
                    const uint8_t* raw = g_TextureParser.DecodedPixels.data() + frameOffset;
                    if (g_TextureParser.DecodedFormat == ETextureFormat::ARGB8888) {
                        memcpy(rgba.data(), raw, physW * physH * 4);
                    }
                    else {
                        // Decompress
                        int blocksX = (physW + 3) / 4;
                        int blocksY = (physH + 3) / 4;
                        int blockSize = (g_TextureParser.DecodedFormat == ETextureFormat::DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1) ? 8 : 16;
                        for (int y = 0; y < blocksY; y++) {
                            for (int x = 0; x < blocksX; x++) {
                                Color32 blockOut[16];
                                const uint8_t* blockSrc = raw + (y * blocksX + x) * blockSize;
                                if (g_TextureParser.DecodedFormat == ETextureFormat::DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1)
                                    TextureUtils::DecompressDXT1Block(blockSrc, blockOut, 4);
                                else if (g_TextureParser.DecodedFormat == ETextureFormat::DXT3)
                                    TextureUtils::DecompressDXT3Block(blockSrc, blockOut, 4);
                                else
                                    TextureUtils::DecompressDXT5Block(blockSrc, blockOut, 4);
                                for (int py = 0; py < 4; py++) {
                                    for (int px = 0; px < 4; px++) {
                                        if (x * 4 + px < physW && y * 4 + py < physH) rgba[(y * 4 + py) * physW + (x * 4 + px)] = blockOut[py * 4 + px];
                                    }
                                }
                            }
                        }
                    }
                    if (ext == ".png") stbi_write_png(path.c_str(), physW, physH, 4, rgba.data(), physW * 4);
                    else if (ext == ".tga") stbi_write_tga(path.c_str(), physW, physH, 4, rgba.data());
                }
            }
        }
    }

    ImGui::SameLine();

    // Replace Frame + Refresh
    if (ImGui::Button("Replace Frame...")) {
        std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
        if (!path.empty()) {
            int targetFrame = isFlatSeq ? 0 : g_SelectedFrame;
            ReplaceTextureFrame(&bank, bank.SelectedEntryIndex, targetFrame, path);
            g_TexViewport.Release();
        }
    }

    ImGui::Separator();
    ImGui::Text("Format: %s", g_TextureParser.GetFormatString().c_str());
}