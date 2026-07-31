#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <regex>
#include <iostream>
#include <set>

namespace fs = std::filesystem;

namespace NAnimationEvents
{
    struct CEvent
    {
        uint32_t EventID = 0;
        bool Flag1 = false;
        bool Flag2 = false;
        float Time = 0.0f;
    };

    struct CAnimRecord
    {
        uint32_t AnimID = 0xFFFFFFFF;
        std::vector<CEvent> Events;
    };

    class CBinaryCompiler
    {
    public:
        std::unordered_map<std::string, uint32_t> EnumSymbolTable;

        static std::string CreateCommentMaskedString(const std::string& input)
        {
            std::string masked = input;
            bool inBlockComment = false;
            bool inLineComment = false;
            bool inString = false;

            for (size_t i = 0; i < masked.length(); ++i) {
                if (inBlockComment) {
                    if (i + 1 < masked.length() && masked[i] == '*' && masked[i + 1] == '/') {
                        masked[i] = ' '; masked[i + 1] = ' '; inBlockComment = false; i++;
                    }
                    else if (masked[i] != '\n' && masked[i] != '\r') masked[i] = ' ';
                }
                else if (inLineComment) {
                    if (masked[i] == '\n') inLineComment = false;
                    else if (masked[i] != '\r') masked[i] = ' ';
                }
                else if (inString) {
                    if (masked[i] == '"' && (i == 0 || masked[i - 1] != '\\')) inString = false;
                }
                else {
                    if (masked[i] == '"') inString = true;
                    else if (i + 1 < masked.length() && masked[i] == '/' && masked[i + 1] == '*') {
                        masked[i] = ' '; masked[i + 1] = ' '; inBlockComment = true; i++;
                    }
                    else if (i + 1 < masked.length() && masked[i] == '/' && masked[i + 1] == '/') {
                        masked[i] = ' '; masked[i + 1] = ' '; inLineComment = true; i++;
                    }
                }
            }
            return masked;
        }

        static std::string StripPreprocessorDirectives(const std::string& input)
        {
            std::stringstream ss(input);
            std::string line, result;
            while (std::getline(ss, line))
            {
                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));

                if (trimmed.rfind("#", 0) == 0 && trimmed.rfind("#define", 0) != 0)
                {
                    result += "\n";
                }
                else
                {
                    result += line + "\n";
                }
            }
            return result;
        }

        void LoadHeaderFile(const std::string& headerPath)
        {
            std::ifstream file(headerPath, std::ios::binary);
            if (!file.is_open()) return;

            std::stringstream buffer;
            buffer << file.rdbuf();

            std::string cleanedContent = StripPreprocessorDirectives(CreateCommentMaskedString(buffer.str()));

            std::regex defineRegex(R"(#define\s+([A-Za-z0-9_]+)\s+((?:0x[0-9A-Fa-f]+|-?[0-9]+)))");
            auto defBegin = std::sregex_iterator(cleanedContent.begin(), cleanedContent.end(), defineRegex);
            auto defEnd = std::sregex_iterator();
            for (auto it = defBegin; it != defEnd; ++it)
            {
                std::string name = (*it)[1].str();
                std::string valStr = (*it)[2].str();
                uint32_t val = 0;
                try {
                    val = std::stoul(valStr, nullptr, 0);
                }
                catch (...) { continue; }

                std::string upperName = name;
                std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
                EnumSymbolTable[upperName] = val;
            }

            std::regex enumBlockRegex(R"(enum(?:\s+(?:class|struct))?\s*(?:[A-Za-z0-9_]+)?\s*(?::\s*[A-Za-z0-9_]+)?\s*\{([^}]*)\})");
            auto enumBegin = std::sregex_iterator(cleanedContent.begin(), cleanedContent.end(), enumBlockRegex);
            auto enumEnd = std::sregex_iterator();

            for (auto it = enumBegin; it != enumEnd; ++it)
            {
                std::string body = (*it)[1].str();
                std::stringstream ss(body);
                std::string token;
                uint32_t currentValue = 0;

                while (std::getline(ss, token, ','))
                {
                    token.erase(0, token.find_first_not_of(" \t\r\n"));
                    token.erase(token.find_last_not_of(" \t\r\n") + 1);
                    if (token.empty()) continue;

                    size_t eqPos = token.find('=');
                    std::string namePart = (eqPos != std::string::npos) ? token.substr(0, eqPos) : token;

                    if (eqPos != std::string::npos)
                    {
                        std::string valStr = token.substr(eqPos + 1);
                        valStr.erase(0, valStr.find_first_not_of(" \t\r\n"));
                        valStr.erase(valStr.find_last_not_of(" \t\r\n") + 1);

                        try {
                            currentValue = std::stoul(valStr, nullptr, 0);
                        }
                        catch (...) {
                            std::string upperVal = valStr;
                            std::transform(upperVal.begin(), upperVal.end(), upperVal.begin(), ::toupper);

                            if (EnumSymbolTable.count(upperVal)) {
                                currentValue = EnumSymbolTable[upperVal];
                            }
                        }
                    }

                    std::regex identRegex(R"([A-Za-z_][A-Za-z0-9_]*)");
                    std::smatch match;
                    if (std::regex_search(namePart, match, identRegex))
                    {
                        std::string name = match.str();
                        if (name != "force_dword" && name != "FORCE_DWORD" && name != "FORCE_32BIT" && name != "enum" && name != "class")
                        {
                            std::string upperName = name;
                            std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
                            EnumSymbolTable[upperName] = currentValue++;
                        }
                    }
                }
            }
        }

        void LoadAllHeaders(const std::string& rootPath)
        {
            EnumSymbolTable.clear();
            std::string searchPath = rootPath + "\\Data\\Defs";
            if (!fs::exists(searchPath)) searchPath = rootPath;

            std::set<std::string> visited;
            try
            {
                for (const auto& entry : fs::recursive_directory_iterator(searchPath))
                {
                    if (!entry.is_regular_file() || entry.path().extension() != ".h") continue;

                    std::string pathStr = entry.path().string();
                    std::string pathLower = pathStr;
                    std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);

                    if (pathLower.find("devheaders") != std::string::npos) continue;
                    if (pathLower.find("retailheaders") != std::string::npos && pathLower.find("xbox") != std::string::npos) continue;

                    if (visited.count(pathStr)) continue;
                    visited.insert(pathStr);

                    LoadHeaderFile(pathStr);
                }
            }
            catch (...) {}
        }

        uint32_t ResolveSymbol(const std::string& token, const std::string& prefix = "")
        {
            if (token.empty()) return 0xFFFFFFFF;

            bool isNumeric = true;
            uint32_t numVal = 0;
            try {
                if (token.find("0x") == 0 || token.find("0X") == 0)
                    numVal = std::stoul(token, nullptr, 16);
                else
                    numVal = std::stoul(token, nullptr, 10);
            }
            catch (...) {
                isNumeric = false;
            }
            if (isNumeric) return numVal;

            std::string upperToken = token;
            std::transform(upperToken.begin(), upperToken.end(), upperToken.begin(), ::toupper);

            auto it = EnumSymbolTable.find(upperToken);
            if (it != EnumSymbolTable.end()) return it->second;

            if (!prefix.empty())
            {
                std::string upperPrefix = prefix;
                std::transform(upperPrefix.begin(), upperPrefix.end(), upperPrefix.begin(), ::toupper);

                std::string prefixed = upperPrefix + upperToken;
                it = EnumSymbolTable.find(prefixed);
                if (it != EnumSymbolTable.end()) return it->second;
            }

            if (!prefix.empty())
            {
                std::string upperPrefix = prefix;
                std::transform(upperPrefix.begin(), upperPrefix.end(), upperPrefix.begin(), ::toupper);

                if (upperToken.rfind(upperPrefix, 0) == 0)
                {
                    std::string stripped = upperToken.substr(upperPrefix.length());
                    it = EnumSymbolTable.find(stripped);
                    if (it != EnumSymbolTable.end()) return it->second;
                }
            }

            return 0xFFFFFFFF;
        }

        std::vector<CAnimRecord> ParseTextFile(const std::string& textPath, const std::string& eventPrefix)
        {
            std::vector<CAnimRecord> records;
            std::ifstream file(textPath);
            if (!file.is_open()) return records;

            std::string line;
            CAnimRecord currentRecord;
            bool inEventBlock = false;

            while (std::getline(file, line))
            {
                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

                if (trimmed.empty() ||
                    trimmed.rfind("//", 0) == 0 ||
                    trimmed.rfind("#", 0) == 0 ||
                    trimmed == "BEGIN_ANIMATION_EVENTS" ||
                    trimmed == "END_ANIMATION_EVENTS")
                {
                    continue;
                }

                if (trimmed.rfind("BEGIN_EVENTS:", 0) == 0)
                {
                    std::string animSymbol = trimmed.substr(13);
                    animSymbol.erase(0, animSymbol.find_first_not_of(" \t"));

                    uint32_t resolvedAnimID = ResolveSymbol(animSymbol, "");
                    inEventBlock = true;
                    currentRecord = CAnimRecord();
                    currentRecord.AnimID = resolvedAnimID;
                }
                else if (trimmed == "END_EVENTS")
                {
                    if (inEventBlock)
                    {
                        records.push_back(currentRecord);
                        inEventBlock = false;
                    }
                }
                else if (inEventBlock)
                {
                    std::stringstream ss(trimmed);
                    std::string eventToken;
                    float timeVal = 0.0f;
                    int flag1Val = 0, flag2Val = 0;

                    if (ss >> eventToken >> timeVal)
                    {
                        uint32_t resolvedEventID = ResolveSymbol(eventToken, eventPrefix);
                        CEvent evt;
                        evt.EventID = resolvedEventID;
                        evt.Time = timeVal;

                        // Trailing token is normally the START/STOP keyword, which maps
                        // to Flag2 (the 0x80000000 bit: START -> false, STOP -> true).
                        // Fall back to numeric 0/1 flags for any other format variants.
                        std::string trailing;
                        if (ss >> trailing)
                        {
                            std::string upperTrailing = trailing;
                            std::transform(upperTrailing.begin(), upperTrailing.end(), upperTrailing.begin(), ::toupper);

                            if (upperTrailing == "START")
                            {
                                evt.Flag2 = false;
                            }
                            else if (upperTrailing == "STOP")
                            {
                                evt.Flag2 = true;
                            }
                            else
                            {
                                try { flag1Val = std::stoi(trailing); evt.Flag1 = (flag1Val != 0); }
                                catch (...) {}
                            }

                            if (ss >> flag2Val) evt.Flag2 = (flag2Val != 0);
                        }

                        currentRecord.Events.push_back(evt);
                    }
                }
            }
            return records;
        }

        // AnimToExtraEventsMap in the original game is a map keyed by AnimID, so
        // records are serialized in ascending AnimID order, not text-file order.
        // Duplicate BEGIN_EVENTS blocks for the same AnimID also collapse into a
        // single map entry, so merge them here to match.
        static std::vector<CAnimRecord> SortAndMergeRecords(const std::vector<CAnimRecord>& records)
        {
            std::vector<CAnimRecord> sorted = records;
            std::stable_sort(sorted.begin(), sorted.end(),
                [](const CAnimRecord& a, const CAnimRecord& b) { return a.AnimID < b.AnimID; });

            std::vector<CAnimRecord> merged;
            for (auto& rec : sorted)
            {
                if (!merged.empty() && merged.back().AnimID == rec.AnimID)
                {
                    merged.back().Events.insert(merged.back().Events.end(),
                        rec.Events.begin(), rec.Events.end());
                }
                else
                {
                    merged.push_back(rec);
                }
            }
            return merged;
        }

        bool SaveBinary(const std::vector<CAnimRecord>& recordsIn, const std::string& outBinPath)
        {
            std::vector<CAnimRecord> records = SortAndMergeRecords(recordsIn);

            std::ofstream binFile(outBinPath, std::ios::binary | std::ios::trunc);
            if (!binFile.is_open()) return false;

            auto WriteSLONG = [&binFile](int32_t val) { binFile.write(reinterpret_cast<const char*>(&val), sizeof(val)); };
            auto WriteULONG = [&binFile](uint32_t val) { binFile.write(reinterpret_cast<const char*>(&val), sizeof(val)); };
            auto WriteBYTE = [&binFile](uint8_t val) { binFile.write(reinterpret_cast<const char*>(&val), sizeof(val)); };
            auto WriteFloat = [&binFile](float val) { binFile.write(reinterpret_cast<const char*>(&val), sizeof(val)); };

            // 1. Total Record Count
            WriteSLONG(static_cast<int32_t>(records.size()));

            for (const auto& record : records)
            {
                // 2. Anim Enum ID (4 bytes)
                WriteSLONG(static_cast<int32_t>(record.AnimID));

                // 3. Event Count for this record (4 bytes)
                WriteSLONG(static_cast<int32_t>(record.Events.size()));

                // 4. Events Stream (10 bytes per event)
                for (const auto& evt : record.Events)
                {
                    WriteULONG(evt.EventID);                      // 4 bytes: Event Enum ID
                    WriteBYTE(evt.Flag1 ? 1 : 0);                 // 1 byte : Flag 1
                    WriteBYTE(evt.Flag2 ? 1 : 0);                 // 1 byte : Flag 2
                    WriteFloat(evt.Time);                         // 4 bytes: IEEE 754 Float Timestamp
                }
            }
            return true;
        }

        bool CompileAnimationEvents(const std::string& rootPath)
        {
            std::string soundTxt = rootPath + "\\Data\\Misc\\sound_animation_events.txt";
            std::string soundBin = rootPath + "\\Data\\Misc\\sound_animation_events.bin";
            std::string gameTxt = rootPath + "\\Data\\Misc\\game_animation_events.txt";
            std::string gameBin = rootPath + "\\Data\\Misc\\game_animation_events.bin";

            LoadAllHeaders(rootPath);

            if (fs::exists(soundTxt)) SaveBinary(ParseTextFile(soundTxt, "SOUND_ANIMATION_EVENT_"), soundBin);
            if (fs::exists(gameTxt))  SaveBinary(ParseTextFile(gameTxt, "GAME_ANIMATION_EVENT_"), gameBin);

            return true;
        }
    };
}