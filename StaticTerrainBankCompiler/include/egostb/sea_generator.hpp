#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include "egostb/bbb_archive.hpp"

namespace egostb {

struct SeaDefConfig {
    float seaHeight = 3.25f;
    float seaRadius = 10000.0f;
    float seaFlatSectionStart = 5000.0f;
    float seaFlatSectionEnd = 6000.0f;
};

struct SeaMapInfo {
    std::string canonicalName;
    int32_t tlx = 0;
    int32_t tly = 0;
    int32_t brx = 0;
    int32_t bry = 0;
    std::vector<uint8_t> waterGrid; // (brx - tlx + 1) * (bry - tly + 1) or callback
    int32_t width = 0;
    int32_t height = 0;
};

struct SeaRegionJob {
    std::string regionName;
    std::string regionDef;
    std::vector<SeaMapInfo> cutoutMaps; // In order: containsMaps followed by seesMaps
};

class SeaGenerator {
public:
    // Parse engine.def, environment.def
    static SeaDefConfig parseDefs(const std::filesystem::path& defsDir);

    // Parse region.def to find definitions with HasSea TRUE
    static std::vector<std::string> parseHasSeaRegionDefs(const std::filesystem::path& defsDir);

    // Generate a single sea entry
    static BBBEntry generateSeaEntry(
        uint32_t entryId,
        const SeaRegionJob& job,
        const SeaDefConfig& defConfig,
        const std::vector<SeaMapInfo>& allWorldMaps
    );

    // Generate all sea entries in parallel
    static std::vector<BBBEntry> generateAllSeas(
        const std::vector<SeaRegionJob>& jobs,
        const SeaDefConfig& defConfig,
        const std::vector<SeaMapInfo>& allWorldMaps,
        int numThreads = 0,
        const std::function<void(const std::string&)>& logCallback = nullptr
    );
};

} // namespace egostb
