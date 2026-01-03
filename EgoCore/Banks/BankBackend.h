#pragma once
#include "Utils.h"
#include "MeshParser.h"
#include "BBMParser.h"
#include "AnimParser.h"
#include "AnimProperties.h"
#include "TextureParser.h"
#include "AudioBackend.h" 
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

    std::vector<std::string> Dependencies;
};

struct InternalBankInfo {
    std::string Name;
    uint32_t Version;
    uint32_t EntryCount;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Align;
};

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

    int SelectedEntryIndex = -1;
    int SelectedLOD = 0;
    char FilterText[128] = "";
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

inline void UpdateFilter(LoadedBank& bank) {
    bank.FilteredIndices.clear();
    std::string filter = bank.FilterText;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    for (size_t i = 0; i < bank.Entries.size(); i++) {
        if (filter.empty()) { bank.FilteredIndices.push_back((int)i); continue; }

        std::string name = bank.Entries[i].Name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        std::string friendly = bank.Entries[i].FriendlyName;
        std::transform(friendly.begin(), friendly.end(), friendly.begin(), ::tolower);

        if (name.find(filter) != std::string::npos || friendly.find(filter) != std::string::npos) {
            bank.FilteredIndices.push_back((int)i);
        }
    }
}