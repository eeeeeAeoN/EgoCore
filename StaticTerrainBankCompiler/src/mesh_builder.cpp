#include "egostb/mesh_builder.hpp"
#include "egostb/normal_lut.hpp"
#include "egostb/theme_manager.hpp"
#include "egostb/bank_resolver.hpp"
#include "egostb/water_utils.hpp"
#include "egostb/range_compressor.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>
#include <cstdint>
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
    const std::array<const ThemeMaterial*, 3>& selected,
    const std::array<uint8_t, 3>& strengths) {
    std::array<unsigned, 3> weights{};
    unsigned total = 0;
    for (size_t slot = 0; slot < 3; ++slot) {
        if (selected[slot] != nullptr && selected[slot]->available) {
            weights[slot] = strengths[slot];
            total += weights[slot];
        }
    }
    if (total == 0) return {};
    for (unsigned& weight : weights) weight = weight * 255u / total;

    std::array<unsigned, 3> baseWeights = weights;
    std::array<unsigned, 3> cliffWeights = weights;
    for (size_t first = 0; first < 3; ++first) {
        if (selected[first] == nullptr || !selected[first]->available) continue;
        for (size_t later = first + 1; later < 3; ++later) {
            if (selected[later] == nullptr || !selected[later]->available) continue;
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
        if (selected[slot] == nullptr || !selected[slot]->available) continue;
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
        return peekCell(x, y).height;
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
        return peekCell(x, y).height;
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

const HeightMapCell& MeshBuilder::peekCell(int gx, int gy, const LevData** outLev) const {
    if (gx >= 0 && gx < m_lev.width && gy >= 0 && gy < m_lev.height) {
        if (outLev) *outLev = &m_lev;
        return m_lev.getCell(gx, gy);
    }
    int wx = m_worldX + gx;
    int wy = m_worldY + gy;
    if (m_neighbors) {
        for (const auto& n : *m_neighbors) {
            if (n.lev && wx >= n.mapX && wx < n.mapX + n.width &&
                wy >= n.mapY && wy < n.mapY + n.height) {
                int lx = wx - n.mapX;
                int ly = wy - n.mapY;
                if (outLev) *outLev = n.lev;
                return n.lev->getCell(lx, ly);
            }
        }
    }
    int cx = std::clamp(gx, 0, m_lev.width - 1);
    int cy = std::clamp(gy, 0, m_lev.height - 1);
    if (outLev) *outLev = &m_lev;
    return m_lev.getCell(cx, cy);
}

float MeshBuilder::peekLandscapeHeight(int gx, int gy) const {
    float h = peekCell(gx, gy).height;
    return static_cast<float>(static_cast<int>(h * 128.0f)) * 0.0078125f;
}

static MeshBuilder::WaterGridCell makeWaterCellFromLev(const LevData& lev, int lx, int ly, const PrecomputedWaterGrid* extGrid = nullptr) {
    MeshBuilder::WaterGridCell wc;
    if (lx < 0 || lx >= lev.width || ly < 0 || ly >= lev.height) return wc;
    const HeightMapCell& cell = lev.getCell(lx, ly);
    float qh = static_cast<float>(static_cast<int>(cell.height * 128.0f)) * 0.0078125f;

    auto getDef = [&](uint8_t thIdx) -> const EngineThemeDef* {
        if (thIdx < lev.themePalette.size() && !lev.themePalette[thIdx].name.empty()) {
            return ThemeManager::instance().getTheme(lev.themePalette[thIdx].name);
        }
        return nullptr;
    };

    const EngineThemeDef* def0 = getDef(cell.themes[0]);
    const EngineThemeDef* def1 = getDef(cell.themes[1]);
    const EngineThemeDef* def2 = getDef(cell.themes[2]);

    // hasWaterFast (0x2d5d620): check slot 0, slot 1 (if slot 0 valid), slot 2 (if slot 0 & 1 valid)
    if (def0 && def0->waterType > 0) {
        wc.hasWaterFast = 1;
    } else if (def1 && def1->waterType > 0) {
        wc.hasWaterFast = 1;
    } else if (def2 && def2->waterType > 0) {
        wc.hasWaterFast = 1;
    }

    // waterType (0x2d5d780): first non-zero, non-7 across slots 0, 1, 2 (or 7 if only 7)
    uint8_t wt = 0;
    if (def0 && def0->waterType > 0) wt = static_cast<uint8_t>(def0->waterType);
    if (def1 && def1->waterType > 0) {
        if (wt == 0 || wt == 7) wt = static_cast<uint8_t>(def1->waterType);
        if (def2 && def2->waterType > 0) {
            if (wt == 0 || wt == 7) wt = static_cast<uint8_t>(def2->waterType);
        }
    }
    wc.waterType = wt;
    wc.hasDummyShorePoint = (wt == 7) ? 1 : 0;

    // waterDepth (0x2d5e000)
    float depth = 0.0f;
    if (def0) {
        float b0 = static_cast<float>(cell.blends[0]) / 255.0f;
        depth += def0->waterHeight * b0;
        if (def1) {
            float b1 = static_cast<float>(cell.blends[1]) / 255.0f;
            depth += def1->waterHeight * b1;
            if (def2) {
                int rem = 255 - static_cast<int>(cell.blends[0]) - static_cast<int>(cell.blends[1]);
                if (rem > 0) {
                    float b2 = static_cast<float>(rem) / 255.0f;
                    depth += def2->waterHeight * b2;
                }
            }
        }
    } else if (extGrid && lx < extGrid->width && ly < extGrid->height) {
        depth = extGrid->cells[ly * extGrid->width + lx].waterDepth;
    }

    wc.waterDepth = depth;
    wc.hasWater = (wc.hasWaterFast && depth > 0.001f) ? 1 : 0;
    wc.waterSurfH = (depth > 0.001f) ? (qh + depth) : 0.0f;
    return wc;
}

const MeshBuilder::WaterGrid& MeshBuilder::ensureWaterGrid() const {
    if (!m_internalWaterGridComputed) {
        m_internalWaterGrid.width = m_lev.width;
        m_internalWaterGrid.height = m_lev.height;
        m_internalWaterGrid.cells.resize(m_lev.width * m_lev.height);
        for (int y = 0; y < m_lev.height; ++y) {
            for (int x = 0; x < m_lev.width; ++x) {
                m_internalWaterGrid.cells[y * m_lev.width + x] = makeWaterCellFromLev(m_lev, x, y, m_waterGrid);
            }
        }
        m_internalWaterGridComputed = true;
    }
    return m_internalWaterGrid;
}

MeshBuilder::WaterGridCell MeshBuilder::peekWaterCell(int gx, int gy) const {
    if (gx >= 0 && gx < m_lev.width && gy >= 0 && gy < m_lev.height) {
        return ensureWaterGrid().get(gx, gy);
    }
    int wx = m_worldX + gx;
    int wy = m_worldY + gy;
    if (m_neighbors) {
        for (const auto& n : *m_neighbors) {
            if (wx >= n.mapX && wx < n.mapX + n.width &&
                wy >= n.mapY && wy < n.mapY + n.height) {
                int lx = wx - n.mapX;
                int ly = wy - n.mapY;
                if (n.lev) {
                    return makeWaterCellFromLev(*n.lev, lx, ly, n.waterGrid);
                } else if (n.waterGrid) {
                    WaterGridCell wc;
                    const auto& c = n.waterGrid->get(lx, ly);
                    wc.hasWater = c.hasWater;
                    wc.hasWaterFast = (c.dominantType > 0) ? 1 : 0;
                    wc.waterType = c.dominantType;
                    wc.hasDummyShorePoint = (c.dominantType == 7) ? 1 : 0;
                    wc.waterDepth = c.waterDepth;
                    float qh = static_cast<float>(static_cast<int>(n.getLandscapeHeight(wx, wy) * 128.0f)) * 0.0078125f;
                    wc.waterSurfH = (c.waterDepth > 0.001f) ? (qh + c.waterDepth) : 0.0f;
                    return wc;
                }
            }
        }
    }
    int cx = std::clamp(gx, 0, m_lev.width - 1);
    int cy = std::clamp(gy, 0, m_lev.height - 1);
    return ensureWaterGrid().get(cx, cy);
}

float MeshBuilder::peekWaterDepth(int gx, int gy) const {
    return peekWaterCell(gx, gy).waterDepth;
}

float MeshBuilder::peekWaterSurfaceHeight(int gx, int gy) const {
    return peekWaterCell(gx, gy).waterSurfH;
}

float MeshBuilder::peekInterpolatedWaterHeight(int gx, int gy, int distanceToLook) const {
    int v18 = gx - distanceToLook;
    int v16 = gy - distanceToLook;
    int v14 = gx + distanceToLook;
    int v15 = gy + distanceToLook;
    if (!peekWaterCell(gx, gy).hasWater) {
        bool found = false;
        for (int i = gx - distanceToLook; i <= v14 && !found; ++i) {
            for (int j = v16; j <= v15 && !found; ++j) {
                if (peekWaterCell(i, j).hasWaterFast) {
                    v18 = i - distanceToLook;
                    v16 = j - distanceToLook;
                    v14 = i + distanceToLook;
                    v15 = j + distanceToLook;
                    found = true;
                }
            }
        }
        if (!found) return 0.0f;
    }
    float sum = 0.0f;
    float count = 0.0f;
    for (int xp = v18; xp <= v14; ++xp) {
        for (int yp = v16; yp <= v15; ++yp) {
            WaterGridCell wc = peekWaterCell(xp, yp);
            if (wc.waterDepth > 0.001f) {
                sum += wc.waterSurfH;
                count += 1.0f;
            }
        }
    }
    return (count <= 0.0f) ? 0.0f : (sum / count);
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
    int waterType = 0;
    for (int xp = startX; xp <= startX + 16; ++xp) {
        for (int yp = startY; yp <= startY + 16; ++yp) {
            WaterGridCell wc = peekWaterCell(xp, yp);
            if (waterType == 0 || waterType == 7) {
                if (wc.waterType != 0 && wc.waterType != 7) {
                    waterType = wc.waterType;
                }
            }
        }
    }
    return waterType;
}

static int cosThetaToBufferIndex(float cosTheta, bool isLeft, int size = 512) {
    cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
    float angle = std::acos(cosTheta) / 6.283185482025146f;
    if (isLeft) {
        angle = 1.0f - angle;
    }
    float v = (static_cast<float>(size) - 1.0f) * angle;
    return std::clamp(static_cast<int>(std::floor(v)), 0, size - 1);
}

static void ensureBufferIsCircular(int& initialEntry, int& finalEntry) {
    if (initialEntry - finalEntry <= finalEntry + 512 - initialEntry) {
        if (finalEntry - initialEntry > initialEntry + 512 - finalEntry) {
            initialEntry += 512;
        }
    } else {
        finalEntry += 512;
    }
}

static void setBufferEntriesToInterpolatedShorePointDistance(
    float* buffer, float vx, float vy,
    int initialEntry, int finalEntry,
    const MeshBuilder::ShorePoint& nextPt,
    const MeshBuilder::ShorePoint& lastPt)
{
    float range = static_cast<float>(finalEntry - initialEntry);
    for (int i = initialEntry; i <= finalEntry; ++i) {
        float interp = (range <= 0.0f) ? 0.0f : (static_cast<float>(i - initialEntry) / range);
        interp = std::clamp(interp, 0.0f, 1.0f);
        float x = static_cast<float>(nextPt.x - lastPt.x) * interp + static_cast<float>(lastPt.x);
        float y = static_cast<float>(nextPt.y - lastPt.y) * interp + static_cast<float>(lastPt.y);
        float dx = x - vx;
        float dy = y - vy;
        float dist = std::sqrt(dx * dx + dy * dy);
        int idx = i % 512;
        if (idx < 0) idx += 512;
        if (buffer[idx] < 0.0f || dist < buffer[idx]) {
            buffer[idx] = dist;
        }
    }
}

static void fillInvalidEntriesWithInterpolatedValues(float buffer[512]) {
    int firstValid = -1;
    for (int i = 0; i < 512; ++i) {
        if (buffer[i] >= 0.0f) {
            firstValid = i;
            break;
        }
    }
    if (firstValid < 0) return;

    for (int i = 0; i < 512; ++i) {
        if (buffer[i] < 0.0f) {
            int j = i - 1;
            int v5 = i + 1;
            int v6 = 1;
            if (i - 1 < 0) {
                for (j = 511; buffer[j] < 0.0f; --j) {
                    ++v6;
                }
            }
            while (buffer[v5 % 512] < 0.0f) {
                ++v5;
                ++v6;
            }
            if (j > v5) {
                v5 += 512;
            }
            for (int k = j; k < v5; ++k) {
                float v3 = static_cast<float>(static_cast<double>(k - j) / static_cast<double>(v6));
                buffer[k % 512] = (buffer[v5 % 512] - buffer[j]) * v3 + buffer[j];
            }
        }
    }
}

static float computeShoreUCoord(float vx, float vy, const MeshBuilder::WaterBody& body, int closestShoreCoordIndex) {
    // In FableWin.exe, CWaterGenerator::GetShoreUCoord (0x2e09ff0) returns 0.0f
    // whenever CEngineWorldMap::PeekStaticMapQuality >= 8 (Static Map mode).
    (void)vx; (void)vy; (void)body; (void)closestShoreCoordIndex;
    return 0.0f;
}

std::vector<MeshBuilder::ShorePoint> MeshBuilder::collectShorePoints() const {
    std::vector<ShorePoint> unsorted;
    int w = m_lev.width;
    int h = m_lev.height;

    auto peekInterpolatedHasWater = [this](int x, int y, int dist) -> bool {
        for (int ix = x - dist; ix <= x + dist; ++ix) {
            for (int iy = y - dist; iy <= y + dist; ++iy) {
                if (peekWaterCell(ix, iy).hasWater) return true;
            }
        }
        return false;
    };

    auto peekInterpolatedWaterDepth = [this](int x, int y, int dist) -> float {
        int v18 = x - dist;
        int v16 = y - dist;
        int v14 = x + dist;
        int v15 = y + dist;
        if (!peekWaterCell(x, y).hasWaterFast) {
            bool found = false;
            for (int i = x - dist; i <= v14 && !found; ++i) {
                for (int j = y - dist; j <= v15 && !found; ++j) {
                    if (peekWaterCell(i, j).hasWaterFast) {
                        v18 = i - dist;
                        v16 = j - dist;
                        v14 = i + dist;
                        v15 = j + dist;
                        found = true;
                    }
                }
            }
            if (!found) return 0.0f;
        }
        float sum = 0.0f;
        float count = 0.0f;
        for (int k = v18; k <= v14; ++k) {
            for (int m = v16; m <= v15; ++m) {
                float d = peekWaterCell(k, m).waterDepth;
                if (d > 0.001f) {
                    sum += d;
                    count += 1.0f;
                }
            }
        }
        return (count <= 0.0f) ? 0.0f : (sum / count);
    };

    auto containsPoint = [&](int px, int py) {
        for (const auto& pt : unsorted) {
            if (pt.x == px && pt.y == py) return true;
        }
        return false;
    };

    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            if (!peekInterpolatedHasWater(cx, cy, 1)) {
                int minY = std::max(0, cy - 1);
                int maxY = std::min(h - 1, cy + 1);
                int minX = std::max(0, cx - 1);
                int maxX = std::min(w - 1, cx + 1);
                bool anyWater = false;
                for (int ny = minY; ny <= maxY && !anyWater; ++ny) {
                    for (int nx = minX; nx <= maxX && !anyWater; ++nx) {
                        if (peekInterpolatedWaterDepth(nx, ny, 1) >= 0.001f) {
                            anyWater = true;
                        }
                    }
                }
                if (anyWater) {
                    int ptX = m_worldX + cx;
                    int ptY = m_worldY + cy;
                    if (!containsPoint(ptX, ptY)) {
                        unsorted.push_back({ptX, ptY, false});
                    }
                }
            } else {
                WaterGridCell wc = peekWaterCell(cx, cy);
                if (wc.waterType == 7 || wc.hasDummyShorePoint) {
                    int ptX = m_worldX + cx;
                    int ptY = m_worldY + cy;
                    if (!containsPoint(ptX, ptY)) {
                        unsorted.push_back({ptX, ptY, false});
                    }
                } else if (cx == 0 || cx == w - 1 || cy == 0 || cy == h - 1) {
                    int ptX = m_worldX + cx;
                    int ptY = m_worldY + cy;
                    if (cx == w - 1) ++ptX;
                    else if (cy == h - 1) ++ptY;
                    if (!containsPoint(ptX, ptY)) {
                        unsorted.push_back({ptX, ptY, true});
                    }
                }
            }
        }
    }
    return unsorted;
}

std::vector<MeshBuilder::WaterBody> MeshBuilder::sortShorePointsIntoWaterBodies(std::vector<ShorePoint> unsorted) {
    const float maxDist = 2.0f;
    const float maxDistWithGap = 25.0f;

    std::vector<uint8_t> used(unsorted.size(), 0);
    size_t remaining = unsorted.size();
    size_t firstUnused = 0;
    std::vector<WaterBody> bodies;

    while (remaining > 0) {
        while (firstUnused < unsorted.size() && used[firstUnused]) {
            ++firstUnused;
        }
        if (firstUnused >= unsorted.size()) break;

        WaterBody body;
        body.points.push_back(unsorted[firstUnused]);
        used[firstUnused] = 1;
        --remaining;

        auto findCloserThan = [&](const ShorePoint& cur, float radius) -> int {
            int bestIdx = -1;
            float bestDist = 1e38f;
            for (size_t i = 0; i < unsorted.size(); ++i) {
                if (used[i]) continue;
                if (unsorted[i].x == cur.x && unsorted[i].y == cur.y) {
                    used[i] = 1;
                    --remaining;
                    continue;
                }
                float dx = static_cast<float>(cur.x - unsorted[i].x);
                float dy = static_cast<float>(cur.y - unsorted[i].y);
                float d = std::sqrt(dx * dx + dy * dy);
                if (d <= radius && d < bestDist) {
                    bool inBody = false;
                    for (const auto& pt : body.points) {
                        if (pt.x == unsorted[i].x && pt.y == unsorted[i].y) {
                            inBody = true;
                            break;
                        }
                    }
                    if (!inBody) {
                        bestDist = d;
                        bestIdx = static_cast<int>(i);
                    }
                }
            }
            return bestIdx;
        };

        bool done = false;
        while (!done && remaining > 0) {
            const ShorePoint cur = body.points.back();
            int bestIdx = findCloserThan(cur, maxDist);
            if (bestIdx >= 0) {
                body.points.push_back(unsorted[bestIdx]);
                used[bestIdx] = 1;
                --remaining;
            } else {
                const ShorePoint& front = body.points.front();
                float dx = static_cast<float>(cur.x - front.x);
                float dy = static_cast<float>(cur.y - front.y);
                float d = std::sqrt(dx * dx + dy * dy);
                if (d <= maxDistWithGap) {
                    if (d <= maxDist || d >= maxDistWithGap) {
                        done = true;
                    } else {
                        int midX = static_cast<int>(front.x + (cur.x - front.x) * 0.5);
                        int midY = static_cast<int>(front.y + (cur.y - front.y) * 0.5);
                        body.points.push_back({midX, midY, false});
                    }
                } else {
                    int gapIdx = findCloserThan(cur, maxDistWithGap);
                    if (gapIdx >= 0) {
                        int midX = static_cast<int>(cur.x + (unsorted[gapIdx].x - cur.x) * 0.5);
                        int midY = static_cast<int>(cur.y + (unsorted[gapIdx].y - cur.y) * 0.5);
                        body.points.push_back({midX, midY, false});
                        body.points.push_back(unsorted[gapIdx]);
                        used[gapIdx] = 1;
                        --remaining;
                    } else {
                        done = true;
                    }
                }
            }
        }

        if (!body.points.empty()) {
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
            bodies.push_back(std::move(body));
        }
    }
    return bodies;
}

std::shared_ptr<const std::vector<MeshBuilder::WaterBody>> MeshBuilder::buildWorldWaterBodies(
    const std::vector<std::vector<ShorePoint>>& perMapShorePoints)
{
    std::vector<ShorePoint> all;
    for (const auto& v : perMapShorePoints) {
        std::vector<ShorePoint> toAdd;
        toAdd.reserve(v.size());
        for (const auto& pt : v) {
            if (pt.isBorder) {
                auto it = std::find_if(all.begin(), all.end(), [&](const ShorePoint& sp) {
                    return sp.x == pt.x && sp.y == pt.y;
                });
                if (it != all.end()) {
                    all.erase(it);
                } else {
                    toAdd.push_back(pt);
                }
            } else {
                toAdd.push_back(pt);
            }
        }
        all.insert(all.end(), toAdd.begin(), toAdd.end());
    }
    return std::make_shared<const std::vector<WaterBody>>(sortShorePointsIntoWaterBodies(std::move(all)));
}

void MeshBuilder::ensureWaterBodies() const {
    if (m_shoreBodiesComputed) return;
    m_shoreBodiesComputed = true;
    if (m_worldWaterBodies) return;

    std::vector<std::vector<ShorePoint>> perMap;
    perMap.push_back(collectShorePoints());
    m_waterBodies = *buildWorldWaterBodies(perMap);
}

bool MeshBuilder::constructVertexDistanceToShoreArray(
    float vx, float vy, float array[12], int size,
    float* waterBodySpan, float* shoreUCoord) const
{
    ensureWaterBodies();
    const std::vector<WaterBody>& bodies = m_worldWaterBodies ? *m_worldWaterBodies : m_waterBodies;

    static constexpr float kZeroedShoreLookUpArray[12] = {
        1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f
    };

    const WaterBody* bodyToUse = nullptr;
    for (const auto& body : bodies) {
        if (vx >= static_cast<float>(body.minX) && vx <= static_cast<float>(body.maxX) &&
            vy >= static_cast<float>(body.minY) && vy <= static_cast<float>(body.maxY)) {
            bodyToUse = &body;
            break;
        }
    }

    if (!bodyToUse) {
        for (int i = 0; i < size && i < 12; ++i) array[i] = kZeroedShoreLookUpArray[i];
        if (waterBodySpan) *waterBodySpan = 0.0f;
        if (shoreUCoord) *shoreUCoord = 0.0f;
        return false;
    }

    float buffer[512];
    for (int i = 0; i < 512; ++i) buffer[i] = -1.0f;

    int closestShoreCoordIndex = -1;
    float closestShoreCoordDistance = 3.4028235e38f;

    auto fillBuffer = [&](const WaterBody& body, bool trackClosest) {
        int numPts = static_cast<int>(body.points.size());
        if (numPts <= 0) return;

        ShorePoint lastPt = body.points.back();
        float dx0 = static_cast<float>(lastPt.x) - vx;
        float dy0 = static_cast<float>(lastPt.y) - vy;
        float dist0 = std::sqrt(dx0 * dx0 + dy0 * dy0);
        float cosTheta0 = (dist0 <= 0.1f) ? 0.0f : (dy0 / dist0);
        int initial_buffer_entry = cosThetaToBufferIndex(cosTheta0, dx0 < 0.0f, 512);

        for (int index = 0; index < numPts; ++index) {
            const ShorePoint& curPt = body.points[index];
            float dx = static_cast<float>(curPt.x) - vx;
            float dy = static_cast<float>(curPt.y) - vy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float cosTheta = (dist <= 0.1f) ? 0.0f : (dy / dist);
            if (trackClosest && dist < closestShoreCoordDistance) {
                closestShoreCoordDistance = dist;
                closestShoreCoordIndex = index;
            }
            int final_buffer_entry = cosThetaToBufferIndex(cosTheta, dx < 0.0f, 512);

            if (final_buffer_entry == initial_buffer_entry) {
                int idx = final_buffer_entry % 512;
                if (idx < 0) idx += 512;
                if (buffer[idx] < 0.0f || dist0 < buffer[idx]) {
                    buffer[idx] = dist0;
                }
            } else {
                int initE = initial_buffer_entry;
                int finE = final_buffer_entry;
                ensureBufferIsCircular(initE, finE);
                if (initE >= finE) {
                    setBufferEntriesToInterpolatedShorePointDistance(buffer, vx, vy, finE, initE, lastPt, curPt);
                } else {
                    setBufferEntriesToInterpolatedShorePointDistance(buffer, vx, vy, initE, finE, curPt, lastPt);
                }
                initial_buffer_entry = finE % 512;
                if (initial_buffer_entry < 0) initial_buffer_entry += 512;
                lastPt = curPt;
                dist0 = dist;
            }
        }
    };

    fillBuffer(*bodyToUse, true);

    // FindPossibleIslandsInWaterBody (0x2e0a4c0)
    for (const auto& other : bodies) {
        if (!(other.minX == bodyToUse->minX && other.minY == bodyToUse->minY &&
              other.maxX == bodyToUse->maxX && other.maxY == bodyToUse->maxY)) {
            if (other.minX >= bodyToUse->minX && other.minY >= bodyToUse->minY &&
                other.maxX <= bodyToUse->maxX && other.maxY <= bodyToUse->maxY) {
                fillBuffer(other, true);
            }
        }
    }

    fillInvalidEntriesWithInterpolatedValues(buffer);

    // SupersampleBufferToArray (0x2e09030)
    int numEntries = static_cast<int>(std::floor(512.0f / static_cast<float>(size))); // 42 for size = 12
    int bIdx = 0;
    for (int s = 0; s < size; ++s) {
        float sum = 0.0f;
        int curEntries = (s == size - 1) ? (511 - bIdx) : numEntries; // 49 for s = 11
        for (int i = 0; i < curEntries && bIdx < 512; ++i) {
            sum += buffer[bIdx++];
        }
        array[s] = (curEntries > 0) ? (sum / static_cast<float>(curEntries)) : 0.0f;
    }

    for (int i = 0; i < size; ++i) {
        for (int j = i - 1; j >= 0; --j) {
            array[i] = array[i] - array[j];
        }
    }

    float spanDx = static_cast<float>(bodyToUse->maxX - bodyToUse->minX);
    float spanDy = static_cast<float>(bodyToUse->maxY - bodyToUse->minY);
    if (waterBodySpan) *waterBodySpan = std::sqrt(spanDx * spanDx + spanDy * spanDy);
    if (shoreUCoord) *shoreUCoord = computeShoreUCoord(vx, vy, *bodyToUse, closestShoreCoordIndex);

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
            const auto& cell = peekCell(gx, gy);
            minZ = std::min(minZ, cell.height);
            maxZ = std::max(maxZ, cell.height);

            float wh = peekInterpolatedWaterHeight(gx, gy, 2);
            patchWaterHeights[cx][cy] = wh;
            if (wh >= 0.001f) {
                patchHasWater = true;
                // The patch bounds are used by the game for foreground
                // culling. Water can sit above the highest terrain vertex, so
                // terrain-only bounds make visible water patches disappear
                // when the camera is nearly level.
                minZ = std::min(minZ, wh);
                maxZ = std::max(maxZ, wh);
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
                const LevData* cellLev = &m_lev;
                const auto& cell = peekCell(startX + x, startY + y, &cellLev);

                std::array<ThemeMaterial, 3> tempMaterials{};
                std::array<const ThemeMaterial*, 3> selected{nullptr, nullptr, nullptr};

                auto resolveSlotMaterial = [&](size_t slot) -> const ThemeMaterial* {
                    uint8_t thIdx = cell.themes[slot];
                    if (cellLev == &m_lev) {
                        if (thIdx < m_themeMaterials.size() && m_themeMaterials[thIdx].available) {
                            return &m_themeMaterials[thIdx];
                        }
                        return nullptr;
                    }
                    if (!cellLev || thIdx >= cellLev->themePalette.size()) return nullptr;
                    const std::string& tName = cellLev->themePalette[thIdx].name;
                    if (tName.empty()) return nullptr;

                    for (size_t ms = 0; ms < m_lev.themePalette.size() && ms < m_themeMaterials.size(); ++ms) {
                        if (m_lev.themePalette[ms].name == tName && m_themeMaterials[ms].available) {
                            return &m_themeMaterials[ms];
                        }
                    }

                    const auto* def = ThemeManager::instance().getTheme(tName);
                    if (!def) return nullptr;
                    auto& mat = tempMaterials[slot];
                    mat.available = true;
                    auto getTex = [](const std::string& name) -> uint32_t {
                        return name.empty() ? 0 : BankResolver::instance().getTextureId(name);
                    };
                    mat.base.textures[0] = getTex(def->baseTexture);
                    mat.base.textures[1] = getTex(def->backgroundTexture.empty() ? def->baseTexture : def->backgroundTexture);
                    mat.base.textures[2] = getTex(def->baseBumpMap);
                    mat.base.selfIllumination = def->baseSelfIllumination;
                    mat.cliff.textures[0] = getTex(def->cliffBaseTexture);
                    mat.cliff.textures[1] = getTex(def->cliffBackgroundTexture.empty() ? def->cliffBaseTexture : def->cliffBackgroundTexture);
                    mat.cliff.textures[2] = getTex(def->cliffBumpMap);
                    mat.cliff.selfIllumination = def->cliffSelfIllumination;
                    return &mat;
                };

                selected[0] = resolveSlotMaterial(0);
                if (selected[0] != nullptr) {
                    selected[1] = resolveSlotMaterial(1);
                    if (selected[1] != nullptr) {
                        selected[2] = resolveSlotMaterial(2);
                    }
                }

                std::array<uint8_t, 3> strengths{0, 0, 0};
                strengths[0] = cell.blends[0];
                strengths[1] = cell.blends[1];
                unsigned stored = unsigned(cell.blends[0]) + unsigned(cell.blends[1]);
                strengths[2] = stored >= 255 ? 0 : static_cast<uint8_t>(255 - stored);

                for (const auto& contribution : buildThemeContributions(selected, strengths)) {
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
            v.z = peekCell(gx, gy).height;

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
        patch.waterPatch.waterBodySpan = 0.0f;
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
                        wh = peekLandscapeHeight(gx, gy) - 1.0f;
                    }
                }

                float offset = (patch.waterPatch.waterType == 8) ? 0.001f : 0.1f; // 8 == WATER_TYPE_ICE
                double v23 = (wh - offset <= 0.0f) ? 0.0 : static_cast<double>(wh - offset);
                float posZ = static_cast<float>(static_cast<uint32_t>(static_cast<int64_t>(v23 * 256.0)) / 256.0);

                float ly = worldY / 6.283185482025146f;
                double za = std::sin(6.283185482025146 * static_cast<double>(ly));
                float lx = worldX / 6.283185482025146f;
                float sa = static_cast<float>(std::sin(6.283185482025146 * static_cast<double>(lx)) + za);

                float lcy = worldY / 6.283185482025146f;
                double zb = std::cos(6.283185482025146 * static_cast<double>(lcy));
                float lcx = worldX / 6.283185482025146f;
                float ca = static_cast<float>(std::cos(6.283185482025146 * static_cast<double>(lcx)) + zb);

                float cell_h = peekLandscapeHeight(gx, gy);
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

                float vertexSpan = 0.0f;
                constructVertexDistanceToShoreArray(worldX, worldY, wv.shoreDists, 12, &vertexSpan, &wv.shoreU);
                patch.waterPatch.waterBodySpan = vertexSpan;
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
