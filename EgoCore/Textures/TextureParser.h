#pragma once
#include "Utils.h"
#include <vector>
#include <string>
#include <algorithm>

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
    uint32_t FrameDataSize, MipSize0;
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

    // Use this for striding, NOT Header.FrameDataSize
    uint32_t TrueFrameStride = 0;

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
        if (isBump) {
            DecodedFormat = ETextureFormat::NormalMap;
            return;
        }

        switch (Header.TransparencyType) {
        case 0: DecodedFormat = ETextureFormat::DXT1; break;
        case 1:
        case 4: DecodedFormat = ETextureFormat::DXT3; break;
        case 2: DecodedFormat = ETextureFormat::DXT1; break;
        case 3: DecodedFormat = ETextureFormat::DXT5; break;
        default:
            if (FormatInfo.ColourDepth == 32) DecodedFormat = ETextureFormat::ARGB8888;
            else DecodedFormat = ETextureFormat::Unknown;
            break;
        }
    }

    //
    uint32_t CalculateTotalFrameSize() {
        uint32_t totalSize = 0;
        uint32_t w = Header.FrameWidth;
        uint32_t h = Header.FrameHeight;
        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        // "MinDimension" from CTextureManager::CalculateTextureSize
        uint32_t minDim = 1;
        uint32_t bitsPerPixel = 0;

        if (DecodedFormat == ETextureFormat::DXT1) {
            minDim = 4;
            bitsPerPixel = 4; // 4 bits per pixel (effectively 8 bytes per 4x4 block)
        }
        else if (DecodedFormat == ETextureFormat::DXT3 || DecodedFormat == ETextureFormat::DXT5 || DecodedFormat == ETextureFormat::NormalMap) {
            minDim = 4;
            bitsPerPixel = 8; // 8 bits per pixel (effectively 16 bytes per 4x4 block)
        }
        else if (DecodedFormat == ETextureFormat::ARGB8888) {
            minDim = 1;
            bitsPerPixel = 32;
        }

        for (int i = 0; i < mips; i++) {
            // Apply Fable's specific clamping logic
            uint32_t currentW = (w < minDim) ? minDim : w;
            uint32_t currentH = (h < minDim) ? minDim : h;

            uint32_t mipArea = currentW * currentH;
            uint32_t mipSize = (mipArea * bitsPerPixel) / 8; // Convert bits to bytes

            totalSize += mipSize;

            if (w > 1) w >>= 1;
            if (h > 1) h >>= 1;
        }
        return totalSize;
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

        // --- FIX: Calculate the full size of mips for buffer allocation ---
        TrueFrameStride = CalculateTotalFrameSize();
        if (TrueFrameStride == 0) TrueFrameStride = Header.FrameDataSize;

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        size_t totalUncompressedSize = (size_t)TrueFrameStride * frames;

        if (pixelData.empty()) return;

        // If data is already uncompressed, copy it. Otherwise decompress to FULL size.
        if (pixelData.size() >= totalUncompressedSize) {
            DecodedPixels = pixelData;
            DebugLog = "Loaded raw pixel stream.";
        }
        else {
            size_t cursor = 0;
            // DecompressLZO will now have enough room for all mips
            DecodedPixels = DecompressLZO(pixelData.data(), cursor, pixelData.size(), totalUncompressedSize);
            DebugLog = "Decompressed LZO stream.";
        }

        if (!DecodedPixels.empty()) IsParsed = true;
    }
};