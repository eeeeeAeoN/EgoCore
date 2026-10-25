#pragma once

#include "egostb/lev_reader.hpp"
#include "egostb/theme_manager.hpp"
#include "egostb/water_utils.hpp"
#include <cstdint>
#include <vector>
#include <memory>

namespace egostb {

struct BgAABB {
    float minX = 0, minY = 0, minZ = 0;
    float maxX = 0, maxY = 0, maxZ = 0;
};

struct BgLodInfo {
    uint8_t optimizedBandRemap = 0;
    int32_t fileBlockPos = 0;
    int32_t fileBlockSize = 0;
    int32_t offsetIntoFileBlock = 0;
    std::vector<uint8_t> compressedFrame; // uLen (4) + cLen (4) + LZO data
};

struct BgTreeNode {
    uint16_t mapX = 0;
    uint16_t mapY = 0;
    uint16_t width = 0;
    uint16_t height = 0;

    uint8_t firstBand = 1;
    uint8_t firstNonSplitBand = 1;
    uint8_t lastBand = 1;

    int32_t fileBlockPos = 0;
    int32_t fileBlockSize = 0;
    int32_t offsetIntoFileBlock = 0;

    BgAABB aabb;

    std::vector<BgLodInfo> lodInfo; // from firstNonSplitBand to lastBand

    std::unique_ptr<BgTreeNode> leftChild;
    std::unique_ptr<BgTreeNode> rightChild;

    bool isLeaf() const { return !leftChild && !rightChild; }
    size_t headerSize() const;
    void serializeHeader(std::vector<uint8_t>& out) const;
};

struct BgTreeBuildResult {
    std::vector<uint8_t> treeAndPatchData; // aligned data to append to payload
    uint32_t patchTreePos = 0;             // file pos of Block 0 (tree node headers)
    uint32_t patchTreeSize = 0;            // size of Block 0
    int totalNodes = 0;
    int totalPatches = 0;
    int totalBlocks = 0;
};

#ifndef EGOSTB_OBSERVING_MAP_BOUNDS_DEFINED
#define EGOSTB_OBSERVING_MAP_BOUNDS_DEFINED
struct ObservingMapBounds {
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
};

template<typename = void>
inline float findShortestDistanceToObservingMapSet(
    float boxMinX, float boxMinY, float boxMinZ,
    float boxMaxX, float boxMaxY, float boxMaxZ,
    const std::vector<ObservingMapBounds>& obs) {
    if (obs.empty()) return 1.0e30f;
    float cx = (boxMinX + boxMaxX) * 0.5f;
    float cy = (boxMinY + boxMaxY) * 0.5f;
    float cz = (boxMinZ + boxMaxZ) * 0.5f;
    float minDist = 1.0e30f;
    for (const auto& m : obs) {
        float p0x = std::clamp(cx, m.minX, m.maxX);
        float p0y = std::clamp(cy, m.minY, m.maxY);
        float p0z = std::clamp(cz, m.minZ, m.maxZ);
        float p1x = std::clamp(p0x, boxMinX, boxMaxX);
        float p1y = std::clamp(p0y, boxMinY, boxMaxY);
        float p1z = std::clamp(p0z, boxMinZ, boxMaxZ);
        float dx = p0x - p1x;
        float dy = p0y - p1y;
        float dz = p0z - p1z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < minDist) minDist = dist;
    }
    return minDist;
}

template<typename = void>
inline float findFurthestDistanceToObservingMapSet(
    float boxMinX, float boxMinY, float boxMinZ,
    float boxMaxX, float boxMaxY, float boxMaxZ,
    const std::vector<ObservingMapBounds>& obs) {
    if (obs.empty()) return 0.0f;
    float maxDist = 0.0f;
    for (const auto& m : obs) {
        float mcx = (m.minX + m.maxX) * 0.5f;
        float mcy = (m.minY + m.maxY) * 0.5f;
        float mcz = (m.minZ + m.maxZ) * 0.5f;
        float p0x = (std::fabs(mcx - boxMaxX) >= std::fabs(mcx - boxMinX)) ? boxMaxX : boxMinX;
        float p0y = (std::fabs(mcy - boxMaxY) >= std::fabs(mcy - boxMinY)) ? boxMaxY : boxMinY;
        float p0z = (std::fabs(mcz - boxMaxZ) >= std::fabs(mcz - boxMinZ)) ? boxMaxZ : boxMinZ;
        float p1x = (std::fabs(p0x - m.maxX) >= std::fabs(p0x - m.minX)) ? m.maxX : m.minX;
        float p1y = (std::fabs(p0y - m.maxY) >= std::fabs(p0y - m.minY)) ? m.maxY : m.minY;
        float p1z = (std::fabs(p0z - m.maxZ) >= std::fabs(p0z - m.minZ)) ? m.maxZ : m.minZ;
        float dx = p0x - p1x;
        float dy = p0y - p1y;
        float dz = p0z - p1z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > maxDist) maxDist = dist;
    }
    return maxDist;
}
#endif

struct BgTreeSettings {
    float meshDetail = 2.0f;
    float lodDistanceScale = 1.0f;
    int textureScale = 1;
    bool generateMipmaps = false;
    bool highQualityDxt = false;
    bool shareLodBands = true;
    int maxLodBand = 7;
    bool enableRangeCompression = true;
};

class BackgroundTreeBuilder {
public:
    // Builds the full background tree hierarchy, generates downsampled LOD patches,
    // bakes procedural DXT1 textures, range-compresses vertices and index buffers,
    // organizes into 2048-byte aligned file blocks, and serializes the complete tree stream.
    static BgTreeBuildResult build(
        const LevData& lev,
        const std::vector<ThemeMaterial>& materials,
        int worldX, int worldY,
        size_t currentPayloadPos,
        const std::vector<ObservingMapBounds>* observingMaps = nullptr,
        const std::vector<WorldWaterNeighbor>* neighbors = nullptr,
        const BgTreeSettings& settings = BgTreeSettings{}
    );

    struct TopologyGrid {
        int mapX = 0, mapY = 0, width = 0, height = 0;
        std::vector<float> unitSizeHor;
        std::vector<float> unitSizeVert;
    };

    static TopologyGrid buildTopologyGrid(
        const LevData& lev, int mapX, int mapY, int width, int height
    );

    static std::unique_ptr<BgTreeNode> buildTreeSubdivideIntoPowersOf2(
        const TopologyGrid& grid,
        const LevData& lev,
        int mapX, int mapY, int width, int height,
        int maxLodBand, float textureDetail, int maxPatchSize,
        const std::vector<ObservingMapBounds>* observingMaps,
        int worldX, int worldY,
        float lodDistanceScale = 1.0f
    );

    static std::unique_ptr<BgTreeNode> buildTree(
        const TopologyGrid& grid,
        const LevData& lev,
        int mapX, int mapY, int width, int height,
        int maxLodBand, float textureDetail, int maxPatchSize,
        const std::vector<ObservingMapBounds>* observingMaps,
        int worldX, int worldY,
        float lodDistanceScale = 1.0f
    );

    static std::unique_ptr<BgTreeNode> buildHierarchy(
        const TopologyGrid& grid,
        int mapX, int mapY, int width, int height,
        int minBand, int maxBand
    );

    static void computeBoundingBoxes(
        BgTreeNode* node,
        const LevData& lev,
        int worldX, int worldY
    );

    struct EngineLandscapeLODMap;

    static void generateNodePatches(
        BgTreeNode* node,
        const LevData& lev,
        const std::vector<ThemeMaterial>& materials,
        int worldX, int worldY,
        const EngineLandscapeLODMap* pLodMap = nullptr,
        const std::vector<WorldWaterNeighbor>* neighbors = nullptr,
        const BgTreeSettings& settings = BgTreeSettings{}
    );

    static std::vector<uint8_t> createPatchFrame(
        const LevData& lev,
        const std::vector<ThemeMaterial>& materials,
        int worldX, int worldY,
        int mapX, int mapY, int patchW, int patchH,
        int lodBand, int texW, int texH,
        const EngineLandscapeLODMap* pLodMap = nullptr,
        const WaterShoreCalculator* pWaterContext = nullptr,
        const BgTreeSettings& settings = BgTreeSettings{},
        const std::vector<WorldWaterNeighbor>* neighbors = nullptr
    );
};

} // namespace egostb
