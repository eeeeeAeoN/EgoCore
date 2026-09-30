#pragma once
#include "imgui.h"
#include "LipSyncParser.h"
#include "BankBackend.h"
#include "ConfigBackend.h"
#include "SpeechAnalyzer.h"
#include "MeshRenderer.h"
#include "AnimParser.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>
#include <fstream>
#include <memory>
#include <filesystem>
#include <cctype> 
#include <cmath>

extern ID3D11Device* g_pd3dDevice;
extern ImTextureID g_PlayTexture;
extern ImTextureID g_PauseTexture;
extern ImTextureID g_StopTexture;
extern ImTextureID g_LoopTexture;
extern ImTextureID g_MusicOnTexture;
extern ImTextureID g_MusicOffTexture;
ID3D11ShaderResourceView* LoadTextureForMesh(int textureID);

static const ImVec4 kLipSyncGoldAccent = ImVec4(0.95f, 0.82f, 0.45f, 1.0f);


static CLipSyncParser g_LipSyncParser;

struct AddedEntryData {
    uint32_t Type;
    std::string NamePrefix;
    std::vector<uint8_t> Raw;
    std::vector<uint8_t> Info;
    std::vector<std::string> Dependencies;
};

struct LipSyncState {
    std::string FilePath;
    std::unique_ptr<std::fstream> Stream;
    std::vector<InternalBankInfo> SubBanks;
    std::map<std::string, int> SubBankMap;

    int CachedSubBankIndex = -1;
    std::vector<BankEntry> CachedEntries;

    std::map<int, std::set<uint32_t>> PendingDeletes;
    std::map<int, std::map<uint32_t, AddedEntryData>> PendingAdds;
};

static LipSyncState g_LipSyncState;

inline std::string GetSubBankNameForSpeech(const std::string& speechBank) {
    std::string stem = speechBank;
    size_t ld = stem.find_last_of('.'); if (ld != std::string::npos) stem = stem.substr(0, ld);
    std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);

    std::string targetSuffix = "";
    if (stem == "dialogue") targetSuffix = "_MAIN";
    else if (stem == "dialogue2") targetSuffix = "_MAIN_2";
    else if (stem == "scriptdialogue") targetSuffix = "_SCRIPT";
    else if (stem == "scriptdialogue2") targetSuffix = "_SCRIPT_2";

    if (!targetSuffix.empty() && !g_LipSyncState.SubBanks.empty()) {
        for (const auto& sb : g_LipSyncState.SubBanks) {
            if (sb.Name.find(targetSuffix) != std::string::npos) return sb.Name;
        }
    }
    return "LIPSYNC_ENGLISH" + targetSuffix;
}

inline bool EnsureLipSyncLoaded() {
    if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) return true;

    std::string path = "";
    const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
    for (const char* l : langs) {
        std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\dialogue.big";
        if (std::filesystem::exists(p)) { path = p; break; }
    }

    if (path.empty()) return false;

    g_LipSyncState.FilePath = path;
    g_LipSyncState.Stream = std::make_unique<std::fstream>(path, std::ios::binary | std::ios::in);

    char magic[4]; g_LipSyncState.Stream->read(magic, 4);
    if (strncmp(magic, "BIGB", 4) != 0) return false;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_LipSyncState.Stream->seekg(0, std::ios::beg);
    g_LipSyncState.Stream->read((char*)&h, sizeof(h));

    g_LipSyncState.Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; g_LipSyncState.Stream->read((char*)&bankCount, 4);

    g_LipSyncState.SubBanks.clear();
    g_LipSyncState.SubBankMap.clear();

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*g_LipSyncState.Stream, b.Name, '\0');
        g_LipSyncState.Stream->read((char*)&b.Version, 4); g_LipSyncState.Stream->read((char*)&b.EntryCount, 4);
        g_LipSyncState.Stream->read((char*)&b.Offset, 4); g_LipSyncState.Stream->read((char*)&b.Size, 4); g_LipSyncState.Stream->read((char*)&b.Align, 4);
        g_LipSyncState.SubBanks.push_back(b);
        g_LipSyncState.SubBankMap[b.Name] = i;
    }
    return true;
}

inline void LoadLipSyncSubBankEntries(int sbIdx) {
    if (g_LipSyncState.CachedSubBankIndex == sbIdx) return;

    g_LipSyncState.CachedEntries.clear();
    const auto& info = g_LipSyncState.SubBanks[sbIdx];
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(info.Offset, std::ios::beg);

    uint32_t checkVal = 0; g_LipSyncState.Stream->read((char*)&checkVal, 4);
    if (checkVal == 42) g_LipSyncState.Stream->seekg(-4, std::ios::cur);
    else if (checkVal < 1000) g_LipSyncState.Stream->seekg(checkVal * 8, std::ios::cur);
    else g_LipSyncState.Stream->seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < info.EntryCount; i++) {
        BankEntry e; uint32_t magicE;
        g_LipSyncState.Stream->read((char*)&magicE, 4); g_LipSyncState.Stream->read((char*)&e.ID, 4);
        g_LipSyncState.Stream->read((char*)&e.Type, 4); g_LipSyncState.Stream->read((char*)&e.Size, 4);
        g_LipSyncState.Stream->read((char*)&e.Offset, 4); g_LipSyncState.Stream->read((char*)&e.CRC, 4);

        if (magicE != 42 && e.Size == 0) continue;

        uint32_t nameLen = 0; g_LipSyncState.Stream->read((char*)&nameLen, 4);
        if (nameLen > 0) g_LipSyncState.Stream->seekg(nameLen, std::ios::cur);
        g_LipSyncState.Stream->seekg(4, std::ios::cur);
        uint32_t depCount = 0; g_LipSyncState.Stream->read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) {
            uint32_t sLen = 0; g_LipSyncState.Stream->read((char*)&sLen, 4);
            if (sLen > 0) g_LipSyncState.Stream->seekg(sLen, std::ios::cur);
        }

        g_LipSyncState.Stream->read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)g_LipSyncState.Stream->tellg();
        if (e.InfoSize > 0) g_LipSyncState.Stream->seekg(e.InfoSize, std::ios::cur);

        g_LipSyncState.CachedEntries.push_back(e);
    }
    g_LipSyncState.CachedSubBankIndex = sbIdx;
}

inline void AddLipSyncEntry(const std::string& speechBank, uint32_t newID, float duration) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    auto blob = CLipSyncParser::GenerateEmpty(duration);

    std::filesystem::path p(speechBank);
    std::string prefix = p.stem().string();
    if (!prefix.empty()) prefix[0] = toupper(prefix[0]);

    AddedEntryData ae;
    ae.Type = 1;
    ae.NamePrefix = prefix;
    ae.Raw = blob.Raw;
    ae.Info = blob.Info;
    ae.Dependencies.push_back("SPEAKER_FEMALE1");

    g_LipSyncState.PendingAdds[sbIdx][newID] = ae;
}

inline void AddLipSyncEntryFromWav(const std::string& speechBank, uint32_t newID, const std::string& wavPath) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    std::vector<int16_t> pcm;
    int sampleRate = 0;
    if (!CSpeechAnalyzer::LoadWav(wavPath, pcm, sampleRate)) {
        AddLipSyncEntry(speechBank, newID, 1.0f);
        return;
    }

    CLipSyncData lsData = CSpeechAnalyzer::AnalyzeWav(pcm, sampleRate);

    CLipSyncParser parser;
    parser.Data = lsData;
    std::vector<uint8_t> newRaw = parser.Recompile();

    std::vector<uint8_t> newInfo(4);
    memcpy(newInfo.data(), &lsData.Duration, 4);

    std::filesystem::path p(speechBank);
    std::string prefix = p.stem().string();
    if (!prefix.empty()) prefix[0] = toupper(prefix[0]);

    AddedEntryData ae;
    ae.Type = 1;
    ae.NamePrefix = prefix;
    ae.Raw = newRaw;
    ae.Info = newInfo;
    ae.Dependencies.push_back("SPEAKER_FEMALE1");

    g_LipSyncState.PendingAdds[sbIdx][newID] = ae;
    std::cout << "[LipSync] Auto-generated from WAV for ID " << newID << std::endl;
}

inline void DeleteLipSyncEntry(const std::string& speechBank, uint32_t id) {
    if (!EnsureLipSyncLoaded()) return;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return;
    if (g_LipSyncState.SubBankMap.find(subName) == g_LipSyncState.SubBankMap.end()) return;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    g_LipSyncState.PendingDeletes[sbIdx].insert(id);
    if (g_LipSyncState.PendingAdds[sbIdx].count(id)) {
        g_LipSyncState.PendingAdds[sbIdx].erase(id);
    }
}

inline std::unique_ptr<CLipSyncData> FetchLipSyncData(int32_t soundID, const std::string& speechBank) {
    if (!EnsureLipSyncLoaded()) return nullptr;
    std::string subName = GetSubBankNameForSpeech(speechBank);
    if (subName.empty()) return nullptr;
    int sbIdx = g_LipSyncState.SubBankMap[subName];

    if (g_LipSyncState.PendingDeletes[sbIdx].count((uint32_t)soundID)) return nullptr;

    if (g_LipSyncState.PendingAdds[sbIdx].count((uint32_t)soundID)) {
        const auto& entry = g_LipSyncState.PendingAdds[sbIdx][(uint32_t)soundID];
        auto result = std::make_unique<CLipSyncData>();
        CLipSyncParser parser;
        parser.Parse(entry.Raw, entry.Info);
        *result = parser.Data;
        result->IsParsed = true;
        if (result->Duration == 0.0f) {
            if (entry.Info.size() >= 4) {
                float dur = *(float*)entry.Info.data();
                result->Duration = dur;
            }
        }
        return result;
    }

    LoadLipSyncSubBankEntries(sbIdx);
    BankEntry* target = nullptr;
    for (auto& e : g_LipSyncState.CachedEntries) {
        if (e.ID == (uint32_t)soundID) { target = &e; break; }
    }
    if (!target) return nullptr;

    std::vector<uint8_t> rawData(target->Size);
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(target->Offset, std::ios::beg);
    g_LipSyncState.Stream->read((char*)rawData.data(), target->Size);

    std::vector<uint8_t> infoData(target->InfoSize);
    g_LipSyncState.Stream->clear();
    g_LipSyncState.Stream->seekg(target->SubheaderFileOffset, std::ios::beg);
    g_LipSyncState.Stream->read((char*)infoData.data(), target->InfoSize);

    auto result = std::make_unique<CLipSyncData>();
    CLipSyncParser parser;
    parser.Parse(rawData, infoData);
    *result = parser.Data;
    return result;
}

inline std::string GetSymbol(uint8_t id, const std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.ID == id) return p.Symbol;
    }
    return "??";
}

inline uint8_t GetOrAddPhonemeID(const std::string& symbol, std::vector<CLipSyncPhonemeRef>& dict) {
    for (const auto& p : dict) {
        if (p.Symbol == symbol) return p.ID;
    }
    std::set<uint8_t> used;
    for (const auto& p : dict) used.insert(p.ID);
    uint8_t nextID = 0;
    while (used.count(nextID)) {
        if (nextID == 255) break;
        nextID++;
    }
    CLipSyncPhonemeRef newRef;
    newRef.ID = nextID;
    newRef.Symbol = symbol;
    dict.push_back(newRef);
    return nextID;
}

static const std::vector<std::string> FABLE_PHONEMES = { "AH", "EE", "MM", "OH", "SZ", "WW" };

struct LipSyncHeadPreset {
    const char* DisplayName;
    const char* MeshName;
    std::unordered_map<std::string, std::string> PhonemeAnims;
};

static const std::vector<LipSyncHeadPreset> kLipSyncPresets = {
    {
        "Bandit Lieutenant",
        "MESH_BANDITLIEUTENANT_HEAD_01",
        {
            {"AH", "ANIM_BANDIT_PHONEME_AH"},
            {"EE", "ANIM_BANDIT_PHONEME_EE"},
            {"MM", "ANIM_BANDIT_PHONEME_MM"},
            {"OH", "ANIM_BANDIT_PHONEME_OH"},
            {"SZ", "ANIM_BANDIT_PHONEME_SZ"},
            {"WW", "ANIM_BANDIT_PHONEME_WW"}
        }
    },
    {
        "Female Villager",
        "MESH_BS_FEMALE_MIDDLE_HEAD_01",
        {
            {"AH", "ANIM_VILLAGER_FEMALE_PHONEME_AH"},
            {"EE", "ANIM_VILLAGER_FEMALE_PHONEME_EE"},
            {"MM", "ANIM_VILLAGER_FEMALE_PHONEME_MM"},
            {"OH", "ANIM_VILLAGER_FEMALE_PHONEME_OH"},
            {"SZ", "ANIM_VILLAGER_FEMALE_PHONEME_SZ"},
            {"WW", "ANIM_VILLAGER_FEMALE_PHONEME_WW"}
        }
    },
    {
        "Male Villager",
        "MESH_BS_MALE_MIDDLE_HEAD_01",
        {
            {"AH", "ANIM_VILLAGER_MALE_PHONEME_AH"},
            {"EE", "ANIM_VILLAGER_MALE_PHONEME_EE"},
            {"MM", "ANIM_VILLAGER_MALE_PHONEME_MM"},
            {"OH", "ANIM_VILLAGER_MALE_PHONEME_OH"},
            {"SZ", "ANIM_VILLAGER_MALE_PHONEME_SZ"},
            {"WW", "ANIM_VILLAGER_MALE_PHONEME_WW"}
        }
    },
    {
        "Demon Door",
        "MESH_DEMON_DOOR_FACE_01",
        {
            {"AH", "ANIM_DEMON_DOOR_PHONEME_AI"},
            {"AI", "ANIM_DEMON_DOOR_PHONEME_AI"},
            {"EE", "ANIM_DEMON_DOOR_PHONEME_EE"},
            {"MM", "ANIM_DEMON_DOOR_PHONEME_MM"},
            {"OH", "ANIM_DEMON_DOOR_PHONEME_OH"},
            {"SZ", "ANIM_DEMON_DOOR_PHONEME_ST"},
            {"WW", "ANIM_DEMON_DOOR_PHONEME_WW"}
        }
    },
    {
        "Male Child",
        "MESH_BS_MALE_CHILD_HEAD_01",
        {
            {"AH", "ANIM_BIPED_BOY_01_PHONEME_AH"},
            {"EE", "ANIM_BIPED_BOY_01_PHONEME_EE"},
            {"MM", "ANIM_BIPED_BOY_01_PHONEME_MM"},
            {"OH", "ANIM_BIPED_BOY_01_PHONEME_OH"},
            {"SZ", "ANIM_BIPED_BOY_01_PHONEME_SZ"},
            {"WW", "ANIM_BIPED_BOY_01_PHONEME_WW"}
        }
    }
};

static MeshRenderer g_LipSyncRenderer;
static bool g_ShowLipSyncRenderer = true;
static float g_LipSyncTime = 0.0f;
static bool g_LipSyncPlaying = false;
static bool g_LipSyncLoop = true;
static std::vector<XMMATRIX> g_LipSyncBoneMats;
static std::vector<XMMATRIX> g_LipSyncGlobalMats;
static int g_CurrentLipSyncPresetIdx = 0;

struct CachedLipSyncPreset {
    int PresetIdx = -1;
    bool MeshLoaded = false;
    C3DMeshContent Mesh;
    std::unordered_map<std::string, C3DAnimationInfo> PhonemeAnims;
    std::vector<std::string> MissingAssets;
};
static CachedLipSyncPreset g_CachedPreset;

// Dialogue Audio State
static AudioPlayer g_LipSyncAudioPlayer;
static bool g_LipSyncAudioEnabled = true;
static int32_t g_LastLoadedAudioSoundID = -1;
static std::vector<uint8_t> g_CachedAudioWavBlob;
static uint32_t g_LastSelectedEntryID = 0xFFFFFFFF;

struct LipSyncGraphicsBankCache {
    std::string FilePath;
    std::unique_ptr<std::fstream> Stream;
    std::vector<BankEntry> Entries;
    std::map<int, std::vector<uint8_t>> Subheaders;
    bool IsLoaded = false;
};
static LipSyncGraphicsBankCache g_GraphicsBigCache;

inline bool EnsureGraphicsBigLoaded() {
    if (g_GraphicsBigCache.IsLoaded) return true;
    for (const auto& b : g_OpenBanks) {
        if (b.Type == EBankType::Graphics || b.Type == EBankType::XboxGraphics) return true;
    }

    std::string path = g_AppConfig.GameRootPath + "\\Data\\graphics\\graphics.big";
    if (!std::filesystem::exists(path)) return false;

    g_GraphicsBigCache.FilePath = path;
    g_GraphicsBigCache.Stream = std::make_unique<std::fstream>(path, std::ios::binary | std::ios::in);
    if (!g_GraphicsBigCache.Stream->is_open()) return false;

    char magic[4]; g_GraphicsBigCache.Stream->read(magic, 4);
    if (strncmp(magic, "BIGB", 4) != 0) return false;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    g_GraphicsBigCache.Stream->seekg(0, std::ios::beg);
    g_GraphicsBigCache.Stream->read((char*)&h, sizeof(h));

    g_GraphicsBigCache.Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0; g_GraphicsBigCache.Stream->read((char*)&bankCount, 4);

    InternalBankInfo mbankInfo;
    bool foundMbank = false;

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*g_GraphicsBigCache.Stream, b.Name, '\0');
        g_GraphicsBigCache.Stream->read((char*)&b.Version, 4); g_GraphicsBigCache.Stream->read((char*)&b.EntryCount, 4);
        g_GraphicsBigCache.Stream->read((char*)&b.Offset, 4); g_GraphicsBigCache.Stream->read((char*)&b.Size, 4); g_GraphicsBigCache.Stream->read((char*)&b.Align, 4);
        if (b.Name == "MBANK_ALLMESHES") {
            mbankInfo = b;
            foundMbank = true;
        }
    }
    if (!foundMbank) return false;

    g_GraphicsBigCache.Stream->clear();
    g_GraphicsBigCache.Stream->seekg(mbankInfo.Offset, std::ios::beg);

    uint32_t checkVal = 0; g_GraphicsBigCache.Stream->read((char*)&checkVal, 4);
    if (checkVal == 42) g_GraphicsBigCache.Stream->seekg(-4, std::ios::cur);
    else if (checkVal < 1000) g_GraphicsBigCache.Stream->seekg(checkVal * 8, std::ios::cur);
    else g_GraphicsBigCache.Stream->seekg(-4, std::ios::cur);

    for (uint32_t i = 0; i < mbankInfo.EntryCount; i++) {
        BankEntry e; uint32_t magicE;
        g_GraphicsBigCache.Stream->read((char*)&magicE, 4); g_GraphicsBigCache.Stream->read((char*)&e.ID, 4);
        g_GraphicsBigCache.Stream->read((char*)&e.Type, 4); g_GraphicsBigCache.Stream->read((char*)&e.Size, 4);
        g_GraphicsBigCache.Stream->read((char*)&e.Offset, 4); g_GraphicsBigCache.Stream->read((char*)&e.CRC, 4);

        if (magicE != 42 && e.Size == 0) continue;

        uint32_t nameLen = 0; g_GraphicsBigCache.Stream->read((char*)&nameLen, 4);
        if (nameLen > 0) {
            e.Name.resize(nameLen);
            g_GraphicsBigCache.Stream->read(&e.Name[0], nameLen);
            e.Name.erase(std::find(e.Name.begin(), e.Name.end(), '\0'), e.Name.end());
        }
        g_GraphicsBigCache.Stream->seekg(4, std::ios::cur);
        uint32_t depCount = 0; g_GraphicsBigCache.Stream->read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) {
            uint32_t sLen = 0; g_GraphicsBigCache.Stream->read((char*)&sLen, 4);
            if (sLen > 0) g_GraphicsBigCache.Stream->seekg(sLen, std::ios::cur);
        }

        g_GraphicsBigCache.Stream->read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)g_GraphicsBigCache.Stream->tellg();
        if (e.InfoSize > 0) {
            std::vector<uint8_t> infoBuf(e.InfoSize);
            g_GraphicsBigCache.Stream->read((char*)infoBuf.data(), e.InfoSize);
            g_GraphicsBigCache.Subheaders[(int)g_GraphicsBigCache.Entries.size()] = infoBuf;
        }

        g_GraphicsBigCache.Entries.push_back(e);
    }

    g_GraphicsBigCache.IsLoaded = true;
    return true;
}

inline bool FindAndLoadGraphicsAsset(const std::string& targetName, std::vector<uint8_t>& outRaw, std::vector<uint8_t>& outSubheader, int32_t& outType) {
    std::string lowerTarget = targetName;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);

    // 1. Check open banks
    for (auto& b : g_OpenBanks) {
        if (b.Type == EBankType::Graphics || b.Type == EBankType::XboxGraphics) {
            for (int i = 0; i < (int)b.Entries.size(); i++) {
                std::string eName = b.Entries[i].Name;
                std::transform(eName.begin(), eName.end(), eName.begin(), ::tolower);
                if (eName == lowerTarget) {
                    outType = b.Entries[i].Type;
                    if (b.SubheaderCache.count(i)) outSubheader = b.SubheaderCache[i];
                    else if (b.Entries[i].InfoSize > 0 && b.Entries[i].SubheaderFileOffset > 0 && b.Stream && b.Stream->is_open()) {
                        b.Stream->clear();
                        b.Stream->seekg(b.Entries[i].SubheaderFileOffset, std::ios::beg);
                        outSubheader.resize(b.Entries[i].InfoSize);
                        b.Stream->read((char*)outSubheader.data(), b.Entries[i].InfoSize);
                    }

                    if (b.ModifiedEntryData.count(i)) outRaw = b.ModifiedEntryData[i];
                    else if (b.Stream && b.Stream->is_open() && b.Entries[i].Size > 0) {
                        b.Stream->clear();
                        b.Stream->seekg(b.Entries[i].Offset, std::ios::beg);
                        outRaw.resize(b.Entries[i].Size);
                        b.Stream->read((char*)outRaw.data(), b.Entries[i].Size);
                    }
                    return true;
                }
            }
        }
    }

    // 2. Check cached graphics.big stream
    if (EnsureGraphicsBigLoaded() && g_GraphicsBigCache.Stream && g_GraphicsBigCache.Stream->is_open()) {
        for (int i = 0; i < (int)g_GraphicsBigCache.Entries.size(); i++) {
            std::string eName = g_GraphicsBigCache.Entries[i].Name;
            std::transform(eName.begin(), eName.end(), eName.begin(), ::tolower);
            if (eName == lowerTarget) {
                outType = g_GraphicsBigCache.Entries[i].Type;
                if (g_GraphicsBigCache.Subheaders.count(i)) outSubheader = g_GraphicsBigCache.Subheaders[i];

                g_GraphicsBigCache.Stream->clear();
                g_GraphicsBigCache.Stream->seekg(g_GraphicsBigCache.Entries[i].Offset, std::ios::beg);
                outRaw.resize(g_GraphicsBigCache.Entries[i].Size);
                g_GraphicsBigCache.Stream->read((char*)outRaw.data(), g_GraphicsBigCache.Entries[i].Size);
                return true;
            }
        }
    }

    return false;
}

inline void EnsurePresetLoaded(int presetIdx) {
    if (presetIdx < 0 || presetIdx >= (int)kLipSyncPresets.size()) return;
    if (g_CachedPreset.PresetIdx == presetIdx && g_CachedPreset.MeshLoaded) return;

    g_CachedPreset.PresetIdx = presetIdx;
    g_CachedPreset.MeshLoaded = false;
    g_CachedPreset.Mesh = C3DMeshContent();
    g_CachedPreset.PhonemeAnims.clear();
    g_CachedPreset.MissingAssets.clear();

    const auto& preset = kLipSyncPresets[presetIdx];

    // 1. Load Head Mesh
    std::vector<uint8_t> meshRaw, meshSub;
    int32_t meshType = 0;
    if (FindAndLoadGraphicsAsset(preset.MeshName, meshRaw, meshSub, meshType)) {
        if (!meshSub.empty()) g_CachedPreset.Mesh.ParseEntryMetadata(meshSub);
        if (!meshRaw.empty()) g_CachedPreset.Mesh.Parse(meshRaw, meshType);
        if (g_CachedPreset.Mesh.IsParsed) {
            g_LipSyncRenderer.Initialize(g_pd3dDevice);
            g_LipSyncRenderer.UploadMesh(g_pd3dDevice, g_CachedPreset.Mesh, true);

            std::vector<MeshRenderer::RenderMaterial> materials;
            int maxMat = 0;
            for (const auto& m : g_CachedPreset.Mesh.Materials) if (m.ID > maxMat) maxMat = m.ID;
            materials.resize(maxMat + 1);

            for (const auto& m : g_CachedPreset.Mesh.Materials) {
                materials[m.ID].SelfIllumination = (float)m.SelfIllumination / 255.0f;
                if (m.DiffuseMapID > 0) materials[m.ID].Diffuse = LoadTextureForMesh(m.DiffuseMapID);
                if (m.BumpMapID > 0) materials[m.ID].Bump = LoadTextureForMesh(m.BumpMapID);
            }
            g_LipSyncRenderer.SetMaterials(materials);
            g_CachedPreset.MeshLoaded = true;
        }
        else {
            g_CachedPreset.MissingAssets.push_back("Failed to parse mesh: " + std::string(preset.MeshName));
        }
    }
    else {
        g_CachedPreset.MissingAssets.push_back("Mesh not found: " + std::string(preset.MeshName));
    }

    // 2. Load Phoneme Animations
    for (const auto& pair : preset.PhonemeAnims) {
        const std::string& symbol = pair.first;
        const std::string& animName = pair.second;

        if (g_CachedPreset.PhonemeAnims.count(symbol) && g_CachedPreset.PhonemeAnims[symbol].IsParsed) continue;

        std::vector<uint8_t> animRaw, animSub;
        int32_t aType = 0;
        if (FindAndLoadGraphicsAsset(animName, animRaw, animSub, aType)) {
            C3DAnimationInfo anim;
            if (!animSub.empty()) anim.Deserialize(animSub);
            if (!animRaw.empty()) {
                AnimParser parser;
                if (parser.Parse(animRaw)) {
                    anim = parser.Data;
                }
            }
            if (anim.IsParsed) {
                g_CachedPreset.PhonemeAnims[symbol] = anim;
            }
            else {
                g_CachedPreset.MissingAssets.push_back("Failed to parse animation: " + animName);
            }
        }
        else {
            g_CachedPreset.MissingAssets.push_back("Animation not found: " + animName);
        }
    }
}

inline void UpdateLipSyncBones(const C3DMeshContent& mesh,
                              const std::unordered_map<std::string, C3DAnimationInfo>& phonemeAnims,
                              const std::unordered_map<std::string, float>& phonemeWeights,
                              std::vector<XMMATRIX>& outBoneMats,
                              std::vector<XMMATRIX>& outGlobalMats)
{
    int boneCount = mesh.BoneCount;
    outBoneMats.resize(boneCount);
    outGlobalMats.resize(boneCount);
    if (boneCount == 0) return;

    // 1. IBM and Bind Local Transforms
    std::vector<XMMATRIX> ibm(boneCount), bindGlobal(boneCount), bindLocal(boneCount);
    for (int i = 0; i < boneCount; i++) {
        if ((i + 1) * 64 <= mesh.BoneTransformsRaw.size()) {
            float* raw = (float*)(mesh.BoneTransformsRaw.data() + i * 64);
            XMMATRIX rawMatrix = XMMATRIX(raw);
            rawMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
            ibm[i] = XMMatrixTranspose(rawMatrix);
            bindGlobal[i] = XMMatrixInverse(nullptr, ibm[i]);
        }
        else {
            ibm[i] = XMMatrixIdentity();
            bindGlobal[i] = XMMatrixIdentity();
        }
    }
    for (int i = 0; i < boneCount; i++) {
        int p = mesh.Bones[i].ParentIndex;
        if (p == -1 || p >= boneCount) bindLocal[i] = bindGlobal[i];
        else bindLocal[i] = XMMatrixMultiply(bindGlobal[i], ibm[p]);
    }

    // 2. Multi-Target Weights Calculation
    // MM is the default/base closed-mouth phoneme in Fable.
    // When there are no phonemes in the frame table (or total weight < 1.0),
    // MM fills the remaining weight to act as the base resting pose.
    float totalWeight = 0.0f;
    for (const auto& pw : phonemeWeights) {
        if (pw.second > 0.001f) totalWeight += pw.second;
    }

    std::unordered_map<std::string, float> effectiveWeights;
    if (totalWeight > 1.0f) {
        for (const auto& pw : phonemeWeights) {
            effectiveWeights[pw.first] = pw.second / totalWeight;
        }
    }
    else {
        effectiveWeights = phonemeWeights;
        float mmWeight = 1.0f - totalWeight;
        if (mmWeight > 0.0001f) {
            effectiveWeights["MM"] += mmWeight;
        }
    }

    auto cleanName = [](const std::string& str) {
        std::string res;
        for (char c : str) if (isalnum((unsigned char)c)) res += (char)tolower((unsigned char)c);
        return res;
    };

    struct ActivePhonemeTrack {
        float Weight;
        const C3DAnimationInfo* Anim;
    };
    std::vector<ActivePhonemeTrack> activePhonemes;
    for (const auto& ew : effectiveWeights) {
        if (ew.second > 0.001f) {
            auto it = phonemeAnims.find(ew.first);
            if (it != phonemeAnims.end() && it->second.IsParsed) {
                activePhonemes.push_back({ ew.second, &it->second });
            }
        }
    }

    std::vector<XMMATRIX> localTransforms(boneCount);

    float activeSum = 0.0f;
    for (const auto& ap : activePhonemes) activeSum += ap.Weight;
    float unassignedWeight = 1.0f - activeSum;
    if (unassignedWeight < 0.0f) unassignedWeight = 0.0f;

    for (int i = 0; i < boneCount; i++) {
        // Decompose bind local transform (r_b_true is unconjugated DirectX quaternion)
        XMVECTOR s_b, r_b_true, t_b;
        XMMatrixDecompose(&s_b, &r_b_true, &t_b, bindLocal[i]);

        if (activePhonemes.empty()) {
            localTransforms[i] = bindLocal[i];
            continue;
        }

        std::string targetBoneName = (i < (int)mesh.BoneNames.size()) ? mesh.BoneNames[i] : "";
        std::string cleanTarget = cleanName(targetBoneName);

        // Pelvis/Hips/Root must NEVER move during head lipsync
        if (cleanTarget.empty() || cleanTarget == "bip01" || cleanTarget == "root" || cleanTarget == "bip01pelvis") {
            localTransforms[i] = bindLocal[i];
            continue;
        }

        // Check if any active phoneme touches this bone
        bool anyTrackFound = false;
        for (const auto& ap : activePhonemes) {
            for (const auto& track : ap.Anim->Tracks) {
                if (cleanTarget == cleanName(track.BoneName)) {
                    anyTrackFound = true;
                    break;
                }
            }
            if (anyTrackFound) break;
        }

        if (!anyTrackFound) {
            localTransforms[i] = bindLocal[i];
            continue;
        }

        // Lock translation for all skeletal base bones (e.g. bip01head, bip01neck) so the head remains anchored
        bool lockTranslation = (cleanTarget.find("bip01") == 0);

        XMVECTOR accumRot = XMVectorScale(r_b_true, unassignedWeight);
        XMVECTOR accumPos = XMVectorScale(t_b, unassignedWeight);

        for (const auto& ap : activePhonemes) {
            XMVECTOR r_target = r_b_true;
            XMVECTOR t_target = t_b;

            for (const auto& track : ap.Anim->Tracks) {
                if (cleanTarget == cleanName(track.BoneName)) {
                    if (track.FrameCount > 0) {
                        Vec3 p = { XMVectorGetX(t_b), XMVectorGetY(t_b), XMVectorGetZ(t_b) };
                        Vec4 q = { 0.0f, 0.0f, 0.0f, 1.0f };
                        track.EvaluateFrame(0, p, q);

                        // If track has rotation, conjugate normalized quaternion (matches AnimProperties.h:97)
                        if (!track.RotationTrack.empty()) {
                            r_target = XMQuaternionConjugate(XMQuaternionNormalize(XMVectorSet(q.x, q.y, q.z, q.w)));
                        }

                        // If track has position and translation is not locked to bind
                        if (!lockTranslation && !track.PositionTrack.empty()) {
                            t_target = XMVectorSet(p.x, p.y, p.z, 1.0f);
                        }
                    }
                    break;
                }
            }

            // Shortest arc relative to r_b_true
            if (XMVectorGetX(XMVector4Dot(r_target, r_b_true)) < 0.0f) {
                r_target = XMVectorNegate(r_target);
            }

            accumRot = XMVectorAdd(accumRot, XMVectorScale(r_target, ap.Weight));
            accumPos = XMVectorAdd(accumPos, XMVectorScale(t_target, ap.Weight));
        }

        XMVECTOR blendedRot = XMQuaternionNormalize(accumRot);
        XMVECTOR blendedPos = accumPos;

        localTransforms[i] = XMMatrixScalingFromVector(s_b) *
                             XMMatrixRotationQuaternion(blendedRot) *
                             XMMatrixTranslationFromVector(blendedPos);
    }

    // 3. Hierarchical Global Transforms
    std::vector<bool> computed(boneCount, false);
    std::function<void(int)> ComputeGlobal = [&](int idx) {
        if (computed[idx]) return;
        int p = mesh.Bones[idx].ParentIndex;
        if (p != -1 && p < boneCount) {
            ComputeGlobal(p);
            outGlobalMats[idx] = XMMatrixMultiply(localTransforms[idx], outGlobalMats[p]);
        }
        else {
            outGlobalMats[idx] = localTransforms[idx];
        }
        computed[idx] = true;
    };

    for (int i = 0; i < boneCount; i++) {
        ComputeGlobal(i);
        outBoneMats[i] = XMMatrixMultiply(ibm[i], outGlobalMats[i]);
    }
}

inline void EnsureDialogueAudioLoaded(uint32_t soundID, const std::string& subBankName) {
    if ((int32_t)soundID == g_LastLoadedAudioSoundID) return;
    g_LastLoadedAudioSoundID = (int32_t)soundID;
    g_CachedAudioWavBlob.clear();

    std::vector<std::string> lutsToTry;
    std::string upperSub = subBankName;
    std::transform(upperSub.begin(), upperSub.end(), upperSub.begin(), ::toupper);

    if (upperSub.find("_MAIN_2") != std::string::npos) {
        lutsToTry = { "dialogue2.lut", "dialogue.lut", "scriptdialogue2.lut", "scriptdialogue.lut" };
    }
    else if (upperSub.find("_MAIN") != std::string::npos) {
        lutsToTry = { "dialogue.lut", "dialogue2.lut", "scriptdialogue.lut", "scriptdialogue2.lut" };
    }
    else if (upperSub.find("_SCRIPT_2") != std::string::npos) {
        lutsToTry = { "scriptdialogue2.lut", "scriptdialogue.lut", "dialogue2.lut", "dialogue.lut" };
    }
    else if (upperSub.find("_SCRIPT") != std::string::npos) {
        lutsToTry = { "scriptdialogue.lut", "scriptdialogue2.lut", "dialogue.lut", "dialogue2.lut" };
    }
    else {
        lutsToTry = { "dialogue.lut", "dialogue2.lut", "scriptdialogue.lut", "scriptdialogue2.lut" };
    }

    for (const auto& lutName : lutsToTry) {
        // Search open banks
        for (auto& b : g_OpenBanks) {
            if (b.Type == EBankType::Audio && b.AudioParser) {
                std::string fname = b.FileName;
                std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
                if (fname == lutName) {
                    for (size_t i = 0; i < b.AudioParser->Entries.size(); ++i) {
                        if (b.AudioParser->Entries[i].SoundID == soundID) {
                            g_CachedAudioWavBlob = b.AudioParser->GetRiffBlob((int)i);
                            return;
                        }
                    }
                }
            }
        }

        // Search disk
        const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
        for (const char* l : langs) {
            std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\" + lutName;
            if (std::filesystem::exists(p)) {
                AudioBankParser parser;
                if (parser.Parse(p)) {
                    for (size_t i = 0; i < parser.Entries.size(); ++i) {
                        if (parser.Entries[i].SoundID == soundID) {
                            g_CachedAudioWavBlob = parser.GetRiffBlob((int)i);
                            return;
                        }
                    }
                }
            }
        }
    }
}

inline void RenderLipSyncFrames(CLipSyncData& d) {
    ImGui::TextColored(kLipSyncGoldAccent, "Animation Frames");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24.0f);
    if (ImGui::Button("+##AddFrameEnd", ImVec2(22, 20))) {
        CLipSyncFrame newFrame;
        d.Frames.push_back(newFrame);
        if (d.FPS > 0.0f) d.Duration = (float)d.Frames.size() / d.FPS;
        d.FrameCount = (uint32_t)d.Frames.size();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Frame at End");

    if (ImGui::BeginChild("FrameStream", ImVec2(0, 0), true)) {
        if (ImGui::BeginTable("FrameTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Phonemes", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            int frameToDelete = -1;

            for (size_t i = 0; i < d.Frames.size(); i++) {
                auto& frame = d.Frames[i];
                ImGui::PushID((int)i);
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%03zu", i);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 0.75f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.1f, 0.1f, 1.0f));
                if (ImGui::SmallButton("X")) {
                    frameToDelete = (int)i;
                }
                ImGui::PopStyleColor(3);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete this frame");

                ImGui::TableSetColumnIndex(1);
                int keyToDelete = -1;
                for (size_t k = 0; k < frame.Keys.size(); k++) {
                    auto& key = frame.Keys[k];
                    ImGui::PushID((int)k);
                    std::string symbol = GetSymbol(key.ID, d.Dictionary);
                    ImGui::AlignTextToFramePadding();

                    float w = std::clamp(key.WeightFloat, 0.0f, 1.0f);
                    float cr = 0.45f * (1.0f - w) + 0.20f * w;
                    float cg = 0.45f * (1.0f - w) + 1.00f * w;
                    float cb = 0.45f * (1.0f - w) + 0.35f * w;
                    float ca = 0.65f + 0.35f * w;
                    ImGui::TextColored(ImVec4(cr, cg, cb, ca), "%s", symbol.c_str());

                    ImGui::SameLine();
                    float wVal = key.WeightFloat;
                    ImGui::SetNextItemWidth(100);
                    if (ImGui::SliderFloat("##w", &wVal, 0.0f, 1.0f, "%.2f")) {
                        key.WeightFloat = wVal;
                        key.WeightByte = (uint8_t)(wVal * 255.0f);
                    }
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 0.65f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.1f, 0.1f, 1.0f));
                    if (ImGui::SmallButton("x")) keyToDelete = (int)k;
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Remove phoneme");

                    ImGui::SameLine();
                    ImGui::Dummy(ImVec2(8, 0));
                    ImGui::SameLine();
                    ImGui::PopID();
                }
                if (keyToDelete != -1) {
                    frame.Keys.erase(frame.Keys.begin() + keyToDelete);
                }
                if (frame.Keys.size() < 4) {
                    if (ImGui::Button("+")) {
                        ImGui::OpenPopup("AddPhonemePopup");
                    }
                    if (ImGui::BeginPopup("AddPhonemePopup")) {
                        for (const auto& ph : FABLE_PHONEMES) {
                            if (ImGui::Selectable(ph.c_str())) {
                                uint8_t pid = GetOrAddPhonemeID(ph, d.Dictionary);
                                CLipSyncFrameKey newKey;
                                newKey.ID = pid;
                                newKey.WeightFloat = 1.0f;
                                newKey.WeightByte = 255;
                                frame.Keys.push_back(newKey);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::PopID();
            }
            if (frameToDelete != -1) {
                d.Frames.erase(d.Frames.begin() + frameToDelete);
                if (d.FPS > 0.0f) d.Duration = (float)d.Frames.size() / d.FPS;
                d.FrameCount = (uint32_t)d.Frames.size();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

inline void DrawLipSyncProperties(LoadedBank* bank, std::function<void()> onSave, std::function<void()> onRecompile) {
    if (!g_LipSyncParser.IsParsed && !g_LipSyncParser.Data.IsParsed) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to parse LipSync data.");
        return;
    }

    auto& d = g_LipSyncParser.Data;

    uint32_t currentSoundID = 0;
    int currentEntryIndex = -1;
    std::string activeSubBank = "";
    if (bank) {
        currentEntryIndex = bank->SelectedEntryIndex;
        if (bank->SelectedEntryIndex >= 0 && bank->SelectedEntryIndex < (int)bank->Entries.size()) {
            currentSoundID = bank->Entries[bank->SelectedEntryIndex].ID;
        }
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < (int)bank->SubBanks.size()) {
            activeSubBank = bank->SubBanks[bank->ActiveSubBankIndex].Name;
        }
    }

    static int s_LastSelectedEntryIndex = -1;
    if (currentEntryIndex != s_LastSelectedEntryIndex || currentSoundID != g_LastSelectedEntryID) {
        s_LastSelectedEntryIndex = currentEntryIndex;
        g_LastSelectedEntryID = currentSoundID;
        g_LastLoadedAudioSoundID = -1;
        g_LipSyncAudioPlayer.Reset();
        g_LipSyncPlaying = false;
        g_LipSyncTime = 0.0f;
        EnsureDialogueAudioLoaded(currentSoundID, activeSubBank);
        if (!g_CachedAudioWavBlob.empty()) {
            g_LipSyncAudioPlayer.PlayWav(g_CachedAudioWavBlob);
            g_LipSyncAudioPlayer.Pause();
        }
    }

    static float rightPanelWidth = 400.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float splitterWidth = 4.0f;
    float leftWidth = avail.x;
    if (g_ShowLipSyncRenderer) {
        leftWidth = avail.x - rightPanelWidth - splitterWidth;
        if (leftWidth < 240.0f) leftWidth = 240.0f;
    }

    // ==========================================
    // LEFT PANEL: HEADER OVERVIEW & FRAMES
    // ==========================================
    ImGui::BeginChild("LipSyncLeftPanel", ImVec2(leftWidth, avail.y), false);
    {
        ImGui::TextColored(kLipSyncGoldAccent, "LipSync Editor");
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Button(g_ShowLipSyncRenderer ? ">##LSToggle" : "<##LSToggle", ImVec2(24, 20))) {
            g_ShowLipSyncRenderer = !g_ShowLipSyncRenderer;
        }
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_ShowLipSyncRenderer ? "Collapse Viewport" : "Expand Viewport");

        ImGui::Separator();

        // Non-collapsing summary overview
        {
            ImGui::TextDisabled("Duration:"); ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 1.0f, 1.0f), "%.3f s", d.Duration);
            ImGui::SameLine(130.0f);
            ImGui::TextDisabled("Frames:"); ImGui::SameLine();
            ImGui::Text("%u", d.FrameCount);
            ImGui::SameLine(230.0f);
            ImGui::TextDisabled("FPS:"); ImGui::SameLine();
            ImGui::Text("%.2f", d.FPS);
            ImGui::SameLine(330.0f);
            ImGui::TextDisabled("Sound ID:"); ImGui::SameLine();
            ImGui::TextColored(kLipSyncGoldAccent, "%u", currentSoundID);
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        RenderLipSyncFrames(d);
    }
    ImGui::EndChild();

    // ==========================================
    // RIGHT PANEL: 3D VIEWPORT & PLAYBACK
    // ==========================================
    if (g_ShowLipSyncRenderer) {
        ImGui::SameLine(0, 0);

        ImVec2 splitterMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("LipSyncVSplitter", ImVec2(splitterWidth, avail.y));
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsItemActive()) {
            rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
            if (rightPanelWidth < 240.0f) rightPanelWidth = 240.0f;
            if (rightPanelWidth > avail.x - 240.0f) rightPanelWidth = avail.x - 240.0f;
        }
        {
            float lineX = splitterMin.x + splitterWidth * 0.5f;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, splitterMin.y), ImVec2(lineX, splitterMin.y + avail.y), IM_COL32(90, 90, 90, 255), 1.0f);
        }

        ImGui::SameLine(0, 0);

        ImGui::BeginChild("LipSyncRightPanel", ImVec2(rightPanelWidth, avail.y), false);
        {
            EnsurePresetLoaded(g_CurrentLipSyncPresetIdx);

            float duration = d.Duration > 0.0f ? d.Duration : 1.0f;

            if (g_LipSyncPlaying) {
                g_LipSyncTime += ImGui::GetIO().DeltaTime;
                if (g_LipSyncTime >= duration) {
                    if (g_LipSyncLoop) {
                        g_LipSyncTime = fmodf(g_LipSyncTime, duration);
                        if (g_LipSyncAudioEnabled && !g_CachedAudioWavBlob.empty()) {
                            g_LipSyncAudioPlayer.Seek(0.0f);
                            g_LipSyncAudioPlayer.Play();
                        }
                    }
                    else {
                        g_LipSyncTime = duration;
                        g_LipSyncPlaying = false;
                        g_LipSyncAudioPlayer.Stop();
                    }
                }
            }

            // Transport row
            float animBtnSize = 24.0f;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.82f, 0.45f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.82f, 0.45f, 0.5f));

            ImTextureID playPauseTex = g_LipSyncPlaying ? g_PauseTexture : g_PlayTexture;
            if (playPauseTex) {
                if (ImGui::ImageButton("##LSPlayPause", playPauseTex, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kLipSyncGoldAccent)) {
                    g_LipSyncPlaying = !g_LipSyncPlaying;
                    if (g_LipSyncPlaying) {
                        if (g_LipSyncAudioEnabled && !g_CachedAudioWavBlob.empty()) {
                            if (g_LipSyncAudioPlayer.GetTotalDuration() == 0.0f) {
                                g_LipSyncAudioPlayer.PlayWav(g_CachedAudioWavBlob);
                            }
                            else {
                                g_LipSyncAudioPlayer.Play();
                            }
                            if (duration > 0.0f) g_LipSyncAudioPlayer.Seek(g_LipSyncTime / duration);
                        }
                    }
                    else {
                        g_LipSyncAudioPlayer.Pause();
                    }
                }
            }
            else if (ImGui::Button(g_LipSyncPlaying ? "Pause" : "Play", ImVec2(50, 0))) {
                g_LipSyncPlaying = !g_LipSyncPlaying;
                if (!g_LipSyncPlaying) g_LipSyncAudioPlayer.Pause();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_LipSyncPlaying ? "Pause" : "Play");

            ImGui::SameLine();
            if (g_StopTexture) {
                if (ImGui::ImageButton("##LSStop", g_StopTexture, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), kLipSyncGoldAccent)) {
                    g_LipSyncPlaying = false;
                    g_LipSyncTime = 0.0f;
                    g_LipSyncAudioPlayer.Stop();
                }
            }
            else if (ImGui::Button("Stop", ImVec2(50, 0))) {
                g_LipSyncPlaying = false;
                g_LipSyncTime = 0.0f;
                g_LipSyncAudioPlayer.Stop();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");

            ImGui::SameLine();
            ImVec4 loopTint = g_LipSyncLoop ? kLipSyncGoldAccent : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (g_LoopTexture) {
                if (ImGui::ImageButton("##LSLoop", g_LoopTexture, ImVec2(animBtnSize, animBtnSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), loopTint)) {
                    g_LipSyncLoop = !g_LipSyncLoop;
                }
            }
            else if (ImGui::Button(g_LipSyncLoop ? "Loop" : "Once", ImVec2(50, 0))) {
                g_LipSyncLoop = !g_LipSyncLoop;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(g_LipSyncLoop ? "Looping enabled" : "Looping disabled");

            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::TextDisabled("%.2fs / %.2fs", g_LipSyncTime, duration);

            // Preset Combo
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##HeadPreset", kLipSyncPresets[g_CurrentLipSyncPresetIdx].DisplayName)) {
                for (int p = 0; p < (int)kLipSyncPresets.size(); p++) {
                    bool isSelected = (g_CurrentLipSyncPresetIdx == p);
                    if (ImGui::Selectable(kLipSyncPresets[p].DisplayName, isSelected)) {
                        g_CurrentLipSyncPresetIdx = p;
                        EnsurePresetLoaded(p);
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select Head Mesh & Phonemes Preset");

            // Scrubber Timeline
            ImVec2 timelineSize = ImVec2(ImGui::GetContentRegionAvail().x, 22.0f);
            ImVec2 tMin = ImGui::GetCursorScreenPos();
            ImVec2 tMax = ImVec2(tMin.x + timelineSize.x, tMin.y + timelineSize.y);

            ImGui::InvisibleButton("##LSTimeline", timelineSize);
            bool timelineActive = ImGui::IsItemActive();
            bool timelineHovered = ImGui::IsItemHovered();
            if (timelineActive && ImGui::IsMouseDown(0) && timelineSize.x > 0.0f) {
                float t = (ImGui::GetMousePos().x - tMin.x) / timelineSize.x;
                t = std::clamp(t, 0.0f, 1.0f);
                g_LipSyncTime = t * duration;
                g_LipSyncPlaying = false;
                if (g_LipSyncAudioEnabled && !g_CachedAudioWavBlob.empty()) {
                    g_LipSyncAudioPlayer.Seek(t);
                    g_LipSyncAudioPlayer.Pause();
                }
            }

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(tMin, tMax, IM_COL32(40, 40, 40, 255), 3.0f);
            dl->AddRect(tMin, tMax, IM_COL32(90, 90, 90, 255), 3.0f);

            float progress = duration > 0.0f ? std::clamp(g_LipSyncTime / duration, 0.0f, 1.0f) : 0.0f;
            dl->AddRectFilled(tMin, ImVec2(tMin.x + timelineSize.x * progress, tMax.y), IM_COL32(90, 140, 200, 90), 3.0f);

            float headX = tMin.x + timelineSize.x * progress;
            dl->AddLine(ImVec2(headX, tMin.y), ImVec2(headX, tMax.y), IM_COL32(255, 255, 255, 255), 2.0f);

            if (timelineHovered && timelineSize.x > 0.0f) {
                float hoverT = std::clamp((ImGui::GetMousePos().x - tMin.x) / timelineSize.x, 0.0f, 1.0f);
                ImGui::SetTooltip("%.2fs (Frame ~%u)", hoverT * duration, (uint32_t)(hoverT * duration * d.FPS));
            }

            ImGui::Dummy(ImVec2(0, 4));

            // Viewport
            ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
            if (viewportAvail.y < 80.0f) viewportAvail.y = 80.0f;

            ImGui::BeginChild("LipSyncViewport", viewportAvail, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImVec2 pMin = ImGui::GetCursorScreenPos();
            ImVec2 pMax = ImVec2(pMin.x + viewportAvail.x, pMin.y + viewportAvail.y);

            if (!g_CachedPreset.MeshLoaded) {
                const char* msg = "Head mesh not loaded or missing from graphics.big";
                ImVec2 textSize = ImGui::CalcTextSize(msg);
                ImGui::SetCursorScreenPos(ImVec2((pMin.x + pMax.x) * 0.5f - textSize.x * 0.5f, (pMin.y + pMax.y) * 0.5f - textSize.y * 0.5f));
                ImGui::TextDisabled("%s", msg);
            }
            else {
                // Evaluate current weights
                std::unordered_map<std::string, float> currentWeights;
                if (d.FPS > 0.0f && !d.Frames.empty()) {
                    float frameFloat = g_LipSyncTime * d.FPS;
                    int f0 = (int)frameFloat;
                    int f1 = f0 + 1;
                    float alpha = frameFloat - (float)f0;

                    f0 = std::clamp(f0, 0, (int)d.Frames.size() - 1);
                    f1 = std::clamp(f1, 0, (int)d.Frames.size() - 1);

                    for (const auto& k : d.Frames[f0].Keys) {
                        std::string sym = GetSymbol(k.ID, d.Dictionary);
                        currentWeights[sym] += k.WeightFloat * (1.0f - alpha);
                    }
                    for (const auto& k : d.Frames[f1].Keys) {
                        std::string sym = GetSymbol(k.ID, d.Dictionary);
                        currentWeights[sym] += k.WeightFloat * alpha;
                    }
                }

                // Demon Door AI mapping
                if (g_CurrentLipSyncPresetIdx == 3) {
                    if (currentWeights.count("AH") && !currentWeights.count("AI")) {
                        currentWeights["AI"] = currentWeights["AH"];
                    }
                }

                UpdateLipSyncBones(g_CachedPreset.Mesh, g_CachedPreset.PhonemeAnims, currentWeights, g_LipSyncBoneMats, g_LipSyncGlobalMats);

                g_LipSyncRenderer.Resize(g_pd3dDevice, viewportAvail.x, viewportAvail.y);
                ID3D11DeviceContext* pCtx;
                g_pd3dDevice->GetImmediateContext(&pCtx);
                ID3D11ShaderResourceView* tex = g_LipSyncRenderer.Render(pCtx, viewportAvail.x, viewportAvail.y, false, false, &g_LipSyncBoneMats);
                pCtx->Release();

                if (tex) {
                    ImGui::SetCursorScreenPos(pMin);
                    ImGui::Image((void*)tex, viewportAvail);
                }

                // Viewport Controls Overlay (Corner Buttons)
                {
                    ImGui::SetCursorScreenPos(ImVec2(pMin.x + 8, pMin.y + 8));
                    ImGui::BeginGroup();

                    // Audio Toggle Button
                    ImTextureID musicTex = g_LipSyncAudioEnabled ? g_MusicOnTexture : g_MusicOffTexture;
                    if (musicTex) {
                        ImVec4 musicTint = g_LipSyncAudioEnabled ? kLipSyncGoldAccent : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                        if (ImGui::ImageButton("##LSAudioToggle", musicTex, ImVec2(22, 22), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), musicTint)) {
                            g_LipSyncAudioEnabled = !g_LipSyncAudioEnabled;
                            if (!g_LipSyncAudioEnabled) {
                                g_LipSyncAudioPlayer.Pause();
                            }
                            else if (g_LipSyncPlaying && !g_CachedAudioWavBlob.empty()) {
                                if (g_LipSyncAudioPlayer.GetTotalDuration() == 0.0f) {
                                    g_LipSyncAudioPlayer.PlayWav(g_CachedAudioWavBlob);
                                }
                                if (duration > 0.0f) g_LipSyncAudioPlayer.Seek(g_LipSyncTime / duration);
                                g_LipSyncAudioPlayer.Play();
                            }
                        }
                    }
                    else {
                        if (ImGui::Button(g_LipSyncAudioEnabled ? "Audio: On" : "Audio: Off")) {
                            g_LipSyncAudioEnabled = !g_LipSyncAudioEnabled;
                            if (!g_LipSyncAudioEnabled) {
                                g_LipSyncAudioPlayer.Pause();
                            }
                            else if (g_LipSyncPlaying && !g_CachedAudioWavBlob.empty()) {
                                if (g_LipSyncAudioPlayer.GetTotalDuration() == 0.0f) {
                                    g_LipSyncAudioPlayer.PlayWav(g_CachedAudioWavBlob);
                                }
                                if (duration > 0.0f) g_LipSyncAudioPlayer.Seek(g_LipSyncTime / duration);
                                g_LipSyncAudioPlayer.Play();
                            }
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        if (!g_CachedAudioWavBlob.empty()) {
                            ImGui::SetTooltip("Dialogue Audio: %s (Sound ID %u)", g_LipSyncAudioEnabled ? "Enabled" : "Muted", currentSoundID);
                        }
                        else {
                            ImGui::SetTooltip("Dialogue Audio: No sound line found for Sound ID %u", currentSoundID);
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset Cam")) {
                        g_LipSyncRenderer.CenterOffset = XMFLOAT3(-g_CachedPreset.Mesh.BoundingSphereCenter[0], -g_CachedPreset.Mesh.BoundingSphereCenter[1], -g_CachedPreset.Mesh.BoundingSphereCenter[2]);
                        g_LipSyncRenderer.CamDist = (g_CachedPreset.Mesh.BoundingSphereRadius > 0) ? g_CachedPreset.Mesh.BoundingSphereRadius * 2.5f : 10.0f;
                        g_LipSyncRenderer.CamPan = { 0, 0 };
                        g_LipSyncRenderer.CamRotX = -XM_PIDIV2;
                        g_LipSyncRenderer.CamRotY = XM_PI;
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset camera framing on head");

                    ImGui::EndGroup();
                }

                // Missing assets overlay
                if (!g_CachedPreset.MissingAssets.empty()) {
                    ImGui::SetCursorScreenPos(ImVec2(pMin.x + 8, pMax.y - 24.0f * g_CachedPreset.MissingAssets.size() - 8));
                    ImGui::BeginGroup();
                    for (const auto& msg : g_CachedPreset.MissingAssets) {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[!] %s", msg.c_str());
                    }
                    ImGui::EndGroup();
                }
            }

            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
}