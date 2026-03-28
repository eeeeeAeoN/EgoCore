#pragma once
#include "imgui.h"
#include "BankBackend.h"
#include "StreamingFontParser.h"
#include "FontProperties.h"
#include "StreamingFontBuilder.h"
#include <algorithm> 

inline ID3D11ShaderResourceView* s_StitchedStringTex = nullptr;
inline int s_StitchedW = 0;
inline int s_StitchedH = 0;
inline int g_LastStreamingFontEntryID = -1;

inline void RebuildStringTexture(const char* text) {
    if (s_StitchedStringTex) { s_StitchedStringTex->Release(); s_StitchedStringTex = nullptr; }
    s_StitchedW = 0; s_StitchedH = 0;

    const auto& meta = g_StreamingFontParser.Metadata;
    const auto& pixels = g_StreamingFontParser.CachedPixelData;
    if (pixels.BinaryData.empty() || pixels.DecompressedMetrics.empty()) return;

    size_t len = strlen(text);
    if (len == 0) return;

    int cursorX = 20;
    int maxPixelX = 20;

    for (size_t i = 0; i < len; i++) {
        uint8_t charCode = (uint8_t)text[i];
        uint32_t chunkIdx = charCode / 64;
        uint32_t localIdx = charCode % 64;

        if (chunkIdx < meta.GlyphBanks.size() && meta.GlyphBanks[chunkIdx].NoGlyphs > 0) {
            const auto& bank = meta.GlyphBanks[chunkIdx];
            if (localIdx >= bank.FirstGlyph && localIdx < (bank.FirstGlyph + bank.NoGlyphs)) {
                uint32_t rawID = bank.Glyphs[localIdx - bank.FirstGlyph].GetGlyphIndex();
                if (rawID > 0) {
                    uint32_t globalID = rawID - 1;
                    if (globalID < pixels.DecompressedMetrics.size()) {
                        const auto& m = pixels.DecompressedMetrics[globalID];
                        uint32_t adv = m.GetAdvance();

                        if (charCode == 32) {
                            cursorX += (adv > 0 ? adv : meta.FontHeight / 3);
                            if (cursorX > maxPixelX) maxPixelX = cursorX;
                            continue;
                        }

                        int pitch = m.IsBigChar() ? 64 : 32;
                        int safeW = (std::min)((int)m.Width, pitch);

                        int rightEdge = cursorX + m.OffsetX + safeW;
                        if (rightEdge > maxPixelX) maxPixelX = rightEdge;

                        cursorX += (adv > 0 ? adv : safeW + 1);
                    }
                }
            }
        }
    }

    int totalWidth = maxPixelX + 20;
    if (totalWidth <= 4 || totalWidth > 4096) return;

    int canvasHeight = meta.MaxHeight > 0 ? meta.MaxHeight + 40 : 80;
    int baselineY = 20;

    struct DrawCommand { int destX, destY, w, h, srcBlock, srcPitch; };
    std::vector<DrawCommand> drawCmds;

    int currentX = 20;

    for (size_t i = 0; i < len; i++) {
        uint8_t charCode = (uint8_t)text[i];
        uint32_t chunkIdx = charCode / 64;
        uint32_t localIdx = charCode % 64;

        if (chunkIdx < meta.GlyphBanks.size() && meta.GlyphBanks[chunkIdx].NoGlyphs > 0) {
            const auto& bank = meta.GlyphBanks[chunkIdx];
            if (localIdx >= bank.FirstGlyph && localIdx < (bank.FirstGlyph + bank.NoGlyphs)) {
                uint32_t rawID = bank.Glyphs[localIdx - bank.FirstGlyph].GetGlyphIndex();
                if (rawID > 0) {
                    uint32_t globalID = rawID - 1;
                    if (globalID < pixels.DecompressedMetrics.size()) {
                        const auto& m = pixels.DecompressedMetrics[globalID];
                        uint32_t adv = m.GetAdvance();

                        if (charCode == 32) {
                            currentX += (adv > 0 ? adv : meta.FontHeight / 3);
                            continue;
                        }

                        int pitch = m.IsBigChar() ? 64 : 32;

                        DrawCommand cmd;
                        cmd.destX = currentX + m.OffsetX;
                        cmd.destY = baselineY + m.OffsetY;
                        cmd.w = (std::min)((int)m.Width, pitch);
                        cmd.h = (std::min)((int)m.Height, pitch);
                        cmd.srcBlock = m.GetMemOffset();
                        cmd.srcPitch = pitch;

                        drawCmds.push_back(cmd);
                        currentX += (adv > 0 ? adv : cmd.w + 1);
                    }
                }
            }
        }
    }

    std::vector<uint8_t> tgaBuffer(18 + (totalWidth * canvasHeight * 4), 0);
    tgaBuffer[2] = 2;
    tgaBuffer[12] = totalWidth & 0xFF; tgaBuffer[13] = (totalWidth >> 8) & 0xFF;
    tgaBuffer[14] = canvasHeight & 0xFF; tgaBuffer[15] = (canvasHeight >> 8) & 0xFF;
    tgaBuffer[16] = 32; tgaBuffer[17] = 0x28;

    for (const auto& cmd : drawCmds) {
        uint32_t blockOffset = cmd.srcBlock * 1024;

        for (int y = 0; y < cmd.h; y++) {
            for (int x = 0; x < cmd.w; x++) {
                size_t srcPx = blockOffset + (y * cmd.srcPitch) + x;
                uint8_t alpha = (srcPx < pixels.BinaryData.size()) ? pixels.BinaryData[srcPx] : 0;

                int dx = cmd.destX + x;
                int dy = cmd.destY + y;
                if (dx >= 0 && dx < totalWidth && dy >= 0 && dy < canvasHeight) {

                    if (alpha > 0) {
                        int outIdx = 18 + ((dy * totalWidth + dx) * 4);
                        tgaBuffer[outIdx + 0] = 255;
                        tgaBuffer[outIdx + 1] = 255;
                        tgaBuffer[outIdx + 2] = 255;

                        int existingAlpha = tgaBuffer[outIdx + 3];
                        tgaBuffer[outIdx + 3] = (alpha > existingAlpha) ? alpha : existingAlpha;
                    }
                }
            }
        }
    }

    LoadTextureFromMemory(tgaBuffer, &s_StitchedStringTex, &s_StitchedW, &s_StitchedH);
}

inline void DrawStreamingFontProperties(LoadedBank* bank, int entryIdx) {
    if (!bank || entryIdx < 0 || entryIdx >= bank->Entries.size()) return;
    const auto& e = bank->Entries[entryIdx];

    if (e.Type == 2) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Raw Pixel Payload");
        ImGui::TextDisabled("This entry contains the compressed pixel chunks.");
        ImGui::TextDisabled("Select the Font Metadata entry (Type 0 or 1) above this to use the Live Preview.");
        return;
    }

    if (!g_StreamingFontParser.IsParsed) {
        ImGui::TextDisabled("Font parsing failed.");
        return;
    }

    BankEntry* type2Entry = nullptr;
    for (auto& otherEntry : bank->Entries) {
        if (otherEntry.Type == 2) { type2Entry = &otherEntry; break; }
    }

    if (type2Entry) {
        if (g_StreamingFontParser.CachedPixelDataID != type2Entry->ID) {
            std::vector<uint8_t> t2Data;
            if (bank->ModifiedEntryData.count(type2Entry->ID)) t2Data = bank->ModifiedEntryData[type2Entry->ID];
            else {
                bank->Stream->clear();
                bank->Stream->seekg(type2Entry->Offset, std::ios::beg);
                t2Data.resize(type2Entry->Size);
                bank->Stream->read((char*)t2Data.data(), type2Entry->Size);
            }
            g_StreamingFontParser.LoadAndCachePixelData(type2Entry->ID, t2Data);
        }
    }

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Dynamic Streaming Font");
    ImGui::TextDisabled("Source File: %s", g_StreamingFontParser.Metadata.SourceName.c_str());
    ImGui::TextDisabled("Base Height: %u px | Max Height: %u px", g_StreamingFontParser.Metadata.FontHeight, g_StreamingFontParser.Metadata.MaxHeight);

    ImGui::TextDisabled("Weight: %u | Italics: %s", g_StreamingFontParser.Metadata.FontWeight, g_StreamingFontParser.Metadata.Italics ? "Yes" : "No");

    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("Replace with TTF", ImVec2(150, 30))) {
        g_StreamingFontBakeState.TargetPixelHeight = (float)g_StreamingFontParser.Metadata.FontHeight;
        g_StreamingFontBakeState.Italics = g_StreamingFontParser.Metadata.Italics;

        g_StreamingFontBakeState.LetterSpacing = 0;
        g_StreamingFontBakeState.BaselineOffset = 0;

        g_ShowStreamingFontImporter = true;
    }

    ImGui::Separator();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Live Font Renderer");

    static char testString[256] = "Fable - The Lost Chapters";
    bool changed = ImGui::InputText("##TestString", testString, 256);

    if (changed || g_LastStreamingFontEntryID != e.ID) {
        RebuildStringTexture(testString);
        g_LastStreamingFontEntryID = e.ID;
    }

    ImGui::Dummy(ImVec2(0, 10));
    if (s_StitchedStringTex) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + s_StitchedW * 2, p.y + s_StitchedH * 2), IM_COL32(45, 45, 48, 255));
        ImGui::Image((void*)s_StitchedStringTex, ImVec2(s_StitchedW * 2, s_StitchedH * 2));
        ImGui::Dummy(ImVec2(s_StitchedW * 2, s_StitchedH * 2));
    }
    else {
        if (!type2Entry) ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: No Type 2 Pixel Payload found in this bank!");
        else ImGui::TextDisabled("Type text to preview...");
    }

    DrawStreamingFontRebuilderModal(bank);
}