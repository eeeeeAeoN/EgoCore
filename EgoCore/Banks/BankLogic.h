#pragma once
#include "BankBackend.h"
#include "ConfigBackend.h"
#include "BinaryParser.h"
#include "MeshProperties.h"
#include "AnimProperties.h"
#include "TextureProperties.h"
#include "TextProperties.h"
#include "LipSyncProperties.h"
#include "GltfExporter.h"
#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <set>

// --- GLOBAL STATE ---
static std::vector<BinaryParser> g_LoadedBinaries;

// --- UTILS ---
inline void WriteBankString(std::ofstream& out, const std::string& s) {
    uint32_t len = (uint32_t)s.length();
    out.write((char*)&len, 4);
    if (len > 0) out.write(s.data(), len);
}

// --- LOGIC FUNCTIONS ---

static void LoadSystemBinaries(const std::string& gameRoot) {
    namespace fs = std::filesystem;
    g_LoadedBinaries.clear();

    std::vector<std::string> targetFiles = {
        "gamesnds.bin",
        "dialoguesnds.bin",         "dialoguesnds.h",
        "dialoguesnds2.bin",        "dialoguesnds2.h",
        "scriptdialoguesnds.bin",   "scriptdialoguesnds.h",
        "scriptdialoguesnds2.bin",  "scriptdialoguesnds2.h"
    };

    fs::path defsPath = fs::path(gameRoot) / "Data" / "Defs";

    for (const auto& fname : targetFiles) {
        fs::path fullPath = defsPath / fname;
        if (fs::exists(fullPath)) {
            bool alreadyLoaded = false;
            for (const auto& loaded : g_LoadedBinaries) {
                if (loaded.Data.FileName == fname) alreadyLoaded = true;
            }
            if (alreadyLoaded) continue;

            BinaryParser parser;
            parser.Parse(fullPath.string());
            if (parser.Data.IsParsed) {
                g_LoadedBinaries.push_back(std::move(parser));
            }
        }
    }
}

inline std::string FetchTextContent(LoadedBank* bank, uint32_t id) {
    if (!bank) return "";
    for (int i = 0; i < bank->Entries.size(); ++i) {
        if (bank->Entries[i].ID == id) {
            CTextParser tempParser;
            bank->Stream->clear();
            if (bank->ModifiedEntryData.count(i)) {
                tempParser.Parse(bank->ModifiedEntryData[i], bank->Entries[i].Type);
            }
            else {
                bank->Stream->seekg(bank->Entries[i].Offset, std::ios::beg);
                size_t size = bank->Entries[i].Size;
                if (size > 0) {
                    std::vector<uint8_t> buffer(size + 64);
                    bank->Stream->read((char*)buffer.data(), size);
                    tempParser.Parse(buffer, bank->Entries[i].Type);
                }
            }
            if (tempParser.IsParsed && !tempParser.IsGroup && !tempParser.IsNarratorList) {
                return WStringToString(tempParser.TextData.Content);
            }
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
            if (entry.ID == item.ID) {
                item.CachedName = entry.Name;
                found = true;
                break;
            }
        }
        if (!found) item.CachedName = "Unknown ID";
        if (found) item.CachedContent = FetchTextContent(bank, item.ID);
        else item.CachedContent = "-";
    }
}

// --- STANDARD BANK FUNCTIONS ---

inline void SelectEntry(LoadedBank* bank, int idx) {
    if (!bank || idx < 0 || idx >= (int)bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->Player.Reset();
    }

    g_TextureParser.DecodedPixels.clear();
    g_TextureParser.IsParsed = false;
    std::vector<uint8_t>().swap(g_TextureParser.DecodedPixels);

    g_BBMParser.IsParsed = false;
    g_ActiveMeshContent = C3DMeshContent();
    g_AnimParseSuccess = false;

    g_TextParser.IsParsed = false;
    g_TextParser.TextData = CTextEntry();
    g_TextParser.GroupData = CTextGroup();
    g_TextParser.NarratorStrings.clear();
    g_TextParser.RawData.clear();
    g_LipSyncParser.Data = CLipSyncData();

    bank->SelectedEntryIndex = idx;
    bank->SelectedLOD = 0;
    const auto& e = bank->Entries[idx];

    if (bank->Type != EBankType::Audio) {
        if (bank->ModifiedEntryData.count(idx)) {
            bank->CurrentEntryRawData = bank->ModifiedEntryData[idx];
        }
        else {
            bank->Stream->clear();
            size_t effectiveOffset = e.Offset;
            size_t effectiveSize = e.Size;

            if ((int)e.Type == 2 && (effectiveOffset == 0 || effectiveSize == 0)) {
                size_t maxEnd = 0;
                for (const auto& other : bank->Entries) {
                    if (other.ID != e.ID && other.Offset > 0) {
                        size_t end = other.Offset + other.Size;
                        if (end > maxEnd) maxEnd = end;
                    }
                }
                if (maxEnd > 0) effectiveOffset = maxEnd;
                bank->Stream->seekg(0, std::ios::end);
                size_t fileEnd = bank->Stream->tellg();
                if (fileEnd > effectiveOffset) effectiveSize = fileEnd - effectiveOffset;
                else effectiveSize = 0;
            }
            else if (effectiveSize > 50000000) effectiveSize = 50000000;

            if (effectiveSize > 0) {
                bank->Stream->seekg(effectiveOffset, std::ios::beg);
                bank->CurrentEntryRawData.resize(effectiveSize + 64);
                bank->Stream->read((char*)bank->CurrentEntryRawData.data(), effectiveSize);
                bank->CurrentEntryRawData.resize(effectiveSize);
            }
            else {
                bank->CurrentEntryRawData.clear();
            }
        }
    }

    if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend) {
        if (bank->SubheaderCache.count(idx)) g_TextureParser.Parse(bank->SubheaderCache[idx], bank->CurrentEntryRawData, e.Type);
    }
    else if (bank->Type == EBankType::Text || bank->Type == EBankType::Dialogue) {
        if (bank->Type == EBankType::Dialogue) g_LipSyncParser.Parse(bank->CurrentEntryRawData, bank->SubheaderCache[idx]);
        g_TextParser.Parse(bank->CurrentEntryRawData, e.Type);
        if (g_TextParser.IsGroup) ResolveGroupMetadata(bank);
    }
    else if (bank->Type != EBankType::Audio) {
        if (e.Type == TYPE_STATIC_PHYSICS_MESH) {
            g_BBMParser.Parse(bank->CurrentEntryRawData);
            g_MeshUploadNeeded = true;
        }
        else if (IsSupportedMesh(e.Type)) {
            if (bank->SubheaderCache.count(idx)) g_ActiveMeshContent.ParseEntryMetadata(bank->SubheaderCache[idx]);
            if (!bank->CurrentEntryRawData.empty()) {
                g_ActiveMeshContent.Parse(bank->CurrentEntryRawData);
                g_MeshUploadNeeded = true;
            }
        }
        else if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
            if (bank->SubheaderCache.count(idx)) g_AnimParseSuccess = g_ActiveAnim.Deserialize(bank->SubheaderCache[idx]);
        }
    }
}

// --- ADD/DELETE LOGIC ---

inline uint32_t GetNextFreeID(LoadedBank* bank) {
    uint32_t maxID = 0;
    for (const auto& e : bank->Entries) {
        if (e.ID > maxID) maxID = e.ID;
    }
    return maxID + 1;
}

inline void CreateNewTextEntry(LoadedBank* bank, int type) {
    if (!bank) return;

    BankEntry newEntry;
    newEntry.ID = GetNextFreeID(bank);
    newEntry.Type = type;
    newEntry.Offset = 0;
    newEntry.Size = 0;
    newEntry.Name = "New_Entry_" + std::to_string(newEntry.ID);

    CTextParser tempParser;
    if (type == 0) {
        tempParser.IsGroup = false;
        tempParser.TextData.Content = L"New Text Content";
        tempParser.TextData.Identifier = "NEW_ID_" + std::to_string(newEntry.ID);
        tempParser.TextData.Speaker = "NoSpeaker";
        tempParser.TextData.SpeechBank = "";
    }
    else if (type == 1) {
        tempParser.IsGroup = true;
    }

    std::vector<uint8_t> data = tempParser.Recompile();
    newEntry.Size = (uint32_t)data.size();

    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;

    bank->ModifiedEntryData[newIndex] = data;

    bank->FilterText[0] = '\0';
    UpdateFilter(*bank);
    SelectEntry(bank, newIndex);
}

inline void DeleteBankEntry(LoadedBank* bank, int index) {
    if (!bank || index < 0 || index >= bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->DeleteEntry(index);
        // FIX: Use -> for pointer
        bank->Entries.erase(bank->Entries.begin() + index);
        UpdateFilter(*bank);
        bank->SelectedEntryIndex = -1;
        return;
    }

    bank->Entries.erase(bank->Entries.begin() + index);

    if (bank->ModifiedEntryData.count(index)) bank->ModifiedEntryData.erase(index);
    if (bank->SubheaderCache.count(index)) bank->SubheaderCache.erase(index);

    std::map<int, std::vector<uint8_t>> newCache;
    for (auto& [k, v] : bank->ModifiedEntryData) {
        if (k < index) newCache[k] = v;
        else if (k > index) newCache[k - 1] = v;
    }
    bank->ModifiedEntryData = newCache;

    bank->SelectedEntryIndex = -1;
    UpdateFilter(*bank);
}

// --- HELPER ---
inline void ParseSelectedLOD(LoadedBank* bank) {
    if (!bank || bank->CurrentEntryRawData.empty()) return;
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

    g_ActiveMeshContent.Parse(slice);
    g_MeshUploadNeeded = true;
}

inline void SaveBigBank(LoadedBank* bank) {
    if (!bank || !bank->Stream->is_open()) return;

    std::string tempPath = bank->FullPath + ".tmp";
    std::ofstream out(tempPath, std::ios::binary);
    if (!out.is_open()) { g_BankStatus = "Failed to create temp file."; return; }

    struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
    TempHeaderBIG header = { {'B','I','G','B'}, 2, 0, 0 };
    out.write((char*)&header, sizeof(header));

    std::vector<std::vector<BankEntry>> allSubBankEntries;

    bank->Stream->clear();

    for (int sbIdx = 0; sbIdx < (int)bank->SubBanks.size(); sbIdx++) {
        std::vector<BankEntry> currentEntries;

        if (sbIdx == bank->ActiveSubBankIndex) {
            currentEntries = bank->Entries;
            for (int i = 0; i < (int)currentEntries.size(); i++) {
                BankEntry& e = currentEntries[i];
                e.Offset = (uint32_t)out.tellp();

                if (bank->ModifiedEntryData.count(i)) {
                    out.write((char*)bank->ModifiedEntryData[i].data(), bank->ModifiedEntryData[i].size());
                    e.Size = (uint32_t)bank->ModifiedEntryData[i].size();
                }
                else {
                    std::vector<uint8_t> buffer(e.Size);
                    uint32_t oldOffset = bank->Entries[i].Offset;
                    bank->Stream->seekg(oldOffset, std::ios::beg);
                    bank->Stream->read((char*)buffer.data(), e.Size);
                    out.write((char*)buffer.data(), e.Size);
                }
            }
        }
        else {
            const auto& info = bank->SubBanks[sbIdx];
            bank->Stream->seekg(info.Offset, std::ios::beg);

            uint32_t statsCount = 0; bank->Stream->read((char*)&statsCount, 4);
            if (statsCount < 1000) bank->Stream->seekg(statsCount * 8, std::ios::cur);
            else bank->Stream->seekg(-4, std::ios::cur);

            for (uint32_t k = 0; k < info.EntryCount; k++) {
                BankEntry e; uint32_t magic;
                bank->Stream->read((char*)&magic, 4); bank->Stream->read((char*)&e.ID, 4);
                bank->Stream->read((char*)&e.Type, 4); bank->Stream->read((char*)&e.Size, 4);
                bank->Stream->read((char*)&e.Offset, 4); bank->Stream->read((char*)&e.CRC, 4);

                if (magic == 42) {
                    e.Name = ReadBankString(*bank->Stream);
                    bank->Stream->seekg(4, std::ios::cur);
                    uint32_t depC = 0; bank->Stream->read((char*)&depC, 4);
                    for (uint32_t d = 0; d < depC; d++) ReadBankString(*bank->Stream);
                }

                bank->Stream->read((char*)&e.InfoSize, 4);
                e.SubheaderFileOffset = (uint32_t)bank->Stream->tellg();
                if (e.InfoSize > 0) bank->Stream->seekg(e.InfoSize, std::ios::cur);

                uint32_t oldDataOffset = e.Offset;
                e.Offset = (uint32_t)out.tellp();

                std::vector<uint8_t> buffer(e.Size);
                std::streampos savePos = bank->Stream->tellg();
                bank->Stream->seekg(oldDataOffset, std::ios::beg);
                bank->Stream->read((char*)buffer.data(), e.Size);
                out.write((char*)buffer.data(), e.Size);

                bank->Stream->seekg(savePos);

                currentEntries.push_back(e);
            }
        }
        allSubBankEntries.push_back(currentEntries);
    }

    header.footOff = (uint32_t)out.tellp();

    uint32_t bankCount = (uint32_t)bank->SubBanks.size();
    out.write((char*)&bankCount, 4);

    std::streampos bankInfoStart = out.tellp();
    for (const auto& info : bank->SubBanks) {
        InternalBankInfo dummy = info;
        WriteBankString(out, dummy.Name);
        out.write((char*)&dummy.Version, 4); out.write((char*)&dummy.EntryCount, 4);
        out.write((char*)&dummy.Offset, 4); out.write((char*)&dummy.Size, 4); out.write((char*)&dummy.Align, 4);
    }

    std::vector<uint32_t> entryTableOffsets;
    for (int i = 0; i < (int)allSubBankEntries.size(); i++) {
        entryTableOffsets.push_back((uint32_t)out.tellp());
        uint32_t stats = 0; out.write((char*)&stats, 4);

        for (const auto& e : allSubBankEntries[i]) {
            uint32_t magic = 42;
            out.write((char*)&magic, 4); out.write((char*)&e.ID, 4);
            out.write((char*)&e.Type, 4); out.write((char*)&e.Size, 4);
            out.write((char*)&e.Offset, 4); out.write((char*)&e.CRC, 4);

            WriteBankString(out, e.Name);
            uint32_t pad = 0; out.write((char*)&pad, 4);

            uint32_t depCount = 0; out.write((char*)&depCount, 4);

            uint32_t isz = 0;
            if (i == bank->ActiveSubBankIndex) {
                out.write((char*)&isz, 4);
            }
            else {
                isz = e.InfoSize;
                out.write((char*)&isz, 4);
                if (isz > 0) {
                    std::vector<uint8_t> m(isz);
                    bank->Stream->seekg(e.SubheaderFileOffset, std::ios::beg);
                    bank->Stream->read((char*)m.data(), isz);
                    out.write((char*)m.data(), isz);
                }
            }
        }
    }

    std::streampos endPos = out.tellp();
    header.footSz = (uint32_t)(endPos - (std::streampos)header.footOff);

    out.seekp(bankInfoStart);
    for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
        InternalBankInfo info = bank->SubBanks[i];
        info.EntryCount = (uint32_t)allSubBankEntries[i].size();
        info.Offset = entryTableOffsets[i];

        WriteBankString(out, info.Name);
        out.write((char*)&info.Version, 4); out.write((char*)&info.EntryCount, 4);
        out.write((char*)&info.Offset, 4); out.write((char*)&info.Size, 4); out.write((char*)&info.Align, 4);
    }

    out.seekp(0);
    out.write((char*)&header, sizeof(header));
    out.close();

    bank->Stream->close();
    std::filesystem::remove(bank->FullPath);
    std::filesystem::rename(tempPath, bank->FullPath);

    g_BankStatus = "Bank Recompiled. Please reload.";
}

inline void SaveEntryChanges(LoadedBank* bank) {
    if (!bank || bank->SelectedEntryIndex == -1) return;
    BankEntry& e = bank->Entries[bank->SelectedEntryIndex];
    std::vector<uint8_t> newBytes;

    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
        newBytes = g_ActiveAnim.Serialize();
    }
    else if (bank->Type == EBankType::Text) {
        newBytes = g_TextParser.Recompile();
        if (e.Type == 0 && !g_TextParser.TextData.Identifier.empty()) {
            e.Name = g_TextParser.TextData.Identifier;
        }
    }
    else {
        return;
    }

    bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
    bank->CurrentEntryRawData = newBytes;
    e.Size = (uint32_t)newBytes.size();

    UpdateFilter(*bank);
    g_BankStatus = "Saved to Memory (Size: " + std::to_string(newBytes.size()) + ")";
}