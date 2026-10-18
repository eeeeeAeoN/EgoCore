#include "egostb/background_tree.hpp"
#include "egostb/range_compressor.hpp"
#include "egostb/dxt1_baker.hpp"
#include "egostb/lzo1x.hpp"
#include "egostb/normal_lut.hpp"
#include "egostb/stripifier.hpp"
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <array>
#include <map>
#include <set>
#include <functional>
#include <tuple>
#include <thread>
#include <atomic>

namespace egostb {

static size_t alignUp(size_t val, size_t alignment) {
    return (val + alignment - 1) & ~(alignment - 1);
}

static constexpr int s_kOffsets[2][2][3][2] = {
    {{{0, 0}, {1, 0}, {1, 1}}, {{0, 0}, {0, 1}, {1, 1}}},
    {{{0, 0}, {1, 0}, {0, 1}}, {{1, 0}, {0, 1}, {1, 1}}},
};

static std::vector<uint16_t> buildGridTriangleListIndices(int nx, int ny) {
    std::vector<uint16_t> indices;
    indices.reserve(nx * ny * 6);
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix < nx; ++ix) {
            uint16_t v00 = static_cast<uint16_t>(iy * (nx + 1) + ix);
            uint16_t v10 = static_cast<uint16_t>(iy * (nx + 1) + (ix + 1));
            uint16_t v01 = static_cast<uint16_t>((iy + 1) * (nx + 1) + ix);
            uint16_t v11 = static_cast<uint16_t>((iy + 1) * (nx + 1) + (ix + 1));
            if (((ix ^ iy) & 1) != 0) {
                // Tri 1: (0,0), (0,1), (1,0)
                indices.push_back(v00);
                indices.push_back(v01);
                indices.push_back(v10);
                // Tri 2: (1,1), (1,0), (0,1)
                indices.push_back(v11);
                indices.push_back(v10);
                indices.push_back(v01);
            } else {
                // Tri 1: (0,0), (0,1), (1,1)
                indices.push_back(v00);
                indices.push_back(v01);
                indices.push_back(v11);
                // Tri 2: (0,0), (1,1), (1,0)
                indices.push_back(v00);
                indices.push_back(v11);
                indices.push_back(v10);
            }
        }
    }
    return indices;
}

size_t BgTreeNode::headerSize() const {
    size_t count = (firstNonSplitBand < 8 && lastBand >= firstNonSplitBand)
                       ? size_t(lastBand - firstNonSplitBand + 1)
                       : 0;
    return 47 + count * 13;
}

void BgTreeNode::serializeHeader(std::vector<uint8_t>& out) const {
    size_t start = out.size();
    out.resize(start + 47);
    uint8_t* p = out.data() + start;

    std::memcpy(p + 0, &mapX, 2);
    std::memcpy(p + 2, &mapY, 2);
    std::memcpy(p + 4, &width, 2);
    std::memcpy(p + 6, &height, 2);
    p[8] = firstBand;
    p[9] = firstNonSplitBand;
    p[10] = lastBand;
    std::memcpy(p + 11, &fileBlockPos, 4);
    std::memcpy(p + 15, &fileBlockSize, 4);
    std::memcpy(p + 19, &offsetIntoFileBlock, 4);

    float bounds[6] = {aabb.minX, aabb.minY, aabb.minZ, aabb.maxX, aabb.maxY, aabb.maxZ};
    for (int i = 0; i < 6; ++i) {
        std::memcpy(p + 23 + i * 4, &bounds[i], 4);
    }

    size_t count = (firstNonSplitBand < 8 && lastBand >= firstNonSplitBand)
                       ? size_t(lastBand - firstNonSplitBand + 1)
                       : 0;

    for (size_t i = 0; i < count; ++i) {
        size_t lodStart = out.size();
        out.resize(lodStart + 13);
        uint8_t* lp = out.data() + lodStart;

        const BgLodInfo* info = (i < lodInfo.size()) ? &lodInfo[i] : nullptr;
        uint8_t opt = info ? info->optimizedBandRemap : 0;
        int32_t fpos = info ? info->fileBlockPos : 0;
        int32_t fsize = info ? info->fileBlockSize : 0;
        int32_t foff = info ? info->offsetIntoFileBlock : 0;

        lp[0] = opt;
        std::memcpy(lp + 1, &fpos, 4);
        std::memcpy(lp + 5, &fsize, 4);
        std::memcpy(lp + 9, &foff, 4);
    }
}

struct BgVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 1.0f;
};

// Exact PeekMapNormal matching CEngineMap::PeekMapNormal (0x125db90)
inline BgVec3 peekMapNormal(const LevData& lev, int gx, int gy) {
    int cx = std::clamp(gx, 0, lev.width - 1);  // 0..127
    int cy = std::clamp(gy, 0, lev.height - 1); // 0..127
    float h00 = lev.getCell(cx, cy).height;
    float h10 = lev.getCell(cx + 1, cy).height;
    float h01 = lev.getCell(cx, cy + 1).height;
    float h11 = lev.getCell(cx + 1, cy + 1).height;
    float nx = h00 - h11;
    float ny = h10 - h01;
    float inv = fastInvSqrtEngine(nx * nx + ny * ny + 2.0f);
    float rot = inv * 0.7071067690849304f;
    return BgVec3{(ny + nx) * rot, (ny - nx) * rot, inv * 1.414213538169861f};
}

// Exact normal packing matching CLandscapeBackgroundPatch::BuildVertex (0x1246a70 / 0x1246c74 / 0x1246980)
static uint32_t calculatePackedNormal(const LevData& lev, int gx, int gy) {
    BgVec3 n = peekMapNormal(lev, gx, gy);
    int ix = static_cast<int>(std::nearbyint(n.x * 1023.0f));
    int iy = static_cast<int>(std::nearbyint(n.y * 1023.0f));
    int iz = static_cast<int>(std::nearbyint(n.z * 511.0f));
    return (static_cast<uint32_t>(ix) & 0x7FFu) |
           ((static_cast<uint32_t>(iy) & 0x7FFu) << 11) |
           ((static_cast<uint32_t>(iz) & 0x3FFu) << 22);
}

BackgroundTreeBuilder::TopologyGrid BackgroundTreeBuilder::buildTopologyGrid(
    const LevData& lev, int mapX, int mapY, int width, int height
) {
    TopologyGrid grid;
    grid.mapX = mapX;
    grid.mapY = mapY;
    grid.width = width;
    grid.height = height;
    grid.unitSizeHor.resize(width * height);
    grid.unitSizeVert.resize(width * height);

    auto getHeight = [&](int x, int y) -> float {
        int cx = std::clamp(x, 0, lev.gridWidth - 1);
        int cy = std::clamp(y, 0, lev.gridHeight - 1);
        return lev.getCell(cx, cy).height;
    };

    for (int y = mapY; y < mapY + height; ++y) {
        for (int x = mapX; x < mapX + width; ++x) {
            float hA = getHeight(x, y);
            float hB = getHeight(x + 1, y);
            float hC = getHeight(x, y + 1);
            float hD = getHeight(x + 1, y + 1);

            float distAB = std::sqrt(1.0f + (hA - hB) * (hA - hB));
            float distCD = std::sqrt(1.0f + (hC - hD) * (hC - hD));
            grid.unitSizeHor[(y - mapY) * width + (x - mapX)] = std::max(distAB, distCD);

            float distAC = std::sqrt(1.0f + (hA - hC) * (hA - hC));
            float distBD = std::sqrt(1.0f + (hB - hD) * (hB - hD));
            grid.unitSizeVert[(y - mapY) * width + (x - mapX)] = std::max(distAC, distBD);
        }
    }
    return grid;
}

static float getNodeCurvature(
    const BackgroundTreeBuilder::TopologyGrid& grid,
    int mapX, int mapY, int width, int height
) {
    float sumH = 0.0f;
    for (int x = 0; x < width; ++x) {
        float maxH = 0.0f;
        for (int y = 0; y < height; ++y) {
            float val = grid.unitSizeHor[(mapY + y) * grid.width + (mapX + x)];
            if (val > maxH) maxH = val;
        }
        sumH += maxH;
    }
    float sumV = 0.0f;
    for (int y = 0; y < height; ++y) {
        float maxV = 0.0f;
        for (int x = 0; x < width; ++x) {
            float val = grid.unitSizeVert[(mapY + y) * grid.width + (mapX + x)];
            if (val > maxV) maxV = val;
        }
        sumV += maxV;
    }
    return std::max(sumH, sumV);
}

static inline bool isPowerOf2(int v) {
    return (v > 0) && ((v & (v - 1)) == 0);
}

static inline int largestPowerOf2LessThan(int v) {
    if (v <= 2) return 1;
    int x = v - 1;
    int p = 1;
    while ((p << 1) <= x) {
        p <<= 1;
    }
    return p;
}

static inline float getLodBandZ(int band, float scale = 1.0f) {
    static constexpr float s_kBaseLODZBandsZ[8] = {
        0.0f, 0.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f
    };
    if (band < 0) return 0.0f;
    if (band >= 8) return s_kBaseLODZBandsZ[7] * scale;
    return s_kBaseLODZBandsZ[band] * scale;
}

static float peekInterpolatedWaterHeight(const LevData& lev, int mapx, int mapy, int distance_to_look, bool* outFound = nullptr);

std::unique_ptr<BgTreeNode> BackgroundTreeBuilder::buildTree(
    const TopologyGrid& grid,
    const LevData& lev,
    int mapX, int mapY, int width, int height,
    int maxLodBand, float textureDetail, int maxPatchSize,
    const std::vector<ObservingMapBounds>* observingMaps,
    int worldX, int worldY,
    float lodDistanceScale
) {
    auto node = std::make_unique<BgTreeNode>();
    node->mapX = static_cast<uint16_t>(mapX);
    node->mapY = static_cast<uint16_t>(mapY);
    node->width = static_cast<uint16_t>(width);
    node->height = static_cast<uint16_t>(height);
    node->firstBand = 1;
    node->lastBand = static_cast<uint8_t>(std::min(maxLodBand, 7));

    node->aabb.minX = static_cast<float>(worldX + mapX);
    node->aabb.minY = static_cast<float>(worldY + mapY);
    node->aabb.maxX = static_cast<float>(worldX + mapX + width);
    node->aabb.maxY = static_cast<float>(worldY + mapY + height);
    float minZ = 1e30f;
    float maxZ = -1e30f;
    for (int y = mapY; y <= mapY + height; ++y) {
        for (int x = mapX; x <= mapX + width; ++x) {
            int cx = std::clamp(x, 0, lev.gridWidth - 1);
            int cy = std::clamp(y, 0, lev.gridHeight - 1);
            float h = lev.getCell(cx, cy).height;
            if (h < minZ) minZ = h;
            if (h > maxZ) maxZ = h;
            bool waterFound = false;
            float wh = peekInterpolatedWaterHeight(lev, x, y, 1, &waterFound);
            if (waterFound) {
                if (wh < minZ) minZ = wh;
                if (wh > maxZ) maxZ = wh;
            }
        }
    }
    node->aabb.minZ = minZ;
    node->aabb.maxZ = maxZ;

    float maxDist = 1.0e30f;
    if (observingMaps) {
        float minDist = findShortestDistanceToObservingMapSet(
            node->aabb.minX, node->aabb.minY, node->aabb.minZ,
            node->aabb.maxX, node->aabb.maxY, node->aabb.maxZ,
            *observingMaps
        );
        maxDist = findFurthestDistanceToObservingMapSet(
            node->aabb.minX, node->aabb.minY, node->aabb.minZ,
            node->aabb.maxX, node->aabb.maxY, node->aabb.maxZ,
            *observingMaps
        );
        while (node->firstBand < node->lastBand - 1) {
            if (getLodBandZ(node->firstBand + 1, lodDistanceScale) >= minDist) {
                break;
            }
            node->firstBand++;
        }
    }

    int maxDim = std::max(width, height);
    float maxC = getNodeCurvature(grid, mapX, mapY, width, height);

    bool isLeafNode = false;
    if (width <= 8 && height <= 8) {
        isLeafNode = true;
    } else if (width == 16 && height == 8) {
        isLeafNode = true;
    } else if (width == 8 && height == 16) {
        isLeafNode = (maxC < 34.0f);
    } else if (width <= 16 && height <= 16) {
        if (maxC < 34.0f) {
            isLeafNode = true;
        }
    }

    uint8_t naturalFirstNonSplitBand = 1;
    if (node->firstBand >= 6) {
        naturalFirstNonSplitBand = (maxDim <= 128) ? node->firstBand : 7;
    } else if (isLeafNode) {
        naturalFirstNonSplitBand = 1;
    } else if (maxDim <= 16) {
        naturalFirstNonSplitBand = (maxC >= 50.0f) ? 3 : 2;
    } else if (maxDim <= 32) {
        if (maxC > 70.0f) {
            naturalFirstNonSplitBand = (width == 32 && height == 32) ? 7 : 3;
        } else if (width == 32 && height == 32) {
            naturalFirstNonSplitBand = (maxC > 34.0f) ? 3 : 2;
        } else {
            naturalFirstNonSplitBand = (maxC > 47.5f) ? 3 : 2;
        }
    } else if (maxDim <= 64) {
        naturalFirstNonSplitBand = (maxC > 64.0f) ? 7 : 3;
    } else {
        naturalFirstNonSplitBand = 7;
    }

    if (isLeafNode || naturalFirstNonSplitBand <= node->firstBand) {
        node->firstNonSplitBand = node->firstBand;
        if (observingMaps) {
            for (int band = node->lastBand; band >= node->firstBand; --band) {
                if (band == node->lastBand || band == node->firstBand || maxDist > getLodBandZ(band, lodDistanceScale)) {
                    node->firstNonSplitBand = static_cast<uint8_t>(band);
                }
            }
        }
        return node;
    }

    node->firstNonSplitBand = naturalFirstNonSplitBand;

    // Determine split direction
    bool splitH = false; // true = Width / 2, false = Height / 2
    if (width > 8 && height <= 8) {
        splitH = true;
    } else if (width <= 8 && height > 8) {
        splitH = false;
    } else if (width > 16 && height <= 16) {
        splitH = true;
    } else if (width <= 16 && height > 16) {
        splitH = false;
    } else {
        std::vector<float> strip_unit_hor(width, 0.0f);
        std::vector<float> strip_unit_ver(height, 0.0f);
        for (int x = 0; x < width; ++x) {
            float maxH = 0.0f;
            for (int y = 0; y < height; ++y) {
                float val = grid.unitSizeHor[(mapY + y) * grid.width + (mapX + x)];
                if (val > maxH) maxH = val;
            }
            strip_unit_hor[x] = maxH;
        }
        for (int y = 0; y < height; ++y) {
            float maxV = 0.0f;
            for (int x = 0; x < width; ++x) {
                float val = grid.unitSizeVert[(mapY + y) * grid.width + (mapX + x)];
                if (val > maxV) maxV = val;
            }
            strip_unit_ver[y] = maxV;
        }

        float sumH = 0.0f;
        for (float v : strip_unit_hor) sumH += v;
        float sumV = 0.0f;
        for (float v : strip_unit_ver) sumV += v;

        if (width == 16 && height == 16 && sumH <= 40.0f && sumV > 40.0f) {
            splitH = false;
        } else if (width == 16 && height == 16 && sumV <= 40.0f && sumH > 40.0f) {
            splitH = true;
        } else {
            int sub_step = std::max(1, (16 * width) / 128);

            float u_needed = 0.0f;
            for (int x = 0; x < width; x += sub_step) {
                float maxH = 0.0f;
                int endX = std::min(width, x + sub_step);
                for (int i = x; i < endX; ++i) {
                    if (strip_unit_hor[i] > maxH) maxH = strip_unit_hor[i];
                }
                u_needed += maxH * sub_step;
            }

            float v_needed = 0.0f;
            for (int y = 0; y < height; y += sub_step) {
                float maxV = 0.0f;
                int endY = std::min(height, y + sub_step);
                for (int j = y; j < endY; ++j) {
                    if (strip_unit_ver[j] > maxV) maxV = strip_unit_ver[j];
                }
                v_needed += maxV * sub_step;
            }

            splitH = (v_needed < u_needed);
        }
    }

    if (splitH) {
        int halfW = width / 2;
        node->leftChild = buildTree(grid, lev, mapX, mapY, halfW, height, node->firstNonSplitBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY, lodDistanceScale);
        node->rightChild = buildTree(grid, lev, mapX + halfW, mapY, halfW, height, node->firstNonSplitBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY, lodDistanceScale);
    } else {
        int halfH = height / 2;
        node->leftChild = buildTree(grid, lev, mapX, mapY, width, halfH, node->firstNonSplitBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY, lodDistanceScale);
        node->rightChild = buildTree(grid, lev, mapX, mapY + halfH, width, halfH, node->firstNonSplitBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY, lodDistanceScale);
    }

    return node;
}

std::unique_ptr<BgTreeNode> BackgroundTreeBuilder::buildTreeSubdivideIntoPowersOf2(
    const TopologyGrid& grid,
    const LevData& lev,
    int mapX, int mapY, int width, int height,
    int maxLodBand, float textureDetail, int maxPatchSize,
    const std::vector<ObservingMapBounds>* observingMaps,
    int worldX, int worldY,
    float lodDistanceScale
) {
    bool splitX = (width > 2) && (width > maxPatchSize || !isPowerOf2(width));
    bool splitY = (height > 2) && (height > maxPatchSize || !isPowerOf2(height));

    if (!splitX && !splitY) {
        return buildTree(grid, lev, mapX, mapY, width, height, maxLodBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY, lodDistanceScale);
    }

    auto node = std::make_unique<BgTreeNode>();
    node->mapX = static_cast<uint16_t>(mapX);
    node->mapY = static_cast<uint16_t>(mapY);
    node->width = static_cast<uint16_t>(width);
    node->height = static_cast<uint16_t>(height);
    node->firstBand = 8;
    node->firstNonSplitBand = 8;
    node->lastBand = 8;

    if (splitX && (!splitY || width >= height)) {
        int leftW = largestPowerOf2LessThan(width);
        node->leftChild = buildTreeSubdivideIntoPowersOf2(
            grid, lev, mapX, mapY, leftW, height,
            maxLodBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY,
            lodDistanceScale
        );
        node->rightChild = buildTreeSubdivideIntoPowersOf2(
            grid, lev, mapX + leftW, mapY, width - leftW, height,
            maxLodBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY,
            lodDistanceScale
        );
    } else if (splitY) {
        int topH = largestPowerOf2LessThan(height);
        node->leftChild = buildTreeSubdivideIntoPowersOf2(
            grid, lev, mapX, mapY, width, topH,
            maxLodBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY,
            lodDistanceScale
        );
        node->rightChild = buildTreeSubdivideIntoPowersOf2(
            grid, lev, mapX, mapY + topH, width, height - topH,
            maxLodBand, textureDetail, maxPatchSize, observingMaps, worldX, worldY,
            lodDistanceScale
        );
    }

    return node;
}

std::unique_ptr<BgTreeNode> BackgroundTreeBuilder::buildHierarchy(
    const TopologyGrid& grid,
    int mapX, int mapY, int width, int height,
    int minBand, int maxBand
) {
    auto node = std::make_unique<BgTreeNode>();
    node->mapX = static_cast<uint16_t>(mapX);
    node->mapY = static_cast<uint16_t>(mapY);
    node->width = static_cast<uint16_t>(width);
    node->height = static_cast<uint16_t>(height);
    node->firstBand = 1;

    int maxDim = std::max(width, height);
    float maxC = getNodeCurvature(grid, mapX, mapY, width, height);

    // Leaf condition: minimum size 8x8 or curvature permits Band 1
    bool isLeafNode = false;
    if (width <= 8 && height <= 8) {
        isLeafNode = true;
    } else if (width == 16 && height == 8) {
        isLeafNode = true; // In retail Fable, 16x8 nodes are always leaves
    } else if (width == 8 && height == 16) {
        isLeafNode = (maxC < 34.0f);
    } else if (width <= 16 && height <= 16) {
        if (maxC < 34.0f) {
            isLeafNode = true;
        }
    }

    if (isLeafNode) {
        node->firstNonSplitBand = 1;
        node->lastBand = static_cast<uint8_t>(maxBand);
        return node;
    }

    // Determine band range matching retail:
    // Every node inherits lastBand directly from parent's firstNonSplitBand (maxBand)
    node->lastBand = static_cast<uint8_t>(maxBand);

    if (maxDim <= 16) {
        node->firstNonSplitBand = (maxC >= 50.0f) ? 3 : 2;
    } else if (maxDim <= 32) {
        if (maxC > 70.0f) {
            node->firstNonSplitBand = (width == 32 && height == 32) ? 7 : 3;
        } else if (width == 32 && height == 32) {
            node->firstNonSplitBand = (maxC > 34.0f) ? 3 : 2;
        } else {
            node->firstNonSplitBand = (maxC > 47.5f) ? 3 : 2;
        }
    } else if (maxDim <= 64) {
        node->firstNonSplitBand = (maxC > 64.0f) ? 7 : 3;
    } else {
        node->firstNonSplitBand = 7;
    }

    // Determine split direction
    bool splitH = false; // true = Width / 2, false = Height / 2
    if (width > 8 && height <= 8) {
        splitH = true;
    } else if (width <= 8 && height > 8) {
        splitH = false;
    } else if (width > 16 && height <= 16) {
        splitH = true;
    } else if (width <= 16 && height > 16) {
        splitH = false;
    } else {
        std::vector<float> strip_unit_hor(width, 0.0f);
        std::vector<float> strip_unit_ver(height, 0.0f);
        for (int x = 0; x < width; ++x) {
            float maxH = 0.0f;
            for (int y = 0; y < height; ++y) {
                float val = grid.unitSizeHor[(mapY + y) * grid.width + (mapX + x)];
                if (val > maxH) maxH = val;
            }
            strip_unit_hor[x] = maxH;
        }
        for (int y = 0; y < height; ++y) {
            float maxV = 0.0f;
            for (int x = 0; x < width; ++x) {
                float val = grid.unitSizeVert[(mapY + y) * grid.width + (mapX + x)];
                if (val > maxV) maxV = val;
            }
            strip_unit_ver[y] = maxV;
        }

        float sumH = 0.0f;
        for (float v : strip_unit_hor) sumH += v;
        float sumV = 0.0f;
        for (float v : strip_unit_ver) sumV += v;

        if (width == 16 && height == 16 && sumH <= 40.0f && sumV > 40.0f) {
            splitH = false; // Only vertical failed tolerance, split vertically
        } else if (width == 16 && height == 16 && sumV <= 40.0f && sumH > 40.0f) {
            splitH = true; // Only horizontal failed tolerance, split horizontally
        } else {
            // Retail CSubPatchTreeLODLockElement::BuildTree evaluates strip at max resolution (128)
            int sub_step = std::max(1, (16 * width) / 128);

            float u_needed = 0.0f;
            for (int x = 0; x < width; x += sub_step) {
                float maxH = 0.0f;
                int endX = std::min(width, x + sub_step);
                for (int i = x; i < endX; ++i) {
                    if (strip_unit_hor[i] > maxH) maxH = strip_unit_hor[i];
                }
                u_needed += maxH * sub_step;
            }

            float v_needed = 0.0f;
            for (int y = 0; y < height; y += sub_step) {
                float maxV = 0.0f;
                int endY = std::min(height, y + sub_step);
                for (int j = y; j < endY; ++j) {
                    if (strip_unit_ver[j] > maxV) maxV = strip_unit_ver[j];
                }
                v_needed += maxV * sub_step;
            }

            splitH = (v_needed < u_needed);
        }
    }

    if (splitH) {
        int halfW = width / 2;
        node->leftChild = buildHierarchy(grid, mapX, mapY, halfW, height, minBand, node->firstNonSplitBand);
        node->rightChild = buildHierarchy(grid, mapX + halfW, mapY, halfW, height, minBand, node->firstNonSplitBand);
    } else {
        int halfH = height / 2;
        node->leftChild = buildHierarchy(grid, mapX, mapY, width, halfH, minBand, node->firstNonSplitBand);
        node->rightChild = buildHierarchy(grid, mapX, mapY + halfH, width, halfH, minBand, node->firstNonSplitBand);
    }

    return node;
}

static bool cellHasWater(const LevData& lev, int x, int y, float* outWaterHeight = nullptr, int* outWaterType = nullptr) {
    const auto& grid = getMapWaterGrid(lev);
    const auto& wc = grid.get(x, y);
    if (wc.hasWater) {
        if (outWaterHeight) *outWaterHeight = wc.waterHeight;
        if (outWaterType) *outWaterType = wc.dominantType;
        return true;
    }
    return false;
}

// Window-recentering peekInterpolatedWaterHeight matching CEngineMap::PeekInterpolatedWaterHeight (0x12615d0)
static float peekInterpolatedWaterHeight(const LevData& lev, int mapx, int mapy, int distance_to_look, bool* outFound) {
    const auto& grid = getMapWaterGrid(lev);
    int minX = mapx - distance_to_look;
    int maxX = mapx + distance_to_look;
    int minY = mapy - distance_to_look;
    int maxY = mapy + distance_to_look;

    bool hasWater = (grid.get(mapx, mapy).hasWater != 0);
    if (!hasWater && distance_to_look > 0) {
        bool found = false;
        int searchMinX = std::max(0, mapx - distance_to_look);
        int searchMaxX = std::min(lev.width, mapx + distance_to_look);
        int searchMinY = std::max(0, mapy - distance_to_look);
        int searchMaxY = std::min(lev.height, mapy + distance_to_look);

        for (int j = searchMinX; j <= searchMaxX && !found; ++j) {
            for (int v5 = searchMinY; v5 <= searchMaxY && !found; ++v5) {
                if (grid.get(j, v5).hasWater) {
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
        if (!hasWater) {
            if (outFound) *outFound = false;
            return 0.0f;
        }
    }

    float totalWaterHeight = 0.0f;
    float pointsWithWater = 0.0f;
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            const auto& wc = grid.get(x, y);
            if (wc.hasWater) {
                totalWaterHeight += wc.waterHeight;
                pointsWithWater += 1.0f;
            }
        }
    }

    if (pointsWithWater > 0.0f) {
        if (outFound) *outFound = true;
        return totalWaterHeight / pointsWithWater;
    }
    if (outFound) *outFound = false;
    return 0.0f;
}


static bool peekInterpolatedHasWaterFast(const LevData& lev, int gx, int gy, int distanceToLook = 1) {
    for (int y = gy - distanceToLook; y <= gy + distanceToLook; ++y) {
        for (int x = gx - distanceToLook; x <= gx + distanceToLook; ++x) {
            if (cellHasWater(lev, x, y)) return true;
        }
    }
    return false;
}

static int peekInterpolatedWaterType(const LevData& lev, int gx, int gy, int distanceToLook = 1) {
    const auto& grid = getMapWaterGrid(lev);
    const auto& wc = grid.get(gx, gy);
    if (wc.hasWater && wc.dominantType != 0 && wc.dominantType != 7) {
        return wc.dominantType;
    }
    for (int y = gy - distanceToLook; y <= gy + distanceToLook; ++y) {
        for (int x = gx - distanceToLook; x <= gx + distanceToLook; ++x) {
            const auto& nwc = grid.get(x, y);
            if (nwc.hasWater && nwc.dominantType != 0 && nwc.dominantType != 7) {
                return nwc.dominantType;
            }
        }
    }
    return 0;
}

static void copyBoundingBoxToAllChildren(BgTreeNode* parent, const BgAABB& aabb) {
    if (!parent) return;
    if (parent->leftChild) {
        parent->leftChild->aabb = aabb;
        copyBoundingBoxToAllChildren(parent->leftChild.get(), aabb);
    }
    if (parent->rightChild) {
        parent->rightChild->aabb = aabb;
        copyBoundingBoxToAllChildren(parent->rightChild.get(), aabb);
    }
}

// Matching CLandscapeBackgroundDynamicBuildTemplate::CopyBoundingBoxToAllChildren (0x12bb820)
static void apply16x16BoundingBoxCopy(BgTreeNode* node) {
    if (!node) return;
    if (node->width == 16 && node->height == 16) {
        copyBoundingBoxToAllChildren(node, node->aabb);
    } else {
        apply16x16BoundingBoxCopy(node->leftChild.get());
        apply16x16BoundingBoxCopy(node->rightChild.get());
    }
}

void BackgroundTreeBuilder::computeBoundingBoxes(
    BgTreeNode* node,
    const LevData& lev,
    int worldX, int worldY
) {
    if (!node) return;

    if (node->leftChild && node->rightChild) {
        computeBoundingBoxes(node->leftChild.get(), lev, worldX, worldY);
        computeBoundingBoxes(node->rightChild.get(), lev, worldX, worldY);

        node->aabb.minX = std::min(node->leftChild->aabb.minX, node->rightChild->aabb.minX);
        node->aabb.minY = std::min(node->leftChild->aabb.minY, node->rightChild->aabb.minY);
        node->aabb.minZ = std::min(node->leftChild->aabb.minZ, node->rightChild->aabb.minZ);
        node->aabb.maxX = std::max(node->leftChild->aabb.maxX, node->rightChild->aabb.maxX);
        node->aabb.maxY = std::max(node->leftChild->aabb.maxY, node->rightChild->aabb.maxY);
        node->aabb.maxZ = std::max(node->leftChild->aabb.maxZ, node->rightChild->aabb.maxZ);
    } else {
        float minZ = 1e30f;
        float maxZ = -1e30f;
        for (int y = node->mapY; y <= node->mapY + node->height; ++y) {
            for (int x = node->mapX; x <= node->mapX + node->width; ++x) {
                int cx = std::clamp(x, 0, lev.gridWidth - 1);
                int cy = std::clamp(y, 0, lev.gridHeight - 1);
                float h = lev.getCell(cx, cy).height;
                if (h < minZ) minZ = h;
                if (h > maxZ) maxZ = h;

                bool waterFound = false;
                float wh = peekInterpolatedWaterHeight(lev, x, y, 1, &waterFound);
                if (waterFound) {
                    if (wh < minZ) minZ = wh;
                    if (wh > maxZ) maxZ = wh;
                }
            }
        }
        node->aabb.minX = static_cast<float>(worldX + node->mapX);
        node->aabb.minY = static_cast<float>(worldY + node->mapY);
        node->aabb.minZ = minZ;
        node->aabb.maxX = static_cast<float>(worldX + node->mapX + node->width);
        node->aabb.maxY = static_cast<float>(worldY + node->mapY + node->height);
        node->aabb.maxZ = maxZ;
    }
}

struct BackgroundTreeBuilder::EngineLandscapeLODMap {
    int mapWidth = 128;
    int mapHeight = 128;
    float lodDistanceScale = 1.0f;
    std::vector<uint8_t> lodBandThreshold;

    uint8_t get(int x, int y) const {
        int cx = std::clamp(x, 0, mapWidth);
        int cy = std::clamp(y, 0, mapHeight);
        return lodBandThreshold[cx + cy * (mapWidth + 1)];
    }

    double calculateBaseZThreshold(const LevData& lev, int xp, int yp, float mesh_detail) const {
        int v4 = yp;
        int v5 = xp;
        int v6 = 1;
        if ((((static_cast<uint8_t>(yp) | static_cast<uint8_t>(xp)) & 1)) == 0) {
            do {
                if (!v6) break;
                v6 *= 2;
            } while (((yp | xp) & v6) == 0);
        }

        int v7 = 0, v8 = 0, v9 = 0, v10 = 0;
        if ((v6 & xp) == 0) {
            v9 = xp;
            v7 = yp;
            v8 = xp;
            v10 = v7 - v6;
            yp += v6;
        } else if ((v6 & yp) != 0) {
            v7 = yp;
            if (((yp ^ xp) & (2 * v6)) != 0) {
                v8 = xp - v6;
                v9 = v6 + xp;
                xp -= v6;
            } else {
                v8 = v6 + xp;
                v9 = xp - v6;
                xp += v6;
            }
            v10 = v7 - v6;
            yp += v6;
        } else {
            v9 = xp - v6;
            v10 = yp;
            xp += v6;
            v8 = xp;
        }

        if (v9 < 0 || v8 < 0 || v9 > mapWidth || v8 > mapWidth ||
            v10 < 0 || yp < 0 || v10 > mapHeight || yp > mapHeight) {
            return 1.0e30;
        }

        auto getHeight = [&](int x, int y) -> float {
            int cx = std::clamp(x, 0, lev.gridWidth - 1);
            int cy = std::clamp(y, 0, lev.gridHeight - 1);
            return lev.getCell(cx, cy).height;
        };

        float zthresholda = getHeight(v5, v4);
        float pdist = getHeight(v9, v10);
        float pdista = (getHeight(xp, yp) + pdist) * 0.5f;
        float zthresholdb = pdista - zthresholda;
        float zthresholdc = std::fabs(zthresholdb);
        float zthreshold = zthresholdc * 400.0f / mesh_detail;

        BgVec3 normal = peekMapNormal(lev, v5, v4);
        BgVec3 pnormal0 = peekMapNormal(lev, v9, v10);
        BgVec3 pnormal1 = peekMapNormal(lev, xp, yp);

        float parentx1 = pnormal0.x * normal.x + normal.y * pnormal0.y + normal.z * pnormal0.z;
        float parenty1 = normal.z * pnormal1.z + normal.x * pnormal1.x + normal.y * pnormal1.y;
        double v13 = parentx1;
        if (parenty1 <= parentx1)
            v13 = parenty1;
        float parentx1a = static_cast<float>(v13);
        float pdistb = static_cast<float>(v5 - v9);
        float pdist_4 = static_cast<float>(v4 - v10);
        float parenty1a = pdist_4 * pdist_4 + pdistb * pdistb;
        float parenty1b = std::sqrt(parenty1a);
        float parentx1b = 1.0f - parentx1a;
        float parentx1c = parenty1b * parentx1b;
        float ldif_zthreshold = parentx1c * 400.0f / mesh_detail;
        if (ldif_zthreshold < zthreshold)
            return zthreshold;
        return ldif_zthreshold;
    }

    void updateThresholdPoint(const LevData& lev, int xp, int yp, float mesh_detail) {
        int v4 = xp;
        if (xp < 0) return;
        while (true) {
            float v7 = mesh_detail;
            if (yp < 0 || v4 > mapWidth || yp > mapHeight) return;
            int idx = v4 + yp * (mapWidth + 1);
            int current_band = lodBandThreshold[idx];
            int new_band = current_band;
            if (current_band == 0) {
                double zthreshold = calculateBaseZThreshold(lev, v4, yp, mesh_detail);
                int v9 = 2;
                while (getLodBandZ(v9, lodDistanceScale) <= zthreshold) {
                    ++v9;
                    if (v9 >= 8) {
                        new_band = 7;
                        break;
                    }
                }
                if (v9 < 8) {
                    new_band = std::max(0, v9 - 1);
                }
            }

            int v12 = 1;
            if (v4 == 0 && yp == 0) {
                if (current_band < new_band)
                    lodBandThreshold[0] = static_cast<uint8_t>(new_band);
                return;
            }

            if ((v4 & 1) == 0) {
                do {
                    if ((v12 & yp) != 0) break;
                    v12 *= 2;
                } while ((v12 & v4) == 0);
            }

            int v17 = 0;
            if ((v4 & v12 & yp) != 0) {
                if (v4 - v12 >= 0) {
                    int neighbor = v4 + yp * (mapWidth + 1) - v12;
                    if (new_band <= lodBandThreshold[neighbor])
                        new_band = lodBandThreshold[neighbor];
                }
                if (v12 + v4 <= mapWidth) {
                    int neighbor = v4 + v12 + yp * (mapWidth + 1);
                    if (new_band <= lodBandThreshold[neighbor])
                        new_band = lodBandThreshold[neighbor];
                }
                if (yp - v12 >= 0) {
                    int neighbor = v4 + (yp - v12) * (mapWidth + 1);
                    if (new_band <= lodBandThreshold[neighbor])
                        new_band = lodBandThreshold[neighbor];
                }
                if (v12 + yp <= mapHeight) {
                    v17 = lodBandThreshold[v4 + (v12 + yp) * (mapWidth + 1)];
                    if (new_band <= v17)
                        new_band = v17;
                }
            } else {
                if (v12 > 1) {
                    int v18 = v12 / 2;
                    if (v4 - v18 >= 0 && yp - v18 >= 0) {
                        int neighbor = v4 + (yp - v18) * (mapWidth + 1) - v18;
                        if (new_band <= lodBandThreshold[neighbor])
                            new_band = lodBandThreshold[neighbor];
                    }
                    if (v18 + v4 <= mapWidth && yp - v18 >= 0) {
                        int val = lodBandThreshold[v4 + v18 + (yp - v18) * (mapWidth + 1)];
                        if (new_band <= val)
                            new_band = val;
                    }
                    if (v4 - v18 >= 0 && v18 + yp <= mapHeight) {
                        int neighbor = v4 + (v18 + yp) * (mapWidth + 1) - v18;
                        if (new_band <= lodBandThreshold[neighbor])
                            new_band = lodBandThreshold[neighbor];
                    }
                    if (v18 + v4 <= mapWidth && v18 + yp <= mapHeight) {
                        v17 = lodBandThreshold[v4 + v18 + (v18 + yp) * (mapWidth + 1)];
                        if (new_band <= v17)
                            new_band = v17;
                    }
                }
            }

            if (current_band < new_band) {
                lodBandThreshold[idx] = static_cast<uint8_t>(new_band);
                if ((v4 & v12) != 0) {
                    if ((v12 & yp) != 0) {
                        int cbA = v4 - v12;
                        updateThresholdPoint(lev, v4 - v12, yp - v12, mesh_detail);
                        v4 += v12;
                        updateThresholdPoint(lev, v4, yp - v12, mesh_detail);
                        yp += v12;
                        updateThresholdPoint(lev, cbA, yp, mesh_detail);
                    } else {
                        updateThresholdPoint(lev, v4, yp - v12, mesh_detail);
                        yp += v12;
                    }
                } else {
                    updateThresholdPoint(lev, v4 - v12, yp, mesh_detail);
                    v4 += v12;
                }
                if (v4 >= 0) continue;
            }
            return;
        }
    }

    void build(const LevData& lev, float mesh_detail = 2.0f, float lod_scale = 1.0f) {
        mapWidth = lev.width;
        mapHeight = lev.height;
        lodDistanceScale = lod_scale;
        lodBandThreshold.assign((mapWidth + 1) * (mapHeight + 1), 0);
        for (int y = 0; y <= mapHeight; ++y) {
            for (int x = 0; x <= mapWidth; ++x) {
                updateThresholdPoint(lev, x, y, mesh_detail);
            }
        }
    }
};

#pragma pack(push, 1)
struct BgVertexRaw {
    uint16_t x = 0, y = 0;
    float z = 0.0f;
    uint32_t normal = 0;
    uint16_t u = 0, v = 0;
};
static_assert(sizeof(BgVertexRaw) == 16, "BgVertexRaw must be 16 bytes");

struct EdgeVertRaw {
    float z = 0.0f;           // +0
    uint32_t normal = 0;      // +4
    int16_t x = 0, y = 0;     // +8, +10
    uint16_t gridIdx = 0;     // +12
    uint16_t edgeIdx = 0;     // +14
    int16_t fanBaseIdx = -1;  // +16
    uint8_t u = 0, v = 0;     // +18, +19
};
static_assert(sizeof(EdgeVertRaw) == 20, "EdgeVertRaw must be 20 bytes");

struct FanBaseVertRaw {
    float z = 0.0f;           // +0
    uint32_t normal = 0;      // +4
    int16_t x = 0, y = 0;     // +8, +10
    uint16_t gridIdx = 0;     // +12
    uint8_t u = 0, v = 0;     // +14, +15
};
static_assert(sizeof(FanBaseVertRaw) == 16, "FanBaseVertRaw must be 16 bytes");

struct BgWaterVertexRaw {
    int16_t x = 0;
    int16_t y = 0;
    float z = 0.0f;
    float shoreLookup[12] = {0.0f};
};
static_assert(sizeof(BgWaterVertexRaw) == 56, "BgWaterVertexRaw must be 56 bytes");

struct BgSeaVertexRaw {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};
static_assert(sizeof(BgSeaVertexRaw) == 12, "BgSeaVertexRaw must be 12 bytes");

struct EdgeWaterVertRaw {
    float z = 0.0f;                      // +0 (4 bytes)
    float shoreLookup[12] = {0.0f};       // +4 (48 bytes)
    uint16_t waterVertexIndex = 0;       // +52 (2 bytes)
    uint16_t waterIndexBufferIndex = 0;  // +54 (2 bytes)
    int16_t fanBaseIndex = -1;           // +56 (2 bytes)
    uint16_t landscapeTargetVertex = 0;  // +58 (2 bytes)
};
static_assert(sizeof(EdgeWaterVertRaw) == 60, "EdgeWaterVertRaw must be 60 bytes");

struct EdgeWaterFanBaseVertRaw {
    float z = 0.0f;                      // +0 (4 bytes)
    float shoreLookup[12] = {0.0f};       // +4 (48 bytes)
    uint16_t waterVertexIndex = 0;       // +52 (2 bytes)
    uint16_t padding = 0;                // +54 (2 bytes)
};
static_assert(sizeof(EdgeWaterFanBaseVertRaw) == 56, "EdgeWaterFanBaseVertRaw must be 56 bytes");
#pragma pack(pop)

struct EdgeMapperElement {
    uint16_t fanBaseVertex = 0xFFFF;
    uint16_t edgeIndexBufferIndex = 0xFFFF;
    int16_t fanBaseGridX = -1;
    int16_t fanBaseGridY = -1;
};

struct EdgeMappers {
    std::vector<EdgeMapperElement> side[4]; // 0: Left, 1: Right, 2: Bottom, 3: Top
};

struct Point2D {
    int16_t x = 0;
    int16_t y = 0;
};

static inline BgVertexRaw buildVertex(
    const LevData& lev,
    int worldX, int worldY,
    int mapX, int mapY,
    int patchW, int patchH,
    int subx, int suby
) {
    int gx = mapX + subx;
    int gy = mapY + suby;
    int cx = std::clamp(gx, 0, lev.gridWidth - 1);
    int cy = std::clamp(gy, 0, lev.gridHeight - 1);

    BgVertexRaw v;
    v.x = static_cast<uint16_t>(worldX + gx);
    v.y = static_cast<uint16_t>(worldY + gy);
    v.z = lev.getCell(cx, cy).height;
    v.normal = calculatePackedNormal(lev, gx, gy);
    v.u = static_cast<uint16_t>(std::nearbyint((subx * 255.0f) / patchW));
    v.v = static_cast<uint16_t>(std::nearbyint((suby * 255.0f) / patchH));
    return v;
}

// Matching BuildSharedIndexBufferGenerationTables (0x123fd80)
static void buildSharedIndexBufferGenerationTables(
    int width, int height,
    std::vector<Point2D>& outVerts,
    int& outStripedIndexCount,
    std::vector<uint16_t>* outTriList = nullptr
) {
    static constexpr int ofs_table_x[2][2][3] = {
        {{0, 1, 1}, {0, 0, 1}},
        {{0, 1, 0}, {1, 0, 1}}
    };
    static constexpr int ofs_table_y[2][2][3] = {
        {{0, 0, 1}, {0, 1, 1}},
        {{0, 0, 1}, {0, 1, 1}}
    };

    std::vector<int> vertex_id(17 * 17, -1);
    std::vector<uint16_t> striped;
    striped.resize(6 * width * height);

    int striped_count = 0;
    int vertex_count = 0;
    outVerts.resize((width + 1) * (height + 1));
    if (outTriList) {
        outTriList->clear();
        outTriList->reserve(width * height * 6);
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int odd_square = (x ^ y) & 1;
            for (int poly = 0; poly < 2; ++poly) {
                int v14 = x + ofs_table_x[odd_square][poly][0];
                int v15 = y + ofs_table_y[odd_square][poly][0];
                int id0_idx = 17 * v14 + v15;
                int v16 = vertex_id[id0_idx];
                if (v16 < 0) {
                    v16 = vertex_count++;
                    outVerts[v16] = {static_cast<int16_t>(v14), static_cast<int16_t>(v15)};
                    vertex_id[id0_idx] = v16;
                }

                int v17 = x + ofs_table_x[odd_square][poly][1];
                int idy = y + ofs_table_y[odd_square][poly][1];
                int id1_idx = 17 * v17 + idy;
                int v18 = vertex_id[id1_idx];
                if (v18 < 0) {
                    v18 = vertex_count++;
                    outVerts[v18] = {static_cast<int16_t>(v17), static_cast<int16_t>(idy)};
                    vertex_id[id1_idx] = v18;
                }

                int v19 = x + ofs_table_x[odd_square][poly][2];
                int v20 = y + ofs_table_y[odd_square][poly][2];
                int id2_idx = 17 * v19 + v20;
                int v22 = vertex_id[id2_idx];
                if (v22 < 0) {
                    v22 = vertex_count++;
                    outVerts[v22] = {static_cast<int16_t>(v19), static_cast<int16_t>(v20)};
                    vertex_id[id2_idx] = v22;
                }

                int v21 = v18;
                if (outTriList) {
                    if (poly == 1) {
                        outTriList->push_back(static_cast<uint16_t>(v16));
                        outTriList->push_back(static_cast<uint16_t>(v21));
                        outTriList->push_back(static_cast<uint16_t>(v22));
                    } else {
                        outTriList->push_back(static_cast<uint16_t>(v21));
                        outTriList->push_back(static_cast<uint16_t>(v16));
                        outTriList->push_back(static_cast<uint16_t>(v22));
                    }
                }

                int v24 = striped_count;
                if (poly == (striped_count & 1)) {
                    if (v24 <= 3) {
                        striped[v24] = static_cast<uint16_t>(v16);
                        striped[v24 + 1] = static_cast<uint16_t>(v16);
                        striped[v24 + 2] = static_cast<uint16_t>(v21);
                        striped[v24 + 3] = static_cast<uint16_t>(v22);
                        striped_count += 4;
                    } else {
                        striped[v24] = striped[v24 - 1];
                        striped[v24 + 1] = static_cast<uint16_t>(v16);
                        striped[v24 + 2] = static_cast<uint16_t>(v16);
                        striped[v24 + 3] = static_cast<uint16_t>(v16);
                        striped[v24 + 4] = static_cast<uint16_t>(v21);
                        striped[v24 + 5] = static_cast<uint16_t>(v22);
                        striped_count += 6;
                    }
                } else if (v24 > 2 && v16 == striped[v24 - 2] && v21 == striped[v24 - 1]) {
                    striped[v24] = static_cast<uint16_t>(v22);
                    ++striped_count;
                } else if (v24 >= 3) {
                    striped[v24] = striped[v24 - 1];
                    striped[v24 + 1] = static_cast<uint16_t>(v16);
                    striped[v24 + 2] = static_cast<uint16_t>(v16);
                    striped[v24 + 3] = static_cast<uint16_t>(v21);
                    striped[v24 + 4] = static_cast<uint16_t>(v22);
                    striped_count += 5;
                } else {
                    striped[v24] = static_cast<uint16_t>(v16);
                    striped[v24 + 1] = static_cast<uint16_t>(v21);
                    striped[v24 + 2] = static_cast<uint16_t>(v22);
                    striped_count += 3;
                }
            }
        }
    }
    outStripedIndexCount = striped_count;
}

// Matching BuildLODSubSectionBuffersTriangle (0x1247230)
static void buildLODSubSectionBuffersTriangle(
    const LevData& lev,
    const BackgroundTreeBuilder::EngineLandscapeLODMap* pLodMap,
    int worldX, int worldY,
    int mapX, int mapY, int patchW, int patchH, int lodBand,
    std::vector<BgVertexRaw>& vb,
    std::vector<uint16_t>& ib,
    std::vector<int>& vertex_grid,
    EdgeMappers& mappers,
    int subx0, int suby0,
    int subx1, int suby1,
    int subx2, int suby2
) {
    while (true) {
        int midX = (subx0 + subx1) / 2;
        int midY = (suby0 + suby1) / 2;
        if ((midX == subx0 || midX == subx1) && (midY == suby0 || midY == suby1)) {
            break; // cannot subdivide further
        }
        if (pLodMap) {
            uint8_t reqBand = pLodMap->get(mapX + midX, mapY + midY);
            if (reqBand < lodBand) {
                break; // coarser than our band, do not split
            }
        }
        // Subdivide into 2 triangles
        buildLODSubSectionBuffersTriangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            vb, ib, vertex_grid, mappers,
            subx1, suby1, subx2, suby2, midX, midY
        );
        suby1 = suby0;
        subx1 = subx0;
        suby0 = suby2;
        subx0 = subx2;
        subx2 = midX;
        suby2 = midY;
    }

    auto ensureVertex = [&](int sx, int sy) -> int {
        int gIdx = sx + sy * (patchW + 1);
        if (vertex_grid[gIdx] < 0) {
            int vIdx = static_cast<int>(vb.size());
            vb.push_back(buildVertex(lev, worldX, worldY, mapX, mapY, patchW, patchH, sx, sy));
            vertex_grid[gIdx] = vIdx;
        }
        return vertex_grid[gIdx];
    };

    int i0 = ensureVertex(subx0, suby0);
    int i1 = ensureVertex(subx1, suby1);
    int i2 = ensureVertex(subx2, suby2);

    uint16_t curIbPos = static_cast<uint16_t>(ib.size());
    ib.push_back(static_cast<uint16_t>(i0));
    ib.push_back(static_cast<uint16_t>(i1));
    ib.push_back(static_cast<uint16_t>(i2));

    int vtx[3] = {subx0, subx1, subx2};
    int vty[3] = {suby0, suby1, suby2};
    uint16_t ib_index[3] = {curIbPos, static_cast<uint16_t>(curIbPos + 1), static_cast<uint16_t>(curIbPos + 2)};

    for (int i = 0; i < 3; ++i) {
        int v31 = (i + 1) % 3;
        int v32 = (v31 + 1) % 3;

        // Side 0: Left edge (x == 0)
        if (vtx[i] == 0 && vtx[v31] == 0) {
            int low_idx = (vty[i] >= vty[v31]) ? v31 : i;
            int low_y = vty[low_idx];
            if (low_y < static_cast<int>(mappers.side[0].size())) {
                int fbX = vtx[v32];
                int fbY = vty[v32];
                int fbV = vertex_grid[fbX + fbY * (patchW + 1)];
                mappers.side[0][low_y].fanBaseVertex = static_cast<uint16_t>(fbV);
                mappers.side[0][low_y].edgeIndexBufferIndex = ib_index[low_idx];
                mappers.side[0][low_y].fanBaseGridX = static_cast<int16_t>(fbX);
                mappers.side[0][low_y].fanBaseGridY = static_cast<int16_t>(fbY);
            }
            return;
        }
        // Side 1: Right edge (x == patchW)
        if (vtx[i] == patchW && vtx[v31] == patchW) {
            int low_idx = (vty[i] >= vty[v31]) ? v31 : i;
            int low_y = vty[low_idx];
            if (low_y < static_cast<int>(mappers.side[1].size())) {
                int fbX = vtx[v32];
                int fbY = vty[v32];
                int fbV = vertex_grid[fbX + fbY * (patchW + 1)];
                mappers.side[1][low_y].fanBaseVertex = static_cast<uint16_t>(fbV);
                mappers.side[1][low_y].edgeIndexBufferIndex = ib_index[low_idx];
                mappers.side[1][low_y].fanBaseGridX = static_cast<int16_t>(fbX);
                mappers.side[1][low_y].fanBaseGridY = static_cast<int16_t>(fbY);
            }
            return;
        }
        // Side 2: Bottom edge (y == 0)
        if (vty[i] == 0 && vty[v31] == 0) {
            int low_idx = (vtx[i] >= vtx[v31]) ? v31 : i;
            int low_x = vtx[low_idx];
            if (low_x < static_cast<int>(mappers.side[2].size())) {
                int fbX = vtx[v32];
                int fbY = vty[v32];
                int fbV = vertex_grid[fbX + fbY * (patchW + 1)];
                mappers.side[2][low_x].fanBaseVertex = static_cast<uint16_t>(fbV);
                mappers.side[2][low_x].edgeIndexBufferIndex = ib_index[low_idx];
                mappers.side[2][low_x].fanBaseGridX = static_cast<int16_t>(fbX);
                mappers.side[2][low_x].fanBaseGridY = static_cast<int16_t>(fbY);
            }
            return;
        }
        // Side 3: Top edge (y == patchH)
        if (vty[i] == patchH && vty[v31] == patchH) {
            int low_idx = (vtx[i] >= vtx[v31]) ? v31 : i;
            int low_x = vtx[low_idx];
            if (low_x < static_cast<int>(mappers.side[3].size())) {
                int fbX = vtx[v32];
                int fbY = vty[v32];
                int fbV = vertex_grid[fbX + fbY * (patchW + 1)];
                mappers.side[3][low_x].fanBaseVertex = static_cast<uint16_t>(fbV);
                mappers.side[3][low_x].edgeIndexBufferIndex = ib_index[low_idx];
                mappers.side[3][low_x].fanBaseGridX = static_cast<int16_t>(fbX);
                mappers.side[3][low_x].fanBaseGridY = static_cast<int16_t>(fbY);
            }
            return;
        }
    }
}

// Matching BuildLODSubSectionBuffersRectangle (0x1247dd0)
static void buildLODSubSectionBuffersRectangle(
    const LevData& lev,
    const BackgroundTreeBuilder::EngineLandscapeLODMap* pLodMap,
    int worldX, int worldY,
    int mapX, int mapY, int patchW, int patchH, int lodBand,
    int lockedSpacingX, int lockedSpacingY,
    std::vector<BgVertexRaw>& vb,
    std::vector<uint16_t>& ib,
    std::vector<int>& vertex_grid,
    EdgeMappers& mappers,
    int subx, int suby, int width, int height
) {
    while (true) {
        while (width > height || width > lockedSpacingX) {
            width /= 2;
            buildLODSubSectionBuffersRectangle(
                lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
                lockedSpacingX, lockedSpacingY, vb, ib, vertex_grid, mappers,
                subx, suby, width, height
            );
            subx += width;
        }
        if (height <= width && height <= lockedSpacingY) {
            break;
        }
        height /= 2;
        buildLODSubSectionBuffersRectangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            lockedSpacingX, lockedSpacingY, vb, ib, vertex_grid, mappers,
            subx, suby, width, height
        );
        suby += height;
    }

    if (width == 1) {
        if (((subx ^ suby) & 1) != 0) {
            buildLODSubSectionBuffersTriangle(
                lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
                vb, ib, vertex_grid, mappers,
                subx, suby, subx, suby + 1, subx + 1, suby
            );
            buildLODSubSectionBuffersTriangle(
                lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
                vb, ib, vertex_grid, mappers,
                subx + 1, suby + 1, subx + 1, suby, subx, suby + 1
            );
        } else {
            buildLODSubSectionBuffersTriangle(
                lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
                vb, ib, vertex_grid, mappers,
                subx, suby, subx, suby + 1, subx + 1, suby + 1
            );
            buildLODSubSectionBuffersTriangle(
                lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
                vb, ib, vertex_grid, mappers,
                subx, suby, subx + 1, suby + 1, subx + 1, suby
            );
        }
    } else {
        int midY = width / 2 + suby;
        int midX = subx + width / 2;
        int endX = subx + width;
        int endY = suby + width;

        buildLODSubSectionBuffersTriangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            vb, ib, vertex_grid, mappers,
            endX, suby, subx, suby, midX, midY
        );
        buildLODSubSectionBuffersTriangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            vb, ib, vertex_grid, mappers,
            subx, endY, endX, endY, midX, midY
        );
        buildLODSubSectionBuffersTriangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            vb, ib, vertex_grid, mappers,
            subx, suby, subx, endY, midX, midY
        );
        buildLODSubSectionBuffersTriangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            vb, ib, vertex_grid, mappers,
            endX, endY, endX, suby, midX, midY
        );
    }
}

// Matching CacheOptimizeBuffers (0x124abe0)
static void cacheOptimizeBuffers(
    std::vector<uint16_t>& ib,
    std::vector<BgVertexRaw>& vb,
    EdgeMappers& mappers,
    std::vector<int>& vertex_grid,
    int patchW, int patchH
) {
    int poly_count = static_cast<int>(ib.size() / 3);
    if (poly_count == 0) return;

    std::vector<uint16_t> old_ib = ib;

    // Stripifier reordering
    TriangleStripifier::reorderTriangleListInPlace(ib, 20);

    // Build poly_index_remap_table
    std::vector<int> poly_index_remap_table(3 * poly_count);
    std::vector<uint32_t> dst_polys_found((poly_count + 31) / 32, 0);

    // Build vertex-to-new-poly candidate map
    std::vector<std::vector<int>> vert_to_new_polys(vb.size());
    for (int new_p = 0; new_p < poly_count; ++new_p) {
        uint16_t nv0 = ib[3 * new_p + 0];
        uint16_t nv1 = ib[3 * new_p + 1];
        uint16_t nv2 = ib[3 * new_p + 2];
        uint16_t min_nv = std::min({nv0, nv1, nv2});
        if (min_nv < vert_to_new_polys.size()) {
            vert_to_new_polys[min_nv].push_back(new_p);
        }
    }

    for (int old_p = 0; old_p < poly_count; ++old_p) {
        uint16_t ov0 = old_ib[3 * old_p + 0];
        uint16_t ov1 = old_ib[3 * old_p + 1];
        uint16_t ov2 = old_ib[3 * old_p + 2];
        uint16_t min_ov = std::min({ov0, ov1, ov2});

        const auto* pCandidates = (min_ov < vert_to_new_polys.size()) ? &vert_to_new_polys[min_ov] : nullptr;
        if (!pCandidates) continue;

        for (int new_p : *pCandidates) {
            if ((dst_polys_found[new_p >> 5] & (1u << (new_p & 31))) != 0) {
                continue;
            }
            uint16_t nv0 = ib[3 * new_p + 0];
            uint16_t nv1 = ib[3 * new_p + 1];
            uint16_t nv2 = ib[3 * new_p + 2];

            int corner_remap[3] = {-1, -1, -1};
            const uint16_t old_corners[3] = {ov0, ov1, ov2};
            const uint16_t new_corners[3] = {nv0, nv1, nv2};
            bool match = true;

            for (int c = 0; c < 3; ++c) {
                int found_k = -1;
                for (int k = 0; k < 3; ++k) {
                    if (old_corners[c] == new_corners[k]) {
                        found_k = k;
                        break;
                    }
                }
                if (found_k < 0) {
                    match = false;
                    break;
                }
                corner_remap[c] = found_k;
            }

            if (match) {
                dst_polys_found[new_p >> 5] |= (1u << (new_p & 31));
                poly_index_remap_table[3 * old_p + 0] = 3 * new_p + corner_remap[0];
                poly_index_remap_table[3 * old_p + 1] = 3 * new_p + corner_remap[1];
                poly_index_remap_table[3 * old_p + 2] = 3 * new_p + corner_remap[2];
                break;
            }
        }
    }


    // Vertex remapping table
    int vertex_count = static_cast<int>(vb.size());
    std::vector<int> verts_remap(vertex_count, -1);
    int next_idx = 0;
    for (size_t j = 0; j < ib.size(); ++j) {
        uint16_t v = ib[j];
        if (v < vertex_count && verts_remap[v] < 0) {
            verts_remap[v] = next_idx++;
        }
    }
    for (int k = 0; k < vertex_count; ++k) {
        if (verts_remap[k] < 0) {
            verts_remap[k] = next_idx++;
        }
    }

    // Permute vertex buffer
    std::vector<BgVertexRaw> reordered_vb(vertex_count);
    for (int i = 0; i < vertex_count; ++i) {
        reordered_vb[verts_remap[i]] = vb[i];
    }
    vb = std::move(reordered_vb);

    // Remap index buffer
    for (size_t m = 0; m < ib.size(); ++m) {
        ib[m] = static_cast<uint16_t>(verts_remap[ib[m]]);
    }

    // Remap edge mappers
    for (int s = 0; s < 4; ++s) {
        for (auto& elem : mappers.side[s]) {
            if (elem.fanBaseVertex < vertex_count) {
                elem.fanBaseVertex = static_cast<uint16_t>(verts_remap[elem.fanBaseVertex]);
                if (elem.edgeIndexBufferIndex < poly_index_remap_table.size()) {
                    elem.edgeIndexBufferIndex = static_cast<uint16_t>(poly_index_remap_table[elem.edgeIndexBufferIndex]);
                }
            }
        }
    }

    // Remap vertex grid
    for (int y = 0; y <= patchH; ++y) {
        for (int x = 0; x <= patchW; ++x) {
            int idx = x + y * (patchW + 1);
            int val = vertex_grid[idx];
            if (val >= 0 && val < vertex_count) {
                vertex_grid[idx] = verts_remap[val];
            }
        }
    }
}

std::vector<uint8_t> BackgroundTreeBuilder::createPatchFrame(
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    int mapX, int mapY, int patchW, int patchH,
    int lodBand, int texW, int texH,
    const EngineLandscapeLODMap* pLodMap,
    const WaterShoreCalculator* pWaterContext,
    const BgTreeSettings& settings
) {
    std::vector<BgVertexRaw> vb;
    std::vector<uint16_t> ib;
    uint16_t polyCount = 0;
    std::vector<int> vertex_grid((patchW + 1) * (patchH + 1), -1);

    EdgeMappers mappers;
    mappers.side[0].resize(patchH + 1);
    mappers.side[1].resize(patchH + 1);
    mappers.side[2].resize(patchW + 1);
    mappers.side[3].resize(patchW + 1);

    std::vector<uint16_t> fullDetailTriangles;
    if (lodBand == 1) {
        std::vector<Point2D> src_verts;
        int stripedCount = 0;
        buildSharedIndexBufferGenerationTables(patchW, patchH, src_verts, stripedCount, &fullDetailTriangles);
        polyCount = static_cast<uint16_t>(stripedCount - 2);

        int totalVerts = (patchW + 1) * (patchH + 1);
        vb.resize(totalVerts);
        for (int i = 0; i < totalVerts; ++i) {
            int sx = src_verts[i].x;
            int sy = src_verts[i].y;
            vb[i] = buildVertex(lev, worldX, worldY, mapX, mapY, patchW, patchH, sx, sy);
            vertex_grid[sx + sy * (patchW + 1)] = i;
        }
    } else {
        int lockedSpacingX = std::max(1, (16 * std::max(patchW, patchH)) / texW);
        int lockedSpacingY = std::max(1, (16 * std::max(patchW, patchH)) / texH);

        buildLODSubSectionBuffersRectangle(
            lev, pLodMap, worldX, worldY, mapX, mapY, patchW, patchH, lodBand,
            lockedSpacingX, lockedSpacingY,
            vb, ib, vertex_grid, mappers,
            0, 0, patchW, patchH
        );

        polyCount = static_cast<uint16_t>(ib.size() / 3);

        cacheOptimizeBuffers(ib, vb, mappers, vertex_grid, patchW, patchH);
    }

    int vertexCount = static_cast<int>(vb.size());

    // Water subpatch preparation (CWaterGenerator::BuildStaticMapBackgroundBuffers 0x12cea20)
    const std::vector<uint16_t>& patchTriangles = (lodBand == 1) ? fullDetailTriangles : ib;

    int patchWorldMinX = worldX + mapX;
    int patchWorldMinY = worldY + mapY;
    int patchWorldMaxX = patchWorldMinX + patchW;
    int patchWorldMaxY = patchWorldMinY + patchH;
    bool areaHasWater = pWaterContext && pWaterContext->hasWater() &&
                        pWaterContext->areaHasWater(patchWorldMinX, patchWorldMinY, patchWorldMaxX, patchWorldMaxY, worldX, worldY);

    std::vector<int> landscape_to_water_vertex_mapper(vb.size(), -1);
    std::vector<int> landscape_to_water_index_mapper(patchTriangles.size(), -1);
    std::vector<BgWaterVertexRaw> temp_water_verts;
    std::vector<uint16_t> water_indices;
    int dominantWaterType = 0;

    if (areaHasWater) {
        int water_type_counts[9] = {0};

        for (size_t c_index = 0; c_index + 2 < patchTriangles.size(); c_index += 3) {
            uint16_t lv0 = patchTriangles[c_index + 0];
            uint16_t lv1 = patchTriangles[c_index + 1];
            uint16_t lv2 = patchTriangles[c_index + 2];
            if (lv0 >= vb.size() || lv1 >= vb.size() || lv2 >= vb.size()) continue;

            int gx0 = vb[lv0].x - worldX;
            int gy0 = vb[lv0].y - worldY;
            int gx1 = vb[lv1].x - worldX;
            int gy1 = vb[lv1].y - worldY;
            int gx2 = vb[lv2].x - worldX;
            int gy2 = vb[lv2].y - worldY;

            if (peekInterpolatedHasWaterFast(lev, gx0, gy0, 1) ||
                peekInterpolatedHasWaterFast(lev, gx1, gy1, 1) ||
                peekInterpolatedHasWaterFast(lev, gx2, gy2, 1)) {

                int wt0 = peekInterpolatedWaterType(lev, gx0, gy0, 1);
                int wt1 = peekInterpolatedWaterType(lev, gx1, gy1, 1);
                int wt2 = peekInterpolatedWaterType(lev, gx2, gy2, 1);
                if (wt0 >= 1 && wt0 <= 8) ++water_type_counts[wt0];
                if (wt1 >= 1 && wt1 <= 8) ++water_type_counts[wt1];
                if (wt2 >= 1 && wt2 <= 8) ++water_type_counts[wt2];

                for (int j = 0; j < 3; ++j) {
                    uint16_t lv = (j == 0) ? lv0 : ((j == 1) ? lv1 : lv2);
                    int gx = (j == 0) ? gx0 : ((j == 1) ? gx1 : gx2);
                    int gy = (j == 0) ? gy0 : ((j == 1) ? gy1 : gy2);

                    int wvi = landscape_to_water_vertex_mapper[lv];
                    if (wvi < 0) {
                        wvi = static_cast<int>(temp_water_verts.size());
                        landscape_to_water_vertex_mapper[lv] = wvi;

                        BgWaterVertexRaw wv;
                        wv.x = static_cast<int16_t>(vb[lv].x);
                        wv.y = static_cast<int16_t>(vb[lv].y);

                        bool found = false;
                        float wh = peekInterpolatedWaterHeight(lev, gx, gy, 2 * lodBand, &found);
                        if (!found || wh <= 0.0f) {
                            for (int k = 1; k < 3; ++k) {
                                int ogx = (k == 1) ? ((j == 0) ? gx1 : ((j == 1) ? gx2 : gx0)) : ((j == 0) ? gx2 : ((j == 1) ? gx0 : gx1));
                                int ogy = (k == 1) ? ((j == 0) ? gy1 : ((j == 1) ? gy2 : gy0)) : ((j == 0) ? gy2 : ((j == 1) ? gy0 : gy1));
                                wh = peekInterpolatedWaterHeight(lev, ogx, ogy, 2 * lodBand, &found);
                                if (found && wh > 0.0f) break;
                            }
                        }
                        wv.z = wh;

                        float span = 0.0f;
                        pWaterContext->computeShoreLookup(
                            static_cast<float>(wv.x), static_cast<float>(wv.y),
                            wv.shoreLookup, 12, &span
                        );

                        temp_water_verts.push_back(wv);
                    }

                    landscape_to_water_index_mapper[c_index + j] = static_cast<int>(water_indices.size());
                    water_indices.push_back(static_cast<uint16_t>(wvi));
                }
            }
        }

        int v35 = 0;
        if (water_type_counts[1] > 0) { v35 = water_type_counts[1]; dominantWaterType = 1; }
        if (water_type_counts[2] > v35) { v35 = water_type_counts[2]; dominantWaterType = 2; }
        if (water_type_counts[3] > v35) { v35 = water_type_counts[3]; dominantWaterType = 3; }
        if (water_type_counts[4] > v35) { v35 = water_type_counts[4]; dominantWaterType = 4; }
        if (water_type_counts[5] > v35) { v35 = water_type_counts[5]; dominantWaterType = 5; }
        if (water_type_counts[6] > v35) { v35 = water_type_counts[6]; dominantWaterType = 6; }
        if (water_type_counts[7] > v35) { v35 = water_type_counts[7]; dominantWaterType = 7; }
        if (water_type_counts[8] > v35) { dominantWaterType = 8; }
    }

    // 4. Procedural texture baking
    auto textureBytes = Dxt1Baker::bakeTexture(
        lev, materials, mapX, mapY, patchW, patchH, texW, texH, worldX, worldY,
        settings.generateMipmaps, settings.highQualityDxt
    );

    // 5. Edge strips (4 strips: Left, Right, Bottom, Top matching BuildEdgeStrips 0x1246c90)
    std::vector<uint8_t> edgeStripsBytes;
    for (int s = 0; s < 4; ++s) {
        uint16_t startPos = 0, size = 0;
        uint8_t vertical = 0;
        int startX = 0, startY = 0;
        int step_x = 0, step_y = 0;
        int edge_len = 0;

        if (s == 0) { // Left (vertical, x = 0)
            startPos = static_cast<uint16_t>(worldY + mapY);
            size = static_cast<uint16_t>(patchH);
            vertical = 1;
            startX = 0;
            startY = 0;
            step_x = 0;
            step_y = 1;
            edge_len = patchH;
        } else if (s == 1) { // Right (vertical, x = patchW)
            startPos = static_cast<uint16_t>(worldY + mapY);
            size = static_cast<uint16_t>(patchH);
            vertical = 1;
            startX = patchW;
            startY = 0;
            step_x = 0;
            step_y = 1;
            edge_len = patchH;
        } else if (s == 2) { // Bottom (horizontal, y = 0)
            startPos = static_cast<uint16_t>(worldX + mapX);
            size = static_cast<uint16_t>(patchW);
            vertical = 0;
            startX = 0;
            startY = 0;
            step_x = 1;
            step_y = 0;
            edge_len = patchW;
        } else { // Top (horizontal, y = patchH)
            startPos = static_cast<uint16_t>(worldX + mapX);
            size = static_cast<uint16_t>(patchW);
            vertical = 0;
            startX = 0;
            startY = patchH;
            step_x = 1;
            step_y = 0;
            edge_len = patchW;
        }

        std::vector<EdgeVertRaw> evs;
        std::vector<FanBaseVertRaw> fanBaseVerts;

        int cur_x = startX;
        int cur_y = startY;
        int cur_v = vertex_grid[cur_x + cur_y * (patchW + 1)];

        for (int i = 0; i <= edge_len; ++i) {
            int next_v = -1;
            if (i < edge_len) {
                next_v = vertex_grid[(cur_x + step_x) + (cur_y + step_y) * (patchW + 1)];
            }
            if (cur_v >= 0) {
                const auto& v_data = vb[cur_v];
                EdgeVertRaw ev;
                ev.x = v_data.x;
                ev.y = v_data.y;
                ev.z = v_data.z;
                ev.normal = v_data.normal;
                ev.u = static_cast<uint8_t>(v_data.u);
                ev.v = static_cast<uint8_t>(v_data.v);
                ev.gridIdx = static_cast<uint16_t>(cur_v);

                const auto& mapElem = mappers.side[s][i];
                ev.edgeIdx = mapElem.edgeIndexBufferIndex;

                if (i == edge_len || next_v >= 0) {
                    ev.fanBaseIdx = -1;
                } else {
                    ev.fanBaseIdx = static_cast<int16_t>(fanBaseVerts.size());
                    int fbVertIdx = mapElem.fanBaseVertex;
                    if (fbVertIdx < static_cast<int>(vb.size())) {
                        const auto& fb_data = vb[fbVertIdx];
                        FanBaseVertRaw fb;
                        fb.x = fb_data.x;
                        fb.y = fb_data.y;
                        fb.z = fb_data.z;
                        fb.normal = fb_data.normal;
                        fb.u = static_cast<uint8_t>(fb_data.u);
                        fb.v = static_cast<uint8_t>(fb_data.v);
                        fb.gridIdx = static_cast<uint16_t>(fbVertIdx);
                        fanBaseVerts.push_back(fb);
                    }
                }
                evs.push_back(ev);
            }
            cur_v = next_v;
            cur_x += step_x;
            cur_y += step_y;
        }

        // Write 5-byte strip header
        size_t off = edgeStripsBytes.size();
        edgeStripsBytes.resize(off + 5);
        std::memcpy(edgeStripsBytes.data() + off + 0, &startPos, 2);
        std::memcpy(edgeStripsBytes.data() + off + 2, &size, 2);
        edgeStripsBytes[off + 4] = vertical;

        // Array 0 (Verts): 20 bytes per vertex, range compressed
        int32_t count0 = static_cast<int32_t>(evs.size());
        size_t c0Pos = edgeStripsBytes.size();
        edgeStripsBytes.resize(c0Pos + 4);
        std::memcpy(edgeStripsBytes.data() + c0Pos, &count0, 4);

        if (count0 > 0) {
            auto compVerts = RangeCompressor::compress(evs.data(), count0, 20, settings.enableRangeCompression);
            int32_t compLen = static_cast<int32_t>(compVerts.size());
            size_t dataPos = edgeStripsBytes.size();
            edgeStripsBytes.resize(dataPos + 4 + compLen);
            std::memcpy(edgeStripsBytes.data() + dataPos, &compLen, 4);
            std::memcpy(edgeStripsBytes.data() + dataPos + 4, compVerts.data(), compLen);
        }

        // Array 1 (FanBaseVerts): 16 bytes per vertex, range compressed if count > 0
        int32_t count1 = static_cast<int32_t>(fanBaseVerts.size());
        size_t c1Pos = edgeStripsBytes.size();
        edgeStripsBytes.resize(c1Pos + 4);
        std::memcpy(edgeStripsBytes.data() + c1Pos, &count1, 4);

        if (count1 > 0) {
            auto compFan = RangeCompressor::compress(fanBaseVerts.data(), count1, 16, settings.enableRangeCompression);
            int32_t compLen = static_cast<int32_t>(compFan.size());
            size_t dataPos = edgeStripsBytes.size();
            edgeStripsBytes.resize(dataPos + 4 + compLen);
            std::memcpy(edgeStripsBytes.data() + dataPos, &compLen, 4);
            std::memcpy(edgeStripsBytes.data() + dataPos + 4, compFan.data(), compLen);
        }

        // Arrays 2 and 3: WaterVerts and WaterFanBaseVerts (CLandscapeBackgroundPatch::BuildEdgeStrips 0x1246c90)
        std::vector<EdgeWaterVertRaw> waterEvs;
        std::vector<EdgeWaterFanBaseVertRaw> waterFanBaseVerts;

        if (areaHasWater && !temp_water_verts.empty()) {
            for (size_t i = 0; i < evs.size(); ++i) {
                int cur_v = evs[i].gridIdx;
                if (cur_v >= 0 && cur_v < static_cast<int>(landscape_to_water_vertex_mapper.size())) {
                    int wvi = landscape_to_water_vertex_mapper[cur_v];
                    if (wvi >= 0) {
                        EdgeWaterVertRaw wev;
                        wev.z = temp_water_verts[wvi].z;
                        std::memcpy(wev.shoreLookup, temp_water_verts[wvi].shoreLookup, 48);
                        wev.waterVertexIndex = static_cast<uint16_t>(wvi);
                        wev.landscapeTargetVertex = static_cast<uint16_t>(i);

                        int edgeIdx = evs[i].edgeIdx;
                        if (edgeIdx >= 0 && edgeIdx < static_cast<int>(landscape_to_water_index_mapper.size()) && landscape_to_water_index_mapper[edgeIdx] >= 0) {
                            wev.waterIndexBufferIndex = static_cast<uint16_t>(landscape_to_water_index_mapper[edgeIdx]);
                        } else {
                            wev.waterIndexBufferIndex = 0xFFFF;
                        }

                        if (evs[i].fanBaseIdx == -1) {
                            wev.fanBaseIndex = -1;
                        } else {
                            wev.fanBaseIndex = static_cast<int16_t>(waterFanBaseVerts.size());
                            EdgeWaterFanBaseVertRaw wfb;
                            int fbGridIdx = fanBaseVerts[evs[i].fanBaseIdx].gridIdx;
                            int fbWvi = (fbGridIdx >= 0 && fbGridIdx < static_cast<int>(landscape_to_water_vertex_mapper.size()))
                                        ? landscape_to_water_vertex_mapper[fbGridIdx] : -1;
                            wfb.waterVertexIndex = (fbWvi >= 0) ? static_cast<uint16_t>(fbWvi) : 0xFFFF;
                            wfb.padding = 0;
                            if (fbWvi >= 0) {
                                wfb.z = temp_water_verts[fbWvi].z;
                                std::memcpy(wfb.shoreLookup, temp_water_verts[fbWvi].shoreLookup, 48);
                            }
                            waterFanBaseVerts.push_back(wfb);
                        }
                        waterEvs.push_back(wev);
                    }
                }
            }
        }

        // Array 2 (WaterVerts): 60 bytes per vertex, range compressed if count > 0
        int32_t count2 = static_cast<int32_t>(waterEvs.size());
        size_t c2Pos = edgeStripsBytes.size();
        edgeStripsBytes.resize(c2Pos + 4);
        std::memcpy(edgeStripsBytes.data() + c2Pos, &count2, 4);

        if (count2 > 0) {
            auto compWaterVerts = RangeCompressor::compress(waterEvs.data(), count2, 60, settings.enableRangeCompression);
            int32_t compLen = static_cast<int32_t>(compWaterVerts.size());
            size_t dataPos = edgeStripsBytes.size();
            edgeStripsBytes.resize(dataPos + 4 + compLen);
            std::memcpy(edgeStripsBytes.data() + dataPos, &compLen, 4);
            std::memcpy(edgeStripsBytes.data() + dataPos + 4, compWaterVerts.data(), compLen);
        }

        // Array 3 (WaterFanBaseVerts): 56 bytes per vertex, range compressed if count > 0
        int32_t count3 = static_cast<int32_t>(waterFanBaseVerts.size());
        size_t c3Pos = edgeStripsBytes.size();
        edgeStripsBytes.resize(c3Pos + 4);
        std::memcpy(edgeStripsBytes.data() + c3Pos, &count3, 4);

        if (count3 > 0) {
            auto compWaterFan = RangeCompressor::compress(waterFanBaseVerts.data(), count3, 56, settings.enableRangeCompression);
            int32_t compLen = static_cast<int32_t>(compWaterFan.size());
            size_t dataPos = edgeStripsBytes.size();
            edgeStripsBytes.resize(dataPos + 4 + compLen);
            std::memcpy(edgeStripsBytes.data() + dataPos, &compLen, 4);
            std::memcpy(edgeStripsBytes.data() + dataPos + 4, compWaterFan.data(), compLen);
        }
    }

    // 6. Range compress VB and IB (IB only for unshared bands > 1)
    auto compVB = RangeCompressor::compress(vb.data(), vertexCount, 16, settings.enableRangeCompression);
    std::vector<uint8_t> compIB;
    if (lodBand > 1 && !ib.empty()) {
        compIB = RangeCompressor::compress(ib.data(), static_cast<int>(ib.size()), 2, settings.enableRangeCompression);
    }

    // 7. Assemble uncompressed payload
    std::vector<uint8_t> payload;

    // 17-byte Header
    payload.resize(17);
    uint16_t pw = static_cast<uint16_t>(patchW);
    uint16_t ph = static_cast<uint16_t>(patchH);
    uint16_t mx = static_cast<uint16_t>(mapX);
    uint16_t my = static_cast<uint16_t>(mapY);
    uint8_t isWaterOnly = 0;
    uint8_t band = static_cast<uint8_t>(lodBand);
    uint16_t vc = static_cast<uint16_t>(vertexCount);
    uint8_t tw = static_cast<uint8_t>(texW);
    uint8_t th = static_cast<uint8_t>(texH);
    uint8_t isCompressed = 1;

    std::memcpy(payload.data() + 0, &pw, 2);
    std::memcpy(payload.data() + 2, &ph, 2);
    std::memcpy(payload.data() + 4, &mx, 2);
    std::memcpy(payload.data() + 6, &my, 2);
    payload[8] = isWaterOnly;
    payload[9] = band;
    std::memcpy(payload.data() + 10, &polyCount, 2);
    std::memcpy(payload.data() + 12, &vc, 2);
    payload[14] = tw;
    payload[15] = th;
    payload[16] = isCompressed;

    // Append Texture
    payload.insert(payload.end(), textureBytes.begin(), textureBytes.end());

    // Append VB: 4 bytes length + compressed bytes
    int32_t vbLen = static_cast<int32_t>(compVB.size());
    size_t vbPos = payload.size();
    payload.resize(vbPos + 4 + vbLen);
    std::memcpy(payload.data() + vbPos, &vbLen, 4);
    std::memcpy(payload.data() + vbPos + 4, compVB.data(), vbLen);

    // Append IB only if unshared band (lodBand > 1)
    if (lodBand > 1 && !compIB.empty()) {
        int32_t ibLen = static_cast<int32_t>(compIB.size());
        size_t ibPos = payload.size();
        payload.resize(ibPos + 4 + ibLen);
        std::memcpy(payload.data() + ibPos, &ibLen, 4);
        std::memcpy(payload.data() + ibPos + 4, compIB.data(), ibLen);
    }

    // Append Edge Strips
    payload.insert(payload.end(), edgeStripsBytes.begin(), edgeStripsBytes.end());

    // Water subpatch (CEngineWaterBackgroundSubPatch::Save 0x12cbda0)
    if (areaHasWater && !temp_water_verts.empty() && dominantWaterType > 0 && dominantWaterType != 7) {
        uint16_t wVertCount = static_cast<uint16_t>(temp_water_verts.size());
        uint16_t wPolyCount = static_cast<uint16_t>(water_indices.size() / 3);
        int32_t wType = dominantWaterType;
        int32_t wStride = (dominantWaterType == 3 || dominantWaterType == 4 || dominantWaterType == 5) ? 12 : 56;

        std::vector<uint8_t> compVb;
        if (wStride == 12) {
            std::vector<BgSeaVertexRaw> sea_verts(wVertCount);
            for (size_t v = 0; v < temp_water_verts.size(); ++v) {
                sea_verts[v].x = static_cast<float>(temp_water_verts[v].x);
                sea_verts[v].y = static_cast<float>(temp_water_verts[v].y);
                sea_verts[v].z = temp_water_verts[v].z;
            }
            compVb = RangeCompressor::compress(sea_verts.data(), wVertCount, 12, settings.enableRangeCompression);
        } else {
            compVb = RangeCompressor::compress(temp_water_verts.data(), wVertCount, 56, settings.enableRangeCompression);
        }

        std::vector<uint8_t> compIb;
        if (wPolyCount > 0) {
            compIb = RangeCompressor::compress(water_indices.data(), static_cast<int>(water_indices.size()), 2, settings.enableRangeCompression);
        }

        payload.push_back(1); // Water flag = 1
        int32_t compVbLen = static_cast<int32_t>(compVb.size());
        size_t wvbPos = payload.size();
        payload.resize(wvbPos + 16 + compVbLen);
        std::memcpy(payload.data() + wvbPos + 0, &wVertCount, 2);
        std::memcpy(payload.data() + wvbPos + 2, &wPolyCount, 2);
        std::memcpy(payload.data() + wvbPos + 4, &wType, 4);
        std::memcpy(payload.data() + wvbPos + 8, &wStride, 4);
        std::memcpy(payload.data() + wvbPos + 12, &compVbLen, 4);
        std::memcpy(payload.data() + wvbPos + 16, compVb.data(), compVbLen);

        if (wPolyCount > 0) {
            int32_t compIbLen = static_cast<int32_t>(compIb.size());
            size_t wibPos = payload.size();
            payload.resize(wibPos + 4 + compIbLen);
            std::memcpy(payload.data() + wibPos + 0, &compIbLen, 4);
            std::memcpy(payload.data() + wibPos + 4, compIb.data(), compIbLen);
        }
    } else {
        payload.push_back(0); // Water flag = 0
    }

    // 8. LZO Compress the entire frame
    auto lzoData = albion::lzo1x::compress(payload.data(), payload.size());

    // Output frame: uLen (4) + cLen (4) + LZO data
    std::vector<uint8_t> frame(8 + lzoData.size());
    uint32_t uLen = static_cast<uint32_t>(payload.size());
    uint32_t cLen = static_cast<uint32_t>(lzoData.size());
    std::memcpy(frame.data() + 0, &uLen, 4);
    std::memcpy(frame.data() + 4, &cLen, 4);
    std::memcpy(frame.data() + 8, lzoData.data(), lzoData.size());

    return frame;
}

void BackgroundTreeBuilder::generateNodePatches(
    BgTreeNode* node,
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    const EngineLandscapeLODMap* pLodMap,
    const std::vector<WorldWaterNeighbor>* neighbors,
    const BgTreeSettings& settings
) {
    if (!node) return;

    WaterShoreCalculator waterContext;
    waterContext.init(lev, worldX, worldY, neighbors);

    struct PatchBuildTask {
        BgLodInfo* lodPtr = nullptr;
        int mapX = 0, mapY = 0, width = 0, height = 0, band = 0, texW = 0, texH = 0;
    };

    std::vector<PatchBuildTask> tasks;

    std::function<void(BgTreeNode*)> setupMetadataAndCollectTasks = [&](BgTreeNode* n) {
        if (!n) return;

        if (n->firstNonSplitBand < 8 && n->lastBand >= n->firstNonSplitBand) {
            int totalBands = n->lastBand - n->firstNonSplitBand + 1;
            n->lodInfo.clear();
            n->lodInfo.reserve(totalBands);

            int texScale = std::max(1, settings.textureScale);
            uint8_t lastSavedBand = 0;
            for (int band = n->firstNonSplitBand; band <= n->lastBand; ++band) {
                int texW = 128 * texScale;
                int texH = 128 * texScale;
                if (band == 1) { texW = 64 * texScale; texH = 64 * texScale; }
                else if (band == 2 || band == 3) { texW = 128 * texScale; texH = 128 * texScale; }
                else if (band == 7) { texW = 64 * texScale; texH = 32 * texScale; }

                BgLodInfo lod;
                if (settings.shareLodBands && lastSavedBand != 0 && (band == 4 || band == 5 || band == 6)) {
                    lod.optimizedBandRemap = lastSavedBand;
                    n->lodInfo.push_back(std::move(lod));
                } else {
                    lastSavedBand = static_cast<uint8_t>(band);
                    lod.optimizedBandRemap = lastSavedBand;
                    n->lodInfo.push_back(std::move(lod));
                    tasks.push_back({
                        &n->lodInfo.back(),
                        n->mapX, n->mapY, n->width, n->height,
                        band, texW, texH
                    });
                }
            }
        }

        if (n->leftChild) {
            setupMetadataAndCollectTasks(n->leftChild.get());
        }
        if (n->rightChild) {
            setupMetadataAndCollectTasks(n->rightChild.get());
        }
    };

    setupMetadataAndCollectTasks(node);

    if (!tasks.empty()) {
        std::atomic<size_t> nextTask{0};
        unsigned int numThreads = std::min(
            std::max(1u, std::thread::hardware_concurrency()),
            static_cast<unsigned int>(tasks.size())
        );
        std::vector<std::thread> workers;
        workers.reserve(numThreads);
        for (unsigned int t = 0; t < numThreads; ++t) {
            workers.emplace_back([&]() {
                while (true) {
                    size_t idx = nextTask.fetch_add(1);
                    if (idx >= tasks.size()) break;
                    const auto& task = tasks[idx];
                    task.lodPtr->compressedFrame = createPatchFrame(
                        lev, materials, worldX, worldY,
                        task.mapX, task.mapY, task.width, task.height,
                        task.band, task.texW, task.texH, pLodMap, &waterContext,
                        settings
                    );
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
    }
}


BgTreeBuildResult BackgroundTreeBuilder::build(
    const LevData& lev,
    const std::vector<ThemeMaterial>& materials,
    int worldX, int worldY,
    size_t currentPayloadPos,
    const std::vector<ObservingMapBounds>* observingMaps,
    const std::vector<WorldWaterNeighbor>* neighbors,
    const BgTreeSettings& settings
) {
    BgTreeBuildResult result;

    // 1. Build hierarchy
    auto grid = buildTopologyGrid(lev, 0, 0, lev.width, lev.height);
    auto root = buildTreeSubdivideIntoPowersOf2(
        grid, lev, 0, 0, lev.width, lev.height,
        settings.maxLodBand, settings.meshDetail, 256, observingMaps, worldX, worldY,
        settings.lodDistanceScale
    );

    // 2. Compute bounding boxes
    computeBoundingBoxes(root.get(), lev, worldX, worldY);
    apply16x16BoundingBoxCopy(root.get());

    // 3. Build LOD map once
    EngineLandscapeLODMap lodMap;
    lodMap.build(lev, settings.meshDetail, settings.lodDistanceScale);

    // 4. Generate patches for all nodes
    generateNodePatches(root.get(), lev, materials, worldX, worldY, &lodMap, neighbors, settings);


    // Collect all nodes into a flat list
    std::vector<BgTreeNode*> allNodes;
    std::vector<BgLodInfo*> allLods;

    std::function<void(BgTreeNode*)> collectNodes = [&](BgTreeNode* n) {
        if (!n) return;
        allNodes.push_back(n);
        for (auto& lod : n->lodInfo) {
            allLods.push_back(&lod);
        }
        collectNodes(n->leftChild.get());
        collectNodes(n->rightChild.get());
    };
    collectNodes(root.get());

    result.totalNodes = static_cast<int>(allNodes.size());
    result.totalPatches = static_cast<int>(allLods.size());

    // 4. Block 0 size: 9881 bytes for 135 nodes
    size_t estBlock0Size = 0;
    for (auto* n : allNodes) {
        estBlock0Size += n->headerSize();
    }

    uint32_t patchTreePos = static_cast<uint32_t>(alignUp(currentPayloadPos, 2048));
    result.patchTreePos = patchTreePos;

    // 5. Partition LOD patches using retail AssignFileBlocks (STBfunctions24.txt:1-139)
    struct BgFileBlockSaveMap {
        std::set<BgTreeNode*> treeNodeBlocks;
        std::array<std::set<BgTreeNode*>, 8> patchBlocks; // 1..7
    };

    struct BgAccumResult {
        int treeNodeCount = 122; // sizeof(CLandscapeBackgroundTreeNode serialization)
        std::array<int, 8> patchSizes = {0};
    };

    std::function<BgAccumResult(BgTreeNode*, bool, BgFileBlockSaveMap&)> assignFileBlocks =
        [&](BgTreeNode* node, bool isRoot, BgFileBlockSaveMap& blockMap) -> BgAccumResult {
        BgAccumResult res;
        res.treeNodeCount = 122;

        if (node->leftChild && node->rightChild) {
            auto sub1 = assignFileBlocks(node->leftChild.get(), false, blockMap);
            auto sub2 = assignFileBlocks(node->rightChild.get(), false, blockMap);

            res.treeNodeCount += sub1.treeNodeCount + sub2.treeNodeCount;
            for (int b = 1; b <= 7; ++b) {
                res.patchSizes[b] += sub1.patchSizes[b] + sub2.patchSizes[b];
            }
        }

        // Add this node's own patches
        for (const auto& lod : node->lodInfo) {
            if (!lod.compressedFrame.empty()) {
                int b = lod.optimizedBandRemap;
                if (b >= 1 && b <= 7) {
                    res.patchSizes[b] += static_cast<int>(lod.compressedFrame.size());
                }
            }
        }

        const int kMinBlockSize = 0x8000; // 32768 (32 KB)

        if (res.treeNodeCount > kMinBlockSize || isRoot) {
            blockMap.treeNodeBlocks.insert(node);
            res.treeNodeCount = 0;
        }

        for (int b = 1; b <= 7; ++b) {
            if (res.patchSizes[b] > kMinBlockSize || isRoot) {
                blockMap.patchBlocks[b].insert(node);
                res.patchSizes[b] = 0;
            }
        }

        return res;
    };

    BgFileBlockSaveMap fileBlockMap;
    assignFileBlocks(root.get(), true, fileBlockMap);

    std::function<void(BgTreeNode*, int, const std::set<BgTreeNode*>&, BgTreeNode*, std::vector<BgLodInfo*>&)>
        collectPatchesForBlock = [&](BgTreeNode* node, int band, const std::set<BgTreeNode*>& bandBlockRoots,
                                     BgTreeNode* curRoot, std::vector<BgLodInfo*>& outLods) {
        if (!node) return;
        if (node != curRoot && bandBlockRoots.count(node)) {
            return;
        }
        for (auto& lod : node->lodInfo) {
            if (lod.optimizedBandRemap == band && !lod.compressedFrame.empty()) {
                outLods.push_back(&lod);
            }
        }
        if (node->leftChild) collectPatchesForBlock(node->leftChild.get(), band, bandBlockRoots, curRoot, outLods);
        if (node->rightChild) collectPatchesForBlock(node->rightChild.get(), band, bandBlockRoots, curRoot, outLods);
    };

    struct DynamicBlock {
        uint32_t startPos = 0;
        uint32_t blockSize = 0;
        std::vector<uint8_t> data;
        std::vector<BgLodInfo*> lods;
        std::vector<size_t> offsets;
    };

    std::vector<DynamicBlock> patchBlocks;

    std::function<void(BgTreeNode*)> saveChildFileBlocks = [&](BgTreeNode* node) {
        if (!node) return;
        // 1. Bands 1..7 for this node (STBfunctions13.txt:250-272)
        for (int b = 1; b <= 7; ++b) {
            if (fileBlockMap.patchBlocks[b].count(node)) {
                std::vector<BgLodInfo*> lods;
                collectPatchesForBlock(node, b, fileBlockMap.patchBlocks[b], node, lods);
                if (!lods.empty()) {
                    DynamicBlock pb;
                    for (auto* lod : lods) {
                        pb.offsets.push_back(pb.data.size());
                        pb.lods.push_back(lod);
                        pb.data.insert(pb.data.end(), lod->compressedFrame.begin(), lod->compressedFrame.end());
                    }
                    patchBlocks.push_back(std::move(pb));
                }
            }
        }
        // 2. Recurse into children in tree order
        if (node->leftChild) saveChildFileBlocks(node->leftChild.get());
        if (node->rightChild) saveChildFileBlocks(node->rightChild.get());
    };

    saveChildFileBlocks(root.get());

    size_t block0Aligned = alignUp(estBlock0Size, 2048);
    uint32_t curBlockPos = patchTreePos + static_cast<uint32_t>(block0Aligned);

    for (auto& pb : patchBlocks) {
        curBlockPos = static_cast<uint32_t>(alignUp(curBlockPos, 2048));
        pb.startPos = curBlockPos;
        pb.blockSize = static_cast<uint32_t>(pb.data.size());
        for (size_t i = 0; i < pb.lods.size(); ++i) {
            pb.lods[i]->fileBlockPos = pb.startPos;
            pb.lods[i]->fileBlockSize = pb.blockSize;
            pb.lods[i]->offsetIntoFileBlock = static_cast<int32_t>(pb.offsets[i]);
        }
        curBlockPos += pb.blockSize;
    }

    result.totalBlocks = 1 + static_cast<int>(patchBlocks.size());

    // 6. Serialize Block 0: Tree Node Headers in Post-Order
    std::vector<uint8_t> block0;
    root->fileBlockPos = patchTreePos;
    root->fileBlockSize = static_cast<int32_t>(estBlock0Size);
    root->serializeHeader(block0);

    std::function<void(BgTreeNode*)> serializeChildren = [&](BgTreeNode* parent) {
        if (!parent || parent->isLeaf()) return;

        serializeChildren(parent->leftChild.get());
        serializeChildren(parent->rightChild.get());

        parent->fileBlockPos = patchTreePos;
        parent->fileBlockSize = static_cast<int32_t>(estBlock0Size);
        parent->offsetIntoFileBlock = static_cast<int32_t>(block0.size());

        parent->leftChild->serializeHeader(block0);
        parent->rightChild->serializeHeader(block0);
    };

    serializeChildren(root.get());

    uint32_t actualBlock0Size = static_cast<uint32_t>(block0.size());
    result.patchTreeSize = actualBlock0Size;

    // Rewrite root header
    std::vector<uint8_t> finalRootHeader;
    root->fileBlockSize = actualBlock0Size;
    root->serializeHeader(finalRootHeader);
    std::memcpy(block0.data(), finalRootHeader.data(), finalRootHeader.size());

    // Pad Block 0 to 2048 (from 9881 to 10240, so next pos is 221184)
    size_t finalBlock0Aligned = alignUp(block0.size(), 2048);
    block0.resize(finalBlock0Aligned, 0);

    // 7. Combine Block 0 and all 23 patch blocks into final stream
    size_t leadingPadding = (patchTreePos > currentPayloadPos) ? (patchTreePos - currentPayloadPos) : 0;
    result.treeAndPatchData.resize(leadingPadding, 0);
    result.treeAndPatchData.insert(result.treeAndPatchData.end(), block0.begin(), block0.end());

    for (const auto& pb : patchBlocks) {
        size_t currentLen = currentPayloadPos + result.treeAndPatchData.size();
        size_t pad = (pb.startPos > currentLen) ? (pb.startPos - currentLen) : 0;
        if (pad > 0) {
            result.treeAndPatchData.insert(result.treeAndPatchData.end(), pad, 0);
        }
        result.treeAndPatchData.insert(result.treeAndPatchData.end(), pb.data.begin(), pb.data.end());
    }

    // Final 2048-alignment padding (ends at 1185146 -> pads to 1185792)
    size_t totalFinalSize = currentPayloadPos + result.treeAndPatchData.size();
    size_t finalAligned = alignUp(totalFinalSize, 2048);
    if (finalAligned > totalFinalSize) {
        result.treeAndPatchData.insert(result.treeAndPatchData.end(), finalAligned - totalFinalSize, 0);
    }

    Dxt1Baker::clearCache();
    return result;
}


} // namespace egostb
