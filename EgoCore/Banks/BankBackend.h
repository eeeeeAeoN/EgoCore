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

enum class EBankType {
    Unknown, Graphics, Textures, Frontend, Effects, Text, Dialogue, Fonts, Shaders, Audio
};

struct BankEntry {
    uint32_t ID = 0;
    std::string Name;
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

    LoadedBank(LoadedBank&& other) noexcept = default;
    LoadedBank& operator=(LoadedBank&& other) noexcept = default;

    LoadedBank(const LoadedBank&) = delete;
    LoadedBank& operator=(const LoadedBank&) = delete;
};

// --- FIXED GLOBALS (Using inline to ensure single instance) ---
inline std::vector<LoadedBank> g_OpenBanks;
inline int g_ActiveBankIndex = -1;
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

inline bool Contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

inline EBankType ResolveBankType(const std::vector<InternalBankInfo>& subBanks) {
    std::set<std::string> folders;
    for (const auto& sb : subBanks) folders.insert(sb.Name);

    if (folders.count("PARTICLE_MAIN")) throw std::runtime_error("Xbox Version Detected.");
    if (folders.count("GBANK_MAIN_PC") || folders.count("GBANK_GUI_PC")) return EBankType::Textures;
    if (folders.count("MBANK_ALLMESHES")) return EBankType::Graphics;
    if (folders.count("GBANK_FRONT_END_PC")) return EBankType::Frontend;
    if (folders.count("PARTICLE_MAIN_PC")) return EBankType::Effects;

    for (const auto& folder : folders) {
        if (StartsWith(folder, "SHADERS_")) return EBankType::Shaders;
        if (StartsWith(folder, "TEXT_")) return EBankType::Text;
        if (StartsWith(folder, "LIPSYNC_")) return EBankType::Dialogue;
        if (StartsWith(folder, "FONT_") || StartsWith(folder, "STREAMING_FONT_")) return EBankType::Fonts;
    }
    return EBankType::Unknown;
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

inline void UpdateFilter(LoadedBank& bank) {
    bank.FilteredIndices.clear();
    std::string filter = bank.FilterText;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    for (size_t i = 0; i < bank.Entries.size(); i++) {
        if (filter.empty()) { bank.FilteredIndices.push_back((int)i); continue; }
        std::string name = bank.Entries[i].Name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find(filter) != std::string::npos) bank.FilteredIndices.push_back((int)i);
    }
}

inline void LoadSubBankEntries(LoadedBank& bank, int subBankIndex) {
    if (subBankIndex < 0 || subBankIndex >= bank.SubBanks.size()) return;
    if (!bank.Stream->is_open()) return;

    bank.ActiveSubBankIndex = subBankIndex;
    bank.Entries.clear();
    bank.FilteredIndices.clear();
    bank.SubheaderCache.clear();

    const auto& info = bank.SubBanks[subBankIndex];
    bank.Stream->seekg(info.Offset, std::ios::beg);

    uint32_t statsCount = 0; bank.Stream->read((char*)&statsCount, 4);
    if (statsCount < 1000) bank.Stream->seekg(statsCount * 8, std::ios::cur);
    else bank.Stream->seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < info.EntryCount; i++) {
        BankEntry e; uint32_t magicE;
        bank.Stream->read((char*)&magicE, 4); bank.Stream->read((char*)&e.ID, 4);
        bank.Stream->read((char*)&e.Type, 4); bank.Stream->read((char*)&e.Size, 4);
        bank.Stream->read((char*)&e.Offset, 4); bank.Stream->read((char*)&e.CRC, 4);

        if (magicE != 42) continue;
        e.Name = ReadBankString(*bank.Stream);

        bank.Stream->seekg(4, std::ios::cur);

        uint32_t depCount = 0;
        bank.Stream->read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) {
            e.Dependencies.push_back(ReadBankString(*bank.Stream));
        }

        bank.Stream->read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)bank.Stream->tellg();

        if (e.InfoSize > 0) {
            std::vector<uint8_t> infoBuf(e.InfoSize);
            bank.Stream->read((char*)infoBuf.data(), e.InfoSize);
            bank.SubheaderCache[i] = infoBuf;
        }
        bank.Entries.push_back(e);
        bank.FilteredIndices.push_back((int)bank.Entries.size() - 1);
    }
    UpdateFilter(bank);
    g_BankStatus = "Loaded: " + info.Name;
}

inline void InitializeBank(LoadedBank& bank) {
    switch (bank.Type) {
    case EBankType::Graphics:
        for (int i = 0; i < bank.SubBanks.size(); i++) { if (bank.SubBanks[i].Name == "MBANK_ALLMESHES") { LoadSubBankEntries(bank, i); break; } }
        break;
    case EBankType::Textures:
    {
        int targetIdx = -1;
        for (int i = 0; i < (int)bank.SubBanks.size(); i++) {
            if (bank.SubBanks[i].Name == "GBANK_MAIN_PC") { targetIdx = i; break; }
            if (bank.SubBanks[i].Name == "GBANK_GUI_PC") { targetIdx = i; }
        }
        if (targetIdx != -1) LoadSubBankEntries(bank, targetIdx);
    }
    break;
    default:
        if (!bank.SubBanks.empty()) LoadSubBankEntries(bank, 0);
        break;
    }
}

// --- NEW: Internal function to load without adding to global list ---
inline std::unique_ptr<LoadedBank> CreateBankFromDisk(const std::string& path) {
    auto newBank = std::make_unique<LoadedBank>();
    newBank->FullPath = path;
    newBank->FileName = fs::path(path).filename().string();

    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".lut") {
        newBank->Type = EBankType::Audio;
        newBank->AudioParser = std::make_shared<AudioBankParser>();
        if (newBank->AudioParser->Parse(path)) {
            for (size_t i = 0; i < newBank->AudioParser->Entries.size(); i++) {
                const auto& audioEntry = newBank->AudioParser->Entries[i];
                BankEntry be;
                be.ID = audioEntry.SoundID;
                be.Name = "Sound ID " + std::to_string(audioEntry.SoundID);
                be.Size = audioEntry.Length;
                be.Offset = audioEntry.Offset;
                be.Type = 999;
                newBank->Entries.push_back(be);
                newBank->FilteredIndices.push_back((int)i);
            }
            return newBank;
        }
        return nullptr;
    }

    newBank->Stream->open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!newBank->Stream->is_open()) {
        newBank->Stream->clear();
        newBank->Stream->open(path, std::ios::binary | std::ios::in);
    }
    if (!newBank->Stream->is_open()) return nullptr;

    char magic[4]; newBank->Stream->read(magic, 4); newBank->Stream->seekg(0, std::ios::beg);
    if (strncmp(magic, "BIGB", 4) != 0) return nullptr;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    newBank->Stream->read((char*)&h, sizeof(h));
    newBank->FileVersion = h.v;

    newBank->Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; newBank->Stream->read((char*)&bankCount, 4);

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*newBank->Stream, b.Name, '\0');
        newBank->Stream->read((char*)&b.Version, 4); newBank->Stream->read((char*)&b.EntryCount, 4);
        newBank->Stream->read((char*)&b.Offset, 4); newBank->Stream->read((char*)&b.Size, 4); newBank->Stream->read((char*)&b.Align, 4);
        newBank->SubBanks.push_back(b);
    }

    try {
        newBank->Type = ResolveBankType(newBank->SubBanks);
        InitializeBank(*newBank);
        return newBank;
    }
    catch (...) { return nullptr; }
}

inline void LoadBank(const std::string& path) {
    if (g_OpenBanks.size() >= 10) { g_BankStatus = "Limit reached."; return; }
    for (const auto& b : g_OpenBanks) { if (b.FullPath == path) { g_BankStatus = "Bank already open."; return; } }

    auto newBank = CreateBankFromDisk(path);
    if (newBank) {
        g_OpenBanks.push_back(std::move(*newBank));
        g_ActiveBankIndex = (int)g_OpenBanks.size() - 1;
        g_BankStatus = "Loaded: " + g_OpenBanks.back().FileName;
    }
    else {
        g_BankStatus = "Failed to load bank.";
    }
}