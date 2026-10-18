#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace egostb {

struct LevThemeSlot {
    std::string name;
    uint32_t used = 0;
};

struct HeightMapCell {
    float height = 0.0f;
    uint8_t themes[3] = {0, 0, 0};
    uint8_t blends[2] = {0, 0};
    uint8_t flags = 0;
};

struct LevData {
    int width = 128;
    int height = 128;
    int gridWidth = 129;
    int gridHeight = 129;
    uint32_t checksum = 0;
    std::vector<LevThemeSlot> themePalette; // 256 entries
    std::vector<std::string> soundThemes;
    std::vector<HeightMapCell> cells; // 129 * 129 = 16641 entries

    const HeightMapCell& getCell(int x, int y) const {
        int cx = std::clamp(x, 0, gridWidth - 1);
        int cy = std::clamp(y, 0, gridHeight - 1);
        return cells[static_cast<size_t>(cy) * gridWidth + cx];
    }
    HeightMapCell& getCell(int x, int y) {
        int cx = std::clamp(x, 0, gridWidth - 1);
        int cy = std::clamp(y, 0, gridHeight - 1);
        return cells[static_cast<size_t>(cy) * gridWidth + cx];
    }
};

class LevReader {
public:
    static LevData loadFromFile(const std::string& path);
};

} // namespace egostb
