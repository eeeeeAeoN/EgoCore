#pragma once

#include "egostb/lev_reader.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace egostb {

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

struct LocalDetailResult {
    bool hasData = false;
    std::vector<uint8_t> headerBytes;   // Inserted into __STATIC_MAP_COMMON_HEADER__
    std::vector<uint8_t> payloadBytes;  // Appended to Entry 1 payload
    uint32_t payloadStartOffset = 0;    // Alignment offset in Entry 1
    int totalInstances = 0;
    int totalBatches = 0;
};

class LocalDetailBuilder {
public:
    static LocalDetailResult build(
        const LevData& lev,
        int worldX, int worldY,
        size_t initialPayloadOffset,
        const std::string& fableRoot = "",
        const std::string& workDir = "",
        const std::vector<ObservingMapBounds>* observingMaps = nullptr
    );
};

} // namespace egostb
