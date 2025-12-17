#pragma once
#include "Utils.h"
#include "MeshParser.h"
#include "BBMParser.h"
#include "AnimParser.h"
#include "AnimProperties.h"
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

struct BankEntry {
    uint32_t ID = 0; std::string Name; int32_t Type = 0;
    uint32_t Offset = 0; uint32_t Size = 0; uint32_t InfoSize = 0;
    uint32_t SubheaderFileOffset = 0;
};

struct LoadedBank {
    std::string FileName; std::string FullPath;
    std::vector<BankEntry> Entries; std::vector<int> FilteredIndices;
};

static LoadedBank g_CurrentBank;
static std::string g_BankStatus = "No Bank Loaded";
static int g_SelectedEntryIndex = -1;
static char g_FilterText[128] = "";
static std::fstream g_BankStream;
static std::map<int, std::vector<uint8_t>> g_SubheaderCache;

static C3DMeshContent g_ActiveMeshContent;
static CBBMParser g_BBMParser;

static int g_SelectedLOD = 0;
static std::vector<uint8_t> g_CurrentEntryRawData;

static C3DAnimationInfo g_ActiveAnim;
static AnimUIContext    g_AnimUIState;
static bool             g_AnimParseSuccess = false;

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

inline void SaveEntryChanges() {
    if (g_SelectedEntryIndex == -1) return;
    if (!g_BankStream.is_open()) return;
    BankEntry& e = g_CurrentBank.Entries[g_SelectedEntryIndex];
    std::vector<uint8_t> newBytes;

    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
        newBytes = g_ActiveAnim.Serialize();
    }
    else return;

    if (newBytes.size() <= e.InfoSize) {
        g_BankStream.clear();
        g_BankStream.seekp(e.SubheaderFileOffset, std::ios::beg);
        g_BankStream.write((char*)newBytes.data(), newBytes.size());
        g_SubheaderCache[g_SelectedEntryIndex] = newBytes;
        g_BankStatus = "Saved.";
        g_BankStream.flush();
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
}

inline void LoadBank(const std::string& path) {
    if (g_BankStream.is_open()) g_BankStream.close();
    g_BankStream.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!g_BankStream) { g_BankStatus = "Error: Could not open file"; return; }

    g_CurrentBank.Entries.clear(); g_CurrentBank.FilteredIndices.clear(); g_SubheaderCache.clear();
    g_CurrentBank.FullPath = path; g_CurrentBank.FileName = fs::path(path).filename().string();
    g_SelectedEntryIndex = -1;

    char magic[4]; g_BankStream.read(magic, 4); g_BankStream.seekg(0, std::ios::beg);
    if (strncmp(magic, "BIGB", 4) != 0) { g_BankStatus = "Error: Invalid .BIG file"; return; }

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_BankStream.read((char*)&h, sizeof(h));
    g_BankStream.seekg(h.footOff, std::ios::beg);

    uint32_t bankCount = 0; g_BankStream.read((char*)&bankCount, 4);
    std::string bankName; std::getline(g_BankStream, bankName, '\0');
    uint32_t ver, count, offset, size, align;
    g_BankStream.read((char*)&ver, 4); g_BankStream.read((char*)&count, 4);
    g_BankStream.read((char*)&offset, 4); g_BankStream.read((char*)&size, 4); g_BankStream.read((char*)&align, 4);

    g_BankStream.seekg(offset, std::ios::beg);
    uint32_t statsCount = 0; g_BankStream.read((char*)&statsCount, 4);
    if (statsCount < 1000) g_BankStream.seekg(statsCount * 8, std::ios::cur); else g_BankStream.seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < count; i++) {
        BankEntry e; uint32_t magicE, crc;
        g_BankStream.read((char*)&magicE, 4); g_BankStream.read((char*)&e.ID, 4); g_BankStream.read((char*)&e.Type, 4);
        g_BankStream.read((char*)&e.Size, 4); g_BankStream.read((char*)&e.Offset, 4); g_BankStream.read((char*)&crc, 4);

        if (magicE != 42) continue;
        e.Name = ReadBankString(g_BankStream);
        g_BankStream.seekg(8, std::ios::cur); g_BankStream.seekg(-4, std::ios::cur);
        uint32_t depCount = 0; g_BankStream.read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) ReadBankString(g_BankStream);

        g_BankStream.read((char*)&e.InfoSize, 4); e.SubheaderFileOffset = (uint32_t)g_BankStream.tellg();
        if (e.InfoSize > 0) {
            std::vector<uint8_t> infoBuf(e.InfoSize); g_BankStream.read((char*)infoBuf.data(), e.InfoSize);
            g_SubheaderCache[i] = infoBuf;
        }
        g_CurrentBank.Entries.push_back(e); g_CurrentBank.FilteredIndices.push_back((int)i);
    }
    UpdateFilter();
    g_BankStatus = "Loaded " + std::to_string(count) + " entries.";
}