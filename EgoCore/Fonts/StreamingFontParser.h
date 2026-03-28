#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <minilzo.h> 

#pragma pack(push, 1)

struct CStreamingGlyph {
    uint32_t RawData;
    uint32_t GetGlyphIndex() const { return RawData & 0xFFFFFF; }
};

struct StreamingGlyphBank {
    uint16_t FirstGlyph;
    uint16_t NoGlyphs;
    CStreamingGlyph Glyphs[64];
};

struct CStreamingGlyphData {
    int8_t OffsetX;
    int8_t OffsetY;
    uint8_t Width;
    uint8_t Height;
    uint32_t MetricB;

    uint32_t GetMemOffset() const { return MetricB & 0x3FFFFF; }
    uint32_t GetAdvance() const { return (MetricB >> 23) & 0xFF; }
    bool IsBigChar() const { return (MetricB & 0x80000000) != 0; }
};

#pragma pack(pop)

struct CStreamingFontMetadata {
    std::string SourceName;
    uint32_t FontHeight;
    uint32_t FontWeight;
    bool Italics;
    uint32_t MaxHeight;
    std::vector<uint32_t> OffsetTable;
    std::vector<StreamingGlyphBank> GlyphBanks;
};

struct CStreamingFontPixelData {
    uint32_t GlyphDataNum;
    uint32_t AdjustmentCount;
    uint32_t BinaryDataStart;
    uint32_t CompressedDataSize;
    uint32_t ChunkSize;
    std::vector<uint8_t> CompressedPixels;
    std::vector<uint32_t> ChunkIndices;
    std::vector<uint8_t> Adjustments;
    std::vector<uint8_t> BinaryData;
    std::vector<CStreamingGlyphData> DecompressedMetrics;
};

class CStreamingFontParser {
public:
    bool IsParsed = false;
    bool IsMetadata = false;
    bool IsPixelData = false;

    CStreamingFontMetadata Metadata;
    CStreamingFontPixelData PixelData;

    int CachedPixelDataID = -1;
    CStreamingFontPixelData CachedPixelData;

    void Parse(const std::vector<uint8_t>& rawData, int type) {
        IsParsed = false;
        IsMetadata = false;
        IsPixelData = false;
        if (rawData.empty()) return;

        if (type == 0 || type == 1) ParseMetadata(rawData);
        else if (type == 2) ParsePixelData(rawData);
    }

    void LoadAndCachePixelData(int entryID, const std::vector<uint8_t>& rawData) {
        if (CachedPixelDataID == entryID) return;
        ParsePixelData(rawData);
        CachedPixelData = PixelData;
        CachedPixelDataID = entryID;
    }

private:
    void ParseMetadata(const std::vector<uint8_t>& rawData) {
        Metadata = CStreamingFontMetadata();
        size_t cursor = 0;

        while (cursor < rawData.size() && rawData[cursor] != '\0') {
            Metadata.SourceName += (char)rawData[cursor];
            cursor++;
        }
        cursor++;

        if (cursor + 17 > rawData.size()) return;

        Metadata.FontHeight = *(uint32_t*)&rawData[cursor]; cursor += 4;
        Metadata.FontWeight = *(uint32_t*)&rawData[cursor]; cursor += 4;
        Metadata.Italics = rawData[cursor] != 0; cursor += 1;
        Metadata.MaxHeight = *(uint32_t*)&rawData[cursor]; cursor += 4;

        if (cursor + 4100 > rawData.size()) return;

        Metadata.OffsetTable.resize(1025);
        memcpy(Metadata.OffsetTable.data(), &rawData[cursor], 4100);
        cursor += 4100;

        size_t glyphBankMemoryStart = cursor;

        for (int i = 0; i < 1024; i++) {
            uint32_t startOffset = Metadata.OffsetTable[i];
            uint32_t endOffset = Metadata.OffsetTable[i + 1];
            uint32_t blockSize = endOffset - startOffset;

            StreamingGlyphBank bankInfo;
            memset(&bankInfo, 0, sizeof(StreamingGlyphBank));
            bankInfo.FirstGlyph = 64;
            bankInfo.NoGlyphs = 0;

            if (blockSize >= 2) {
                size_t actualStart = glyphBankMemoryStart + startOffset;
                if (actualStart + 2 <= rawData.size()) {
                    uint16_t header = 0;
                    memcpy(&header, &rawData[actualStart], 2);

                    bool isCompressed = (header & 0x8000) != 0;
                    size_t compSize = blockSize - 2;

                    if (isCompressed) {
                        lzo_uint outLen = sizeof(StreamingGlyphBank);
                        std::vector<uint8_t> uncompressed(outLen, 0);

                        if (lzo1x_decompress_safe(&rawData[actualStart + 2], compSize, uncompressed.data(), &outLen, nullptr) == LZO_E_OK) {
                            memcpy(&bankInfo, uncompressed.data(), outLen);
                        }
                    }
                    else {
                        size_t copySize = (compSize > sizeof(StreamingGlyphBank)) ? sizeof(StreamingGlyphBank) : compSize;
                        memcpy(&bankInfo, &rawData[actualStart + 2], copySize);
                    }
                }
            }
            Metadata.GlyphBanks.push_back(bankInfo);
        }
        IsMetadata = true;
        IsParsed = true;
    }

    void ParsePixelData(const std::vector<uint8_t>& rawData) {
        PixelData = CStreamingFontPixelData();
        if (rawData.size() < 20) return;

        size_t cursor = 0;
        PixelData.GlyphDataNum = *(uint32_t*)&rawData[cursor]; cursor += 4;
        PixelData.AdjustmentCount = *(uint32_t*)&rawData[cursor]; cursor += 4;
        PixelData.BinaryDataStart = *(uint32_t*)&rawData[cursor]; cursor += 4;
        PixelData.CompressedDataSize = *(uint32_t*)&rawData[cursor]; cursor += 4;
        PixelData.ChunkSize = *(uint32_t*)&rawData[cursor]; cursor += 4;

        if (cursor + PixelData.CompressedDataSize > rawData.size()) return;

        PixelData.CompressedPixels.resize(PixelData.CompressedDataSize);
        memcpy(PixelData.CompressedPixels.data(), &rawData[cursor], PixelData.CompressedDataSize);
        cursor += PixelData.CompressedDataSize;

        uint32_t numIndices = ((PixelData.GlyphDataNum + 63) >> 6) + 1;
        size_t indicesByteSize = numIndices * 4;
        if (cursor + indicesByteSize > rawData.size()) return;

        PixelData.ChunkIndices.resize(numIndices);
        memcpy(PixelData.ChunkIndices.data(), &rawData[cursor], indicesByteSize);
        cursor += indicesByteSize;

        size_t adjustmentsByteSize = PixelData.BinaryDataStart - cursor;
        if (adjustmentsByteSize > 0 && cursor + adjustmentsByteSize <= rawData.size()) {
            PixelData.Adjustments.resize(adjustmentsByteSize);
            memcpy(PixelData.Adjustments.data(), &rawData[cursor], adjustmentsByteSize);
            cursor += adjustmentsByteSize;
        }

        size_t binaryDataSize = rawData.size() - cursor;
        if (binaryDataSize > 0) {
            PixelData.BinaryData.resize(binaryDataSize);
            memcpy(PixelData.BinaryData.data(), &rawData[cursor], binaryDataSize);
        }

        PixelData.DecompressedMetrics.clear();
        PixelData.DecompressedMetrics.reserve(PixelData.GlyphDataNum);

        for (size_t i = 0; i < PixelData.ChunkIndices.size() - 1; i++) {
            uint32_t startOffset = PixelData.ChunkIndices[i];
            uint32_t endOffset = PixelData.ChunkIndices[i + 1];
            uint32_t compSize = endOffset - startOffset;

            if (compSize > 0 && startOffset + compSize <= PixelData.CompressedPixels.size()) {
                uint32_t expectedGlyphs = PixelData.ChunkSize;
                if (i == PixelData.ChunkIndices.size() - 2) {
                    expectedGlyphs = PixelData.GlyphDataNum % PixelData.ChunkSize;
                    if (expectedGlyphs == 0) expectedGlyphs = PixelData.ChunkSize;
                }

                lzo_uint outLen = expectedGlyphs * sizeof(CStreamingGlyphData);
                std::vector<uint8_t> uncompressed(outLen, 0);

                if (lzo1x_decompress_safe(&PixelData.CompressedPixels[startOffset], compSize, uncompressed.data(), &outLen, nullptr) == LZO_E_OK) {
                    CStreamingGlyphData* glyphs = (CStreamingGlyphData*)uncompressed.data();
                    int numDecompressed = outLen / sizeof(CStreamingGlyphData);
                    for (int g = 0; g < numDecompressed; g++) PixelData.DecompressedMetrics.push_back(glyphs[g]);
                }
            }
        }
        IsPixelData = true;
        IsParsed = true;
    }
};

inline CStreamingFontParser g_StreamingFontParser;