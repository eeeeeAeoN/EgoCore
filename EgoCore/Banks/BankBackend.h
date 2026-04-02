#pragma once
#include "Utils.h"
#include "MeshParser.h"
#include "BBMParser.h"
#include "AnimParser.h"
#include "TextureParser.h"
#include "ParticleParser.h"
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
#include "ShaderParser.h"

namespace fs = std::filesystem;

inline bool g_ShowSuccessPopup = false;
inline std::string g_SuccessMessage = "";

inline std::string g_ShaderCompileError;
inline bool g_ShowShaderErrorPopup = false;

enum class EBankType {
    Unknown, Graphics, Textures, Frontend, Effects, Text, Dialogue, Fonts, Shaders, Audio, XboxGraphics
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
    uint32_t Timestamp = 0;

    std::vector<std::string> Dependencies;
};

struct InternalBankInfo {
    std::string Name;
    uint32_t Version;
    uint32_t EntryCount;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Align;
    std::vector<uint32_t> HeaderData;
};

enum class EFilterMode { Name, ID, Speaker };

struct StagedTextureInfo {
    ETextureFormat TargetFormat = ETextureFormat::DXT3;
    CGraphicHeader Header;
    std::vector<std::vector<uint8_t>> RawFrames; 
};


struct CLipSyncData;

struct StagedEntry {
    std::vector<std::shared_ptr<C3DMeshContent>> MeshLODs; 
    CMeshEntryMetadata MeshMeta; 

    std::shared_ptr<CBBMParser> Physics;
    std::shared_ptr<C3DAnimationInfo> Anim;
    std::shared_ptr<StagedTextureInfo> Texture;
    std::shared_ptr<CTextEntry> Text;
    std::shared_ptr<CTextGroup> TextGroup;
    std::shared_ptr<std::vector<std::string>> NarratorList;
    std::shared_ptr<CLipSyncData> LipSync;
    std::shared_ptr<std::string> ShaderCode;
    std::shared_ptr<CParticleEmitter> Particle;
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
    std::shared_ptr<LugParser> LugParserPtr;
    std::shared_ptr<MetParser> MetParserPtr;

    int SelectedEntryIndex = -1;
    int SelectedLOD = 0;

    char FilterText[128] = "";
    EFilterMode FilterMode = EFilterMode::Name;
    int FilterTypeMask = -1;
    int FilterTextureFormatMask = -1;

    std::map<int, std::vector<uint8_t>> SubheaderCache;
    std::vector<uint8_t> CurrentEntryRawData;

    std::map<int, StagedEntry> StagedEntries; 

    std::map<int, std::vector<uint8_t>> ModifiedEntryData; 

    LoadedBank() {
        Stream = std::make_unique<std::fstream>();
    }

    LoadedBank(const LoadedBank&) = delete;
    LoadedBank& operator=(const LoadedBank&) = delete;
    LoadedBank(LoadedBank&&) = default;
    LoadedBank& operator=(LoadedBank&&) = default;
};

inline std::vector<LoadedBank> g_OpenBanks;
inline int g_ActiveBankIndex = -1;
inline bool g_ForceTabSwitch = false;
inline std::string g_BankStatus = "Ready";

inline C3DMeshContent g_ActiveMeshContent;
inline CBBMParser g_BBMParser;
inline CTextureParser g_TextureParser;
inline CParticleEmitter g_ActiveParticleEmitter;
inline bool g_MeshUploadNeeded = false;
inline C3DAnimationInfo g_ActiveAnim;
inline bool             g_AnimParseSuccess = false;
inline AnimUIContext g_AnimUIState;
inline AnimParser g_AnimParser;
inline CShaderParser g_ShaderParser;

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

inline bool IsTextureSubBank(LoadedBank* bank) {
    if (!bank || bank->ActiveSubBankIndex < 0 || bank->ActiveSubBankIndex >= bank->SubBanks.size()) return false;
    return StartsWith(bank->SubBanks[bank->ActiveSubBankIndex].Name, "GBANK");
}

inline bool IsGraphicsSubBank(LoadedBank* bank) {
    if (!bank || bank->ActiveSubBankIndex < 0 || bank->ActiveSubBankIndex >= bank->SubBanks.size()) return false;
    return StartsWith(bank->SubBanks[bank->ActiveSubBankIndex].Name, "MBANK");
}

inline EBankType ResolveBankType(const std::vector<InternalBankInfo>& subBanks) {
    std::set<std::string> folders;
    for (const auto& sb : subBanks) folders.insert(sb.Name);

    if (folders.count("MBANK_ALLMESHES") && (folders.count("GBANK_MAIN") || folders.count("GBANK_GUI"))) return EBankType::XboxGraphics;
    if (folders.count("GBANK_MAIN_PC") || folders.count("GBANK_GUI_PC")) return EBankType::Textures;
    if (folders.count("MBANK_ALLMESHES")) return EBankType::Graphics;
    if (folders.count("GBANK_FRONT_END_PC")) return EBankType::Frontend;

    for (const auto& folder : folders) {
        if (StartsWith(folder, "TEXT_")) return EBankType::Text;
        if (StartsWith(folder, "LIPSYNC_")) return EBankType::Dialogue;

        std::string upperFolder = folder;
        std::transform(upperFolder.begin(), upperFolder.end(), upperFolder.begin(), ::toupper);
        if (upperFolder.find("SHADER") != std::string::npos) return EBankType::Shaders;
        if (upperFolder.find("FONT") != std::string::npos) return EBankType::Fonts;
    }
    if (folders.count("PARTICLE_MAIN_PC")) return EBankType::Effects;
    return EBankType::Unknown;
}

inline std::string PeekSpeakerFast(LoadedBank& bank, int index) {
    const auto& e = bank.Entries[index];
    if (e.Type != 0) return ""; 

    if (bank.ModifiedEntryData.count(index)) {
        CTextParser p; p.Parse(bank.ModifiedEntryData[index], 0);
        return p.IsParsed ? p.TextData.Speaker : "";
    }

    if (!bank.Stream->is_open()) return "";
    bank.Stream->clear();
    bank.Stream->seekg(e.Offset, std::ios::beg);

    std::vector<uint8_t> buf(2048);
    bank.Stream->read((char*)buf.data(), (std::min)((uint32_t)buf.size(), e.Size));

    size_t cursor = 0;
    size_t max = buf.size();

    while (cursor + 2 <= max) {
        uint16_t c = *(uint16_t*)(buf.data() + cursor);
        cursor += 2;
        if (c == 0) break;
    }

    if (cursor + 4 > max) return "";
    uint32_t len = *(uint32_t*)(buf.data() + cursor);
    cursor += 4 + len;

    if (cursor + 4 > max) return "";
    len = *(uint32_t*)(buf.data() + cursor);
    cursor += 4;

    if (len > 0 && cursor + len <= max) {
        return std::string((char*)(buf.data() + cursor), len);
    }
    return "";
}

inline ETextureFormat PeekTextureFormatFast(LoadedBank& bank, int index) {
    if (!bank.SubheaderCache.count(index)) return ETextureFormat::Unknown;
    const auto& meta = bank.SubheaderCache[index];
    if (meta.size() < 28) return ETextureFormat::Unknown;

    CGraphicHeader header;
    memcpy(&header, meta.data(), 28);

    CPixelFormatInit formatInfo = { 0, 0, 0, 0, 0, 0 };
    if (meta.size() >= 34) memcpy(&formatInfo, meta.data() + 28, 6);

    bool isBump = (bank.Entries[index].Type == 0x2 || bank.Entries[index].Type == 0x3);

    if (formatInfo.ColourDepth == 32) return ETextureFormat::ARGB8888;

    if (isBump) {
        if (header.TransparencyType == 0 || header.TransparencyType == 2) return ETextureFormat::NormalMap_DXT1;
        else return ETextureFormat::NormalMap_DXT5;
    }
    else {
        switch (header.TransparencyType) {
        case 0: return ETextureFormat::DXT1;
        case 1: return ETextureFormat::DXT3;
        case 2: return ETextureFormat::DXT1;
        case 3: return ETextureFormat::DXT5;
        case 4: return ETextureFormat::DXT3;
        default: return ETextureFormat::DXT1;
        }
    }
}

inline void UpdateFilter(LoadedBank& bank) {
    bank.FilteredIndices.clear();
    std::string filter = bank.FilterText;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    bool isTextureBank = (bank.Type == EBankType::Textures || bank.Type == EBankType::Frontend || bank.Type == EBankType::Effects || (bank.Type == EBankType::XboxGraphics && IsTextureSubBank(&bank)));

    for (size_t i = 0; i < bank.Entries.size(); i++) {
        if (bank.FilterTypeMask != -1) {
            if (bank.Entries[i].Type != bank.FilterTypeMask) continue;
        }

        if (isTextureBank && bank.FilterTextureFormatMask != -1) {
            ETextureFormat fmt = PeekTextureFormatFast(bank, (int)i);
            int mappedFmt = -1;
            if (fmt == ETextureFormat::DXT1 || fmt == ETextureFormat::NormalMap_DXT1) mappedFmt = 0;
            else if (fmt == ETextureFormat::DXT3) mappedFmt = 1;
            else if (fmt == ETextureFormat::DXT5 || fmt == ETextureFormat::NormalMap_DXT5) mappedFmt = 2;
            else if (fmt == ETextureFormat::ARGB8888) mappedFmt = 3;

            if (mappedFmt != bank.FilterTextureFormatMask) continue;
        }

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

// -- I know a project of all headers isn't even remotely how one should program. I don't care, bite me! --
inline void FlushStagedEntries(LoadedBank* bank);
inline void SaveEntryChanges(LoadedBank* bank);

inline void SaveBigBank(LoadedBank* bank);
inline void SaveAudioBank(LoadedBank* bank);