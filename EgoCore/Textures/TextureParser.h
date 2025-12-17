#pragma once
#include "Utils.h"
#include <vector>
#include <string>

#pragma pack(push, 1)
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
    uint32_t FrameDataSize, MipSize0; // MipSize0 is CompressedMipmapSizes[0] from disassembly
};
#pragma pack(pop)

enum class ETextureFormat { Unknown, DXT1, DXT3, DXT5, ARGB8888, NormalMap };

class CTextureParser {
public:
    CGraphicHeader Header;
    CPixelFormatInit FormatInfo;
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

    void DecodeFormat(bool isBump) {
        // Disassembly: CGraphicDataBank::GetPixelFormats + GetDXTCFormat
        if (isBump) {
            DecodedFormat = ETextureFormat::NormalMap;
            return;
        }

        switch (Header.TransparencyType) {
        case 0: DecodedFormat = ETextureFormat::DXT1; break; // NonAlpha
        case 1:
        case 4: DecodedFormat = ETextureFormat::DXT3; break; // Alpha/Explicit
        case 2: DecodedFormat = ETextureFormat::DXT1; break; // Boolean (DXT1 1-bit)
        case 3: DecodedFormat = ETextureFormat::DXT5; break; // Interpolated
        default:
            if (FormatInfo.ColourDepth == 32) DecodedFormat = ETextureFormat::ARGB8888;
            else DecodedFormat = ETextureFormat::Unknown;
            break;
        }
    }

    void Parse(const std::vector<uint8_t>& metadata, const std::vector<uint8_t>& pixelData, int32_t entryType) {
        IsParsed = false;
        DecodedPixels.clear();
        DebugLog = "";

        if (metadata.size() < 28) return;
        memcpy(&Header, metadata.data(), 28);
        if (metadata.size() >= 34) memcpy(&FormatInfo, metadata.data() + 28, 6);

        bool isBump = (entryType == 0x2 || entryType == 0x3);
        DecodeFormat(isBump);

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        // FrameDataSize is the uncompressed size of a SINGLE frame (all mips)
        size_t totalUncompressedSize = (size_t)Header.FrameDataSize * frames;

        if (pixelData.empty()) return;

        // Disassembly: Sequences are written RAW, Mip0 of single textures is compressed
        if (pixelData.size() == totalUncompressedSize || Header.FrameCount > 1) {
            DecodedPixels = pixelData;
            DebugLog = "Loaded raw pixel stream.";
        }
        else {
            size_t cursor = 0;
            DecodedPixels = DecompressLZO(pixelData.data(), cursor, pixelData.size(), totalUncompressedSize);
            DebugLog = "Decompressed Mip0 LZO block.";
        }

        if (!DecodedPixels.empty()) IsParsed = true;
    }
};