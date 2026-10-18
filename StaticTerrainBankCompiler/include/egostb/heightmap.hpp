#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>
#include "egostb/normal_lut.hpp"
#include "egostb/lev_reader.hpp"

namespace egostb {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 1.0f;
};

class Heightmap {
public:
    Heightmap(int32_t width = 128, int32_t height = 128);
    explicit Heightmap(const LevData& lev);

    int32_t width() const { return width_; }
    int32_t height() const { return height_; }
    int32_t gridWidth() const { return gridWidth_; }
    int32_t gridHeight() const { return gridHeight_; }

    float peekHeight(int32_t x, int32_t y) const;
    bool peekHasWater(int32_t x, int32_t y) const;
    float peekWaterDepth(int32_t x, int32_t y) const;
    float peekWaterHeight(int32_t x, int32_t y) const;

    // Task 2: Diagonal stencil peekNormal
    Vec3 peekNormal(int32_t x, int32_t y) const;

    // Task 2: Bilinear interpolated normal with fastInvSqrtEngine normalization
    Vec3 peekInterpolatedNormal(float x, float y) const;

    // 4-neighbor map normal matching CEngineMap::PeekMapNormal (0x125db90)
    Vec3 peekMapNormal(int32_t x, int32_t y) const;

    // Task 3: peekInterpolatedWaterHeight with window recentering (0x12615d0)
    float peekInterpolatedWaterHeight(int32_t mapx, int32_t mapy, int32_t distance_to_look = 2) const;

    void setHeight(int32_t x, int32_t y, float h);
    void setWater(int32_t x, int32_t y, bool hasWater, float depth, float waterHeight);

private:
    int32_t width_ = 128;
    int32_t height_ = 128;
    int32_t gridWidth_ = 129;
    int32_t gridHeight_ = 129;
    std::vector<float> heights_;
    std::vector<uint8_t> hasWater_;
    std::vector<float> waterDepths_;
    std::vector<float> waterHeights_;
};

} // namespace egostb
