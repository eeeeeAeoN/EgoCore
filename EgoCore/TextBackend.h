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

static CTextParser g_TextParser;

// --- PERSISTENT STATE ---
static bool g_IsTextDirty = false;
static int g_LastEntryID = -1;
static void* g_LastBankPtr = nullptr;

// --- BACKGROUND AUDIO STATE ---
static std::map<std::string, std::shared_ptr<AudioBankParser>> g_BackgroundAudioBanks;

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

static std::string FormatAudioTime(float seconds) {
    int m = (int)seconds / 60;
    int s = (int)seconds % 60;
    int ms = (int)((seconds - (int)seconds) * 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%02d", m, s, ms);
    return std::string(buf);
}

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
    std::string key = bankName;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (g_BackgroundAudioBanks.count(key)) return g_BackgroundAudioBanks[key];

    std::string stem = bankName;
    size_t lastDot = stem.find_last_of('.');
    if (lastDot != std::string::npos) stem = stem.substr(0, lastDot);
    std::string filename = stem + ".lut";

    std::string root = g_AppConfig.GameRootPath;
    std::vector<std::string> candidates = {
        root + "\\Data\\Lang\\English\\" + filename,
        root + "\\Data\\" + filename,
        root + "\\Data\\Audio\\" + filename
    };

    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            auto parser = std::make_shared<AudioBankParser>();
            if (parser->Parse(path)) {
                g_BackgroundAudioBanks[key] = parser;
                return parser;
            }
        }
    }
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

        // --- CHANGED: Update MEMORY ONLY. Do NOT write to disk yet. ---

        // If we are currently viewing this header in the editor (not in defs mode), sync it too
        if (!g_DefWorkspace.ShowDefsMode && g_DefWorkspace.SelectedEnumIndex == enumIdx) {
            g_DefWorkspace.Editor.SetText(entry.FullContent);
            g_DefWorkspace.OriginalContent = entry.FullContent; // treat as saved/synced for editor logic
        }
    }
}

// Helper to save header from memory to disk when user saves text entry
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

inline void DeleteLinkedMedia(const std::string& speechBank, const std::string& identifier) {
    if (speechBank.empty() || identifier.empty()) return;

    int32_t soundID = ResolveAudioID(speechBank, identifier);
    if (soundID == -1) return;

    auto audioBank = GetOrLoadAudioBank(speechBank);
    if (audioBank) {
        for (int i = 0; i < (int)audioBank->Entries.size(); i++) {
            if (audioBank->Entries[i].SoundID == (uint32_t)soundID) {
                audioBank->DeleteEntry(i);
                break;
            }
        }
    }

    DeleteLipSyncEntry(speechBank, (uint32_t)soundID);

    std::string headerName = GetHeaderName(speechBank);
    int hIdx = FindHeaderIndex(headerName);
    if (hIdx != -1) {
        auto& entry = g_DefWorkspace.AllEnums[hIdx];
        std::string idSafe = identifier;
        std::string pattern = "\\s*(SND_|TEXT_SND_)" + idSafe + "\\s*=\\s*" + std::to_string(soundID) + "\\s*,?\\n?";
        std::regex re(pattern);
        entry.FullContent = std::regex_replace(entry.FullContent, re, "\n");

        // Sync Editor if open
        if (!g_DefWorkspace.ShowDefsMode && g_DefWorkspace.SelectedEnumIndex == hIdx) {
            g_DefWorkspace.Editor.SetText(entry.FullContent);
            g_DefWorkspace.OriginalContent = entry.FullContent;
        }
    }
}