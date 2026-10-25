#include "egostb/stb_compiler.hpp"
#include "egostb/bbb_archive.hpp"
#include "egostb/lzo1x.hpp"
#include "egostb/lev_reader.hpp"
#include "egostb/theme_manager.hpp"
#include "egostb/mesh_builder.hpp"
#include "egostb/bank_resolver.hpp"
#include "egostb/background_tree.hpp"
#include "egostb/local_detail_builder.hpp"
#include "egostb/local_detail_manager.hpp"
#include "egostb/ism_builder.hpp"
#include "egostb/sea_generator.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <set>
#include <map>
#include <thread>
#include <atomic>


namespace egostb {

namespace fs = std::filesystem;

#ifndef EGOSTB_OBSERVING_MAP_BOUNDS_DEFINED
#define EGOSTB_OBSERVING_MAP_BOUNDS_DEFINED
struct ObservingMapBounds {
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
};
#endif

static float findShortestDistanceToObservingMapSet(
    float minX, float minY, float minZ,
    float maxX, float maxY, float maxZ,
    const std::vector<ObservingMapBounds>& observingMaps)
{
    if (observingMaps.empty()) {
        return 1.0e30f;
    }
    float minDist = 1.0e30f;
    float boxCenterX = (minX + maxX) * 0.5f;
    float boxCenterY = (minY + maxY) * 0.5f;
    float boxCenterZ = (minZ + maxZ) * 0.5f;

    for (const auto& obs : observingMaps) {
        float p0X = boxCenterX;
        if (p0X < obs.minX) p0X = obs.minX;
        else if (p0X > obs.maxX) p0X = obs.maxX;

        float p0Y = boxCenterY;
        if (p0Y < obs.minY) p0Y = obs.minY;
        else if (p0Y > obs.maxY) p0Y = obs.maxY;

        float p0Z = boxCenterZ;
        if (p0Z < obs.minZ) p0Z = obs.minZ;
        else if (p0Z > obs.maxZ) p0Z = obs.maxZ;

        float p1X = p0X;
        if (p1X < minX) p1X = minX;
        else if (p1X > maxX) p1X = maxX;

        float p1Y = p0Y;
        if (p1Y < minY) p1Y = minY;
        else if (p1Y > maxY) p1Y = maxY;

        float p1Z = p0Z;
        if (p1Z < minZ) p1Z = minZ;
        else if (p1Z > maxZ) p1Z = maxZ;

        float dx = p0X - p1X;
        float dy = p0Y - p1Y;
        float dz = p0Z - p1Z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < minDist) {
            minDist = dist;
        }
    }
    return minDist;
}

static uint32_t morton2D(int x, int y) {
    uint32_t morton = 0;
    for (int b = 0; b < 16; ++b) {
        morton |= ((static_cast<uint32_t>(x >> b) & 1u) << (2 * b)) |
                  ((static_cast<uint32_t>(y >> b) & 1u) << (2 * b + 1));
    }
    return morton;
}

template<typename... Args>
auto invokeBgTreeBuild(const LevData& lev, const std::vector<ThemeMaterial>& materials, int worldX, int worldY, size_t pos, const std::vector<ObservingMapBounds>* obs, const std::vector<WorldWaterNeighbor>* neighbors, const BgTreeSettings& settings = BgTreeSettings{}) {
    if constexpr (std::is_invocable_v<decltype(BackgroundTreeBuilder::build), const LevData&, const std::vector<ThemeMaterial>&, int, int, size_t, const std::vector<ObservingMapBounds>*, const std::vector<WorldWaterNeighbor>*, const BgTreeSettings&>) {
        return BackgroundTreeBuilder::build(lev, materials, worldX, worldY, pos, obs, neighbors, settings);
    } else if constexpr (std::is_invocable_v<decltype(BackgroundTreeBuilder::build), const LevData&, const std::vector<ThemeMaterial>&, int, int, size_t, const std::vector<ObservingMapBounds>*, const std::vector<WorldWaterNeighbor>*>) {
        return BackgroundTreeBuilder::build(lev, materials, worldX, worldY, pos, obs, neighbors);
    } else if constexpr (std::is_invocable_v<decltype(BackgroundTreeBuilder::build), const LevData&, const std::vector<ThemeMaterial>&, int, int, size_t, const std::vector<ObservingMapBounds>*>) {
        return BackgroundTreeBuilder::build(lev, materials, worldX, worldY, pos, obs);
    } else {
        return BackgroundTreeBuilder::build(lev, materials, worldX, worldY, pos);
    }
}

template<typename... Args>
auto invokeLdBuild(const LevData& lev, int worldX, int worldY, size_t pos, const std::string& fableRoot, const std::string& workDir, const std::vector<ObservingMapBounds>* obs) {
    if constexpr (std::is_invocable_v<decltype(LocalDetailBuilder::build), const LevData&, int, int, size_t, const std::string&, const std::string&, const std::vector<ObservingMapBounds>*>) {
        return LocalDetailBuilder::build(lev, worldX, worldY, pos, fableRoot, workDir, obs);
    } else {
        return LocalDetailBuilder::build(lev, worldX, worldY, pos, fableRoot, workDir);
    }
}

static size_t alignUp(size_t val, size_t alignment) {
    return (val + alignment - 1) & ~(alignment - 1);
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n;");
    if (last < first) return "";
    return str.substr(first, (last - first + 1));
}

static std::string normalizeSeparators(const std::string& p) {
    std::string res = p;
    for (char& c : res) {
        if (c == '/') c = '\\';
    }
    return res;
}

static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static std::string cleanLevelRelPath(const std::string& levelName) {
    std::string s = normalizeSeparators(levelName);
    const std::string pfx1 = "Data\\Levels\\";
    const std::string pfx2 = "Data\\Levels/";
    if (toLower(s).rfind("data\\levels\\", 0) == 0) s = s.substr(12);
    else if (toLower(s).rfind("data/levels/", 0) == 0) s = s.substr(12);
    if (toLower(s).rfind("levels\\", 0) == 0) s = s.substr(7);
    else if (toLower(s).rfind("levels/", 0) == 0) s = s.substr(7);
    return s;
}

struct SubheaderConfig {
    uint32_t bankFileIndex = 0;
    int worldX = 0;
    int worldY = 0;
    int mapWidth = 128;
    int mapHeight = 128;
    float minHeight = 0.0f;
    float maxHeight = 20.0f;
    uint32_t levelChecksum = 0;
    bool retailMode = true;
    const std::vector<std::string>* localPalette = nullptr;
    uint32_t patchTreePos = 0;
    uint32_t patchTreeSize = 0;
    uint32_t patchesPos = 2048;
};

struct RetailMapSubheaderConfig {
    std::string canonicalName;
    uint32_t bankFileIndex = 0;
    int worldX = 0;
    int worldY = 0;
    int mapWidth = 128;
    int mapHeight = 128;
    float minHeight = 0.0f;
    float maxHeight = 20.0f;
    uint32_t levelChecksum = 0;
    uint32_t patchTreePos = 0;
    uint32_t patchTreeSize = 0;
    uint32_t edgeHeightFileSize = 0;
    uint32_t shorePointArraySize = 0;
    uint32_t shorePointArrayStart = 0;
    uint32_t checksumBlockFilePtr = 0x052BB590;
    uint32_t checksumBlockFileSize = 0x03C00000;
    uint32_t patchesPos = 0;
    size_t subheaderStart = 0;
    std::vector<uint8_t> localDetailData;
};

static void appendMapSubheaderToCommonStream(
    std::vector<uint8_t>& stream,
    const RetailMapSubheaderConfig& cfg)
{
    size_t subheaderStart = cfg.subheaderStart;
    if (stream.size() < subheaderStart) {
        stream.resize(subheaderStart, 0);
    }

    // 1. StaticMapInfoBlock (92 bytes)
    stream.resize(subheaderStart + 92, 0);
    uint8_t* infoPtr = stream.data() + subheaderStart;

    int32_t versionId = 1;
    std::memcpy(infoPtr + 0, &versionId, 4);

    int32_t bankIdx = static_cast<int32_t>(cfg.bankFileIndex);
    std::memcpy(infoPtr + 4, &bankIdx, 4);

    std::memcpy(infoPtr + 8, &cfg.edgeHeightFileSize, 4);

    int32_t mw = cfg.mapWidth, mh = cfg.mapHeight;
    std::memcpy(infoPtr + 12, &mw, 4);
    std::memcpy(infoPtr + 16, &mh, 4);

    int32_t wx = cfg.worldX, wy = cfg.worldY;
    std::memcpy(infoPtr + 20, &wx, 4);
    std::memcpy(infoPtr + 24, &wy, 4);

    int32_t edgeHeightFilePtr = 0;
    std::memcpy(infoPtr + 28, &edgeHeightFilePtr, 4);

    int32_t landscapePtr = static_cast<int32_t>(subheaderStart + 92);
    std::memcpy(infoPtr + 32, &landscapePtr, 4);

    int32_t quality = 10;
    std::memcpy(infoPtr + 40, &quality, 4);

    std::memcpy(infoPtr + 44, &cfg.shorePointArraySize, 4);
    std::memcpy(infoPtr + 48, &cfg.shorePointArrayStart, 4);

    std::memcpy(infoPtr + 52, &cfg.levelChecksum, 4);

    std::memcpy(infoPtr + 56, &cfg.checksumBlockFilePtr, 4);
    std::memcpy(infoPtr + 60, &cfg.checksumBlockFileSize, 4);

    float bounds[6] = {
        static_cast<float>(cfg.worldX),
        static_cast<float>(cfg.worldY),
        cfg.minHeight,
        static_cast<float>(cfg.worldX + cfg.mapWidth),
        static_cast<float>(cfg.worldY + cfg.mapHeight),
        cfg.maxHeight
    };
    std::memcpy(infoPtr + 64, bounds, 24);

    // 2. MapFileHeader (20 bytes) at landscapePtr
    size_t mapHeaderPos = stream.size();
    stream.resize(mapHeaderPos + 20, 0);

    int32_t texPalPos = static_cast<int32_t>(mapHeaderPos + 20);
    int32_t texPalSize = 1;
    int32_t patchesPos = static_cast<int32_t>(cfg.patchesPos);
    int32_t ptPos = static_cast<int32_t>(cfg.patchTreePos);
    int32_t ptSize = static_cast<int32_t>(cfg.patchTreeSize);

    std::memcpy(stream.data() + mapHeaderPos + 0, &texPalPos, 4);
    std::memcpy(stream.data() + mapHeaderPos + 4, &texPalSize, 4);
    std::memcpy(stream.data() + mapHeaderPos + 8, &patchesPos, 4);
    std::memcpy(stream.data() + mapHeaderPos + 12, &ptPos, 4);
    std::memcpy(stream.data() + mapHeaderPos + 16, &ptSize, 4);

    // 3. TexturePalette (1 byte)
    uint8_t retailEbool = 1;
    stream.push_back(retailEbool);

    // 4. LocalDetailMapHeader (12 bytes)
    size_t localDetailPos = stream.size();
    int32_t ldPtr = static_cast<int32_t>(localDetailPos);
    std::memcpy(stream.data() + subheaderStart + 36, &ldPtr, 4);

    int32_t ldDataPos = static_cast<int32_t>(localDetailPos + 12);
    int32_t ldDataSize = static_cast<int32_t>(cfg.localDetailData.size());
    int32_t ldHasTree = cfg.localDetailData.empty() ? 0 : 1;

    uint8_t ldHeader[12];
    std::memcpy(ldHeader + 0, &ldDataPos, 4);
    std::memcpy(ldHeader + 4, &ldDataSize, 4);
    std::memcpy(ldHeader + 8, &ldHasTree, 4);
    stream.insert(stream.end(), ldHeader, ldHeader + 12);

    if (!cfg.localDetailData.empty()) {
        stream.insert(stream.end(), cfg.localDetailData.begin(), cfg.localDetailData.end());
    }

    // 5. HeaderEndPtr
    int32_t headerEndPtr = static_cast<int32_t>(stream.size());
    std::memcpy(stream.data() + subheaderStart + 88, &headerEndPtr, 4);
}

static std::vector<uint8_t> buildSubheader(const SubheaderConfig& cfg) {
    std::vector<uint8_t> extra;

    // 1. StaticMapInfoBlock (92 bytes, offset 0..91)
    extra.resize(92, 0);

    // VersionID = 1 (offset 0)
    int32_t versionId = 1;
    std::memcpy(extra.data() + 0, &versionId, 4);

    // BankFileIndex (offset 4)
    int32_t bankIdx = static_cast<int32_t>(cfg.bankFileIndex);
    std::memcpy(extra.data() + 4, &bankIdx, 4);

    // MapWidth, MapHeight (offset 12, 16)
    int32_t mw = cfg.mapWidth, mh = cfg.mapHeight;
    std::memcpy(extra.data() + 12, &mw, 4);
    std::memcpy(extra.data() + 16, &mh, 4);

    // WorldX, WorldY (offset 20, 24)
    int32_t wx = cfg.worldX, wy = cfg.worldY;
    std::memcpy(extra.data() + 20, &wx, 4);
    std::memcpy(extra.data() + 24, &wy, 4);

    // LandscapeMapPtr = 92 (0x5C, offset 32)
    int32_t landscapePtr = 92;
    std::memcpy(extra.data() + 32, &landscapePtr, 4);

    // Quality = 10 (hardcoded retail quality, offset 40)
    int32_t quality = 10;
    std::memcpy(extra.data() + 40, &quality, 4);

    // LevelChecksum (offset 52)
    std::memcpy(extra.data() + 52, &cfg.levelChecksum, 4);

    // CameraMapBounds: minX, minY, minZ, maxX, maxY, maxZ (offset 64..87)
    float bounds[6] = {
        static_cast<float>(cfg.worldX),
        static_cast<float>(cfg.worldY),
        cfg.minHeight,
        static_cast<float>(cfg.worldX + cfg.mapWidth),
        static_cast<float>(cfg.worldY + cfg.mapHeight),
        cfg.maxHeight
    };
    std::memcpy(extra.data() + 64, bounds, 24);

    // 2. MapFileHeader (20 bytes, offset 92..111)
    size_t mapHeaderPos = 92;
    extra.resize(112, 0);

    int32_t texPalPos = 112; // 0x70
    std::memcpy(extra.data() + mapHeaderPos + 0, &texPalPos, 4);

    int32_t patchesPos = static_cast<int32_t>(cfg.patchesPos);
    std::memcpy(extra.data() + mapHeaderPos + 8, &patchesPos, 4);

    int32_t ptPos = static_cast<int32_t>(cfg.patchTreePos);
    int32_t ptSize = static_cast<int32_t>(cfg.patchTreeSize);
    std::memcpy(extra.data() + mapHeaderPos + 12, &ptPos, 4);
    std::memcpy(extra.data() + mapHeaderPos + 16, &ptSize, 4);

    // 3. Serialize TexturePalette at offset 112
    if (cfg.retailMode) {
        uint8_t ebool = 1;
        extra.push_back(ebool);
    } else {
        uint8_t ebool = 0;
        extra.push_back(ebool);
        uint32_t count = cfg.localPalette ? static_cast<uint32_t>(cfg.localPalette->size()) : 0;
        uint8_t countBytes[4];
        std::memcpy(countBytes, &count, 4);
        extra.insert(extra.end(), countBytes, countBytes + 4);
        if (cfg.localPalette) {
            for (const auto& s : *cfg.localPalette) {
                extra.insert(extra.end(), s.begin(), s.end());
                extra.push_back('\0');
            }
        }
    }
    int32_t texPalSize = static_cast<int32_t>(extra.size() - texPalPos);
    std::memcpy(extra.data() + mapHeaderPos + 4, &texPalSize, 4);

    // 4. LocalDetailMap header (12 bytes)
    int32_t localDetailPos = static_cast<int32_t>(extra.size());
    std::memcpy(extra.data() + 36, &localDetailPos, 4); // infoBlock.LocalDetailMapPtr

    int32_t ldDataPos = localDetailPos + 12;
    int32_t ldDataSize = 0;
    int32_t ldHasTree = 0;
    uint8_t ldHeader[12];
    std::memcpy(ldHeader + 0, &ldDataPos, 4);
    std::memcpy(ldHeader + 4, &ldDataSize, 4);
    std::memcpy(ldHeader + 8, &ldHasTree, 4);
    extra.insert(extra.end(), ldHeader, ldHeader + 12);

    // 5. HeaderEndPtr
    int32_t headerEndPtr = static_cast<int32_t>(extra.size());
    std::memcpy(extra.data() + 88, &headerEndPtr, 4); // infoBlock.HeaderEndPtr

    return extra;
}

struct WldMapEntry {
    int id = 0;
    int mapX = 0;
    int mapY = 0;
    int mapUID = 0;
    std::string levelName;
    bool isSea = false;
    bool loadedOnPlayerProximity = true;
};

struct WldRegionEntry {
    int id = 0;
    std::string regionName;
    std::string regionDef;
    std::vector<std::string> containsMaps;
    std::vector<std::string> seesMaps;
    std::vector<int> containsMapIndices; // 1-based map index (1..numMaps)
    std::vector<int> seesMapIndices;     // 1-based map index (1..numMaps)
};

struct WldData {
    std::vector<WldMapEntry> maps;
    std::vector<WldRegionEntry> regions;
};

static std::string stripQuotesAndSemicolon(const std::string& str) {
    std::string s = trim(str);
    while (!s.empty() && (s.back() == ';' || s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
    s = trim(s);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
    return s;
}

static WldData parseWldFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open WLD file: " + path);
    }

    WldData data;
    std::string line;
    bool inMap = false;
    bool inRegion = false;
    WldMapEntry currentMap;
    WldRegionEntry currentRegion;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') continue;

        std::istringstream iss(trimmed);
        std::string token;
        iss >> token;
        if (!token.empty() && token.back() == ';') {
            token.pop_back();
        }

        if (token == "NewMap") {
            inMap = true;
            inRegion = false;
            currentMap = WldMapEntry();
            std::string idStr;
            if (iss >> idStr) {
                try { currentMap.id = std::stoi(stripQuotesAndSemicolon(idStr)); } catch (...) {}
            }
        } else if (token == "EndMap") {
            if (inMap) {
                data.maps.push_back(currentMap);
                inMap = false;
            }
        } else if (token == "NewRegion") {
            inRegion = true;
            inMap = false;
            currentRegion = WldRegionEntry();
            std::string idStr;
            if (iss >> idStr) {
                try { currentRegion.id = std::stoi(stripQuotesAndSemicolon(idStr)); } catch (...) {}
            }
        } else if (token == "EndRegion") {
            if (inRegion) {
                data.regions.push_back(currentRegion);
                inRegion = false;
            }
        } else if (inMap) {
            std::string valStr;
            std::getline(iss, valStr);
            valStr = stripQuotesAndSemicolon(valStr);

            if (token == "MapX") {
                try { currentMap.mapX = std::stoi(valStr); } catch (...) {}
            } else if (token == "MapY") {
                try { currentMap.mapY = std::stoi(valStr); } catch (...) {}
            } else if (token == "MapUID") {
                try { currentMap.mapUID = std::stoi(valStr); } catch (...) {}
            } else if (token == "LevelName") {
                currentMap.levelName = valStr;
            } else if (token == "IsSea") {
                std::string l = toLower(valStr);
                currentMap.isSea = (l == "true" || l == "1");
            } else if (token == "LoadedOnPlayerProximity") {
                std::string l = toLower(valStr);
                currentMap.loadedOnPlayerProximity = (l == "true" || l == "1");
            }
        } else if (inRegion) {
            std::string valStr;
            std::getline(iss, valStr);
            valStr = stripQuotesAndSemicolon(valStr);

            if (token == "RegionName" || token == "Name") {
                currentRegion.regionName = valStr;
            } else if (token == "RegionDef" || token == "Def") {
                currentRegion.regionDef = valStr;
            } else if (token == "ContainsMap") {
                currentRegion.containsMaps.push_back(valStr);
            } else if (token == "SeesMap") {
                currentRegion.seesMaps.push_back(valStr);
            }
        }
    }

    return data;
}

std::string StbCompiler::findGameRootFromIni(const std::string& explicitIni) {
    std::vector<std::string> candidates;
    if (!explicitIni.empty()) candidates.push_back(explicitIni);
    candidates.push_back("egocore_config.ini");
    candidates.push_back("..\\egocore_config.ini");
    candidates.push_back("..\\..\\egocore_config.ini");
    candidates.push_back("EgoCore\\egocore_config.ini");
    candidates.push_back("..\\EgoCore\\egocore_config.ini");
    candidates.push_back("..\\..\\EgoCore\\egocore_config.ini");
    candidates.push_back("..\\..\\x64\\Release\\egocore_config.ini");

    for (const auto& c : candidates) {
        if (fs::exists(c)) {
            std::ifstream file(c);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    std::string t = trim(line);
                    if (toLower(t).rfind("root=", 0) == 0) {
                        return trim(t.substr(5));
                    }
                }
            }
        }
    }
    return "";
}

bool StbCompiler::loadConfigFile(const std::string& iniPath, CompilerConfig& config) {
    if (!fs::exists(iniPath)) return false;
    std::ifstream file(iniPath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == ';' || t[0] == '#' || t[0] == '[') {
            continue;
        }

        size_t eqPos = t.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = toLower(trim(t.substr(0, eqPos)));
        std::string valStr = trim(t.substr(eqPos + 1));

        // Strip inline comments if present
        size_t commentPos = valStr.find_first_of(";#");
        if (commentPos != std::string::npos) {
            valStr = trim(valStr.substr(0, commentPos));
        }

        // Strip quotes if wrapped
        if (valStr.size() >= 2 && ((valStr.front() == '"' && valStr.back() == '"') || (valStr.front() == '\'' && valStr.back() == '\''))) {
            valStr = valStr.substr(1, valStr.size() - 2);
        }

        std::string lowerVal = toLower(valStr);

        auto parseBool = [&](bool current) -> bool {
            if (lowerVal == "true" || lowerVal == "1" || lowerVal == "yes" || lowerVal == "on" || lowerVal == "enabled") return true;
            if (lowerVal == "false" || lowerVal == "0" || lowerVal == "no" || lowerVal == "off" || lowerVal == "disabled") return false;
            return current;
        };

        try {
            if (key == "maxdetailerrorfactor" || key == "meshdetail" || key == "meshdetailfactor" || key == "detailerrorfactor") {
                config.meshDetail = std::max(0.1f, std::stof(valStr));
            } else if (key == "foregrounddistance" || key == "foregrounddistancethreshold" || key == "fgdistance" || key == "fgdist") {
                config.fgDistanceThreshold = std::max(0.0f, std::stof(valStr));
            } else if (key == "loddistancescale" || key == "lodscale" || key == "loddistancemultiplier") {
                config.lodDistanceScale = std::max(0.1f, std::stof(valStr));
            } else if (key == "maxlodband" || key == "maxlod") {
                config.maxLodBand = std::clamp(std::stoi(valStr), 1, 7);
            } else if (key == "generateism" || key == "outputism" || key == "generateismfile") {
                config.outputIsm = parseBool(config.outputIsm);
            } else if (key == "sharelodbands" || key == "sharelod" || key == "sharelodbands46") {
                config.shareLodBands = parseBool(config.shareLodBands);
            } else if (key == "enablerangecompression" || key == "rangecompression") {
                config.enableRangeCompression = parseBool(config.enableRangeCompression);
            } else if (key == "highqualitydxt1compression" || key == "highqualitydxt" || key == "highqualitydxt1") {
                config.highQualityDxt = parseBool(config.highQualityDxt);
            } else if (key == "generatecompletemipmapchains" || key == "generatemipmaps" || key == "generatemipmapschains") {
                config.generateMipmaps = parseBool(config.generateMipmaps);
            }
        } catch (...) {}
    }
    return true;
}

CompileResult StbCompiler::compileWorld(const CompilerConfig& config) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompileResult result;

    auto log = [&](const std::string& msg) {
        if (config.verbose) std::cout << msg << std::endl;
        if (config.logCallback) config.logCallback(msg);
    };

    auto reportProgress = [&](float p, const std::string& status) {
        if (config.progressCallback) {
            config.progressCallback(std::clamp(p, 0.0f, 1.0f), status);
        }
    };

    auto checkCancel = [&]() {
        if (config.cancelToken && *(config.cancelToken)) {
            throw std::runtime_error("Compilation cancelled by user.");
        }
    };

    try {
        reportProgress(0.01f, "Initializing compiler configuration...");
        checkCancel();

        // 1. Determine Fable Game Root
        std::string fableRoot = config.fableRoot;
        if (fableRoot.empty() || fableRoot == ".") {
            std::string iniRoot = findGameRootFromIni(config.iniConfigPath);
            if (!iniRoot.empty()) {
                fableRoot = iniRoot;
            }
        }
        if (fableRoot.empty()) {
            fableRoot = ".";
        }
        log("Fable Root: " + fableRoot);
        log("STB Quality Settings:");
        log("  Mesh Detail Factor  : " + std::to_string(config.meshDetail));
        log("  Foreground Distance : " + std::to_string(config.fgDistanceThreshold) + " m");
        log("  LOD Distance Scale  : " + std::to_string(config.lodDistanceScale) + "x");
        log("  Texture Scale       : " + std::to_string(config.textureScale) + "x");
        log("  Generate Mipmaps    : " + std::string(config.generateMipmaps ? "YES" : "NO (1 level)"));
        log("  DXT1 Encoder        : " + std::string(config.highQualityDxt ? "High Quality (Cluster Fit)" : "Fast Bounding-Box"));
        log("  LOD Band Sharing    : " + std::string(config.shareLodBands ? "Enabled (Bands 4-6 shared)" : "Disabled (All bands unique)"));
        log("  Max LOD Band        : " + std::to_string(config.maxLodBand));
        log("  Range Compression   : " + std::string(config.enableRangeCompression ? "Enabled" : "Disabled"));

        // 2. Locate WLD file
        std::string wldName = config.wldName;
        reportProgress(0.02f, "Locating world file: " + wldName);
        checkCancel();
        if (wldName.empty()) {
            throw std::runtime_error("No WLD name specified in CompilerConfig.");
        }
        if (wldName.size() < 4 || toLower(wldName.substr(wldName.size() - 4)) != ".wld") {
            wldName += ".wld";
        }

        fs::path fablePath(fableRoot);
        fs::path wldPath;
        std::vector<fs::path> wldCandidates = {
            fs::path(wldName),
            fs::path("..") / wldName,
            fablePath / "Data" / "Levels" / wldName,
            fablePath / "Data\\Levels" / wldName,
            fablePath / wldName,
            fs::path("STBstuff") / wldName,
            fs::path("STBStuff") / wldName,
            fs::path("..") / "STBstuff" / wldName,
            fs::path("..") / "STBStuff" / wldName,
            fs::path("..") / ".." / "STBStuff" / wldName
        };
        for (const auto& c : wldCandidates) {
            if (fs::exists(c)) {
                wldPath = c;
                break;
            }
        }
        if (wldPath.empty()) {
            throw std::runtime_error("Could not find WLD file '" + wldName + "' in game root or candidate paths.");
        }
        log("WLD File  : " + wldPath.string());

        // 3. Locate engine_theme.def
        reportProgress(0.05f, "Loading theme definitions...");
        checkCancel();
        fs::path defPath;
        std::vector<fs::path> defCandidates = {
            fablePath / "Data" / "Defs" / "engine_theme.def",
            fablePath / "Data\\Defs" / "engine_theme.def",
            fablePath / "Resources" / "Data" / "Defs" / "engine_theme.def",
            fablePath / "engine_theme.def",
            fs::path("..") / "x64" / "Release" / "Resources" / "Data" / "Defs" / "engine_theme.def",
            fs::path("..") / "Resources" / "Data" / "Defs" / "engine_theme.def",
            fs::path("STBstuff") / "engine_theme.def"
        };
        for (const auto& c : defCandidates) {
            if (fs::exists(c)) {
                defPath = c;
                break;
            }
        }
        if (defPath.empty()) {
            throw std::runtime_error("Could not find engine_theme.def in game root or candidate paths.");
        }
        log("Theme Def : " + defPath.string());
        ThemeManager::instance().loadFromDefFile(defPath.string());

        // 3b. Locate engine_local_detail.def
        fs::path ldDefPath;
        std::vector<fs::path> ldDefCandidates = {
            fablePath / "Data" / "Defs" / "engine_local_detail.def",
            fablePath / "Data\\Defs" / "engine_local_detail.def",
            fablePath / "Resources" / "Data" / "Defs" / "engine_local_detail.def",
            fablePath / "engine_local_detail.def",
            fs::path("..") / "x64" / "Release" / "Resources" / "Data" / "Defs" / "engine_local_detail.def",
            fs::path("..") / "Resources" / "Data" / "Defs" / "engine_local_detail.def",
            fs::path("STBstuff") / "engine_local_detail.def"
        };
        for (const auto& c : ldDefCandidates) {
            if (fs::exists(c)) {
                ldDefPath = c;
                break;
            }
        }
        if (!ldDefPath.empty()) {
            log("Local Detail Def: " + ldDefPath.string());
            LocalDetailManager::instance().loadFromDefFile(ldDefPath.string());
        }

        // 3c. Load retail headers if present
        fs::path meshHeader = fablePath / "Data" / "Defs" / "RetailHeaders" / "meshdata.h";
        fs::path texHeader = fablePath / "Data" / "Defs" / "RetailHeaders" / "pc" / "textures.h";
        if (fs::exists(meshHeader) || fs::exists(texHeader)) {
            BankResolver::instance().loadRetailHeaders(meshHeader.string(), texHeader.string());
            log("Loaded Retail Headers for meshes/textures");
        }

        // 4. Load BigBanks for direct retail texture ID resolution and terrain texture sampling
        reportProgress(0.08f, "Loading textures and graphics banks...");
        checkCancel();
        std::vector<fs::path> bankDirs = {
            fablePath / "Data" / "graphics" / "pc",
            fablePath / "Data" / "graphics",
            fablePath / "Data\\graphics\\pc",
            fablePath / "Data\\graphics"
        };
        for (const auto& bDir : bankDirs) {
            if (fs::exists(bDir / "textures.big")) {
                BankResolver::instance().loadTextureBank((bDir / "textures.big").string());
                log("Loaded Bank: " + (bDir / "textures.big").string());
                break;
            }
        }
        if (config.retailMode) {
            for (const auto& bDir : bankDirs) {
                if (fs::exists(bDir / "graphics.big")) {
                    BankResolver::instance().loadMeshBank((bDir / "graphics.big").string());
                    log("Loaded Bank: " + (bDir / "graphics.big").string());
                    break;
                }
            }
        }

        // 5. Parse WLD Map and Region entries
        reportProgress(0.12f, "Parsing world maps and regions...");
        checkCancel();
        auto wldData = parseWldFile(wldPath.string());
        auto& wldMaps = wldData.maps;
        auto& wldRegions = wldData.regions;
        log("Maps in WLD   : " + std::to_string(wldMaps.size()));
        log("Regions in WLD: " + std::to_string(wldRegions.size()));

        // Map ContainsMap and SeesMap to 1-based map index (1..numMaps)
        std::map<std::string, int> levelNameTo1BasedIdx;
        for (size_t i = 0; i < wldMaps.size(); ++i) {
            std::string key = toLower(cleanLevelRelPath(wldMaps[i].levelName));
            levelNameTo1BasedIdx[key] = static_cast<int>(i + 1);
        }

        auto findMap1BasedIdx = [&](const std::string& name) -> int {
            std::string k = toLower(cleanLevelRelPath(name));
            auto it = levelNameTo1BasedIdx.find(k);
            if (it != levelNameTo1BasedIdx.end()) return it->second;
            std::string fn = toLower(fs::path(k).filename().string());
            for (size_t i = 0; i < wldMaps.size(); ++i) {
                if (toLower(fs::path(cleanLevelRelPath(wldMaps[i].levelName)).filename().string()) == fn) {
                    return static_cast<int>(i + 1);
                }
            }
            return 0;
        };

        for (auto& r : wldRegions) {
            for (const auto& mName : r.containsMaps) {
                int idx = findMap1BasedIdx(mName);
                if (idx > 0) r.containsMapIndices.push_back(idx);
            }
            for (const auto& mName : r.seesMaps) {
                int idx = findMap1BasedIdx(mName);
                if (idx > 0) r.seesMapIndices.push_back(idx);
            }
        }

        // 6. Pass 0: Lightweight Metadata & In-Memory ISM Generation (Memory Efficiency)
        reportProgress(0.14f, "Scanning map metadata and building ISMs...");
        checkCancel();
        log("\n--- Pass 0: Scanning Maps & Computing Optimal Build Order ---");
        struct MapMetadata {
            int id = 0; // 1-based index (1..numMaps)
            int mapX = 0;
            int mapY = 0;
            int width = 0;
            int height = 0;
            float minH = 0.0f;
            float maxH = 0.0f;
            uint32_t checksum = 0;
            std::string cleanRel;
            std::string canonicalName;
            fs::path levPath;
            PrecomputedWaterGrid waterGrid;
            IsmBuildResult ismResult;
            LevData lev;
        };

        std::vector<MapMetadata> mapMetas(wldMaps.size());

        if (!wldMaps.empty()) {
            std::atomic<size_t> nextMap{0};
            unsigned int numThreads = std::min(
                std::max(1u, std::thread::hardware_concurrency()),
                static_cast<unsigned int>(wldMaps.size())
            );
            std::vector<std::thread> workers;
            workers.reserve(numThreads);
            for (unsigned int t = 0; t < numThreads; ++t) {
                workers.emplace_back([&]() {
                    while (true) {
                        size_t i = nextMap.fetch_add(1);
                        if (i >= wldMaps.size()) break;

                        const auto& mapEntry = wldMaps[i];
                        std::string cleanRel = cleanLevelRelPath(mapEntry.levelName);
                        std::string canonicalName = "Data\\Levels\\" + cleanRel;

                        // Locate .lev file
                        fs::path foundLev;
                        std::vector<fs::path> levCandidates = {
                            wldPath.parent_path() / cleanRel,
                            wldPath.parent_path() / fs::path(cleanRel).filename(),
                            fablePath / "Data" / "Levels" / cleanRel,
                            fablePath / "Data\\Levels" / cleanRel,
                            fs::path("..") / fs::path(cleanRel).filename(),
                            fs::path("STBstuff") / fs::path(cleanRel).filename(),
                            fs::path("STBStuff") / fs::path(cleanRel).filename(),
                            fs::path("..") / "STBstuff" / fs::path(cleanRel).filename(),
                            fs::path("..") / "STBStuff" / fs::path(cleanRel).filename(),
                            fs::path("..") / ".." / "STBStuff" / fs::path(cleanRel).filename()
                        };
                        for (const auto& c : levCandidates) {
                            if (fs::exists(c)) {
                                foundLev = c;
                                break;
                            }
                        }

                        MapMetadata meta;
                        meta.id = static_cast<int>(i + 1);
                        meta.mapX = mapEntry.mapX;
                        meta.mapY = mapEntry.mapY;
                        meta.cleanRel = cleanRel;
                        meta.canonicalName = canonicalName;
                        meta.levPath = foundLev;

                        if (!foundLev.empty()) {
                            auto lev = LevReader::loadFromFile(foundLev.string());
                            meta.width = lev.width;
                            meta.height = lev.height;
                            meta.checksum = lev.checksum;

                            float minH = 1e30f, maxH = -1e30f;
                            for (int y = 0; y < lev.height; ++y) {
                                for (int x = 0; x < lev.width; ++x) {
                                    float h = lev.getCell(x, y).height;
                                    float qh = static_cast<float>(static_cast<int>(h * 128.0f)) * 0.0078125f;
                                    if (qh < minH) minH = qh;
                                    if (qh > maxH) maxH = qh;
                                }
                            }
                            if (minH > maxH) { minH = 0.0f; maxH = 20.0f; }
                            meta.minH = minH;
                            meta.maxH = maxH;

                            meta.waterGrid = buildWaterGrid(lev);
                            // Build ISM payload in-memory (empty output path)
                            meta.ismResult = IsmBuilder::build(lev, meta.mapX, meta.mapY, canonicalName, "");
                            meta.lev = std::move(lev);
                        } else {
                            meta.width = 128;
                            meta.height = 128;
                            meta.minH = 0.0f;
                            meta.maxH = 20.0f;
                        }

                        mapMetas[i] = std::move(meta);
                    }
                });
            }
            for (auto& w : workers) {
                w.join();
            }
        }


        // 7. Compute CameraMapBounds (CalcCameraMapBounds 0x12605e0)
        size_t numMaps = wldMaps.size();
        size_t numRegions = wldRegions.size();

        std::vector<ObservingMapBounds> cameraMapBounds(numMaps + 1);
        for (size_t m = 1; m <= numMaps; ++m) {
            const auto& meta = mapMetas[m - 1];
            cameraMapBounds[m].minX = static_cast<float>(meta.mapX);
            cameraMapBounds[m].minY = static_cast<float>(meta.mapY);
            cameraMapBounds[m].minZ = meta.minH;
            cameraMapBounds[m].maxX = static_cast<float>(meta.mapX + meta.width);
            cameraMapBounds[m].maxY = static_cast<float>(meta.mapY + meta.height);
            cameraMapBounds[m].maxZ = meta.maxH + 40.0f; // 40.0f EstimatedMaxCameraHeightAboveLandscape
        }

        // 8. Build Region Visibility Info & Observing Map sets (0xd5af10 & 0x1262630)
        // Treat every level (both ContainsMap and SeesMap) as a ContainsMap so no seam/culling
        // optimizations degrade SeesMap boundaries.
        std::vector<std::set<int>> mapVisibleMaps(numMaps + 1);
        for (size_t m = 1; m <= numMaps; ++m) {
            mapVisibleMaps[m].insert(static_cast<int>(m));
        }
        for (const auto& r : wldRegions) {
            std::set<int> regionMaps;
            for (int cm : r.containsMapIndices) regionMaps.insert(cm);
            for (int sm : r.seesMapIndices) regionMaps.insert(sm);

            for (int obsIdx : regionMaps) {
                if (obsIdx >= 1 && obsIdx <= static_cast<int>(numMaps)) {
                    for (int targetIdx : regionMaps) {
                        mapVisibleMaps[obsIdx].insert(targetIdx);
                    }
                }
            }
        }

        std::vector<std::vector<ObservingMapBounds>> targetObservingBounds(numMaps + 1);
        for (int obsIdx = 1; obsIdx <= static_cast<int>(numMaps); ++obsIdx) {
            for (int targetIdx : mapVisibleMaps[obsIdx]) {
                if (targetIdx >= 1 && targetIdx <= static_cast<int>(numMaps)) {
                    targetObservingBounds[targetIdx].push_back(cameraMapBounds[obsIdx]);
                }
            }
        }

        // 9. BuildOptimalStaticMapBuildOrderList (0xd53a10)
        std::vector<uint32_t> region_sort_keys(numRegions, 0);
        std::vector<uint32_t> map_sort_keys(numMaps + 1, 0);
        std::vector<int> map_region_dependance_count(numMaps + 1, 0);
        std::vector<int> map_last_dependant_region(numMaps + 1, 0);

        for (size_t r = 0; r < numRegions; ++r) {
            int count = 0;
            int sumX = 0;
            int sumY = 0;

            auto processRegionMap = [&](int mapIdx) {
                if (mapIdx >= 1 && mapIdx <= static_cast<int>(numMaps)) {
                    const auto& m = mapMetas[mapIdx - 1];
                    int centerX = (m.mapX + m.mapX + m.width) / 2;
                    int centerY = (m.mapY + m.mapY + m.height) / 2;
                    sumX += centerX;
                    sumY += centerY;
                    count++;
                    map_region_dependance_count[mapIdx]++;
                    map_last_dependant_region[mapIdx] = static_cast<int>(r);
                }
            };

            for (int cm : wldRegions[r].containsMapIndices) {
                processRegionMap(cm);
            }
            for (int sm : wldRegions[r].seesMapIndices) {
                processRegionMap(sm);
            }

            if (count > 0) {
                int avgX = sumX / count;
                int avgY = sumY / count;
                region_sort_keys[r] = morton2D(avgX, avgY);
            } else {
                region_sort_keys[r] = 0;
            }
        }

        for (size_t m = 1; m <= numMaps; ++m) {
            if (map_region_dependance_count[m] == 1) {
                map_sort_keys[m] = region_sort_keys[map_last_dependant_region[m]];
            } else {
                const auto& meta = mapMetas[m - 1];
                int centerX = (meta.mapX + meta.mapX + meta.width) / 2;
                int centerY = (meta.mapY + meta.mapY + meta.height) / 2;
                map_sort_keys[m] = morton2D(centerX, centerY);
            }
        }

        // Selection loop across regions and maps
        std::vector<bool> regions_added(numRegions, false);
        std::vector<bool> maps_added(numMaps + 1, false);
        std::vector<std::pair<int, bool>> buildOrderList;

        while (true) {
            int bestIdx = -1;
            bool bestIsMap = false;
            uint32_t bestKey = 0;

            for (size_t r = 0; r < numRegions; ++r) {
                if (!regions_added[r]) {
                    if (bestIdx < 0 || region_sort_keys[r] < bestKey) {
                        bestKey = region_sort_keys[r];
                        bestIdx = static_cast<int>(r);
                        bestIsMap = false;
                    }
                }
            }

            for (size_t m = 1; m <= numMaps; ++m) {
                if (!maps_added[m]) {
                    if (bestIdx < 0 || map_sort_keys[m] < bestKey) {
                        bestKey = map_sort_keys[m];
                        bestIdx = static_cast<int>(m);
                        bestIsMap = true;
                    }
                }
            }

            if (bestIdx < 0) {
                break;
            }

            buildOrderList.push_back({bestIdx, bestIsMap});
            if (bestIsMap) {
                maps_added[bestIdx] = true;
            } else {
                regions_added[bestIdx] = true;
            }
        }

        // Expand buildOrderList using maps_done to obtain final compilation order
        std::set<int> maps_done;
        std::vector<int> finalMapOrder; // 1-based map indices

        for (const auto& item : buildOrderList) {
            if (item.second) { // is_map
                int m = item.first;
                if (maps_done.find(m) == maps_done.end()) {
                    maps_done.insert(m);
                    finalMapOrder.push_back(m);
                }
            } else { // is_region
                int r = item.first;
                for (int m : wldRegions[r].containsMapIndices) {
                    if (maps_done.find(m) == maps_done.end()) {
                        maps_done.insert(m);
                        finalMapOrder.push_back(m);
                    }
                }
            }
        }

        for (size_t m = 1; m <= numMaps; ++m) {
            if (maps_done.find(static_cast<int>(m)) == maps_done.end()) {
                maps_done.insert(static_cast<int>(m));
                finalMapOrder.push_back(static_cast<int>(m));
            }
        }

        log("Optimal Build Order determined (" + std::to_string(finalMapOrder.size()) + " maps):");
        for (size_t orderIdx = 0; orderIdx < finalMapOrder.size(); ++orderIdx) {
            int m = finalMapOrder[orderIdx];
            log("  [" + std::to_string(orderIdx) + "] Map " + std::to_string(m) + ": " +
                mapMetas[m - 1].canonicalName + " (SortKey: 0x" +
                [&]() {
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << map_sort_keys[m];
                    return ss.str();
                }() + ")");
        }

        // 10. Determine output paths
        std::string wldStem = wldPath.stem().string();
        std::string outStbPath = config.outputPath;
        if (outStbPath.empty()) {
            fs::path defOut = fablePath / "Data" / "Levels" / (wldStem + "_RT.stb");
            outStbPath = defOut.string();
        }
        log("\nOutput STB: " + outStbPath);

        // 11. Initialize STB BBB Archive
        BBBArchive outArchive;
        outArchive.alignment = 2048;
        outArchive.version1 = 115;
        outArchive.version2 = 174;
        outArchive.version3 = 1;

        // 11. Parse Sea Definitions & Generate Sea Static Map Bank Entries
        fs::path defsDir = fablePath / "Data" / "Defs";
        SeaDefConfig seaDefs = SeaGenerator::parseDefs(defsDir);
        std::vector<std::string> hasSeaDefsList = SeaGenerator::parseHasSeaRegionDefs(defsDir);
        std::set<std::string> hasSeaSet(hasSeaDefsList.begin(), hasSeaDefsList.end());

        log("\n--- Sea Static Map Generation ---");
        log("Sea Height      : " + std::to_string(seaDefs.seaHeight));
        log("Sea Radius      : " + std::to_string(seaDefs.seaRadius));
        log("Flat Section    : " + std::to_string(seaDefs.seaFlatSectionStart) + " -> " + std::to_string(seaDefs.seaFlatSectionEnd));
        log("HasSea Reg Defs : " + std::to_string(hasSeaSet.size()));

        std::vector<SeaMapInfo> allWorldMaps(numMaps);
        for (size_t i = 0; i < numMaps; ++i) {
            allWorldMaps[i].canonicalName = mapMetas[i].canonicalName;
            allWorldMaps[i].tlx = mapMetas[i].mapX;
            allWorldMaps[i].tly = mapMetas[i].mapY;
            allWorldMaps[i].brx = mapMetas[i].mapX + mapMetas[i].width;
            allWorldMaps[i].bry = mapMetas[i].mapY + mapMetas[i].height;
            allWorldMaps[i].width = mapMetas[i].width;
            allWorldMaps[i].height = mapMetas[i].height;
            allWorldMaps[i].waterGrid.resize((mapMetas[i].width + 1) * (mapMetas[i].height + 1), 0);
            for (int y = 0; y <= mapMetas[i].height; ++y) {
                for (int x = 0; x <= mapMetas[i].width; ++x) {
                    const auto& cell = mapMetas[i].lev.getCell(x, y);
                    float depth = 0.0f;
                    for (int t = 0; t < 3; ++t) {
                        uint8_t thIdx = cell.themes[t];
                        if (thIdx < mapMetas[i].lev.themePalette.size()) {
                            const auto& tname = mapMetas[i].lev.themePalette[thIdx].name;
                            if (!tname.empty()) {
                                const auto* def = ThemeManager::instance().getTheme(tname);
                                if (def && def->waterType > 0) {
                                    float blend = 0.0f;
                                    if (t == 0) blend = static_cast<float>(cell.blends[0]) / 255.0f;
                                    else if (t == 1) blend = static_cast<float>(cell.blends[1]) / 255.0f;
                                    else {
                                        int rem = 255 - static_cast<int>(cell.blends[0]) - static_cast<int>(cell.blends[1]);
                                        if (rem > 0) blend = static_cast<float>(rem) / 255.0f;
                                    }
                                    depth += def->waterHeight * blend;
                                }
                            }
                        }
                    }
                    if (depth > 0.001f) {
                        allWorldMaps[i].waterGrid[y * (mapMetas[i].width + 1) + x] = 1;
                    }
                }
            }
        }

        std::vector<SeaRegionJob> seaJobs;
        for (const auto& r : wldRegions) {
            if (hasSeaSet.count(r.regionDef) > 0) {
                SeaRegionJob job;
                job.regionName = r.regionName;
                job.regionDef = r.regionDef;

                auto addMapToJob = [&](int mapIdx) {
                    if (mapIdx >= 1 && mapIdx <= static_cast<int>(numMaps)) {
                        job.cutoutMaps.push_back(allWorldMaps[mapIdx - 1]);
                    }
                };
                for (int cm : r.containsMapIndices) addMapToJob(cm);
                for (int sm : r.seesMapIndices) addMapToJob(sm);

                seaJobs.push_back(std::move(job));
            }
        }

        if (!seaJobs.empty()) {
            reportProgress(0.20f, "Generating " + std::to_string(seaJobs.size()) + " Sea Static Maps...");
            checkCancel();
            log("Generating " + std::to_string(seaJobs.size()) + " Sea Static Maps (multithreaded)...");
            auto seaEntries = SeaGenerator::generateAllSeas(
                seaJobs,
                seaDefs,
                allWorldMaps,
                0,
                [&](const std::string& msg) { log(msg); }
            );
            for (auto& se : seaEntries) {
                outArchive.entries.push_back(std::move(se));
            }
            log("Sea Generation Complete: " + std::to_string(seaEntries.size()) + " sea entries added.");
        } else {
            // Water bank header fallback if no sea regions
            BBBEntry waterBank;
            waterBank.id = 1;
            waterBank.type = 0;
            waterBank.devType = 1;
            waterBank.name = "__ENGINE_WATER_STATIC_MAP_BANK_FILE__";
            waterBank.devName = "__ENGINE_WATER_STATIC_MAP_BANK_FILE__";
            waterBank.extra = {0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            outArchive.entries.push_back(waterBank);
        }

        // 12. Initialize single multi-entry ISM Archive
        BBBArchive ismArchive;
        ismArchive.alignment = 2048;
        ismArchive.version1 = 115;
        ismArchive.version2 = 174;
        ismArchive.version3 = 1;

        int totalNodesAcc = 0;
        int totalPatchesAcc = 0;

        std::vector<RetailMapSubheaderConfig> retailConfigs;

        // Neighbour lookup (maps within a 32-cell margin) used for water queries.
        auto buildNeighbors = [&](const auto& meta) {
            std::vector<WorldWaterNeighbor> neighbors;
            const int neighborMargin = 32;
            int minX = meta.mapX - neighborMargin;
            int maxX = meta.mapX + meta.width + neighborMargin;
            int minY = meta.mapY - neighborMargin;
            int maxY = meta.mapY + meta.height + neighborMargin;

            for (const auto& other : mapMetas) {
                if (other.id == meta.id || other.levPath.empty()) continue;
                if (other.mapX + other.width >= minX && other.mapX <= maxX &&
                    other.mapY + other.height >= minY && other.mapY <= maxY) {
                    WorldWaterNeighbor n;
                    n.mapX = other.mapX;
                    n.mapY = other.mapY;
                    n.width = other.width;
                    n.height = other.height;
                    n.lev = &other.lev;
                    n.waterGrid = &other.waterGrid;
                    n.shorePoints = &other.ismResult.shorePoints;
                    neighbors.push_back(n);
                }
            }
            return neighbors;
        };

        // World-consistent shore/water bodies: gathered from every map once so that
        // vertices shared across a map seam receive identical shore data on both sides.
        std::shared_ptr<const std::vector<MeshBuilder::WaterBody>> worldWaterBodies;
        {
            std::vector<std::vector<MeshBuilder::ShorePoint>> perMapShore;
            std::vector<MeshBuilder::ShorePoint> runningUnsorted;
            for (size_t orderIdx = 0; orderIdx < finalMapOrder.size(); ++orderIdx) {
                auto& m = mapMetas[finalMapOrder[orderIdx] - 1];
                if (m.levPath.empty()) continue;
                auto nb = buildNeighbors(m);
                MeshBuilder collector(m.lev, m.mapX, m.mapY, {}, &nb, &m.waterGrid);
                auto shorePts = collector.collectShorePoints();

                // Match CWaterGenerator::FindShorePointsInMap (0x2e07490):
                // If a border shore point is already in runningUnsorted, erase it from
                // runningUnsorted and do not add it to this map's saved ShorePoints.
                std::vector<std::pair<int, int>> ismShorePts;
                ismShorePts.reserve(shorePts.size());
                for (const auto& pt : shorePts) {
                    if (pt.isBorder) {
                        auto it = std::find_if(runningUnsorted.begin(), runningUnsorted.end(),
                            [&](const MeshBuilder::ShorePoint& sp) {
                                return sp.x == pt.x && sp.y == pt.y;
                            });
                        if (it != runningUnsorted.end()) {
                            runningUnsorted.erase(it);
                        } else {
                            ismShorePts.push_back({pt.x, pt.y});
                        }
                    } else {
                        ismShorePts.push_back({pt.x, pt.y});
                    }
                }
                for (const auto& pt : ismShorePts) {
                    runningUnsorted.push_back({pt.first, pt.second, false});
                }
                m.ismResult.shorePoints = ismShorePts;

                // Rewrite Part B: CEngineMap::SaveShorePointsToStream at shorePointArrayStart
                // in m.ismResult.payload and m.ismResult.entryPayload
                size_t start = m.ismResult.shorePointArrayStart;
                m.ismResult.payload.resize(start);
                auto writeI32 = [](std::vector<uint8_t>& buf, int32_t val) {
                    size_t cur = buf.size();
                    buf.resize(cur + 4);
                    std::memcpy(buf.data() + cur, &val, 4);
                };
                writeI32(m.ismResult.payload, static_cast<int32_t>(ismShorePts.size()));
                for (const auto& pt : ismShorePts) {
                    writeI32(m.ismResult.payload, pt.first);
                    writeI32(m.ismResult.payload, pt.second);
                }
                m.ismResult.shorePointArraySize = static_cast<uint32_t>(m.ismResult.payload.size() - start);
                m.ismResult.entryPayload = m.ismResult.payload;

                perMapShore.push_back(std::move(shorePts));
            }
            worldWaterBodies = MeshBuilder::buildWorldWaterBodies(perMapShore);
            log("World water bodies: " + std::to_string(worldWaterBodies->size()));
        }
        // 13. Pass 1: Compile each map in optimal Morton build order
        for (size_t orderIdx = 0; orderIdx < finalMapOrder.size(); ++orderIdx) {
            checkCancel();
            int m = finalMapOrder[orderIdx];
            const auto& mapEntry = wldMaps[m - 1];
            auto& meta = mapMetas[m - 1];
            const auto& mapObservingBounds = targetObservingBounds[m];

            if (meta.levPath.empty()) {
                log("Warning: Map " + std::to_string(orderIdx + 1) + "/" + std::to_string(finalMapOrder.size()) +
                    " (" + meta.canonicalName + ") .lev not found on disk. Skipping.");
                continue;
            }

            float mapProgress = 0.22f + 0.70f * (static_cast<float>(orderIdx) / static_cast<float>(finalMapOrder.size()));
            std::string mapStatus = "Compiling " + meta.canonicalName + " (" +
                                    std::to_string(orderIdx + 1) + "/" + std::to_string(finalMapOrder.size()) + ")...";
            reportProgress(mapProgress, mapStatus);

            int pct = static_cast<int>((orderIdx * 100) / finalMapOrder.size());
            int barWidth = 30;
            int filled = static_cast<int>((orderIdx * barWidth) / finalMapOrder.size());
            std::string bar = "[" + std::string(filled, '=') + (filled < barWidth ? ">" : "") + std::string(barWidth - filled - (filled < barWidth ? 1 : 0), ' ') + "]";

            log("\n" + bar + " " + std::to_string(pct) + "% (" + std::to_string(orderIdx + 1) + "/" + std::to_string(finalMapOrder.size()) + ") Compiling: " + meta.canonicalName);
            log("  Source LEV : " + meta.levPath.string());
            log("  World Pos  : (" + std::to_string(meta.mapX) + ", " + std::to_string(meta.mapY) + ")");
            log("  Dimensions : " + std::to_string(meta.width) + "x" + std::to_string(meta.height));
            log("  Observing  : " + std::to_string(mapObservingBounds.size()) + " bounds");

            const auto& lev = meta.lev;

            std::vector<std::string> localPalette;
            auto themeMaterials = ThemeManager::instance().buildMaterials(lev, config.retailMode, localPalette);

            // A. Foreground: Variable patch grid & CalcIfNeededInStaticMapFile culling
            int patchGridW = lev.width / 16;
            int patchGridH = lev.height / 16;
            int numFgPatches = patchGridW * patchGridH;

            std::vector<CompiledPatch> fgPatches(numFgPatches);
            std::vector<std::vector<uint8_t>> compressedFgFrames(numFgPatches);
            std::atomic<bool> mapNeedsForeground{false};

            std::vector<WorldWaterNeighbor> neighbors = buildNeighbors(meta);

            if (numFgPatches > 0) {
                std::atomic<int> nextPatch{0};
                unsigned int numThreads = std::min(
                    std::max(1u, std::thread::hardware_concurrency()),
                    static_cast<unsigned int>(numFgPatches)
                );
                std::vector<std::thread> workers;
                workers.reserve(numThreads);
                for (unsigned int t = 0; t < numThreads; ++t) {
                    workers.emplace_back([&]() {
                        MeshBuilder threadBuilder(lev, meta.mapX, meta.mapY, themeMaterials, &neighbors, &meta.waterGrid);
                        threadBuilder.setWorldWaterBodies(worldWaterBodies);
                        while (true) {
                            int pIdx = nextPatch.fetch_add(1);
                            if (pIdx >= numFgPatches) break;

                            int py = pIdx / patchGridW;
                            int px = pIdx % patchGridW;

                            auto p = threadBuilder.buildPatch(px, py);

                            float minDist = findShortestDistanceToObservingMapSet(
                                p.minCoord[0], p.minCoord[1], p.minCoord[2],
                                p.maxCoord[0], p.maxCoord[1], p.maxCoord[2],
                                mapObservingBounds
                            );
                            bool patchNeeded = (minDist <= config.fgDistanceThreshold);
                            p.fgNeeded = patchNeeded ? 1 : 0;
                            if (p.fgNeeded != 0) {
                                mapNeedsForeground.store(true, std::memory_order_relaxed);
                            }

                            auto uncomp = p.serializeUncompressed();
                            auto comp = albion::lzo1x::compress(uncomp.data(), uncomp.size());

                            std::vector<uint8_t> frame(8 + comp.size());
                            uint32_t uLen = static_cast<uint32_t>(uncomp.size());
                            uint32_t cLen = static_cast<uint32_t>(comp.size());
                            std::memcpy(frame.data(), &uLen, 4);
                            std::memcpy(frame.data() + 4, &cLen, 4);
                            std::memcpy(frame.data() + 8, comp.data(), comp.size());

                            p.dataSize = static_cast<uint32_t>(frame.size());
                            fgPatches[pIdx] = std::move(p);
                            compressedFgFrames[pIdx] = std::move(frame);
                        }
                    });
                }
                for (auto& w : workers) {
                    w.join();
                }
            }


            // Assemble foreground stream
            std::vector<uint8_t> payload;
            uint32_t ebool = 1;
            payload.resize(4);
            std::memcpy(payload.data(), &ebool, 4);
            payload.resize(2048, 0); // patch_header_pos = 2048
            size_t patchHeadersPos = 2048;
            payload.resize(2048 + 36 * numFgPatches, 0);

            uint32_t patchesPos = 0;

            if (mapNeedsForeground) {
                patchesPos = 2048;
                for (size_t pIdx = 0; pIdx < fgPatches.size(); ++pIdx) {
                    if (fgPatches[pIdx].fgNeeded != 0) {
                        size_t alignedPos = alignUp(payload.size(), 2048);
                        payload.resize(alignedPos, 0);
                        fgPatches[pIdx].filePos = static_cast<uint32_t>(alignedPos);
                        payload.insert(payload.end(), compressedFgFrames[pIdx].begin(), compressedFgFrames[pIdx].end());
                    } else {
                        fgPatches[pIdx].filePos = 0;
                        fgPatches[pIdx].dataSize = 0;
                    }
                }

                for (size_t pIdx = 0; pIdx < fgPatches.size(); ++pIdx) {
                    uint8_t* hp = payload.data() + patchHeadersPos + pIdx * 36;
                    std::memcpy(hp + 0, &fgPatches[pIdx].filePos, 4);
                    std::memcpy(hp + 4, &fgPatches[pIdx].dataSize, 4);
                    std::memcpy(hp + 8, fgPatches[pIdx].minCoord, 12);
                    std::memcpy(hp + 20, fgPatches[pIdx].maxCoord, 12);
                    std::memcpy(hp + 32, &fgPatches[pIdx].fgNeeded, 4);
                }

                payload.resize(alignUp(payload.size(), 2048), 0);
                log("  Foreground: " + std::to_string(numFgPatches) + " patches (" + std::to_string(payload.size()) + " bytes, needed: TRUE)");
            } else {
                patchesPos = 0;
                payload.resize(alignUp(payload.size(), 2048), 0);
                log("  Foreground: " + std::to_string(numFgPatches) + " patches (needed: FALSE, culled to " + std::to_string(payload.size()) + " bytes)");
            }

            // B. Background: Generate LOD quadtree & LOD patches
            log("  Building background LOD quadtree...");
            BgTreeSettings bgSettings;
            bgSettings.meshDetail = config.meshDetail;
            bgSettings.lodDistanceScale = config.lodDistanceScale;
            bgSettings.textureScale = config.textureScale;
            bgSettings.generateMipmaps = config.generateMipmaps;
            bgSettings.highQualityDxt = config.highQualityDxt;
            bgSettings.shareLodBands = config.shareLodBands;
            bgSettings.maxLodBand = config.maxLodBand;
            bgSettings.enableRangeCompression = config.enableRangeCompression;

            auto bgResult = invokeBgTreeBuild(
                lev, themeMaterials,
                meta.mapX, meta.mapY,
                payload.size(),
                &mapObservingBounds,
                &neighbors,
                bgSettings
            );

            log("  BG Tree: " + std::to_string(bgResult.totalNodes) + " nodes, " +
                std::to_string(bgResult.totalPatches) + " LOD patches in " +
                std::to_string(bgResult.totalBlocks) + " blocks.");

            totalNodesAcc += bgResult.totalNodes;
            totalPatchesAcc += bgResult.totalPatches;

            payload.insert(payload.end(),
                           bgResult.treeAndPatchData.begin(),
                           bgResult.treeAndPatchData.end());

            log("  Map payload after BG Tree: " + std::to_string(payload.size()) + " bytes");

            // C. Append to single multi-entry ISM Archive
            BBBEntry ismEntry;
            ismEntry.id = static_cast<uint32_t>(ismArchive.entries.size() + 1);
            ismEntry.type = 0;
            ismEntry.devType = 1;
            ismEntry.name = meta.canonicalName;
            ismEntry.devName = meta.canonicalName;
            ismEntry.payload = std::move(meta.ismResult.entryPayload);
            ismArchive.entries.push_back(std::move(ismEntry));

            // D. Generate LocalDetail clutter standalone from defs + lev
            std::vector<uint8_t> localDetailQuadtree;

            if (config.retailMode) {
                log("  Building LocalDetail clutter (standalone)...");
                auto ldResult = invokeLdBuild(
                    lev,
                    meta.mapX, meta.mapY,
                    payload.size(),
                    fablePath.string(),
                    wldPath.parent_path().string(),
                    &mapObservingBounds
                );

                if (ldResult.hasData && !ldResult.payloadBytes.empty()) {
                    if (payload.size() < ldResult.payloadStartOffset) {
                        payload.resize(ldResult.payloadStartOffset, 0);
                    }
                    payload.insert(payload.end(),
                                   ldResult.payloadBytes.begin(),
                                   ldResult.payloadBytes.end());
                    localDetailQuadtree = std::move(ldResult.headerBytes);
                    log("  Generated LocalDetail: " + std::to_string(ldResult.totalInstances) +
                        " instances in " + std::to_string(ldResult.totalBatches) + " batches (" +
                        std::to_string(localDetailQuadtree.size()) + " tree bytes, " +
                        std::to_string(ldResult.payloadBytes.size()) + " payload bytes)");
                } else if (!ldResult.headerBytes.empty()) {
                    localDetailQuadtree = std::move(ldResult.headerBytes);
                    log("  LocalDetail: Empty clutter tree (" + std::to_string(localDetailQuadtree.size()) + " tree bytes).");
                } else {
                    log("  LocalDetail: No clutter generated. Map will load cleanly without clutter.");
                }
            }

            log("  Total map payload: " + std::to_string(payload.size()) + " bytes");

            // E. Build Map Entry and Subheader
            BBBEntry outEntry;
            outEntry.id = static_cast<uint32_t>(outArchive.entries.size() + 1);
            outEntry.type = 0;
            outEntry.devType = 1;
            outEntry.name = meta.canonicalName;
            outEntry.devName = meta.canonicalName;
            outEntry.payload = std::move(payload);

            if (config.retailMode) {
                outEntry.extra = {}; // Retail mode: extra is empty on map entry!
                RetailMapSubheaderConfig rc;
                rc.canonicalName = meta.canonicalName;
                rc.bankFileIndex = outEntry.id;
                rc.worldX = meta.mapX;
                rc.worldY = meta.mapY;
                rc.mapWidth = meta.width;
                rc.mapHeight = meta.height;
                rc.minHeight = meta.minH;
                rc.maxHeight = meta.maxH + 40.0f;
                rc.levelChecksum = meta.checksum;
                rc.patchTreePos = bgResult.patchTreePos;
                rc.patchTreeSize = bgResult.patchTreeSize;
                rc.edgeHeightFileSize = meta.ismResult.edgeHeightFileSize;
                rc.shorePointArraySize = meta.ismResult.shorePointArraySize;
                rc.shorePointArrayStart = meta.ismResult.shorePointArrayStart;
                rc.patchesPos = patchesPos;
                rc.localDetailData = std::move(localDetailQuadtree);
                retailConfigs.push_back(std::move(rc));
            } else {
                SubheaderConfig sc;
                sc.bankFileIndex = outEntry.id;
                sc.worldX = meta.mapX;
                sc.worldY = meta.mapY;
                sc.mapWidth = meta.width;
                sc.mapHeight = meta.height;
                sc.minHeight = meta.minH;
                sc.maxHeight = meta.maxH + 40.0f;
                sc.levelChecksum = meta.checksum;
                sc.retailMode = false;
                sc.localPalette = &localPalette;
                sc.patchTreePos = bgResult.patchTreePos;
                sc.patchTreeSize = bgResult.patchTreeSize;
                sc.patchesPos = patchesPos;
                outEntry.extra = buildSubheader(sc);
            }

            outArchive.entries.push_back(std::move(outEntry));
            result.mapsCompiled++;
        }

        // Retail Mode: build __STATIC_MAP_COMMON_HEADER__
        if (config.retailMode && !retailConfigs.empty()) {
            std::vector<uint8_t> commonPayload;
            uint32_t numMapsU32 = static_cast<uint32_t>(retailConfigs.size());
            commonPayload.resize(4);
            std::memcpy(commonPayload.data(), &numMapsU32, 4);

            struct DirEntry {
                std::string canonicalName;
                size_t configIndex = 0;
                size_t ptrOffsetInCommon = 0;
            };
            std::vector<DirEntry> sortedDir(retailConfigs.size());
            for (size_t mi = 0; mi < retailConfigs.size(); ++mi) {
                sortedDir[mi].canonicalName = retailConfigs[mi].canonicalName;
                sortedDir[mi].configIndex = mi;
            }
            std::sort(sortedDir.begin(), sortedDir.end(), [](const DirEntry& a, const DirEntry& b) {
                return a.canonicalName < b.canonicalName;
            });

            for (auto& entry : sortedDir) {
                commonPayload.insert(commonPayload.end(), entry.canonicalName.begin(), entry.canonicalName.end());
                commonPayload.push_back('\0');
                entry.ptrOffsetInCommon = commonPayload.size();
                commonPayload.resize(commonPayload.size() + 4, 0); // placeholder for subheader pointer
            }

            for (size_t mi = 0; mi < retailConfigs.size(); ++mi) {
                auto& c = retailConfigs[mi];
                c.subheaderStart = commonPayload.size();
                int32_t subPtr = static_cast<int32_t>(c.subheaderStart);
                for (const auto& entry : sortedDir) {
                    if (entry.configIndex == mi) {
                        std::memcpy(commonPayload.data() + entry.ptrOffsetInCommon, &subPtr, 4);
                        break;
                    }
                }
                appendMapSubheaderToCommonStream(commonPayload, c);
            }

            BBBEntry commonEntry;
            commonEntry.id = static_cast<uint32_t>(outArchive.entries.size() + 1);
            commonEntry.type = 0;
            commonEntry.devType = 1;
            commonEntry.name = "__STATIC_MAP_COMMON_HEADER__";
            commonEntry.devName = "__STATIC_MAP_COMMON_HEADER__";
            commonEntry.payload = std::move(commonPayload);
            outArchive.entries.push_back(std::move(commonEntry));
            log("Added __STATIC_MAP_COMMON_HEADER__ (size: " + std::to_string(outArchive.entries.back().payload.size()) + " bytes)");
        }

        // 14. Write single multi-entry ISM Archive (when config.outputIsm is true)
        fs::path outDir = fs::path(outStbPath).parent_path();
        if (!outDir.empty() && !fs::exists(outDir)) {
            fs::create_directories(outDir);
        }

        if (config.outputIsm && !ismArchive.entries.empty()) {
            reportProgress(0.93f, "Writing multi-map ISM archive...");
            checkCancel();
            fs::path outIsmPath = outDir / (wldStem + "_RT.ism");
            if (outIsmPath.string().find("_reference.ism") == std::string::npos) {
                ismArchive.write(outIsmPath);
                log("Written Multi-Map ISM: " + outIsmPath.string() + " (" +
                    std::to_string(fs::file_size(outIsmPath)) + " bytes, " +
                    std::to_string(ismArchive.entries.size()) + " entries)");
            }
        }

        // 15. Write final STB Archive
        reportProgress(0.96f, "Writing Static Terrain Bank archive (.stb)...");
        checkCancel();
        outArchive.write(outStbPath);

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;

        result.success = true;
        result.outputPath = outStbPath;
        result.fileSize = fs::file_size(outStbPath);
        result.totalNodes = totalNodesAcc;
        result.totalPatches = totalPatchesAcc;
        result.durationSeconds = elapsed.count();

        reportProgress(1.0f, "Static Terrain Bank compiled successfully!");

        log("\n===========================================");
        log("       STB Compilation Succeeded!          ");
        log("===========================================");
        log("Output STB   : " + result.outputPath);
        log("File Size    : " + std::to_string(result.fileSize) + " bytes (" +
            std::to_string(result.fileSize / (1024 * 1024)) + " MB)");
        log("Maps Built   : " + std::to_string(result.mapsCompiled));
        log("BG Nodes     : " + std::to_string(result.totalNodes));
        log("BG Patches   : " + std::to_string(result.totalPatches));
        log("Elapsed Time : " + std::to_string(result.durationSeconds) + " seconds");

    } catch (const std::exception& ex) {
        result.success = false;
        result.errorMessage = ex.what();
        log("Compilation Error: " + result.errorMessage);
        reportProgress(1.0f, "Error: " + result.errorMessage);
    } catch (...) {
        result.success = false;
        result.errorMessage = "An unknown error occurred during STB compilation.";
        log("Compilation Error: " + result.errorMessage);
        reportProgress(1.0f, "Error: " + result.errorMessage);
    }

    return result;
}

} // namespace egostb
