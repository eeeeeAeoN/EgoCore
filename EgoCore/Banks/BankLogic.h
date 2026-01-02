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
#include "TextCompiler.h"
#include "LipSyncCompiler.h" 
#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <set>

// --- GLOBAL STATE ---
inline std::vector<BinaryParser> g_LoadedBinaries;

// --- UTILS ---
inline void WriteBankString(std::ofstream& out, const std::string& s) {
    uint32_t len = (uint32_t)s.length();
    out.write((char*)&len, 4);
    if (len > 0) out.write(s.data(), len);
}

inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
    out.write(s.c_str(), s.length() + 1);
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
    if (!bank || index < 0 || index >= (int)bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->DeleteEntry(index);
        bank->Entries.erase(bank->Entries.begin() + index);
        UpdateFilter(*bank);
        bank->SelectedEntryIndex = -1;
        return;
    }

    if (bank->Type == EBankType::Text) {
        std::vector<uint8_t> rawData;
        if (bank->ModifiedEntryData.count(index)) {
            rawData = bank->ModifiedEntryData[index];
        }
        else {
            bank->Stream->clear();
            bank->Stream->seekg(bank->Entries[index].Offset, std::ios::beg);
            rawData.resize(bank->Entries[index].Size);
            bank->Stream->read((char*)rawData.data(), bank->Entries[index].Size);
        }

        CTextParser tempParser;
        tempParser.Parse(rawData, bank->Entries[index].Type);

        if (!tempParser.IsGroup && !tempParser.IsNarratorList) {
            DeleteLinkedMedia(tempParser.TextData.SpeechBank, tempParser.TextData.Identifier);
        }
    }

    bank->Entries.erase(bank->Entries.begin() + index);

    if (bank->ModifiedEntryData.count(index)) {
        bank->ModifiedEntryData.erase(index);
    }
    if (bank->SubheaderCache.count(index)) {
        bank->SubheaderCache.erase(index);
    }

    std::map<int, std::vector<uint8_t>> newCache;
    for (auto& [k, v] : bank->ModifiedEntryData) {
        if (k < index) newCache[k] = v;
        else if (k > index) newCache[k - 1] = v;
    }
    bank->ModifiedEntryData = newCache;

    bank->SelectedEntryIndex = -1;
    UpdateFilter(*bank);
}

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

// --- RELOAD HELPER ---
inline void ReloadBankInPlace(LoadedBank* bank) {
    if (!bank) return;
    std::string path = bank->FullPath;

    // Close current stream
    if (bank->Stream && bank->Stream->is_open()) bank->Stream->close();

    // Create new bank state from disk
    auto newBankPtr = CreateBankFromDisk(path);
    if (newBankPtr) {
        // Move content to existing pointer to keep UI references valid-ish
        *bank = std::move(*newBankPtr);
        g_BankStatus = "Bank Reloaded.";
    }
    else {
        g_BankStatus = "Error: Failed to reload bank after compilation!";
    }
}

// --- SAVE BIG BANK WRAPPER & CASCADE LOGIC ---
inline void SaveBigBank(LoadedBank* bank) {
    if (bank->Type == EBankType::Text) {
        // 1. Compile Text Bank (Primary)
        if (TextCompiler::CompileTextBank(bank)) {
            g_BankStatus = "Text Bank Recompiled.";

            // RELOAD TEXT BANK IMMEDIATELY TO FIX READ ERRORS
            ReloadBankInPlace(bank);

            // 2. CHECK FOR CASCADE REQUIREMENTS
            bool mediaModified = false;
            if (!g_LipSyncState.PendingAdds.empty() || !g_LipSyncState.PendingDeletes.empty()) mediaModified = true;

            for (auto& [key, parser] : g_BackgroundAudioBanks) {
                if (!parser->ModifiedCache.empty()) mediaModified = true;
            }

            // 3. EXECUTE CASCADE IF NEEDED
            if (mediaModified) {
                g_BankStatus = "Compiling Linked Media Chain...";

                // A. Save Headers to Disk
                for (auto& en : g_DefWorkspace.AllEnums) {
                    if (en.FilePath.find("snds.h") != std::string::npos) {
                        std::ofstream out(en.FilePath, std::ios::binary);
                        if (out.is_open()) { out << en.FullContent; out.close(); }
                    }
                }

                // B. Compile Binaries
                std::string log;
                std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
                BinaryParser::CompileSoundBinaries(defsPath, log);

                // C. Compile Audio Banks (.LUT)
                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (!parser->ModifiedCache.empty()) {
                        parser->SaveBank(parser->FileName);
                        parser->ModifiedCache.clear();
                    }
                }

                // D. Compile Dialogue Bank (LipSync)
                // 1. Check if dialogue.big is currently open in the Generic Viewer
                LoadedBank* openDialogueBank = nullptr;
                for (auto& b : g_OpenBanks) {
                    if (b.FileName == "dialogue.big") {
                        openDialogueBank = &b;
                        // IMPORTANT: Close stream to avoid sharing violation during overwrite
                        if (b.Stream && b.Stream->is_open()) b.Stream->close();
                        break;
                    }
                }

                // 2. Ensure the LipSync backend is loaded
                if (EnsureLipSyncLoaded()) {
                    // 3. Compile using the global state (which contains the pending adds)
                    if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                        g_BankStatus = "Chain Complete: Text, Binaries, Audio, Dialogue saved.";

                        // Clear pending changes
                        g_LipSyncState.PendingAdds.clear();
                        g_LipSyncState.PendingDeletes.clear();
                        g_LipSyncState.CachedSubBankIndex = -1;

                        // 4. If the Generic Viewer had dialogue.big open, reload it now
                        if (openDialogueBank) {
                            ReloadBankInPlace(openDialogueBank);
                        }
                    }
                    else {
                        g_BankStatus = "Error: Failed to compile dialogue.big (LipSyncCompiler)";
                        // Try to restore the viewer even if failed
                        if (openDialogueBank) ReloadBankInPlace(openDialogueBank);
                    }
                }
                else {
                    g_BankStatus = "Error: Could not load dialogue.big into LipSync State";
                    // Try to restore the viewer
                    if (openDialogueBank) ReloadBankInPlace(openDialogueBank);
                }
            }
        }
        else {
            g_BankStatus = "Text Bank Compilation Failed.";
        }
    }
    else {
        g_BankStatus = "Recompilation not supported for this bank type.";
    }
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