#pragma once
#include "ImageBackend.h"
#include "TextureParser.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace DXT {
    inline void EmitAlphaBlock3(uint8_t* dest, const uint8_t* block) {
        for (int i = 0; i < 8; i++) {
            uint8_t a1 = block[(i * 2) * 4 + 3] >> 4;
            uint8_t a2 = block[(i * 2 + 1) * 4 + 3] >> 4;
            dest[i] = (a2 << 4) | a1;
        }
    }

    inline void EmitAlphaBlock5(uint8_t* dest, const uint8_t* block) {
        uint8_t minA = 255, maxA = 0;
        for (int i = 0; i < 16; i++) {
            uint8_t a = block[i * 4 + 3];
            if (a < minA) minA = a;
            if (a > maxA) maxA = a;
        }
        dest[0] = maxA;
        dest[1] = minA;

        uint64_t indices = 0;
        for (int i = 0; i < 16; i++) {
            uint8_t a = block[i * 4 + 3];
            uint64_t index = 0;
            if (maxA > minA) {
                int dist = maxA - a;
                int range = maxA - minA;
                int step = (dist * 7 + (range / 2)) / range;
                if (step == 0) index = 0;
                else if (step == 7) index = 1;
                else index = step + 1;
            }
            indices |= (index << (3 * i));
        }

        dest[2] = (indices >> 0) & 0xFF;
        dest[3] = (indices >> 8) & 0xFF;
        dest[4] = (indices >> 16) & 0xFF;
        dest[5] = (indices >> 24) & 0xFF;
        dest[6] = (indices >> 32) & 0xFF;
        dest[7] = (indices >> 40) & 0xFF;
    }
}

class TextureBuilder {
public:
struct ImportOptions {
        ETextureFormat Format = ETextureFormat::DXT3;
        bool GenerateMipmaps = true;
        int ForceMipLevels = 0;
        bool ResizeToPowerOfTwo = true;
        int TargetWidth = 0;
        int TargetHeight = 0;
        bool IsBumpmap = false;
        float BumpFactor = 5.0f;
        uint32_t PreservePixelFormatIdx = 0; // Add this
        uint8_t PreserveFlags = 0;           // Add this
    };

    struct BuildResult {
        std::vector<uint8_t> FullData;
        std::vector<uint8_t> HeaderInfo;
        uint32_t Width, Height;
        bool Success = false;
        std::string Error;
    };

    static void ConvertRGBAToFableNormalMap(std::vector<uint8_t>& rgbaPixels, int width, int height, float bumpFactor) {
        std::vector<uint8_t> normalMap(width * height * 4);

        auto getLuminance = [&](int x, int y) -> float {
            int idx = (y * width + x) * 4;
            return (rgbaPixels[idx] * 0.299f) + (rgbaPixels[idx + 1] * 0.587f) + (rgbaPixels[idx + 2] * 0.114f);
            };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float center = getLuminance(x, y);
                float right = getLuminance((x + 1) % width, y);
                float down = getLuminance(x, ((y + 1) % height));

                float du = (right - center) * bumpFactor;
                float dv = (down - center) * bumpFactor;

                du = std::clamp(du, -127.0f, 127.0f);
                dv = std::clamp(dv, -127.0f, 127.0f);

                float length = std::sqrt(du * du + dv * dv + (127.0f * 127.0f));
                float normalizer = 127.0f / length;

                uint8_t r = (uint8_t)std::clamp(std::round(du * normalizer + 128.0f), 0.0f, 255.0f);
                uint8_t g = (uint8_t)std::clamp(std::round(dv * normalizer + 128.0f), 0.0f, 255.0f);
                uint8_t b = (uint8_t)std::clamp(std::round(128.0f - (127.0f * normalizer)), 0.0f, 255.0f);
                uint8_t a = rgbaPixels[(y * width + x) * 4 + 3];

                int outIdx = (y * width + x) * 4;
                normalMap[outIdx] = r;
                normalMap[outIdx + 1] = g;
                normalMap[outIdx + 2] = b;
                normalMap[outIdx + 3] = a;
            }
        }
        rgbaPixels = normalMap;
    }

    static BuildResult ImportImage(const std::string& path, ImportOptions opts) {
        BuildResult result;
        int x, y, c;
        stbi_uc* pixels = stbi_load(path.c_str(), &x, &y, &c, 4);

        if (!pixels) {
            result.Error = "Failed to load image: " + path;
            return result;
        }

        int physW = x, physH = y;

        if (opts.TargetWidth > 0 && opts.TargetHeight > 0) {
            physW = opts.TargetWidth;
            physH = opts.TargetHeight;
        }
        else if (opts.ResizeToPowerOfTwo) {
            physW = RoundToPow2(x);
            physH = RoundToPow2(y);
        }

        std::vector<uint8_t> sourceData;
        if (physW != x || physH != y) {
            sourceData.resize(physW * physH * 4);
            stbir_resize_uint8(pixels, x, y, 0, sourceData.data(), physW, physH, 0, 4);
        }
        else {
            sourceData.assign(pixels, pixels + (x * y * 4));
        }
        stbi_image_free(pixels);

        if (opts.IsBumpmap) {
            ConvertRGBAToFableNormalMap(sourceData, physW, physH, opts.BumpFactor);
        }

        CGraphicHeader header = {};
        header.FrameWidth = physW; header.FrameHeight = physH;
        header.Width = physW; header.Height = physH;
        header.Depth = 0;
        header.FrameCount = 1;
        header.PixelFormatIdx = opts.PreservePixelFormatIdx;
        header.Flags = opts.PreserveFlags;

        CPixelFormatInit pixelFmt = {};

        switch (opts.Format) {
        case ETextureFormat::DXT1:
        case ETextureFormat::NormalMap_DXT1:
            header.TransparencyType = 0;
            pixelFmt.Type = 3;
            pixelFmt.ColourDepth = 4;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 31;
            break;

        case ETextureFormat::DXT3:
        case ETextureFormat::NormalMap_DXT5:
            header.TransparencyType = 1;
            pixelFmt.Type = 2;
            pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 32;
            break;

        case ETextureFormat::DXT5:
            header.TransparencyType = 3;
            pixelFmt.Type = 5;
            pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 35;
            break;

        case ETextureFormat::ARGB8888:
            header.TransparencyType = 255;
            pixelFmt.Type = 1;
            pixelFmt.ColourDepth = 32;
            pixelFmt.RBits = 8; pixelFmt.GBits = 8; pixelFmt.BBits = 8; pixelFmt.ABits = 8;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 3;
            break;

        default:
            header.TransparencyType = 1; pixelFmt.Type = 1; pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            break;
        }

        std::vector<uint8_t> pixelBlob;
        int mips = 1;
        if (opts.GenerateMipmaps) {
            int mw = physW, mh = physH;
            int minDim = (opts.Format == ETextureFormat::ARGB8888) ? 1 : 4;

            while (mw > minDim || mh > minDim) {
                mips++;
                mw = (mw > 1) ? mw >> 1 : 1;
                mh = (mh > 1) ? mh >> 1 : 1;
            }
        }
        if (opts.ForceMipLevels > 0) mips = opts.ForceMipLevels;

        header.MipmapLevels = mips;

        int curW = physW; int curH = physH;
        std::vector<uint8_t> currentMip = sourceData;
        uint32_t mip0Size = 0;

        for (int m = 0; m < mips; m++) {
            if (opts.Format == ETextureFormat::ARGB8888) {
                std::vector<uint8_t> bgraMip = currentMip;
                for (size_t i = 0; i < bgraMip.size() / 4; i++) {
                    std::swap(bgraMip[i * 4 + 0], bgraMip[i * 4 + 2]);
                }
                pixelBlob.insert(pixelBlob.end(), bgraMip.begin(), bgraMip.end());
                if (m == 0) mip0Size = (uint32_t)bgraMip.size();
            }
            else {
                std::vector<uint8_t> compData = CompressDXT(currentMip, curW, curH, opts.Format);
                pixelBlob.insert(pixelBlob.end(), compData.begin(), compData.end());
                if (m == 0) mip0Size = (uint32_t)compData.size();
            }

            if (m < mips - 1) {
                int nextW = (curW > 1) ? curW >> 1 : 1;
                int nextH = (curH > 1) ? curH >> 1 : 1;
                std::vector<uint8_t> nextMip(nextW * nextH * 4);

                stbir_resize_uint8(currentMip.data(), curW, curH, 0,
                    nextMip.data(), nextW, nextH, 0, 4);

                currentMip = nextMip;
                curW = nextW; curH = nextH;
            }
        }

        header.FrameDataSize = mip0Size;
        header.MipSize0 = 0;

        result.HeaderInfo.resize(sizeof(CGraphicHeader) + sizeof(CPixelFormatInit));
        memcpy(result.HeaderInfo.data(), &header, sizeof(CGraphicHeader));
        memcpy(result.HeaderInfo.data() + sizeof(CGraphicHeader), &pixelFmt, sizeof(CPixelFormatInit));

        result.FullData = pixelBlob; result.Width = physW; result.Height = physH;
        result.Success = true;

        return result;
    }

    static int RoundToPow2(int v) {
        int p = 1; while (p < v) p <<= 1; return p;
    }

    static BuildResult CompileFromRGBA(const std::vector<uint8_t>& rgba, int physW, int physH, ImportOptions opts) {
        BuildResult result;

        std::vector<uint8_t> baseRGBA = rgba;
        if (opts.IsBumpmap) {
            ConvertRGBAToFableNormalMap(baseRGBA, physW, physH, opts.BumpFactor);
        }

        CGraphicHeader header = {};
        header.FrameWidth = physW; header.FrameHeight = physH;
        header.Width = physW; header.Height = physH;
        header.Depth = 1; header.FrameCount = 1;
        header.PixelFormatIdx = opts.PreservePixelFormatIdx;
        header.Flags = opts.PreserveFlags;

        CPixelFormatInit pixelFmt = {};

        switch (opts.Format) {
        case ETextureFormat::DXT1:
            header.TransparencyType = 0; pixelFmt.Type = 1; pixelFmt.ColourDepth = 4;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 14;
            break;
        case ETextureFormat::NormalMap_DXT1:
            header.TransparencyType = 0; pixelFmt.Type = 3; pixelFmt.ColourDepth = 4;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 31;
            break;
        case ETextureFormat::DXT3:
            header.TransparencyType = 1; pixelFmt.Type = 1; pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 15;
            break;
        case ETextureFormat::DXT5:
            header.TransparencyType = 3; pixelFmt.Type = 1; pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 16;
            break;
        case ETextureFormat::NormalMap_DXT5:
            header.TransparencyType = 1; pixelFmt.Type = 3; pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 32;
            break;
        case ETextureFormat::ARGB8888:
            header.TransparencyType = 255; pixelFmt.Type = 1; pixelFmt.ColourDepth = 32;
            pixelFmt.RBits = 8; pixelFmt.GBits = 8; pixelFmt.BBits = 8; pixelFmt.ABits = 8;
            if (header.PixelFormatIdx == 0) header.PixelFormatIdx = 3;
            break;
        default:
            header.TransparencyType = 1; pixelFmt.Type = 1; pixelFmt.ColourDepth = 8;
            pixelFmt.RBits = 0; pixelFmt.GBits = 0; pixelFmt.BBits = 0; pixelFmt.ABits = 0;
            break;
        }

        std::vector<uint8_t> pixelBlob;
        int mips = 1;
        if (opts.GenerateMipmaps) {
            int mw = physW, mh = physH;
            int minDim = (opts.Format == ETextureFormat::ARGB8888) ? 1 : 4;

            while (mw > minDim || mh > minDim) {
                mips++;
                mw = (mw > 1) ? mw >> 1 : 1;
                mh = (mh > 1) ? mh >> 1 : 1;
            }
        }
        if (opts.ForceMipLevels > 0) mips = opts.ForceMipLevels;
        header.MipmapLevels = mips;

        int curW = physW, curH = physH;
        std::vector<uint8_t> currentMip = baseRGBA;
        uint32_t mip0Size = 0;

        for (int m = 0; m < mips; m++) {
            if (opts.Format == ETextureFormat::ARGB8888) {
                std::vector<uint8_t> bgraMip = currentMip;
                for (size_t i = 0; i < bgraMip.size() / 4; i++) {
                    std::swap(bgraMip[i * 4 + 0], bgraMip[i * 4 + 2]);
                }
                pixelBlob.insert(pixelBlob.end(), bgraMip.begin(), bgraMip.end());
                if (m == 0) mip0Size = (uint32_t)bgraMip.size();
            }
            else {
                std::vector<uint8_t> compData = CompressDXT(currentMip, curW, curH, opts.Format);
                pixelBlob.insert(pixelBlob.end(), compData.begin(), compData.end());
                if (m == 0) mip0Size = (uint32_t)compData.size();
            }

            if (m < mips - 1) {
                int nextW = (curW > 1) ? curW >> 1 : 1;
                int nextH = (curH > 1) ? curH >> 1 : 1;
                std::vector<uint8_t> nextMip(nextW * nextH * 4);
                stbir_resize_uint8(currentMip.data(), curW, curH, 0, nextMip.data(), nextW, nextH, 0, 4);
                currentMip = nextMip; curW = nextW; curH = nextH;
            }
        }

        header.FrameDataSize = mip0Size;
        header.MipSize0 = 0;

        result.HeaderInfo.resize(sizeof(CGraphicHeader) + sizeof(CPixelFormatInit));
        memcpy(result.HeaderInfo.data(), &header, sizeof(CGraphicHeader));
        memcpy(result.HeaderInfo.data() + sizeof(CGraphicHeader), &pixelFmt, sizeof(CPixelFormatInit));

        result.FullData = pixelBlob;
        result.Width = physW; result.Height = physH;
        result.Success = true;
        return result;
    }

private:

    static std::vector<uint8_t> CompressDXT(const std::vector<uint8_t>& rgba, int w, int h, ETextureFormat fmt) {
        int blocksX = (w + 3) / 4;
        int blocksY = (h + 3) / 4;
        int blockSize = (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1) ? 8 : 16;

        std::vector<uint8_t> output;
        output.resize(blocksX * blocksY * blockSize);

        uint8_t block[64];

        for (int y = 0; y < blocksY; y++) {
            for (int x = 0; x < blocksX; x++) {

                for (int by = 0; by < 4; by++) {
                    for (int bx = 0; bx < 4; bx++) {
                        int sx = (std::min)(x * 4 + bx, w - 1);
                        int sy = (std::min)(y * 4 + by, h - 1);
                        int srcIdx = (sy * w + sx) * 4;
                        memcpy(&block[(by * 4 + bx) * 4], &rgba[srcIdx], 4);
                    }
                }

                uint8_t* dest = &output[(y * blocksX + x) * blockSize];

                if (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1) {
                    stb_compress_dxt_block(dest, block, 0, STB_DXT_HIGHQUAL);
                }
                else if (fmt == ETextureFormat::DXT3) {
                    DXT::EmitAlphaBlock3(dest, block);
                    stb_compress_dxt_block(dest + 8, block, 0, STB_DXT_HIGHQUAL);
                }
                else if (fmt == ETextureFormat::DXT5 || fmt == ETextureFormat::NormalMap_DXT5) {
                    DXT::EmitAlphaBlock5(dest, block);
                    stb_compress_dxt_block(dest + 8, block, 0, STB_DXT_HIGHQUAL);
                }
            }
        }
        return output;
    }
};