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

enum class ETextureFormat { Unknown, DXT1, DXT3, DXT5, ARGB8888, NormalMap };

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
        // Uncomment to see logs in console
        // std::cout << "[TextureParser] " << msg << std::endl;
        DebugLog += msg + "\n";
    }

    // [RESTORED] Missing function that caused C2039
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

    // Helper: Calculate size of a specific mip level in bytes
    uint32_t GetMipSize(uint32_t w, uint32_t h) {
        uint32_t minDim = 1;
        uint32_t bitsPerPixel = 0;

        if (DecodedFormat == ETextureFormat::DXT1) {
            minDim = 4; bitsPerPixel = 4;
        }
        else if (DecodedFormat == ETextureFormat::DXT3 || DecodedFormat == ETextureFormat::DXT5 || DecodedFormat == ETextureFormat::NormalMap) {
            minDim = 4; bitsPerPixel = 8;
        }
        else if (DecodedFormat == ETextureFormat::ARGB8888) {
            minDim = 1; bitsPerPixel = 32;
        }

        // Fable clamps mip dimensions to minDim (4x4 for DXT)
        uint32_t currentW = (w < minDim) ? minDim : w;
        uint32_t currentH = (h < minDim) ? minDim : h;
        return (currentW * currentH * bitsPerPixel) / 8;
    }

    void DecodeFormat(bool isBump) {
        if (isBump) {
            DecodedFormat = ETextureFormat::NormalMap;
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
        uint32_t w = Header.FrameWidth;
        uint32_t h = Header.FrameHeight;
        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        for (int i = 0; i < mips; i++) {
            totalSize += GetMipSize(w, h);
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

        TrueFrameStride = CalculateTotalFrameSize();
        if (TrueFrameStride == 0) TrueFrameStride = Header.FrameDataSize;

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        size_t expectedTotalSize = (size_t)TrueFrameStride * frames;

        // [CRITICAL FIX] Heap Corruption Protection
        // Resize to Expected + 4KB padding. LZO often writes slightly past the end during operation.
        // We will resize back down at the end.
        DecodedPixels.resize(expectedTotalSize + 4096, 0);

        if (pixelData.empty()) return;

        size_t inputCursor = 0;
        size_t outputOffset = 0;
        const uint8_t* src = pixelData.data();
        size_t inputSize = pixelData.size();

        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        for (uint32_t f = 0; f < frames; f++) {
            uint32_t w = Header.FrameWidth;
            uint32_t h = Header.FrameHeight;

            for (int m = 0; m < mips; m++) {
                uint32_t mipSize = GetMipSize(w, h);

                // DISASSEMBLY LOGIC: 
                // If (m > 0 || MipSize0 == 0) -> RAW COPY (Always uncompressed)
                // If (m == 0 && MipSize0 > 0) -> COMPRESSED READ (LZO)

                bool isCompressed = (m == 0 && Header.MipSize0 > 0);

                if (!isCompressed) {
                    // --- RAW COPY ---
                    // Used for all Mips > 0, and Mip 0 if the header says size is 0 (uncompressed flag)
                    if (inputCursor + mipSize <= inputSize) {
                        memcpy(DecodedPixels.data() + outputOffset, src + inputCursor, mipSize);
                        inputCursor += mipSize;
                    }
                    else {
                        LogDebug("Error: Unexpected EOF in Raw Mip (M:" + std::to_string(m) + ")");
                        break;
                    }
                }
                else {
                    // --- LZO DECOMPRESS ---
                    // Used ONLY for Mip 0 when flagged as compressed

                    // 1. Read Chunk Header (2 or 4 bytes)
                    if (inputCursor + 2 > inputSize) break;
                    uint32_t compSize = *(uint16_t*)(src + inputCursor);
                    inputCursor += 2;
                    if (compSize == 0xFFFF) {
                        if (inputCursor + 4 > inputSize) break;
                        compSize = *(uint32_t*)(src + inputCursor);
                        inputCursor += 4;
                    }

                    // 2. Decompress
                    if (compSize == 0) {
                        // 0 Size usually implies raw copy of 'mipSize' bytes in this context
                        if (inputCursor + mipSize <= inputSize) {
                            memcpy(DecodedPixels.data() + outputOffset, src + inputCursor, mipSize);
                            inputCursor += mipSize;
                        }
                    }
                    else {
                        if (inputCursor + compSize > inputSize) break;
                        size_t outLen = 0;

                        // Note: DecodedPixels has +4096 padding, so this won't heap corrupt
                        LZO1X_Decompress(src + inputCursor, compSize, DecodedPixels.data() + outputOffset, &outLen);
                        inputCursor += compSize;

                        // 3. Fable "+3 Bytes" Fix
                        // The last 3 bytes of the texture data are stored uncompressed AFTER the LZO block.
                        // We must overwrite the end of the decompressed data with these 3 bytes.
                        if (inputCursor + 3 <= inputSize && mipSize >= 3) {
                            uint8_t* destEnd = DecodedPixels.data() + outputOffset + mipSize - 3;
                            memcpy(destEnd, src + inputCursor, 3);
                            inputCursor += 3;
                        }
                    }
                }

                outputOffset += mipSize;

                // Update dims for next mip
                if (w > 1) w >>= 1;
                if (h > 1) h >>= 1;
            }
        }

        // Restore correct size (strip safety padding)
        DecodedPixels.resize(expectedTotalSize);

        if (!DecodedPixels.empty()) IsParsed = true;
    }
};