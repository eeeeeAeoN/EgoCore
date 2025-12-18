#pragma once
#include "TextureParser.h"
#include <fstream>
#include <vector>

const uint32_t DDS_MAGIC = 0x20534444;

#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};
#pragma pack(pop)

const uint32_t DDSD_CAPS = 0x00000001;
const uint32_t DDSD_HEIGHT = 0x00000002;
const uint32_t DDSD_WIDTH = 0x00000004;
const uint32_t DDSD_PITCH = 0x00000008;
const uint32_t DDSD_PIXELFORMAT = 0x00001000;
const uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
const uint32_t DDSD_LINEARSIZE = 0x00080000;
const uint32_t DDSD_DEPTH = 0x00800000;

const uint32_t DDPF_ALPHAPIXELS = 0x00000001;
const uint32_t DDPF_FOURCC = 0x00000004;
const uint32_t DDPF_RGB = 0x00000040;

const uint32_t DDSCAPS_COMPLEX = 0x00000008;
const uint32_t DDSCAPS_TEXTURE = 0x00001000;
const uint32_t DDSCAPS_MIPMAP = 0x00400000;

class TextureExporter {
public:
    static bool ExportDDS(const CTextureParser& parser, const std::string& filename, int frameIndex) {
        if (!parser.IsParsed || parser.DecodedPixels.empty()) return false;

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        DDS_HEADER header = {};
        header.dwSize = sizeof(DDS_HEADER);
        header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT | DDSD_LINEARSIZE;

        header.dwHeight = parser.Header.FrameHeight;
        header.dwWidth = parser.Header.FrameWidth;

        // Handle Depth for Volume Textures
        if (parser.Header.Depth > 1) {
            header.dwDepth = parser.Header.Depth;
            header.dwFlags |= DDSD_DEPTH;
        }
        else {
            header.dwDepth = 0;
        }

        header.dwMipMapCount = (parser.Header.MipmapLevels > 0) ? parser.Header.MipmapLevels : 1;
        header.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;

        header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);

        if (parser.DecodedFormat == ETextureFormat::ARGB8888) {
            header.ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
            header.ddspf.dwRGBBitCount = 32;
            header.ddspf.dwRBitMask = 0x00FF0000;
            header.ddspf.dwGBitMask = 0x0000FF00;
            header.ddspf.dwBBitMask = 0x000000FF;
            header.ddspf.dwABitMask = 0xFF000000;
            header.dwPitchOrLinearSize = (parser.Header.FrameWidth * 32 + 7) / 8;
        }
        else {
            header.ddspf.dwFlags = DDPF_FOURCC;
            switch (parser.DecodedFormat) {
            case ETextureFormat::DXT1:
            case ETextureFormat::NormalMap_DXT1:
                header.ddspf.dwFourCC = 0x31545844; // "DXT1"
                break;
            case ETextureFormat::DXT3:
                header.ddspf.dwFourCC = 0x33545844; // "DXT3"
                break;
            case ETextureFormat::DXT5:
            case ETextureFormat::NormalMap_DXT5:
                header.ddspf.dwFourCC = 0x35545844; // "DXT5"
                break;
            default: return false;
            }
            header.dwPitchOrLinearSize = parser.Header.FrameDataSize;
        }

        file.write((const char*)&DDS_MAGIC, sizeof(uint32_t));
        file.write((const char*)&header, sizeof(DDS_HEADER));

        // Export the whole "Frame" (which includes all Mips + all Depth slices)
        size_t frameOffset = (size_t)parser.TrueFrameStride * frameIndex;

        if (frameOffset + parser.TrueFrameStride <= parser.DecodedPixels.size()) {
            file.write((const char*)parser.DecodedPixels.data() + frameOffset, parser.TrueFrameStride);
        }
        else {
            return false;
        }

        file.close();
        return true;
    }
};