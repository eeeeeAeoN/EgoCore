#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

struct WldMergeResult {
    bool mapsChanged = false;           // True if any NewMap entry was added or modified (triggers STB rebuild)
    std::map<int, int> remappedMapIds;  // Maps mod's original NewMap ID -> assigned target NewMap ID
};

class WldMerger {
public:
    static WldMergeResult ProcessWldMod(const std::string& modFile, const std::string& targetFile) {
        WldMergeResult result;

        std::ifstream mIn(modFile, std::ios::binary);
        if (!mIn.is_open()) return result;
        std::string modContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>());
        mIn.close();

        // Check for [Settings] Replace=true/1
        bool replaceFile = false;
        std::string cleanedModContent = modContent;

        size_t settingsStart = modContent.find("[Settings]");
        if (settingsStart != std::string::npos) {
            size_t settingsEnd = modContent.find("START_INITIAL_QUESTS", settingsStart);
            if (settingsEnd == std::string::npos) {
                settingsEnd = modContent.find("NewMap", settingsStart);
            }
            if (settingsEnd == std::string::npos) {
                settingsEnd = modContent.find("NewRegion", settingsStart);
            }
            if (settingsEnd == std::string::npos) settingsEnd = modContent.length();

            std::string settingsBlock = modContent.substr(settingsStart, settingsEnd - settingsStart);
            cleanedModContent.erase(settingsStart, settingsEnd - settingsStart);

            if (settingsBlock.find("Replace=true") != std::string::npos ||
                settingsBlock.find("Replace=1") != std::string::npos) {
                replaceFile = true;
            }
        }

        if (replaceFile) {
            std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
            out << cleanedModContent;
            out.close();
            result.mapsChanged = true;
            return result;
        }

        std::ifstream tIn(targetFile, std::ios::binary);
        if (!tIn.is_open()) return result;
        std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>());
        tIn.close();

        // 1. Process INITIAL_QUESTS section
        ProcessQuests(cleanedModContent, tContent);

        // 2. Process NewMap entries
        ProcessMaps(cleanedModContent, tContent, result);

        // 3. Process NewRegion entries
        ProcessRegions(cleanedModContent, tContent);

        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
        out << tContent;
        out.close();

        return result;
    }

private:
    static std::string toLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return (char)::tolower(c); });
        return r;
    }

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string normalizeSeparators(const std::string& p) {
        std::string res = p;
        for (char& c : res) {
            if (c == '/') c = '\\';
        }
        return res;
    }

    // --- 1. INITIAL QUESTS ---
    static void ProcessQuests(const std::string& modContent, std::string& targetContent) {
        size_t mQStart = modContent.find("START_INITIAL_QUESTS");
        if (mQStart == std::string::npos) return;
        size_t mQEnd = modContent.find("END_INITIAL_QUESTS", mQStart);
        if (mQEnd == std::string::npos) return;

        size_t tQStart = targetContent.find("START_INITIAL_QUESTS");
        if (tQStart == std::string::npos) return;
        size_t tQEnd = targetContent.find("END_INITIAL_QUESTS", tQStart);
        if (tQEnd == std::string::npos) return;

        // Skip to after the newline following START_INITIAL_QUESTS
        size_t mLineEnd = modContent.find('\n', mQStart);
        if (mLineEnd == std::string::npos || mLineEnd >= mQEnd) return;

        std::string modQuestsBlock = modContent.substr(mLineEnd + 1, mQEnd - (mLineEnd + 1));
        std::istringstream stream(modQuestsBlock);
        std::string line;

        while (std::getline(stream, line)) {
            std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == '/') continue;

            // Delete Quest command: [DELETE]QuestName;
            if (trimmed.find("[DELETE]") == 0) {
                std::string questToDelete = trim(trimmed.substr(8));
                if (!questToDelete.empty()) {
                    if (questToDelete.back() != ';') questToDelete += ";";
                    size_t pos = targetContent.find(questToDelete, tQStart);
                    if (pos != std::string::npos && pos < tQEnd) {
                        size_t lineEnd = targetContent.find('\n', pos);
                        if (lineEnd == std::string::npos || lineEnd > tQEnd) lineEnd = pos + questToDelete.length();
                        else lineEnd += 1;
                        targetContent.erase(pos, lineEnd - pos);
                        tQEnd = targetContent.find("END_INITIAL_QUESTS", tQStart);
                    }
                }
            }
            else {
                // Add Quest command: QuestName;
                std::string questToAdd = trimmed;
                if (questToAdd.back() != ';') questToAdd += ";";

                // Check if already in target
                size_t pos = targetContent.find(questToAdd, tQStart);
                if (pos == std::string::npos || pos >= tQEnd) {
                    targetContent.insert(tQEnd, questToAdd + "\n");
                    tQEnd += questToAdd.length() + 1;
                }
            }
        }
    }

    // --- 2. MAP ENTRIES ---
    static void ProcessMaps(const std::string& modContent, std::string& targetContent, WldMergeResult& result) {
        // Find maximum existing Map ID in target
        int maxTargetMapId = 0;
        std::regex mapIdRegex(R"(NewMap\s+(\d+);)");
        std::sregex_iterator begin(targetContent.begin(), targetContent.end(), mapIdRegex), end;
        for (auto it = begin; it != end; ++it) {
            int id = std::stoi((*it)[1].str());
            if (id > maxTargetMapId) maxTargetMapId = id;
        }

        // Parse all NewMap blocks in the mod file
        size_t mCursor = 0;
        while (true) {
            size_t mapStart = modContent.find("NewMap", mCursor);
            if (mapStart == std::string::npos) break;
            size_t mapEnd = modContent.find("EndMap;", mapStart);
            if (mapEnd == std::string::npos) break;

            std::string modMapBlock = modContent.substr(mapStart, (mapEnd + 7) - mapStart);
            mCursor = mapEnd + 7;

            // Extract mod's original Map ID
            int modMapId = 0;
            std::smatch idMatch;
            if (std::regex_search(modMapBlock, idMatch, mapIdRegex)) {
                modMapId = std::stoi(idMatch[1].str());
            }

            // Extract LevelName from mod's block
            std::regex levRegex("LevelName\\s+\"([^\"]+)\";", std::regex::icase);
            std::smatch levMatch;
            if (!std::regex_search(modMapBlock, levMatch, levRegex)) continue;

            std::string modLevPath = normalizeSeparators(toLower(trim(levMatch[1].str())));

            // Check if LevelName already exists in target
            bool mapExists = false;
            size_t tCursor = 0;
            size_t existingStart = std::string::npos;
            size_t existingEnd = std::string::npos;

            while (true) {
                size_t tMapStart = targetContent.find("NewMap", tCursor);
                if (tMapStart == std::string::npos) break;
                size_t tMapEnd = targetContent.find("EndMap;", tMapStart);
                if (tMapEnd == std::string::npos) break;

                std::string tMapBlock = targetContent.substr(tMapStart, (tMapEnd + 7) - tMapStart);
                std::smatch tLevMatch;
                if (std::regex_search(tMapBlock, tLevMatch, levRegex)) {
                    std::string tLevPath = normalizeSeparators(toLower(trim(tLevMatch[1].str())));
                    if (tLevPath == modLevPath) {
                        mapExists = true;
                        existingStart = tMapStart;
                        existingEnd = tMapEnd + 7;

                        std::smatch tIdMatch;
                        if (std::regex_search(tMapBlock, tIdMatch, mapIdRegex)) {
                            int existingTargetId = std::stoi(tIdMatch[1].str());
                            result.remappedMapIds[modMapId] = existingTargetId;
                        }
                        break;
                    }
                }
                tCursor = tMapEnd + 7;
            }

            if (mapExists) {
                // Overwrite the existing map entry in target, preserving target's original Map ID
                int targetId = result.remappedMapIds[modMapId];
                std::string updatedModMapBlock = std::regex_replace(
                    modMapBlock,
                    std::regex(R"(NewMap\s+\d+;)"),
                    "NewMap " + std::to_string(targetId) + ";"
                );
                targetContent.replace(existingStart, existingEnd - existingStart, updatedModMapBlock);
                result.mapsChanged = true;
            }
            else {
                // Append new map at the end of the NewMap section (before NewRegion 1;)
                maxTargetMapId++;
                int assignedId = maxTargetMapId;
                result.remappedMapIds[modMapId] = assignedId;

                // Replace the mod's declared NewMap <modMapId>; with NewMap <assignedId>;
                std::string updatedModMapBlock = std::regex_replace(
                    modMapBlock,
                    std::regex(R"(NewMap\s+\d+;)"),
                    "NewMap " + std::to_string(assignedId) + ";"
                );

                // Find insertion position: right before the first NewRegion entry
                size_t regionPos = targetContent.find("NewRegion");
                if (regionPos != std::string::npos) {
                    targetContent.insert(regionPos, updatedModMapBlock + "\n\n");
                }
                else {
                    // Fallback if no NewRegion exists in file
                    targetContent.append("\n" + updatedModMapBlock + "\n");
                }

                result.mapsChanged = true;
            }
        }
    }

    // --- 3. REGION ENTRIES ---
    static void ProcessRegions(const std::string& modContent, std::string& targetContent) {
        // Find maximum existing Region ID in target
        int maxTargetRegionId = 0;
        std::regex regionIdRegex(R"(NewRegion\s+(\d+);)");
        std::sregex_iterator begin(targetContent.begin(), targetContent.end(), regionIdRegex), end;
        for (auto it = begin; it != end; ++it) {
            int id = std::stoi((*it)[1].str());
            if (id > maxTargetRegionId) maxTargetRegionId = id;
        }

        // Parse all NewRegion blocks in the mod file
        size_t mCursor = 0;
        std::regex regNameRegex("RegionName\\s+\"([^\"]+)\";", std::regex::icase);

        while (true) {
            size_t regStart = modContent.find("NewRegion", mCursor);
            if (regStart == std::string::npos) break;
            size_t regEnd = modContent.find("EndRegion;", regStart);
            if (regEnd == std::string::npos) break;

            std::string modRegBlock = modContent.substr(regStart, (regEnd + 10) - regStart);
            mCursor = regEnd + 10;

            std::smatch regMatch;
            if (!std::regex_search(modRegBlock, regMatch, regNameRegex)) continue;
            std::string modRegName = toLower(trim(regMatch[1].str()));

            // Check if RegionName matches existing in target
            bool regionExists = false;
            size_t tCursor = 0;
            size_t existingStart = std::string::npos;
            size_t existingEnd = std::string::npos;
            int existingTargetRegId = 0;

            while (true) {
                size_t tRegStart = targetContent.find("NewRegion", tCursor);
                if (tRegStart == std::string::npos) break;
                size_t tRegEnd = targetContent.find("EndRegion;", tRegStart);
                if (tRegEnd == std::string::npos) break;

                std::string tRegBlock = targetContent.substr(tRegStart, (tRegEnd + 10) - tRegStart);
                std::smatch tRegMatch;
                if (std::regex_search(tRegBlock, tRegMatch, regNameRegex)) {
                    std::string tRegName = toLower(trim(tRegMatch[1].str()));
                    if (tRegName == modRegName) {
                        regionExists = true;
                        existingStart = tRegStart;
                        existingEnd = tRegEnd + 10;

                        std::smatch tRegIdMatch;
                        if (std::regex_search(tRegBlock, tRegIdMatch, regionIdRegex)) {
                            existingTargetRegId = std::stoi(tRegIdMatch[1].str());
                        }
                        break;
                    }
                }
                tCursor = tRegEnd + 10;
            }

            if (regionExists) {
                // Replace matching region, preserving existing target region ID
                std::string updatedModRegBlock = modRegBlock;
                if (existingTargetRegId > 0) {
                    updatedModRegBlock = std::regex_replace(
                        modRegBlock,
                        std::regex(R"(NewRegion\s+\d+;)"),
                        "NewRegion " + std::to_string(existingTargetRegId) + ";"
                    );
                }
                targetContent.replace(existingStart, existingEnd - existingStart, updatedModRegBlock);
            }
            else {
                // Append new region at the end of the file with MaxID + 1
                maxTargetRegionId++;
                int assignedId = maxTargetRegionId;

                std::string updatedModRegBlock = std::regex_replace(
                    modRegBlock,
                    std::regex(R"(NewRegion\s+\d+;)"),
                    "NewRegion " + std::to_string(assignedId) + ";"
                );

                targetContent.append("\n" + updatedModRegBlock + "\n");
            }
        }
    }
};
