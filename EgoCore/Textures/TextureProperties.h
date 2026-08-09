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
extern ImTextureID g_DeleteTexture;
extern ImTextureID g_AddTexture;
extern ImTextureID g_ImportTexture;
extern ImTextureID g_ExportTexture;
extern ImTextureID g_ResizeTexture;
extern ImTextureID g_ZoomInTexture;
extern ImTextureID g_ZoomOutTexture;

static const ImVec4 kTexImportIconTint = ImVec4(0.45f, 0.85f, 0.45f, 1.0f);
static const ImVec4 kTexExportIconTint = ImVec4(0.45f, 0.70f, 0.95f, 1.0f);

static int g_SelectedFrame = 0;
static int g_SelectedSlice = 0;
static int g_ViewChannel = 0;
static float g_TexZoom = 1.0f;
static bool g_TexFitToView = false;
static ID3D11ShaderResourceView* g_BackgroundSRV = nullptr;
static bool g_ShouldOpenImportPopup = false;

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
        ImGui::TextColored(
            ImVec4(1, 0, 0, 1),
            "Parse Failed or Empty."
        );
        return;
    }

    if (g_ActiveBankIndex == -1 ||
        g_ActiveBankIndex >= g_OpenBanks.size())
        return;

    auto& bank = g_OpenBanks[g_ActiveBankIndex];

    if (bank.SelectedEntryIndex == -1)
        return;

    auto& entry =
        bank.Entries[bank.SelectedEntryIndex];

    static int lastEntryID = -1;

    if (lastEntryID != entry.ID) {

        lastEntryID = entry.ID;

        g_TexZoom = 1.0f;
        g_TexFitToView = true;

        g_SelectedFrame = 0;
        g_SelectedSlice = 0;

        g_TexViewport.Release();
    }

    bool isVolume =
        (g_TextureParser.Header.Depth > 1);

    bool isFlatSeq =
        (entry.Type == 0x5);

    const int physW =
        g_TextureParser.Header.Width
        ? g_TextureParser.Header.Width
        : g_TextureParser.Header.FrameWidth;

    const int physH =
        g_TextureParser.Header.Height
        ? g_TextureParser.Header.Height
        : g_TextureParser.Header.FrameHeight;

    int logW =
        g_TextureParser.Header.FrameWidth;

    int logH =
        g_TextureParser.Header.FrameHeight;

    int maxFrames =
        (std::max)(
            1,
            (int)g_TextureParser.Header.FrameCount
            );

    if (g_SelectedFrame >= maxFrames)
        g_SelectedFrame = 0;

    if (!isVolume) {

        g_SelectedSlice = 0;
    }
    else {

        int maxDepth =
            (std::max)(
                1,
                (int)g_TextureParser.Header.Depth
                );

        if (g_SelectedSlice >= maxDepth)
            g_SelectedSlice = 0;
    }

    int uploadFrameIdx =
        isFlatSeq
        ? 0
        : g_SelectedFrame;

    g_TexViewport.Update(
        g_pd3dDevice,
        g_TextureParser,
        entry.ID,
        uploadFrameIdx,
        g_SelectedSlice,
        g_ViewChannel
    );

    ImVec2 previewSize =
        ImGui::GetContentRegionAvail();

    if (previewSize.x < 1.0f ||
        previewSize.y < 1.0f)
        return;

    ImGui::BeginChild(
        "TexPreviewFullscreen",
        previewSize,
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    ImVec2 previewMin =
        ImGui::GetWindowPos();

    ImVec2 previewMax(
        previewMin.x +
        ImGui::GetWindowSize().x,
        previewMin.y +
        ImGui::GetWindowSize().y
    );

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    const float bottomBarHeight = 44.0f;
    const float overlayPadding = 10.0f;

    ImVec2 bottomBarMin(
        previewMin.x,
        previewMax.y -
        bottomBarHeight
    );

    ImVec2 bottomBarMax(
        previewMax.x,
        previewMax.y
    );

    ImVec2 viewportSize =
        ImGui::GetWindowSize();

    float textureAreaW =
        viewportSize.x;

    float textureAreaH =
        viewportSize.y -
        bottomBarHeight;

    // ---------- Dynamic max zoom (2.5x fit scale) ----------
    float fitScale = 1.0f;
    if (physW > 0 && physH > 0 && textureAreaW > 0 && textureAreaH > 0) {
        fitScale = (std::min)(textureAreaW / (float)physW, textureAreaH / (float)physH);
    }
    static float g_MaxZoom = 10.0f;
    g_MaxZoom = (std::max)(fitScale * 2.5f, 1.0f);

    // ---------- Compute draw size ----------
    float drawW =
        (float)physW;

    float drawH =
        (float)physH;

    if (g_TexFitToView &&
        physW > 0 &&
        physH > 0) {

        float aspect =
            (float)physH /
            (float)physW;

        drawW =
            textureAreaW;

        drawH =
            drawW * aspect;

        if (drawH > textureAreaH) {

            drawH =
                textureAreaH;

            drawW =
                drawH / aspect;
        }
    }
    else {

        drawW =
            (float)physW *
            g_TexZoom;

        drawH =
            (float)physH *
            g_TexZoom;
    }

    static ImVec2 textureOffset =
        ImVec2(0.0f, 0.0f);

    static int lastPanEntryID = -1;

    if (lastPanEntryID != entry.ID) {

        lastPanEntryID = entry.ID;

        textureOffset =
            ImVec2(0.0f, 0.0f);
    }

    if (g_TexFitToView) {

        textureOffset =
            ImVec2(0.0f, 0.0f);
    }


    bool mouseOverPreview =
        ImGui::IsWindowHovered(
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
            ImGuiHoveredFlags_AllowWhenBlockedByPopup
        );

    bool mouseOverOverview =
        ImGui::IsMouseHoveringRect(
            bottomBarMin,
            bottomBarMax,
            true
        );

    bool mouseOverTexture =
        mouseOverPreview &&
        !mouseOverOverview;

    bool channelPopupOpen =
        ImGui::IsPopupOpen(
            "##TexChannel"
        ) ||
        ImGui::IsPopupOpen(
            nullptr,
            ImGuiPopupFlags_AnyPopupId
        );

    bool uiActive =
        mouseOverPreview ||
        channelPopupOpen;

    if (mouseOverTexture) {

        float wheel =
            ImGui::GetIO().MouseWheel;

        if (wheel != 0.0f) {

            g_TexFitToView =
                false;

            const float zoomStep =
                1.20f;

            if (wheel > 0.0f) {

                for (int i = 0;
                    i < (int)wheel;
                    ++i) {

                    g_TexZoom *=
                        zoomStep;
                }
            }
            else {

                for (int i = 0;
                    i < (int)-wheel;
                    ++i) {

                    g_TexZoom /=
                        zoomStep;
                }
            }

            // Clamp to new max
            g_TexZoom =
                (std::max)(
                    0.05f,
                    (std::min)(
                        g_MaxZoom,
                        g_TexZoom
                        )
                    );

            textureOffset =
                ImVec2(0.0f, 0.0f);
        }
    }

    if (!g_TexFitToView &&
        mouseOverTexture &&
        ImGui::IsMouseDragging(
            ImGuiMouseButton_Left
        )) {

        ImVec2 delta =
            ImGui::GetIO().MouseDelta;

        textureOffset.x +=
            delta.x;

        textureOffset.y +=
            delta.y;
    }

    static float controlsAlpha =
        0.0f;

    float targetAlpha =
        uiActive
        ? 1.0f
        : 0.0f;

    const float fadeSpeed =
        8.0f;

    float deltaTime =
        ImGui::GetIO().DeltaTime;

    if (controlsAlpha < targetAlpha) {

        controlsAlpha =
            (std::min)(
                targetAlpha,
                controlsAlpha +
                fadeSpeed *
                deltaTime
                );
    }
    else if (controlsAlpha > targetAlpha) {

        controlsAlpha =
            (std::max)(
                targetAlpha,
                controlsAlpha -
                fadeSpeed *
                deltaTime
                );
    }

    float overlayAlpha =
        controlsAlpha * 0.92f;

    float baseX =
        previewMin.x +
        (textureAreaW -
            drawW) * 0.5f;

    float baseY =
        previewMin.y +
        (textureAreaH -
            drawH) * 0.5f;

    baseX +=
        textureOffset.x;

    baseY +=
        textureOffset.y;

    ImVec2 texMin(
        baseX,
        baseY
    );

    ImVec2 texMax(
        baseX + drawW,
        baseY + drawH
    );

    ImU32 editorBackground =
        ImGui::GetColorU32(
            ImGuiCol_ChildBg
        );

    drawList->AddRectFilled(
        previewMin,
        previewMax,
        editorBackground
    );

    if (g_BackgroundSRV) {

        drawList->AddImage(
            (void*)g_BackgroundSRV,
            texMin,
            texMax
        );
    }

    if (g_TexViewport.SRV) {

        drawList->AddImage(
            (void*)g_TexViewport.SRV,
            texMin,
            texMax
        );
    }
    else {

        const char* msg =
            "No preview available";

        ImVec2 textSize =
            ImGui::CalcTextSize(msg);

        drawList->AddText(
            ImVec2(
                previewMin.x +
                (textureAreaW -
                    textSize.x) * 0.5f,

                previewMin.y +
                (textureAreaH -
                    textSize.y) * 0.5f
            ),
            IM_COL32(
                180,
                180,
                180,
                255
            ),
            msg
        );
    }

    if (g_TexViewport.SRV &&
        logW > 0 &&
        logH > 0 &&
        (logW != physW ||
            logH != physH)) {

        float sx =
            drawW /
            (float)physW;

        float sy =
            drawH /
            (float)physH;

        float boxX = 0.0f;
        float boxY = 0.0f;

        if (isFlatSeq &&
            physW >= logW) {

            int cols =
                physW / logW;

            if (cols < 1)
                cols = 1;

            int c =
                g_SelectedFrame %
                cols;

            int r =
                g_SelectedFrame /
                cols;

            boxX =
                (float)(c * logW);

            boxY =
                (float)(r * logH);
        }

        drawList->AddRect(
            ImVec2(
                texMin.x +
                boxX * sx,

                texMin.y +
                boxY * sy
            ),

            ImVec2(
                texMin.x +
                (boxX + logW) * sx,

                texMin.y +
                (boxY + logH) * sy
            ),

            IM_COL32(
                0,
                255,
                255,
                220
            ),

            0.0f,
            0,
            2.0f
        );
    }

    // ---------- Bottom Overview bar (always fully visible) ----------
    // Background – constant alpha
    drawList->AddRectFilled(
        bottomBarMin,
        bottomBarMax,
        IM_COL32(8, 8, 8, 205)      // no fade
    );

    // Separator line – constant alpha
    drawList->AddLine(
        ImVec2(bottomBarMin.x, bottomBarMin.y),
        ImVec2(bottomBarMax.x, bottomBarMin.y),
        IM_COL32(95, 95, 95, 150),   // no fade
        1.0f
    );

    // ----- Bar content (no fade) -----
    ImGui::SetCursorScreenPos(
        ImVec2(
            bottomBarMin.x +
            overlayPadding,

            bottomBarMin.y +
            8.0f
        )
    );

    // Do NOT push ImGuiStyleVar_Alpha here – use full opacity

    // Overview
    ImGui::TextColored(
        ImVec4(
            0.95f,
            0.82f,
            0.45f,
            1.0f
        ),
        "Overview"
    );

    ImGui::SameLine();
    ImGui::TextDisabled("|");

    // Resolution
    ImGui::SameLine();
    ImGui::Text(
        "%d x %d",
        physW,
        physH
    );

    ImGui::SameLine();
    ImGui::TextDisabled("|");

    // Format
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        g_TextureParser
        .GetFormatString()
        .c_str()
    );

    ImGui::SameLine();
    ImGui::TextDisabled("|");


    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("Frame Width", &logW)) {
        if (logW > 0) { g_TextureParser.Header.FrameWidth = (uint16_t)logW; g_TexViewport.Release(); }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("Frame Height", &logH)) {
        if (logH > 0) { g_TextureParser.Header.FrameHeight = (uint16_t)logH; g_TexViewport.Release(); }
    }

    // Frame
    if (maxFrames > 1) {

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            180.0f
        );

        ImGui::SliderInt(
            "##TexFrameSelect",
            &g_SelectedFrame,
            0,
            maxFrames - 1,
            "Frame %d"
        );
    }

    if (isVolume) {

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        int maxDepth =
            (std::max)(
                1,
                (int)
                g_TextureParser
                .Header.Depth
                );

        ImGui::SetNextItemWidth(
            140.0f
        );

        ImGui::SliderInt(
            "##TexSliceSelect",
            &g_SelectedSlice,
            0,
            maxDepth - 1,
            "Slice %d"
        );
    }

    // ----- Add / Delete frame buttons (aligned vertically) -----
    const float frameButtonSize =
        26.0f;

    const float frameButtonSpacing =
        16.0f;

    const float frameButtonMargin =
        8.0f;

    float deleteX =
        bottomBarMax.x -
        overlayPadding -
        frameButtonMargin -
        frameButtonSize;

    float addX =
        deleteX -
        frameButtonSpacing -
        frameButtonSize;

    float frameButtonY =
        bottomBarMin.y + 4.0f;

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0, 0, 0, 0)
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(
            0.95f,
            0.82f,
            0.45f,
            0.30f
        )
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(
            0.95f,
            0.82f,
            0.45f,
            0.50f
        )
    );

    ImGui::SetCursorScreenPos(
        ImVec2(
            addX,
            frameButtonY
        )
    );

    if (entry.Type != 0x4 &&
        entry.Type != 0x5) {

        auto TriggerAddFrame = [&]() {
            std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
            if (!path.empty()) {
                g_TexFrameImportPath = path;
                g_IsAddFrame = true;
                g_ImportOptions.Format = g_TextureParser.DecodedFormat;
                g_ImportOptions.IsBumpmap = (g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT5);
                g_ShouldOpenImportPopup = true;
            }
        };

        if (g_AddTexture) {

            if (ImGui::ImageButton(
                "##TexAddFrame",
                g_AddTexture,
                ImVec2(
                    frameButtonSize,
                    frameButtonSize
                ),
                ImVec2(0, 0),
                ImVec2(1, 1),
                ImVec4(0, 0, 0, 0),
                ImVec4(
                    0.9f,
                    0.9f,
                    0.9f,
                    1.0f
                )
            )) {

                TriggerAddFrame();
            }
        }
        else {

            if (ImGui::Button(
                "+##TexAddFrameFallback",
                ImVec2(
                    frameButtonSize,
                    frameButtonSize
                )
            )) {

                TriggerAddFrame();
            }
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Add Frame"
            );

        ImGui::SetCursorScreenPos(
            ImVec2(
                deleteX,
                frameButtonY
            )
        );

        if (maxFrames > 1) {

            if (g_DeleteTexture) {

                if (ImGui::ImageButton(
                    "##TexDelFrame",
                    g_DeleteTexture,
                    ImVec2(
                        frameButtonSize,
                        frameButtonSize
                    ),
                    ImVec2(0, 0),
                    ImVec2(1, 1),
                    ImVec4(0, 0, 0, 0),
                    ImVec4(
                        0.9f,
                        0.9f,
                        0.9f,
                        1.0f
                    )
                )) {

                    DeleteTextureFrame(
                        &bank,
                        bank.SelectedEntryIndex,
                        g_SelectedFrame
                    );

                    g_TexViewport.Release();

                    if (g_SelectedFrame >=
                        maxFrames - 1) {

                        g_SelectedFrame =
                            (std::max)(
                                0,
                                maxFrames - 2
                                );
                    }
                }
            }
            else {

                if (ImGui::Button(
                    "-##TexDelFrameFallback",
                    ImVec2(
                        frameButtonSize,
                        frameButtonSize
                    )
                )) {

                    DeleteTextureFrame(
                        &bank,
                        bank.SelectedEntryIndex,
                        g_SelectedFrame
                    );

                    g_TexViewport.Release();

                    if (g_SelectedFrame >=
                        maxFrames - 1) {

                        g_SelectedFrame =
                            (std::max)(
                                0,
                                maxFrames - 2
                                );
                    }
                }
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "Delete Frame"
                );
        }
    }

    ImGui::PopStyleColor(3);

    // ---------- Upper toolbar (fades) ----------
    const float toolSize =
        27.0f;

    const float toolSpacing =
        5.0f;

    ImGui::SetCursorScreenPos(
        ImVec2(
            previewMin.x +
            overlayPadding,

            bottomBarMin.y -
            toolSize -
            10.0f
        )
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_Alpha,
        overlayAlpha
    );

    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(
            0.03f,
            0.03f,
            0.03f,
            0.75f
        )
    );

    ImGui::SetNextItemWidth(
        100.0f
    );

    const char* viewModes[] = {
        "RGB",
        "Alpha",
        "Red",
        "Green",
        "Blue"
    };

    ImGui::Combo(
        "##TexChannel",
        &g_ViewChannel,
        viewModes,
        IM_ARRAYSIZE(viewModes)
    );

    ImGui::PopStyleColor();

    ImGui::SameLine(
        0.0f,
        toolSpacing
    );

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(
            0,
            0,
            0,
            0.60f
        )
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(
            0.95f,
            0.82f,
            0.45f,
            0.30f
        )
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(
            0.95f,
            0.82f,
            0.45f,
            0.50f
        )
    );

    ImVec4 resizeTint =
        g_TexFitToView
        ? ImVec4(
            0.95f,
            0.82f,
            0.45f,
            1.0f
        )
        : ImVec4(
            0.85f,
            0.85f,
            0.85f,
            1.0f
        );

    if (g_ResizeTexture) {

        if (ImGui::ImageButton(
            "##TexResize",
            g_ResizeTexture,
            ImVec2(
                toolSize,
                toolSize
            ),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            resizeTint
        )) {

            if (g_TexFitToView) {
                g_TexFitToView =
                    false;

                g_TexZoom =
                    1.0f;

                textureOffset =
                    ImVec2(0.0f, 0.0f);
            }
            else {

                // Enable Fit.
                g_TexFitToView =
                    true;

                textureOffset =
                    ImVec2(0.0f, 0.0f);
            }
        }
    }
    else {

        if (ImGui::Button(
            g_TexFitToView
            ? "1:1"
            : "Fit",
            ImVec2(
                42.0f,
                toolSize
            )
        )) {

            if (g_TexFitToView) {

                g_TexFitToView =
                    false;

                g_TexZoom =
                    1.0f;

                textureOffset =
                    ImVec2(0.0f, 0.0f);
            }
            else {

                g_TexFitToView =
                    true;

                textureOffset =
                    ImVec2(0.0f, 0.0f);
            }
        }
    }

    if (ImGui::IsItemHovered()) {

        ImGui::SetTooltip(
            g_TexFitToView
            ? "Return to original resolution"
            : "Fit texture to preview"
        );
    }

    ImGui::SameLine(
        0.0f,
        toolSpacing
    );

    if (g_ZoomOutTexture) {

        if (ImGui::ImageButton(
            "##TexZoomOut",
            g_ZoomOutTexture,
            ImVec2(
                toolSize,
                toolSize
            ),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            ImVec4(
                0.85f,
                0.85f,
                0.85f,
                1.0f
            )
        )) {

            g_TexFitToView =
                false;

            g_TexZoom -=
                g_TexZoom * 0.20f;

            if (g_TexZoom < 0.05f)
                g_TexZoom = 0.05f;

            // Clamp to new max
            g_TexZoom = (std::min)(g_TexZoom, g_MaxZoom);

            textureOffset =
                ImVec2(0.0f, 0.0f);
        }
    }
    else {

        if (ImGui::Button(
            "-##TexZoomOut",
            ImVec2(
                toolSize,
                toolSize
            )
        )) {

            g_TexFitToView =
                false;

            g_TexZoom -=
                g_TexZoom * 0.20f;

            if (g_TexZoom < 0.05f)
                g_TexZoom = 0.05f;

            g_TexZoom = (std::min)(g_TexZoom, g_MaxZoom);

            textureOffset =
                ImVec2(0.0f, 0.0f);
        }
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Zoom Out 20%%"
        );

    ImGui::SameLine(
        0.0f,
        toolSpacing
    );

    if (g_ZoomInTexture) {

        if (ImGui::ImageButton(
            "##TexZoomIn",
            g_ZoomInTexture,
            ImVec2(
                toolSize,
                toolSize
            ),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            ImVec4(
                0.85f,
                0.85f,
                0.85f,
                1.0f
            )
        )) {

            g_TexFitToView =
                false;

            g_TexZoom +=
                g_TexZoom * 0.20f;

            g_TexZoom = (std::min)(g_TexZoom, g_MaxZoom);

            textureOffset =
                ImVec2(0.0f, 0.0f);
        }
    }
    else {

        if (ImGui::Button(
            "+##TexZoomIn",
            ImVec2(
                toolSize,
                toolSize
            )
        )) {

            g_TexFitToView =
                false;

            g_TexZoom +=
                g_TexZoom * 0.20f;

            g_TexZoom = (std::min)(g_TexZoom, g_MaxZoom);

            textureOffset =
                ImVec2(0.0f, 0.0f);
        }
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Zoom In 20%%"
        );

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // ----- Import / Export buttons (fade) -----
    const float importExportSize =
        30.0f;

    const float importExportSpacing =
        12.0f;

    const float importExportMargin =
        6.0f;

    float exportX =
        previewMax.x -
        overlayPadding -
        importExportMargin -
        importExportSize;

    float importX =
        exportX -
        importExportSpacing -
        importExportSize;

    float importExportY =
        bottomBarMin.y -
        importExportSize -
        10.0f;

    ImGui::SetCursorScreenPos(
        ImVec2(
            importX,
            importExportY
        )
    );

    ImGui::PushStyleVar(
        ImGuiStyleVar_Alpha,
        overlayAlpha
    );

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(
            0,
            0,
            0,
            0.60f
        )
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(
            1,
            1,
            1,
            0.12f
        )
    );

    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(
            1,
            1,
            1,
            0.22f
        )
    );

    auto DoImport = [&]() {
        std::string path = OpenFileDialog("Images\0*.png;*.tga;*.jpg;*.bmp\0All Files\0*.*\0");
        if (!path.empty()) {
            g_TexFrameImportPath = path;
            g_IsAddFrame = false;
            g_PendingImportTargetFrame = isFlatSeq ? 0 : g_SelectedFrame;
            g_ImportOptions.IsBumpmap = (g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT1 || g_TextureParser.DecodedFormat == ETextureFormat::NormalMap_DXT5);
            g_ShouldOpenImportPopup = true;
        }
    };

    ImGui::SetCursorScreenPos(
        ImVec2(
            importX,
            importExportY
        )
    );

    if (g_ImportTexture) {

        if (ImGui::ImageButton(
            "##TexImport",
            g_ImportTexture,
            ImVec2(
                importExportSize,
                importExportSize
            ),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            kTexImportIconTint
        )) {

            DoImport();
        }
    }
    else {

        if (ImGui::Button(
            "Import##TexImportFallback",
            ImVec2(
                70.0f,
                importExportSize
            )
        )) {

            DoImport();
        }
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Import"
        );

    auto DoExport = [&]() {

        std::string path =
            SaveFileDialog(
                "PNG Image\0*.png\0"
                "TGA Image\0*.tga\0"
                "DDS Texture\0*.dds\0"
            );

        if (!path.empty()) {

            std::string ext =
                std::filesystem::path(path)
                .extension()
                .string();

            std::transform(
                ext.begin(),
                ext.end(),
                ext.begin(),
                ::tolower
            );

            if (ext.empty()) {

                path += ".png";
                ext = ".png";
            }

            int expIdx =
                isFlatSeq
                ? 0
                : g_SelectedFrame;

            if (ext == ".dds") {

                TextureExporter::ExportDDS(
                    g_TextureParser,
                    path,
                    expIdx
                );
            }
            else if (
                g_TextureParser.IsParsed &&
                !g_TextureParser
                .DecodedPixels
                .empty()
                ) {

                uint32_t stride =
                    g_TextureParser
                    .TrueFrameStride;

                uint32_t frameOffset =
                    stride * expIdx;

                std::vector<Color32> rgba(
                    physW * physH
                );

                if (
                    frameOffset + stride <=
                    g_TextureParser
                    .DecodedPixels
                    .size()
                    ) {

                    const uint8_t* raw =
                        g_TextureParser
                        .DecodedPixels
                        .data() +
                        frameOffset;

                    if (
                        g_TextureParser
                        .DecodedFormat ==
                        ETextureFormat::ARGB8888
                        ) {

                        for (
                            int i = 0;
                            i < physW * physH;
                            i++
                            ) {

                            rgba[i].r =
                                raw[i * 4 + 2];

                            rgba[i].g =
                                raw[i * 4 + 1];

                            rgba[i].b =
                                raw[i * 4 + 0];

                            rgba[i].a =
                                raw[i * 4 + 3];
                        }
                    }
                    else {

                        int blocksX =
                            (physW + 3) / 4;

                        int blocksY =
                            (physH + 3) / 4;

                        int blockSize =
                            (
                                g_TextureParser
                                .DecodedFormat ==
                                ETextureFormat::DXT1 ||

                                g_TextureParser
                                .DecodedFormat ==
                                ETextureFormat::NormalMap_DXT1
                                )
                            ? 8
                            : 16;

                        for (
                            int y = 0;
                            y < blocksY;
                            y++
                            ) {

                            for (
                                int x = 0;
                                x < blocksX;
                                x++
                                ) {

                                Color32 blockOut[16];

                                const uint8_t* blockSrc =
                                    raw +
                                    (
                                        y * blocksX +
                                        x
                                        ) *
                                    blockSize;

                                if (
                                    g_TextureParser
                                    .DecodedFormat ==
                                    ETextureFormat::DXT1 ||

                                    g_TextureParser
                                    .DecodedFormat ==
                                    ETextureFormat::NormalMap_DXT1
                                    ) {

                                    TextureUtils::
                                        DecompressDXT1Block(
                                            blockSrc,
                                            blockOut,
                                            4
                                        );
                                }
                                else if (
                                    g_TextureParser
                                    .DecodedFormat ==
                                    ETextureFormat::DXT3
                                    ) {

                                    TextureUtils::
                                        DecompressDXT3Block(
                                            blockSrc,
                                            blockOut,
                                            4
                                        );
                                }
                                else {

                                    TextureUtils::
                                        DecompressDXT5Block(
                                            blockSrc,
                                            blockOut,
                                            4
                                        );
                                }

                                for (
                                    int py = 0;
                                    py < 4;
                                    py++
                                    ) {

                                    for (
                                        int px = 0;
                                        px < 4;
                                        px++
                                        ) {

                                        int gx =
                                            x * 4 +
                                            px;

                                        int gy =
                                            y * 4 +
                                            py;

                                        if (
                                            gx < physW &&
                                            gy < physH
                                            ) {

                                            rgba[
                                                gy * physW +
                                                    gx
                                            ] =
                                                blockOut[
                                                    py * 4 +
                                                        px
                                                ];
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (ext == ".png") {

                        stbi_write_png(
                            path.c_str(),
                            physW,
                            physH,
                            4,
                            rgba.data(),
                            physW * 4
                        );
                    }
                    else if (ext == ".tga") {

                        stbi_write_tga(
                            path.c_str(),
                            physW,
                            physH,
                            4,
                            rgba.data()
                        );
                    }
                }
            }
        }
        };

    ImGui::SetCursorScreenPos(
        ImVec2(
            exportX,
            importExportY
        )
    );

    if (g_ExportTexture) {

        if (ImGui::ImageButton(
            "##TexExport",
            g_ExportTexture,
            ImVec2(
                importExportSize,
                importExportSize
            ),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            kTexExportIconTint
        )) {

            DoExport();
        }
    }
    else {

        if (ImGui::Button(
            "Export##TexExportFallback",
            ImVec2(
                70.0f,
                importExportSize
            )
        )) {

            DoExport();
        }
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Export"
        );

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGui::EndChild();

    if (g_ShouldOpenImportPopup) {
        ImGui::OpenPopup("Import Options");
        g_ShouldOpenImportPopup = false;
    }

    if (ImGui::BeginPopupModal(
        "Import Options",
        NULL,
        ImGuiWindowFlags_AlwaysAutoResize
    )) {

        ImGui::Text(
            "File: %s",
            std::filesystem::path(
                g_TexFrameImportPath
            )
            .filename()
            .string()
            .c_str()
        );

        ImGui::Separator();

        ImGui::TextColored(
            ImVec4(1, 1, 0, 1),
            "Bumpmap Settings:"
        );

        ImGui::Checkbox(
            "Convert image to a bumpmap.",
            &g_ImportOptions.IsBumpmap
        );

        if (g_ImportOptions.IsBumpmap) {

            ImGui::SliderFloat(
                "Bump Intensity",
                &g_ImportOptions.BumpFactor,
                0.1f,
                20.0f,
                "%.1f"
            );
        }

        ImGui::Dummy(
            ImVec2(0, 10)
        );

        ImGui::TextColored(
            ImVec4(1, 0, 1, 1),
            "Compression Format:"
        );

        static int formatRadio = 1;

        if (
            g_ImportOptions.Format ==
            ETextureFormat::DXT1 ||

            g_ImportOptions.Format ==
            ETextureFormat::NormalMap_DXT1
            ) {

            formatRadio = 0;
        }
        else if (
            g_ImportOptions.Format ==
            ETextureFormat::DXT3
            ) {

            formatRadio = 1;
        }
        else if (
            g_ImportOptions.Format ==
            ETextureFormat::DXT5 ||

            g_ImportOptions.Format ==
            ETextureFormat::NormalMap_DXT5
            ) {

            formatRadio = 2;
        }
        else if (
            g_ImportOptions.Format ==
            ETextureFormat::ARGB8888
            ) {

            formatRadio = 3;
        }

        ImGui::RadioButton(
            "DXT1",
            &formatRadio,
            0
        );

        ImGui::RadioButton(
            "DXT3",
            &formatRadio,
            1
        );

        ImGui::RadioButton(
            "DXT5",
            &formatRadio,
            2
        );

        ImGui::RadioButton(
            "ARGB",
            &formatRadio,
            3
        );

        if (formatRadio == 0) {

            g_ImportOptions.Format =
                g_ImportOptions.IsBumpmap
                ? ETextureFormat::NormalMap_DXT1
                : ETextureFormat::DXT1;
        }
        else if (formatRadio == 1) {

            g_ImportOptions.Format =
                ETextureFormat::DXT3;
        }
        else if (formatRadio == 2) {

            g_ImportOptions.Format =
                g_ImportOptions.IsBumpmap
                ? ETextureFormat::NormalMap_DXT5
                : ETextureFormat::DXT5;
        }
        else if (formatRadio == 3) {

            g_ImportOptions.Format =
                ETextureFormat::ARGB8888;
        }

        ImGui::Separator();

        if (ImGui::Button(
            "Import",
            ImVec2(120, 0)
        )) {

            g_ImportOptions.PreserveFlags =
                g_TextureParser.Header.Flags;

            bool originalIsBump =
                (
                    g_TextureParser
                    .DecodedFormat ==
                    ETextureFormat::NormalMap_DXT1 ||

                    g_TextureParser
                    .DecodedFormat ==
                    ETextureFormat::NormalMap_DXT5
                    );

            if (
                g_ImportOptions.IsBumpmap ==
                originalIsBump
                ) {

                g_ImportOptions
                    .PreservePixelFormatIdx =
                    g_TextureParser
                    .Header
                    .PixelFormatIdx;
            }
            else {

                g_ImportOptions
                    .PreservePixelFormatIdx =
                    0;
            }

            g_ImportOptions.GenerateMipmaps =
                true;

            g_ImportOptions.ForceMipLevels =
                0;

            g_ImportOptions.ResizeToPowerOfTwo =
                true;

            g_ImportOptions.TargetWidth =
                0;

            g_ImportOptions.TargetHeight =
                0;

            if (g_IsAddFrame) {

                AddTextureFrame(
                    &bank,
                    bank.SelectedEntryIndex,
                    g_TexFrameImportPath,
                    g_ImportOptions
                );
            }
            else {

                ReplaceTextureFrame(
                    &bank,
                    bank.SelectedEntryIndex,
                    g_PendingImportTargetFrame,
                    g_TexFrameImportPath,
                    g_ImportOptions
                );
            }

            g_TexViewport.Release();

            g_TexZoom =
                1.0f;

            g_TexFitToView =
                true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();

        ImGui::SameLine();

        if (ImGui::Button(
            "Cancel",
            ImVec2(120, 0)
        )) {

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}