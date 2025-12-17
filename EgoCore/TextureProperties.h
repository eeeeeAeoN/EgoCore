#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include <d3d11.h> // Required for GPU upload
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

// --- GPU TEXTURE HELPERS ---
struct TextureViewport {
    ID3D11ShaderResourceView* SRV = nullptr;
    uint32_t CurrentID = 0xFFFFFFFF; // Track if we've already uploaded this entry

    void Release() {
        if (SRV) { SRV->Release(); SRV = nullptr; }
        CurrentID = 0xFFFFFFFF;
    }

    bool Update(ID3D11Device* device, const CTextureParser& parser, uint32_t entryID) {
        if (entryID == CurrentID && SRV) return true; // Already up to date
        Release();

        if (!parser.IsParsed || parser.DecodedPixels.empty()) return false;

        DXGI_FORMAT dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        uint32_t blockWidth = 1;

        // Map Fable formats to DXGI formats
        switch (parser.DecodedFormat) {
        case ETextureFormat::DXT1:      dxFormat = DXGI_FORMAT_BC1_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT3:      dxFormat = DXGI_FORMAT_BC2_UNORM; blockWidth = 4; break;
        case ETextureFormat::DXT5:      dxFormat = DXGI_FORMAT_BC3_UNORM; blockWidth = 4; break;
        case ETextureFormat::ARGB8888:  dxFormat = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        default: return false;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = parser.Header.Width;
        desc.Height = parser.Header.Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = dxFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subData = {};
        subData.pSysMem = parser.DecodedPixels.data();

        // Calculate SysMemPitch based on compressed block size or raw pixels
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

        CurrentID = entryID;
        return true;
    }
};

static TextureViewport g_TexViewport;

// --- EXISTING UTILS ---
inline std::string BytesToHexString(const uint8_t* data, size_t size, size_t bytesPerLine = 16) {
    std::stringstream ss;
    for (size_t i = 0; i < size; i++) {
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
        if ((i + 1) % bytesPerLine == 0 && i != size - 1) ss << "\n";
    }
    return ss.str();
}

// --- MAIN DRAW FUNCTION ---
inline void DrawTextureProperties() {
    // Reference the device from main.cpp
    extern ID3D11Device* g_pd3dDevice;

    if (!g_TextureParser.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No texture data available or parse failed.");
        ImGui::TextWrapped("Debug: %s", g_TextureParser.DebugLog.c_str());
        g_TexViewport.Release();
        return;
    }

    // 1. UPDATE VIEWPORT
    const auto& entry = g_CurrentBank.Entries[g_SelectedEntryIndex];
    g_TexViewport.Update(g_pd3dDevice, g_TextureParser, entry.ID);

    // 2. RENDER VIEWPORT
    if (g_TexViewport.SRV) {
        float availWidth = ImGui::GetContentRegionAvail().x;
        float aspect = (float)g_TextureParser.Header.Height / (float)g_TextureParser.Header.Width;
        ImVec2 displaySize = ImVec2(availWidth, availWidth * aspect);

        // Limit height to keep UI usable
        if (displaySize.y > 400) {
            displaySize.y = 400;
            displaySize.x = 400 / aspect;
        }

        ImGui::BeginChild("TextureFrame", ImVec2(0, displaySize.y + 10), true);
        ImGui::Image((void*)g_TexViewport.SRV, displaySize);
        ImGui::EndChild();
    }
    else {
        ImGui::TextDisabled("(Failed to create GPU Texture Preview)");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "--- TEXTURE HEADER DATA ---");

    // Display Basic Dimensions
    ImGui::Text("Width:  %d", g_TextureParser.Header.Width);
    ImGui::Text("Height: %d", g_TextureParser.Header.Height);
    ImGui::Text("Format: %s", g_TextureParser.GetFormatString().c_str());

    ImGui::Separator();

    // Animated Sequence Info
    if (g_TextureParser.Header.FrameCount > 1) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "SEQUENCE DATA");
        ImGui::Text("Frames: %d", g_TextureParser.Header.FrameCount);
        ImGui::Text("Frame Size: %d x %d", g_TextureParser.Header.FrameWidth, g_TextureParser.Header.FrameHeight);
        ImGui::Separator();
    }

    // Hex Dumps
    if (ImGui::CollapsingHeader("Debug: Hex Dumps")) {
        if (ImGui::TreeNode("Metadata Hex Dump")) {
            if (g_SelectedEntryIndex >= 0 && g_SubheaderCache.count(g_SelectedEntryIndex)) {
                const auto& metadata = g_SubheaderCache[g_SelectedEntryIndex];
                std::string hexDump = BytesToHexString(metadata.data(), metadata.size());
                ImGui::BeginChild("MetadataHexScroll", ImVec2(0, 150), true);
                ImGui::TextUnformatted(hexDump.c_str());
                ImGui::EndChild();
            }
            ImGui::TreePop();
        }
    }
}