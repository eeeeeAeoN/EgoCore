#include "egostb/lev_reader.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <iostream>

namespace egostb {

LevData LevReader::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open .lev file: " + path);
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (buffer.size() < 67639) {
        throw std::runtime_error("Invalid .lev file (too small): " + path);
    }

    LevData data;
    uint32_t mapW = 128, mapH = 128;
    if (buffer.size() >= 46) {
        std::memcpy(&mapW, buffer.data() + 38, 4);
        std::memcpy(&mapH, buffer.data() + 42, 4);
    }
    data.width = (mapW > 0 && mapW <= 1024) ? static_cast<int>(mapW) : 128;
    data.height = (mapH > 0 && mapH <= 1024) ? static_cast<int>(mapH) : 128;
    data.gridWidth = data.width + 1;
    data.gridHeight = data.height + 1;

    // Theme table starts at offset 47 (25 bytes level header + 22 bytes map header)
    size_t curr = 47;
    data.themePalette.resize(256);
    for (int i = 0; i < 256; ++i) {
        char nameBuf[129] = {0};
        std::memcpy(nameBuf, buffer.data() + curr, 128);
        data.themePalette[i].name = nameBuf;
        std::memcpy(&data.themePalette[i].used, buffer.data() + curr + 128, 4);
        curr += 132;
    }

    // Now look for Level Checksum
    // In dean_2003, it's at offset 67639
    // But let's find the sound themes start safely:
    // After themePalette (curr = 47 + 256*132 = 33839)
    // At offset 67639 is the 4-byte checksum, followed by sound themes
    size_t chkPos = 67639;
    if (chkPos + 4 <= buffer.size()) {
        std::memcpy(&data.checksum, buffer.data() + chkPos, 4);
    }

    curr = chkPos + 4;
    // Read sound themes
    // Each sound theme: 4-byte length + ASCII chars
    while (curr + 4 < buffer.size()) {
        uint32_t strLen = 0;
        std::memcpy(&strLen, buffer.data() + curr, 4);
        if (strLen == 0 || strLen > 256 || curr + 4 + strLen > buffer.size()) {
            break; // hit cell data
        }
        // check if ascii string
        bool isAscii = true;
        for (uint32_t c = 0; c < strLen; ++c) {
            uint8_t ch = buffer[curr + 4 + c];
            if (ch < 32 || ch >= 127) {
                isAscii = false;
                break;
            }
        }
        if (!isAscii) {
            break;
        }
        std::string s(reinterpret_cast<const char*>(buffer.data() + curr + 4), strLen);
        data.soundThemes.push_back(s);
        curr += 4 + strLen;
    }

    // Now HeightMap cells start at curr!
    // Total cells = 129 * 129 = 16,641
    size_t totalCells = data.gridWidth * data.gridHeight;
    data.cells.resize(totalCells);

    for (size_t idx = 0; idx < totalCells; ++idx) {
        if (curr + 4 > buffer.size()) {
            break;
        }
        uint32_t cellLen = 0;
        std::memcpy(&cellLen, buffer.data() + curr, 4);
        if (cellLen < 11 || curr + cellLen > buffer.size()) {
            break;
        }

        const uint8_t* p = buffer.data() + curr + 4;
        // p[0]: version (usually 7)
        // p[1..4]: float normalized height (multiply by 2048.0f to get world meters)
        float normHeight = 0.0f;
        std::memcpy(&normHeight, p + 1, 4);
        // CEngineMap::PeekLandscapeHeight quantizes to 1/128 (int(z * 128) * 0.0078125).
        // Round-to-nearest reproduces every vertex z in the retail STBs.
        data.cells[idx].height =
            std::floor(normHeight * 2048.0f * 128.0f + 0.5f) * 0.0078125f;

        // p[5]: flags / unused
        // p[6..8]: themes 0, 1, 2
        data.cells[idx].themes[0] = p[6];
        data.cells[idx].themes[1] = p[7];
        data.cells[idx].themes[2] = p[8];

        // p[9..10]: blends 0, 1
        data.cells[idx].blends[0] = p[9];
        data.cells[idx].blends[1] = p[10];

        curr += cellLen;
    }

    return data;
}

} // namespace egostb
