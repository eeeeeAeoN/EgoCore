#pragma once

#include "egostb/lev_reader.hpp"
#include "egostb/theme_manager.hpp"
#include <vector>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace egostb {

struct ShorePoint {
    int x = 0;
    int y = 0;
};

struct WaterBody {
    std::vector<ShorePoint> points;
    int minX = 0, maxX = 0;
    int minY = 0, maxY = 0;
    float span = 0.0f;
};

struct WaterCell {
    uint8_t hasWater = 0;
    uint8_t dominantType = 0;
    uint8_t hasWaterFast = 0;
    uint8_t waterType = 0;
    float waterDepth = 0.0f;
    float waterHeight = 0.0f;
};

struct PrecomputedWaterGrid {
    int width = 0;
    int height = 0;
    std::vector<WaterCell> cells;

    inline const WaterCell& get(int x, int y) const {
        int cx = std::clamp(x, 0, width - 1);
        int cy = std::clamp(y, 0, height - 1);
        return cells[cy * width + cx];
    }
};

struct WorldWaterNeighbor {
    int mapX = 0;
    int mapY = 0;
    int width = 0;
    int height = 0;
    const LevData* lev = nullptr;
    const PrecomputedWaterGrid* waterGrid = nullptr;
    const std::vector<std::pair<int, int>>* shorePoints = nullptr;

    inline bool containsWorld(int wx, int wy) const {
        return (wx >= mapX && wx < mapX + width &&
                wy >= mapY && wy < mapY + height);
    }

    inline float getWaterDepth(int wx, int wy) const {
        if (!waterGrid) return 0.0f;
        int lx = wx - mapX;
        int ly = wy - mapY;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return 0.0f;
        return waterGrid->cells[ly * waterGrid->width + lx].waterDepth;
    }

    inline const WaterCell* getWaterCell(int wx, int wy) const {
        if (!waterGrid) return nullptr;
        int lx = wx - mapX;
        int ly = wy - mapY;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return nullptr;
        return &waterGrid->cells[ly * waterGrid->width + lx];
    }

    inline float getLandscapeHeight(int wx, int wy) const {
        if (!lev) return 0.0f;
        int lx = wx - mapX;
        int ly = wy - mapY;
        if (lx < 0 || lx >= width || ly < 0 || ly >= height) return 0.0f;
        float h = lev->getCell(lx, ly).height;
        return static_cast<float>(static_cast<int>(h * 128.0f)) * 0.0078125f;
    }
};

inline PrecomputedWaterGrid buildWaterGrid(const LevData& lev) {
    PrecomputedWaterGrid grid;
    grid.width = lev.gridWidth;
    grid.height = lev.gridHeight;
    grid.cells.resize(grid.width * grid.height);

    std::vector<const EngineThemeDef*> paletteDefs(lev.themePalette.size(), nullptr);
    for (size_t i = 0; i < lev.themePalette.size(); ++i) {
        if (!lev.themePalette[i].name.empty()) {
            paletteDefs[i] = ThemeManager::instance().getTheme(lev.themePalette[i].name);
        }
    }

    for (int cy = 0; cy < grid.height; ++cy) {
        for (int cx = 0; cx < grid.width; ++cx) {
            const auto& cell = lev.getCell(cx, cy);

            // Quantize height matching CEngineMap::QuantiseLandscapeHeight (0x2ccc750)
            float qh = static_cast<float>(static_cast<int>(cell.height * 128.0f)) * 0.0078125f;

            // Determine themes in slots 0, 1, 2
            // In FableWin.exe, slot 1 is only checked if slot 0 has a valid theme (!name0.empty()),
            // and slot 2 is only checked if slot 1 also has a valid theme (!name1.empty()).
            const EngineThemeDef* def0 = nullptr;
            if (cell.themes[0] < paletteDefs.size()) def0 = paletteDefs[cell.themes[0]];

            const EngineThemeDef* def1 = nullptr;
            if (def0 != nullptr && cell.themes[1] < paletteDefs.size()) def1 = paletteDefs[cell.themes[1]];

            const EngineThemeDef* def2 = nullptr;
            if (def1 != nullptr && cell.themes[2] < paletteDefs.size()) def2 = paletteDefs[cell.themes[2]];

            uint8_t hasWaterFast = 0;
            if (def0 && def0->waterType > 0) {
                hasWaterFast = 1;
            } else if (def1 && def1->waterType > 0) {
                hasWaterFast = 1;
            } else if (def2 && def2->waterType > 0) {
                hasWaterFast = 1;
            }

            uint8_t waterType = 0;
            if (def0 && def0->waterType > 0) {
                waterType = static_cast<uint8_t>(def0->waterType);
            }
            if (def1 && def1->waterType > 0) {
                if (waterType == 0 || waterType == 7) {
                    waterType = static_cast<uint8_t>(def1->waterType);
                }
                if (def2 && def2->waterType > 0) {
                    if (waterType == 0 || waterType == 7) {
                        waterType = static_cast<uint8_t>(def2->waterType);
                    }
                }
            }

            float depth = 0.0f;
            int dominantType = 0;
            float maxWaterContribution = 0.0f;
            for (int t = 0; t < 3; ++t) {
                uint8_t thIdx = cell.themes[t];
                if (thIdx < paletteDefs.size() && paletteDefs[thIdx] != nullptr) {
                    const auto* def = paletteDefs[thIdx];
                    if (def->waterType > 0) {
                        float blend = 0.0f;
                        if (t == 0) {
                            blend = static_cast<float>(cell.blends[0]) / 255.0f;
                        } else if (t == 1) {
                            blend = static_cast<float>(cell.blends[1]) / 255.0f;
                        } else {
                            int b0 = cell.blends[0];
                            int b1 = cell.blends[1];
                            int rem = 255 - b0 - b1;
                            if (rem > 0) blend = static_cast<float>(rem) / 255.0f;
                        }
                        float contrib = def->waterHeight * blend;
                        depth += contrib;
                        if (contrib > maxWaterContribution || dominantType == 0) {
                            maxWaterContribution = contrib;
                            dominantType = def->waterType;
                        }
                    }
                }
            }

            if (dominantType == 0 || waterType != 0) {
                dominantType = (waterType != 0) ? waterType : dominantType;
            }

            auto& wc = grid.cells[cy * grid.width + cx];
            wc.hasWaterFast = hasWaterFast;
            wc.waterType = waterType;
            wc.hasWater = (hasWaterFast && depth > 0.001f) ? 1 : 0;
            wc.dominantType = static_cast<uint8_t>(dominantType);
            wc.waterDepth = depth;
            wc.waterHeight = (depth > 0.001f) ? (qh + depth) : 0.0f;
        }
    }
    return grid;
}

inline const PrecomputedWaterGrid& getMapWaterGrid(const LevData& lev) {
    static thread_local uint32_t tl_cachedLevChecksum = 0;
    static thread_local PrecomputedWaterGrid tl_cachedWaterGrid;
    if (tl_cachedLevChecksum != lev.checksum || tl_cachedWaterGrid.cells.empty()) {
        tl_cachedWaterGrid = buildWaterGrid(lev);
        tl_cachedLevChecksum = lev.checksum;
    }
    return tl_cachedWaterGrid;
}

class WaterGenerator {
public:
    static float peekWaterDepth(const LevData& lev, int gx, int gy) {
        if (gx < 0 || gx >= lev.gridWidth || gy < 0 || gy >= lev.gridHeight) return 0.0f;
        const auto& cell = lev.getCell(gx, gy);
        float depth = 0.0f;
        for (int t = 0; t < 3; ++t) {
            uint8_t thIdx = cell.themes[t];
            if (thIdx < lev.themePalette.size()) {
                const auto& name = lev.themePalette[thIdx].name;
                if (!name.empty()) {
                    const auto* def = ThemeManager::instance().getTheme(name);
                    if (def && def->waterType > 0) {
                        float blend = 0.0f;
                        if (t == 0) {
                            blend = static_cast<float>(cell.blends[0]) / 255.0f;
                        } else if (t == 1) {
                            blend = static_cast<float>(cell.blends[1]) / 255.0f;
                        } else {
                            int b0 = cell.blends[0];
                            int b1 = cell.blends[1];
                            int rem = 255 - b0 - b1;
                            if (rem > 0) blend = static_cast<float>(rem) / 255.0f;
                        }
                        depth += def->waterHeight * blend;
                    }
                }
            }
        }
        return depth;
    }

    const WaterCell* resolveWaterCell(int gx, int gy) const {
        if (gx >= 0 && gx < m_width && gy >= 0 && gy < m_height) {
            return &m_waterGrid.cells[gy * m_waterGrid.width + gx];
        }
        int wx = m_worldX + gx;
        int wy = m_worldY + gy;
        if (m_neighbors) {
            for (const auto& n : *m_neighbors) {
                if (n.containsWorld(wx, wy)) {
                    const WaterCell* cell = n.getWaterCell(wx, wy);
                    if (cell) return cell;
                }
            }
        }
        int cx = std::clamp(gx, 0, m_width - 1);
        int cy = std::clamp(gy, 0, m_height - 1);
        return &m_waterGrid.cells[cy * m_waterGrid.width + cx];
    }

    bool peekHasWater(int gx, int gy) const {
        const WaterCell* c = resolveWaterCell(gx, gy);
        return c ? (c->hasWater != 0) : false;
    }

    bool peekHasWaterFast(int gx, int gy) const {
        const WaterCell* c = resolveWaterCell(gx, gy);
        return c ? (c->hasWaterFast != 0) : false;
    }

    float peekWaterDepth(int gx, int gy) const {
        const WaterCell* c = resolveWaterCell(gx, gy);
        return c ? c->waterDepth : 0.0f;
    }

    uint8_t peekWaterType(int gx, int gy) const {
        const WaterCell* c = resolveWaterCell(gx, gy);
        return c ? c->waterType : 0;
    }

    bool peekInterpolatedHasWater(int gx, int gy, int distanceToLook = 1) const {
        for (int ix = gx - distanceToLook; ix <= gx + distanceToLook; ++ix) {
            for (int iy = gy - distanceToLook; iy <= gy + distanceToLook; ++iy) {
                if (peekHasWater(ix, iy)) return true;
            }
        }
        return false;
    }

    float peekInterpolatedWaterDepth(int mapx, int mapy, int distance_to_look = 1) const {
        int x0 = mapx - distance_to_look;
        int y0 = mapy - distance_to_look;
        int x1 = mapx + distance_to_look;
        int y1 = mapy + distance_to_look;
        if (!peekHasWaterFast(mapx, mapy)) {
            bool found = false;
            for (int i = mapx - distance_to_look; i <= x1 && !found; ++i) {
                for (int j = mapy - distance_to_look; j <= y1 && !found; ++j) {
                    if (peekHasWaterFast(i, j)) {
                        x0 = i - distance_to_look;
                        y0 = j - distance_to_look;
                        x1 = i + distance_to_look;
                        y1 = j + distance_to_look;
                        found = true;
                    }
                }
            }
            if (!found) return 0.0f;
        }
        float sum = 0.0f;
        float count = 0.0f;
        for (int k = x0; k <= x1; ++k) {
            for (int m = y0; m <= y1; ++m) {
                float d = peekWaterDepth(k, m);
                if (d > 0.001f) {
                    sum += d;
                    count += 1.0f;
                }
            }
        }
        if (count <= 0.0f) return 0.0f;
        return sum / count;
    }

    void init(const LevData& lev, int worldX, int worldY, const std::vector<WorldWaterNeighbor>* neighbors = nullptr) {
        m_bodies.clear();
        m_hasAnyWater = false;
        m_waterGrid = buildWaterGrid(lev);
        m_width = lev.width;
        m_height = lev.height;
        m_worldX = worldX;
        m_worldY = worldY;
        m_neighbors = neighbors;

        int w = m_width;
        int h = m_height;

        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                if (m_waterGrid.cells[cy * m_waterGrid.width + cx].hasWater) {
                    m_hasAnyWater = true;
                    break;
                }
            }
            if (m_hasAnyWater) break;
        }
        if (!m_hasAnyWater) return;

        // 1. FindShorePointsInMap (0x2e07490)
        std::vector<ShorePoint> unsorted;
        std::unordered_set<uint64_t> seen;

        auto addPoint = [&](int px, int py) {
            uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(px)) << 32) | static_cast<uint32_t>(py);
            if (seen.insert(key).second) {
                unsorted.push_back({px, py});
            }
        };

        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                if (!peekInterpolatedHasWater(cx, cy, 1)) {
                    int minX = std::max(0, cx - 1);
                    int maxX = std::min(w - 1, cx + 1);
                    int minY = std::max(0, cy - 1);
                    int maxY = std::min(h - 1, cy + 1);
                    bool hasWaterNear = false;
                    for (int ny = minY; ny <= maxY && !hasWaterNear; ++ny) {
                        for (int nx = minX; nx <= maxX && !hasWaterNear; ++nx) {
                            if (peekInterpolatedWaterDepth(nx, ny, 1) >= 0.001f) {
                                hasWaterNear = true;
                            }
                        }
                    }
                    if (hasWaterNear) {
                        addPoint(worldX + cx, worldY + cy);
                    }
                } else {
                    const WaterCell* wc = resolveWaterCell(cx, cy);
                    if (wc && wc->waterType == 7) { // WATER_TYPE_DUMMY_SHORE_POINT
                        addPoint(worldX + cx, worldY + cy);
                    } else if (cx == 0 || cx == w - 1 || cy == 0 || cy == h - 1) {
                        bool emitBorder = false;
                        if (cx == 0 && !peekInterpolatedHasWater(-1, cy, 1)) emitBorder = true;
                        if (cx == w - 1 && !peekInterpolatedHasWater(w, cy, 1)) emitBorder = true;
                        if (cy == 0 && !peekInterpolatedHasWater(cx, -1, 1)) emitBorder = true;
                        if (cy == h - 1 && !peekInterpolatedHasWater(cx, h, 1)) emitBorder = true;

                        if (emitBorder) {
                            int px = worldX + cx + (cx == w - 1 ? 1 : 0);
                            int py = worldY + cy + (cx != w - 1 && cy == h - 1 ? 1 : 0);
                            addPoint(px, py);
                        }
                    }
                }
            }
        }

        // Include real shoreline points from touching neighbors within a margin
        if (neighbors) {
            const int margin = 32;
            int minWx = worldX - margin;
            int maxWx = worldX + w + margin;
            int minWy = worldY - margin;
            int maxWy = worldY + h + margin;

            for (const auto& n : *neighbors) {
                if (!n.shorePoints) continue;
                for (const auto& pt : *n.shorePoints) {
                    if (pt.first >= minWx && pt.first <= maxWx &&
                        pt.second >= minWy && pt.second <= maxWy) {
                        if (n.getWaterDepth(pt.first, pt.second) <= 0.001f) {
                            addPoint(pt.first, pt.second);
                        }
                    }
                }
            }
        }

        // 2. SortShorePointsIntoWaterBodies (0x2e083b0)
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
                        if (d <= maxDist || d >= maxDistWithGap) {
                            done = true;
                        } else {
                            body.points.push_back({
                                static_cast<int>(body.points.front().x + (cur.x - body.points.front().x) * 0.5),
                                static_cast<int>(body.points.front().y + (cur.y - body.points.front().y) * 0.5)
                            });
                        }
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
                                static_cast<int>(cur.x + (unsorted[gapIdx].x - cur.x) * 0.5),
                                static_cast<int>(cur.y + (unsorted[gapIdx].y - cur.y) * 0.5)
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
                m_bodies.push_back(std::move(body));
            }
        }
    }

    bool computeShoreLookup(
        float vx, float vy, float array[12], int size = 12,
        float* waterBodySpan = nullptr, float* shoreUCoord = nullptr) const
    {
        const WaterBody* bodyToUse = nullptr;
        for (const auto& body : m_bodies) {
            if (vx >= static_cast<float>(body.minX) && vx <= static_cast<float>(body.maxX) &&
                vy >= static_cast<float>(body.minY) && vy <= static_cast<float>(body.maxY)) {
                bodyToUse = &body;
                break;
            }
        }

        if (!bodyToUse) {
            static constexpr float kZeroedShoreLookUpArray[12] = {
                1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f
            };
            for (int i = 0; i < size && i < 12; ++i) array[i] = kZeroedShoreLookUpArray[i];
            if (waterBodySpan) *waterBodySpan = 0.0f;
            if (shoreUCoord) *shoreUCoord = 0.0f;
            return false;
        }

        if (waterBodySpan) *waterBodySpan = bodyToUse->span;
        if (shoreUCoord) *shoreUCoord = 0.0f;

        float buffer[512];
        for (int i = 0; i < 512; ++i) buffer[i] = -1.0f;

        auto cosThetaToBufferIndex = [](float cosTheta, bool isInQuad3Or4) -> int {
            cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
            float angle = std::acos(cosTheta) / 6.283185482025146f;
            if (isInQuad3Or4) angle = 1.0f - angle;
            return std::clamp(static_cast<int>(std::floor(angle * 511.0f)), 0, 511);
        };

        auto ensureBufferIsCircular = [](int& initial, int& final) {
            if (initial - final <= final + 512 - initial) {
                if (final - initial > initial + 512 - final) {
                    initial += 512;
                }
            } else {
                final += 512;
            }
        };

        auto setBufferEntryIfCloser = [&](int idx, float dist) {
            int bIdx = idx % 512;
            if (bIdx < 0) bIdx += 512;
            if (buffer[bIdx] < 0.0f || dist < buffer[bIdx]) {
                buffer[bIdx] = dist;
            }
        };

        auto fillBuffer = [&](const WaterBody& body) {
            int numPts = static_cast<int>(body.points.size());
            if (numPts < 2) return;

            ShorePoint lastPt = body.points.back();
            float dx0 = static_cast<float>(lastPt.x) - vx;
            float dy0 = static_cast<float>(lastPt.y) - vy;
            float dist0 = std::sqrt(dx0 * dx0 + dy0 * dy0);
            float cosTheta0 = (dist0 <= 0.1f) ? 0.0f : (dy0 / dist0);
            int lastEntry = cosThetaToBufferIndex(cosTheta0, dx0 < 0.0f);

            for (int i = 0; i < numPts; ++i) {
                const ShorePoint& curPt = body.points[i];
                float dx = static_cast<float>(curPt.x) - vx;
                float dy = static_cast<float>(curPt.y) - vy;
                float dist = std::sqrt(dx * dx + dy * dy);
                float cosTheta = (dist <= 0.1f) ? 0.0f : (dy / dist);
                int curEntry = cosThetaToBufferIndex(cosTheta, dx < 0.0f);

                if (curEntry == lastEntry) {
                    setBufferEntryIfCloser(curEntry, dist0);
                } else {
                    int startE = lastEntry;
                    int endE = curEntry;
                    ensureBufferIsCircular(startE, endE);
                    if (startE < endE) {
                        int count = endE - startE;
                        for (int k = 0; k <= count; ++k) {
                            float interp = (count <= 0) ? 0.0f : (static_cast<float>(k) / static_cast<float>(count));
                            float sx = static_cast<float>(lastPt.x) + static_cast<float>(curPt.x - lastPt.x) * interp;
                            float sy = static_cast<float>(lastPt.y) + static_cast<float>(curPt.y - lastPt.y) * interp;
                            float d = std::sqrt((sx - vx) * (sx - vx) + (sy - vy) * (sy - vy));
                            setBufferEntryIfCloser(startE + k, d);
                        }
                    } else {
                        int count = startE - endE;
                        for (int k = 0; k <= count; ++k) {
                            float interp = (count <= 0) ? 0.0f : (static_cast<float>(k) / static_cast<float>(count));
                            float sx = static_cast<float>(curPt.x) + static_cast<float>(lastPt.x - curPt.x) * interp;
                            float sy = static_cast<float>(curPt.y) + static_cast<float>(lastPt.y - curPt.y) * interp;
                            float d = std::sqrt((sx - vx) * (sx - vx) + (sy - vy) * (sy - vy));
                            setBufferEntryIfCloser(endE + k, d);
                        }
                    }
                    lastEntry = endE % 512;
                    if (lastEntry < 0) lastEntry += 512;
                    lastPt = curPt;
                    dist0 = dist;
                }
            }
        };

        fillBuffer(*bodyToUse);

        for (const auto& other : m_bodies) {
            bool isSame = (other.minX == bodyToUse->minX && other.minY == bodyToUse->minY &&
                           other.maxX == bodyToUse->maxX && other.maxY == bodyToUse->maxY);
            if (!isSame &&
                other.minX >= bodyToUse->minX && other.minY >= bodyToUse->minY &&
                other.maxX <= bodyToUse->maxX && other.maxY <= bodyToUse->maxY) {
                fillBuffer(other);
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

        int bIdx = 0;
        int numEntries = static_cast<int>(std::floor(512.0f / static_cast<float>(size)));
        for (int s = 0; s < size; ++s) {
            float sum = 0.0f;
            int curEntries = (s == size - 1) ? (511 - bIdx) : numEntries;
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

    bool hasWater() const { return m_hasAnyWater; }
    const std::vector<WaterBody>& bodies() const { return m_bodies; }

    bool areaHasWater(int minX, int minY, int maxX, int maxY, int worldX = 0, int worldY = 0) const {
        if (!m_hasAnyWater) return false;
        if (m_waterGrid.cells.empty()) return true;
        int lx0 = std::clamp(minX - worldX, 0, m_waterGrid.width - 1);
        int lx1 = std::clamp(maxX - worldX, 0, m_waterGrid.width - 1);
        int ly0 = std::clamp(minY - worldY, 0, m_waterGrid.height - 1);
        int ly1 = std::clamp(maxY - worldY, 0, m_waterGrid.height - 1);
        for (int y = ly0; y <= ly1; ++y) {
            for (int x = lx0; x <= lx1; ++x) {
                if (m_waterGrid.cells[y * m_waterGrid.width + x].hasWater) return true;
            }
        }
        return false;
    }

private:
    std::vector<WaterBody> m_bodies;
    bool m_hasAnyWater = false;
    PrecomputedWaterGrid m_waterGrid;
    int m_width = 0;
    int m_height = 0;
    int m_worldX = 0;
    int m_worldY = 0;
    const std::vector<WorldWaterNeighbor>* m_neighbors = nullptr;
};

using WaterShoreCalculator = WaterGenerator;

} // namespace egostb
