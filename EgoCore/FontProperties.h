#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "FontParser.h"
#include "FontBuilder.h"
#include "ConfigBackend.h" // Fixes g_AppConfig
#include "ImageBackend.h"  // Fixes stb_image
#include <d3d11.h>         // Fixes ID3D11ShaderResourceView
#include <fstream>

extern ID3D11Device* g_pd3dDevice;

// Inlined DirectX 11 Texture Loader (No separate TextureLoader.h needed)
inline bool LoadTextureFromMemory(const std::vector<uint8_t>& rawData, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    if (rawData.empty()) return false;

    int image_width = 0;
    int image_height = 0;
    int image_channels = 0;
    unsigned char* image_data = stbi_load_from_memory(rawData.data(), (int)rawData.size(), &image_width, &image_height, &image_channels, 4);

    if (image_data == nullptr) return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    stbi_image_free(image_data);

    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Global parser instance
inline CFontParser g_FontParser;
inline int g_LastFontEntryID = -1;

// Global state for the active font texture
inline ID3D11ShaderResourceView* g_FontAtlasSRV = nullptr;
inline int g_FontAtlasWidth = 0;
inline int g_FontAtlasHeight = 0;

inline void DrawFontProperties(int currentEntryID) {
    if (!g_FontParser.IsParsed) {
        ImGui::TextDisabled("No font data available or parsed.");
        return;
    }

    // --- THE STREAMING WARNING ---
    if (g_FontParser.IsStreaming) {
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Streaming Font Detected");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));
        ImGui::TextWrapped("This font uses Lionhead's proprietary dynamic LZO glyph streaming architecture.");
        ImGui::TextDisabled("Streaming fonts are currently not supported for editing or viewing.");
        return;
    }
    // -----------------------------

    const auto& data = g_FontParser.Data;

    // --- Load the texture ONLY when the user selects a new font ---
    if (g_LastFontEntryID != currentEntryID) {
        // Clean up the old texture from the GPU
        if (g_FontAtlasSRV) {
            g_FontAtlasSRV->Release();
            g_FontAtlasSRV = nullptr;
        }

        // Upload the new one
        if (!data.RawTGAData.empty()) {
            LoadTextureFromMemory(data.RawTGAData, &g_FontAtlasSRV, &g_FontAtlasWidth, &g_FontAtlasHeight);
        }
        g_LastFontEntryID = currentEntryID;
    }
    // --------------------------------------------------------------

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Font Metadata");
    ImGui::Separator();

    ImGui::Text("Name: "); ImGui::SameLine(); ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", data.FontName.c_str());
    ImGui::Text("Base Height: %u px", data.FontHeight);
    ImGui::Text("Max Height: %u px", data.MaxHeight);
    ImGui::Text("Weight: %u", data.Weight);
    ImGui::Text("Italics: %s", data.Italics ? "Yes" : "No");
    ImGui::Text("Texture Atlas Size: %u x %u", data.TexWidth, data.TexHeight);
    ImGui::Text("Character Range: %u to %u", data.MinChar, data.MaxChar);

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Glyph Banks (%zu)", data.GlyphBanks.size());
    ImGui::Separator();

    // Render the Glyph Tables
    for (size_t i = 0; i < data.GlyphBanks.size(); ++i) {
        if (ImGui::TreeNode((void*)(intptr_t)i, "Bank %u (Total Glyphs: %u)", data.GlyphBanks[i].BankIndex, data.GlyphBanks[i].GlyphCount)) {

            if (ImGui::BeginTable("GlyphTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 250))) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("UV Left");
                ImGui::TableSetupColumn("UV Top");
                ImGui::TableSetupColumn("UV Right");
                ImGui::TableSetupColumn("UV Bottom");
                ImGui::TableSetupColumn("Offset X");
                ImGui::TableSetupColumn("Advance");
                ImGui::TableHeadersRow();

                for (size_t g = 0; g < data.GlyphBanks[i].Glyphs.size(); ++g) {
                    const auto& glyph = data.GlyphBanks[i].Glyphs[g];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%zu", g);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", glyph.Left);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", glyph.Top);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", glyph.Right);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%.4f", glyph.Bottom);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%d", glyph.Offset);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%d", glyph.Advance);
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Texture Atlas Preview");
    ImGui::Separator();

    if (g_FontAtlasSRV) {
        // Draw the texture in ImGui with a dark background rectangle
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2((float)g_FontAtlasWidth, (float)g_FontAtlasHeight);

        ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 20, 255));
        ImGui::Image((void*)g_FontAtlasSRV, size);
    }
    else {
        ImGui::TextDisabled("No texture atlas available.");
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Live Text Preview");
    ImGui::Separator();

    if (g_FontAtlasSRV && !data.GlyphBanks.empty()) {
        static char previewText[256] = "Hello Fable Modders!";
        ImGui::InputText("Type here", previewText, 256);

        ImGui::Dummy(ImVec2(0, 5));

        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float boxHeight = (float)data.MaxHeight + 20.0f;
        float boxWidth = ImGui::GetContentRegionAvail().x;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(startPos, ImVec2(startPos.x + boxWidth, startPos.y + boxHeight), IM_COL32(20, 20, 20, 255));

        float cursorX = startPos.x + 10.0f;
        float cursorY = startPos.y + 10.0f;

        // Loop through the typed string and draw each character
        for (int i = 0; previewText[i] != '\0'; i++) {
            unsigned char c = previewText[i];

            // Calculate the exact index in our flattened array
            int glyphIndex = c - data.MinChar;

            // Ensure the character is within the font's supported range
            if (glyphIndex >= 0 && glyphIndex < data.AllGlyphs.size()) {
                const auto& targetGlyph = data.AllGlyphs[glyphIndex];

                float charWidth = (targetGlyph.Right - targetGlyph.Left) * g_FontAtlasWidth;
                float charHeight = (targetGlyph.Bottom - targetGlyph.Top) * g_FontAtlasHeight;
                float drawX = cursorX + targetGlyph.Offset;

                drawList->AddImage(
                    (void*)g_FontAtlasSRV,
                    ImVec2(drawX, cursorY),
                    ImVec2(drawX + charWidth, cursorY + charHeight),
                    ImVec2(targetGlyph.Left, targetGlyph.Top),
                    ImVec2(targetGlyph.Right, targetGlyph.Bottom)
                );

                cursorX += targetGlyph.Advance;
            }
        }

        ImGui::Dummy(ImVec2(0, boxHeight));
    }

    ImGui::Dummy(ImVec2(0, 10));

    // Add the "Replace with TTF" button
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("Replace with TTF", ImVec2(150, 30))) {
        g_ShowFontImporter = true;
    }

    ImGui::SameLine();

    // Export TGA Button
    if (!data.RawTGAData.empty()) {
        if (ImGui::Button("Export TGA to Disk", ImVec2(150, 30))) {
            std::string outPath = g_AppConfig.GameRootPath + "\\" + data.FontName + "_atlas.tga";
            std::ofstream outFile(outPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write((const char*)data.RawTGAData.data(), data.RawTGAData.size());
                outFile.close();
                g_BankStatus = "Exported: " + data.FontName + "_atlas.tga";
            }
            else {
                g_BankStatus = "Error: Could not write TGA file.";
            }
        }
    }

    // Render the actual popup (pass the global active bank)
    LoadedBank* activeBank = nullptr;
    if (g_ActiveBankIndex >= 0 && g_ActiveBankIndex < g_OpenBanks.size()) {
        activeBank = &g_OpenBanks[g_ActiveBankIndex];
    }
    DrawFontRebuilderModal(activeBank);
}