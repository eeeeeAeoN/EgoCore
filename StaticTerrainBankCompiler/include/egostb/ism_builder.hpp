#pragma once

#include "egostb/lev_reader.hpp"
#include "egostb/bbb_archive.hpp"
#include "egostb/theme_manager.hpp"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace egostb {

struct IsmBuildResult {
    uint32_t edgeHeightFileSize = 0;
    uint32_t shorePointArrayStart = 0;
    uint32_t shorePointArraySize = 0;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> entryPayload;
    std::vector<std::pair<int, int>> shorePoints;
    std::filesystem::path writtenPath;
};

class IsmBuilder {
public:
    static float peekWaterDepth(const LevData& lev, int gx, int gy) {
        if (gx < 0 || gx >= lev.gridWidth || gy < 0 || gy >= lev.gridHeight) return 0.0f;
        const auto& cell = lev.getCell(gx, gy);
        float depth = 0.0f;
        for (int t = 0; t < 3; ++t) {
            uint8_t thIdx = cell.themes[t];
            if (thIdx < lev.themePalette.size()) {
                const auto& name = lev.themePalette[thIdx].name;
                if (!name.empty()) {
                    const auto* def = ThemeManager::instance().getTheme(name);
                    if (def && def->waterType > 0) {
                        float blend = 0.0f;
                        if (t == 0) {
                            blend = static_cast<float>(cell.blends[0]) / 255.0f;
                        } else if (t == 1) {
                            blend = static_cast<float>(cell.blends[1]) / 255.0f;
                        } else {
                            int b0 = cell.blends[0];
                            int b1 = cell.blends[1];
                            int rem = 255 - b0 - b1;
                            if (rem > 0) blend = static_cast<float>(rem) / 255.0f;
                        }
                        depth += def->waterHeight * blend;
                    }
                }
            }
        }
        return depth;
    }

    static bool peekInterpolatedHasWater(const LevData& lev, int gx, int gy, int distanceToLook = 1) {
        int minX = std::max(0, gx - distanceToLook);
        int maxX = std::min(lev.gridWidth - 1, gx + distanceToLook);
        int minY = std::max(0, gy - distanceToLook);
        int maxY = std::min(lev.gridHeight - 1, gy + distanceToLook);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (peekWaterDepth(lev, x, y) >= 0.001f) {
                    return true;
                }
            }
        }
        return false;
    }

    static std::vector<std::pair<int, int>> findShorePointsInMap(
        const LevData& lev, int worldX, int worldY)
    {
        std::vector<std::pair<int, int>> shorePoints;
        const int mapW = lev.width;
        const int mapH = lev.height;
        const int maxW = mapW - 1;
        const int maxH = mapH - 1;

        for (int cy = 0; cy < mapH; ++cy) {
            for (int cx = 0; cx < mapW; ++cx) {
                if (!peekInterpolatedHasWater(lev, cx, cy, 1)) {
                    int minX = std::max(0, cx - 1);
                    int maxX = std::min(maxW, cx + 1);
                    int minY = std::max(0, cy - 1);
                    int maxY = std::min(maxH, cy + 1);
                    bool isShore = false;
                    for (int ny = minY; ny <= maxY && !isShore; ++ny) {
                        for (int nx = minX; nx <= maxX && !isShore; ++nx) {
                            if (peekInterpolatedHasWater(lev, nx, ny, 1)) {
                                isShore = true;
                            }
                        }
                    }
                    if (isShore) {
                        shorePoints.push_back({worldX + cx, worldY + cy});
                    }
                } else {
                    // Check if cell has dummy shore point theme
                    const auto& cell = lev.getCell(cx, cy);
                    bool isDummyShore = false;
                    for (int t = 0; t < 3; ++t) {
                        uint8_t thIdx = cell.themes[t];
                        if (thIdx < lev.themePalette.size()) {
                            const auto* def = ThemeManager::instance().getTheme(lev.themePalette[thIdx].name);
                            if (def && def->waterType == 7) { // WATER_TYPE_DUMMY_SHORE_POINT
                                isDummyShore = true;
                                break;
                            }
                        }
                    }
                    if (isDummyShore) {
                        std::pair<int, int> pt = {worldX + cx, worldY + cy};
                        if (std::find(shorePoints.begin(), shorePoints.end(), pt) == shorePoints.end()) {
                            shorePoints.push_back(pt);
                        }
                    } else if (cx == 0 || cx == maxW || cy == 0 || cy == maxH) {
                        int ptX = worldX + cx;
                        int ptY = worldY + cy;
                        if (cx == maxW) ptX++;
                        else if (cy == maxH) ptY++;
                        shorePoints.push_back({ptX, ptY});
                    }
                }
            }
        }
        return shorePoints;
    }

    static IsmBuildResult build(
        const LevData& lev,
        int worldX,
        int worldY,
        const std::string& canonicalName,
        const std::filesystem::path& outIsmPath = "")
    {
        IsmBuildResult result;

        const int width = lev.width;
        const int height = lev.height;

        std::vector<float> leftHeights(4 * height, 0.0f);
        std::vector<float> rightHeights(4 * height, 0.0f);
        std::vector<float> topHeights(4 * width, 0.0f);
        std::vector<float> bottomHeights(4 * width, 0.0f);

        std::vector<uint8_t> leftThemes(12 * height, 0);
        std::vector<uint8_t> rightThemes(12 * height, 0);
        std::vector<uint8_t> topThemes(12 * width, 0);
        std::vector<uint8_t> bottomThemes(12 * width, 0);

        std::vector<uint8_t> leftThemeBlends(8 * height, 0);
        std::vector<uint8_t> rightThemeBlends(8 * height, 0);
        std::vector<uint8_t> topThemeBlends(8 * width, 0);
        std::vector<uint8_t> bottomThemeBlends(8 * width, 0);

        std::vector<std::string> themeNamesPalette;
        std::vector<uint32_t> themeIDPalette;

        auto registerTheme = [&](uint8_t levSlot) -> uint8_t {
            std::string themeName;
            uint32_t themeId = 0;
            if (levSlot < lev.themePalette.size()) {
                themeName = lev.themePalette[levSlot].name;
                themeId = lev.themePalette[levSlot].used;
            }
            if (themeName.empty()) {
                themeId = 0;
            }
            for (size_t i = 0; i < themeNamesPalette.size(); ++i) {
                if (themeNamesPalette[i] == themeName) {
                    return static_cast<uint8_t>(i);
                }
            }
            uint8_t newIdx = static_cast<uint8_t>(themeNamesPalette.size());
            themeNamesPalette.push_back(themeName);
            themeIDPalette.push_back(themeId);
            return newIdx;
        };

        // 1. Scan Top & Bottom strips (matches retail ReadHeights loop order)
        for (int x = 0; x < width; ++x) {
            for (int ry = 0; ry < 4; ++ry) {
                int topY = ry;
                int botY = height - 4 + ry;

                // Top cell: (x, topY)
                const auto& topCell = lev.getCell(x, topY);
                topHeights[x + ry * width] = static_cast<float>(static_cast<int>(topCell.height * 128.0f)) * 0.0078125f;
                for (int i = 0; i < 3; ++i) {
                    topThemes[3 * (x + ry * width) + i] = registerTheme(topCell.themes[i]);
                }
                topThemeBlends[2 * (x + ry * width) + 0] = topCell.blends[0];
                topThemeBlends[2 * (x + ry * width) + 1] = topCell.blends[1];

                // Bottom cell: (x, botY)
                const auto& botCell = lev.getCell(x, botY);
                bottomHeights[x + ry * width] = static_cast<float>(static_cast<int>(botCell.height * 128.0f)) * 0.0078125f;
                for (int i = 0; i < 3; ++i) {
                    bottomThemes[3 * (x + ry * width) + i] = registerTheme(botCell.themes[i]);
                }
                bottomThemeBlends[2 * (x + ry * width) + 0] = botCell.blends[0];
                bottomThemeBlends[2 * (x + ry * width) + 1] = botCell.blends[1];
            }
        }

        // 2. Scan Left & Right strips
        for (int rx = 0; rx < 4; ++rx) {
            int leftX = rx;
            int rightX = width - 4 + rx;
            for (int y = 0; y < height; ++y) {
                // Left cell: (leftX, y)
                const auto& leftCell = lev.getCell(leftX, y);
                leftHeights[y + rx * height] = static_cast<float>(static_cast<int>(leftCell.height * 128.0f)) * 0.0078125f;
                for (int i = 0; i < 3; ++i) {
                    leftThemes[3 * (y + rx * height) + i] = registerTheme(leftCell.themes[i]);
                }
                leftThemeBlends[2 * (y + rx * height) + 0] = leftCell.blends[0];
                leftThemeBlends[2 * (y + rx * height) + 1] = leftCell.blends[1];

                // Right cell: (rightX, y)
                const auto& rightCell = lev.getCell(rightX, y);
                rightHeights[y + rx * height] = static_cast<float>(static_cast<int>(rightCell.height * 128.0f)) * 0.0078125f;
                for (int i = 0; i < 3; ++i) {
                    rightThemes[3 * (y + rx * height) + i] = registerTheme(rightCell.themes[i]);
                }
                rightThemeBlends[2 * (y + rx * height) + 0] = rightCell.blends[0];
                rightThemeBlends[2 * (y + rx * height) + 1] = rightCell.blends[1];
            }
        }

        // Assemble Part A: CEngineStaticMapEdgeHeights::Save
        std::vector<uint8_t>& payload = result.payload;

        auto writeI32 = [&](int32_t val) {
            size_t cur = payload.size();
            payload.resize(cur + 4);
            std::memcpy(payload.data() + cur, &val, 4);
        };

        auto writeFloats = [&](const std::vector<float>& vec) {
            size_t cur = payload.size();
            payload.resize(cur + vec.size() * 4);
            std::memcpy(payload.data() + cur, vec.data(), vec.size() * 4);
        };

        auto writeBytes = [&](const std::vector<uint8_t>& vec) {
            payload.insert(payload.end(), vec.begin(), vec.end());
        };

        writeI32(width);
        writeI32(height);

        writeFloats(leftHeights);
        writeFloats(rightHeights);
        writeFloats(topHeights);
        writeFloats(bottomHeights);

        writeBytes(leftThemes);
        writeBytes(rightThemes);
        writeBytes(topThemes);
        writeBytes(bottomThemes);

        writeBytes(leftThemeBlends);
        writeBytes(rightThemeBlends);
        writeBytes(topThemeBlends);
        writeBytes(bottomThemeBlends);

        writeI32(static_cast<int32_t>(themeNamesPalette.size()));
        for (size_t i = 0; i < themeNamesPalette.size(); ++i) {
            writeI32(static_cast<int32_t>(themeIDPalette[i]));
            const std::string& name = themeNamesPalette[i];
            payload.insert(payload.end(), name.begin(), name.end());
            payload.push_back('\0'); // CDataOutputStream::WriteNullTerminatedString
        }

        result.edgeHeightFileSize = static_cast<uint32_t>(payload.size());

        // Assemble Part B: CEngineMap::SaveShorePointsToStream
        result.shorePointArrayStart = result.edgeHeightFileSize;
        result.shorePoints = findShorePointsInMap(lev, worldX, worldY);

        writeI32(static_cast<int32_t>(result.shorePoints.size()));
        for (const auto& pt : result.shorePoints) {
            writeI32(pt.first);
            writeI32(pt.second);
        }
        result.shorePointArraySize = static_cast<uint32_t>(payload.size() - result.shorePointArrayStart);
        result.entryPayload = payload;

        // Optionally write out BBB archive to outIsmPath
        if (!outIsmPath.empty()) {
            std::string outPathStr = outIsmPath.string();
            // Never overwrite reference files!
            if (outPathStr.find("_reference.ism") == std::string::npos) {
                BBBArchive archive;
                archive.version1 = 115;
                archive.version2 = 174;
                archive.version3 = 1;
                archive.alignment = 2048;

                BBBEntry entry;
                entry.id = 1;
                entry.type = 0;
                entry.devId = 0;
                entry.devType = 1;
                entry.name = canonicalName;
                entry.devName = canonicalName;
                entry.extra = {};
                entry.payload = payload;

                archive.entries.push_back(std::move(entry));

                if (outIsmPath.has_parent_path()) {
                    std::filesystem::create_directories(outIsmPath.parent_path());
                }
                archive.write(outIsmPath);
                result.writtenPath = outIsmPath;
            }
        }

        return result;
    }
};

} // namespace egostb
