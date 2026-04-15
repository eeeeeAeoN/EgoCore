#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "TextureParser.h"
#include "TextureExporter.h"
#include "ImageBackend.h" 
#include "TextureBuilder.h"
#include <d3d11.h>
#include <string>
#include <vector>

void AddTextureFrame(LoadedBank* bank, int entryIdx, const std::string& filePath, TextureBuilder::ImportOptions opts);
void DeleteTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx);
void RenameTextureEntry(LoadedBank* bank, int entryIdx, const std::string& newName);
void ReplaceTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx, const std::string& filePath, TextureBuilder::ImportOptions opts);

extern ID3D11Device* g_pd3dDevice;

static int g_SelectedFrame = 0;
static int g_SelectedSlice = 0;
static int g_ViewChannel = 0;
static float g_TexZoom = 1.0f;
static ID3D11ShaderResourceView* g_BackgroundSRV = nullptr;

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

        if (!parser.IsParsed || !device) return false;

        uint32_t width = parser.Header.Width ? parser.Header.Width : parser.Header.FrameWidth;
        uint32_t height = parser.Header.Height ? parser.Header.Height : parser.Header.FrameHeight;

        if (width == 0 || height == 0) return false;

        if (parser.IsStagedRaw) {
            if (frameIdx >= parser.RawFrames.size() || parser.RawFrames[frameIdx].empty()) return false;

            const uint8_t* uploadData = parser.RawFrames[frameIdx].data();

            uint32_t sliceSize = width * height * 4;
            if (sliceIdx * sliceSize + sliceSize <= parser.RawFrames[frameIdx].size()) {
                uploadData += (sliceIdx * sliceSize);
            }

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width; desc.Height = height; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            std::vector<Color32> tempPixels;
            if (channelMode != 0) {
                tempPixels.resize(width * height);
                memcpy(tempPixels.data(), uploadData, width * height * 4);
                for (auto& px : tempPixels) {
                    uint8_t val = 0;
                    if (channelMode == 1) val = px.a; else if (channelMode == 2) val = px.r; else if (channelMode == 3) val = px.g; else if (channelMode == 4) val = px.b;
                    px.r = val; px.g = val; px.b = val; px.a = 255;
                }
                uploadData = (const uint8_t*)tempPixels.data();
            }

            D3D11_SUBRESOURCE_DATA subData = {};
            subData.pSysMem = uploadData;
            subData.SysMemPitch = width * 4;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &subData, &tex))) {
                device->CreateShaderResourceView(tex, nullptr, &SRV);
                tex->Release();
            }
            CurrentEntryID = entryID; CurrentFrame = frameIdx; CurrentSlice = sliceIdx; CurrentChannel = channelMode;
            return true;
        }

        if (parser.DecodedPixels.empty()) return false;

        uint32_t singleSliceSize = parser.GetMipSize(width, height, 1);
        size_t finalOffset = ((size_t)parser.TrueFrameStride * frameIdx) + (sliceIdx * singleSliceSize);
        if (finalOffset + singleSliceSize > parser.DecodedPixels.size()) return false;

        const uint8_t* rawData = parser.DecodedPixels.data() + finalOffset;
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
static bool g_IsAddFrame = false;
static std::string g_TexFrameImportPath = "";
static int g_PendingImportTargetFrame = 0;
static TextureBuilder::ImportOptions g_ImportOptions;

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

    static int lastEntryID = -1;
    if (lastEntryID != entry.ID) {
        lastEntryID = entry.ID;
        g_TexZoom = 1.0f;
    }

    ImGui::Separator();

    bool isVolume = (g_TextureParser.Header.Depth > 1);
    bool isFlatSeq = (entry.Type == 0x5);

    int physW = g_TextureParser.Header.Width ? g_TextureParser.Header.Width : g_TextureParser.Header.FrameWidth;
    int physH = g_TextureParser.Header.Height ? g_TextureParser.Header.Height : g_TextureParser.Header.FrameHeight;
    int logW = g_TextureParser.Header.FrameWidth;
    int logH = g_TextureParser.Header.FrameHeight;
    int maxFrames = (std::max)(1, (int)g_TextureParser.Header.FrameCount);

    ImGui::Text("Physical: %dx%d | Format: %s | Frames: %d", physW, physH, g_TextureParser.GetFormatString().c_str(), maxFrames);

    bool dimChanged = false;
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("Frame Width", &logW)) {
        if (logW > 0) { g_TextureParser.Header.FrameWidth = (uint16_t)logW; dimChanged = true; }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("Frame Height", &logH)) {
        if (logH > 0) { g_TextureParser.Header.FrameHeight = (uint16_t)logH; dimChanged = true; }
    }

    ImGui::SetNextItemWidth(100.0f);
    const char* viewModes[] = { "RGB", "Alpha", "Red", "Green", "Blue" };
    ImGui::Combo("##channel", &g_ViewChannel, viewModes, IM_ARRAYSIZE(viewModes));

    if (entry.Type != 0x4 && entry.Type != 0x5) {


        if (entry.Type != 0x4 && entry.Type != 0x5) {
            ImGui::SameLine();
            if (ImGui::Button("Add Frame")) {
                std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
                if (!path.empty()) {
                    g_TexFrameImportPath = path;
                    g_IsAddFrame = true;
                    g_ImportOptions.Format = g_TextureParser.DecodedFormat;
                    g_ImportOptions.IsBumpmap = (g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT5);
                    ImGui::OpenPopup("Import Options");
                }
            }
        }

        if (maxFrames > 1) {
            ImGui::SameLine();
            if (ImGui::Button("Delete Frame")) {
                DeleteTextureFrame(&bank, bank.SelectedEntryIndex, g_SelectedFrame);
                g_TexViewport.Release();
            }
        }
    }

    if (maxFrames > 1) {
        if (g_SelectedFrame >= maxFrames) g_SelectedFrame = 0;
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderInt("Frame Select", &g_SelectedFrame, 0, maxFrames - 1);
    }
    else { g_SelectedFrame = 0; }

    if (isVolume) {
        int maxDepth = (std::max)(1, (int)g_TextureParser.Header.Depth);
        if (g_SelectedSlice >= maxDepth) g_SelectedSlice = 0;
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderInt("Z-Slice", &g_SelectedSlice, 0, maxDepth - 1);
    }

    ImGui::Dummy(ImVec2(0, 5));

    int uploadFrameIdx = isFlatSeq ? 0 : g_SelectedFrame;
    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID, uploadFrameIdx, g_SelectedSlice, g_ViewChannel);

    if (g_TexViewport.SRV) {
        float availW = ImGui::GetContentRegionAvail().x;
        float ratio = (float)physH / (float)physW;
        if (physW == 0) ratio = 1.0f;

        float baseH = (std::min)(510.0f, availW * ratio);
        float baseW = baseH / ratio;

        float w = baseW * g_TexZoom;
        float h = baseH * g_TexZoom;

        ImGuiWindowFlags viewFlags = ImGuiWindowFlags_HorizontalScrollbar;
        if (ImGui::GetIO().KeyCtrl) viewFlags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::BeginChild("TexView", ImVec2(0, baseH + 20), true, viewFlags);

        if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                g_TexZoom += wheel * (g_TexZoom * 0.15f);
                if (g_TexZoom < 0.1f) g_TexZoom = 0.1f;
                if (g_TexZoom > 10.0f) g_TexZoom = 10.0f;
            }
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
        }

        ImVec2 p = ImGui::GetCursorScreenPos();

        if (g_BackgroundSRV) ImGui::GetWindowDrawList()->AddImage((void*)g_BackgroundSRV, p, ImVec2(p.x + w, p.y + h));
        ImGui::GetWindowDrawList()->AddImage((void*)g_TexViewport.SRV, p, ImVec2(p.x + w, p.y + h));

        if (logW > 0 && logH > 0 && (logW != physW || logH != physH)) {
            float sx = w / physW;
            float sy = h / physH;

            float boxX = 0;
            float boxY = 0;

            if (isFlatSeq && physW >= logW) {
                int cols = physW / logW;
                if (cols < 1) cols = 1;

                int c = g_SelectedFrame % cols;
                int r = g_SelectedFrame / cols;

                boxX = c * logW;
                boxY = r * logH;
            }

            ImGui::GetWindowDrawList()->AddRect(
                ImVec2(p.x + boxX * sx, p.y + boxY * sy),
                ImVec2(p.x + (boxX + logW) * sx, p.y + (boxY + logH) * sy),
                0xFF00FFFF, 0.0f, 0, 2.0f);
        }

        ImGui::Dummy(ImVec2(w, h));

        ImGui::EndChild();
    }

    if (ImGui::Button("Export")) {
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
                        for (int i = 0; i < physW * physH; i++) {
                            rgba[i].r = raw[i * 4 + 2];
                            rgba[i].g = raw[i * 4 + 1];
                            rgba[i].b = raw[i * 4 + 0];
                            rgba[i].a = raw[i * 4 + 3];
                        }
                    }
                    else {
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
    if (ImGui::Button("Import")) {
        std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
        if (!path.empty()) {
            g_TexFrameImportPath = path;
            g_IsAddFrame = false;
            g_PendingImportTargetFrame = isFlatSeq ? 0 : g_SelectedFrame;
            g_ImportOptions.IsBumpmap = (g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT5);
            ImGui::OpenPopup("Import Options");
        }
    }

    if (ImGui::BeginPopupModal("Import Options", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File: %s", std::filesystem::path(g_TexFrameImportPath).filename().string().c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Bumpmap Settings:");
        ImGui::Checkbox("Convert image to a bumpmap.", &g_ImportOptions.IsBumpmap);

        if (g_ImportOptions.IsBumpmap) {
            ImGui::SliderFloat("Bump Intensity", &g_ImportOptions.BumpFactor, 0.1f, 20.0f, "%.1f");
        }
        ImGui::Dummy(ImVec2(0, 5));

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextColored(ImVec4(1, 0, 1, 1), "Compression Format:");

        static int formatRadio = 1;
        if (g_ImportOptions.Format == ETextureFormat::DXT1 || g_ImportOptions.Format == ETextureFormat::NormalMap_DXT1) formatRadio = 0;
        else if (g_ImportOptions.Format == ETextureFormat::DXT3) formatRadio = 1;
        else if (g_ImportOptions.Format == ETextureFormat::DXT5 || g_ImportOptions.Format == ETextureFormat::NormalMap_DXT5) formatRadio = 2;
        else if (g_ImportOptions.Format == ETextureFormat::ARGB8888) formatRadio = 3;

        ImGui::RadioButton("DXT1", &formatRadio, 0);
        ImGui::RadioButton("DXT3", &formatRadio, 1);
        ImGui::RadioButton("DXT5", &formatRadio, 2);
        ImGui::RadioButton("ARGB", &formatRadio, 3);

        if (formatRadio == 0) g_ImportOptions.Format = g_ImportOptions.IsBumpmap ? ETextureFormat::NormalMap_DXT1 : ETextureFormat::DXT1;
        else if (formatRadio == 1) g_ImportOptions.Format = ETextureFormat::DXT3;
        else if (formatRadio == 2) g_ImportOptions.Format = g_ImportOptions.IsBumpmap ? ETextureFormat::NormalMap_DXT5 : ETextureFormat::DXT5;
        else if (formatRadio == 3) g_ImportOptions.Format = ETextureFormat::ARGB8888;

        ImGui::Separator();

        if (ImGui::Button("Import", ImVec2(120, 0))) {
            g_ImportOptions.PreserveFlags = g_TextureParser.Header.Flags;

            bool originalIsBump = (g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT5);

            if (g_ImportOptions.IsBumpmap == originalIsBump) {
                g_ImportOptions.PreservePixelFormatIdx = g_TextureParser.Header.PixelFormatIdx;
            }
            else {
                g_ImportOptions.PreservePixelFormatIdx = 0;
            }

            g_ImportOptions.GenerateMipmaps = true;
            g_ImportOptions.ForceMipLevels = 0;
            g_ImportOptions.ResizeToPowerOfTwo = true;
            g_ImportOptions.TargetWidth = 0;
            g_ImportOptions.TargetHeight = 0;

            if (g_IsAddFrame) {
                AddTextureFrame(&bank, bank.SelectedEntryIndex, g_TexFrameImportPath, g_ImportOptions);
            }
            else {
                ReplaceTextureFrame(&bank, bank.SelectedEntryIndex, g_PendingImportTargetFrame, g_TexFrameImportPath, g_ImportOptions);
            }

            g_TexViewport.Release();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}