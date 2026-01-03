#pragma once
#include "BankLoader.h"
#include "GltfExporter.h"
#include "TextCompiler.h"
#include "LipSyncCompiler.h"

// --- UTILS ---
inline void WriteBankString(std::ofstream& out, const std::string& s) {
    uint32_t len = (uint32_t)s.length(); out.write((char*)&len, 4); if (len > 0) out.write(s.data(), len);
}
inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
    out.write(s.c_str(), s.length() + 1);
}

// --- ID HELPER ---
inline uint32_t GetNextFreeID(LoadedBank* bank) {
    uint32_t maxID = 0;
    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        for (const auto& e : bank->AudioParser->Entries) if (e.SoundID > maxID) maxID = e.SoundID;
    }
    else {
        for (const auto& e : bank->Entries) if (e.ID > maxID) maxID = e.ID;
    }
    return (maxID > 0) ? maxID + 1 : 20000;
}

// --- CREATION FUNCTIONS ---
inline void CreateNewDialogueEntry(LoadedBank* bank) {
    if (!bank) return;
    uint32_t newID = GetNextFreeID(bank);

    std::string stem = std::filesystem::path(bank->FileName).stem().string();
    if (!stem.empty()) stem[0] = toupper(stem[0]);
    std::string name = stem + "_" + std::to_string(newID);

    auto blob = CLipSyncParser::GenerateEmpty(1.0f);
    BankEntry newEntry; newEntry.ID = newID; newEntry.Name = name; newEntry.FriendlyName = name; newEntry.Type = 1;
    newEntry.Offset = 0; newEntry.Size = (uint32_t)blob.Raw.size(); newEntry.InfoSize = (uint32_t)blob.Info.size();

    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;
    bank->ModifiedEntryData[newIndex] = blob.Raw; bank->SubheaderCache[newIndex] = blob.Info;

    // SYNC WITH COMPILER STATE
    if (bank->Type == EBankType::Dialogue && EnsureLipSyncLoaded()) {
        int sbIdx = 0;
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
            std::string currentName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
            if (g_LipSyncState.SubBankMap.count(currentName)) {
                sbIdx = g_LipSyncState.SubBankMap[currentName];
            }
        }
        AddedEntryData ae; ae.Type = 1; ae.NamePrefix = stem; ae.Raw = blob.Raw; ae.Info = blob.Info;
        g_LipSyncState.PendingAdds[sbIdx][newID] = ae;
    }

    bank->FilterText[0] = '\0'; UpdateFilter(*bank); SelectEntry(bank, newIndex);
    g_BankStatus = "Added Dialogue Entry: " + name;
}

inline void CreateNewTextEntry(LoadedBank* bank, int type) {
    if (!bank) return;
    BankEntry newEntry; newEntry.ID = GetNextFreeID(bank); newEntry.Type = type; newEntry.Offset = 0; newEntry.Size = 0;
    newEntry.Name = "New_Entry_" + std::to_string(newEntry.ID); newEntry.FriendlyName = newEntry.Name;
    CTextParser tempParser;
    if (type == 0) {
        tempParser.IsGroup = false; tempParser.TextData.Content = L"New Text Content";
        tempParser.TextData.Identifier = "NEW_ID_" + std::to_string(newEntry.ID); tempParser.TextData.Speaker = "NoSpeaker"; tempParser.TextData.SpeechBank = "";
    }
    else tempParser.IsGroup = true;
    std::vector<uint8_t> data = tempParser.Recompile(); newEntry.Size = (uint32_t)data.size();
    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;
    bank->ModifiedEntryData[newIndex] = data;
    bank->FilterText[0] = '\0'; UpdateFilter(*bank); SelectEntry(bank, newIndex);
    g_BankStatus = "Added Text Entry ID " + std::to_string(newEntry.ID);
}

inline void DuplicateBankEntry(LoadedBank* bank, int sourceIndex) {
    if (!bank || sourceIndex < 0 || sourceIndex >= bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        if (bank->AudioParser->CloneEntry(sourceIndex)) {
            std::string headerName = GetHeaderName(bank->FileName);
            std::map<uint32_t, std::string> friendlyNames = BuildFriendlyNameMap(headerName);
            bank->Entries.clear(); bank->FilteredIndices.clear();
            for (int i = 0; i < bank->AudioParser->Entries.size(); i++) {
                const auto& ae = bank->AudioParser->Entries[i];
                BankEntry be; be.ID = ae.SoundID; be.Name = "Sound ID " + std::to_string(ae.SoundID);
                if (friendlyNames.count(be.ID)) be.FriendlyName = friendlyNames[be.ID]; else be.FriendlyName = be.Name;
                be.Size = ae.Length; be.Offset = ae.Offset; be.Type = 999;
                bank->Entries.push_back(be); bank->FilteredIndices.push_back(i);
            }
            UpdateFilter(*bank); g_BankStatus = "Duplicated Audio Entry";
        }
        return;
    }

    BankEntry newEntry = bank->Entries[sourceIndex];
    newEntry.ID = GetNextFreeID(bank);

    // FIX: Naming Convention
    if (bank->Type == EBankType::Dialogue) {
        std::string stem = std::filesystem::path(bank->FileName).stem().string();
        if (!stem.empty()) stem[0] = toupper(stem[0]);
        newEntry.Name = stem + "_" + std::to_string(newEntry.ID);
        newEntry.FriendlyName = newEntry.Name;
    }
    else {
        newEntry.Name += "_Copy"; newEntry.FriendlyName += "_Copy";
    }

    newEntry.Offset = 0;
    std::vector<uint8_t> sourceData;
    if (bank->ModifiedEntryData.count(sourceIndex)) sourceData = bank->ModifiedEntryData[sourceIndex];
    else {
        bank->Stream->clear(); bank->Stream->seekg(bank->Entries[sourceIndex].Offset, std::ios::beg);
        sourceData.resize(bank->Entries[sourceIndex].Size); bank->Stream->read((char*)sourceData.data(), bank->Entries[sourceIndex].Size);
    }

    if (bank->Type == EBankType::Text && newEntry.Type == 0) {
        CTextParser p; p.Parse(sourceData, newEntry.Type);
        if (p.IsParsed) { p.TextData.Identifier += "_COPY"; newEntry.Name = p.TextData.Identifier; newEntry.FriendlyName = p.TextData.Identifier; sourceData = p.Recompile(); }
    }

    newEntry.Size = (uint32_t)sourceData.size();
    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;
    bank->ModifiedEntryData[newIndex] = sourceData;

    std::vector<uint8_t> infoData;
    if (bank->SubheaderCache.count(sourceIndex)) {
        infoData = bank->SubheaderCache[sourceIndex];
        bank->SubheaderCache[newIndex] = infoData;
    }

    if (bank->Type == EBankType::Dialogue && EnsureLipSyncLoaded()) {
        int sbIdx = 0;
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
            std::string currentName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
            if (g_LipSyncState.SubBankMap.count(currentName)) sbIdx = g_LipSyncState.SubBankMap[currentName];
        }
        AddedEntryData ae; ae.Type = newEntry.Type;
        std::string stem = std::filesystem::path(bank->FileName).stem().string();
        if (!stem.empty()) stem[0] = toupper(stem[0]);
        ae.NamePrefix = stem;
        ae.Raw = sourceData; ae.Info = infoData;
        g_LipSyncState.PendingAdds[sbIdx][newEntry.ID] = ae;
    }

    UpdateFilter(*bank); SelectEntry(bank, newIndex); g_BankStatus = "Duplicated Entry ID " + std::to_string(newEntry.ID);
}

inline void DeleteBankEntry(LoadedBank* bank, int index) {
    if (!bank || index < 0 || index >= (int)bank->Entries.size()) return;
    uint32_t targetID = bank->Entries[index].ID;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->DeleteEntry(index); bank->Entries.erase(bank->Entries.begin() + index);
        UpdateFilter(*bank); bank->SelectedEntryIndex = -1; return;
    }

    if (bank->Type == EBankType::Dialogue) {
        DeleteLipSyncEntry(bank->FileName, targetID);
    }

    bank->Entries.erase(bank->Entries.begin() + index);
    if (bank->ModifiedEntryData.count(index)) bank->ModifiedEntryData.erase(index);
    if (bank->SubheaderCache.count(index)) bank->SubheaderCache.erase(index);

    std::map<int, std::vector<uint8_t>> newCache; for (auto& [k, v] : bank->ModifiedEntryData) if (k < index) newCache[k] = v; else if (k > index) newCache[k - 1] = v;
    bank->ModifiedEntryData = newCache;
    std::map<int, std::vector<uint8_t>> newMeta; for (auto& [k, v] : bank->SubheaderCache) if (k < index) newMeta[k] = v; else if (k > index) newMeta[k - 1] = v;
    bank->SubheaderCache = newMeta;

    bank->SelectedEntryIndex = -1; UpdateFilter(*bank);
}

// --- SAVE LUT BANK (Sync) ---
inline void SaveLutBank(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio || !bank->AudioParser) return;

    // Release handles
    std::string key = bank->FileName;
    size_t dot = key.find_last_of('.'); if (dot != std::string::npos) key = key.substr(0, dot) + ".lut";
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (g_BackgroundAudioBanks.count(key)) { g_BackgroundAudioBanks[key]->Player.Reset(); g_BackgroundAudioBanks.erase(key); }

    if (bank->AudioParser->SaveBank(bank->FullPath)) {
        g_BankStatus = "Audio Bank (.LUT) Recompiled.";
        ReloadBankInPlace(bank);
        g_ShowSuccessPopup = true;
        g_SuccessMessage = "Audio Bank Compiled Successfully!";
    }
    else {
        g_BankStatus = "Error: Failed to save audio bank.";
    }
}

// --- SAVE BIG BANK (Sync) ---
inline void SaveBigBank(LoadedBank* bank) {
    // 1. TEXT BANK
    if (bank->Type == EBankType::Text) {
        if (TextCompiler::CompileTextBank(bank)) {
            g_BankStatus = "Text Bank Recompiled.";
            ReloadBankInPlace(bank);

            bool mediaModified = false;
            if (!g_LipSyncState.PendingAdds.empty() || !g_LipSyncState.PendingDeletes.empty()) mediaModified = true;
            for (auto& [key, parser] : g_BackgroundAudioBanks) if (!parser->ModifiedCache.empty()) mediaModified = true;

            if (mediaModified) {
                g_BankStatus = "Compiling Linked Media...";
                // Headers
                for (auto& en : g_DefWorkspace.AllEnums) {
                    if (en.FilePath.find("snds.h") != std::string::npos) { std::ofstream out(en.FilePath, std::ios::binary); if (out.is_open()) { out << en.FullContent; out.close(); } }
                }
                // Binaries
                std::string log; std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
                BinaryParser::CompileSoundBinaries(defsPath, log);
                // Audio
                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (!parser->ModifiedCache.empty()) { parser->SaveBank(parser->FileName); parser->ModifiedCache.clear(); }
                }

                // Dialogue Cascade
                if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) g_LipSyncState.Stream->close();
                LoadedBank* openDialogue = nullptr;
                for (auto& b : g_OpenBanks) if (b.FileName == "dialogue.big") { openDialogue = &b; if (b.Stream && b.Stream->is_open()) b.Stream->close(); break; }

                if (EnsureLipSyncLoaded()) {
                    if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                        g_BankStatus = "Chain Complete!";
                        g_LipSyncState.PendingAdds.clear(); g_LipSyncState.PendingDeletes.clear(); g_LipSyncState.CachedSubBankIndex = -1;
                        if (openDialogue) ReloadBankInPlace(openDialogue);
                    }
                }
            }
            g_ShowSuccessPopup = true;
            g_SuccessMessage = "Text Bank (and chain) Compiled Successfully!";
        }
        else {
            g_BankStatus = "Text Bank Compilation Failed.";
        }
    }
    // 2. DIALOGUE BANK
    else if (bank->Type == EBankType::Dialogue) {
        if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) g_LipSyncState.Stream->close();
        if (bank->Stream && bank->Stream->is_open()) bank->Stream->close();

        if (EnsureLipSyncLoaded()) {
            if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                g_BankStatus = "Dialogue Bank Recompiled Successfully.";
                g_LipSyncState.PendingAdds.clear(); g_LipSyncState.PendingDeletes.clear(); g_LipSyncState.CachedSubBankIndex = -1;
                ReloadBankInPlace(bank);
                g_ShowSuccessPopup = true;
                g_SuccessMessage = "Dialogue Bank Compiled Successfully!";
            }
            else {
                g_BankStatus = "Error: Failed to recompile dialogue.big.";
                ReloadBankInPlace(bank);
            }
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
        if (e.Type == 0 && !g_TextParser.TextData.Identifier.empty()) e.Name = g_TextParser.TextData.Identifier;
    }
    else if (bank->Type == EBankType::Dialogue) {
        if (g_LipSyncParser.IsParsed) {
            newBytes = g_LipSyncParser.Recompile();
            bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
            bank->CurrentEntryRawData = newBytes;
            e.Size = (uint32_t)newBytes.size();
            std::vector<uint8_t> newInfo(4); memcpy(newInfo.data(), &g_LipSyncParser.Data.Duration, 4);
            bank->SubheaderCache[bank->SelectedEntryIndex] = newInfo; e.InfoSize = 4;

            if (EnsureLipSyncLoaded()) {
                int sbIdx = 0;
                if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
                    std::string currentName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
                    if (g_LipSyncState.SubBankMap.count(currentName)) sbIdx = g_LipSyncState.SubBankMap[currentName];
                }

                AddedEntryData ae; ae.Type = e.Type;
                std::string prefix = "Dialogue"; size_t u = e.Name.find('_'); if (u != std::string::npos) prefix = e.Name.substr(0, u);
                ae.NamePrefix = prefix; ae.Raw = newBytes; ae.Info = newInfo;
                g_LipSyncState.PendingAdds[sbIdx][e.ID] = ae;
            }
            g_BankStatus = "LipSync Entry Saved to Memory (Pending Recompile).";
            return;
        }
    }
    else return;

    bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
    bank->CurrentEntryRawData = newBytes;
    e.Size = (uint32_t)newBytes.size();
    UpdateFilter(*bank);
    g_BankStatus = "Saved to Memory (Size: " + std::to_string(newBytes.size()) + ")";
}