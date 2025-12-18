#pragma once
#include "Utils.h"
#include "MeshParser.h"
#include "BBMParser.h"
#include "AnimParser.h"
#include "AnimProperties.h"
#include "TextureParser.h"
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

enum class EBankType {
    Unknown,
    Graphics,   // graphics.big
    Textures,   // textures.big
    Frontend,   // frontend.big
    Effects,    // effects.big
    Text,       // text.big
    Dialogue,   // dialogue.big
    Fonts,      // fonts.big
    Shaders     // shaders.big
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
    std::string FileName; std::string FullPath;
    EBankType Type = EBankType::Unknown;
    std::vector<InternalBankInfo> SubBanks;
    int ActiveSubBankIndex = -1;
    std::vector<BankEntry> Entries;
    std::vector<int> FilteredIndices;
};

static LoadedBank g_CurrentBank;
static std::string g_BankStatus = "No Bank Loaded";
static int g_SelectedEntryIndex = -1;
static char g_FilterText[128] = "";
static std::fstream g_BankStream;
static std::map<int, std::vector<uint8_t>> g_SubheaderCache;

static C3DMeshContent g_ActiveMeshContent;
static CBBMParser g_BBMParser;
static CTextureParser g_TextureParser;

// [FIX] Global flag moved here so BankExplorer and MeshProperties can both see it
static bool g_MeshUploadNeeded = false;

static int g_SelectedLOD = 0;
static std::vector<uint8_t> g_CurrentEntryRawData;

static C3DAnimationInfo g_ActiveAnim;
static AnimUIContext    g_AnimUIState;
static bool             g_AnimParseSuccess = false;

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

    if (folders.count("PARTICLE_MAIN")) throw std::runtime_error("Xbox Version Detected. Only PC is supported.");
    if (folders.count("GBANK_FRONT_END")) throw std::runtime_error("Xbox Version Detected. Only PC is supported.");
    if (folders.count("GBANK_GUI")) throw std::runtime_error("Xbox Version Detected. Only PC is supported.");
    if (folders.count("GBANK_MAIN")) throw std::runtime_error("Xbox Version Detected. Only PC is supported.");

    bool hasMeshes = folders.count("MBANK_ALLMESHES");
    bool hasEngine = folders.count("MBANK_ENGINE");

    if (hasMeshes || hasEngine) {
        if (folders.size() != 2 || !hasMeshes || !hasEngine) throw std::runtime_error("Graphics Bank: Structure invalid.");
        return EBankType::Graphics;
    }

    if (folders.count("GBANK_GUI_PC") && folders.count("GBANK_MAIN_PC")) return EBankType::Textures;
    if (folders.count("GBANK_FRONT_END_PC")) return EBankType::Frontend;
    if (folders.count("PARTICLE_MAIN_PC")) return EBankType::Effects;

    for (const auto& folder : folders) {
        if (StartsWith(folder, "SHADERS_")) return EBankType::Shaders;
        if (StartsWith(folder, "TEXT_") && Contains(folder, "_MAIN")) return EBankType::Text;
        if (StartsWith(folder, "LIPSYNC_")) return EBankType::Dialogue;
        if (StartsWith(folder, "STREAMING_FONT_") && Contains(folder, "_PC")) return EBankType::Fonts;
        if (StartsWith(folder, "FONT_") && Contains(folder, "_MAIN")) return EBankType::Fonts;
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

inline void UpdateFilter() {
    g_CurrentBank.FilteredIndices.clear();
    std::string filter = g_FilterText;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    for (size_t i = 0; i < g_CurrentBank.Entries.size(); i++) {
        if (filter.empty()) { g_CurrentBank.FilteredIndices.push_back((int)i); continue; }
        std::string name = g_CurrentBank.Entries[i].Name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find(filter) != std::string::npos) g_CurrentBank.FilteredIndices.push_back((int)i);
    }
}

inline void LoadSubBankEntries(int subBankIndex) {
    if (subBankIndex < 0 || subBankIndex >= g_CurrentBank.SubBanks.size()) return;
    if (!g_BankStream.is_open()) return;

    g_CurrentBank.ActiveSubBankIndex = subBankIndex;
    g_CurrentBank.Entries.clear();
    g_CurrentBank.FilteredIndices.clear();
    g_SubheaderCache.clear();

    const auto& info = g_CurrentBank.SubBanks[subBankIndex];
    g_BankStream.seekg(info.Offset, std::ios::beg);

    uint32_t statsCount = 0; g_BankStream.read((char*)&statsCount, 4);
    if (statsCount < 1000) g_BankStream.seekg(statsCount * 8, std::ios::cur);
    else g_BankStream.seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < info.EntryCount; i++) {
        BankEntry e; uint32_t magicE;
        g_BankStream.read((char*)&magicE, 4); g_BankStream.read((char*)&e.ID, 4); g_BankStream.read((char*)&e.Type, 4);
        g_BankStream.read((char*)&e.Size, 4); g_BankStream.read((char*)&e.Offset, 4); g_BankStream.read((char*)&e.CRC, 4);

        if (magicE != 42) continue;
        e.Name = ReadBankString(g_BankStream);
        g_BankStream.seekg(8, std::ios::cur);
        g_BankStream.seekg(-4, std::ios::cur);
        uint32_t depCount = 0; g_BankStream.read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) ReadBankString(g_BankStream);

        g_BankStream.read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)g_BankStream.tellg();

        if (e.InfoSize > 0) {
            std::vector<uint8_t> infoBuf(e.InfoSize);
            g_BankStream.read((char*)infoBuf.data(), e.InfoSize);
            g_SubheaderCache[i] = infoBuf;
        }
        g_CurrentBank.Entries.push_back(e);
        g_CurrentBank.FilteredIndices.push_back((int)g_CurrentBank.Entries.size() - 1);
    }
    UpdateFilter();
    g_BankStatus = "Loaded folder: " + info.Name;
}

inline void InitializeBank() {
    switch (g_CurrentBank.Type) {
    case EBankType::Graphics:
        g_BankStatus = "Graphics Bank Initialized (PC).";
        for (int i = 0; i < g_CurrentBank.SubBanks.size(); i++) {
            if (g_CurrentBank.SubBanks[i].Name == "MBANK_ALLMESHES") { LoadSubBankEntries(i); break; }
        }
        break;
    case EBankType::Textures:
        g_BankStatus = "Textures Bank Identified.";
        {
            int targetIdx = -1;
            for (int i = 0; i < (int)g_CurrentBank.SubBanks.size(); i++) {
                if (g_CurrentBank.SubBanks[i].Name == "GBANK_MAIN_PC") { targetIdx = i; break; }
                if (g_CurrentBank.SubBanks[i].Name == "GBANK_GUI_PC") { targetIdx = i; }
            }
            if (targetIdx != -1) LoadSubBankEntries(targetIdx);
        }
        break;
    case EBankType::Frontend: g_BankStatus = "Frontend Bank Identified."; LoadSubBankEntries(0); break;
    case EBankType::Effects: g_BankStatus = "Effects/Particles Bank Identified."; LoadSubBankEntries(0); break;
    case EBankType::Shaders: g_BankStatus = "Shaders Bank Identified."; LoadSubBankEntries(0); break;
    case EBankType::Text: g_BankStatus = "Localization/Text Bank Identified."; LoadSubBankEntries(0); break;
    case EBankType::Dialogue: g_BankStatus = "Dialogue/Lipsync Bank Identified."; LoadSubBankEntries(0); break;
    case EBankType::Fonts: g_BankStatus = "Fonts Bank Identified."; LoadSubBankEntries(0); break;
    default: g_BankStatus = "Unknown Bank Type. Loaded Default."; if (!g_CurrentBank.SubBanks.empty()) LoadSubBankEntries(0); break;
    }
}

inline void LoadBank(const std::string& path) {
    if (g_BankStream.is_open()) g_BankStream.close();
    g_CurrentBank = LoadedBank();
    g_SelectedEntryIndex = -1; g_SubheaderCache.clear(); g_CurrentEntryRawData.clear();
    g_ActiveMeshContent = C3DMeshContent(); g_BBMParser.IsParsed = false; g_AnimParseSuccess = false; g_TextureParser.IsParsed = false;

    g_BankStream.clear();
    g_BankStream.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!g_BankStream) { g_BankStream.clear(); g_BankStream.open(path, std::ios::binary | std::ios::in); }
    if (!g_BankStream) { g_BankStatus = "Error: Could not open file (Check if locked or path is valid)"; return; }

    g_CurrentBank.FullPath = path;
    g_CurrentBank.FileName = fs::path(path).filename().string();

    char magic[4]; g_BankStream.read(magic, 4); g_BankStream.seekg(0, std::ios::beg);
    if (strncmp(magic, "BIGB", 4) != 0) { g_BankStatus = "Error: Invalid .BIG file"; return; }

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_BankStream.read((char*)&h, sizeof(h));
    g_BankStream.seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; g_BankStream.read((char*)&bankCount, 4);

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(g_BankStream, b.Name, '\0');
        g_BankStream.read((char*)&b.Version, 4); g_BankStream.read((char*)&b.EntryCount, 4);
        g_BankStream.read((char*)&b.Offset, 4); g_BankStream.read((char*)&b.Size, 4); g_BankStream.read((char*)&b.Align, 4);
        g_CurrentBank.SubBanks.push_back(b);
    }
    g_BankStatus = "Found " + std::to_string(bankCount) + " internal banks.";
    try { g_CurrentBank.Type = ResolveBankType(g_CurrentBank.SubBanks); InitializeBank(); }
    catch (const std::exception& e) { g_CurrentBank.SubBanks.clear(); g_CurrentBank.Entries.clear(); g_CurrentBank.Type = EBankType::Unknown; g_BankStatus = "Error: " + std::string(e.what()); }
}

inline void SaveEntryChanges() {
    if (g_SelectedEntryIndex == -1) return;
    if (!g_BankStream.is_open()) return;
    BankEntry& e = g_CurrentBank.Entries[g_SelectedEntryIndex];
    std::vector<uint8_t> newBytes;
    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) { newBytes = g_ActiveAnim.Serialize(); }
    else return;

    if (newBytes.size() <= e.InfoSize) {
        g_BankStream.clear(); g_BankStream.seekp(e.SubheaderFileOffset, std::ios::beg);
        g_BankStream.write((char*)newBytes.data(), newBytes.size());
        g_SubheaderCache[g_SelectedEntryIndex] = newBytes;
        g_BankStatus = "Saved."; g_BankStream.flush();
    }
    else { g_BankStatus = "Error: New header too big!"; }
}

inline void ParseSelectedLOD() {
    if (g_CurrentEntryRawData.empty()) return;
    size_t offset = 0; size_t size = g_CurrentEntryRawData.size();

    if (g_ActiveMeshContent.EntryMeta.HasData && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
        if (g_SelectedLOD >= g_ActiveMeshContent.EntryMeta.LODCount) g_SelectedLOD = 0;
        size_t currentOffset = 0;
        for (int i = 0; i < g_SelectedLOD; i++) currentOffset += g_ActiveMeshContent.EntryMeta.LODSizes[i];
        size_t currentSize = g_ActiveMeshContent.EntryMeta.LODSizes[g_SelectedLOD];
        if (currentOffset + currentSize <= g_CurrentEntryRawData.size()) { offset = currentOffset; size = currentSize; }
        else { g_BankStatus = "Error: LOD Offset out of bounds!"; }
    }
    std::vector<uint8_t> slice;
    if (size > 0) { slice.resize(size); memcpy(slice.data(), g_CurrentEntryRawData.data() + offset, size); }
    g_ActiveMeshContent.Parse(slice);

    // [FIX] Signal Renderer that LOD changed
    g_MeshUploadNeeded = true;
}