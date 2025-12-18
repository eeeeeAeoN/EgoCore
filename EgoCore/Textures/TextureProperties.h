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

static int g_SelectedFrame = 0;

struct TextureViewport {
    ID3D11ShaderResourceView* SRV = nullptr;
    uint32_t CurrentEntryID = 0xFFFFFFFF;
    int CurrentFrame = -1;

    void Release() {
        if (SRV) { SRV->Release(); SRV = nullptr; }
        CurrentEntryID = 0xFFFFFFFF;
        CurrentFrame = -1;
    }

    bool Update(ID3D11Device* device, const CTextureParser& parser, uint32_t entryID, int frameIdx) {
        if (entryID == CurrentEntryID && frameIdx == CurrentFrame && SRV) return true;
        Release();

        if (!parser.IsParsed || parser.DecodedPixels.empty() || !device) return false;

        DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t blockWidth = 1;

        switch (parser.DecodedFormat) {
        case ETextureFormat::DXT1:      dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT3:      dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT5:      dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
        case ETextureFormat::NormalMap: dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
        case ETextureFormat::ARGB8888:  dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        default: return false;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = parser.Header.FrameWidth;
        desc.Height = parser.Header.FrameHeight;
        desc.MipLevels = 1; // We only upload the top mip for visualization
        desc.ArraySize = 1;
        desc.Format = dxFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Calculate offset using the TrueFrameStride (Total size of Frame + All Mips)
        size_t frameOffset = (size_t)parser.TrueFrameStride * frameIdx;

        // Safety check
        if (frameOffset + parser.Header.FrameDataSize > parser.DecodedPixels.size()) return false;

        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = parser.DecodedPixels.data() + frameOffset;

        if (blockWidth == 4) {
            uint32_t blocksPerRow = (desc.Width + 3) / 4;
            subData.SysMemPitch = blocksPerRow * (parser.DecodedFormat == ETextureFormat::DXT1 ? 8 : 16);
        }
        else {
            subData.SysMemPitch = desc.Width * 4;
        }

        ID3D11Texture2D* tex = nullptr;
        if (FAILED(device->CreateTexture2D(&desc, &subData, &tex))) return false;
        device->CreateShaderResourceView(tex, nullptr, &SRV);
        tex->Release();

        CurrentEntryID = entryID;
        CurrentFrame = frameIdx;
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
    if (!g_TextureParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Parse Failed: %s", g_TextureParser.DebugLog.c_str());
        g_TexViewport.Release();
        return;
    }

    const auto& entry = g_CurrentBank.Entries[g_SelectedEntryIndex];

    int maxFrames = (std::max)(1, (int)g_TextureParser.Header.FrameCount);
    if (maxFrames > 1) {
        if (g_SelectedFrame >= maxFrames) g_SelectedFrame = 0;
        ImGui::SliderInt("Frame Selector", &g_SelectedFrame, 0, maxFrames - 1);
        ImGui::Text("Frame Dimensions: %d x %d", g_TextureParser.Header.FrameWidth, g_TextureParser.Header.FrameHeight);
    }
    else {
        g_SelectedFrame = 0;
    }

    //Export button
    if (ImGui::Button("Export Current Frame (.DDS)")) {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        std::string defaultName = entry.Name + "_Frame" + std::to_string(g_SelectedFrame) + ".dds";
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
            bool success = TextureExporter::ExportDDS(g_TextureParser, ofn.lpstrFile, g_SelectedFrame);
            if (!success) MessageBoxA(NULL, "Export Failed", "Error", MB_OK | MB_ICONERROR);
        }
    }

    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID, g_SelectedFrame);

    if (g_TexViewport.SRV) {
        float availWidth = ImGui::GetContentRegionAvail().x;
        float aspect = (float)g_TextureParser.Header.FrameHeight / (float)g_TextureParser.Header.FrameWidth;
        float displayH = (std::min)(400.0f, availWidth * aspect);
        float displayW = displayH / aspect;

        ImGui::BeginChild("TexView", ImVec2(0, displayH + 20), true);
        ImGui::Image((void*)g_TexViewport.SRV, ImVec2(displayW, displayH));
        ImGui::EndChild();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "--- TEXTURE HEADER DATA ---");
    ImGui::Text("Internal Format: %s", g_TextureParser.GetFormatString().c_str());
    ImGui::Text("Sequence Count: %d", g_TextureParser.Header.FrameCount);
    ImGui::Text("Mips in Bank: %d", g_TextureParser.Header.MipmapLevels);

    if (ImGui::CollapsingHeader("Raw Metadata (Hex)")) {
        if (g_SelectedEntryIndex >= 0 && g_SubheaderCache.count(g_SelectedEntryIndex)) {
            const auto& metadata = g_SubheaderCache[g_SelectedEntryIndex];
            std::string hex = BytesToHexString(metadata.data(), metadata.size());
            ImGui::BeginChild("MetaHex", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(hex.c_str());
            ImGui::EndChild();
        }
    }
}