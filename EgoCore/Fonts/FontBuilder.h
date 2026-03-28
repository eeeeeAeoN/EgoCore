#pragma once
#include "imgui.h"
#include "ImageBackend.h" 
#include "FontParser.h"
#include "BankBackend.h"
#include "FileDialogs.h"
#include <vector>
#include <string>
#include <cmath>
#include <fstream>

void SelectEntry(LoadedBank* bank, int idx);

struct FontBakeOptions {
    std::string SourceTTFPath = "";
    float TargetPixelHeight = 24.0f;
    int Weight = 400;
    bool Italics = false;
};

#pragma pack(push, 1)
struct FableTGAHeader {
    uint8_t  IDLength = 0;
    uint8_t  ColorMapType = 0;
    uint8_t  ImageType = 2;
    uint8_t  ColorMapSpec[5] = { 0, 0, 0, 0, 0 };
    uint16_t XOrigin = 0;
    uint16_t YOrigin = 0;
    uint16_t Width;
    uint16_t Height;
    uint8_t  PixelDepth = 32;
    uint8_t  ImageDescriptor = 0x28;
};
#pragma pack(pop)

class CFontBuilder {
public:
    static bool BakeFont(const FontBakeOptions& opts, const std::string& fontName, std::vector<uint8_t>& out_BigPayload) {
        if (opts.SourceTTFPath.empty()) return false;

        std::ifstream ttfFile(opts.SourceTTFPath, std::ios::binary | std::ios::ate);
        if (!ttfFile.is_open()) return false;
        std::streamsize ttfSize = ttfFile.tellg();
        ttfFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> ttfBuffer(ttfSize);
        if (!ttfFile.read((char*)ttfBuffer.data(), ttfSize)) return false;

        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, ttfBuffer.data(), stbtt_GetFontOffsetForIndex(ttfBuffer.data(), 0))) return false;

        float scale = stbtt_ScaleForPixelHeight(&font, opts.TargetPixelHeight);
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

        int baseline = (int)std::round(ascent * scale);
        int cellHeight = (int)std::round((ascent - descent + lineGap) * scale);
        if (cellHeight <= 0) cellHeight = (int)opts.TargetPixelHeight;

        struct GlyphInfo { int w, h, xoff, yoff, advance; unsigned char* bitmap; };
        std::vector<GlyphInfo> glyphs(96);
        for (int i = 0; i < 96; i++) {
            int cp = 32 + i;
            GlyphInfo& g = glyphs[i];
            g.bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, cp, &g.w, &g.h, &g.xoff, &g.yoff);
            int adv; stbtt_GetCodepointHMetrics(&font, cp, &adv, 0);
            g.advance = (int)std::round(adv * scale);
        }

        int texSize = 128;
        int cursorX = 0, cursorY = 0;
        while (texSize <= 4096) {
            cursorX = 0; cursorY = 0;
            bool fit = true;
            for (int i = 0; i < 96; i++) {
                int padW = glyphs[i].w + 2;
                if (cursorX + padW > texSize) {
                    cursorX = 0;
                    cursorY += cellHeight + 2;
                }
                if (cursorY + cellHeight + 2 > texSize) { fit = false; break; }
                cursorX += padW;
            }
            if (fit) break;
            texSize *= 2;
        }

        if (texSize > 4096) {
            for (auto& g : glyphs) if (g.bitmap) stbtt_FreeBitmap(g.bitmap, nullptr);
            return false;
        }

        std::vector<uint8_t> atlas(texSize * texSize, 0);
        cursorX = 0; cursorY = 0;
        std::vector<FableGlyph> fableGlyphs(96);

        for (int i = 0; i < 96; i++) {
            GlyphInfo& g = glyphs[i];
            int padW = g.w + 2;
            if (cursorX + padW > texSize) {
                cursorX = 0;
                cursorY += cellHeight + 2;
            }

            int drawX = cursorX + 1;
            int drawY = cursorY + 1 + baseline + g.yoff;

            if (g.bitmap) {
                for (int by = 0; by < g.h; by++) {
                    for (int bx = 0; bx < g.w; bx++) {
                        if (drawY + by >= 0 && drawY + by < texSize && drawX + bx >= 0 && drawX + bx < texSize) {
                            atlas[(drawY + by) * texSize + (drawX + bx)] = g.bitmap[by * g.w + bx];
                        }
                    }
                }
                stbtt_FreeBitmap(g.bitmap, nullptr);
            }

            FableGlyph fg;
            fg.Left = (float)cursorX / (float)texSize;
            fg.Top = (float)cursorY / (float)texSize;
            fg.Right = (float)(cursorX + padW) / (float)texSize;
            fg.Bottom = (float)(cursorY + cellHeight + 2) / (float)texSize;
            fg.Offset = (int16_t)g.xoff - 1;
            fg.Width = padW;
            fg.Advance = (int16_t)g.advance;

            fableGlyphs[i] = fg;
            cursorX += padW;
        }

        std::vector<uint8_t> tgaData;
        FableTGAHeader tgaHeader;
        tgaHeader.Width = texSize;
        tgaHeader.Height = texSize;

        uint8_t* tgaHdrPtr = (uint8_t*)&tgaHeader;
        tgaData.insert(tgaData.end(), tgaHdrPtr, tgaHdrPtr + sizeof(FableTGAHeader));

        for (int i = 0; i < texSize * texSize; i++) {
            uint8_t alpha = atlas[i];
            tgaData.push_back(255);   
            tgaData.push_back(255);   
            tgaData.push_back(255);   
            tgaData.push_back(alpha); 
        }

        auto write32 = [&](uint32_t v) { uint8_t* p = (uint8_t*)&v; out_BigPayload.insert(out_BigPayload.end(), p, p + 4); };
        auto write8 = [&](uint8_t v) { out_BigPayload.push_back(v); };

        for (char c : fontName) out_BigPayload.push_back(c);
        out_BigPayload.push_back(0);

        write32(cellHeight);
        write32(opts.Weight);
        write8(opts.Italics ? 1 : 0);
        write32(cellHeight);
        write32(texSize);
        write32(texSize);
        write32(32);  
        write32(127); 
        write32(2);   

        auto writeBank = [&](uint32_t index, uint32_t start, uint32_t count, int offset) {
            write32(index);
            write32(start);
            write32(count);
            for (uint32_t i = 0; i < count; i++) {
                uint8_t* p = (uint8_t*)&fableGlyphs[offset + i];
                out_BigPayload.insert(out_BigPayload.end(), p, p + sizeof(FableGlyph));
            }
            };

        writeBank(0, 32, 32, 0);
        writeBank(1, 64, 64, 32);
        write32((uint32_t)tgaData.size());
        out_BigPayload.insert(out_BigPayload.end(), tgaData.begin(), tgaData.end());

        return true;
    }
};

inline FontBakeOptions g_FontBakeState;
inline bool g_ShowFontImporter = false;

inline void DrawFontRebuilderModal(LoadedBank* activeBank) {
    if (!g_ShowFontImporter) return;
    ImGui::OpenPopup("Replace Font with TTF");

    if (ImGui::BeginPopupModal("Replace Font with TTF", &g_ShowFontImporter, ImGuiWindowFlags_AlwaysAutoResize)) {

        if (activeBank && activeBank->SelectedEntryIndex != -1) {
            ImGui::Text("Replacing Entry:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "%s", activeBank->Entries[activeBank->SelectedEntryIndex].Name.c_str());
        }
        ImGui::Separator();

        std::string displayPath = g_FontBakeState.SourceTTFPath.empty() ? "None Selected" : std::filesystem::path(g_FontBakeState.SourceTTFPath).filename().string();
        ImGui::Text("File: %s", displayPath.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Browse##TTF")) {
            std::string path = OpenFileDialog("TrueType Fonts\0*.ttf\0All Files\0*.*\0");
            if (!path.empty()) {
                g_FontBakeState.SourceTTFPath = path;
            }
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::SliderFloat("Pixel Height", &g_FontBakeState.TargetPixelHeight, 8.0f, 72.0f, "%.0f px");
        ImGui::Checkbox("Is Italic", &g_FontBakeState.Italics);

        ImGui::Dummy(ImVec2(0, 15));
        ImGui::Separator();

        if (ImGui::Button("Bake & Replace", ImVec2(120, 0))) {
            if (!g_FontBakeState.SourceTTFPath.empty() && activeBank && activeBank->SelectedEntryIndex != -1) {
                std::vector<uint8_t> finalPayload;
                int idx = activeBank->SelectedEntryIndex;
                std::string currentName = activeBank->Entries[idx].Name;

                if (CFontBuilder::BakeFont(g_FontBakeState, currentName, finalPayload)) {

                    activeBank->ModifiedEntryData[idx] = finalPayload;
                    UpdateFilter(*activeBank);
                    SelectEntry(activeBank, idx);

                    extern int g_LastFontEntryID;
                    g_LastFontEntryID = -1;

                    g_BankStatus = "Font Replaced Successfully!";
                }
                else {
                    g_BankStatus = "Error: Failed to bake TrueType Font.";
                }
            }
            g_ShowFontImporter = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowFontImporter = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}