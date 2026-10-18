#pragma once

#include "egostb/lev_reader.hpp"
#include "egostb/theme_manager.hpp"
#include <vector>
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
        return (wx >= mapX && wx <= mapX + width &&
                wy >= mapY && wy <= mapY + height);
    }

    inline float getWaterDepth(int wx, int wy) const {
        if (!waterGrid) return 0.0f;
        int lx = wx - mapX;
        int ly = wy - mapY;
        if (lx < 0 || lx >= waterGrid->width || ly < 0 || ly >= waterGrid->height) return 0.0f;
        return waterGrid->cells[ly * waterGrid->width + lx].waterDepth;
    }

    inline const WaterCell* getWaterCell(int wx, int wy) const {
        if (!waterGrid) return nullptr;
        int lx = wx - mapX;
        int ly = wy - mapY;
        if (lx < 0 || lx >= waterGrid->width || ly < 0 || ly >= waterGrid->height) return nullptr;
        return &waterGrid->cells[ly * waterGrid->width + lx];
    }

    inline float getLandscapeHeight(int wx, int wy) const {
        if (!lev) return 0.0f;
        int lx = wx - mapX;
        int ly = wy - mapY;
        int cx = std::clamp(lx, 0, lev->width - 1);
        int cy = std::clamp(ly, 0, lev->height - 1);
        return lev->getCell(cx, cy).height;
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
            auto& wc = grid.cells[cy * grid.width + cx];
            if (depth > 0.001f) {
                wc.hasWater = 1;
                wc.dominantType = static_cast<uint8_t>(dominantType);
                wc.waterDepth = depth;
                wc.waterHeight = cell.height + depth;
            } else {
                wc.hasWater = 0;
                wc.dominantType = 0;
                wc.waterDepth = 0.0f;
                wc.waterHeight = cell.height;
            }
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

class WaterShoreCalculator {
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

    void init(const LevData& lev, int worldX, int worldY, const std::vector<WorldWaterNeighbor>* neighbors = nullptr) {
        m_bodies.clear();
        m_hasAnyWater = false;
        m_waterGrid = buildWaterGrid(lev);

        int w = lev.gridWidth;
        int h = lev.gridHeight;

        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                if (m_waterGrid.cells[cy * w + cx].hasWater) {
                    m_hasAnyWater = true;
                    break;
                }
            }
            if (m_hasAnyWater) break;
        }
        if (!m_hasAnyWater) return;

        // 1. FindShorePointsInMap (0x12cd3e0)
        std::vector<ShorePoint> unsorted;
        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                float d = m_waterGrid.cells[cy * w + cx].waterDepth;
                if (d <= 0.001f) {
                    int minX = std::max(0, cx - 1);
                    int maxX = std::min(w - 1, cx + 1);
                    int minY = std::max(0, cy - 1);
                    int maxY = std::min(h - 1, cy + 1);
                    bool hasWaterNear = false;
                    for (int ny = minY; ny <= maxY && !hasWaterNear; ++ny) {
                        for (int nx = minX; nx <= maxX && !hasWaterNear; ++nx) {
                            if (m_waterGrid.cells[ny * w + nx].waterDepth >= 0.001f) {
                                hasWaterNear = true;
                            }
                        }
                    }
                    if (hasWaterNear) {
                        unsorted.push_back({worldX + cx, worldY + cy});
                    }
                } else {
                    if (cx == 0 || cx == w - 1 || cy == 0 || cy == h - 1) {
                        bool neighborHasWater = false;
                        if (neighbors) {
                            int checkX = worldX + cx;
                            int checkY = worldY + cy;
                            if (cx == 0) checkX -= 1;
                            else if (cx == w - 1) checkX += 1;
                            if (cy == 0) checkY -= 1;
                            else if (cy == h - 1) checkY += 1;

                            for (const auto& n : *neighbors) {
                                if (n.containsWorld(checkX, checkY) && n.getWaterDepth(checkX, checkY) > 0.001f) {
                                    neighborHasWater = true;
                                    break;
                                }
                            }
                        }
                        if (!neighborHasWater) {
                            unsorted.push_back({worldX + cx, worldY + cy});
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

        if (!bodyToUse && !m_bodies.empty()) {
            float bestDistSq = 1e30f;
            for (const auto& body : m_bodies) {
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

        if (waterBodySpan) *waterBodySpan = bodyToUse->span;
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
};


} // namespace egostb
