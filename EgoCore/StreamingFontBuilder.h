#pragma once
#include "imgui.h"
#include "ImageBackend.h" 
#include "StreamingFontParser.h"
#include "BankBackend.h"
#include "FileDialogs.h"
#include <minilzo.h>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <algorithm>

struct StreamingFontBakeOptions {
    std::string SourceTTFPath = "";
    float TargetPixelHeight = 32.0f;
    int Weight = 400;
    bool Italics = false;
};

class CStreamingFontBuilder {
public:
    static bool BakeFont(const StreamingFontBakeOptions& opts, const std::string& fontName, const CStreamingFontPixelData& existingPixelData, std::vector<uint8_t>& out_Type0, std::vector<uint8_t>& out_Type2) {
        if (opts.SourceTTFPath.empty()) return false;

        if (lzo_init() != LZO_E_OK) return false;

        // 1. Read TTF File
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
        int maxFontHeight = (int)std::round((ascent - descent + lineGap) * scale);

        int baseline = (int)std::round(ascent * scale);

        // --- THE FIX: START FROM THE EXISTING PAYLOAD INSTEAD OF SCRATCH ---
        std::vector<CStreamingGlyphData> globalMetrics = existingPixelData.DecompressedMetrics;
        std::vector<uint8_t> rawPixelBuffer = existingPixelData.BinaryData;

        std::vector<uint32_t> charToGlobalID(65536, 0);

        // 2. Rasterize Glyphs and Append Blocks
        for (int cp = 32; cp <= 255; cp++) {
            int w, h, xoff, yoff;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, cp, &w, &h, &xoff, &yoff);

            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
            int scaledAdvance = (int)std::round(advance * scale);

            bool isBigChar = (w >= 32 || h >= 32);
            int blockSize = isBigChar ? 4096 : 1024;
            int blockPitch = isBigChar ? 64 : 32;

            // Calculate memory offset based on the CURRENT size of the appended buffer
            uint32_t memOffsetKB = (uint32_t)(rawPixelBuffer.size() / 1024);

            size_t startPx = rawPixelBuffer.size();
            rawPixelBuffer.resize(startPx + blockSize, 0);

            if (bitmap) {
                for (int y = 0; y < h && y < blockPitch; y++) {
                    for (int x = 0; x < w && x < blockPitch; x++) {
                        rawPixelBuffer[startPx + (y * blockPitch) + x] = bitmap[y * w + x];
                    }
                }
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            CStreamingGlyphData m;
            m.OffsetX = (int8_t)xoff;
            m.OffsetY = (int8_t)(baseline + yoff);
            m.Width = (uint8_t)(std::min)(w, 255);
            m.Height = (uint8_t)(std::min)(h, 255);

            uint32_t advClamped = (std::min)(scaledAdvance, 255);
            m.MetricB = (memOffsetKB & 0x3FFFFF) | ((advClamped & 0xFF) << 23) | (isBigChar ? 0x80000000 : 0);

            globalMetrics.push_back(m);
            // charToGlobalID maps codepoint to the 1-based index Fable expects
            charToGlobalID[cp] = (uint32_t)globalMetrics.size();
        }

        // 3. Build Type 0 Metadata Payload
        auto write32 = [&](std::vector<uint8_t>& buf, uint32_t v) { uint8_t* p = (uint8_t*)&v; buf.insert(buf.end(), p, p + 4); };
        auto write8 = [&](std::vector<uint8_t>& buf, uint8_t v) { buf.push_back(v); };

        out_Type0.clear();
        for (char c : fontName) write8(out_Type0, c);
        write8(out_Type0, 0);

        write32(out_Type0, (uint32_t)opts.TargetPixelHeight);
        write32(out_Type0, (uint32_t)opts.Weight);
        write8(out_Type0, opts.Italics ? 1 : 0);
        write32(out_Type0, (uint32_t)maxFontHeight);

        std::vector<uint32_t> offsetTable(1025, 0);
        std::vector<uint8_t> bankData;

        for (int i = 0; i < 1024; i++) {
            offsetTable[i] = (uint32_t)bankData.size();

            StreamingGlyphBank bank;
            memset(&bank, 0, sizeof(bank));
            bank.FirstGlyph = 64;
            bank.NoGlyphs = 0;

            int firstActive = -1;
            int lastActive = -1;

            for (int k = 0; k < 64; k++) {
                int cp = (i * 64) + k;
                if (charToGlobalID[cp] > 0) {
                    if (firstActive == -1) firstActive = k;
                    lastActive = k;
                }
            }

            if (firstActive != -1) {
                bank.FirstGlyph = (uint16_t)firstActive;
                bank.NoGlyphs = (uint16_t)(lastActive - firstActive + 1);
                for (int k = 0; k < bank.NoGlyphs; k++) {
                    int cp = (i * 64) + bank.FirstGlyph + k;
                    bank.Glyphs[k].RawData = charToGlobalID[cp] & 0xFFFFFF;
                }
            }

            uint16_t headerFlags = 0;
            uint8_t* pHead = (uint8_t*)&headerFlags;
            bankData.push_back(pHead[0]); bankData.push_back(pHead[1]);

            uint8_t* pBank = (uint8_t*)&bank;
            bankData.insert(bankData.end(), pBank, pBank + sizeof(StreamingGlyphBank));
        }
        offsetTable[1024] = (uint32_t)bankData.size();

        for (uint32_t off : offsetTable) write32(out_Type0, off);
        out_Type0.insert(out_Type0.end(), bankData.begin(), bankData.end());

        // 4. Build Type 2 Pixel Payload (Compressing the COMBINED metrics)
        out_Type2.clear();
        std::vector<uint8_t> compressedMetrics;
        std::vector<uint32_t> chunkIndices;

        std::vector<lzo_align_t> wrkmem(((LZO1X_1_MEM_COMPRESS)+sizeof(lzo_align_t) - 1) / sizeof(lzo_align_t));

        for (size_t i = 0; i < globalMetrics.size(); i += 64) {
            chunkIndices.push_back((uint32_t)compressedMetrics.size());

            size_t chunkCount = (std::min)((size_t)64, globalMetrics.size() - i);
            std::vector<uint8_t> uncompressed(chunkCount * sizeof(CStreamingGlyphData));
            memcpy(uncompressed.data(), &globalMetrics[i], uncompressed.size());

            std::vector<uint8_t> compBuf(uncompressed.size() + (uncompressed.size() / 16) + 64 + 3);
            lzo_uint out_len = 0;
            lzo1x_1_compress(uncompressed.data(), uncompressed.size(), compBuf.data(), &out_len, wrkmem.data());

            compressedMetrics.insert(compressedMetrics.end(), compBuf.data(), compBuf.data() + out_len);
        }
        chunkIndices.push_back((uint32_t)compressedMetrics.size());

        uint32_t GlyphDataNum = (uint32_t)globalMetrics.size();
        uint32_t AdjustmentCount = existingPixelData.AdjustmentCount; // KEEP OLD KERNINGS!
        uint32_t CompressedDataSize = (uint32_t)compressedMetrics.size();
        uint32_t ChunkSize = 64;

        uint32_t headersSize = 20 + (chunkIndices.size() * 4) + existingPixelData.Adjustments.size() + CompressedDataSize;
        uint32_t BinaryDataStart = (headersSize + 2047) & ~2047;

        write32(out_Type2, GlyphDataNum);
        write32(out_Type2, AdjustmentCount);
        write32(out_Type2, BinaryDataStart);
        write32(out_Type2, CompressedDataSize);
        write32(out_Type2, ChunkSize);

        out_Type2.insert(out_Type2.end(), compressedMetrics.begin(), compressedMetrics.end());
        for (uint32_t idx : chunkIndices) write32(out_Type2, idx);

        // Write the existing Kerning/Adjustment data back so other fonts don't break
        if (!existingPixelData.Adjustments.empty()) {
            out_Type2.insert(out_Type2.end(), existingPixelData.Adjustments.begin(), existingPixelData.Adjustments.end());
        }

        while (out_Type2.size() < BinaryDataStart) write8(out_Type2, 0);
        out_Type2.insert(out_Type2.end(), rawPixelBuffer.begin(), rawPixelBuffer.end());

        return true;
    }
};

inline StreamingFontBakeOptions g_StreamingFontBakeState;
inline bool g_ShowStreamingFontImporter = false;

inline void DrawStreamingFontRebuilderModal(LoadedBank* activeBank) {
    if (!g_ShowStreamingFontImporter) return;
    ImGui::OpenPopup("Rebuild Streaming Font");

    if (ImGui::BeginPopupModal("Rebuild Streaming Font", &g_ShowStreamingFontImporter, ImGuiWindowFlags_AlwaysAutoResize)) {

        if (activeBank && activeBank->SelectedEntryIndex != -1) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "Warning: This appends your font to the shared Type 2 payload.");
        }
        ImGui::Separator();

        std::string displayPath = g_StreamingFontBakeState.SourceTTFPath.empty() ? "None Selected" : std::filesystem::path(g_StreamingFontBakeState.SourceTTFPath).filename().string();
        ImGui::Text("File: %s", displayPath.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Browse##StreamTTF")) {
            std::string path = OpenFileDialog("TrueType Fonts\0*.ttf\0All Files\0*.*\0");
            if (!path.empty()) g_StreamingFontBakeState.SourceTTFPath = path;
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::SliderFloat("Pixel Height", &g_StreamingFontBakeState.TargetPixelHeight, 8.0f, 72.0f, "%.0f px");
        ImGui::SliderInt("Weight", &g_StreamingFontBakeState.Weight, 100, 900);
        ImGui::Checkbox("Is Italic", &g_StreamingFontBakeState.Italics);

        ImGui::Dummy(ImVec2(0, 15));
        ImGui::Separator();

        if (ImGui::Button("Bake & Replace", ImVec2(120, 0))) {
            if (!g_StreamingFontBakeState.SourceTTFPath.empty() && activeBank && activeBank->SelectedEntryIndex != -1) {

                int type0Idx = activeBank->SelectedEntryIndex;
                int type2Idx = -1;

                for (int i = type0Idx + 1; i < activeBank->Entries.size(); i++) {
                    if (activeBank->Entries[i].Type == 2) {
                        type2Idx = i;
                        break;
                    }
                }

                if (type0Idx != -1 && type2Idx != -1) {
                    // WE MUST EXTRACT THE OLD TYPE 2 DATA TO APPEND TO IT
                    std::vector<uint8_t> t2Data;
                    if (activeBank->ModifiedEntryData.count(type2Idx)) t2Data = activeBank->ModifiedEntryData[type2Idx];
                    else {
                        activeBank->Stream->clear();
                        activeBank->Stream->seekg(activeBank->Entries[type2Idx].Offset, std::ios::beg);
                        t2Data.resize(activeBank->Entries[type2Idx].Size);
                        activeBank->Stream->read((char*)t2Data.data(), activeBank->Entries[type2Idx].Size);
                    }

                    CStreamingFontParser tempParser;
                    tempParser.Parse(t2Data, 2);

                    if (tempParser.IsParsed) {
                        std::vector<uint8_t> payload0, payload2;
                        std::string fontName = activeBank->Entries[type0Idx].Name;

                        if (CStreamingFontBuilder::BakeFont(g_StreamingFontBakeState, fontName, tempParser.PixelData, payload0, payload2)) {
                            activeBank->ModifiedEntryData[type0Idx] = payload0;
                            activeBank->ModifiedEntryData[type2Idx] = payload2;

                            g_StreamingFontParser.CachedPixelDataID = -1;
                            g_StreamingFontParser.IsParsed = false;

                            extern int g_LastStreamingFontEntryID;
                            g_LastStreamingFontEntryID = -1;

                            UpdateFilter(*activeBank);
                            SelectEntry(activeBank, type0Idx);

                            g_BankStatus = "Streaming Font Appended and Rebuilt Successfully!";
                        }
                        else {
                            g_BankStatus = "Error: Failed to bake Streaming TTF.";
                        }
                    }
                    else {
                        g_BankStatus = "Error: Failed to parse existing Type 2 payload for appending.";
                    }
                }
                else {
                    g_BankStatus = "Error: Could not locate the paired Type 2 entry below this font.";
                }
            }
            g_ShowStreamingFontImporter = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            g_ShowStreamingFontImporter = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}