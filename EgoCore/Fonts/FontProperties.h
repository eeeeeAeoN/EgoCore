#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "FontParser.h"
#include "FontBuilder.h"
#include "ConfigBackend.h" 
#include "ImageBackend.h" 
#include <d3d11.h>        
#include <fstream>

extern ID3D11Device* g_pd3dDevice;
extern ImTextureID g_ImportTexture;
static const ImVec4 kFontImportTint = ImVec4(0.45f, 0.85f, 0.45f, 1.0f);

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

inline CFontParser g_FontParser;
inline int g_LastFontEntryID = -1;

inline ID3D11ShaderResourceView* g_FontAtlasSRV = nullptr;
inline int g_FontAtlasWidth = 0;
inline int g_FontAtlasHeight = 0;

inline void DrawFontProperties(int currentEntryID, LoadedBank* activeBank = nullptr) {
    if (!g_FontParser.IsParsed) {
        ImGui::TextDisabled("No font data available or parsed.");
        return;
    }

    const auto& data = g_FontParser.Data;

    if (g_LastFontEntryID != currentEntryID) {
        if (g_FontAtlasSRV) {
            g_FontAtlasSRV->Release();
            g_FontAtlasSRV = nullptr;
        }
        if (!data.RawTGAData.empty()) {
            LoadTextureFromMemory(data.RawTGAData, &g_FontAtlasSRV, &g_FontAtlasWidth, &g_FontAtlasHeight);
        }
        g_LastFontEntryID = currentEntryID;
    }
    else if (!g_FontAtlasSRV && !data.RawTGAData.empty()) {
        LoadTextureFromMemory(data.RawTGAData, &g_FontAtlasSRV, &g_FontAtlasWidth, &g_FontAtlasHeight);
    }

    ImGui::TextDisabled("Source File: %s", data.FontName.c_str());
    ImGui::TextDisabled("Base Height: %u px | Max Height: %u px", data.FontHeight, data.MaxHeight);
    ImGui::TextDisabled("Weight: %u | Italics: %s", data.Weight, data.Italics ? "Yes" : "No");
    ImGui::TextDisabled("Atlas Size: %u x %u | Char Range: %u to %u", data.TexWidth, data.TexHeight, data.MinChar, data.MaxChar);

    ImGui::Dummy(ImVec2(0, 6.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 6.0f));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Live Font Renderer");

    static char previewText[256] = "Fable - The Lost Chapters";

    float btnSize = 24.0f;
    float btnTotalW = btnSize + ImGui::GetStyle().FramePadding.x * 2.0f;
    float availW = ImGui::GetContentRegionAvail().x;
    float inputW = availW - btnTotalW - ImGui::GetStyle().ItemSpacing.x;
    if (inputW > 50.0f) {
        ImGui::SetNextItemWidth(inputW);
    }

    ImGui::InputTextWithHint("##RegularFontTestString", "Type text to preview...", previewText, sizeof(previewText));

    ImGui::SameLine();

    bool openImporter = false;
    if (g_ImportTexture) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.85f, 0.45f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.85f, 0.45f, 0.5f));
        if (ImGui::ImageButton("##ReplaceRegularFontBtn", g_ImportTexture, ImVec2(btnSize, btnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kFontImportTint)) {
            openImporter = true;
        }
        ImGui::PopStyleColor(3);
    }
    else {
        if (ImGui::Button("Import", ImVec2(btnTotalW, btnSize + ImGui::GetStyle().FramePadding.y * 2.0f))) {
            openImporter = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Replace with .ttf font");
    }

    if (openImporter) {
        g_FontBakeState.TargetPixelHeight = (float)data.FontHeight;
        g_FontBakeState.Italics = data.Italics;
        g_ShowFontImporter = true;
    }

    ImGui::Dummy(ImVec2(0, 6.0f));

    float boxWidth = ImGui::GetContentRegionAvail().x;
    float fontH = (float)(data.MaxHeight > 0 ? data.MaxHeight : (data.FontHeight > 0 ? data.FontHeight : 32));
    float minH = fontH + 28.0f;
    if (minH < 70.0f) minH = 70.0f;
    float boxHeight = minH;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Dark rounded preview background with subtle border
    drawList->AddRectFilled(p, ImVec2(p.x + boxWidth, p.y + boxHeight), IM_COL32(24, 25, 29, 255), 4.0f);
    drawList->AddRect(p, ImVec2(p.x + boxWidth, p.y + boxHeight), IM_COL32(50, 52, 60, 255), 4.0f);

    drawList->PushClipRect(p, ImVec2(p.x + boxWidth - 4.0f, p.y + boxHeight), true);

    if (g_FontAtlasSRV && !data.GlyphBanks.empty() && previewText[0] != '\0') {
        float cursorX = p.x + 12.0f;
        float cursorY = p.y + (boxHeight - fontH) * 0.5f;
        if (cursorY < p.y + 10.0f) cursorY = p.y + 10.0f;

        for (int i = 0; previewText[i] != '\0'; i++) {
            unsigned char c = (unsigned char)previewText[i];
            int glyphIndex = c - data.MinChar;
            if (glyphIndex >= 0 && glyphIndex < (int)data.AllGlyphs.size()) {
                const auto& targetGlyph = data.AllGlyphs[glyphIndex];

                float charWidth = (targetGlyph.Right - targetGlyph.Left) * (float)g_FontAtlasWidth;
                float charHeight = (targetGlyph.Bottom - targetGlyph.Top) * (float)g_FontAtlasHeight;
                float drawX = cursorX + targetGlyph.Offset;

                if (charWidth > 0.0f && charHeight > 0.0f) {
                    drawList->AddImage(
                        (void*)g_FontAtlasSRV,
                        ImVec2(drawX, cursorY),
                        ImVec2(drawX + charWidth, cursorY + charHeight),
                        ImVec2(targetGlyph.Left, targetGlyph.Top),
                        ImVec2(targetGlyph.Right, targetGlyph.Bottom)
                    );
                }

                if (targetGlyph.Advance > 0) {
                    cursorX += targetGlyph.Advance;
                }
                else if (c == ' ') {
                    cursorX += (fontH > 0 ? fontH / 3.0f : 8.0f);
                }
                else {
                    cursorX += (int)charWidth + 1;
                }
            }
            else if (c == ' ') {
                cursorX += (fontH > 0 ? fontH / 3.0f : 8.0f);
            }
        }
    }
    else {
        if (!g_FontAtlasSRV) {
            ImVec2 textPos = ImVec2(p.x + 14.0f, p.y + (boxHeight - ImGui::GetTextLineHeight()) * 0.5f);
            drawList->AddText(textPos, IM_COL32(180, 80, 80, 255), "No texture atlas available.");
        }
        else {
            ImVec2 textPos = ImVec2(p.x + 14.0f, p.y + (boxHeight - ImGui::GetTextLineHeight()) * 0.5f);
            drawList->AddText(textPos, IM_COL32(130, 130, 140, 255), "Type text to preview...");
        }
    }

    drawList->PopClipRect();

    ImGui::Dummy(ImVec2(boxWidth, boxHeight));

    if (!activeBank && g_ActiveBankIndex >= 0 && g_ActiveBankIndex < (int)g_OpenBanks.size()) {
        activeBank = &g_OpenBanks[g_ActiveBankIndex];
    }
    DrawFontRebuilderModal(activeBank);
}