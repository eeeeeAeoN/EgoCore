#pragma once
#include "Utils.h"
#include "MeshParser.h"
#include "BBMParser.h"
#include "AnimParser.h"
#include "AnimProperties.h"
#include "TextureParser.h"
#include "AudioBackend.h" 
#include "TextParser.h" 
#include "LugParser.h"
#include "MetParser.h"
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>
#include <set>
#include <memory>

namespace fs = std::filesystem;

// --- SIMPLE UI STATE ---
inline bool g_ShowSuccessPopup = false;
inline std::string g_SuccessMessage = "";

enum class EBankType {
    Unknown, Graphics, Textures, Frontend, Effects, Text, Dialogue, Fonts, Shaders, Audio
};

struct BankEntry {
    uint32_t ID = 0;
    std::string Name;
    std::string FriendlyName;
    int32_t Type = 0;
    uint32_t Offset = 0;
    uint32_t Size = 0;
    uint32_t CRC = 0;
    uint32_t InfoSize = 0;
    uint32_t SubheaderFileOffset = 0;
    uint32_t Timestamp = 0; // [NEW] Preserved timestamp/reserved field

    std::vector<std::string> Dependencies;
};

struct InternalBankInfo {
    std::string Name;
    uint32_t Version;
    uint32_t EntryCount;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Align;
    std::vector<uint32_t> HeaderData; // [NEW] Preserves the "statsCount" block
};

// NEW: Filter Modes
enum class EFilterMode { Name, ID, Speaker };

struct LoadedBank {
    std::string FileName;
    std::string FullPath;
    EBankType Type = EBankType::Unknown;

    uint32_t FileVersion = 2;

    std::vector<InternalBankInfo> SubBanks;
    int ActiveSubBankIndex = -1;
    std::vector<BankEntry> Entries;
    std::vector<int> FilteredIndices;

    std::unique_ptr<std::fstream> Stream;
    std::shared_ptr<AudioBankParser> AudioParser;
    std::shared_ptr<LugParser> LugParserPtr;
    std::shared_ptr<MetParser> MetParserPtr;

    int SelectedEntryIndex = -1;
    int SelectedLOD = 0;

    // --- UPDATED FILTER STATE ---
    char FilterText[128] = "";
    EFilterMode FilterMode = EFilterMode::Name; // Default: Name
    int FilterTypeMask = -1; // -1: All, 0: Text, 1: Group, 2: Narrator

    std::map<int, std::vector<uint8_t>> SubheaderCache;
    std::vector<uint8_t> CurrentEntryRawData;
    std::map<int, std::vector<uint8_t>> ModifiedEntryData;

    LoadedBank() {
        Stream = std::make_unique<std::fstream>();
    }

    LoadedBank(const LoadedBank&) = delete;
    LoadedBank& operator=(const LoadedBank&) = delete;
    LoadedBank(LoadedBank&&) = default;
    LoadedBank& operator=(LoadedBank&&) = default;
};

// --- GLOBALS ---
inline std::vector<LoadedBank> g_OpenBanks;
inline int g_ActiveBankIndex = -1;
inline bool g_ForceTabSwitch = false;
inline std::string g_BankStatus = "Ready";

inline C3DMeshContent g_ActiveMeshContent;
inline CBBMParser g_BBMParser;
inline CTextureParser g_TextureParser;
inline bool g_MeshUploadNeeded = false;
inline C3DAnimationInfo g_ActiveAnim;
inline AnimUIContext    g_AnimUIState;
inline bool             g_AnimParseSuccess = false;

// --- UTILS ---
inline bool StartsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) return false;
    return str.compare(0, prefix.length(), prefix) == 0;
}

inline bool IsSupportedMesh(int32_t type) {
    return type == TYPE_STATIC_MESH || type == TYPE_STATIC_REPEATED_MESH ||
        type == TYPE_STATIC_PHYSICS_MESH || type == TYPE_STATIC_PARTICLE_MESH ||
        type == TYPE_ANIMATED_MESH;
}

inline std::string ReadBankString(std::fstream& file) {
    uint32_t len = 0; file.read((char*)&len, 4);
    if (len > 0) {
        std::string s(len, '\0'); file.read(&s[0], len);
        s.erase(std::find(s.begin(), s.end(), '\0'), s.end());
        return s;
    }
    return "";
}

inline bool HasUnsavedBankChanges() {
    for (const auto& b : g_OpenBanks) {
        if (b.Type == EBankType::Audio && b.LugParserPtr && b.LugParserPtr->IsDirty) {
            return true;
        }
    }
    return false;
}

inline EBankType ResolveBankType(const std::vector<InternalBankInfo>& subBanks) {
    std::set<std::string> folders;
    for (const auto& sb : subBanks) folders.insert(sb.Name);

    if (folders.count("GBANK_MAIN_PC") || folders.count("GBANK_GUI_PC")) return EBankType::Textures;
    if (folders.count("MBANK_ALLMESHES")) return EBankType::Graphics;
    if (folders.count("GBANK_FRONT_END_PC")) return EBankType::Frontend;

    for (const auto& folder : folders) {
        if (StartsWith(folder, "TEXT_")) return EBankType::Text;
        if (StartsWith(folder, "LIPSYNC_")) return EBankType::Dialogue;
    }
    if (folders.count("PARTICLE_MAIN_PC")) return EBankType::Effects;
    return EBankType::Unknown;
}

// Helper to peek speaker without full parsing overhead
inline std::string PeekSpeakerFast(LoadedBank& bank, int index) {
    const auto& e = bank.Entries[index];
    if (e.Type != 0) return ""; // Only Type 0 has Speaker

    // 1. Check Modified Cache first
    if (bank.ModifiedEntryData.count(index)) {
        CTextParser p; p.Parse(bank.ModifiedEntryData[index], 0);
        return p.IsParsed ? p.TextData.Speaker : "";
    }

    // 2. Read from Disk (Partial Read)
    if (!bank.Stream->is_open()) return "";
    bank.Stream->clear();
    bank.Stream->seekg(e.Offset, std::ios::beg);

    // Skip Content (WString)
    // We need to read char-by-char to find double null terminator
    // Optimization: Buffer small chunk
    std::vector<uint8_t> buf(2048); // Should cover most entries
    bank.Stream->read((char*)buf.data(), (std::min)((uint32_t)buf.size(), e.Size));

    size_t cursor = 0;
    size_t max = buf.size();

    // Skip WString Content
    while (cursor + 2 <= max) {
        uint16_t c = *(uint16_t*)(buf.data() + cursor);
        cursor += 2;
        if (c == 0) break;
    }

    // Skip SpeechBank (Presized)
    if (cursor + 4 > max) return "";
    uint32_t len = *(uint32_t*)(buf.data() + cursor);
    cursor += 4 + len;

    // Read Speaker (Presized)
    if (cursor + 4 > max) return "";
    len = *(uint32_t*)(buf.data() + cursor);
    cursor += 4;

    if (len > 0 && cursor + len <= max) {
        return std::string((char*)(buf.data() + cursor), len);
    }
    return "";
}

inline void UpdateFilter(LoadedBank& bank) {
    bank.FilteredIndices.clear();
    std::string filter = bank.FilterText;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    for (size_t i = 0; i < bank.Entries.size(); i++) {
        // 1. FILTER BY TYPE (Modifier)
        if (bank.FilterTypeMask != -1) {
            if (bank.Entries[i].Type != bank.FilterTypeMask) continue;
        }

        // 2. FILTER BY TEXT
        if (filter.empty()) {
            bank.FilteredIndices.push_back((int)i);
            continue;
        }

        bool match = false;

        if (bank.FilterMode == EFilterMode::ID) {
            std::string idStr = std::to_string(bank.Entries[i].ID);
            if (idStr.find(filter) != std::string::npos) match = true;
        }
        else if (bank.FilterMode == EFilterMode::Speaker && bank.Type == EBankType::Text) {
            if (bank.Entries[i].Type == 0) {
                std::string spk = PeekSpeakerFast(bank, (int)i);
                std::transform(spk.begin(), spk.end(), spk.begin(), ::tolower);
                if (spk.find(filter) != std::string::npos) match = true;
            }
        }
        else {
            std::string name = bank.Entries[i].Name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::string friendly = bank.Entries[i].FriendlyName;
            std::transform(friendly.begin(), friendly.end(), friendly.begin(), ::tolower);

            if (name.find(filter) != std::string::npos || friendly.find(filter) != std::string::npos) {
                match = true;
            }
        }

        if (match) bank.FilteredIndices.push_back((int)i);
    }
}