#pragma once
#include "BankBackend.h" 
#include "TextParser.h"
#include "BinaryParser.h" 
#include "DefBackend.h" 
#include "AudioBackend.h" 
#include "LipSyncProperties.h"
#include <string>
#include <vector>
#include <cstring>
#include <algorithm> 
#include <functional>
#include <regex>
#include <map>
#include <memory>
#include <filesystem>
#include <cmath>

inline CTextParser g_TextParser;

// --- PERSISTENT STATE ---
inline bool g_IsTextDirty = false;
inline int g_LastEntryID = -1;
inline void* g_LastBankPtr = nullptr;
// Moved here so BankEditor can track renames
inline std::string g_OriginalIdentifier = "";

// --- BACKGROUND AUDIO STATE ---
inline std::map<std::string, std::shared_ptr<AudioBankParser>> g_BackgroundAudioBanks;

// --- HELPERS ---

inline std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

inline std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

inline std::string PeekEntryName(LoadedBank* bank, uint32_t id) {
    if (!bank) return "Unknown";
    for (const auto& e : bank->Entries) {
        if (e.ID == id) return e.Name;
    }
    return "Unknown ID";
}

// --- AUDIO LOGIC HELPERS ---

inline std::string EnforceLugExtension(const std::string& bankName) {
    std::string fixed = bankName;
    size_t lastDot = fixed.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string ext = fixed.substr(lastDot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".lut" || ext == ".bin") {
            fixed = fixed.substr(0, lastDot) + ".lug";
        }
    }
    else if (!fixed.empty()) {
        fixed += ".lug";
    }
    return fixed;
}

static std::string FormatAudioTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

// Maps "SpeechBank" from text (e.g. "dialogue") to header file
inline std::string GetHeaderName(const std::string& speechBank) {
    std::string stem = speechBank;
    size_t lastDot = stem.find_last_of('.');
    if (lastDot != std::string::npos) stem = stem.substr(0, lastDot);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);

    if (stem == "dialogue")             return "dialoguesnds.h";
    if (stem == "dialogue2")            return "dialoguesnds2.h";
    if (stem == "scriptdialogue")       return "scriptdialoguesnds.h";
    if (stem == "scriptdialogue2")      return "scriptdialoguesnds2.h";
    return stem + "snds.h";
}

// Maps Internal SubBank Name (e.g. "LIPSYNC_ENGLISH_MAIN") to header file
inline std::string GetHeaderForSubBank(const std::string& subBankName) {
    if (subBankName == "LIPSYNC_ENGLISH_MAIN") return "dialoguesnds.h";
    if (subBankName == "LIPSYNC_ENGLISH_MAIN_2") return "dialoguesnds2.h";
    if (subBankName == "LIPSYNC_ENGLISH_SCRIPT") return "scriptdialoguesnds.h";
    if (subBankName == "LIPSYNC_ENGLISH_SCRIPT_2") return "scriptdialoguesnds2.h";
    return "";
}

inline int FindHeaderIndex(const std::string& headerName) {
    for (int i = 0; i < (int)g_DefWorkspace.AllEnums.size(); i++) {
        std::string path = g_DefWorkspace.AllEnums[i].FilePath;
        std::string fname = std::filesystem::path(path).filename().string();
        std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
        if (fname == headerName) return i;
    }
    return -1;
}

inline int32_t ResolveAudioID(const std::string& speechBank, const std::string& identifier) {
    if (speechBank.empty() || identifier.empty()) return -1;
    std::string headerName = GetHeaderName(speechBank);
    int enumIdx = FindHeaderIndex(headerName);
    if (enumIdx == -1) return -1;

    const std::string& content = g_DefWorkspace.AllEnums[enumIdx].FullContent;
    std::string idSafe = identifier;
    std::string patternStr = "(SND_|TEXT_SND_)" + idSafe + "\\s*=\\s*(\\d+)";
    std::regex re(patternStr, std::regex::icase);
    std::smatch match;
    if (std::regex_search(content, match, re)) {
        if (match.size() >= 3) {
            try { return std::stoi(match[2].str()); }
            catch (...) { return -1; }
        }
    }
    return -1;
}

// Reverse Lookup: ID -> Name (e.g. 20001 -> "SND_QUEST_START")
inline std::string ResolveNameFromID(const std::string& headerName, uint32_t id) {
    int enumIdx = FindHeaderIndex(headerName);
    if (enumIdx == -1) return "";

    const std::string& content = g_DefWorkspace.AllEnums[enumIdx].FullContent;
    std::string patternStr = "([A-Z0-9_]+)\\s*=\\s*" + std::to_string(id) + "[,\\s]";
    std::regex re(patternStr);
    std::smatch match;

    if (std::regex_search(content, match, re)) {
        if (match.size() >= 2) {
            return match[1].str();
        }
    }
    return "";
}

inline uint32_t GetNextIDFromHeader(const std::string& speechBank) {
    std::string headerName = GetHeaderName(speechBank);
    int idx = FindHeaderIndex(headerName);
    if (idx == -1) return 20000;

    const std::string& content = g_DefWorkspace.AllEnums[idx].FullContent;
    std::regex re("=\\s*(\\d+)");
    std::sregex_iterator next(content.begin(), content.end(), re);
    std::sregex_iterator end;

    uint32_t maxID = 0;
    while (next != end) {
        std::smatch match = *next;
        try {
            uint32_t val = std::stoul(match[1].str());
            if (val > maxID && val < 999999) maxID = val;
        }
        catch (...) {}
        next++;
    }
    return (maxID > 0) ? maxID + 1 : 20000;
}

inline std::shared_ptr<AudioBankParser> GetOrLoadAudioBank(const std::string& bankName) {
    // 1. NORMALIZE INPUT TO .LUT FILENAME
    // The text bank usually refers to banks as "Creature.lug" or just "Creature".
    // We need to standardize this to "creature.lut" for consistent matching.
    std::string searchKey = bankName;
    std::transform(searchKey.begin(), searchKey.end(), searchKey.begin(), ::tolower);

    // If it has .lug extension, swap it to .lut
    size_t lugPos = searchKey.find(".lug");
    if (lugPos != std::string::npos) {
        searchKey.replace(lugPos, 4, ".lut");
    }
    // If it has no extension, append .lut
    else if (searchKey.find(".") == std::string::npos) {
        searchKey += ".lut";
    }
    // Result is now guaranteed to be "filename.lut" (lowercase)

    // 2. CHECK OPEN TABS (CRITICAL)
    // If the user has this bank open in the editor, we MUST use that instance.
    // This ensures that if you have unsaved changes in the tab, the linker sees them.
    for (auto& b : g_OpenBanks) {
        if (b.Type == EBankType::Audio && b.AudioParser) {
            std::string bName = b.FileName;
            std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);

            if (bName == searchKey) {
                // Found it live in the UI!
                // Map it in the background cache so cascade compilation can find it easily later.
                g_BackgroundAudioBanks[searchKey] = b.AudioParser;
                return b.AudioParser;
            }
        }
    }

    // 3. CHECK BACKGROUND CACHE
    // If it's not in a tab, maybe we already loaded it in the background previously?
    if (g_BackgroundAudioBanks.count(searchKey)) {
        return g_BackgroundAudioBanks[searchKey];
    }

    // 4. LOAD FROM DISK (STRICT)
    // Only look in Data/Lang/English as requested.
    std::string path = g_AppConfig.GameRootPath + "\\Data\\Lang\\English\\" + searchKey;

    if (std::filesystem::exists(path)) {
        auto parser = std::make_shared<AudioBankParser>();
        if (parser->Parse(path)) {
            g_BackgroundAudioBanks[searchKey] = parser;
            return parser;
        }
    }

    // 5. FAIL
    // If not found in English, we return null. We do not fallback to other folders.
    return nullptr;
}

inline uint32_t GetMaxIDInAudioBank(std::shared_ptr<AudioBankParser> bank) {
    uint32_t maxID = 0;
    for (const auto& e : bank->Entries) {
        if (e.SoundID > maxID) maxID = e.SoundID;
    }
    return maxID;
}

inline void InjectHeaderDefinition(int enumIdx, const std::string& entryName, uint32_t id) {
    if (enumIdx < 0 || enumIdx >= g_DefWorkspace.AllEnums.size()) return;
    auto& entry = g_DefWorkspace.AllEnums[enumIdx];
    size_t closing = entry.FullContent.rfind("};");
    if (closing != std::string::npos) {
        std::string insertion = "\t" + entryName + " = " + std::to_string(id) + ",\n";
        entry.FullContent.insert(closing, insertion);

        if (!g_DefWorkspace.ShowDefsMode && g_DefWorkspace.SelectedEnumIndex == enumIdx) {
            g_DefWorkspace.Editor.SetText(entry.FullContent);
            g_DefWorkspace.OriginalContent = entry.FullContent;
        }
    }
}

inline void UpdateHeaderDefinition(const std::string& speechBank, const std::string& oldID, const std::string& newID) {
    if (speechBank.empty() || oldID.empty() || newID.empty()) return;

    std::string headerName = GetHeaderName(speechBank);
    int idx = FindHeaderIndex(headerName);
    if (idx == -1) return;

    auto& entry = g_DefWorkspace.AllEnums[idx];

    // Regex to find: (SND_ or TEXT_SND_) + oldID + (any spaces) + = + (any spaces) + (ID number)
    std::string patternStr = "(SND_|TEXT_SND_)" + oldID + "(\\s*=\\s*\\d+)";
    std::regex re(patternStr);

    // Replace with: $1 + newID + $2
    std::string replacement = "$1" + newID + "$2";

    std::string newContent = std::regex_replace(entry.FullContent, re, replacement);

    if (newContent != entry.FullContent) {
        entry.FullContent = newContent;
        if (!g_DefWorkspace.ShowDefsMode && g_DefWorkspace.SelectedEnumIndex == idx) {
            g_DefWorkspace.Editor.SetText(entry.FullContent);
            g_DefWorkspace.OriginalContent = entry.FullContent;
        }
    }
}

inline void SaveAssociatedHeader(const std::string& speechBank) {
    std::string hName = GetHeaderName(speechBank);
    int hIdx = FindHeaderIndex(hName);
    if (hIdx != -1) {
        auto& entry = g_DefWorkspace.AllEnums[hIdx];
        std::ofstream outFile(entry.FilePath, std::ios::binary);
        if (outFile.is_open()) {
            outFile << entry.FullContent;
            outFile.close();
        }
    }
}

// --- NEW HELPER: Remove Definition ---
inline void RemoveHeaderDefinition(const std::string& speechBank, const std::string& identifier) {
    if (speechBank.empty() || identifier.empty()) return;

    std::string headerName = GetHeaderName(speechBank);
    int idx = FindHeaderIndex(headerName);
    if (idx == -1) return;

    auto& entry = g_DefWorkspace.AllEnums[idx];

    std::string patternStr = "\\s*(SND_|TEXT_SND_)" + identifier + "\\s*=\\s*\\d+,?";
    std::regex re(patternStr);

    std::string newContent = std::regex_replace(entry.FullContent, re, "");

    if (newContent != entry.FullContent) {
        entry.FullContent = newContent;
        std::ofstream outFile(entry.FilePath, std::ios::binary);
        if (outFile.is_open()) {
            outFile << entry.FullContent;
            outFile.close();
        }
    }
}