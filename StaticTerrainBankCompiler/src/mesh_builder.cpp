#include "egostb/mesh_builder.hpp"
#include "egostb/normal_lut.hpp"
#include "egostb/theme_manager.hpp"
#include "egostb/water_utils.hpp"
#include "egostb/range_compressor.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>

namespace egostb {

static constexpr int s_kOffsets[2][2][3][2] = {
    {{{0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}}, {{1, 0}, {0, 1}, {1, 1}}},
};

struct LayerTopology {
    std::vector<std::pair<uint8_t, uint8_t>> vertices;
    std::vector<uint16_t> indices;
    bool fullPatch = false;
};

static LayerTopology buildLayerTopology(const std::array<bool, 512>& triangleMask) {
    LayerTopology result;
    std::array<int, 17 * 17> vertexIds;
    vertexIds.fill(-1);
    size_t enabled = 0;

    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            for (int triangle = 0; triangle < 2; ++triangle) {
                if (!triangleMask[(x * 16 + y) * 2 + triangle]) continue;
                ++enabled;
                std::array<uint16_t, 3> ids{};
                for (int corner = 0; corner < 3; ++corner) {
                    const int vx = x + s_kOffsets[(x ^ y) & 1][triangle][corner][0];
                    const int vy = y + s_kOffsets[(x ^ y) & 1][triangle][corner][1];
                    int& id = vertexIds[vy * 17 + vx];
                    if (id < 0) {
                        id = static_cast<int>(result.vertices.size());
                        result.vertices.push_back(
                            {static_cast<uint8_t>(vx), static_cast<uint8_t>(vy)});
                    }
                    ids[corner] = static_cast<uint16_t>(id);
                }

                auto& strip = result.indices;
                if (strip.empty()) {
                    if (triangle == 0) strip.push_back(ids[0]);
                    strip.insert(strip.end(), ids.begin(), ids.end());
                } else if (triangle != (strip.size() & 1) && strip.size() >= 2 &&
                           strip[strip.size() - 2] == ids[0] && strip.back() == ids[1]) {
                    strip.push_back(ids[2]);
                } else if (triangle == (strip.size() & 1)) {
                    strip.insert(strip.end(),
                                 {strip.back(), ids[0], ids[0], ids[0], ids[1], ids[2]});
                } else {
                    strip.insert(strip.end(),
                                 {strip.back(), ids[0], ids[0], ids[1], ids[2]});
                }
            }
        }
    }
    result.fullPatch = (enabled == triangleMask.size());
    return result;
}

struct TerrainLayerContribution {
    MaterialTuple material;
    uint8_t mappingDirection = 0;
    uint8_t blend = 0;
};

struct ThemeBlend {
    std::array<uint8_t, 3> indices{0, 0, 0};
    std::array<uint8_t, 3> strengths{0, 0, 0};
};

static std::vector<TerrainLayerContribution> buildThemeContributions(
    const ThemeBlend& blend, const std::vector<ThemeMaterial>& themes) {
    std::array<unsigned, 3> weights{};
    std::array<const ThemeMaterial*, 3> selected{};
    unsigned total = 0;
    for (size_t slot = 0; slot < 3; ++slot) {
        const size_t themeIndex = blend.indices[slot];
        if (themeIndex < themes.size() && themes[themeIndex].available) {
            selected[slot] = &themes[themeIndex];
            weights[slot] = blend.strengths[slot];
            total += weights[slot];
        }
    }
    if (total == 0) return {};
    for (unsigned& weight : weights) weight = weight * 255u / total;

    std::array<unsigned, 3> baseWeights = weights;
    std::array<unsigned, 3> cliffWeights = weights;
    for (size_t first = 0; first < 3; ++first) {
        if (selected[first] == nullptr) continue;
        for (size_t later = first + 1; later < 3; ++later) {
            if (selected[later] == nullptr) continue;
            if (baseWeights[first] && baseWeights[later] &&
                selected[first]->base.textures[0] != 0 &&
                selected[first]->base == selected[later]->base) {
                baseWeights[first] += baseWeights[later];
                baseWeights[later] = 0;
            }
            if (cliffWeights[first] && cliffWeights[later] &&
                selected[first]->cliff.textures[0] != 0 &&
                selected[first]->cliff == selected[later]->cliff) {
                cliffWeights[first] += cliffWeights[later];
                cliffWeights[later] = 0;
            }
        }
    }

    std::vector<TerrainLayerContribution> result;
    for (size_t slot = 0; slot < 3; ++slot) {
        if (selected[slot] == nullptr) continue;
        if (selected[slot]->base.textures[0] != 0 && baseWeights[slot] > 0x10) {
            result.push_back({selected[slot]->base, 0,
                              static_cast<uint8_t>(baseWeights[slot])});
        }
        if (selected[slot]->cliff.textures[0] != 0 && cliffWeights[slot] > 0x10) {
            for (uint8_t direction = 1; direction <= 4; ++direction) {
                result.push_back({selected[slot]->cliff, direction,
                                  static_cast<uint8_t>(cliffWeights[slot])});
            }
        }
    }
    return result;
}

MeshBuilder::MeshBuilder(const LevData& lev, int worldX, int worldY,
                         const std::vector<ThemeMaterial>& themeMaterials,
                         const std::vector<WorldWaterNeighbor>* neighbors,
                         const PrecomputedWaterGrid* waterGrid)
    : m_lev(lev), m_worldX(worldX), m_worldY(worldY), m_themeMaterials(themeMaterials),
      m_neighbors(neighbors), m_waterGrid(waterGrid ? waterGrid : &getMapWaterGrid(lev)) {}

MeshBuilder::Vector3 MeshBuilder::mapNormal(int gx, int gy) const {
    auto sampleHeight = [this](int x, int y) -> float {
        int cx = std::clamp(x, 0, m_lev.width - 1);
        int cy = std::clamp(y, 0, m_lev.height - 1);
        return m_lev.getCell(cx, cy).height;
    };
    auto normalise2 = [](float a, float b) {
        float inv = fastInvSqrtEngine(a * a + b * b);
        return std::pair<float, float>{a * inv, b * inv};
    };
    const auto horizontal = normalise2(sampleHeight(gx - 1, gy) - sampleHeight(gx + 1, gy), 2.0f);
    const auto vertical   = normalise2(sampleHeight(gx, gy - 1) - sampleHeight(gx, gy + 1), 2.0f);
    float nx = horizontal.first;
    float ny = vertical.first;
    float nz = horizontal.second * vertical.second;
    float invLen = fastInvSqrtEngine(nx * nx + ny * ny + nz * nz);
    return {nx * invLen, ny * invLen, nz * invLen};
}

uint32_t MeshBuilder::packNormal(const Vector3& norm) {
    const int ix = static_cast<int>(std::nearbyint(norm.x * 1023.0f));
    const int iy = static_cast<int>(std::nearbyint(norm.y * 1023.0f));
    const int iz = static_cast<int>(std::nearbyint(norm.z * 511.0f));
    return (static_cast<uint32_t>(ix) & 0x7FF) |
           (((static_cast<uint32_t>(iz) << 11) | (static_cast<uint32_t>(iy) & 0x7FF)) << 11);
}

uint8_t MeshBuilder::packDirMaskByte(float component) {
    return static_cast<uint8_t>(std::clamp(static_cast<int>((component * 0.5f + 0.5f) * 255.0f), 0, 255));
}

MeshBuilder::Vector3 MeshBuilder::buildMapDirMask(int gx, int gy) const {
    auto sampleHeight = [this](int x, int y) -> float {
        int cx = std::clamp(x, 0, m_lev.width - 1);
        int cy = std::clamp(y, 0, m_lev.height - 1);
        return m_lev.getCell(cx, cy).height;
    };
    const float center = sampleHeight(gx, gy);
    const float left   = sampleHeight(gx - 1, gy) - center;
    const float right  = sampleHeight(gx + 1, gy) - center;
    const float up     = sampleHeight(gx, gy - 1) - center;
    const float down   = sampleHeight(gx, gy + 1) - center;
    Vector3 faces[8]{};
    size_t count = 0;
    if (((m_worldX + gx) ^ (m_worldY + gy)) & 1) {
        faces[0] = {-right, up, 1.0f};
        faces[1] = {-right, -down, 1.0f};
        faces[2] = {left, -down, 1.0f};
        faces[3] = {left, up, 1.0f};
        count = 4;
    } else {
        const float ul = sampleHeight(gx - 1, gy - 1) - center;
        const float ur = sampleHeight(gx + 1, gy - 1) - center;
        const float dl = sampleHeight(gx - 1, gy + 1) - center;
        const float dr = sampleHeight(gx + 1, gy + 1) - center;
        faces[0] = {up - ur, up, 1.0f};
        faces[1] = {-right, ur - right, 1.0f};
        faces[2] = {-right, right - dr, 1.0f};
        faces[3] = {down - dr, -down, 1.0f};
        faces[4] = {dl - down, -down, 1.0f};
        faces[5] = {left, left - dl, 1.0f};
        faces[6] = {left, ul - left, 1.0f};
        faces[7] = {ul - up, up, 1.0f};
        count = 8;
    }
    Vector3 sum{0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < count; ++i) {
        Vector3& face = faces[i];
        const float invLength = fastInvSqrtEngine(
            face.x * face.x + face.y * face.y + face.z * face.z);
        face.x *= invLength; face.y *= invLength; face.z *= invLength;
        auto penalty = [](float component) {
            return 0.5f / (std::abs(component) + 0.0625f) + 0.5294118f;
        };
        const float weight = penalty(face.x) * penalty(face.y) * penalty(face.z);
        sum.x += face.x * weight;
        sum.y += face.y * weight;
        sum.z += face.z * weight;
    }
    const float invLength = 1.0f / std::sqrt(
        sum.x * sum.x + sum.y * sum.y + sum.z * sum.z);
    sum.x *= invLength; sum.y *= invLength; sum.z *= invLength;
    return sum;
}

bool MeshBuilder::directionActive(uint8_t direction, int gx, int gy) const {
    const auto normal = buildMapDirMask(gx, gy);
    const float topness = std::clamp(
        (std::asin(std::clamp(normal.z, -1.0f, 1.0f)) /
             (3.14159265358979323846f * 0.5f) - 0.5f) * 4.0f,
        0.0f, 1.0f);
    if (direction == 0) return topness > 0.0f;
    if (direction > 4 || topness == 1.0f) return false;
    const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    if (length == 0.0f) return false;
    static constexpr float dirs[5][2] = {
        {0.0f, 0.0f},
        {0.0f, -1.0f}, // 1 = Front
        {0.0f, 1.0f},  // 2 = Back
        {-1.0f, 0.0f}, // 3 = Left
        {1.0f, 0.0f}   // 4 = Right
    };
    const float dot = std::clamp(
        (normal.x / length) * dirs[direction][0] +
        (normal.y / length) * dirs[direction][1],
        -1.0f, 1.0f);
    const float sideness = std::clamp(
        1.0f - 2.0f * (std::acos(dot) / (3.14159265358979323846f * 0.5f) - 0.25f),
        0.0f, 1.0f);
    return (1.0f - topness) * sideness > 0.0f;
}

float MeshBuilder::peekWaterDepth(int gx, int gy) const {
    const auto& grid = m_waterGrid ? *m_waterGrid : getMapWaterGrid(m_lev);
    if (gx >= 0 && gx < grid.width && gy >= 0 && gy < grid.height) {
        return grid.cells[gy * grid.width + gx].waterDepth;
    }
    if (m_neighbors) {
        int wx = m_worldX + gx;
        int wy = m_worldY + gy;
        for (const auto& n : *m_neighbors) {
            if (n.containsWorld(wx, wy)) {
                return n.getWaterDepth(wx, wy);
            }
        }
    }
    return 0.0f;
}

float MeshBuilder::peekInterpolatedWaterHeight(int gx, int gy, int distanceToLook) const {
    const auto& grid = m_waterGrid ? *m_waterGrid : getMapWaterGrid(m_lev);
    int minX = gx - distanceToLook;
    int maxX = gx + distanceToLook;
    int minY = gy - distanceToLook;
    int maxY = gy + distanceToLook;

    bool hasWater = (peekWaterDepth(gx, gy) > 0.001f);
    if (!hasWater) {
        bool found = false;
        int searchMinX = std::max(0, gx - distanceToLook);
        int searchMaxX = std::min(m_lev.gridWidth - 1, gx + distanceToLook);
        int searchMinY = std::max(0, gy - distanceToLook);
        int searchMaxY = std::min(m_lev.gridHeight - 1, gy + distanceToLook);

        for (int j = searchMinX; j <= searchMaxX && !found; ++j) {
            for (int v5 = searchMinY; v5 <= searchMaxY && !found; ++v5) {
                if (grid.get(j, v5).hasWater) {
                    minX = j - distanceToLook;
                    maxX = j + distanceToLook;
                    minY = v5 - distanceToLook;
                    maxY = v5 + distanceToLook;
                    found = true;
                    hasWater = true;
                }
            }
        }
        if (!hasWater) return 0.0f;
    }

    float totalWaterDepth = 0.0f;
    float pointsWithWater = 0.0f;
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            if (x >= 0 && x < grid.width && y >= 0 && y < grid.height) {
                const auto& wc = grid.cells[y * grid.width + x];
                if (wc.hasWater) {
                    totalWaterDepth += wc.waterHeight;
                    pointsWithWater += 1.0f;
                }
            } else if (m_neighbors) {
                int wx = m_worldX + x;
                int wy = m_worldY + y;
                for (const auto& n : *m_neighbors) {
                    if (n.containsWorld(wx, wy)) {
                        const auto* nwc = n.getWaterCell(wx, wy);
                        if (nwc && nwc->hasWater) {
                            totalWaterDepth += nwc->waterHeight;
                            pointsWithWater += 1.0f;
                            break;
                        }
                    }
                }
            } else {
                const auto& wc = grid.get(x, y);
                if (wc.hasWater) {
                    totalWaterDepth += wc.waterHeight;
                    pointsWithWater += 1.0f;
                }
            }
        }
    }
    if (pointsWithWater > 0.0f) {
        return totalWaterDepth / pointsWithWater;
    }
    return 0.0f;
}


bool MeshBuilder::findCorrectWaterLevel(const float heights[17][17], int x, int y, float* outHeight) const {
    int minX = std::max(0, x - 2);
    int maxX = std::min(16, x + 2);
    int minY = std::max(0, y - 2);
    int maxY = std::min(16, y + 2);

    float sum = 0.0f;
    float count = 0.0f;
    for (int nx = minX; nx <= maxX; ++nx) {
        for (int ny = minY; ny <= maxY; ++ny) {
            if (heights[nx][ny] > 0.001f) {
                sum += heights[nx][ny];
                count += 1.0f;
            }
        }
    }
    if (count > 0.0f) {
        if (outHeight) *outHeight = sum / count;
        return true;
    }
    return false;
}

int MeshBuilder::getPatchWaterType(int startX, int startY) const {
    if (!m_waterGrid) return 0;
    int water_type_counts[9] = {0};
    for (int y = startY; y <= startY + 16 && y < m_lev.gridHeight; ++y) {
        for (int x = startX; x <= startX + 16 && x < m_lev.gridWidth; ++x) {
            const auto& wc = m_waterGrid->get(x, y);
            if (wc.hasWater && wc.dominantType >= 1 && wc.dominantType <= 8) {
                water_type_counts[wc.dominantType]++;
            }
        }
    }
    int bestCount = 0;
    int dominantWaterType = 0;
    for (int wt = 1; wt <= 8; ++wt) {
        if (wt == 7) continue;
        if (water_type_counts[wt] > bestCount) {
            bestCount = water_type_counts[wt];
            dominantWaterType = wt;
        }
    }
    return dominantWaterType;
}

void MeshBuilder::ensureWaterBodies() const {
    if (m_shoreBodiesComputed) return;
    m_shoreBodiesComputed = true;

    // 1. FindShorePointsInMap (0x12cd3e0)
    std::vector<ShorePoint> unsorted;
    int w = m_lev.gridWidth;
    int h = m_lev.gridHeight;
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            float d = peekWaterDepth(cx, cy);
            if (d <= 0.001f) {
                int minX = std::max(0, cx - 1);
                int maxX = std::min(w - 1, cx + 1);
                int minY = std::max(0, cy - 1);
                int maxY = std::min(h - 1, cy + 1);
                bool hasWaterNear = false;
                for (int ny = minY; ny <= maxY && !hasWaterNear; ++ny) {
                    for (int nx = minX; nx <= maxX && !hasWaterNear; ++nx) {
                        if (peekWaterDepth(nx, ny) >= 0.001f) {
                            hasWaterNear = true;
                        }
                    }
                }
                if (hasWaterNear) {
                    unsorted.push_back({m_worldX + cx, m_worldY + cy});
                }
            } else {
                if (cx == 0 || cx == w - 1 || cy == 0 || cy == h - 1) {
                    bool neighborHasWater = false;
                    if (m_neighbors) {
                        int checkX = m_worldX + cx;
                        int checkY = m_worldY + cy;
                        if (cx == 0) checkX -= 1;
                        else if (cx == w - 1) checkX += 1;
                        if (cy == 0) checkY -= 1;
                        else if (cy == h - 1) checkY += 1;

                        for (const auto& n : *m_neighbors) {
                            if (n.containsWorld(checkX, checkY) && n.getWaterDepth(checkX, checkY) > 0.001f) {
                                neighborHasWater = true;
                                break;
                            }
                        }
                    }
                    if (!neighborHasWater) {
                        unsorted.push_back({m_worldX + cx, m_worldY + cy});
                    }
                }
            }
        }
    }

    // Include real shoreline points from touching neighbors within a margin
    if (m_neighbors) {
        const int margin = 32;
        int minWx = m_worldX - margin;
        int maxWx = m_worldX + w + margin;
        int minWy = m_worldY - margin;
        int maxWy = m_worldY + h + margin;

        for (const auto& n : *m_neighbors) {
            if (!n.shorePoints) continue;
            for (const auto& pt : *n.shorePoints) {
                if (pt.first >= minWx && pt.first <= maxWx &&
                    pt.second >= minWy && pt.second <= maxWy) {
                    // Only add real land shore points, never boundary water points
                    if (n.getWaterDepth(pt.first, pt.second) <= 0.001f) {
                        unsorted.push_back({pt.first, pt.second});
                    }
                }
            }
        }
    }

    // Deduplicate unsorted shore points
    std::sort(unsorted.begin(), unsorted.end(), [](const ShorePoint& a, const ShorePoint& b) {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    });
    unsorted.erase(std::unique(unsorted.begin(), unsorted.end(), [](const ShorePoint& a, const ShorePoint& b) {
        return a.x == b.x && a.y == b.y;
    }), unsorted.end());

    // 2. SortShorePointsIntoWaterBodies (0x12ce190)
    const float maxDist = 2.0f;
    const float maxDistWithGap = 25.0f;
    std::vector<uint8_t> used(unsorted.size(), 0);
    size_t remaining = unsorted.size();
    size_t firstUnused = 0;

    while (remaining > 0) {
        while (firstUnused < unsorted.size() && used[firstUnused]) {
            ++firstUnused;
        }
        if (firstUnused >= unsorted.size()) break;

        WaterBody body;
        body.points.push_back(unsorted[firstUnused]);
        used[firstUnused] = 1;
        --remaining;

        bool done = false;
        while (!done && remaining > 0) {
            const ShorePoint& cur = body.points.back();
            int bestIdx = -1;
            float bestDistSq = maxDist * maxDist;
            for (size_t i = 0; i < unsorted.size(); ++i) {
                if (used[i]) continue;
                float dx = static_cast<float>(unsorted[i].x - cur.x);
                float dy = static_cast<float>(unsorted[i].y - cur.y);
                float d2 = dx * dx + dy * dy;
                if (d2 <= bestDistSq) {
                    bestDistSq = d2;
                    bestIdx = static_cast<int>(i);
                }
            }
            if (bestIdx >= 0) {
                body.points.push_back(unsorted[bestIdx]);
                used[bestIdx] = 1;
                --remaining;
            } else {
                float dx = static_cast<float>(body.points.front().x - cur.x);
                float dy = static_cast<float>(body.points.front().y - cur.y);
                float d = std::sqrt(dx * dx + dy * dy);
                if (d <= maxDistWithGap) {
                    if (d > maxDist) {
                        body.points.push_back({
                            static_cast<int>(std::nearbyint((body.points.front().x + cur.x) * 0.5f)),
                            static_cast<int>(std::nearbyint((body.points.front().y + cur.y) * 0.5f))
                        });
                    }
                    done = true;
                } else {
                    int gapIdx = -1;
                    float gapDistSq = maxDistWithGap * maxDistWithGap;
                    for (size_t i = 0; i < unsorted.size(); ++i) {
                        if (used[i]) continue;
                        float gdx = static_cast<float>(unsorted[i].x - cur.x);
                        float gdy = static_cast<float>(unsorted[i].y - cur.y);
                        float gd2 = gdx * gdx + gdy * gdy;
                        if (gd2 <= gapDistSq) {
                            gapDistSq = gd2;
                            gapIdx = static_cast<int>(i);
                        }
                    }
                    if (gapIdx >= 0) {
                        body.points.push_back({
                            static_cast<int>(std::nearbyint((unsorted[gapIdx].x + cur.x) * 0.5f)),
                            static_cast<int>(std::nearbyint((unsorted[gapIdx].y + cur.y) * 0.5f))
                        });
                        body.points.push_back(unsorted[gapIdx]);
                        used[gapIdx] = 1;
                        --remaining;
                    } else {
                        done = true;
                    }
                }
            }
        }
        if (body.points.size() >= 3) {
            body.minX = body.points[0].x; body.maxX = body.points[0].x;
            body.minY = body.points[0].y; body.maxY = body.points[0].y;
            for (const auto& pt : body.points) {
                body.minX = std::min(body.minX, pt.x);
                body.maxX = std::max(body.maxX, pt.x);
                body.minY = std::min(body.minY, pt.y);
                body.maxY = std::max(body.maxY, pt.y);
            }
            float spanDx = static_cast<float>(body.maxX - body.minX);
            float spanDy = static_cast<float>(body.maxY - body.minY);
            body.span = std::sqrt(spanDx * spanDx + spanDy * spanDy);
            m_waterBodies.push_back(std::move(body));
        }
    }
}

bool MeshBuilder::constructVertexDistanceToShoreArray(
    float vx, float vy, float array[12], int size,
    float* waterBodySpan, float* shoreUCoord) const
{
    ensureWaterBodies();

    const WaterBody* bodyToUse = nullptr;
    for (const auto& body : m_waterBodies) {
        if (vx >= static_cast<float>(body.minX) && vx <= static_cast<float>(body.maxX) &&
            vy >= static_cast<float>(body.minY) && vy <= static_cast<float>(body.maxY)) {
            bodyToUse = &body;
            break;
        }
    }

    if (!bodyToUse && !m_waterBodies.empty()) {
        float bestDistSq = 1e30f;
        for (const auto& body : m_waterBodies) {
            float cx = std::clamp(vx, static_cast<float>(body.minX), static_cast<float>(body.maxX));
            float cy = std::clamp(vy, static_cast<float>(body.minY), static_cast<float>(body.maxY));
            float d2 = (vx - cx) * (vx - cx) + (vy - cy) * (vy - cy);
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                bodyToUse = &body;
            }
        }
    }

    if (!bodyToUse) {
        static constexpr float kZeroedShoreLookUpArray[12] = {
            1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f
        };
        for (int i = 0; i < size && i < 12; ++i) array[i] = kZeroedShoreLookUpArray[i];
        if (waterBodySpan && *waterBodySpan == 0.0f) *waterBodySpan = 0.0f;
        if (shoreUCoord) *shoreUCoord = 0.0f;
        return false;
    }

    float spanDx = static_cast<float>(bodyToUse->maxX - bodyToUse->minX);
    float spanDy = static_cast<float>(bodyToUse->maxY - bodyToUse->minY);
    if (waterBodySpan) *waterBodySpan = std::sqrt(spanDx * spanDx + spanDy * spanDy);
    if (shoreUCoord) *shoreUCoord = 0.0f;

    float buffer[512];
    for (int i = 0; i < 512; ++i) buffer[i] = -1.0f;

    int numPts = static_cast<int>(bodyToUse->points.size());
    if (numPts >= 2) {
        ShorePoint lastPt = bodyToUse->points.back();
        float dx0 = static_cast<float>(lastPt.x) - vx;
        float dy0 = static_cast<float>(lastPt.y) - vy;
        float dist0 = std::sqrt(dx0 * dx0 + dy0 * dy0);
        float cosTheta0 = (dist0 <= 0.1f) ? 0.0f : (dy0 / dist0);
        cosTheta0 = std::clamp(cosTheta0, -1.0f, 1.0f);
        float angle0 = std::acos(cosTheta0) / 6.283185482025146f;
        if (dx0 < 0.0f) angle0 = 1.0f - angle0;
        int lastEntry = static_cast<int>(std::nearbyint(angle0 * 511.0f));

        for (int i = 0; i < numPts; ++i) {
            const ShorePoint& curPt = bodyToUse->points[i];
            float dx = static_cast<float>(curPt.x) - vx;
            float dy = static_cast<float>(curPt.y) - vy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float cosTheta = (dist <= 0.1f) ? 0.0f : (dy / dist);
            cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
            float angle = std::acos(cosTheta) / 6.283185482025146f;
            if (dx < 0.0f) angle = 1.0f - angle;
            int curEntry = static_cast<int>(std::nearbyint(angle * 511.0f));

            if (curEntry == lastEntry) {
                int idx = curEntry % 512;
                if (idx < 0) idx += 512;
                if (buffer[idx] < 0.0f || dist < buffer[idx]) {
                    buffer[idx] = dist;
                }
            } else {
                int startE = lastEntry;
                int endE = curEntry;
                if (endE < startE) {
                    if (startE - endE > endE - startE + 512) {
                        endE += 512;
                    }
                } else {
                    if (endE - startE > startE - endE + 512) {
                        startE += 512;
                    }
                }
                int initialEntry = std::min(startE, endE);
                int finalEntry = std::max(startE, endE);
                int count = finalEntry - initialEntry;
                for (int k = 0; k <= count; ++k) {
                    float interp = (count <= 0) ? 0.0f : (static_cast<float>(k) / static_cast<float>(count));
                    float sx = static_cast<float>(lastPt.x) + static_cast<float>(curPt.x - lastPt.x) * interp;
                    float sy = static_cast<float>(lastPt.y) + static_cast<float>(curPt.y - lastPt.y) * interp;
                    float d = std::sqrt((sx - vx) * (sx - vx) + (sy - vy) * (sy - vy));
                    int bIdx = (initialEntry + k) % 512;
                    if (bIdx < 0) bIdx += 512;
                    if (buffer[bIdx] < 0.0f || d < buffer[bIdx]) {
                        buffer[bIdx] = d;
                    }
                }
            }
            lastEntry = curEntry % 512;
            lastPt = curPt;
        }
    }

    int firstValid = -1;
    for (int i = 0; i < 512; ++i) {
        if (buffer[i] >= 0.0f) {
            firstValid = i;
            break;
        }
    }
    if (firstValid >= 0) {
        for (int i = 0; i < 512; ++i) {
            if (buffer[i] < 0.0f) {
                int prevValid = (i - 1 + 512) % 512;
                while (buffer[prevValid] < 0.0f) {
                    prevValid = (prevValid - 1 + 512) % 512;
                }
                int nextValid = (i + 1) % 512;
                int count = 1;
                while (buffer[nextValid] < 0.0f) {
                    nextValid = (nextValid + 1) % 512;
                    ++count;
                }
                float vPrev = buffer[prevValid];
                float vNext = buffer[nextValid];
                int gap = (nextValid >= prevValid) ? (nextValid - prevValid) : (nextValid + 512 - prevValid);
                for (int k = 0; k < count; ++k) {
                    int idx = (i + k) % 512;
                    float t = static_cast<float>(k + 1) / static_cast<float>(gap);
                    buffer[idx] = vPrev + t * (vNext - vPrev);
                }
                i += count - 1;
            }
        }
    }

    int bIdx = 0;
    int numEntries = static_cast<int>(std::nearbyint(512.0f / static_cast<float>(size)));
    for (int s = 0; s < size; ++s) {
        float sum = 0.0f;
        int curEntries = (s == size - 1) ? (512 - bIdx) : numEntries;
        int end = bIdx + curEntries;
        for (int i = bIdx; i < end && i < 512; ++i) {
            sum += buffer[i];
        }
        bIdx = end;
        array[s] = (curEntries > 0) ? (sum / static_cast<float>(curEntries)) : 0.0f;
    }

    for (int k = size - 1; k >= 1; --k) {
        array[k] -= array[k - 1];
    }

    return true;
}

CompiledPatch MeshBuilder::buildPatch(int patchX, int patchY) {
    CompiledPatch patch;
    int startX = patchX * 16;
    int startY = patchY * 16;

    float minZ = 1e9f, maxZ = -1e9f;
    float patchWaterHeights[17][17];
    bool patchHasWater = false;

    for (int cy = 0; cy <= 16; ++cy) {
        for (int cx = 0; cx <= 16; ++cx) {
            int gx = startX + cx;
            int gy = startY + cy;
            const auto& cell = m_lev.getCell(gx, gy);
            minZ = std::min(minZ, cell.height);
            maxZ = std::max(maxZ, cell.height);

            float wh = peekInterpolatedWaterHeight(gx, gy, 2);
            patchWaterHeights[cx][cy] = wh;
            if (wh >= 0.001f) {
                patchHasWater = true;
            }
        }
    }
    if (minZ > maxZ) { minZ = 0.0f; maxZ = 0.0f; }

    patch.minCoord[0] = static_cast<float>(m_worldX + startX);
    patch.minCoord[1] = static_cast<float>(m_worldY + startY);
    patch.minCoord[2] = minZ;

    patch.maxCoord[0] = static_cast<float>(m_worldX + startX + 16);
    patch.maxCoord[1] = static_cast<float>(m_worldY + startY + 16);
    patch.maxCoord[2] = maxZ;
    patch.fgNeeded = 1;
    patch.hasWater = patchHasWater;

    // Mirrors CEngineLandscapeMeshBuilder::CLayer
    struct Pass {
        MaterialTuple material;
        uint8_t direction = 0;
        std::array<uint8_t, 17 * 17> blend{};   // VertexBlend, indexed [y * 17 + x]
        std::array<bool, 512> triangles{};      // PolyMaskGrid, indexed [(x * 16 + y) * 2 + tri]
        int polyCount = 0;
    };

    std::vector<Pass> passes;

    if (m_themeMaterials.empty()) {
        // Fallback: 1 default base layer
        Pass pass;
        pass.direction = 0;
        pass.material.textures = {1, 1, 0};
        pass.blend.fill(255);
        pass.triangles.fill(true);
        pass.polyCount = 512;
        passes.push_back(pass);
    } else {
        // CEngineLandscapeMeshBuilder::BuildMapDirMask: per-vertex
        // "GetMappingDirectionBlend(dir) > 0" for all 5 mapping directions.
        std::array<std::array<bool, 5>, 17 * 17> dirMask{};
        for (int y = 0; y <= 16; ++y)
            for (int x = 0; x <= 16; ++x)
                for (uint8_t d = 0; d < 5; ++d)
                    dirMask[size_t(y) * 17 + x][d] = directionActive(d, startX + x, startY + y);

        // CEngineLandscapeMeshBuilder::GetPassFromTexture: passes are created in
        // first-use order.
        auto getPass = [&](const MaterialTuple& material, uint8_t direction) -> Pass& {
            for (auto& p : passes) {
                if (p.direction == direction && p.material == material) return p;
            }
            passes.push_back({});
            passes.back().material = material;
            passes.back().direction = direction;
            return passes.back();
        };

        // CEngineLandscapeMeshBuilder::AddPolysSurroundingPointWithMask: every
        // triangle touching (px, py) is added if any of its corners passes the
        // direction mask. This applies to the TOP direction as well, so purely
        // vertical faces get no top-mapped polygons at all.
        auto addPolys = [&](Pass& pass, int px, int py) {
            for (int qy = py - 1; qy <= py; ++qy) {
                if (qy < 0 || qy > 15) continue;
                for (int qx = px - 1; qx <= px; ++qx) {
                    if (qx < 0 || qx > 15) continue;
                    for (int tri = 0; tri < 2; ++tri) {
                        bool mask = false;
                        bool sharesPoint = false;
                        for (int corner = 0; corner < 3; ++corner) {
                            const int vx = qx + s_kOffsets[(qx ^ qy) & 1][tri][corner][0];
                            const int vy = qy + s_kOffsets[(qx ^ qy) & 1][tri][corner][1];
                            mask |= dirMask[size_t(vy) * 17 + vx][pass.direction];
                            sharesPoint |= (vx == px && vy == py);
                        }
                        bool& slot = pass.triangles[size_t((qx * 16 + qy) * 2 + tri)];
                        if (mask && sharesPoint && !slot) {
                            slot = true;
                            ++pass.polyCount;
                        }
                    }
                }
            }
        };

        // CEngineLandscapeMeshBuilder::BuildLayersFromThemes -> ReadThemesAndCreateLayers
        for (int y = 0; y <= 16; ++y) {
            for (int x = 0; x <= 16; ++x) {
                const auto& cell = m_lev.getCell(startX + x, startY + y);
                ThemeBlend blend;
                blend.indices[0] = cell.themes[0];
                blend.indices[1] = cell.themes[1];
                blend.indices[2] = cell.themes[2];
                blend.strengths[0] = cell.blends[0];
                blend.strengths[1] = cell.blends[1];
                unsigned stored = unsigned(cell.blends[0]) + unsigned(cell.blends[1]);
                blend.strengths[2] = stored >= 255 ? 0 : (255 - stored);

                for (const auto& contribution : buildThemeContributions(blend, m_themeMaterials)) {
                    Pass& pass = getPass(contribution.material, contribution.mappingDirection);
                    pass.blend[size_t(y) * 17 + x] = contribution.blend;
                    addPolys(pass, x, y);
                }
            }
        }
    }

    // CEngineLandscapeMeshBuilder::BuildPatchMesh: the pass with the highest
    // poly count becomes the first (base) layer; the rest follow in creation order.
    size_t basePass = 0;
    int maxCoverage = 0;
    for (size_t i = 0; i < passes.size(); ++i) {
        if (passes[i].polyCount > maxCoverage) {
            maxCoverage = passes[i].polyCount;
            basePass = i;
        }
    }
    std::vector<const Pass*> orderedPasses;
    if (!passes.empty()) orderedPasses.push_back(&passes[basePass]);
    for (size_t i = 0; i < passes.size(); ++i) {
        if (i != basePass && passes[i].polyCount > 0) orderedPasses.push_back(&passes[i]);
    }

    // Convert passes to CompiledLayers
    for (const Pass* passPtr : orderedPasses) {
        const Pass& pass = *passPtr;
        auto topology = buildLayerTopology(pass.triangles);
        if (topology.vertices.empty()) continue;

        CompiledLayer layer;
        layer.mappingDirection = pass.direction;
        layer.fgTexture = pass.material.textures[0];
        layer.bgTexture = pass.material.textures[1];
        layer.bumpTexture = pass.material.textures[2];
        layer.minTextureMipMap = pass.material.textureMaxSize;
        layer.minBumpMapMipMap = pass.material.bumpMaxSize;
        layer.selfIllumination = pass.material.selfIllumination;
        layer.usesSharedIndexBuffer = topology.fullPatch ? 1 : 0;
        layer.polyCountIncDegenerates = static_cast<uint16_t>(topology.indices.size() - 2);
        if (!layer.usesSharedIndexBuffer) {
            layer.indexBuffer = topology.indices;
        }

        layer.vertexCount = static_cast<uint16_t>(topology.vertices.size());
        layer.vertices.reserve(layer.vertexCount);

        for (const auto& coord : topology.vertices) {
            int gx = startX + coord.first;
            int gy = startY + coord.second;

            CompiledVertex v;
            v.x = static_cast<uint16_t>(m_worldX + gx);
            v.y = static_cast<uint16_t>(m_worldY + gy);
            v.z = m_lev.getCell(gx, gy).height;

            Vector3 norm = mapNormal(gx, gy);
            v.normal = packNormal(norm);
            v.blend = pass.blend[size_t(coord.second) * 17 + coord.first];

            Vector3 dir = buildMapDirMask(gx, gy);
            v.cliffU = packDirMaskByte(dir.x);
            v.cliffV = packDirMaskByte(dir.y);

            layer.vertices.push_back(v);
        }

        patch.layers.push_back(std::move(layer));
    }

    // If water present, generate water mesh
    if (patchHasWater) {
        patch.waterPatch.offsetX = startX;
        patch.waterPatch.offsetY = startY;
        patch.waterPatch.waterType = getPatchWaterType(startX, startY);
        float span = std::sqrt(static_cast<float>(m_lev.width * m_lev.width + m_lev.height * m_lev.height));
        patch.waterPatch.waterBodySpan = span;
        patch.waterPatch.vertices.resize(289);

        for (int cx = 0; cx <= 16; ++cx) {
            for (int cy = 0; cy <= 16; ++cy) {
                int vIdx = cx * 17 + cy;
                int gx = startX + cx;
                int gy = startY + cy;
                float worldX = static_cast<float>(m_worldX + gx);
                float worldY = static_cast<float>(m_worldY + gy);

                float wh = patchWaterHeights[cx][cy];
                if (wh <= 0.0f) {
                    float correctH = 0.0f;
                    if (findCorrectWaterLevel(patchWaterHeights, cx, cy, &correctH)) {
                        wh = correctH;
                    } else {
                        wh = m_lev.getCell(gx, gy).height - 1.0f;
                    }
                }

                float v30 = 256.0f * std::max(0.0f, wh - 0.1f);
                float posZ = 0.00390625f * static_cast<float>(static_cast<int>(std::nearbyint(v30)));

                float ly = worldY / 6.283185482025146f;
                double za = std::sin(6.283185482025146 * static_cast<double>(ly));
                float lx = worldX / 6.283185482025146f;
                float sa = static_cast<float>(std::sin(6.283185482025146 * static_cast<double>(lx)) + za);

                float lcy = worldY / 6.283185482025146f;
                double zb = std::cos(6.283185482025146 * static_cast<double>(lcy));
                float lcx = worldX / 6.283185482025146f;
                float ca = static_cast<float>(std::cos(6.283185482025146 * static_cast<double>(lcx)) + zb);

                float cell_h = m_lev.getCell(gx, gy).height;
                float d = posZ + 0.1f - cell_h;
                if (d < 0.0f) d = 0.0f;
                if (d > 2.0f) d = 2.0f;

                auto& wv = patch.waterPatch.vertices[vIdx];
                wv.x = static_cast<uint16_t>(worldX);
                wv.y = static_cast<uint16_t>(worldY);
                wv.z = posZ;
                wv.sinWave = static_cast<int16_t>(std::nearbyint(sa * 32767.0 * 0.5));
                wv.cosWave = static_cast<int16_t>(std::nearbyint(ca * 32767.0 * 0.5));
                wv.depth = static_cast<int16_t>(std::nearbyint(d * 32767.0 * 0.5));
                wv.shoreU = 0.0f;

                constructVertexDistanceToShoreArray(worldX, worldY, wv.shoreDists, 12, &patch.waterPatch.waterBodySpan, &wv.shoreU);
            }
        }
    }

    return patch;
}

std::vector<uint8_t> CompiledPatch::serializeUncompressed() const {
    std::vector<uint8_t> out;

    // Layer count: uint16
    uint16_t layerCount = static_cast<uint16_t>(layers.size());
    out.resize(2);
    std::memcpy(out.data(), &layerCount, 2);

    for (const auto& layer : layers) {
        size_t off = out.size();
        out.resize(off + 30);
        uint8_t* hdr = out.data() + off;

        std::memcpy(hdr + 0, &layer.vertexCount, 2);
        std::memcpy(hdr + 2, &layer.polyCountIncDegenerates, 2);
        hdr[4] = layer.mappingDirection;
        std::memcpy(hdr + 5, &layer.fgTexture, 4);
        std::memcpy(hdr + 9, &layer.bgTexture, 4);
        std::memcpy(hdr + 13, &layer.bumpTexture, 4);
        hdr[17] = layer.usesSharedIndexBuffer;
        std::memcpy(hdr + 18, &layer.minTextureMipMap, 4);
        std::memcpy(hdr + 22, &layer.minBumpMapMipMap, 4);
        std::memcpy(hdr + 26, &layer.selfIllumination, 4);

        // Append vertices: 15 bytes per vertex
        off = out.size();
        out.resize(off + layer.vertexCount * 15);
        uint8_t* vp = out.data() + off;
        for (const auto& v : layer.vertices) {
            std::memcpy(vp + 0, &v.x, 2);
            std::memcpy(vp + 2, &v.y, 2);
            std::memcpy(vp + 4, &v.z, 4);
            std::memcpy(vp + 8, &v.normal, 4);
            vp[12] = v.blend;
            vp[13] = v.cliffU;
            vp[14] = v.cliffV;
            vp += 15;
        }

        // If not using shared index buffer, append index buffer
        if (!layer.usesSharedIndexBuffer && !layer.indexBuffer.empty()) {
            off = out.size();
            size_t ibBytes = layer.indexBuffer.size() * sizeof(uint16_t);
            out.resize(off + ibBytes);
            std::memcpy(out.data() + off, layer.indexBuffer.data(), ibBytes);
        }
    }

    // Water flag: uint8 (0x00 if no water, 0x01 if water)
    if (!hasWater || waterPatch.vertices.empty()) {
        out.push_back(0);
    } else {
        out.push_back(1);
        std::vector<uint8_t> rawVerts(waterPatch.vertices.size() * 66);
        uint8_t* vp = rawVerts.data();
        for (const auto& wv : waterPatch.vertices) {
            std::memcpy(vp + 0, &wv.x, 2);
            std::memcpy(vp + 2, &wv.y, 2);
            std::memcpy(vp + 4, &wv.z, 4);
            std::memcpy(vp + 8, &wv.sinWave, 2);
            std::memcpy(vp + 10, &wv.cosWave, 2);
            std::memcpy(vp + 12, &wv.depth, 2);
            std::memcpy(vp + 14, &wv.shoreU, 4);
            std::memcpy(vp + 18, wv.shoreDists, 48);
            vp += 66;
        }

        auto compWater = RangeCompressor::compress(rawVerts.data(), static_cast<int>(waterPatch.vertices.size()), 66);
        uint32_t compSz = static_cast<uint32_t>(compWater.size());

        size_t off = out.size();
        out.resize(off + 20 + compSz);
        uint8_t* p = out.data() + off;

        std::memcpy(p + 0, &waterPatch.offsetX, 4);
        std::memcpy(p + 4, &waterPatch.offsetY, 4);
        std::memcpy(p + 8, &waterPatch.waterBodySpan, 4);
        std::memcpy(p + 12, &waterPatch.waterType, 4);
        std::memcpy(p + 16, &compSz, 4);
        std::memcpy(p + 20, compWater.data(), compSz);
    }
    return out;
}

} // namespace egostb
