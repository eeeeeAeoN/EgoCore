// Gemini/TextureParser.h
#pragma once
#include "Utils.h"
#include <vector>
#include <string>

#pragma pack(push, 1)
// Define this BEFORE CTextureParser uses it
struct CPixelFormatInit {
    uint8_t Type;
    uint8_t ColourDepth;
    uint8_t RBits, GBits, BBits, ABits;
};

struct CGraphicHeader {
    uint16_t Width, Height, Depth;
    uint16_t FrameWidth, FrameHeight, FrameCount;
    uint32_t PixelFormatIdx;
    uint8_t  TransparencyType, MipmapLevels, Flags, Padding;
    uint32_t FrameDataSize, MipSize0;
};
#pragma pack(pop)

enum class ETextureFormat { Unknown, DXT1, DXT3, DXT5, ARGB8888, NormalMap };

class CTextureParser {
public:
    CGraphicHeader Header;
    CPixelFormatInit FormatInfo;
    std::vector<uint32_t> MipSizes;
    ETextureFormat DecodedFormat = ETextureFormat::Unknown;
    std::vector<uint8_t> DecodedPixels;
    bool IsParsed = false;
    std::string DebugLog;

    std::string GetFormatString() {
        switch (DecodedFormat) {
        case ETextureFormat::DXT1: return "DXT1";
        case ETextureFormat::DXT3: return "DXT3";
        case ETextureFormat::DXT5: return "DXT5";
        case ETextureFormat::ARGB8888: return "ARGB8888";
        case ETextureFormat::NormalMap: return "BUMP/Normal";
        default: return "Unknown";
        }
    }

    void Parse(const std::vector<uint8_t>& metadata, const std::vector<uint8_t>& pixelData) {
        IsParsed = false;
        MipSizes.clear();
        DecodedPixels.clear();
        DebugLog = "";

        if (metadata.size() < 28) return;
        memcpy(&Header, metadata.data(), 28);
        if (metadata.size() >= 34) memcpy(&FormatInfo, metadata.data() + 28, 6);

        DecodeFormat();

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        size_t totalUncompressedSize = (size_t)Header.FrameDataSize * frames;

        if (pixelData.empty()) return;

        // Use size mismatch to detect if we need the multi-block LZO path
        if (pixelData.size() == totalUncompressedSize) {
            DecodedPixels = pixelData;
            DebugLog = "Loaded raw sequence data.";
        }
        else {
            size_t cursor = 0;
            DecodedPixels = DecompressLZO(pixelData.data(), cursor, pixelData.size(), totalUncompressedSize);
            DebugLog = "Decompressed multi-block stream.";
        }

        if (!DecodedPixels.empty()) IsParsed = true;
    }

private:
    void DecodeFormat() {
        // Direct mapping from the transparency types in the bank assembly
        if (Header.TransparencyType == 0) DecodedFormat = ETextureFormat::DXT1;
        else if (Header.TransparencyType == 1) DecodedFormat = ETextureFormat::DXT3;
        else if (Header.TransparencyType == 3) DecodedFormat = ETextureFormat::DXT5;
        else if (FormatInfo.ColourDepth == 32) DecodedFormat = ETextureFormat::ARGB8888;
        else DecodedFormat = ETextureFormat::Unknown;
    }
};