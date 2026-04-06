#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>

namespace fs = std::filesystem;

class TngMerger {
public:
    static void ProcessTngMod(const std::string& modFile, const std::string& targetFile) {
        std::ifstream mIn(modFile, std::ios::binary);
        if (!mIn.is_open()) return;
        std::string mContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>());
        mIn.close();

        bool replaceFile = false;
        std::vector<std::string> uidsToDelete;
        std::string cleanedModContent = mContent;

        size_t settingsStart = mContent.find("[Settings]");
        if (settingsStart != std::string::npos) {
            size_t settingsEnd = mContent.find("XXXSectionStart", settingsStart);
            if (settingsEnd == std::string::npos) settingsEnd = mContent.length();

            std::string settingsBlock = mContent.substr(settingsStart, settingsEnd - settingsStart);
            cleanedModContent.erase(settingsStart, settingsEnd - settingsStart);

            if (settingsBlock.find("Replace=true") != std::string::npos || settingsBlock.find("Replace=1") != std::string::npos) {
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

        for (const auto& uid : uidsToDelete) {
            DeleteThingByUID(tContent, uid);
        }

        size_t mCursor = 0;
        while (true) {
            size_t secStart = cleanedModContent.find("XXXSectionStart", mCursor);
            if (secStart == std::string::npos) break;
            size_t secEnd = cleanedModContent.find("XXXSectionEnd;", secStart);
            if (secEnd == std::string::npos) break;

            size_t nameStart = secStart + 16;
            size_t nameEnd = cleanedModContent.find(";", nameStart);
            std::string secName = cleanedModContent.substr(nameStart, nameEnd - nameStart);

            std::string modSection = cleanedModContent.substr(nameEnd + 1, secEnd - (nameEnd + 1));

            MergeSection(tContent, secName, modSection);

            mCursor = secEnd + 14;
        }

        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
        out << tContent;
        out.close();
    }

private:
    static void DeleteThingByUID(std::string& targetContent, const std::string& uid) {
        std::string searchStr = "UID " + uid + ";";
        size_t uidPos = targetContent.find(searchStr);
        if (uidPos == std::string::npos) return;

        size_t newThingPos = targetContent.rfind("NewThing", uidPos);
        size_t endThingPos = targetContent.find("EndThing;", uidPos);

        if (newThingPos != std::string::npos && endThingPos != std::string::npos) {
            targetContent.erase(newThingPos, (endThingPos + 9) - newThingPos);
        }
    }

    static void MergeSection(std::string& targetContent, const std::string& secName, const std::string& modSection) {
        std::string searchSec = "XXXSectionStart " + secName + ";";
        size_t tSecStart = targetContent.find(searchSec);
        if (tSecStart == std::string::npos) return;

        size_t tSecEnd = targetContent.find("XXXSectionEnd;", tSecStart);
        if (tSecEnd == std::string::npos) return;

        size_t modCursor = 0;
        while (true) {
            size_t nThingStart = modSection.find("NewThing", modCursor);
            if (nThingStart == std::string::npos) break;
            size_t eThingEnd = modSection.find("EndThing;", nThingStart);
            if (eThingEnd == std::string::npos) break;

            std::string modThing = modSection.substr(nThingStart, (eThingEnd + 9) - nThingStart);

            std::regex uidRegex(R"(UID\s+(\d+);)");
            std::smatch match;
            if (std::regex_search(modThing, match, uidRegex)) {
                std::string uid = match[1].str();
                std::string uidSearch = "UID " + uid + ";";

                size_t tUidPos = targetContent.find(uidSearch, tSecStart);

                if (tUidPos != std::string::npos && tUidPos < tSecEnd) {
                    size_t tNewThing = targetContent.rfind("NewThing", tUidPos);
                    size_t tEndThing = targetContent.find("EndThing;", tUidPos);
                    if (tNewThing >= tSecStart && tEndThing <= tSecEnd) {
                        targetContent.replace(tNewThing, (tEndThing + 9) - tNewThing, modThing);
                        tSecEnd = targetContent.find("XXXSectionEnd;", tSecStart);
                    }
                }
                else {
                    targetContent.insert(tSecEnd, modThing + "\n");
                    tSecEnd += modThing.length() + 1;
                }
            }
            modCursor = eThingEnd + 9;
        }
    }
};