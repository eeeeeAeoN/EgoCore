#pragma once
#include "ImageBackend.h"
#include "TextureParser.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// --- DXT COMPRESSION HELPERS ---
namespace DXT {
    // DXT3 Alpha is 4-bits per pixel (explicit)
    inline void EmitAlphaBlock3(uint8_t* dest, const uint8_t* block) {
        for (int i = 0; i < 8; i++) {
            uint8_t a1 = block[(i * 2) * 4 + 3] >> 4;
            uint8_t a2 = block[(i * 2 + 1) * 4 + 3] >> 4;
            dest[i] = (a2 << 4) | a1;
        }
    }

    // DXT5 Alpha uses 8 interpolated steps and 3-bit indices
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
                // Calculate which of the 8 steps we are closest to
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
        int TargetWidth = 0;   // [NEW]
        int TargetHeight = 0;  // [NEW]
    };

    struct BuildResult {
        std::vector<uint8_t> FullData;
        std::vector<uint8_t> HeaderInfo;
        uint32_t Width, Height;
        bool Success = false;
        std::string Error;
    };

    static BuildResult ImportImage(const std::string& path, ImportOptions opts) {
        BuildResult result;
        int x, y, c;
        stbi_uc* pixels = stbi_load(path.c_str(), &x, &y, &c, 4);

        if (!pixels) {
            result.Error = "Failed to load image: " + path;
            return result;
        }

        int physW = x, physH = y;

        // Force resize to target dimensions if provided
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

        CGraphicHeader header = {};
        header.FrameWidth = x;
        header.FrameHeight = y;
        header.Width = physW;
        header.Height = physH;
        header.Depth = 1;
        header.FrameCount = 1;

        CPixelFormatInit pixelFmt = {};
        pixelFmt.Type = 1;

        switch (opts.Format) {
        case ETextureFormat::DXT1: header.TransparencyType = 0; pixelFmt.ColourDepth = 4; break;
        case ETextureFormat::DXT3: header.TransparencyType = 1; pixelFmt.ColourDepth = 8; break;
        case ETextureFormat::DXT5: header.TransparencyType = 3; pixelFmt.ColourDepth = 8; break;
        case ETextureFormat::ARGB8888: header.TransparencyType = 255; pixelFmt.ColourDepth = 32; break;
        default: header.TransparencyType = 1; pixelFmt.ColourDepth = 8; break;
        }

        pixelFmt.RBits = 8; pixelFmt.GBits = 8; pixelFmt.BBits = 8; pixelFmt.ABits = 8;

        std::vector<uint8_t> pixelBlob;

        int mips = 1;
        if (opts.GenerateMipmaps) {
            int mw = physW, mh = physH;
            while (mw > 1 || mh > 1) {
                mips++;
                mw = (mw > 1) ? mw >> 1 : 1;
                mh = (mh > 1) ? mh >> 1 : 1;
            }
        }
        if (opts.ForceMipLevels > 0) mips = opts.ForceMipLevels;

        header.MipmapLevels = mips;

        int curW = physW;
        int curH = physH;
        std::vector<uint8_t> currentMip = sourceData;

        uint32_t mip0Size = 0; // [FIX] Track Level 0 size

        for (int m = 0; m < mips; m++) {
            if (opts.Format == ETextureFormat::ARGB8888) {
                pixelBlob.insert(pixelBlob.end(), currentMip.begin(), currentMip.end());
                if (m == 0) mip0Size = (uint32_t)currentMip.size();
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
                curW = nextW;
                curH = nextH;
            }
        }

        // [FIX] FrameDataSize MUST be the size of the base mipmap only!
        header.FrameDataSize = mip0Size;
        header.MipSize0 = 0;

        result.HeaderInfo.resize(sizeof(CGraphicHeader) + sizeof(CPixelFormatInit));
        memcpy(result.HeaderInfo.data(), &header, sizeof(CGraphicHeader));
        memcpy(result.HeaderInfo.data() + sizeof(CGraphicHeader), &pixelFmt, sizeof(CPixelFormatInit));

        result.FullData = pixelBlob;
        result.Width = physW;
        result.Height = physH;
        result.Success = true;

        return result;
    }

private:
    static int RoundToPow2(int v) {
        int p = 1;
        while (p < v) p <<= 1;
        return p;
    }

    static std::vector<uint8_t> CompressDXT(const std::vector<uint8_t>& rgba, int w, int h, ETextureFormat fmt) {
        int blocksX = (w + 3) / 4;
        int blocksY = (h + 3) / 4;
        int blockSize = (fmt == ETextureFormat::DXT1) ? 8 : 16;

        std::vector<uint8_t> output;
        output.resize(blocksX * blocksY * blockSize);

        uint8_t block[64];

        for (int y = 0; y < blocksY; y++) {
            for (int x = 0; x < blocksX; x++) {

                // [FIX] Edge-Clamping: Prevents black borders on mipmaps and stops STB from crashing on 0-alpha padding
                for (int by = 0; by < 4; by++) {
                    for (int bx = 0; bx < 4; bx++) {
                        int sx = (std::min)(x * 4 + bx, w - 1);
                        int sy = (std::min)(y * 4 + by, h - 1);
                        int srcIdx = (sy * w + sx) * 4;
                        memcpy(&block[(by * 4 + bx) * 4], &rgba[srcIdx], 4);
                    }
                }

                uint8_t* dest = &output[(y * blocksX + x) * blockSize];

                if (fmt == ETextureFormat::DXT1) {
                    // [FIX] Alpha=0 (Fable DXT1 is opaque). Mode=STB_DXT_HIGHQUAL (2) for max quality.
                    stb_compress_dxt_block(dest, block, 0, STB_DXT_HIGHQUAL);
                }
                else if (fmt == ETextureFormat::DXT3) {
                    DXT::EmitAlphaBlock3(dest, block);
                    stb_compress_dxt_block(dest + 8, block, 0, STB_DXT_HIGHQUAL);
                }
                else if (fmt == ETextureFormat::DXT5) {
                    DXT::EmitAlphaBlock5(dest, block);
                    stb_compress_dxt_block(dest + 8, block, 0, STB_DXT_HIGHQUAL);
                }
            }
        }
        return output;
    }
};