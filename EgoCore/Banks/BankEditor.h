#pragma once
#include "BankLoader.h"
#include "GltfExporter.h"
#include "TextCompiler.h"
#include "LipSyncCompiler.h"
#include "TextBackend.h" 
#include <thread>

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

// --- NAMING HELPER (Folder Name -> Prefix) ---
inline std::string GetPrefixForSubBank(const std::string& folderName) {
    std::string upper = folderName;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "LIPSYNC_ENGLISH_MAIN") return "Dialogue";
    if (upper == "LIPSYNC_ENGLISH_MAIN_2") return "Dialogue2";
    if (upper == "LIPSYNC_ENGLISH_SCRIPT") return "ScriptDialogue";
    if (upper == "LIPSYNC_ENGLISH_SCRIPT_2") return "ScriptDialogue2";
    if (upper == "LIPSYNC_ENGLISH_GUILD") return "GuildDialogue";
    if (upper == "LIPSYNC_ENGLISH_CREATURE") return "CreatureDialogue";
    return folderName;
}

// --- MAPPING HELPER (SpeechBank -> Folder Name) ---
// Used to find the correct sub-bank in dialogue.big without opening it in the UI
inline std::string GetSubBankNameForSpeechBank(const std::string& speechBank) {
    std::string s = speechBank;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    size_t dot = s.find('.');
    if (dot != std::string::npos) s = s.substr(0, dot);

    if (s == "dialogue") return "LIPSYNC_ENGLISH_MAIN";
    if (s == "dialogue2") return "LIPSYNC_ENGLISH_MAIN_2";
    if (s == "scriptdialogue") return "LIPSYNC_ENGLISH_SCRIPT";
    if (s == "scriptdialogue2") return "LIPSYNC_ENGLISH_SCRIPT_2";
    if (s == "guilddialogue") return "LIPSYNC_ENGLISH_GUILD";
    if (s == "creaturedialogue") return "LIPSYNC_ENGLISH_CREATURE";
    return ""; // Fallback or unknown
}

inline void ResetAudioPlayers() {
    for (auto& [key, parser] : g_BackgroundAudioBanks) {
        if (parser) parser->Player.Reset();
    }
    for (auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Audio && bank.AudioParser) {
            bank.AudioParser->Player.Reset();
        }
    }
}

inline bool LoadDialogueBankInBackground() {
    // 1. If we already have the structure, we are good.
    if (!g_LipSyncState.SubBanks.empty()) return true;

    std::string path = g_AppConfig.GameRootPath + "\\Data\\Lang\\English\\dialogue.big";

    // 2. CHECK OPEN TABS (Sync State from Tab)
    for (const auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Dialogue && bank.FullPath == path) {
            g_LipSyncState.SubBanks = bank.SubBanks; // COPY STRUCTURE
            g_LipSyncState.SubBankMap.clear();
            for (int i = 0; i < (int)bank.SubBanks.size(); i++) {
                g_LipSyncState.SubBankMap[bank.SubBanks[i].Name] = i;
            }
            g_LipSyncState.FilePath = path;
            return true;
        }
    }

    // 3. LOAD FROM DISK (If not in Tab)
    if (!fs::exists(path)) return false;

    auto tempBank = CreateBankFromDisk(path);
    if (!tempBank) return false;

    g_LipSyncState.SubBanks = tempBank->SubBanks;
    g_LipSyncState.SubBankMap.clear();
    for (int i = 0; i < (int)tempBank->SubBanks.size(); i++) {
        g_LipSyncState.SubBankMap[tempBank->SubBanks[i].Name] = i;
    }
    g_LipSyncState.FilePath = path;

    // Close the file handle so the Compiler can write to it later
    if (tempBank->Stream && tempBank->Stream->is_open()) {
        tempBank->Stream->close();
    }

    return true;
}

inline void CreateNewDialogueEntry(LoadedBank* bank) {
    if (!bank) return;
    uint32_t newID = GetNextFreeID(bank);

    std::string prefix = "Dialogue";
    if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
        prefix = GetPrefixForSubBank(bank->SubBanks[bank->ActiveSubBankIndex].Name);
    }
    std::string name = prefix + "_" + std::to_string(newID);

    auto blob = CLipSyncParser::GenerateEmpty(1.0f);
    BankEntry newEntry; newEntry.ID = newID; newEntry.Name = name; newEntry.FriendlyName = name; newEntry.Type = 1;
    newEntry.Offset = 0; newEntry.Size = (uint32_t)blob.Raw.size(); newEntry.InfoSize = (uint32_t)blob.Info.size();

    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;
    bank->ModifiedEntryData[newIndex] = blob.Raw; bank->SubheaderCache[newIndex] = blob.Info;

    if (bank->Type == EBankType::Dialogue && EnsureLipSyncLoaded()) {
        int sbIdx = 0;
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
            std::string subName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
            if (g_LipSyncState.SubBankMap.count(subName)) sbIdx = g_LipSyncState.SubBankMap[subName];
        }
        AddedEntryData ae; ae.Type = 1; ae.NamePrefix = prefix; ae.Raw = blob.Raw; ae.Info = blob.Info;
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

    std::string prefix = "Dialogue";
    if (bank->Type == EBankType::Dialogue) {
        if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
            prefix = GetPrefixForSubBank(bank->SubBanks[bank->ActiveSubBankIndex].Name);
        }
        newEntry.Name = prefix + "_" + std::to_string(newEntry.ID);
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
            std::string subName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
            if (g_LipSyncState.SubBankMap.count(subName)) sbIdx = g_LipSyncState.SubBankMap[subName];
        }
        AddedEntryData ae; ae.Type = newEntry.Type; ae.NamePrefix = prefix;
        ae.Raw = sourceData; ae.Info = infoData;

        ae.Dependencies = newEntry.Dependencies;

        g_LipSyncState.PendingAdds[sbIdx][newEntry.ID] = ae;
    }

    UpdateFilter(*bank); SelectEntry(bank, newIndex); g_BankStatus = "Duplicated Entry ID " + std::to_string(newEntry.ID);
}

// --- DELETE ENTRY ---
inline void DeleteBankEntry(LoadedBank* bank, int index) {
    if (!bank || index < 0 || index >= (int)bank->Entries.size()) return;
    uint32_t targetID = bank->Entries[index].ID;

    if (bank->Type == EBankType::Audio && bank->AudioParser) {
        bank->AudioParser->DeleteEntry(index); bank->Entries.erase(bank->Entries.begin() + index);
        UpdateFilter(*bank); bank->SelectedEntryIndex = -1; return;
    }

    if (bank->Type == EBankType::Dialogue) {
        if (EnsureLipSyncLoaded()) {
            int sbIdx = 0;
            if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
                std::string currentName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
                if (g_LipSyncState.SubBankMap.count(currentName)) {
                    sbIdx = g_LipSyncState.SubBankMap[currentName];
                }
            }
            g_LipSyncState.PendingDeletes[sbIdx].insert(targetID);
            if (g_LipSyncState.PendingAdds.count(sbIdx)) {
                g_LipSyncState.PendingAdds[sbIdx].erase(targetID);
            }
        }
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

    std::string key = bank->FileName;
    size_t dot = key.find_last_of('.'); if (dot != std::string::npos) key = key.substr(0, dot) + ".lut";
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (g_BackgroundAudioBanks.count(key)) { g_BackgroundAudioBanks[key]->Player.Reset(); g_BackgroundAudioBanks.erase(key); }

    if (bank->AudioParser->SaveBank(bank->FullPath)) {
        g_BankStatus = "Audio Bank (.LUT) Recompiled.";
        ReloadBankInPlace(bank);
        g_ShowSuccessPopup = true; g_SuccessMessage = "Audio Bank Compiled Successfully!";
    }
    else {
        g_BankStatus = "Error: Failed to save audio bank.";
    }
}

inline void SaveBigBank(LoadedBank* bank) {
    if (bank->Type == EBankType::Text) {
        if (TextCompiler::CompileTextBank(bank)) {
            g_BankStatus = "Text Bank Recompiled.";
            ReloadBankInPlace(bank);

            // CHECK CASCADE
            bool mediaModified = false;
            if (!g_LipSyncState.PendingAdds.empty() || !g_LipSyncState.PendingDeletes.empty()) {
                mediaModified = true;
            }
            for (auto& [key, parser] : g_BackgroundAudioBanks) {
                // Check IsDirty OR ModifiedCache
                if (parser->IsLoaded && (parser->IsDirty || !parser->ModifiedCache.empty())) {
                    parser->SaveBank(parser->FileName);
                    parser->ModifiedCache.clear();
                    parser->IsDirty = false; // Reset flag
                }
            }

            if (mediaModified) {
                g_BankStatus = "Compiling Linked Media (Cascade)...";

                // A. Release Locks
                ResetAudioPlayers();
                std::vector<int> closedBankIndices;

                // Close background audio banks
                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (parser->Stream.is_open()) {
                        parser->Stream.close();
                    }
                }

                // Close open bank tabs
                for (int i = 0; i < (int)g_OpenBanks.size(); ++i) {
                    auto& b = g_OpenBanks[i];
                    if (b.Type == EBankType::Audio && b.AudioParser) {
                        if (b.AudioParser->Stream.is_open()) {
                            b.AudioParser->Stream.close();
                        }
                        closedBankIndices.push_back(i);
                    }
                    else if (b.Type == EBankType::Dialogue) {
                        if (b.Stream && b.Stream->is_open()) {
                            b.Stream->close();
                        }
                        closedBankIndices.push_back(i);
                    }
                }

                // Close global LipSync state stream (will reopen for compilation)
                if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) {
                    g_LipSyncState.Stream->close();
                }

                // B. Save Headers
                for (auto& en : g_DefWorkspace.AllEnums) {
                    if (en.FilePath.find("snds.h") != std::string::npos) {
                        std::ofstream out(en.FilePath, std::ios::binary);
                        if (out.is_open()) {
                            out << en.FullContent;
                            out.close();
                        }
                    }
                }

                // C. Save Audio Banks
                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (parser->IsLoaded && !parser->ModifiedCache.empty()) {
                        parser->SaveBank(parser->FileName);
                        parser->ModifiedCache.clear();
                    }
                }

                // D. Compile Sound Binaries
                std::string log;
                std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
                BinaryParser::CompileSoundBinaries(defsPath, log);

                // E. Compile LipSync (Independent of UI State)
                if (LoadDialogueBankInBackground()) {
                    // Find if dialogue.big is open in a tab
                    LoadedBank* dialogueTab = nullptr;
                    for (auto& b : g_OpenBanks) {
                        if (b.Type == EBankType::Dialogue && b.FullPath == g_LipSyncState.FilePath) {
                            dialogueTab = &b;
                            break;
                        }
                    }

                    // Ensure g_LipSyncState has its own stream ready
                    if (!g_LipSyncState.Stream) {
                        g_LipSyncState.Stream = std::make_unique<std::fstream>();
                    }

                    // Close and reopen for writing
                    if (g_LipSyncState.Stream->is_open()) {
                        g_LipSyncState.Stream->close();
                    }

                    g_LipSyncState.Stream->open(
                        g_LipSyncState.FilePath,
                        std::ios::binary | std::ios::in | std::ios::out
                    );

                    if (g_LipSyncState.Stream->is_open()) {
                        if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                            g_LipSyncState.PendingAdds.clear();
                            g_LipSyncState.PendingDeletes.clear();
                            g_LipSyncState.CachedSubBankIndex = -1;

                            // Close the compilation stream
                            g_LipSyncState.Stream->close();

                            // Reload the tab if it was open
                            if (dialogueTab) {
                                ReloadBankInPlace(dialogueTab);
                            }

                            g_BankStatus = "Dialogue Bank Compiled Successfully (Cascade)!";
                        }
                        else {
                            g_BankStatus = "Error: Failed to recompile dialogue.big.";
                            g_LipSyncState.Stream->close();

                            // Reload tab on failure too (to restore state)
                            if (dialogueTab) {
                                ReloadBankInPlace(dialogueTab);
                            }
                        }
                    }
                    else {
                        g_BankStatus = "Error: Could not open dialogue.big for writing!";
                    }
                }

                // F. Restore Audio Banks (Reparse from disk)
                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    parser->Parse(parser->FileName);
                }

                // Reload other closed banks (skip dialogue - already handled above)
                for (int idx : closedBankIndices) {
                    if (idx < g_OpenBanks.size()) {
                        auto& b = g_OpenBanks[idx];
                        // Skip dialogue.big - we already reloaded it after compilation
                        if (!(b.Type == EBankType::Dialogue && b.FullPath == g_LipSyncState.FilePath)) {
                            ReloadBankInPlace(&b);
                        }
                    }
                }

                g_BankStatus = "Chain Complete!";
            }

            g_ShowSuccessPopup = true;
            g_SuccessMessage = "Text Bank & Linked Media Compiled Successfully!";
        }
        else {
            g_BankStatus = "Text Bank Compilation Failed.";
        }
    }
    else if (bank->Type == EBankType::Dialogue) {
        // Close streams before compilation
        if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) {
            g_LipSyncState.Stream->close();
        }
        if (bank->Stream && bank->Stream->is_open()) {
            bank->Stream->close();
        }

        g_LipSyncState.FilePath = bank->FullPath;

        if (LoadDialogueBankInBackground()) {
            // Open stream for writing
            if (!g_LipSyncState.Stream) {
                g_LipSyncState.Stream = std::make_unique<std::fstream>();
            }

            g_LipSyncState.Stream->open(
                g_LipSyncState.FilePath,
                std::ios::binary | std::ios::in | std::ios::out
            );

            if (g_LipSyncState.Stream->is_open()) {
                if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                    g_BankStatus = "Dialogue Bank Recompiled Successfully.";
                    g_LipSyncState.PendingAdds.clear();
                    g_LipSyncState.PendingDeletes.clear();
                    g_LipSyncState.CachedSubBankIndex = -1;
                    g_LipSyncState.Stream->close();
                    ReloadBankInPlace(bank);
                    g_ShowSuccessPopup = true;
                    g_SuccessMessage = "Dialogue Bank Compiled Successfully!";
                }
                else {
                    g_BankStatus = "Error: Failed to recompile dialogue.big.";
                    g_LipSyncState.Stream->close();
                    ReloadBankInPlace(bank);
                }
            }
            else {
                g_BankStatus = "Error: Could not open dialogue.big for writing!";
                ReloadBankInPlace(bank);
            }
        }
    }
    else if (bank->Type == EBankType::Audio) {
        SaveLutBank(bank);
    }
}

// --- SAVE ENTRY CHANGES (Memory Only) ---
inline void SaveEntryChanges(LoadedBank* bank) {
    if (!bank || bank->SelectedEntryIndex == -1) return;
    BankEntry& e = bank->Entries[bank->SelectedEntryIndex];
    std::vector<uint8_t> newBytes;

    if (e.Type == TYPE_ANIMATION || e.Type == TYPE_LIPSYNC_ANIMATION) {
        newBytes = g_ActiveAnim.Serialize();
    }
    else if (bank->Type == EBankType::Text) {
        g_TextParser.TextData.SpeechBank = EnforceLugExtension(g_TextParser.TextData.SpeechBank);
        if (!g_OriginalIdentifier.empty() && g_TextParser.TextData.Identifier != g_OriginalIdentifier) {
            if (!g_TextParser.TextData.SpeechBank.empty()) UpdateHeaderDefinition(g_TextParser.TextData.SpeechBank, g_OriginalIdentifier, g_TextParser.TextData.Identifier);
            e.Name = g_TextParser.TextData.Identifier; g_OriginalIdentifier = g_TextParser.TextData.Identifier;
        }
        else if (e.Type == 0 && !g_TextParser.TextData.Identifier.empty()) e.Name = g_TextParser.TextData.Identifier;

        if (!g_TextParser.TextData.SpeechBank.empty()) {
            bool exists = false;
            for (const auto& d : e.Dependencies) if (d == g_TextParser.TextData.SpeechBank) exists = true;
            if (!exists) { e.Dependencies.clear(); e.Dependencies.push_back(g_TextParser.TextData.SpeechBank); }
        }
        if (!g_TextParser.TextData.SpeechBank.empty()) SaveAssociatedHeader(g_TextParser.TextData.SpeechBank);
        newBytes = g_TextParser.Recompile();
    }
    else if (bank->Type == EBankType::Dialogue) {
        if (g_LipSyncParser.IsParsed) {
            newBytes = g_LipSyncParser.Recompile();
            bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
            bank->CurrentEntryRawData = newBytes;
            e.Size = (uint32_t)newBytes.size();
            std::vector<uint8_t> newInfo(4); memcpy(newInfo.data(), &g_LipSyncParser.Data.Duration, 4);
            bank->SubheaderCache[bank->SelectedEntryIndex] = newInfo; e.InfoSize = 4;

            if (LoadDialogueBankInBackground()) {
                int sbIdx = 0;
                if (bank->ActiveSubBankIndex >= 0 && bank->ActiveSubBankIndex < bank->SubBanks.size()) {
                    std::string currentName = bank->SubBanks[bank->ActiveSubBankIndex].Name;
                    if (g_LipSyncState.SubBankMap.count(currentName)) sbIdx = g_LipSyncState.SubBankMap[currentName];
                }
                std::string prefix = GetPrefixForSubBank(bank->SubBanks[bank->ActiveSubBankIndex].Name);
                AddedEntryData ae; ae.Type = e.Type; ae.NamePrefix = prefix; ae.Raw = newBytes; ae.Info = newInfo;
                if (!e.Dependencies.empty()) ae.Dependencies = e.Dependencies; else ae.Dependencies.push_back("SPEAKER_FEMALE1");
                g_LipSyncState.PendingAdds[sbIdx][e.ID] = ae;
            }
            g_BankStatus = "LipSync Entry Saved to Memory (Pending Recompile)."; return;
        }
    }
    else return;

    bank->ModifiedEntryData[bank->SelectedEntryIndex] = newBytes;
    bank->CurrentEntryRawData = newBytes;
    e.Size = (uint32_t)newBytes.size();
    UpdateFilter(*bank);
    g_BankStatus = "Saved to Memory."; g_IsTextDirty = false;
}

// In BankEditor.h

inline void DeleteLinkedMedia(const std::string& speechBankName, const std::string& identifier) {
    // 1. Resolve the ID from the text identifier (e.g., "SND_HELLO" -> 20401)
    int32_t soundID = ResolveAudioID(speechBankName, identifier);
    if (soundID == -1) {
        g_BankStatus = "Error: Could not resolve Sound ID for deletion.";
        return;
    }

    // 2. Remove the definition from the Header file (Memory & Disk)
    RemoveHeaderDefinition(speechBankName, identifier);

    // 3. Get the Audio Bank (Prioritizing Open Tabs via our new logic)
    // We pass the raw speechBankName (e.g. "Creature.lug"); GetOrLoad handles the .lut conversion.
    auto audioParser = GetOrLoadAudioBank(speechBankName);

    if (audioParser) {
        int idx = -1;
        for (int i = 0; i < (int)audioParser->Entries.size(); i++) {
            if (audioParser->Entries[i].SoundID == (uint32_t)soundID) {
                idx = i;
                break;
            }
        }

        if (idx != -1) {
            // A. Mark for Deletion in the Parser (Memory Only - Sets IsDirty=true)
            audioParser->DeleteEntry(idx);

            // B. Sync the UI List if this bank is currently visible in a tab
            // This ensures the user sees the item disappear immediately.
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Audio && b.AudioParser == audioParser) {
                    if (idx < b.Entries.size()) {
                        b.Entries.erase(b.Entries.begin() + idx);
                        // If we deleted the currently selected item, deselect it
                        if (b.SelectedEntryIndex == idx) b.SelectedEntryIndex = -1;
                        UpdateFilter(b);
                    }
                    break;
                }
            }
        }
    }

    // 4. Handle LipSync (Dialogue.big) - Memory Only
    // We queue the deletion in the LipSyncState. It will be committed to disk
    // when the user triggers the "Cascade" compile (Save Text Bank).
    if (LoadDialogueBankInBackground()) {
        std::string subName = GetSubBankNameForSpeechBank(speechBankName);
        if (!subName.empty() && g_LipSyncState.SubBankMap.count(subName)) {
            int sbIdx = g_LipSyncState.SubBankMap[subName];

            // Add to Pending Deletes
            g_LipSyncState.PendingDeletes[sbIdx].insert((uint32_t)soundID);

            // If we previously added it in this session (but haven't compiled yet),
            // we can just remove it from the "Pending Adds" queue entirely.
            if (g_LipSyncState.PendingAdds.count(sbIdx)) {
                g_LipSyncState.PendingAdds[sbIdx].erase((uint32_t)soundID);
            }
        }
    }

    g_BankStatus = "Linked Media Marked for Deletion (Pending Recompile)...";
}