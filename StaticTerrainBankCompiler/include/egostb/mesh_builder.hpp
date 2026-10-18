#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <array>
#include <functional>
#include "egostb/lev_reader.hpp"

namespace egostb {

struct WorldWaterNeighbor;
struct PrecomputedWaterGrid;

struct MaterialTuple {
    std::array<uint32_t, 3> textures{0, 0, 0}; // fgTexture, bgTexture, bumpTexture
    uint32_t textureMaxSize = 0;
    uint32_t bumpMaxSize = 0;
    float selfIllumination = 0.0f;

    bool operator==(const MaterialTuple& o) const {
        return textures == o.textures &&
               textureMaxSize == o.textureMaxSize &&
               bumpMaxSize == o.bumpMaxSize &&
               selfIllumination == o.selfIllumination;
    }
};

struct ThemeMaterial {
    bool available = false;
    MaterialTuple base;
    MaterialTuple cliff;
};

struct CompiledVertex {
    uint16_t x = 0;
    uint16_t y = 0;
    float z = 0.0f;
    uint32_t normal = 0x7FC00000;
    uint8_t blend = 255;
    uint8_t cliffU = 127;
    uint8_t cliffV = 127;
};

struct CompiledLayer {
    uint16_t vertexCount = 0;
    uint16_t polyCountIncDegenerates = 0;
    uint8_t mappingDirection = 0;
    uint32_t fgTexture = 0;
    uint32_t bgTexture = 0;
    uint32_t bumpTexture = 0;
    uint8_t usesSharedIndexBuffer = 0;
    int32_t minTextureMipMap = 0;
    int32_t minBumpMapMipMap = 0;
    float selfIllumination = 0.0f;

    std::vector<CompiledVertex> vertices;
    std::vector<uint16_t> indexBuffer; // Empty if usesSharedIndexBuffer == 1
};

struct CompiledWaterVertex {
    uint16_t x = 0;
    uint16_t y = 0;
    float z = 0.0f;
    int16_t sinWave = 0;
    int16_t cosWave = 0;
    int16_t depth = 0;
    float shoreU = 0.0f;
    float shoreDists[12] = {0.0f};
};

struct CompiledWaterPatch {
    int32_t offsetX = 0;
    int32_t offsetY = 0;
    float waterBodySpan = 0.0f;
    int32_t waterType = 4; // WATER_TYPE_REFLECTIVE_SEA
    std::vector<CompiledWaterVertex> vertices; // 289 vertices
};

struct CompiledPatch {
    uint32_t filePos = 0;
    uint32_t dataSize = 0;
    float minCoord[3] = {0, 0, 0};
    float maxCoord[3] = {0, 0, 0};
    uint32_t fgNeeded = 1;

    std::vector<CompiledLayer> layers;
    bool hasWater = false;
    CompiledWaterPatch waterPatch;

    std::vector<uint8_t> serializeUncompressed() const;
};

class MeshBuilder {
public:
    MeshBuilder(const LevData& lev, int worldX = 3072, int worldY = 2816,
                const std::vector<ThemeMaterial>& themeMaterials = {},
                const std::vector<WorldWaterNeighbor>* neighbors = nullptr,
                const PrecomputedWaterGrid* waterGrid = nullptr);

    CompiledPatch buildPatch(int patchX, int patchY);

    struct Vector3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 1.0f;
    };

    Vector3 buildMapDirMask(int gx, int gy) const;
    Vector3 mapNormal(int gx, int gy) const;
    static uint32_t packNormal(const Vector3& norm);
    static uint8_t packDirMaskByte(float component);
    bool directionActive(uint8_t direction, int gx, int gy) const;

private:
    const LevData& m_lev;
    int m_worldX;
    int m_worldY;
    std::vector<ThemeMaterial> m_themeMaterials;
    const std::vector<WorldWaterNeighbor>* m_neighbors = nullptr;
    const PrecomputedWaterGrid* m_waterGrid = nullptr;

    float peekWaterDepth(int gx, int gy) const;
    float peekInterpolatedWaterHeight(int gx, int gy, int distanceToLook = 2) const;
    bool findCorrectWaterLevel(const float heights[17][17], int x, int y, float* outHeight) const;
    int getPatchWaterType(int startX, int startY) const;

    struct ShorePoint {
        int x = 0;
        int y = 0;
    };
    struct WaterBody {
        std::vector<ShorePoint> points;
        int minX = 0, minY = 0, maxX = 0, maxY = 0;
        float span = 0.0f;
    };

    mutable bool m_shoreBodiesComputed = false;
    mutable std::vector<WaterBody> m_waterBodies;
    void ensureWaterBodies() const;
    bool constructVertexDistanceToShoreArray(float vx, float vy, float array[12], int size, float* waterBodySpan, float* shoreUCoord) const;
};

} // namespace egostb
