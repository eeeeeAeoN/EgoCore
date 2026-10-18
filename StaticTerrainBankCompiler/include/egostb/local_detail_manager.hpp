#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "egostb/bank_resolver.hpp"
#include "egostb/lev_reader.hpp"

namespace egostb {

#pragma pack(push, 1)
struct SerializedClutterType {
    uint32_t meshBankIndex = 0;           // 0..3
    uint32_t shadowMeshBankIndex = 0;     // 4..7
    uint32_t zSpriteMeshBankIndex = 0;    // 8..11
    // PlainDataBlock starts here (offset 12, size 52):
    float fadeStart = 0.0f;               // 12..15 (+0x00)
    float fadeEnd = 0.0f;                 // 16..19 (+0x04)
    int32_t alphaRef = 0;                 // 20..23 (+0x08)
    float alphaMipBias = 0.0f;            // 24..27 (+0x0C)
    float windSkewConstantFactor = 0.0f;  // 28..31 (+0x10)
    float windSkewRandomFactor = 0.0f;    // 32..35 (+0x14)
    float windSkewSpeedFactor = 0.0f;     // 36..39 (+0x18)
    float zSpriteFadeStart = 0.0f;        // 40..43 (+0x1C)
    float zSpriteFadeEnd = 0.0f;          // 44..47 (+0x20)
    int32_t primitiveType = 0;            // 48..51 (+0x24) (0=MESH, 1=REPEATED_MESH, 2=HYBRID_MESH_ZSPRITE)
    int32_t cacheGroup = -1;              // 52..55 (+0x28)
    uint8_t alphaIsBoolean = 0;           // 56     (+0x2C)
    uint8_t landscapeNormalLighting = 0;  // 57     (+0x2D)
    uint8_t hasWindSkew = 0;              // 58     (+0x2E)
    uint8_t castsShadows = 0;             // 59     (+0x2F)
    uint8_t receivesShadows = 0;          // 60     (+0x30)
    uint8_t padding[3] = {0, 0, 0};       // 61..63 (+0x31)
};
#pragma pack(pop)
static_assert(sizeof(SerializedClutterType) == 64, "SerializedClutterType must be 64 bytes");

struct LocalDetailObjectDef {
    std::string mesh;
    std::string shadowMesh;
    std::string zSpriteMesh;
    float scale = 1.0f;
    float scaleRandomElement = 0.0f;
    float probability = 1.0f;
    float fadeStart = 0.0f;
    float fadeEnd = 0.0f;
    float zSpriteFadeStart = 0.0f;
    float zSpriteFadeEnd = 0.0f;
    bool alphaIsBoolean = false;
    int alphaRef = -1;
    bool castShadows = false;
    bool receiveShadows = false;
    bool isRepeatedMesh = false;
    bool hasLandscapeNormalLighting = false;
    float alphaMipBias = 0.0f;
    bool hasWindSkew = false;
    float windSkewConstantFactor = 0.0f;
    float windSkewRandomFactor = 0.0f;
    float windSkewSpeedFactor = 0.0f;
    float slopeFadeStart = 0.0f;
    float slopeFadeEnd = 0.0f;
    float themeBlendThreshold = 0.0f;
    bool tiltToSlope = false;
};

struct LocalDetailLayerDef {
    std::vector<float> spacingFromLayer;
    std::vector<LocalDetailObjectDef> objects;
};

struct LocalDetailGeneratorDef {
    std::string name;
    std::vector<LocalDetailLayerDef> layers;
};

struct CacheFadeGroup {
    float startDistance = 0.0f;
    float endDistance = 0.0f;
    uint32_t primitiveTypeMask = 0;
};

struct ClutterPaletteEntry {
    std::string name;
    SerializedClutterType data;
    MeshBounds bounds;
    float baseScale = 1.0f;
    float scaleRandom = 0.0f;
    float slopeFadeStart = 0.0f;
    float slopeFadeEnd = 0.0f;
    bool tiltToSlope = false;
    int cacheGroupIndex = 0;
};

struct LocalDetailLayerRuntime {
    float spacing = 1.0f;
    std::vector<float> spacingFromLayer;
    int gridIndex = 0;
    int objectSelectionTable[32]{};
    std::vector<int> objectPalIndices;
    std::vector<LocalDetailObjectDef> objects;
};

struct LocalDetailGeneratorRuntime {
    std::string name;
    std::vector<LocalDetailLayerRuntime> layers;
};

struct BuiltClutterPalette {
    std::vector<ClutterPaletteEntry> entries;
    std::vector<CacheFadeGroup> cacheFadeGroups;
    std::unordered_map<std::string, LocalDetailGeneratorRuntime> generatorRuntimes;
};

class LocalDetailManager {
public:
    static LocalDetailManager& instance();

    bool loadFromDefFile(const std::string& path);
    const LocalDetailGeneratorDef* getGenerator(const std::string& name) const;

    float getEnumConstant(const std::string& name, float defaultVal = 0.0f) const;

    BuiltClutterPalette buildPaletteForLevel(const LevData& lev) const;

    size_t generatorCount() const { return m_generators.size(); }
    const std::vector<CacheFadeGroup>& globalCacheFadeGroups() const { return m_globalCacheFadeGroups; }
    int getCacheGroupForFadeEnd(float fadeEnd) const;

private:
    void buildGlobalCacheFadeGroups();

    std::unordered_map<std::string, float> m_enums;
    std::unordered_map<std::string, LocalDetailGeneratorDef> m_generators;
    std::vector<CacheFadeGroup> m_globalCacheFadeGroups;
};

} // namespace egostb
