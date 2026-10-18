#include "egostb/heightmap.hpp"
#include "egostb/theme_manager.hpp"
#include <cmath>

namespace egostb {

Heightmap::Heightmap(int32_t width, int32_t height)
    : width_(width), height_(height), gridWidth_(width + 1), gridHeight_(height + 1),
      heights_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f),
      hasWater_(static_cast<size_t>(gridWidth_) * gridHeight_, 0),
      waterDepths_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f),
      waterHeights_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f) {}

Heightmap::Heightmap(const LevData& lev)
    : width_(lev.width), height_(lev.height), gridWidth_(lev.gridWidth), gridHeight_(lev.gridHeight),
      heights_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f),
      hasWater_(static_cast<size_t>(gridWidth_) * gridHeight_, 0),
      waterDepths_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f),
      waterHeights_(static_cast<size_t>(gridWidth_) * gridHeight_, 0.0f) {
    for (int y = 0; y < gridHeight_; ++y) {
        for (int x = 0; x < gridWidth_; ++x) {
            size_t idx = static_cast<size_t>(y) * gridWidth_ + x;
            const auto& cell = lev.getCell(x, y);
            heights_[idx] = cell.height;

            float depth = 0.0f;
            for (int t = 0; t < 3; ++t) {
                uint8_t thIdx = cell.themes[t];
                if (thIdx < lev.themePalette.size()) {
                    const auto& name = lev.themePalette[thIdx].name;
                    if (!name.empty()) {
                        const auto* def = ThemeManager::instance().getTheme(name);
                        if (def && def->waterType > 0) {
                            float blend = 0.0f;
                            if (t == 0) blend = static_cast<float>(cell.blends[0]) / 255.0f;
                            else if (t == 1) blend = static_cast<float>(cell.blends[1]) / 255.0f;
                            else {
                                int rem = 255 - static_cast<int>(cell.blends[0]) - static_cast<int>(cell.blends[1]);
                                if (rem > 0) blend = static_cast<float>(rem) / 255.0f;
                            }
                            depth += def->waterHeight * blend;
                        }
                    }
                }
            }
            waterDepths_[idx] = depth;
            if (depth > 0.001f) {
                hasWater_[idx] = 1;
                waterHeights_[idx] = cell.height + depth;
            } else {
                hasWater_[idx] = 0;
                waterHeights_[idx] = 0.0f;
            }
        }
    }
}

float Heightmap::peekHeight(int32_t x, int32_t y) const {
    int32_t cx = std::clamp(x, 0, width_ - 1);
    int32_t cy = std::clamp(y, 0, height_ - 1);
    return heights_[static_cast<size_t>(cy) * gridWidth_ + cx];
}

bool Heightmap::peekHasWater(int32_t x, int32_t y) const {
    if (x < 0 || x >= gridWidth_ || y < 0 || y >= gridHeight_) return false;
    return hasWater_[static_cast<size_t>(y) * gridWidth_ + x] != 0;
}

float Heightmap::peekWaterDepth(int32_t x, int32_t y) const {
    if (x < 0 || x >= gridWidth_ || y < 0 || y >= gridHeight_) return 0.0f;
    return waterDepths_[static_cast<size_t>(y) * gridWidth_ + x];
}

float Heightmap::peekWaterHeight(int32_t x, int32_t y) const {
    if (x < 0 || x >= gridWidth_ || y < 0 || y >= gridHeight_) return 0.0f;
    return waterHeights_[static_cast<size_t>(y) * gridWidth_ + x];
}

void Heightmap::setHeight(int32_t x, int32_t y, float h) {
    if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
        heights_[static_cast<size_t>(y) * gridWidth_ + x] = h;
    }
}

void Heightmap::setWater(int32_t x, int32_t y, bool hasWater, float depth, float waterHeight) {
    if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
        size_t idx = static_cast<size_t>(y) * gridWidth_ + x;
        hasWater_[idx] = hasWater ? 1 : 0;
        waterDepths_[idx] = depth;
        waterHeights_[idx] = waterHeight;
    }
}

// Task 2: Diagonal stencil peekNormal
Vec3 Heightmap::peekNormal(int32_t x, int32_t y) const {
    int32_t cx = std::clamp(x, 0, width_ - 1);
    int32_t cy = std::clamp(y, 0, height_ - 1);

    int32_t cx1 = std::clamp(cx + 1, 0, width_);
    int32_t cy1 = std::clamp(cy + 1, 0, height_);

    float h00 = heights_[static_cast<size_t>(cy) * gridWidth_ + cx];
    float h10 = heights_[static_cast<size_t>(cy) * gridWidth_ + cx1];
    float h01 = heights_[static_cast<size_t>(cy1) * gridWidth_ + cx];
    float h11 = heights_[static_cast<size_t>(cy1) * gridWidth_ + cx1];

    float nx = h00 - h11;
    float ny = h10 - h01;
    float inv = fastInvSqrtEngine(nx * nx + ny * ny + 2.0f);
    constexpr float rot_factor = 0.7071067690849304f;
    constexpr float z_factor = 1.414213538169861f;

    Vec3 out;
    out.x = (ny + nx) * (inv * rot_factor);
    out.y = (ny - nx) * (inv * rot_factor);
    out.z = inv * z_factor;
    return out;
}

// 4-neighbor map normal matching CEngineMap::PeekMapNormal (0x125db90 / 0x125e54b)
Vec3 Heightmap::peekMapNormal(int32_t x, int32_t y) const {
    auto sampleH = [&](int32_t sx, int32_t sy) {
        int32_t cx = std::clamp(sx, 0, width_ - 1);
        int32_t cy = std::clamp(sy, 0, height_ - 1);
        return heights_[static_cast<size_t>(cy) * gridWidth_ + cx];
    };

    float hx = sampleH(x - 1, y) - sampleH(x + 1, y);
    float hy = 2.0f;
    float h_inv = fastInvSqrtEngine(hx * hx + hy * hy);
    hx *= h_inv;
    hy *= h_inv;

    float vx = sampleH(x, y - 1) - sampleH(x, y + 1);
    float vy = 2.0f;
    float v_inv = fastInvSqrtEngine(vx * vx + vy * vy);
    vx *= v_inv;
    vy *= v_inv;

    float nz = vy * hy;
    float nx = hx;
    float ny = vx;
    float n_inv = fastInvSqrtEngine(nx * nx + ny * ny + nz * nz);
    return {nx * n_inv, ny * n_inv, nz * n_inv};
}

// Task 2: Bilinear interpolated normal with fastInvSqrtEngine normalization (0x125e5b0)
Vec3 Heightmap::peekInterpolatedNormal(float x, float y) const {
    int32_t v29 = static_cast<int32_t>(x - 0.5f);
    if (x == static_cast<float>(v29 + 1)) ++v29;
    int32_t ypa = static_cast<int32_t>(y - 0.5f);
    if (y == static_cast<float>(ypa + 1)) ++ypa;

    float frac_x = x - static_cast<float>(v29);
    float frac_y = y - static_cast<float>(ypa);

    Vec3 normal00 = peekMapNormal(v29, ypa);
    Vec3 normal10 = peekMapNormal(v29 + 1, ypa);
    Vec3 normal01 = peekMapNormal(v29, ypa + 1);
    Vec3 normal11 = peekMapNormal(v29 + 1, ypa + 1);

    float w00 = (1.0f - frac_x) * (1.0f - frac_y);
    float w10 = frac_x * (1.0f - frac_y);
    float w01 = (1.0f - frac_x) * frac_y;
    float w11 = frac_x * frac_y;

    float nx = normal00.x * w00 + normal10.x * w10 + normal01.x * w01 + normal11.x * w11;
    float ny = normal00.y * w00 + normal10.y * w10 + normal01.y * w01 + normal11.y * w11;
    float nz = normal00.z * w00 + normal10.z * w10 + normal01.z * w01 + normal11.z * w11;

    float invLen = fastInvSqrtEngine(nx * nx + ny * ny + nz * nz);
    return {nx * invLen, ny * invLen, nz * invLen};
}

// Task 3: peekInterpolatedWaterHeight with window recentering (matching 0x12615d0)
float Heightmap::peekInterpolatedWaterHeight(int32_t mapx, int32_t mapy, int32_t distance_to_look) const {
    int32_t minX = mapx - distance_to_look;
    int32_t maxX = mapx + distance_to_look;
    int32_t minY = mapy - distance_to_look;
    int32_t maxY = mapy + distance_to_look;

    bool hasWater = peekHasWater(mapx, mapy);
    if (!hasWater) {
        bool found = false;
        int32_t searchMinX = std::max(0, mapx - distance_to_look);
        int32_t searchMaxX = std::min(width_, mapx + distance_to_look);
        int32_t searchMinY = std::max(0, mapy - distance_to_look);
        int32_t searchMaxY = std::min(height_, mapy + distance_to_look);

        for (int32_t j = searchMinX; j <= searchMaxX && !found; ++j) {
            for (int32_t v5 = searchMinY; v5 <= searchMaxY && !found; ++v5) {
                if (peekHasWater(j, v5)) {
                    // Re-center averaging window around (j, v5)
                    minX = j - distance_to_look;
                    maxX = j + distance_to_look;
                    minY = v5 - distance_to_look;
                    maxY = v5 + distance_to_look;
                    found = true;
                    hasWater = true;
                }
            }
        }
        if (!hasWater) return 0.0f;
    }

    float totalWaterDepth = 0.0f;
    float pointsWithWater = 0.0f;
    for (int32_t x = minX; x <= maxX; ++x) {
        for (int32_t y = minY; y <= maxY; ++y) {
            if (peekHasWater(x, y)) {
                totalWaterDepth += peekWaterHeight(x, y);
                pointsWithWater += 1.0f;
            }
        }
    }

    if (pointsWithWater > 0.0f) {
        return totalWaterDepth / pointsWithWater;
    }
    return 0.0f;
}

} // namespace egostb
