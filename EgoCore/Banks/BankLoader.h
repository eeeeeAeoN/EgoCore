#pragma once
#include "BankBackend.h"
#include "ConfigBackend.h"
#include "BinaryParser.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "TextProperties.h"
#include "LipSyncProperties.h"
#include "FontProperties.h"
#include "StreamingFontParser.h"
#include "DefBackend.h" 
#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <set>
#include <map>
#include <regex>


inline std::vector<BinaryParser> g_LoadedBinaries;

inline std::map<uint32_t, std::string> BuildFriendlyNameMap(const std::string& headerName) {
    std::map<uint32_t, std::string> result;
    int enumIdx = FindHeaderIndex(headerName);
    if (enumIdx == -1) return result;
    const std::string& content = g_DefWorkspace.AllEnums[enumIdx].FullContent;
    std::regex re("([A-Z0-9_]+)\\s*=\\s*(\\d+)");
    std::sregex_iterator next(content.begin(), content.end(), re);
    std::sregex_iterator end;
    while (next != end) {
        try { result[std::stoul((*next)[2].str())] = (*next)[1].str(); }
        catch (...) {}
        next++;
    }
    return result;
}

static void LoadSystemBinaries(const std::string& gameRoot) {
    namespace fs = std::filesystem;
    g_LoadedBinaries.clear();
    std::vector<std::string> targetFiles = { "gamesnds.bin", "dialoguesnds.bin", "dialoguesnds.h", "scriptdialoguesnds.bin", "scriptdialoguesnds.h" };
    fs::path defsPath = fs::path(gameRoot) / "Data" / "Defs";
    for (const auto& fname : targetFiles) {
        fs::path fullPath = defsPath / fname;
        if (fs::exists(fullPath)) {
            BinaryParser parser; parser.Parse(fullPath.string());
            if (parser.Data.IsParsed) g_LoadedBinaries.push_back(std::move(parser));
        }
    }
}

inline std::string FetchTextContent(LoadedBank* bank, uint32_t id) {
    if (!bank) return "";
    for (int i = 0; i < bank->Entries.size(); ++i) {
        if (bank->Entries[i].ID == id) {
            CTextParser tempParser;
            bank->Stream->clear();
            if (bank->ModifiedEntryData.count(i)) tempParser.Parse(bank->ModifiedEntryData[i], bank->Entries[i].Type);
            else {
                bank->Stream->seekg(bank->Entries[i].Offset, std::ios::beg);
                size_t size = bank->Entries[i].Size;
                if (size > 0) {
                    std::vector<uint8_t> buffer(size + 64);
                    bank->Stream->read((char*)buffer.data(), size);
                    tempParser.Parse(buffer, bank->Entries[i].Type);
                }
            }
            if (tempParser.IsParsed && !tempParser.IsGroup && !tempParser.IsNarratorList) return WStringToString(tempParser.TextData.Content);
            return "[Content]";
        }
    }
    return "[ID Not Found]";
}

inline void ResolveGroupMetadata(LoadedBank* bank) {
    if (!g_TextParser.IsParsed || !g_TextParser.IsGroup || !bank) return;
    for (auto& item : g_TextParser.GroupData.Items) {
        bool found = false;
        for (const auto& entry : bank->Entries) {
            if (entry.ID == item.ID) { item.CachedName = entry.Name; found = true; break; }
        }
        if (!found) item.CachedName = "Unknown ID";
        if (found) item.CachedContent = FetchTextContent(bank, item.ID); else item.CachedContent = "-";
    }
}

inline void SelectEntry(LoadedBank* bank, int idx) {
    if (!bank || idx < 0 || idx >= (int)bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio) {
        if (bank->AudioParser) bank->AudioParser->Player.Reset();
        player.Reset();
    }

    g_TextureParser.DecodedPixels.clear(); g_TextureParser.IsParsed = false; g_TextureParser.IsStagedRaw = false;
    g_BBMParser.IsParsed = false; g_ActiveMeshContent = C3DMeshContent();
    g_AnimParser.Data = C3DAnimationInfo();
    g_TextParser.IsParsed = false; g_TextParser.TextData = CTextEntry(); g_TextParser.GroupData = CTextGroup(); g_TextParser.NarratorStrings.clear(); g_TextParser.RawData.clear();
    g_LipSyncParser.Data = CLipSyncData();
    g_ShaderParser.IsParsed = false;
    g_ShaderParser.Data = CShaderData();
    g_StreamingFontParser.IsParsed = false;

    bank->SelectedEntryIndex = idx; bank->SelectedLOD = 0;
    const auto& e = bank->Entries[idx];

    if (bank->StagedEntries.count(idx)) {
        const auto& staged = bank->StagedEntries[idx];

        if (!staged.MeshLODs.empty()) {
            bank->SelectedLOD = 0;
            g_ActiveMeshContent = *staged.MeshLODs[0];
            g_ActiveMeshContent.EntryMeta = staged.MeshMeta;
            g_MeshUploadNeeded = true;
        }
        else if (staged.Physics) {
            g_BBMParser = *staged.Physics;
            g_MeshUploadNeeded = true;
        }
        else if (staged.Anim) {
            g_AnimParser.Data = *staged.Anim;
            g_AnimParser.Data.IsParsed = true;
        }
        else if (staged.Texture) {
            g_TextureParser.DecodedFormat = staged.Texture->TargetFormat;
            g_TextureParser.Header = staged.Texture->Header;
            g_TextureParser.RawFrames = staged.Texture->RawFrames;
            g_TextureParser.IsParsed = true;
            g_TextureParser.IsStagedRaw = true;
            g_TextureParser.PendingName = e.Name;
        }
        else if (staged.Text) {
            g_TextParser.TextData = *staged.Text;
            g_TextParser.IsParsed = true;
            g_TextParser.IsGroup = false;
            g_TextParser.IsNarratorList = false;
        }
        else if (staged.TextGroup) {
            g_TextParser.GroupData = *staged.TextGroup;
            g_TextParser.IsParsed = true;
            g_TextParser.IsGroup = true;
            g_TextParser.IsNarratorList = false;
            ResolveGroupMetadata(bank);
        }
        else if (staged.NarratorList) {
            g_TextParser.NarratorStrings = *staged.NarratorList;
            g_TextParser.IsParsed = true;
            g_TextParser.IsNarratorList = true;
        }
        else if (staged.LipSync) {
            g_LipSyncParser.Data = *staged.LipSync;
            g_LipSyncParser.IsParsed = true;
        }

        else if (staged.ShaderCode) {
            if (bank->ModifiedEntryData.count(idx)) bank->CurrentEntryRawData = bank->ModifiedEntryData[idx];
            else {
                bank->Stream->clear();
                bank->Stream->seekg(e.Offset, std::ios::beg);
                bank->CurrentEntryRawData.resize(e.Size);
                bank->Stream->read((char*)bank->CurrentEntryRawData.data(), e.Size);
            }
            g_ShaderParser.Parse(bank->CurrentEntryRawData);
            g_ShaderParser.DecompiledText = *staged.ShaderCode;
        }

        return;
    }

    if (bank->Type != EBankType::Audio) {
        if (bank->ModifiedEntryData.count(idx)) bank->CurrentEntryRawData = bank->ModifiedEntryData[idx];
        else {
            bank->Stream->clear();
            size_t effectiveOffset = e.Offset; size_t effectiveSize = e.Size;
            if ((int)e.Type == 2 && (effectiveOffset == 0 || effectiveSize == 0)) {
                size_t maxEnd = 0;
                for (const auto& other : bank->Entries) if (other.ID != e.ID && other.Offset > 0) { size_t end = other.Offset + other.Size; if (end > maxEnd) maxEnd = end; }
                if (maxEnd > 0) effectiveOffset = maxEnd;
                bank->Stream->seekg(0, std::ios::end);
                size_t fileEnd = bank->Stream->tellg();
                effectiveSize = (fileEnd > effectiveOffset) ? fileEnd - effectiveOffset : 0;
            }
            else if (effectiveSize > 50000000) effectiveSize = 50000000;

            if (effectiveSize > 0) {
                bank->Stream->seekg(effectiveOffset, std::ios::beg);
                bank->CurrentEntryRawData.resize(effectiveSize + 64);
                bank->Stream->read((char*)bank->CurrentEntryRawData.data(), effectiveSize);
                bank->CurrentEntryRawData.resize(effectiveSize);
            }
            else bank->CurrentEntryRawData.clear();
        }
    }

    if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend || (bank->Type == EBankType::XboxGraphics && IsTextureSubBank(bank))) {
        if (bank->SubheaderCache.count(idx)) g_TextureParser.Parse(bank->SubheaderCache[idx], bank->CurrentEntryRawData, e.Type);
        g_TextureParser.PendingName = e.Name;
    }
    else if (bank->Type == EBankType::Effects) {
        g_ActiveParticleEmitter = CParticleEmitter();
        if (!bank->CurrentEntryRawData.empty()) {
            CParticleStream stream(bank->CurrentEntryRawData);
            g_ActiveParticleEmitter.Parse(stream);
        }
    }
    else if (bank->Type == EBankType::Text || bank->Type == EBankType::Dialogue) {
        if (bank->Type == EBankType::Dialogue) g_LipSyncParser.Parse(bank->CurrentEntryRawData, bank->SubheaderCache[idx]);
        g_TextParser.Parse(bank->CurrentEntryRawData, e.Type);
        if (g_TextParser.IsGroup) ResolveGroupMetadata(bank);
    }
    else if (bank->Type == EBankType::Shaders) {
        g_ShaderParser.Parse(bank->CurrentEntryRawData);
    }
    else if (bank->Type == EBankType::Fonts) {
        if (bank->ModifiedEntryData.count(idx)) bank->CurrentEntryRawData = bank->ModifiedEntryData[idx];
        else {
            bank->Stream->clear();
            bank->Stream->seekg(e.Offset, std::ios::beg);
            bank->CurrentEntryRawData.resize(e.Size);
            bank->Stream->read((char*)bank->CurrentEntryRawData.data(), e.Size);
        }
        std::string subBank = "";
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
            subBank = bank->SubBanks[bank->ActiveSubBankIndex].Name;
        }
        std::string upperSubBank = subBank;
        std::transform(upperSubBank.begin(), upperSubBank.end(), upperSubBank.begin(), ::toupper);

        if (upperSubBank.find("STREAMING") != std::string::npos) {
            g_StreamingFontParser.Parse(bank->CurrentEntryRawData, e.Type);
        }
        else {
            g_FontParser.Parse(bank->CurrentEntryRawData, subBank);
        }
    }
    else if (bank->Type != EBankType::Audio) {
        if (e.Type == 3) {
            g_BBMParser.Parse(bank->CurrentEntryRawData);
            g_MeshUploadNeeded = true;
        }
        else if (IsSupportedMesh(e.Type)) {
            if (bank->SubheaderCache.count(idx)) g_ActiveMeshContent.ParseEntryMetadata(bank->SubheaderCache[idx]);
            if (!bank->CurrentEntryRawData.empty()) { g_ActiveMeshContent.Parse(bank->CurrentEntryRawData, e.Type);; g_MeshUploadNeeded = true; }
        }
        else if (e.Type == 6 || e.Type == 7 || e.Type == 9) {
            if (bank->SubheaderCache.count(idx)) {
                g_AnimParser.Data.Deserialize(bank->SubheaderCache[idx]);
            }
            if (!bank->CurrentEntryRawData.empty()) {
                g_AnimParser.Parse(bank->CurrentEntryRawData);
            }
        }
    }
}

inline void LoadSubBankEntries(LoadedBank* bank, int subBankIndex) {
    if (subBankIndex < 0 || subBankIndex >= bank->SubBanks.size()) return;
    if (!bank->Stream->is_open()) return;

    bank->ActiveSubBankIndex = subBankIndex;
    bank->Entries.clear();
    bank->FilteredIndices.clear();
    bank->SubheaderCache.clear();

    auto& info = bank->SubBanks[subBankIndex];

    bank->Stream->seekg(info.Offset, std::ios::beg);

    uint32_t statsCount = 0;
    bank->Stream->read((char*)&statsCount, 4);

    info.HeaderData.clear();

    if (statsCount < 1000) {
        info.HeaderData.push_back(statsCount);
        for (uint32_t k = 0; k < statsCount * 2; ++k) {
            uint32_t val = 0;
            bank->Stream->read((char*)&val, 4);
            info.HeaderData.push_back(val);
        }
    }
    else {
        bank->Stream->seekg(-4, std::ios::cur);
        info.HeaderData.push_back(0);
    }

    std::string headerFile = GetHeaderForSubBank(info.Name);
    std::map<uint32_t, std::string> friendlyNames;
    if (!headerFile.empty()) friendlyNames = BuildFriendlyNameMap(headerFile);

    for (uint32_t i = 0; i < info.EntryCount; i++) {
        BankEntry e;
        uint32_t magicE;

        bank->Stream->read((char*)&magicE, 4);
        bank->Stream->read((char*)&e.ID, 4);
        bank->Stream->read((char*)&e.Type, 4);
        bank->Stream->read((char*)&e.Size, 4);
        bank->Stream->read((char*)&e.Offset, 4);
        bank->Stream->read((char*)&e.CRC, 4);

        if (magicE != 42) {
            // If magic is wrong (like with CBox entries), we might need to skip bytes or abort, 
            // but usually we just skip adding it to the list.
            // We continue reading to maintain file pointer consistency for now.
        }

        e.Name = ReadBankString(*bank->Stream);
        if (friendlyNames.count(e.ID)) e.FriendlyName = friendlyNames[e.ID];
        else e.FriendlyName = e.Name;

        bank->Stream->read((char*)&e.Timestamp, 4);

        uint32_t depCount = 0;
        bank->Stream->read((char*)&depCount, 4);
        for (uint32_t d = 0; d < depCount; d++) {
            e.Dependencies.push_back(ReadBankString(*bank->Stream));
        }

        bank->Stream->read((char*)&e.InfoSize, 4);
        e.SubheaderFileOffset = (uint32_t)bank->Stream->tellg();

        if (e.InfoSize > 0) {
            std::vector<uint8_t> infoBuf(e.InfoSize);
            bank->Stream->read((char*)infoBuf.data(), e.InfoSize);
            bank->SubheaderCache[i] = infoBuf;
        }

        bool isValidEntry = (magicE == 42) || (e.Size > 0 && e.ID > 0);

        if (isValidEntry) {
            bank->Entries.push_back(e);
            bank->FilteredIndices.push_back((int)bank->Entries.size() - 1);
        }
    }

    UpdateFilter(*bank);
    g_BankStatus = "Loaded: " + info.Name;
}

inline void InitializeBank(LoadedBank& bank) {
    if (bank.Type == EBankType::Textures) {
        int tIdx = -1;
        for (int i = 0; i < (int)bank.SubBanks.size(); i++) {
            if (bank.SubBanks[i].Name == "GBANK_MAIN_PC") { tIdx = i; break; }
            if (bank.SubBanks[i].Name == "GBANK_GUI_PC") tIdx = i;
        }
        if (tIdx != -1) LoadSubBankEntries(&bank, tIdx);
    }
    else if (bank.Type == EBankType::Graphics || bank.Type == EBankType::XboxGraphics) {
        for (int i = 0; i < bank.SubBanks.size(); i++) if (bank.SubBanks[i].Name == "MBANK_ALLMESHES") { LoadSubBankEntries(&bank, i); break; }
    }
    else if (!bank.SubBanks.empty()) {
        LoadSubBankEntries(&bank, 0);
    }
}

inline std::unique_ptr<LoadedBank> CreateBankFromDisk(const std::string& path) {
    auto newBank = std::make_unique<LoadedBank>();
    newBank->FullPath = path;
    newBank->FileName = fs::path(path).filename().string();
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".lut") {
        newBank->Type = EBankType::Audio;
        newBank->AudioParser = std::make_shared<AudioBankParser>();
        std::string headerName = GetHeaderName(newBank->FileName);
        std::map<uint32_t, std::string> friendlyNames = BuildFriendlyNameMap(headerName);

        if (newBank->AudioParser->Parse(path)) {
            for (size_t i = 0; i < newBank->AudioParser->Entries.size(); i++) {
                const auto& audioEntry = newBank->AudioParser->Entries[i];
                BankEntry be;
                be.ID = audioEntry.SoundID;
                be.Name = "Sound ID " + std::to_string(audioEntry.SoundID);
                if (friendlyNames.count(be.ID)) be.FriendlyName = friendlyNames[be.ID];
                else be.FriendlyName = be.Name;
                be.Size = audioEntry.Length;
                be.Offset = audioEntry.Offset;
                newBank->Entries.push_back(be);
                newBank->FilteredIndices.push_back((int)i);
            }
            return newBank;
        }
        return nullptr;
    }
    else if (ext == ".lug") {
        newBank->Type = EBankType::Audio;
        newBank->LugParserPtr = std::make_shared<LugParser>();
        newBank->AudioParser = std::make_shared<AudioBankParser>();

        if (newBank->LugParserPtr->Parse(path)) {
            for (size_t i = 0; i < newBank->LugParserPtr->Entries.size(); i++) {
                const auto& le = newBank->LugParserPtr->Entries[i];
                BankEntry be;
                be.ID = le.SoundID;
                be.Name = le.Name;
                be.FriendlyName = le.Name;
                be.Size = le.Length;
                be.Offset = le.Offset;
                be.Dependencies.push_back(le.FullPath);
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

    char magic[4];
    newBank->Stream->read(magic, 4);
    newBank->Stream->seekg(0, std::ios::beg);

    if (strncmp(magic, "BIGB", 4) != 0) return nullptr;

    struct HeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; } h;
    newBank->Stream->read((char*)&h, sizeof(h));
    newBank->FileVersion = h.v;
    newBank->Stream->seekg(h.footOff, std::ios::beg);
    uint32_t bankCount = 0;
    newBank->Stream->read((char*)&bankCount, 4);

    for (uint32_t i = 0; i < bankCount; i++) {
        InternalBankInfo b;
        std::getline(*newBank->Stream, b.Name, '\0');
        newBank->Stream->read((char*)&b.Version, 4);
        newBank->Stream->read((char*)&b.EntryCount, 4);
        newBank->Stream->read((char*)&b.Offset, 4);
        newBank->Stream->read((char*)&b.Size, 4);
        newBank->Stream->read((char*)&b.Align, 4);
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
    if (newBank) { g_OpenBanks.push_back(std::move(*newBank)); g_ActiveBankIndex = (int)g_OpenBanks.size() - 1; g_BankStatus = "Loaded: " + g_OpenBanks.back().FileName; }
    else g_BankStatus = "Failed to load bank.";
}

inline void JumpToBankEntry(const std::string& targetFile, uint32_t id, const std::string& speechBankRef) {
    auto findBankIndex = [&](const std::string& tFile) -> int {
        std::string search = tFile; std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        for (int i = 0; i < g_OpenBanks.size(); i++) {
            std::string fname = g_OpenBanks[i].FileName; std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
            if (fname.find(search) != std::string::npos) return i;
        }
        return -1;
        };

    int bankIdx = findBankIndex(targetFile);

    if (bankIdx == -1) {
        std::vector<std::string> pathsToTry;
        const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };

        for (const char* l : langs) {
            pathsToTry.push_back(g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\" + targetFile);
        }
        pathsToTry.push_back(g_AppConfig.GameRootPath + "\\Data\\" + targetFile);

        for (const auto& p : pathsToTry) {
            if (std::filesystem::exists(p)) {
                LoadBank(p);
                break;
            }
        }
        bankIdx = findBankIndex(targetFile);
    }

    if (bankIdx == -1) {
        g_BankStatus = "Jump failed: Could not find/load " + targetFile;
        return;
    }

    auto& bank = g_OpenBanks[bankIdx];
    g_ActiveBankIndex = bankIdx;
    g_ForceTabSwitch = true;

    if (!speechBankRef.empty() && !bank.SubBanks.empty()) {
        std::string targetSub = GetSubBankNameForSpeech(speechBankRef);
        if (!targetSub.empty()) {
            for (int s = 0; s < bank.SubBanks.size(); s++) {
                if (bank.SubBanks[s].Name == targetSub) {
                    if (bank.ActiveSubBankIndex != s) LoadSubBankEntries(&bank, s);
                    break;
                }
            }
        }
    }

    for (int k = 0; k < bank.Entries.size(); k++) {
        if (bank.Entries[k].ID == id) {
            SelectEntry(&bank, k);
            return;
        }
    }

    g_BankStatus = "Jump failed: ID " + std::to_string(id) + " not found.";
}

inline void ParseSelectedLOD(LoadedBank* bank) {
    if (!bank) return;
    int idx = bank->SelectedEntryIndex;

    if (bank->StagedEntries.count(idx) && !bank->StagedEntries[idx].MeshLODs.empty()) {
        auto& staged = bank->StagedEntries[idx];
        if (bank->SelectedLOD >= staged.MeshLODs.size()) bank->SelectedLOD = 0;
        g_ActiveMeshContent = *staged.MeshLODs[bank->SelectedLOD];
        g_ActiveMeshContent.EntryMeta = staged.MeshMeta;
        g_MeshUploadNeeded = true;
        return;
    }

    if (bank->CurrentEntryRawData.empty()) return;
    size_t offset = 0; size_t size = bank->CurrentEntryRawData.size();
    if (g_ActiveMeshContent.EntryMeta.HasData && g_ActiveMeshContent.EntryMeta.LODCount > 0) {
        if (bank->SelectedLOD >= g_ActiveMeshContent.EntryMeta.LODCount) bank->SelectedLOD = 0;
        size_t currentOffset = 0;
        for (int i = 0; i < bank->SelectedLOD; i++) currentOffset += g_ActiveMeshContent.EntryMeta.LODSizes[i];
        size_t currentSize = g_ActiveMeshContent.EntryMeta.LODSizes[bank->SelectedLOD];
        if (currentOffset + currentSize <= bank->CurrentEntryRawData.size()) { offset = currentOffset; size = currentSize; }
    }
    std::vector<uint8_t> slice;
    if (size > 0) { slice.resize(size); memcpy(slice.data(), bank->CurrentEntryRawData.data() + offset, size); }
    g_ActiveMeshContent.Parse(slice, bank->Entries[idx].Type);
    g_MeshUploadNeeded = true;
}

inline void ReloadBankInPlace(LoadedBank* bank) {
    if (!bank) return;
    std::string path = bank->FullPath;
    if (bank->Stream && bank->Stream->is_open()) bank->Stream->close();
    auto newBankPtr = CreateBankFromDisk(path);
    if (newBankPtr) { *bank = std::move(*newBankPtr); g_BankStatus = "Bank Reloaded."; }
    else g_BankStatus = "Error: Failed to reload bank after compilation!";
}

static void PerformAutoLoad() {
    if (!g_AppConfig.IsConfigured) return;
    LoadDefsFromFolder(g_AppConfig.GameRootPath);
    for (const auto& relativePath : g_AppConfig.AutoLoadBanks) {
        std::string fullPath = g_AppConfig.GameRootPath + relativePath; if (fs::exists(fullPath)) LoadBank(fullPath);
    }
}

static void InitializeSetup(const std::string& selectedPath) {
    g_AppConfig.GameRootPath = selectedPath; g_AppConfig.AutoLoadBanks = DEFAULT_BANKS;
    g_AppConfig.IsConfigured = true; SaveConfig(); PerformAutoLoad();
}