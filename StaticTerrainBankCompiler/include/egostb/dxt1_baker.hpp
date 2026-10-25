#pragma once

#include "egostb/lev_reader.hpp"
#include "egostb/theme_manager.hpp"
#include "egostb/water_utils.hpp"
#include <cstdint>
#include <vector>

namespace egostb {

struct ColorRGB {
    uint8_t r = 128;
    uint8_t g = 128;
    uint8_t b = 128;
};

struct DecodedTexture;

class Dxt1Baker {
public:
    // Bake a procedural background texture for a patch covering [mapX, mapX+width] x [mapY, mapY+height]
    // at resolution texWidth x texHeight.
    // Returns the 19-byte CTexture header followed by the raw DXT1 block data.
    static std::vector<uint8_t> bakeTexture(
        const LevData& lev,
        const std::vector<ThemeMaterial>& materials,
        int mapX, int mapY, int patchW, int patchH,
        int texWidth, int texHeight,
        int worldX = 3072, int worldY = 2816,
        bool generateMipmaps = false,
        bool highQualityDxt = false,
        const std::vector<WorldWaterNeighbor>* neighbors = nullptr
    );

    // Encode a buffer of RGBA or RGB pixels (width x height) into raw DXT1 block bytes
    static std::vector<uint8_t> encodeDXT1(const ColorRGB* pixels, int width, int height, bool highQuality = false);

    static void clearCache();

private:
    static uint16_t toRGB565(uint8_t r, uint8_t g, uint8_t b);
    static ColorRGB fromRGB565(uint16_t c);
    static ColorRGB sampleDecodedTextureBilinear(const DecodedTexture& dt, float u, float v);
};

} // namespace egostb
