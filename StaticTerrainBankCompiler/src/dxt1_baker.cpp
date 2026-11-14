#include "egostb/dxt1_baker.hpp"
#include "egostb/bank_resolver.hpp"
#include "egostb/normal_lut.hpp"
#include "egostb/theme_manager.hpp"
#define STB_DXT_STATIC
#define STB_DXT_IMPLEMENTATION
#include "egostb/stb_dxt.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <string>
#include <array>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <thread>
#include <atomic>

namespace egostb {

namespace {

static constexpr int s_kOffsets[2][2][3][2] = {
    {{{0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}}, {{1, 0}, {0, 1}, {1, 1}}},
};

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 1.0f;
};

static const HeightMapCell& peekCellWithNeighbors(
    const LevData& lev, int worldX, int worldY, int gx, int gy,
    const std::vector<WorldWaterNeighbor>* neighbors,
    const LevData** outLev = nullptr
) {
    if (gx >= 0 && gx < lev.width && gy >= 0 && gy < lev.height) {
        if (outLev) *outLev = &lev;
        return lev.getCell(gx, gy);
    }
    int wx = worldX + gx;
    int wy = worldY + gy;
    if (neighbors) {
        for (const auto& n : *neighbors) {
            if (n.lev && wx >= n.mapX && wx < n.mapX + n.width &&
                wy >= n.mapY && wy < n.mapY + n.height) {
                if (outLev) *outLev = n.lev;
                return n.lev->getCell(wx - n.mapX, wy - n.mapY);
            }
        }
    }
    int cx = std::clamp(gx, 0, lev.width - 1);
    int cy = std::clamp(gy, 0, lev.height - 1);
    if (outLev) *outLev = &lev;
    return lev.getCell(cx, cy);
}

static Vector3 buildMapDirMask(
    const LevData& lev, int worldX, int worldY, int gx, int gy,
    const std::vector<WorldWaterNeighbor>* neighbors = nullptr
) {
    auto sampleHeight = [&](int x, int y) -> float {
        return peekCellWithNeighbors(lev, worldX, worldY, x, y, neighbors).height;
    };
    const float center = sampleHeight(gx, gy);
    const float left   = sampleHeight(gx - 1, gy) - center;
    const float right  = sampleHeight(gx + 1, gy) - center;
    const float up     = sampleHeight(gx, gy - 1) - center;
    const float down   = sampleHeight(gx, gy + 1) - center;
    Vector3 faces[8]{};
    size_t count = 0;
    if (((worldX + gx) ^ (worldY + gy)) & 1) {
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

static inline float getDirectionBlend(uint8_t direction, const Vector3& normal) {
    const float topness = std::clamp(
        (std::asin(std::clamp(normal.z, -1.0f, 1.0f)) /
             (3.14159265358979323846f * 0.5f) - 0.5f) * 4.0f,
        0.0f, 1.0f);
    if (direction == 0) return topness;
    if (direction > 4 || topness >= 1.0f) return 0.0f;
    const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    if (length == 0.0f) return 0.0f;
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
    return (1.0f - topness) * sideness;
}

static inline bool directionActive(uint8_t direction, const Vector3& normal) {
    return getDirectionBlend(direction, normal) > 0.0f;
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

struct Pass {
    MaterialTuple material;
    uint8_t direction = 0;
    std::array<uint8_t, 17 * 17> blend{};   // VertexBlend, indexed [y * 17 + x]
    std::array<bool, 512> triangles{};      // PolyMaskGrid, indexed [(x * 16 + y) * 2 + tri]
    int polyCount = 0;
    const DecodedTexture* decodedTexture = nullptr;
    std::array<float, 17 * 17> dirAlpha{};
    std::array<float, 17 * 17> combinedAlpha{};
};

struct FgPatchData {
    int patchX = 0;
    int patchY = 0;
    int startX = 0;
    int startY = 0;
    std::array<Vector3, 17 * 17> normals;
    std::array<float, 17 * 17> heights;
    std::vector<Pass> orderedPasses;
};

static const DecodedTexture* resolveFallbackTexture(
    const MaterialTuple& mat,
    const std::vector<ThemeMaterial>& materials,
    const LevData& lev
) {
    for (size_t s = 0; s < materials.size() && s < lev.themePalette.size(); ++s) {
        if (!materials[s].available) continue;
        if (materials[s].base == mat || materials[s].cliff == mat) {
            const std::string& name = lev.themePalette[s].name;
            if (!name.empty()) {
                const auto* def = ThemeManager::instance().getTheme(name);
                if (def) {
                    if (materials[s].cliff == mat) {
                        if (!def->cliffBackgroundTexture.empty()) {
                            auto* dt = BankResolver::instance().getDecodedTexture(def->cliffBackgroundTexture);
                            if (dt) return dt;
                        }
                        if (!def->cliffBaseTexture.empty()) {
                            auto* dt = BankResolver::instance().getDecodedTexture(def->cliffBaseTexture);
                            if (dt) return dt;
                        }
                    }
                    if (!def->backgroundTexture.empty()) {
                        auto* dt = BankResolver::instance().getDecodedTexture(def->backgroundTexture);
                        if (dt) return dt;
                    }
                    if (!def->baseTexture.empty()) {
                        auto* dt = BankResolver::instance().getDecodedTexture(def->baseTexture);
                        if (dt) return dt;
                    }
                }
            }
        }
    }
    return nullptr;
}

static void initFgPatch(
    FgPatchData& fgPatch,
    int px, int py,
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    const std::vector<WorldWaterNeighbor>* neighbors = nullptr
) {
    fgPatch.patchX = px;
    fgPatch.patchY = py;
    fgPatch.startX = px * 16;
    fgPatch.startY = py * 16;

    for (int y = 0; y <= 16; ++y) {
        for (int x = 0; x <= 16; ++x) {
            int gx = fgPatch.startX + x;
            int gy = fgPatch.startY + y;
            fgPatch.heights[size_t(y) * 17 + x] = peekCellWithNeighbors(lev, worldX, worldY, gx, gy, neighbors).height;
            fgPatch.normals[size_t(y) * 17 + x] = buildMapDirMask(lev, worldX, worldY, gx, gy, neighbors);
        }
    }

    std::vector<Pass> passes;
    if (materials.empty()) {
        Pass pass;
        pass.direction = 0;
        pass.material.textures = {1, 1, 0};
        pass.blend.fill(255);
        pass.triangles.fill(true);
        pass.polyCount = 512;
        passes.push_back(pass);
    } else {
        std::array<std::array<bool, 5>, 17 * 17> dirMask{};
        for (int y = 0; y <= 16; ++y) {
            for (int x = 0; x <= 16; ++x) {
                for (uint8_t d = 0; d < 5; ++d) {
                    dirMask[size_t(y) * 17 + x][d] = directionActive(d, fgPatch.normals[size_t(y) * 17 + x]);
                }
            }
        }

        auto getPass = [&](const MaterialTuple& mat, uint8_t dir) -> Pass& {
            for (auto& p : passes) {
                if (p.direction == dir && p.material == mat) return p;
            }
            passes.push_back({});
            passes.back().material = mat;
            passes.back().direction = dir;
            return passes.back();
        };

        auto addPolys = [&](Pass& pass, int ppx, int ppy) {
            for (int qy = ppy - 1; qy <= ppy; ++qy) {
                if (qy < 0 || qy > 15) continue;
                for (int qx = ppx - 1; qx <= ppx; ++qx) {
                    if (qx < 0 || qx > 15) continue;
                    for (int tri = 0; tri < 2; ++tri) {
                        bool mask = false;
                        bool sharesPoint = false;
                        for (int corner = 0; corner < 3; ++corner) {
                            const int vx = qx + s_kOffsets[(qx ^ qy) & 1][tri][corner][0];
                            const int vy = qy + s_kOffsets[(qx ^ qy) & 1][tri][corner][1];
                            mask |= dirMask[size_t(vy) * 17 + vx][pass.direction];
                            sharesPoint |= (vx == ppx && vy == ppy);
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

        passes.reserve(8);
        std::vector<ThemeMaterial> customMaterials;

        for (int y = 0; y <= 16; ++y) {
            for (int x = 0; x <= 16; ++x) {
                const LevData* cellLev = &lev;
                const auto& cell = peekCellWithNeighbors(lev, worldX, worldY, fgPatch.startX + x, fgPatch.startY + y, neighbors, &cellLev);

                const std::vector<ThemeMaterial>* activeMaterials = &materials;
                ThemeBlend blend;
                for (size_t slot = 0; slot < 3; ++slot) {
                    uint8_t thIdx = cell.themes[slot];
                    if (cellLev == &lev) {
                        blend.indices[slot] = thIdx;
                    } else if (cellLev && thIdx < cellLev->themePalette.size()) {
                        const std::string& tName = cellLev->themePalette[thIdx].name;
                        uint8_t mappedIdx = 0;
                        if (!tName.empty()) {
                            for (size_t ms = 0; ms < lev.themePalette.size() && ms < materials.size(); ++ms) {
                                if (lev.themePalette[ms].name == tName && materials[ms].available) {
                                    mappedIdx = static_cast<uint8_t>(ms);
                                    break;
                                }
                            }
                            if (mappedIdx == 0) {
                                if (customMaterials.empty()) {
                                    customMaterials = materials;
                                }
                                activeMaterials = &customMaterials;
                                const auto* def = ThemeManager::instance().getTheme(tName);
                                if (def) {
                                    ThemeMaterial mat{};
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
                                    if (customMaterials.size() < 250) {
                                        mappedIdx = static_cast<uint8_t>(customMaterials.size());
                                        customMaterials.push_back(mat);
                                    }
                                }
                            }
                        }
                        blend.indices[slot] = mappedIdx;
                    }
                }
                blend.strengths[0] = cell.blends[0];
                blend.strengths[1] = cell.blends[1];
                unsigned stored = unsigned(cell.blends[0]) + unsigned(cell.blends[1]);
                blend.strengths[2] = stored >= 255 ? 0 : (255 - stored);

                for (const auto& contribution : buildThemeContributions(blend, *activeMaterials)) {
                    Pass& pass = getPass(contribution.material, contribution.mappingDirection);
                    pass.blend[size_t(y) * 17 + x] = contribution.blend;
                    addPolys(pass, x, y);
                }
            }
        }
    }

    size_t basePass = 0;
    int maxCoverage = 0;
    for (size_t i = 0; i < passes.size(); ++i) {
        if (passes[i].polyCount > maxCoverage) {
            maxCoverage = passes[i].polyCount;
            basePass = i;
        }
    }

    if (!passes.empty()) {
        fgPatch.orderedPasses.push_back(passes[basePass]);
    }
    for (size_t i = 0; i < passes.size(); ++i) {
        if (i != basePass && passes[i].polyCount > 0) {
            fgPatch.orderedPasses.push_back(passes[i]);
        }
    }

    if (fgPatch.orderedPasses.empty()) {
        Pass pass;
        pass.direction = 0;
        pass.material.textures = {1, 1, 0};
        pass.blend.fill(255);
        pass.triangles.fill(true);
        pass.polyCount = 512;
        fgPatch.orderedPasses.push_back(pass);
    }

    for (auto& pass : fgPatch.orderedPasses) {
        uint32_t bgId = pass.material.textures[1];
        uint32_t fgId = pass.material.textures[0];
        uint32_t texId = (bgId != 0) ? bgId : fgId;
        if (texId != 0) {
            pass.decodedTexture = BankResolver::instance().getDecodedTexture(texId);
        }
        if (!pass.decodedTexture) {
            pass.decodedTexture = resolveFallbackTexture(pass.material, materials, lev);
        }
        if (!pass.decodedTexture) {
            pass.decodedTexture = BankResolver::instance().getDecodedTexture("LANDSCAPE_INVALID_THEME_TEXTURE");
        }
    }

    for (auto& pass : fgPatch.orderedPasses) {
        for (size_t v = 0; v < 17 * 17; ++v) {
            float dirA = getDirectionBlend(pass.direction, fgPatch.normals[v]);
            pass.dirAlpha[v] = dirA;
            pass.combinedAlpha[v] = (pass.blend[v] / 255.0f) * dirA;
        }
    }
}

struct MapFgPatchGrid {
    int countX = 0;
    int countY = 0;
    std::vector<FgPatchData> patches;
};

struct MapKey {
    int worldX = 0;
    int worldY = 0;
    int width = 0;
    int height = 0;
    uint32_t checksum = 0;

    bool operator==(const MapKey& o) const noexcept {
        return worldX == o.worldX && worldY == o.worldY &&
               width == o.width && height == o.height &&
               checksum == o.checksum;
    }
};

struct MapKeyHash {
    size_t operator()(const MapKey& k) const noexcept {
        size_t h = std::hash<uint32_t>{}(k.checksum);
        h ^= std::hash<int>{}(k.worldX) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.worldY) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.width) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.height) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

static std::mutex s_gridCacheMutex;
static std::unordered_map<MapKey, std::shared_ptr<MapFgPatchGrid>, MapKeyHash> s_gridCache;

static std::shared_ptr<MapFgPatchGrid> getOrBuildMapGrid(
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    const std::vector<WorldWaterNeighbor>* neighbors
) {
    MapKey key{worldX, worldY, lev.width, lev.height, lev.checksum};
    {
        std::lock_guard<std::mutex> lock(s_gridCacheMutex);
        auto it = s_gridCache.find(key);
        if (it != s_gridCache.end()) {
            return it->second;
        }
    }

    auto newGrid = std::make_shared<MapFgPatchGrid>();
    int countX = std::max(1, (lev.width + 15) / 16);
    int countY = std::max(1, (lev.height + 15) / 16);
    newGrid->countX = countX;
    newGrid->countY = countY;
    int totalPatches = countX * countY;
    newGrid->patches.resize(static_cast<size_t>(totalPatches));

    unsigned int numThreads = std::min(
        std::max(1u, std::thread::hardware_concurrency()),
        static_cast<unsigned int>(totalPatches)
    );
    if (numThreads <= 1 || totalPatches < 4) {
        for (int pIdx = 0; pIdx < totalPatches; ++pIdx) {
            int fgy = pIdx / countX;
            int fgx = pIdx % countX;
            initFgPatch(newGrid->patches[static_cast<size_t>(pIdx)],
                        fgx, fgy, lev, materials, worldX, worldY, neighbors);
        }
    } else {
        std::atomic<int> nextPatch{0};
        std::vector<std::thread> workers;
        workers.reserve(numThreads);
        for (unsigned int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&]() {
                while (true) {
                    int pIdx = nextPatch.fetch_add(1, std::memory_order_relaxed);
                    if (pIdx >= totalPatches) break;
                    int fgy = pIdx / countX;
                    int fgx = pIdx % countX;
                    initFgPatch(newGrid->patches[static_cast<size_t>(pIdx)],
                                fgx, fgy, lev, materials, worldX, worldY, neighbors);
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
    }

    std::lock_guard<std::mutex> lock(s_gridCacheMutex);
    auto [it, inserted] = s_gridCache.emplace(key, newGrid);
    return it->second;
}

static float invertUvLockAxis(const std::vector<float>* uvTable, int dim, float t) {
    if (!uvTable || static_cast<int>(uvTable->size()) != dim + 1 || dim <= 0) {
        return t * static_cast<float>(dim);
    }
    const auto& tbl = *uvTable;
    if (t <= tbl.front()) return 0.0f;
    if (t >= tbl.back()) return static_cast<float>(dim);
    auto it = std::upper_bound(tbl.begin() + 1, tbl.end(), t);
    int hi = static_cast<int>(it - tbl.begin());
    int lo = std::clamp(hi - 1, 0, dim - 1);
    float u0 = tbl[lo];
    float u1 = tbl[lo + 1];
    float denom = u1 - u0;
    float frac = (denom > 1e-7f) ? std::clamp((t - u0) / denom, 0.0f, 1.0f) : 0.5f;
    return static_cast<float>(lo) + frac;
}

struct ColorRGBF {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

static ColorRGBF sampleMipLevelBilinearF(
    const uint8_t* rgb, int w, int h, float uWrap, float vWrap
) {
    float tx = uWrap * static_cast<float>(w) - 0.5f;
    float ty = vWrap * static_cast<float>(h) - 0.5f;
    int x0 = static_cast<int>(std::floor(tx));
    int y0 = static_cast<int>(std::floor(ty));
    float fx = tx - static_cast<float>(x0);
    float fy = ty - static_cast<float>(y0);

    int x0_w, x1_w, y0_w, y1_w;
    if ((w & (w - 1)) == 0 && (h & (h - 1)) == 0 && w > 0 && h > 0) {
        int maskW = w - 1;
        int maskH = h - 1;
        x0_w = x0 & maskW;
        x1_w = (x0 + 1) & maskW;
        y0_w = y0 & maskH;
        y1_w = (y0 + 1) & maskH;
    } else {
        x0_w = ((x0 % w) + w) % w;
        x1_w = ((x0 + 1) % w + w) % w;
        y0_w = ((y0 % h) + h) % h;
        y1_w = ((y0 + 1) % h + h) % h;
    }

    const uint8_t* p00 = rgb + (static_cast<size_t>(y0_w) * w + x0_w) * 3;
    const uint8_t* p10 = rgb + (static_cast<size_t>(y0_w) * w + x1_w) * 3;
    const uint8_t* p01 = rgb + (static_cast<size_t>(y1_w) * w + x0_w) * 3;
    const uint8_t* p11 = rgb + (static_cast<size_t>(y1_w) * w + x1_w) * 3;

    float w00 = (1.0f - fx) * (1.0f - fy);
    float w10 = fx * (1.0f - fy);
    float w01 = (1.0f - fx) * fy;
    float w11 = fx * fy;

    return {
        p00[0] * w00 + p10[0] * w10 + p01[0] * w01 + p11[0] * w11,
        p00[1] * w00 + p10[1] * w10 + p01[1] * w01 + p11[1] * w11,
        p00[2] * w00 + p10[2] * w10 + p01[2] * w01 + p11[2] * w11
    };
}

static ColorRGBF sampleDecodedTextureTrilinearF(
    const DecodedTexture& dt,
    float u, float v,
    float dudx, float dvdx,
    float dudy, float dvdy
) {
    float uWrap = u - std::floor(u);
    float vWrap = v - std::floor(v);

    int maxMip = static_cast<int>(dt.mips.size());
    float sx1 = dudx * static_cast<float>(dt.width);
    float sy1 = dvdx * static_cast<float>(dt.height);
    float sx2 = dudy * static_cast<float>(dt.width);
    float sy2 = dvdy * static_cast<float>(dt.height);
    float rhoSq = std::max(sx1 * sx1 + sy1 * sy1, sx2 * sx2 + sy2 * sy2);
    float lod = (rhoSq > 1.0f) ? (0.5f * std::log2(rhoSq)) : 0.0f;

    // Vanilla RenderProceduralTexture (0x1249647) skips mip 0 when source FrameWidth > 256.
    float minLod = (dt.width > 256 || dt.height > 256) ? 1.0f : 0.0f;
    lod = std::clamp(lod, minLod, static_cast<float>(maxMip));

    int m0 = static_cast<int>(std::floor(lod));
    int m1 = std::min(m0 + 1, maxMip);
    float frac = lod - static_cast<float>(m0);

    auto getMip = [&](int level, int& outW, int& outH) -> const uint8_t* {
        if (level <= 0 || dt.mips.empty()) {
            outW = dt.width;
            outH = dt.height;
            return dt.rgb.data();
        }
        const auto& m = dt.mips[static_cast<size_t>(level - 1)];
        outW = m.width;
        outH = m.height;
        return m.rgb.data();
    };

    int w0 = 0, h0 = 0;
    const uint8_t* rgb0 = getMip(m0, w0, h0);
    ColorRGBF c0 = sampleMipLevelBilinearF(rgb0, w0, h0, uWrap, vWrap);
    if (frac <= 1e-3f || m1 == m0) {
        return c0;
    }

    int w1 = 0, h1 = 0;
    const uint8_t* rgb1 = getMip(m1, w1, h1);
    ColorRGBF c1 = sampleMipLevelBilinearF(rgb1, w1, h1, uWrap, vWrap);
    return {
        c0.r + (c1.r - c0.r) * frac,
        c0.g + (c1.g - c0.g) * frac,
        c0.b + (c1.b - c0.b) * frac
    };
}

} // anonymous namespace

void Dxt1Baker::prepareMapCache(
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    const std::vector<WorldWaterNeighbor>* neighbors
) {
    (void)getOrBuildMapGrid(lev, materials, worldX, worldY, neighbors);
}

void Dxt1Baker::clearCache() {
    std::lock_guard<std::mutex> lock(s_gridCacheMutex);
    s_gridCache.clear();
}

void Dxt1Baker::eraseMapCache(int worldX, int worldY, int width, int height, uint32_t checksum) {
    MapKey key{worldX, worldY, width, height, checksum};
    std::lock_guard<std::mutex> lock(s_gridCacheMutex);
    s_gridCache.erase(key);
}

uint16_t Dxt1Baker::toRGB565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

ColorRGB Dxt1Baker::fromRGB565(uint16_t c) {
    ColorRGB col;
    col.r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
    col.g = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);
    col.b = static_cast<uint8_t>((c & 0x1F) * 255 / 31);
    return col;
}

std::vector<uint8_t> Dxt1Baker::encodeDXT1(const ColorRGB* pixels, int width, int height, bool highQuality) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    size_t outSize = static_cast<size_t>(blocksX) * blocksY * 8;
    std::vector<uint8_t> out(outSize, 0);

    struct SolidDxtEntry {
        uint32_t rgba = 0;
        uint64_t block = 0;
        bool valid = false;
    };
    thread_local SolidDxtEntry s_solidDxtCache[256];

    const bool aligned4 = (width >= 4 && height >= 4 && (width & 3) == 0 && (height & 3) == 0);
    size_t outOffset = 0;
    for (int by = 0; by < blocksY; ++by) {
        const int y0 = by * 4;
        for (int bx = 0; bx < blocksX; ++bx) {
            const int x0 = bx * 4;
            if (highQuality) {
                unsigned char blockRgba[64];
                if (aligned4) {
                    for (int py = 0; py < 4; ++py) {
                        const ColorRGB* row = pixels + static_cast<size_t>(y0 + py) * width + x0;
                        unsigned char* dst = blockRgba + py * 16;
                        for (int px = 0; px < 4; ++px) {
                            dst[px * 4 + 0] = row[px].r;
                            dst[px * 4 + 1] = row[px].g;
                            dst[px * 4 + 2] = row[px].b;
                            dst[px * 4 + 3] = 255;
                        }
                    }
                } else {
                    for (int py = 0; py < 4; ++py) {
                        int y = std::min(y0 + py, height - 1);
                        for (int px = 0; px < 4; ++px) {
                            int x = std::min(x0 + px, width - 1);
                            ColorRGB col = pixels[y * width + x];
                            int idx = (py * 4 + px) * 4;
                            blockRgba[idx + 0] = col.r;
                            blockRgba[idx + 1] = col.g;
                            blockRgba[idx + 2] = col.b;
                            blockRgba[idx + 3] = 255;
                        }
                    }
                }

                uint32_t p32[16];
                std::memcpy(p32, blockRgba, 64);
                const uint32_t firstPx = p32[0];
                bool allSame = true;
                for (int k = 1; k < 16; ++k) {
                    if (p32[k] != firstPx) {
                        allSame = false;
                        break;
                    }
                }
                if (allSame) {
                    SolidDxtEntry& cached = s_solidDxtCache[(firstPx * 2654435761u) >> 24];
                    if (cached.valid && cached.rgba == firstPx) {
                        std::memcpy(out.data() + outOffset, &cached.block, 8);
                    } else {
                        stb_compress_dxt_block(out.data() + outOffset, blockRgba, 0, STB_DXT_HIGHQUAL);
                        cached.rgba = firstPx;
                        std::memcpy(&cached.block, out.data() + outOffset, 8);
                        cached.valid = true;
                    }
                } else {
                    stb_compress_dxt_block(out.data() + outOffset, blockRgba, 0, STB_DXT_HIGHQUAL);
                }
                outOffset += 8;
            } else {
                ColorRGB block[16];
                int rMin = 255, gMin = 255, bMin = 255;
                int rMax = 0, gMax = 0, bMax = 0;

                if (aligned4) {
                    for (int py = 0; py < 4; ++py) {
                        const ColorRGB* row = pixels + static_cast<size_t>(y0 + py) * width + x0;
                        ColorRGB* bDst = block + py * 4;
                        for (int px = 0; px < 4; ++px) {
                            const ColorRGB col = row[px];
                            bDst[px] = col;
                            if (col.r < rMin) rMin = col.r;
                            if (col.r > rMax) rMax = col.r;
                            if (col.g < gMin) gMin = col.g;
                            if (col.g > gMax) gMax = col.g;
                            if (col.b < bMin) bMin = col.b;
                            if (col.b > bMax) bMax = col.b;
                        }
                    }
                } else {
                    for (int py = 0; py < 4; ++py) {
                        int y = std::min(y0 + py, height - 1);
                        for (int px = 0; px < 4; ++px) {
                            int x = std::min(x0 + px, width - 1);
                            ColorRGB col = pixels[y * width + x];
                            block[py * 4 + px] = col;

                            if (col.r < rMin) rMin = col.r;
                            if (col.r > rMax) rMax = col.r;
                            if (col.g < gMin) gMin = col.g;
                            if (col.g > gMax) gMax = col.g;
                            if (col.b < bMin) bMin = col.b;
                            if (col.b > bMax) bMax = col.b;
                        }
                    }
                }

                uint16_t c0 = toRGB565(static_cast<uint8_t>(rMax), static_cast<uint8_t>(gMax), static_cast<uint8_t>(bMax));
                uint16_t c1 = toRGB565(static_cast<uint8_t>(rMin), static_cast<uint8_t>(gMin), static_cast<uint8_t>(bMin));

                if (c0 < c1) {
                    std::swap(c0, c1);
                } else if (c0 == c1) {
                    if (c0 < 0xFFFF) c0++;
                    else if (c1 > 0) c1--;
                }

                ColorRGB p0 = fromRGB565(c0);
                ColorRGB p1 = fromRGB565(c1);
                ColorRGB pal[4];
                pal[0] = p0;
                pal[1] = p1;
                pal[2] = {
                    static_cast<uint8_t>((2 * p0.r + p1.r) / 3),
                    static_cast<uint8_t>((2 * p0.g + p1.g) / 3),
                    static_cast<uint8_t>((2 * p0.b + p1.b) / 3)
                };
                pal[3] = {
                    static_cast<uint8_t>((p0.r + 2 * p1.r) / 3),
                    static_cast<uint8_t>((p0.g + 2 * p1.g) / 3),
                    static_cast<uint8_t>((p0.b + 2 * p1.b) / 3)
                };

                uint32_t indices = 0;
                for (int p = 0; p < 16; ++p) {
                    const auto& col = block[p];
                    int bestDist = 0x7fffffff;
                    int bestIdx = 0;
                    for (int k = 0; k < 4; ++k) {
                        int dr = int(col.r) - int(pal[k].r);
                        int dg = int(col.g) - int(pal[k].g);
                        int db = int(col.b) - int(pal[k].b);
                        int dist = dr * dr + dg * dg + db * db;
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestIdx = k;
                        }
                    }
                    indices |= (bestIdx << (2 * p));
                }

                std::memcpy(out.data() + outOffset + 0, &c0, 2);
                std::memcpy(out.data() + outOffset + 2, &c1, 2);
                std::memcpy(out.data() + outOffset + 4, &indices, 4);
                outOffset += 8;
            }
        }
    }

    return out;
}

ColorRGB Dxt1Baker::sampleDecodedTextureBilinear(const DecodedTexture& dt, float u, float v) {
    float uWrap = u - std::floor(u);
    float vWrap = v - std::floor(v);

    float tx = uWrap * dt.width - 0.5f;
    float ty = vWrap * dt.height - 0.5f;
    int x0 = static_cast<int>(std::floor(tx));
    int y0 = static_cast<int>(std::floor(ty));
    float fx = tx - x0;
    float fy = ty - y0;

    int w = dt.width;
    int h = dt.height;

    int x0_w, x1_w, y0_w, y1_w;
    if ((w & (w - 1)) == 0 && (h & (h - 1)) == 0 && w > 0 && h > 0) {
        int maskW = w - 1;
        int maskH = h - 1;
        x0_w = x0 & maskW;
        x1_w = (x0 + 1) & maskW;
        y0_w = y0 & maskH;
        y1_w = (y0 + 1) & maskH;
    } else {
        x0_w = ((x0 % w) + w) % w;
        x1_w = ((x0 + 1) % w + w) % w;
        y0_w = ((y0 % h) + h) % h;
        y1_w = ((y0 + 1) % h + h) % h;
    }

    const uint8_t* p00 = &dt.rgb[(static_cast<size_t>(y0_w) * w + x0_w) * 3];
    const uint8_t* p10 = &dt.rgb[(static_cast<size_t>(y0_w) * w + x1_w) * 3];
    const uint8_t* p01 = &dt.rgb[(static_cast<size_t>(y1_w) * w + x0_w) * 3];
    const uint8_t* p11 = &dt.rgb[(static_cast<size_t>(y1_w) * w + x1_w) * 3];

    float r0 = p00[0] * (1.0f - fx) + p10[0] * fx;
    float r1 = p01[0] * (1.0f - fx) + p11[0] * fx;
    float g0 = p00[1] * (1.0f - fx) + p10[1] * fx;
    float g1 = p01[1] * (1.0f - fx) + p11[1] * fx;
    float b0 = p00[2] * (1.0f - fx) + p10[2] * fx;
    float b1 = p01[2] * (1.0f - fx) + p11[2] * fx;

    return {
        static_cast<uint8_t>(std::clamp(static_cast<int>(r0 * (1.0f - fy) + r1 * fy + 0.5f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(g0 * (1.0f - fy) + g1 * fy + 0.5f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(b0 * (1.0f - fy) + b1 * fy + 0.5f), 0, 255))
    };
}

std::vector<uint8_t> Dxt1Baker::bakeTexture(
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int mapX, int mapY, int patchW, int patchH,
    int texWidth, int texHeight,
    int worldX, int worldY,
    bool generateMipmaps,
    bool highQualityDxt,
    const std::vector<WorldWaterNeighbor>* neighbors,
    const std::vector<float>* uvTableU,
    const std::vector<float>* uvTableV
) {
    if (texWidth <= 0) texWidth = 64;
    if (texHeight <= 0) texHeight = 64;

    std::shared_ptr<MapFgPatchGrid> grid = getOrBuildMapGrid(lev, materials, worldX, worldY, neighbors);

    int fgMinX = std::clamp(mapX >> 4, 0, grid->countX - 1);
    int fgMaxX = std::clamp(std::max(fgMinX, (mapX + patchW - 1) >> 4), 0, grid->countX - 1);
    int fgMinY = std::clamp(mapY >> 4, 0, grid->countY - 1);
    int fgMaxY = std::clamp(std::max(fgMinY, (mapY + patchH - 1) >> 4), 0, grid->countY - 1);

    // Precompute grid coordinates at texel centers (tx + 0.5) / texWidth using inverse UVLock curve
    static thread_local std::vector<float> gxOfTx, dgxOfTx, gyOfTy, dgyOfTy;
    gxOfTx.resize(texWidth);
    dgxOfTx.resize(texWidth);
    for (int tx = 0; tx < texWidth; ++tx) {
        float uPatch = (static_cast<float>(tx) + 0.5f) / static_cast<float>(texWidth);
        gxOfTx[tx] = static_cast<float>(mapX) + invertUvLockAxis(uvTableU, patchW, uPatch);
    }
    if (texWidth == 1) {
        dgxOfTx[0] = static_cast<float>(patchW);
    } else {
        for (int tx = 0; tx < texWidth; ++tx) {
            if (tx == 0) dgxOfTx[tx] = gxOfTx[1] - gxOfTx[0];
            else if (tx == texWidth - 1) dgxOfTx[tx] = gxOfTx[texWidth - 1] - gxOfTx[texWidth - 2];
            else dgxOfTx[tx] = 0.5f * (gxOfTx[tx + 1] - gxOfTx[tx - 1]);
        }
    }

    gyOfTy.resize(texHeight);
    dgyOfTy.resize(texHeight);
    for (int ty = 0; ty < texHeight; ++ty) {
        float vPatch = (static_cast<float>(ty) + 0.5f) / static_cast<float>(texHeight);
        gyOfTy[ty] = static_cast<float>(mapY) + invertUvLockAxis(uvTableV, patchH, vPatch);
    }
    if (texHeight == 1) {
        dgyOfTy[0] = static_cast<float>(patchH);
    } else {
        for (int ty = 0; ty < texHeight; ++ty) {
            if (ty == 0) dgyOfTy[ty] = gyOfTy[1] - gyOfTy[0];
            else if (ty == texHeight - 1) dgyOfTy[ty] = gyOfTy[texHeight - 1] - gyOfTy[texHeight - 2];
            else dgyOfTy[ty] = 0.5f * (gyOfTy[ty + 1] - gyOfTy[ty - 1]);
        }
    }

    struct PixelGeo {
        float cgx = 0.0f;
        float cgy = 0.0f;
        float w0 = 0.0f;
        float w1 = 0.0f;
        float w2 = 0.0f;
        uint16_t patchIndex = 0;
        uint8_t lx = 0;
        uint8_t ly = 0;
        uint8_t tri = 0;
    };

    const size_t numPixels = static_cast<size_t>(texWidth) * texHeight;
    static thread_local std::vector<PixelGeo> geos;
    static thread_local std::vector<float> hBuf;
    static thread_local std::vector<ColorRGB> pixels;
    geos.resize(numPixels);
    hBuf.resize(numPixels);

    for (int ty = 0; ty < texHeight; ++ty) {
        float cgy = std::clamp(gyOfTy[ty], 0.0f, float(lev.height));

        int iy = static_cast<int>(std::floor(cgy));
        float fy = 0.0f;
        if (iy >= lev.height - 1) {
            iy = std::max(0, lev.height - 1);
            fy = (cgy >= float(lev.height)) ? 1.0f : (cgy - iy);
        } else {
            fy = cgy - iy;
        }

        int fgY = std::clamp(iy >> 4, fgMinY, fgMaxY);

        for (int tx = 0; tx < texWidth; ++tx) {
            float cgx = std::clamp(gxOfTx[tx], 0.0f, float(lev.width));

            int ix = static_cast<int>(std::floor(cgx));
            float fx = 0.0f;
            if (ix >= lev.width - 1) {
                ix = std::max(0, lev.width - 1);
                fx = (cgx >= float(lev.width)) ? 1.0f : (cgx - ix);
            } else {
                fx = cgx - ix;
            }

            int fgX = std::clamp(ix >> 4, fgMinX, fgMaxX);
            size_t patchIndex = static_cast<size_t>(fgY) * grid->countX + fgX;
            const auto& patch = grid->patches[patchIndex];

            int lx = std::clamp(ix - patch.startX, 0, 15);
            int ly = std::clamp(iy - patch.startY, 0, 15);

            int odd = (lx ^ ly) & 1;
            int tri = 0;
            float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f;
            if (odd == 0) {
                if (fx >= fy) {
                    tri = 0;
                    w2 = fy;
                    w1 = fx - fy;
                    w0 = 1.0f - fx;
                } else {
                    tri = 1;
                    w2 = fx;
                    w1 = fy - fx;
                    w0 = 1.0f - fy;
                }
            } else {
                if (fx + fy <= 1.0f) {
                    tri = 0;
                    w1 = fx;
                    w2 = fy;
                    w0 = 1.0f - fx - fy;
                } else {
                    tri = 1;
                    w2 = fx + fy - 1.0f;
                    w0 = 1.0f - fy;
                    w1 = 1.0f - fx;
                }
            }

            int c0x = s_kOffsets[odd][tri][0][0];
            int c0y = s_kOffsets[odd][tri][0][1];
            int c1x = s_kOffsets[odd][tri][1][0];
            int c1y = s_kOffsets[odd][tri][1][1];
            int c2x = s_kOffsets[odd][tri][2][0];
            int c2y = s_kOffsets[odd][tri][2][1];

            size_t v0Idx = static_cast<size_t>(ly + c0y) * 17 + (lx + c0x);
            size_t v1Idx = static_cast<size_t>(ly + c1y) * 17 + (lx + c1x);
            size_t v2Idx = static_cast<size_t>(ly + c2y) * 17 + (lx + c2x);

            size_t pIdx = static_cast<size_t>(ty) * texWidth + tx;
            hBuf[pIdx] = w0 * patch.heights[v0Idx] + w1 * patch.heights[v1Idx] + w2 * patch.heights[v2Idx];
            geos[pIdx] = {
                cgx, cgy, w0, w1, w2,
                static_cast<uint16_t>(patchIndex),
                static_cast<uint8_t>(lx),
                static_cast<uint8_t>(ly),
                static_cast<uint8_t>(tri)
            };
        }
    }

    pixels.resize(numPixels);

    for (int ty = 0; ty < texHeight; ++ty) {
        const float dgy = dgyOfTy[ty];
        int ym1 = std::max(0, ty - 1);
        int yp1 = std::min(texHeight - 1, ty + 1);
        float ySpan = static_cast<float>(std::max(1, yp1 - ym1));

        for (int tx = 0; tx < texWidth; ++tx) {
            const float dgx = dgxOfTx[tx];
            size_t pIdx = static_cast<size_t>(ty) * texWidth + tx;
            const auto& g = geos[pIdx];
            const auto& patch = grid->patches[g.patchIndex];
            const float h = hBuf[pIdx];

            int odd = (g.lx ^ g.ly) & 1;
            int tri = g.tri;
            int c0x = s_kOffsets[odd][tri][0][0];
            int c0y = s_kOffsets[odd][tri][0][1];
            int c1x = s_kOffsets[odd][tri][1][0];
            int c1y = s_kOffsets[odd][tri][1][1];
            int c2x = s_kOffsets[odd][tri][2][0];
            int c2y = s_kOffsets[odd][tri][2][1];

            size_t v0Idx = static_cast<size_t>(g.ly + c0y) * 17 + (g.lx + c0x);
            size_t v1Idx = static_cast<size_t>(g.ly + c1y) * 17 + (g.lx + c1x);
            size_t v2Idx = static_cast<size_t>(g.ly + c2y) * 17 + (g.lx + c2x);
            size_t triIdx = static_cast<size_t>((int(g.lx) * 16 + int(g.ly)) * 2 + tri);

            float dhdx = 0.0f;
            float dhdy = 0.0f;
            bool hasDh = false;

            float curR = 0.0f, curG = 0.0f, curB = 0.0f;
            bool hasDrawn = false;

            for (size_t p = 0; p < patch.orderedPasses.size(); ++p) {
                const auto& pass = patch.orderedPasses[p];
                if (!pass.triangles[triIdx]) continue;

                float alpha = 0.0f;
                if (p == 0 || !hasDrawn) {
                    alpha = g.w0 * pass.dirAlpha[v0Idx] + g.w1 * pass.dirAlpha[v1Idx] + g.w2 * pass.dirAlpha[v2Idx];
                } else {
                    alpha = g.w0 * pass.combinedAlpha[v0Idx] + g.w1 * pass.combinedAlpha[v1Idx] + g.w2 * pass.combinedAlpha[v2Idx];
                }
                if (alpha <= 0.0f) continue;

                float uTex = 0.0f, vTex = 0.0f;
                float dudx = 0.0f, dvdx = 0.0f, dudy = 0.0f, dvdy = 0.0f;
                if (pass.direction == 0) {
                    uTex = g.cgx * 0.015625f;
                    vTex = g.cgy * 0.015625f;
                    dudx = dgx * 0.015625f;
                    dvdy = dgy * 0.015625f;
                } else {
                    if (!hasDh) {
                        int xm1 = std::max(0, tx - 1);
                        int xp1 = std::min(texWidth - 1, tx + 1);
                        float xSpan = static_cast<float>(std::max(1, xp1 - xm1));
                        dhdx = (hBuf[static_cast<size_t>(ty) * texWidth + xp1] -
                                hBuf[static_cast<size_t>(ty) * texWidth + xm1]) / xSpan;
                        dhdy = (hBuf[static_cast<size_t>(yp1) * texWidth + tx] -
                                hBuf[static_cast<size_t>(ym1) * texWidth + tx]) / ySpan;
                        hasDh = true;
                    }
                    if (pass.direction == 1 || pass.direction == 2) {
                        uTex = g.cgx * 0.015625f;
                        vTex = -h * 0.015625f;
                        dudx = dgx * 0.015625f;
                        dvdx = -dhdx * 0.015625f;
                        dvdy = -dhdy * 0.015625f;
                    } else {
                        uTex = g.cgy * 0.015625f;
                        vTex = -h * 0.015625f;
                        dudy = dgy * 0.015625f;
                        dvdx = -dhdx * 0.015625f;
                        dvdy = -dhdy * 0.015625f;
                    }
                }

                ColorRGBF texColor{0.0f, 0.0f, 0.0f};
                if (pass.decodedTexture && pass.decodedTexture->width > 0 && pass.decodedTexture->height > 0 && !pass.decodedTexture->rgb.empty()) {
                    texColor = sampleDecodedTextureTrilinearF(
                        *pass.decodedTexture, uTex, vTex, dudx, dvdx, dudy, dvdy);
                }

                if (p == 0 || !hasDrawn) {
                    curR = texColor.r;
                    curG = texColor.g;
                    curB = texColor.b;
                    hasDrawn = true;
                } else {
                    curR = curR * (1.0f - alpha) + texColor.r * alpha;
                    curG = curG * (1.0f - alpha) + texColor.g * alpha;
                    curB = curB * (1.0f - alpha) + texColor.b * alpha;
                }
            }

            pixels[pIdx] = {
                static_cast<uint8_t>(std::clamp(static_cast<int>(curR + 0.5f), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(curG + 0.5f), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(curB + 0.5f), 0, 255))
            };
        }
    }

    std::vector<uint8_t> dxt1Blocks;
    int numMips = 1;

    if (generateMipmaps && texWidth >= 8 && texHeight >= 8) {
        int curW = texWidth;
        int curH = texHeight;
        std::vector<ColorRGB> curPixels = pixels;

        while (curW >= 4 && curH >= 4) {
            auto blocks = encodeDXT1(curPixels.data(), curW, curH, highQualityDxt);
            dxt1Blocks.insert(dxt1Blocks.end(), blocks.begin(), blocks.end());

            if (curW > 4 && curH > 4) {
                int nextW = curW / 2;
                int nextH = curH / 2;
                std::vector<ColorRGB> nextPixels(nextW * nextH);
                for (int ny = 0; ny < nextH; ++ny) {
                    for (int nx = 0; nx < nextW; ++nx) {
                        const auto& p00 = curPixels[(ny * 2) * curW + (nx * 2)];
                        const auto& p10 = curPixels[(ny * 2) * curW + (nx * 2 + 1)];
                        const auto& p01 = curPixels[(ny * 2 + 1) * curW + (nx * 2)];
                        const auto& p11 = curPixels[(ny * 2 + 1) * curW + (nx * 2 + 1)];
                        nextPixels[ny * nextW + nx] = {
                            static_cast<uint8_t>((int(p00.r) + int(p10.r) + int(p01.r) + int(p11.r)) / 4),
                            static_cast<uint8_t>((int(p00.g) + int(p10.g) + int(p01.g) + int(p11.g)) / 4),
                            static_cast<uint8_t>((int(p00.b) + int(p10.b) + int(p01.b) + int(p11.b)) / 4)
                        };
                    }
                }
                curPixels = std::move(nextPixels);
                curW = nextW;
                curH = nextH;
                numMips++;
            } else {
                break;
            }
        }
    } else {
        dxt1Blocks = encodeDXT1(pixels.data(), texWidth, texHeight, highQualityDxt);
    }

    std::vector<uint8_t> out;
    out.reserve(19 + dxt1Blocks.size());

    uint16_t w16 = static_cast<uint16_t>(texWidth);
    uint16_t h16 = static_cast<uint16_t>(texHeight);
    uint8_t wBytes[2]; std::memcpy(wBytes, &w16, 2);
    uint8_t hBytes[2]; std::memcpy(hBytes, &h16, 2);

    out.insert(out.end(), wBytes, wBytes + 2);
    out.insert(out.end(), hBytes, hBytes + 2);
    out.push_back(static_cast<uint8_t>(numMips)); // MipLevels

    static const uint8_t formatTrailer[14] = {
        0x23, 0x04, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00
    };
    out.insert(out.end(), formatTrailer, formatTrailer + 14);

    out.insert(out.end(), dxt1Blocks.begin(), dxt1Blocks.end());

    return out;
}

} // namespace egostb
