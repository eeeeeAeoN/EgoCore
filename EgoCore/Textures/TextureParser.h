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

enum class ETextureFormat { Unknown, DXT1, DXT3, DXT5, ARGB8888, NormalMap_DXT1, NormalMap_DXT5 };

struct Color32 { uint8_t r, g, b, a; };

class TextureUtils {
public:
    static void GetColorBlockColors(Color32* colors, const uint8_t* block) {
        uint16_t c0; memcpy(&c0, block, 2);
        uint16_t c1; memcpy(&c1, block + 2, 2);

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
        uint32_t indices; memcpy(&indices, block + 4, 4);

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
            uint16_t alphaRow; memcpy(&alphaRow, block + 2 * y, 2);
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

        uint64_t fullBlock; memcpy(&fullBlock, block, 8);
        uint64_t alphaMask = fullBlock >> 16;

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

    static std::vector<uint8_t> DecompressFrameToRGBA(const uint8_t* rawData, uint32_t width, uint32_t height, ETextureFormat fmt) {
        std::vector<uint8_t> rgba(width * height * 4);
        Color32* output = (Color32*)rgba.data();

        if (fmt == ETextureFormat::ARGB8888) {
            memcpy(rgba.data(), rawData, rgba.size());
            return rgba;
        }

        uint32_t blocksX = (width + 3) / 4;
        uint32_t blocksY = (height + 3) / 4;
        uint32_t blockSize = (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1) ? 8 : 16;

        for (uint32_t y = 0; y < blocksY; y++) {
            for (uint32_t x = 0; x < blocksX; x++) {
                Color32 blockOut[16];
                const uint8_t* blockSrc = rawData + (y * blocksX + x) * blockSize;

                if (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1) DecompressDXT1Block(blockSrc, blockOut, 4);
                else if (fmt == ETextureFormat::DXT3) DecompressDXT3Block(blockSrc, blockOut, 4);
                else DecompressDXT5Block(blockSrc, blockOut, 4);

                for (int py = 0; py < 4; py++) {
                    for (int px = 0; px < 4; px++) {
                        if (x * 4 + px < width && y * 4 + py < height) {
                            output[(y * 4 + py) * width + (x * 4 + px)] = blockOut[py * 4 + px];
                        }
                    }
                }
            }
        }
        return rgba;
    }
};

class CTextureParser {
public:
    CGraphicHeader Header;
    CPixelFormatInit FormatInfo;
    ETextureFormat DecodedFormat = ETextureFormat::Unknown;
    std::vector<uint8_t> DecodedPixels;       // Used for binary DXT reading
    std::vector<std::vector<uint8_t>> RawFrames; // NEW: Used for Staged RGBA editing
    bool IsParsed = false;
    bool IsStagedRaw = false;
    std::string DebugLog;
    uint32_t TrueFrameStride = 0;
    std::string PendingName;

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
        bool isUncompressed = (FormatInfo.ColourDepth == 32);

        if (isUncompressed) {
            DecodedFormat = ETextureFormat::ARGB8888;
            return;
        }

        if (isBump) {
            switch (Header.TransparencyType) {
            case 0: case 2: DecodedFormat = ETextureFormat::NormalMap_DXT1; break;
            default:        DecodedFormat = ETextureFormat::NormalMap_DXT5; break;
            }
        }
        else {
            switch (Header.TransparencyType) {
            case 0: DecodedFormat = ETextureFormat::DXT1; break;
            case 1: DecodedFormat = ETextureFormat::DXT3; break;
            case 2: DecodedFormat = ETextureFormat::DXT1; break;
            case 3: DecodedFormat = ETextureFormat::DXT5; break;
            case 4: DecodedFormat = ETextureFormat::DXT3; break;
            default: DecodedFormat = ETextureFormat::DXT1; break;
            }
        }
    }

    uint32_t CalculateTotalFrameSize() {
        uint32_t totalSize = 0;
        uint32_t w = Header.Width;
        uint32_t h = Header.Height;

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
        IsStagedRaw = false;
        DecodedPixels.clear();
        RawFrames.clear(); // <--- Add this clear
        PendingName = "";

        if (metadata.size() < 28) return;

        memcpy(&Header, metadata.data(), 28);

        if (metadata.size() >= 34) memcpy(&FormatInfo, metadata.data() + 28, 6);
        else FormatInfo = { 0, 0, 0, 0, 0, 0 };

        bool isBump = (entryType == 0x2 || entryType == 0x3);
        DecodeFormat(isBump);

        TrueFrameStride = CalculateTotalFrameSize();
        if (TrueFrameStride < Header.FrameDataSize) TrueFrameStride = Header.FrameDataSize;

        uint32_t frames = (Header.FrameCount > 0) ? Header.FrameCount : 1;
        size_t expectedTotalSize = (size_t)TrueFrameStride * frames;

        // Pad to guarantee decompression write safety
        DecodedPixels.resize(expectedTotalSize + 65536, 0);

        if (pixelData.empty()) return;

        size_t inputCursor = 0;
        size_t outputOffset = 0;
        const uint8_t* src = pixelData.data();
        size_t inputSize = pixelData.size();

        int mips = (Header.MipmapLevels > 0) ? Header.MipmapLevels : 1;

        for (uint32_t f = 0; f < frames; f++) {
            uint32_t w = Header.Width ? Header.Width : Header.FrameWidth;
            uint32_t h = Header.Height ? Header.Height : Header.FrameHeight;
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
                    size_t bytesDecompressedForMip = 0;
                    size_t lzoTarget = (mipVolumeSize > 3) ? (mipVolumeSize - 3) : 0;

                    while (bytesDecompressedForMip < lzoTarget) {
                        if (inputCursor + 2 > inputSize) break;

                        uint16_t compSize16; memcpy(&compSize16, src + inputCursor, 2);
                        inputCursor += 2;

                        uint32_t compSize = compSize16;
                        if (compSize == 0xFFFF) {
                            if (inputCursor + 4 > inputSize) break;
                            memcpy(&compSize, src + inputCursor, 4);
                            inputCursor += 4;
                        }

                        if (compSize == 0) {
                            size_t remaining = lzoTarget - bytesDecompressedForMip;
                            size_t toCopy = (std::min)(remaining, (size_t)(inputSize - inputCursor));
                            memcpy(DecodedPixels.data() + outputOffset + bytesDecompressedForMip, src + inputCursor, toCopy);
                            inputCursor += toCopy;
                            bytesDecompressedForMip += toCopy;
                        }
                        else {
                            if (inputCursor + compSize > inputSize) break;

                            size_t outLen = 0;
                            size_t maxOutput = DecodedPixels.size() - (outputOffset + bytesDecompressedForMip);

                            int ret = LZO1X_Decompress_Safe(
                                src + inputCursor,
                                compSize,
                                DecodedPixels.data() + outputOffset + bytesDecompressedForMip,
                                maxOutput,
                                &outLen
                            );

                            inputCursor += compSize;
                            bytesDecompressedForMip += outLen;

                            if (ret != 0 || outLen == 0) break;
                        }
                    }

                    if (inputCursor + 3 <= inputSize && mipVolumeSize >= 3) {
                        uint8_t* destEnd = DecodedPixels.data() + outputOffset + mipVolumeSize - 3;
                        memcpy(destEnd, src + inputCursor, 3);
                        inputCursor += 3;
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