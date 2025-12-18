#pragma once
#include "Utils.h"
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

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

enum class ETextureFormat { Unknown, DXT1, DXT3, DXT5, ARGB8888, NormalMap_DXT1, NormalMap_DXT5 };

class CTextureParser {
public:
    CGraphicHeader Header;
    CPixelFormatInit FormatInfo;
    ETextureFormat DecodedFormat = ETextureFormat::Unknown;
    std::vector<uint8_t> DecodedPixels;
    bool IsParsed = false;
    std::string DebugLog;

    uint32_t TrueFrameStride = 0;

    void LogDebug(const std::string& msg) {
        DebugLog += msg + "\n";
    }

    std::string GetFormatString() {
        switch (DecodedFormat) {
        case ETextureFormat::DXT1: return "DXT1";
        case ETextureFormat::DXT3: return "DXT3";
        case ETextureFormat::DXT5: return "DXT5";
        case ETextureFormat::ARGB8888: return "ARGB8888";
        case ETextureFormat::NormalMap_DXT1: return "BUMP (DXT1)";
        case ETextureFormat::NormalMap_DXT5: return "BUMP (DXT5)";
        default: return "Unknown";
        }
    }

    uint32_t GetMipSize(uint32_t w, uint32_t h, uint32_t d) {
        uint32_t minDim = 1;
        uint32_t bitsPerPixel = 0;

        if (DecodedFormat == ETextureFormat::DXT1 || DecodedFormat == ETextureFormat::NormalMap_DXT1) {
            minDim = 4; bitsPerPixel = 4;
        }
        else if (DecodedFormat == ETextureFormat::DXT3 || DecodedFormat == ETextureFormat::DXT5 || DecodedFormat == ETextureFormat::NormalMap_DXT5) {
            minDim = 4; bitsPerPixel = 8;
        }
        else if (DecodedFormat == ETextureFormat::ARGB8888) {
            minDim = 1; bitsPerPixel = 32;
        }

        uint32_t currentW = (w < minDim) ? minDim : w;
        uint32_t currentH = (h < minDim) ? minDim : h;
        uint32_t currentD = (d < 1) ? 1 : d;

        return ((currentW * currentH * bitsPerPixel) / 8) * currentD;
    }

    void DecodeFormat(bool isBump) {
        if (isBump) {
            switch (Header.TransparencyType) {
            case 0: case 2: DecodedFormat = ETextureFormat::NormalMap_DXT1; break;
            default:        DecodedFormat = ETextureFormat::NormalMap_DXT5; break;
            }
            return;
        }

        switch (Header.TransparencyType) {
        case 0: DecodedFormat = ETextureFormat::DXT1; break;
        case 1: DecodedFormat = ETextureFormat::DXT3; break;
        case 4: DecodedFormat = ETextureFormat::DXT3; break;
        case 2: DecodedFormat = ETextureFormat::DXT1; break;
        case 3: DecodedFormat = ETextureFormat::DXT5; break;
        default:
            if (FormatInfo.ColourDepth == 32) DecodedFormat = ETextureFormat::ARGB8888;
            else DecodedFormat = ETextureFormat::Unknown;
            break;
        }
    }

    uint32_t CalculateTotalFrameSize() {
        uint32_t totalSize = 0;
        // [FIX] CRITICAL: Use Physical Dimensions (Width/Height) not Logical (FrameWidth)
        // Memory is allocated based on the power-of-two container.
        uint32_t w = Header.Width;
        uint32_t h = Header.Height;

        // Sanity Check: If Width is 0 (rare), fallback to FrameWidth
        if (w == 0) w = Header.FrameWidth;
        if (h == 0) h = Header.FrameHeight;

        uint32_t d = (Header.Depth > 0) ? Header.Depth : 1;
        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        for (int i = 0; i < mips; i++) {
            totalSize += GetMipSize(w, h, d);
            if (w > 1) w >>= 1;
            if (h > 1) h >>= 1;
            if (d > 1) d >>= 1;
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

        TrueFrameStride = CalculateTotalFrameSize();
        // Fallback: If calc is smaller than header claim, trust header (prevents truncation)
        if (TrueFrameStride < Header.FrameDataSize) TrueFrameStride = Header.FrameDataSize;

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        size_t expectedTotalSize = (size_t)TrueFrameStride * frames;

        // Large Padding for LZO safety (prevents overflow on bad decompression)
        DecodedPixels.resize(expectedTotalSize + 65536, 0);

        if (pixelData.empty()) return;

        size_t inputCursor = 0;
        size_t outputOffset = 0;
        const uint8_t* src = pixelData.data();
        size_t inputSize = pixelData.size();

        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        for (uint32_t f = 0; f < frames; f++) {
            // [FIX] Use Physical Dimensions for Loop state
            uint32_t w = Header.Width;
            uint32_t h = Header.Height;
            if (w == 0) w = Header.FrameWidth;
            if (h == 0) h = Header.FrameHeight;

            uint32_t d = (Header.Depth > 0) ? Header.Depth : 1;

            for (int m = 0; m < mips; m++) {
                uint32_t mipVolumeSize = GetMipSize(w, h, d);
                bool isCompressed = (m == 0 && Header.MipSize0 > 0);

                if (!isCompressed) {
                    if (inputCursor + mipVolumeSize <= inputSize) {
                        memcpy(DecodedPixels.data() + outputOffset, src + inputCursor, mipVolumeSize);
                        inputCursor += mipVolumeSize;
                    }
                }
                else {
                    if (inputCursor + 2 > inputSize) break;
                    uint32_t compSize = *(uint16_t*)(src + inputCursor);
                    inputCursor += 2;
                    if (compSize == 0xFFFF) {
                        if (inputCursor + 4 > inputSize) break;
                        compSize = *(uint32_t*)(src + inputCursor);
                        inputCursor += 4;
                    }

                    if (compSize == 0) {
                        if (inputCursor + mipVolumeSize <= inputSize) {
                            memcpy(DecodedPixels.data() + outputOffset, src + inputCursor, mipVolumeSize);
                            inputCursor += mipVolumeSize;
                        }
                    }
                    else {
                        if (inputCursor + compSize > inputSize) break;
                        size_t outLen = 0;
                        LZO1X_Decompress(src + inputCursor, compSize, DecodedPixels.data() + outputOffset, &outLen);
                        inputCursor += compSize;

                        if (inputCursor + 3 <= inputSize && mipVolumeSize >= 3) {
                            uint8_t* destEnd = DecodedPixels.data() + outputOffset + mipVolumeSize - 3;
                            memcpy(destEnd, src + inputCursor, 3);
                            inputCursor += 3;
                        }
                    }
                }
                outputOffset += mipVolumeSize;

                if (w > 1) w >>= 1;
                if (h > 1) h >>= 1;
                if (d > 1) d >>= 1;
            }
        }
        DecodedPixels.resize(expectedTotalSize);
        if (!DecodedPixels.empty()) IsParsed = true;
    }
};