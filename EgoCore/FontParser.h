#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#pragma pack(push, 1)
struct FableGlyph {
    float Left;       // UV Left
    float Top;        // UV Top
    float Right;      // UV Right
    float Bottom;     // UV Bottom
    int16_t Offset;   // X-Offset
    int16_t Width;    // Pixel Width
    int16_t Advance;  // Cursor Advance
};
#pragma pack(pop)

struct GlyphBank {
    uint32_t BankIndex;
    uint32_t UnknownData;
    uint32_t GlyphCount;
    std::vector<FableGlyph> Glyphs;
};

struct CFontData {
    std::string FontName;
    uint32_t FontHeight;
    uint32_t Weight;
    bool Italics;
    uint32_t MaxHeight;
    uint32_t TexWidth;
    uint32_t TexHeight;
    uint32_t MinChar;
    uint32_t MaxChar;

    std::vector<GlyphBank> GlyphBanks;
    std::vector<FableGlyph> AllGlyphs; // <--- ADD THIS FLATTENED ARRAY

    std::vector<uint8_t> RawTGAData;
};

class CFontParser {
public:
    bool IsParsed = false;
    bool IsStreaming = false;
    CFontData Data;

    void Parse(const std::vector<uint8_t>& rawData, const std::string& subBankName = "") {
        IsParsed = false;
        IsStreaming = false;
        Data = CFontData();
        if (rawData.empty()) return;

        std::string upperSubBank = subBankName;
        std::transform(upperSubBank.begin(), upperSubBank.end(), upperSubBank.begin(), ::toupper);

        if (upperSubBank.find("STREAMING") != std::string::npos) {
            IsStreaming = true;
            IsParsed = true;
            return;
        }

        size_t offset = 0;
        size_t maxOffset = rawData.size();

        size_t nameStart = offset;
        while (offset < maxOffset && rawData[offset] != '\0') offset++;
        if (offset < maxOffset) {
            Data.FontName = std::string((char*)&rawData[nameStart], offset - nameStart);
            offset++;
        }

        if (offset + 33 > maxOffset) return;

        Data.FontHeight = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.Weight = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.Italics = rawData[offset] != 0;           offset += 1;
        Data.MaxHeight = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.TexWidth = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.TexHeight = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.MinChar = *(uint32_t*)(&rawData[offset]); offset += 4;
        Data.MaxChar = *(uint32_t*)(&rawData[offset]); offset += 4;

        uint32_t bankCount = *(uint32_t*)(&rawData[offset]); offset += 4;

        // 3. Read Glyph Arrays
        for (uint32_t i = 0; i < bankCount; ++i) {
            if (offset + 12 > maxOffset) break;

            GlyphBank bank;
            bank.BankIndex = *(uint32_t*)(&rawData[offset]); offset += 4;
            bank.UnknownData = *(uint32_t*)(&rawData[offset]); offset += 4;
            bank.GlyphCount = *(uint32_t*)(&rawData[offset]); offset += 4;

            size_t glyphPayloadSize = bank.GlyphCount * sizeof(FableGlyph);
            if (offset + glyphPayloadSize <= maxOffset) {
                bank.Glyphs.resize(bank.GlyphCount);
                memcpy(bank.Glyphs.data(), &rawData[offset], glyphPayloadSize);

                // Add the chunk to our master array so we don't have to search for it later!
                Data.AllGlyphs.insert(Data.AllGlyphs.end(), bank.Glyphs.begin(), bank.Glyphs.end());

                offset += glyphPayloadSize;
            }
            Data.GlyphBanks.push_back(bank);
        }

        if (offset + 4 <= maxOffset) {
            // Read and skip the 4-byte Payload Size prefix Fable injects
            uint32_t tgaPayloadSize = *(uint32_t*)(&rawData[offset]);
            offset += 4;

            size_t bytesToRead = tgaPayloadSize;
            if (offset + bytesToRead > maxOffset) bytesToRead = maxOffset - offset;

            if (bytesToRead > 0) {
                Data.RawTGAData.resize(bytesToRead);
                memcpy(Data.RawTGAData.data(), &rawData[offset], bytesToRead);
            }
        }

        IsParsed = true;
    }
};