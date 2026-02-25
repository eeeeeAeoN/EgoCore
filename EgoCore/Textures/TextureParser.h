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

struct Color32 { uint8_t r, g, b, a; };

class TextureUtils {
public:
    static void GetColorBlockColors(Color32* colors, const uint8_t* block) {
        uint16_t c0 = *(uint16_t*)(block);
        uint16_t c1 = *(uint16_t*)(block + 2);

        colors[0].r = (c0 >> 11) * 255 / 31;
        colors[0].g = ((c0 >> 5) & 0x3F) * 255 / 63;
        colors[0].b = (c0 & 0x1F) * 255 / 31;
        colors[0].a = 255;

        colors[1].r = (c1 >> 11) * 255 / 31;
        colors[1].g = ((c1 >> 5) & 0x3F) * 255 / 63;
        colors[1].b = (c1 & 0x1F) * 255 / 31;
        colors[1].a = 255;

        if (c0 > c1) {
            colors[2].r = (2 * colors[0].r + colors[1].r) / 3;
            colors[2].g = (2 * colors[0].g + colors[1].g) / 3;
            colors[2].b = (2 * colors[0].b + colors[1].b) / 3;
            colors[2].a = 255;

            colors[3].r = (colors[0].r + 2 * colors[1].r) / 3;
            colors[3].g = (colors[0].g + 2 * colors[1].g) / 3;
            colors[3].b = (colors[0].b + 2 * colors[1].b) / 3;
            colors[3].a = 255;
        }
        else {
            colors[2].r = (colors[0].r + colors[1].r) / 2;
            colors[2].g = (colors[0].g + colors[1].g) / 2;
            colors[2].b = (colors[0].b + colors[1].b) / 2;
            colors[2].a = 255;

            colors[3].r = 0; colors[3].g = 0; colors[3].b = 0; colors[3].a = 0;
        }
    }

    static void DecompressDXT1Block(const uint8_t* block, Color32* output, uint32_t stride) {
        Color32 colors[4];
        GetColorBlockColors(colors, block);
        uint32_t indices = *(uint32_t*)(block + 4);

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                uint8_t idx = (indices >> (2 * (4 * y + x))) & 0x03;
                output[y * stride + x] = colors[idx];
            }
        }
    }

    static void DecompressDXT3Block(const uint8_t* block, Color32* output, uint32_t stride) {
        DecompressDXT1Block(block + 8, output, stride);
        for (int y = 0; y < 4; y++) {
            uint16_t alphaRow = *(uint16_t*)(block + 2 * y);
            for (int x = 0; x < 4; x++) {
                uint8_t alpha = (alphaRow >> (4 * x)) & 0x0F;
                output[y * stride + x].a = (alpha * 255) / 15;
            }
        }
    }

    static void DecompressDXT5Block(const uint8_t* block, Color32* output, uint32_t stride) {
        DecompressDXT1Block(block + 8, output, stride);
        uint8_t a0 = block[0];
        uint8_t a1 = block[1];
        uint64_t alphaMask = (*(uint64_t*)(block)) >> 16;

        uint8_t alphas[8];
        alphas[0] = a0;
        alphas[1] = a1;
        if (a0 > a1) {
            for (int i = 2; i < 8; i++) alphas[i] = ((8 - i) * a0 + (i - 1) * a1) / 7;
        }
        else {
            for (int i = 2; i < 6; i++) alphas[i] = ((6 - i) * a0 + (i - 1) * a1) / 5;
            alphas[6] = 0;
            alphas[7] = 255;
        }

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                uint8_t idx = (uint8_t)((alphaMask >> (3 * (4 * y + x))) & 0x07);
                output[y * stride + x].a = alphas[idx];
            }
        }
    }
};

class CTextureParser {
public:
    CGraphicHeader Header;
    CPixelFormatInit FormatInfo;
    ETextureFormat DecodedFormat = ETextureFormat::Unknown;
    std::vector<uint8_t> DecodedPixels;
    bool IsParsed = false;
    std::string DebugLog;
    uint32_t TrueFrameStride = 0;
    std::string PendingName;

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
        // [FIX] MATCHING GAME ASSEMBLY LOGIC (CGraphicDataBank::GetTextureType)
        // 1. Check Compression Status first (heuristic via ColourDepth).
        // 32-bit is definitively Uncompressed ARGB in Fable 1 contexts.
        bool isUncompressed = (FormatInfo.ColourDepth == 32);

        if (isUncompressed) {
            // If it's bump mapped but uncompressed, Fable treats it as specialized uncompressed bump.
            // For viewer purposes, treating as ARGB8888 is usually correct for raw visualization.
            DecodedFormat = ETextureFormat::ARGB8888;
            return;
        }

        // 2. If Compressed, switch on TransparencyType
        if (isBump) {
            switch (Header.TransparencyType) {
            case 0: case 2: DecodedFormat = ETextureFormat::NormalMap_DXT1; break;
            default:        DecodedFormat = ETextureFormat::NormalMap_DXT5; break;
            }
        }
        else {
            switch (Header.TransparencyType) {
            case 0: DecodedFormat = ETextureFormat::DXT1; break;
            case 1: DecodedFormat = ETextureFormat::DXT3; break; // Explicit Alpha (4-bit)
            case 2: DecodedFormat = ETextureFormat::DXT1; break; // 1-bit Alpha
            case 3: DecodedFormat = ETextureFormat::DXT5; break; // Interpolated Alpha
            case 4: DecodedFormat = ETextureFormat::DXT3; break; // Unknown, typically DXT3 in fallback
            default:
                // Fallback for weird types, but we already handled 32-bit above.
                // If it ends up here with < 32 bits and unknown type, default to DXT1 or Unknown.
                DecodedFormat = ETextureFormat::DXT1;
                break;
            }
        }
    }

    uint32_t CalculateTotalFrameSize() {
        uint32_t totalSize = 0;
        // Use Physical Dimensions (Width/Height) not Logical (FrameWidth)
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
        PendingName = "";

        if (metadata.size() < 28) return;
        memcpy(&Header, metadata.data(), 28);

        // Ensure we actually have the PixelFormatInit struct (6 bytes)
        if (metadata.size() >= 34) {
            memcpy(&FormatInfo, metadata.data() + 28, 6);
        }
        else {
            // Fallback if missing (shouldn't happen in standard .BIG files)
            FormatInfo = { 0, 0, 0, 0, 0, 0 };
        }

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
            // Use Physical Dimensions for Loop state
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
                    // LZO Header handling
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

                        // LZO Trailing Bytes
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