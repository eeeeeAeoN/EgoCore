#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

class GtgMerger {
public:
    static void ProcessGtgMod(const std::string& modFile, const std::string& targetFile, const std::map<int, int>& remappedMapIds) {
        std::ifstream mIn(modFile, std::ios::binary);
        if (!mIn.is_open()) return;
        std::string modContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>());
        mIn.close();

        bool replaceFile = false;
        std::vector<std::string> uidsToDelete;
        std::string cleanedModContent = modContent;

        size_t settingsStart = modContent.find("[Settings]");
        if (settingsStart != std::string::npos) {
            size_t settingsEnd = modContent.find("NEWMAP", settingsStart);
            if (settingsEnd == std::string::npos) settingsEnd = modContent.length();

            std::string settingsBlock = modContent.substr(settingsStart, settingsEnd - settingsStart);
            cleanedModContent.erase(settingsStart, settingsEnd - settingsStart);

            if (settingsBlock.find("Replace=true") != std::string::npos ||
                settingsBlock.find("Replace=1") != std::string::npos) {
                replaceFile = true;
            }

            size_t deleteStart = settingsBlock.find("DeleteUIDs:");
            if (deleteStart != std::string::npos) {
                std::string uidList = settingsBlock.substr(deleteStart + 11);
                std::regex uidRegex(R"(\b(\d+)\b)");
                std::sregex_iterator begin(uidList.begin(), uidList.end(), uidRegex), end;
                for (auto it = begin; it != end; ++it) uidsToDelete.push_back((*it)[1].str());
            }
        }

        if (replaceFile) {
            std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
            out << cleanedModContent;
            out.close();
            return;
        }

        std::ifstream tIn(targetFile, std::ios::binary);
        if (!tIn.is_open()) return;
        std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>());
        tIn.close();

        // 1. Delete requested UIDs from target
        for (const auto& uid : uidsToDelete) {
            DeleteThingByUID(tContent, uid);
        }

        // 2. Find max NEWMAP ID in target
        int maxTargetMapId = 0;
        std::regex mapIdRegex(R"(NEWMAP\s+(\d+))", std::regex::icase);
        std::sregex_iterator begin(tContent.begin(), tContent.end(), mapIdRegex), end;
        for (auto it = begin; it != end; ++it) {
            int id = std::stoi((*it)[1].str());
            if (id > maxTargetMapId) maxTargetMapId = id;
        }

        // 3. Process each NEWMAP section in the mod
        size_t mCursor = 0;
        while (true) {
            size_t mapStart = cleanedModContent.find("NEWMAP", mCursor);
            if (mapStart == std::string::npos) break;
            size_t mapEnd = cleanedModContent.find("ENDMAP", mapStart);
            if (mapEnd == std::string::npos) break;

            std::string modMapBlock = cleanedModContent.substr(mapStart, (mapEnd + 6) - mapStart);
            mCursor = mapEnd + 6;

            // Extract original mod Map ID
            int modMapId = 0;
            std::smatch idMatch;
            if (std::regex_search(modMapBlock, idMatch, mapIdRegex)) {
                modMapId = std::stoi(idMatch[1].str());
            }

            // Remap ID if it was remapped by WldMerger
            int targetMapId = modMapId;
            auto itRemap = remappedMapIds.find(modMapId);
            if (itRemap != remappedMapIds.end()) {
                targetMapId = itRemap->second;
            }

            // Find target NEWMAP block: Hierarchy step 1 (NewMap)
            size_t tMapStart = std::string::npos;
            size_t tMapEnd = std::string::npos;
            FindMapBlock(tContent, targetMapId, tMapStart, tMapEnd);

            if (tMapStart != std::string::npos && tMapEnd != std::string::npos) {
                // Map exists in target: Merge sections and things
                MergeMapBlock(tContent, tMapStart, tMapEnd, modMapBlock);
            }
            else {
                // Map does not exist in target: Append new map block at bottom
                if (targetMapId <= 0) {
                    maxTargetMapId++;
                    targetMapId = maxTargetMapId;
                } else if (targetMapId > maxTargetMapId) {
                    maxTargetMapId = targetMapId;
                }

                // Update NEWMAP header to assigned ID
                std::string updatedModMapBlock = std::regex_replace(
                    modMapBlock,
                    std::regex(R"(NEWMAP\s+\d+)", std::regex::icase),
                    "NEWMAP " + std::to_string(targetMapId)
                );

                tContent.append("\n" + updatedModMapBlock + "\n");
            }
        }

        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
        out << tContent;
        out.close();
    }

private:
    static void FindMapBlock(const std::string& content, int mapId, size_t& outStart, size_t& outEnd) {
        outStart = std::string::npos;
        outEnd = std::string::npos;

        std::string searchPfx = "NEWMAP " + std::to_string(mapId);
        size_t pos = 0;
        while ((pos = content.find("NEWMAP", pos)) != std::string::npos) {
            // Check word boundary/number
            size_t numStart = pos + 6;
            while (numStart < content.length() && (content[numStart] == ' ' || content[numStart] == '\t')) numStart++;
            size_t numEnd = numStart;
            while (numEnd < content.length() && std::isdigit(content[numEnd])) numEnd++;

            if (numEnd > numStart) {
                int foundId = std::stoi(content.substr(numStart, numEnd - numStart));
                if (foundId == mapId) {
                    outStart = pos;
                    size_t endPos = content.find("ENDMAP", outStart);
                    if (endPos != std::string::npos) {
                        outEnd = endPos + 6;
                    }
                    return;
                }
            }
            pos += 6;
        }
    }

    // Hierarchy Step 2 & 3: NewSection -> NewThing
    static void MergeMapBlock(std::string& targetContent, size_t& tMapStart, size_t& tMapEnd, const std::string& modMapBlock) {
        size_t mSecCursor = 0;

        while (true) {
            size_t mSecStart = modMapBlock.find("XXXSectionStart", mSecCursor);
            if (mSecStart == std::string::npos) break;
            size_t mSecEnd = modMapBlock.find("XXXSectionEnd;", mSecStart);
            if (mSecEnd == std::string::npos) break;

            size_t nameStart = mSecStart + 16;
            size_t nameEnd = modMapBlock.find(";", nameStart);
            std::string secName = modMapBlock.substr(nameStart, nameEnd - nameStart);

            std::string modSectionContent = modMapBlock.substr(nameEnd + 1, mSecEnd - (nameEnd + 1));
            mSecCursor = mSecEnd + 14;

            // Search for this section inside the target map block
            std::string searchSec = "XXXSectionStart " + secName + ";";
            size_t tSecStart = targetContent.find(searchSec, tMapStart);

            if (tSecStart != std::string::npos && tSecStart < tMapEnd) {
                // Section exists in target map: merge NewThings
                size_t tSecEnd = targetContent.find("XXXSectionEnd;", tSecStart);
                if (tSecEnd != std::string::npos && tSecEnd <= tMapEnd) {
                    MergeSectionThings(targetContent, tSecStart, tSecEnd, tMapEnd, modSectionContent);
                }
            }
            else {
                // Section does NOT exist in target map: insert before ENDMAP
                size_t tEndMap = targetContent.rfind("ENDMAP", tMapEnd);
                if (tEndMap != std::string::npos && tEndMap >= tMapStart) {
                    std::string fullSection = "XXXSectionStart " + secName + ";\n" + modSectionContent + "XXXSectionEnd;\n\n";
                    targetContent.insert(tEndMap, fullSection);
                    tMapEnd += fullSection.length();
                }
            }
        }
    }

    // Hierarchy Step 3: NewThing matching and replacement / appending by UID
    static void MergeSectionThings(std::string& targetContent, size_t tSecStart, size_t& tSecEnd, size_t& tMapEnd, const std::string& modSectionContent) {
        size_t modCursor = 0;
        std::regex uidRegex(R"(UID\s+(\d+);)");

        while (true) {
            size_t nThingStart = modSectionContent.find("NewThing", modCursor);
            if (nThingStart == std::string::npos) break;
            size_t eThingEnd = modSectionContent.find("EndThing;", nThingStart);
            if (eThingEnd == std::string::npos) break;

            std::string modThing = modSectionContent.substr(nThingStart, (eThingEnd + 9) - nThingStart);
            modCursor = eThingEnd + 9;

            std::smatch match;
            if (std::regex_search(modThing, match, uidRegex)) {
                std::string uid = match[1].str();
                std::string uidSearch = "UID " + uid + ";";

                size_t tUidPos = targetContent.find(uidSearch, tSecStart);

                if (tUidPos != std::string::npos && tUidPos < tSecEnd) {
                    // Existing UID: Replace existing NewThing block
                    size_t tNewThing = targetContent.rfind("NewThing", tUidPos);
                    size_t tEndThing = targetContent.find("EndThing;", tUidPos);
                    if (tNewThing >= tSecStart && tEndThing <= tSecEnd) {
                        size_t oldLen = (tEndThing + 9) - tNewThing;
                        targetContent.replace(tNewThing, oldLen, modThing);

                        long long diff = (long long)modThing.length() - (long long)oldLen;
                        tSecEnd = (size_t)((long long)tSecEnd + diff);
                        tMapEnd = (size_t)((long long)tMapEnd + diff);
                    }
                }
                else {
                    // New UID: Insert right before XXXSectionEnd;
                    std::string toInsert = modThing + "\n";
                    targetContent.insert(tSecEnd, toInsert);
                    tSecEnd += toInsert.length();
                    tMapEnd += toInsert.length();
                }
            }
        }
    }

    static void DeleteThingByUID(std::string& targetContent, const std::string& uid) {
        std::string searchStr = "UID " + uid + ";";
        size_t uidPos = 0;
        while ((uidPos = targetContent.find(searchStr, uidPos)) != std::string::npos) {
            size_t newThingPos = targetContent.rfind("NewThing", uidPos);
            size_t endThingPos = targetContent.find("EndThing;", uidPos);

            if (newThingPos != std::string::npos && endThingPos != std::string::npos) {
                targetContent.erase(newThingPos, (endThingPos + 9) - newThingPos);
                uidPos = newThingPos;
            } else {
                uidPos += searchStr.length();
            }
        }
    }
};
