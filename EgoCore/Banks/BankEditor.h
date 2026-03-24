#pragma once
#include "BankLoader.h"
#include "TextureBuilder.h"
#include "BigBankCompiler.h"
#include "GltfExporter.h"
#include "TextCompiler.h"
#include "LipSyncCompiler.h"
#include "TextBackend.h" 
#include <thread>
#include "MeshCompiler.h"

extern float g_ImportBumpFactor;

inline std::string SanitizeEnumName(std::string name) {
    std::string res;
    for (char c : name) {
        if (std::isalnum((unsigned char)c) || c == '_') res += c;
        else if (c == ' ' || c == '.' || c == '-') res += '_'; // Replaces .tga etc with underscore
    }
    if (!res.empty() && std::isdigit((unsigned char)res[0])) res = "_" + res;
    return res;
}

inline void SyncBankEnums(LoadedBank* bank) {
    if (!bank || bank->SubBanks.empty()) return;

    auto RebuildEnum = [&](const std::string& enumName, const std::vector<BankEntry>& entries, std::function<bool(const BankEntry&)> filter) {
        std::string body;
        for (const auto& e : entries) {
            if (filter(e)) {
                body += "\t" + SanitizeEnumName(e.Name) + " = " + std::to_string(e.ID) + ",\n";
            }
        }
        ReplaceAndSaveEnum(enumName, body);
        };

    if (bank->Type == EBankType::Textures) {
        for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
            if (bank->SubBanks[i].Name == "GBANK_MAIN_PC" && bank->ActiveSubBankIndex == i) {
                RebuildEnum("EEngineGraphic", bank->Entries, [](const BankEntry& e) {
                    return !e.Name.empty() && e.Name[0] != '[';
                    });
            }
            else if (bank->SubBanks[i].Name == "GBANK_GUI_PC" && bank->ActiveSubBankIndex == i) {
                RebuildEnum("EGuiGraphicBank", bank->Entries, [](const BankEntry& e) { return true; });
            }
        }
    }
    else if (bank->Type == EBankType::Frontend) {
        if (bank->ActiveSubBankIndex >= 0) {
            RebuildEnum("EFrontEndGraphicBank", bank->Entries, [](const BankEntry& e) { return true; });
        }
    }
    else if (bank->Type == EBankType::Graphics) {
        for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
            if (bank->SubBanks[i].Name == "MBANK_ALLMESHES" && bank->ActiveSubBankIndex == i) {
                RebuildEnum("EAnimType2", bank->Entries, [](const BankEntry& e) {
                    return e.Type == 6 || e.Type == 7 || e.Type == 9;
                    });
                RebuildEnum("EMeshType2_1", bank->Entries, [](const BankEntry& e) {
                    return e.Type != 3 && e.Type != 6 && e.Type != 7 && e.Type != 9;
                    });
            }
            else if (bank->SubBanks[i].Name == "MBANK_ENGINE" && bank->ActiveSubBankIndex == i) {
                RebuildEnum("EEngineMeshType", bank->Entries, [](const BankEntry& e) { return true; });
            }
        }
    }
    else if (bank->Type == EBankType::Text) {
        for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
            if (bank->ActiveSubBankIndex == i) {
                std::string sbName = bank->SubBanks[i].Name;
                std::transform(sbName.begin(), sbName.end(), sbName.begin(), ::toupper);

                // Supports TEXT_ENGLISH_MAIN, TEXT_FRENCH_MAIN, etc.
                if (sbName.find("TEXT_") == 0 && sbName.find("_MAIN") != std::string::npos) {
                    RebuildEnum("EGameText", bank->Entries, [](const BankEntry& e) {
                        return e.Type != 2; // Exclude Type 2 (Narrator List)
                        });
                }
            }
        }
    }
    else if (bank->Type == EBankType::Dialogue) {
        for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
            if (bank->ActiveSubBankIndex == i) {
                std::string sbName = bank->SubBanks[i].Name;
                std::transform(sbName.begin(), sbName.end(), sbName.begin(), ::toupper);

                std::string targetEnum = "";
                if (sbName.find("LIPSYNC_") == 0) {
                    if (sbName.find("_MAIN_2") != std::string::npos) targetEnum = "ELipSync3";
                    else if (sbName.find("_MAIN") != std::string::npos) targetEnum = "ELipSync";
                    else if (sbName.find("_SCRIPT_2") != std::string::npos) targetEnum = "ELipSync4";
                    else if (sbName.find("_SCRIPT") != std::string::npos) targetEnum = "ELipSync2";
                }

                if (!targetEnum.empty()) {
                    RebuildEnum(targetEnum, bank->Entries, [](const BankEntry& e) { return true; });
                }
            }
        }
    }
}

inline void WriteBankString(std::ofstream& out, const std::string& s) {
    uint32_t len = (uint32_t)s.length(); out.write((char*)&len, 4); if (len > 0) out.write(s.data(), len);
}
inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
    out.write(s.c_str(), s.length() + 1);
}

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

inline std::string GetPrefixForSubBank(const std::string& folderName) {
    std::string upper = folderName;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper.find("LIPSYNC_") == 0) {
        if (upper.find("_MAIN_2") != std::string::npos) return "Dialogue2";
        if (upper.find("_MAIN") != std::string::npos) return "Dialogue";
        if (upper.find("_SCRIPT_2") != std::string::npos) return "ScriptDialogue2";
        if (upper.find("_SCRIPT") != std::string::npos) return "ScriptDialogue";
        if (upper.find("_GUILD") != std::string::npos) return "GuildDialogue";
        if (upper.find("_CREATURE") != std::string::npos) return "CreatureDialogue";
    }
    return folderName;
}

inline std::string GetSubBankNameForSpeechBank(const std::string& speechBank) {
    std::string s = speechBank;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    size_t dot = s.find('.');
    if (dot != std::string::npos) s = s.substr(0, dot);

    std::string targetSuffix = "";
    if (s == "dialogue") targetSuffix = "_MAIN";
    else if (s == "dialogue2") targetSuffix = "_MAIN_2";
    else if (s == "scriptdialogue") targetSuffix = "_SCRIPT";
    else if (s == "scriptdialogue2") targetSuffix = "_SCRIPT_2";
    else if (s == "guilddialogue") targetSuffix = "_GUILD";
    else if (s == "creaturedialogue") targetSuffix = "_CREATURE";

    // Dynamically match the loaded bank's language (e.g. LIPSYNC_SPANISH_MAIN)
    if (!targetSuffix.empty() && !g_LipSyncState.SubBanks.empty()) {
        for (const auto& sb : g_LipSyncState.SubBanks) {
            if (sb.Name.find(targetSuffix) != std::string::npos) return sb.Name;
        }
    }
    return "LIPSYNC_ENGLISH" + targetSuffix; // Safe fallback
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
    if (!g_LipSyncState.SubBanks.empty()) return true;

    std::string path = "";
    const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };

    for (const char* l : langs) {
        std::string p = g_AppConfig.GameRootPath + "\\Data\\Lang\\" + std::string(l) + "\\dialogue.big";
        if (fs::exists(p)) { path = p; break; }
    }

    if (path.empty()) return false;

    for (const auto& bank : g_OpenBanks) {
        if (bank.Type == EBankType::Dialogue && bank.FullPath == path) {
            g_LipSyncState.SubBanks = bank.SubBanks;
            g_LipSyncState.SubBankMap.clear();
            for (int i = 0; i < (int)bank.SubBanks.size(); i++) {
                g_LipSyncState.SubBankMap[bank.SubBanks[i].Name] = i;
            }
            g_LipSyncState.FilePath = path;
            return true;
        }
    }

    auto tempBank = CreateBankFromDisk(path);
    if (!tempBank) return false;

    g_LipSyncState.SubBanks = tempBank->SubBanks;
    g_LipSyncState.SubBankMap.clear();
    for (int i = 0; i < (int)tempBank->SubBanks.size(); i++) {
        g_LipSyncState.SubBankMap[tempBank->SubBanks[i].Name] = i;
    }
    g_LipSyncState.FilePath = path;

    if (tempBank->Stream && tempBank->Stream->is_open()) tempBank->Stream->close();
    return true;
}

inline void SaveAudioBank(LoadedBank* bank) {
    if (!bank || bank->Type != EBankType::Audio) return;

    if (bank->LugParserPtr) {
        auto& lug = bank->LugParserPtr;
        if (lug->Save(lug->FileName)) {
            if (MetParser::GenerateMetFile(lug->FileName, *lug)) {
                g_SuccessMessage = "Bank (.LUG) and Metadata (.MET) recompiled successfully!";
            }
            else {
                g_SuccessMessage = "Bank (.LUG) saved, but .MET generation failed!";
            }
            g_BankStatus = "Script Bank and Metadata Recompiled.";
            g_ShowSuccessPopup = true;
            ReloadBankInPlace(bank);
        }
        else {
            g_BankStatus = "Error: Failed to save .LUG bank.";
        }
    }
    else if (bank->AudioParser) {
        std::string key = bank->FileName;
        size_t dot = key.find_last_of('.');
        if (dot != std::string::npos) key = key.substr(0, dot) + ".lut";
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (g_BackgroundAudioBanks.count(key)) {
            g_BackgroundAudioBanks[key]->Player.Reset();
            g_BackgroundAudioBanks.erase(key);
        }

        if (bank->AudioParser->SaveBank(bank->FullPath)) {
            g_BankStatus = "Audio Bank (.LUT) Recompiled.";
            ReloadBankInPlace(bank);
            g_ShowSuccessPopup = true;
            g_SuccessMessage = "Audio Bank Compiled Successfully!";
        }
        else {
            g_BankStatus = "Error: Failed to save .LUT bank.";
        }
    }
}

bool CreateNewTextureEntry(LoadedBank* bank, const std::string& filePath, ETextureFormat format, int type, bool generateBump = false) {
    if (!bank) return false;

    TextureBuilder::ImportOptions opts;
    opts.Format = format;
    opts.GenerateMipmaps = true;
    opts.ResizeToPowerOfTwo = true;

    // Hook: Use the user's UI choice for generation
    opts.IsBumpmap = generateBump;
    opts.BumpFactor = g_ImportBumpFactor;

    auto result = TextureBuilder::ImportImage(filePath, opts);
    if (!result.Success) {
        g_BankStatus = "Import Failed: " + result.Error;
        return false;
    }

    BankEntry e;
    e.ID = GetNextFreeID(bank);
    e.Name = std::filesystem::path(filePath).stem().string();
    e.FriendlyName = e.Name;
    e.Type = type;
    e.Size = (uint32_t)result.FullData.size();
    e.InfoSize = (uint32_t)result.HeaderInfo.size(); // [FIX] Properly flag compiler that it requires metadata
    e.Offset = 0;
    e.Timestamp = 0;

    // Store compiled data and metadata
    bank->Entries.push_back(e);
    int newIdx = (int)bank->Entries.size() - 1;
    bank->ModifiedEntryData[newIdx] = result.FullData;
    bank->SubheaderCache[newIdx] = result.HeaderInfo;

    bank->SelectedEntryIndex = newIdx;
    UpdateFilter(*bank);

    // FIX: Force the parser to instantly read the newly generated data!
    SelectEntry(bank, newIdx);

    return true;
}

inline void AddTextureFrame(LoadedBank* bank, int entryIdx, const std::string& filePath) {
    if (!bank->StagedEntries.count(entryIdx)) SaveEntryChanges(bank); // Stage if not staged
    auto& stagedTex = bank->StagedEntries[entryIdx].Texture;

    int x, y, c; stbi_uc* pixels = stbi_load(filePath.c_str(), &x, &y, &c, 4);
    if (!pixels) { g_BankStatus = "Error loading frame."; return; }

    int physW = stagedTex->Header.Width ? stagedTex->Header.Width : stagedTex->Header.FrameWidth;
    int physH = stagedTex->Header.Height ? stagedTex->Header.Height : stagedTex->Header.FrameHeight;

    std::vector<uint8_t> rgba(physW * physH * 4);
    stbir_resize_uint8(pixels, x, y, 0, rgba.data(), physW, physH, 0, 4); // Force conform to sequence size
    stbi_image_free(pixels);

    // Apply generation dynamically if adding to a normal map sequence
    bool isBump = (bank->Entries[entryIdx].Type == 2 || bank->Entries[entryIdx].Type == 3);
    if (isBump) {
        TextureBuilder::ConvertRGBAToFableNormalMap(rgba, physW, physH, g_ImportBumpFactor);
    }

    stagedTex->RawFrames.push_back(rgba);
    stagedTex->Header.FrameCount = (uint16_t)stagedTex->RawFrames.size();

    SelectEntry(bank, entryIdx); g_BankStatus = "Frame Added (Staged).";
}

inline void DeleteTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx) {
    if (!bank->StagedEntries.count(entryIdx)) SaveEntryChanges(bank);
    auto& stagedTex = bank->StagedEntries[entryIdx].Texture;

    if (stagedTex->RawFrames.size() > 1 && frameIdx < stagedTex->RawFrames.size()) {
        stagedTex->RawFrames.erase(stagedTex->RawFrames.begin() + frameIdx);
        stagedTex->Header.FrameCount = (uint16_t)stagedTex->RawFrames.size();
        SelectEntry(bank, entryIdx); g_BankStatus = "Frame Deleted (Staged).";
    }
    else {
        g_BankStatus = "Cannot delete last frame.";
    }
}

inline void ReplaceTextureFrame(LoadedBank* bank, int entryIdx, int frameIdx, const std::string& filePath) {
    if (!bank->StagedEntries.count(entryIdx)) SaveEntryChanges(bank);
    auto& stagedTex = bank->StagedEntries[entryIdx].Texture;

    int x, y, c; stbi_uc* pixels = stbi_load(filePath.c_str(), &x, &y, &c, 4);
    if (!pixels) { g_BankStatus = "Error loading frame."; return; }

    int physW = stagedTex->Header.Width ? stagedTex->Header.Width : stagedTex->Header.FrameWidth;
    int physH = stagedTex->Header.Height ? stagedTex->Header.Height : stagedTex->Header.FrameHeight;

    std::vector<uint8_t> rgba(physW * physH * 4);
    stbir_resize_uint8(pixels, x, y, 0, rgba.data(), physW, physH, 0, 4);
    stbi_image_free(pixels);

    // Apply generation dynamically if replacing frame on a normal map!
    bool isBump = (bank->Entries[entryIdx].Type == 2 || bank->Entries[entryIdx].Type == 3);
    if (isBump) {
        TextureBuilder::ConvertRGBAToFableNormalMap(rgba, physW, physH, g_ImportBumpFactor);
    }

    if (frameIdx < stagedTex->RawFrames.size()) {
        stagedTex->RawFrames[frameIdx] = rgba;
        SelectEntry(bank, entryIdx); g_BankStatus = "Frame Replaced (Staged).";
    }
}

inline void CreateNewAnimationEntry(LoadedBank* bank, const std::string& gltfPath, int animType) {
    if (!bank) return;

    C3DAnimationInfo newAnim;
    newAnim.Duration = 1.0f;
    newAnim.NonLoopingDuration = 1.0f;
    newAnim.IsCyclic = true;
    newAnim.HasHelper = false;
    newAnim.Rotation = 0.0f;
    newAnim.MovementVector = { 0.0f, 0.0f, 0.0f };
    newAnim.ObjectName = std::filesystem::path(gltfPath).stem().string();

    int outType = animType;
    std::string err = GltfAnimImporter::Import(gltfPath, g_ActiveMeshContent, newAnim, outType);
    if (!err.empty()) {
        g_ShowSuccessPopup = true;
        g_SuccessMessage = "Import Error: " + err;
        return;
    }

    if (!newAnim.BoneMaskBits.empty()) outType = 9;
    if (outType == 7) {
        newAnim.MovementVector = { 0.0f, 0.0f, 0.0f };
        newAnim.HelperTracks.erase(std::remove_if(newAnim.HelperTracks.begin(), newAnim.HelperTracks.end(),
            [](const AnimTrack& t) { return t.BoneName == ""; }), newAnim.HelperTracks.end());
    }

    uint32_t newID = GetNextFreeID(bank);
    BankEntry entry;
    entry.ID = newID;
    std::string fname = newAnim.ObjectName;
    std::transform(fname.begin(), fname.end(), fname.begin(), ::toupper);
    entry.Name = fname;
    entry.FriendlyName = fname;
    entry.Type = outType;
    entry.Size = 0;      // Will be calculated during Flush
    entry.InfoSize = 0;  // Will be calculated during Flush
    entry.Offset = 0;
    entry.Timestamp = 0;

    bank->Entries.push_back(entry);
    int newIndex = (int)bank->Entries.size() - 1;

    // --- DEFERRED STAGING ---
    StagedEntry staged;
    staged.Anim = std::make_shared<C3DAnimationInfo>(newAnim);
    bank->StagedEntries[newIndex] = staged;

    bank->FilterText[0] = '\0';
    UpdateFilter(*bank);
    SelectEntry(bank, newIndex);

    g_ShowSuccessPopup = true;
    g_SuccessMessage = "Animation Entry Created: " + fname + "\n(Staged for Compilation)";
}

inline bool CreateNewMeshEntry(LoadedBank* bank, const std::string& gltfPath, int type, int reps, bool forceRecalculate = false) {
    if (!bank) return false;

    std::string fname = std::filesystem::path(gltfPath).stem().string();
    std::transform(fname.begin(), fname.end(), fname.begin(), ::toupper);

    uint32_t newID = GetNextFreeID(bank);
    BankEntry be;
    be.ID = newID;
    be.Type = type;
    be.Name = fname;
    be.FriendlyName = fname;
    be.Size = 0;
    be.InfoSize = 0;
    be.Offset = 0;
    be.Timestamp = 0;

    // --- HANDLE PHYSICS MESH (TYPE 3) ---
    if (type == 3) {
        CBBMParser newBBM;
        std::string err = GltfMeshImporter::ImportType3(gltfPath, fname, newBBM);
        if (!err.empty()) {
            g_ShowSuccessPopup = true;
            g_SuccessMessage = "Import Error: " + err;
            return false;
        }
        bank->Entries.push_back(be);
        int newIndex = (int)bank->Entries.size() - 1;

        StagedEntry staged;
        staged.Physics = std::make_shared<CBBMParser>(newBBM); // DEFERRED TO RAM
        bank->StagedEntries[newIndex] = staged;

        bank->FilterText[0] = '\0';
        UpdateFilter(*bank);
        SelectEntry(bank, newIndex);
        return true;
    }

    // --- HANDLE GRAPHICS MESHES (TYPES 1, 2, 4, 5) ---
    C3DMeshContent newMesh;
    std::string err;

    // PASS THE BOOLEAN TO TYPE 1 AND TYPE 5
    if (type == 1) err = GltfMeshImporter::ImportType1(gltfPath, fname, newMesh);
    else if (type == 2) err = GltfMeshImporter::ImportType2(gltfPath, fname, newMesh, reps);
    else if (type == 4) err = GltfMeshImporter::ImportType4(gltfPath, fname, newMesh);
    else if (type == 5) err = GltfMeshImporter::ImportType5(gltfPath, fname, newMesh, forceRecalculate);

    if (!err.empty()) {
        g_ShowSuccessPopup = true;
        g_SuccessMessage = "Import Error: " + err;
        return false;
    }

    newMesh.EntryMeta.PhysicsIndex = 0;
    newMesh.EntryMeta.LODCount = 1;
    newMesh.EntryMeta.LODErrors.clear();

    bank->Entries.push_back(be);
    int newIndex = (int)bank->Entries.size() - 1;

    StagedEntry staged;
    staged.MeshLODs.push_back(std::make_shared<C3DMeshContent>(newMesh));
    staged.MeshMeta = newMesh.EntryMeta;

    bank->StagedEntries[newIndex] = staged;
    bank->FilterText[0] = '\0';
    UpdateFilter(*bank);
    SelectEntry(bank, newIndex);

    return true;
}

inline void RenameTextureEntry(LoadedBank* bank, int entryIdx, const std::string& newName) {
    if (entryIdx < 0) return;
    bank->Entries[entryIdx].Name = newName;
    bank->Entries[entryIdx].FriendlyName = newName;
    UpdateFilter(*bank);
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

    StagedEntry staged;

    if (type == 0) {
        CTextEntry t; t.Content = L"New Text Content"; t.Identifier = "NEW_ID_" + std::to_string(newEntry.ID);
        staged.Text = std::make_shared<CTextEntry>(t);
    }
    else {
        CTextGroup g;
        staged.TextGroup = std::make_shared<CTextGroup>(g);
    }

    bank->Entries.push_back(newEntry);
    int newIndex = (int)bank->Entries.size() - 1;
    bank->StagedEntries[newIndex] = staged;

    bank->FilterText[0] = '\0'; UpdateFilter(*bank); SelectEntry(bank, newIndex);
    g_BankStatus = "Added and Staged Text Entry ID " + std::to_string(newEntry.ID);
}

inline void DuplicateBankEntry(LoadedBank* bank, int sourceIndex) {
    if (!bank || sourceIndex < 0 || sourceIndex >= bank->Entries.size()) return;

    if (bank->Type == EBankType::Audio) {
        if (bank->LugParserPtr) {
            bank->LugParserPtr->CloneEntry(sourceIndex);
            bank->Entries.clear(); bank->FilteredIndices.clear();
            for (size_t k = 0; k < bank->LugParserPtr->Entries.size(); k++) {
                BankEntry be;
                be.ID = bank->LugParserPtr->Entries[k].SoundID;
                be.Name = bank->LugParserPtr->Entries[k].Name;
                be.FriendlyName = be.Name;
                be.Size = bank->LugParserPtr->Entries[k].Length;
                be.Offset = bank->LugParserPtr->Entries[k].Offset;
                bank->Entries.push_back(be);
                bank->FilteredIndices.push_back((int)k);
            }
            UpdateFilter(*bank);
            g_BankStatus = "Duplicated Script Entry";
            return;
        }
        else if (bank->AudioParser) {
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

inline void DeleteBankEntry(LoadedBank* bank, int index) {
    if (!bank || index < 0 || index >= (int)bank->Entries.size()) return;
    uint32_t targetID = bank->Entries[index].ID;

    if (bank->Type == EBankType::Audio) {
        if (bank->LugParserPtr) {
            bank->LugParserPtr->DeleteEntry(index);
            bank->Entries.erase(bank->Entries.begin() + index);
            UpdateFilter(*bank);
            bank->SelectedEntryIndex = -1;
            return;
        }
        else if (bank->AudioParser) {
            bank->AudioParser->DeleteEntry(index);
            bank->Entries.erase(bank->Entries.begin() + index);
            UpdateFilter(*bank);
            bank->SelectedEntryIndex = -1;
            return;
        }
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

inline void FlushStagedEntries(LoadedBank* bank) {
    if (!bank || bank->StagedEntries.empty()) return;

    for (auto& [idx, staged] : bank->StagedEntries) {
        BankEntry& e = bank->Entries[idx];
        std::vector<uint8_t> newBytes;
        std::vector<uint8_t> newInfo;

        if (staged.Anim) {
            newInfo = staged.Anim->Serialize();
            newBytes = AnimCompiler::Compile(*staged.Anim);
            e.InfoSize = (uint32_t)newInfo.size();
        }
        else if (!staged.MeshLODs.empty()) {
            newBytes.clear();
            staged.MeshMeta.LODSizes.clear();

            // Compile every LOD in the array sequentially
            for (size_t i = 0; i < staged.MeshLODs.size(); i++) {
                auto& lod = staged.MeshLODs[i];
                lod->AutoCalculateBounds();
                std::vector<uint8_t> lodBytes = MeshCompiler::CompileSingleLOD(*lod);

                // FIX: Apply the Ghost Buffer padding trick to BOTH Type 2 (Foliage) and Type 5 (Animated) meshes!
                if ((e.Type == 2 || e.Type == 5) && i == staged.MeshLODs.size() - 1) {
                    // Create an exact structural copy of LOD 0 (keeps bounds, skeleton, helpers)
                    C3DMeshContent ghostLOD = *lod;

                    // Strip out the renderable geometry Fable doesn't need in a Ghost LOD
                    ghostLOD.Materials.clear();
                    ghostLOD.MaterialCount = 0;
                    ghostLOD.Primitives.clear();
                    ghostLOD.PrimitiveCount = 0;
                    ghostLOD.TotalStaticBlocks = 0;
                    ghostLOD.TotalAnimatedBlocks = 0;

                    std::vector<uint8_t> ghostBytes = MeshCompiler::CompileSingleLOD(ghostLOD);

                    staged.MeshMeta.LODSizes.push_back((uint32_t)lodBytes.size());
                    newBytes.insert(newBytes.end(), lodBytes.begin(), lodBytes.end());
                    newBytes.insert(newBytes.end(), ghostBytes.begin(), ghostBytes.end());
                }
                else {
                    staged.MeshMeta.LODSizes.push_back((uint32_t)lodBytes.size());
                    newBytes.insert(newBytes.end(), lodBytes.begin(), lodBytes.end());
                }
            }

            staged.MeshMeta.LODCount = (uint32_t)staged.MeshLODs.size();

            // Update Base Metadata from LOD 0
            memcpy(staged.MeshMeta.BoundingSphereCenter, staged.MeshLODs[0]->BoundingSphereCenter, 12);
            staged.MeshMeta.BoundingSphereRadius = staged.MeshLODs[0]->BoundingSphereRadius;
            memcpy(staged.MeshMeta.BoundingBoxMin, staged.MeshLODs[0]->BoundingBoxMin, 12);
            memcpy(staged.MeshMeta.BoundingBoxMax, staged.MeshLODs[0]->BoundingBoxMax, 12);

            // Collect Unique Texture IDs
            staged.MeshMeta.TextureIDs.clear();
            for (const auto& mat : staged.MeshLODs[0]->Materials) {
                auto addTex = [&](int id) { if (id > 0 && std::find(staged.MeshMeta.TextureIDs.begin(), staged.MeshMeta.TextureIDs.end(), id) == staged.MeshMeta.TextureIDs.end()) staged.MeshMeta.TextureIDs.push_back(id); };
                addTex(mat.DiffuseMapID); addTex(mat.BumpMapID); addTex(mat.ReflectionMapID); addTex(mat.IlluminationMapID); addTex(mat.DecalID);
            }

            staged.MeshMeta.HasData = true;

            // Build the TOC
            auto serializeMeta = [&](const CMeshEntryMetadata& m) {
                std::vector<uint8_t> d;
                auto W = [&](const void* v, size_t l) { size_t s = d.size(); d.resize(s + l); memcpy(d.data() + s, v, l); };
                W(&m.PhysicsIndex, 4); W(m.BoundingSphereCenter, 12); W(&m.BoundingSphereRadius, 4); W(m.BoundingBoxMin, 12); W(m.BoundingBoxMax, 12);
                W(&m.LODCount, 4); for (uint32_t s : m.LODSizes) W(&s, 4);
                W(&m.SafeBoundingRadius, 4); for (float err : m.LODErrors) W(&err, 4);
                uint32_t tC = (uint32_t)m.TextureIDs.size(); W(&tC, 4); for (int32_t id : m.TextureIDs) W(&id, 4);
                return d;
                };
            newInfo = serializeMeta(staged.MeshMeta);
            e.InfoSize = (uint32_t)newInfo.size();
        }
        else if (staged.Physics) {
            newBytes = MeshCompiler::CompilePhysics(*staged.Physics);

            // CRITICAL TOC FIX: Physics meshes DO NOT use TOC Info blocks! 
            // The InfoSize MUST be strictly 0, or the engine will misalign the next file.
            e.InfoSize = 0;
            newInfo.clear();

            e.Type = TYPE_STATIC_PHYSICS_MESH;
        }
        else if (staged.Texture) {
            TextureBuilder::ImportOptions opts;
            opts.Format = staged.Texture->TargetFormat;
            opts.GenerateMipmaps = true;

            int w = staged.Texture->Header.Width ? staged.Texture->Header.Width : staged.Texture->Header.FrameWidth;
            int h = staged.Texture->Header.Height ? staged.Texture->Header.Height : staged.Texture->Header.FrameHeight;

            newBytes.clear();
            newInfo.clear();

            // Loop array and run DXT compression on every frame sequentially
            for (size_t i = 0; i < staged.Texture->RawFrames.size(); i++) {
                auto result = TextureBuilder::CompileFromRGBA(staged.Texture->RawFrames[i], w, h, opts);
                if (i == 0) {
                    staged.Texture->Header.MipmapLevels = ((CGraphicHeader*)result.HeaderInfo.data())->MipmapLevels;
                    staged.Texture->Header.FrameDataSize = ((CGraphicHeader*)result.HeaderInfo.data())->FrameDataSize;
                    newInfo = result.HeaderInfo; // [FIX] Grab 34 byte header
                }
                newBytes.insert(newBytes.end(), result.FullData.begin(), result.FullData.end());
            }

            staged.Texture->Header.FrameCount = (uint16_t)staged.Texture->RawFrames.size();

            // Fable structural rule for sequences
            if (staged.Texture->Header.FrameCount > 1) {
                staged.Texture->Header.MipSize0 = 0;
                if (e.Type == 0) e.Type = 1;
                else if (e.Type == 2) e.Type = 3;
            }

            // [FIX] Update only the structural section of our extracted 34 byte array. 
            // Avoids wiping out Fable pixel formats.
            if (newInfo.size() >= sizeof(CGraphicHeader)) {
                memcpy(newInfo.data(), &staged.Texture->Header, sizeof(CGraphicHeader));
            }
            e.InfoSize = (uint32_t)newInfo.size();
        }
        else if (staged.Text || staged.TextGroup || staged.NarratorList) {
            CTextParser tempParser;
            if (staged.Text) { tempParser.TextData = *staged.Text; tempParser.IsGroup = false; tempParser.IsNarratorList = false; }
            else if (staged.TextGroup) { tempParser.GroupData = *staged.TextGroup; tempParser.IsGroup = true; tempParser.IsNarratorList = false; }
            else if (staged.NarratorList) { tempParser.NarratorStrings = *staged.NarratorList; tempParser.IsGroup = false; tempParser.IsNarratorList = true; }
            newBytes = tempParser.Recompile();
            e.InfoSize = 0;
        }
        else if (staged.LipSync) {
            CLipSyncParser tempParser;
            tempParser.Data = *staged.LipSync;
            newBytes = tempParser.Recompile();
            float duration = tempParser.Data.Duration;
            newInfo.resize(4); memcpy(newInfo.data(), &duration, 4);
            e.InfoSize = 4;
        }

        // Commit to the final binary blob cache
        if (!newBytes.empty()) {
            bank->ModifiedEntryData[idx] = newBytes;
            e.Size = (uint32_t)newBytes.size();
        }
        if (!newInfo.empty()) {
            bank->SubheaderCache[idx] = newInfo;
        }
    }

    // Clear staged items, they are now permanently baked into the ModifedEntryData blobs!
    bank->StagedEntries.clear();
}

inline void SaveBigBank(LoadedBank* bank) {

    FlushStagedEntries(bank);

    if (bank->Type == EBankType::Text) {
        if (TextCompiler::CompileTextBank(bank)) {
            SyncBankEnums(bank);
            g_BankStatus = "Text Bank Recompiled.";
            ReloadBankInPlace(bank);

            bool mediaModified = false;
            if (!g_LipSyncState.PendingAdds.empty() || !g_LipSyncState.PendingDeletes.empty()) {
                mediaModified = true;
            }
            for (auto& [key, parser] : g_BackgroundAudioBanks) {
                if (parser->IsLoaded && (parser->IsDirty || !parser->ModifiedCache.empty())) {
                    parser->SaveBank(parser->FileName);
                    parser->ModifiedCache.clear();
                    parser->IsDirty = false;
                }
            }

            if (mediaModified) {
                g_BankStatus = "Compiling Linked Media (Cascade)...";

                ResetAudioPlayers();
                std::vector<int> closedBankIndices;

                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (parser->Stream.is_open()) {
                        parser->Stream.close();
                    }
                }

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

                if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) {
                    g_LipSyncState.Stream->close();
                }

                for (auto& en : g_DefWorkspace.AllEnums) {
                    if (en.FilePath.find("snds.h") != std::string::npos) {
                        std::ofstream out(en.FilePath, std::ios::binary);
                        if (out.is_open()) {
                            out << en.FullContent;
                            out.close();
                        }
                    }
                }

                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    if (parser->IsLoaded && !parser->ModifiedCache.empty()) {
                        parser->SaveBank(parser->FileName);
                        parser->ModifiedCache.clear();
                    }
                }

                std::string log;
                std::string defsPath = g_AppConfig.GameRootPath + "\\Data\\Defs";
                BinaryParser::CompileSoundBinaries(defsPath, log);

                if (LoadDialogueBankInBackground()) {
                    LoadedBank* dialogueTab = nullptr;
                    for (auto& b : g_OpenBanks) {
                        if (b.Type == EBankType::Dialogue && b.FullPath == g_LipSyncState.FilePath) {
                            dialogueTab = &b;
                            break;
                        }
                    }

                    if (!g_LipSyncState.Stream) {
                        g_LipSyncState.Stream = std::make_unique<std::fstream>();
                    }

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

                            g_LipSyncState.Stream->close();

                            if (dialogueTab) {
                                SyncBankEnums(dialogueTab);
                                ReloadBankInPlace(dialogueTab);
                            }
                            g_BankStatus = "Dialogue Bank Compiled Successfully (Cascade)!";
                        }
                        else {
                            g_BankStatus = "Error: Failed to recompile dialogue.big.";
                            g_LipSyncState.Stream->close();
                            if (dialogueTab) ReloadBankInPlace(dialogueTab);
                        }
                    }
                    else {
                        g_BankStatus = "Error: Could not open dialogue.big for writing!";
                    }
                }

                for (auto& [key, parser] : g_BackgroundAudioBanks) {
                    parser->Parse(parser->FileName);
                }

                for (int idx : closedBankIndices) {
                    if (idx < g_OpenBanks.size()) {
                        auto& b = g_OpenBanks[idx];
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
        if (g_LipSyncState.Stream && g_LipSyncState.Stream->is_open()) g_LipSyncState.Stream->close();
        if (bank->Stream && bank->Stream->is_open()) bank->Stream->close();

        g_LipSyncState.FilePath = bank->FullPath;

        if (LoadDialogueBankInBackground()) {
            if (!g_LipSyncState.Stream) {
                g_LipSyncState.Stream = std::make_unique<std::fstream>();
            }

            g_LipSyncState.Stream->open(
                g_LipSyncState.FilePath,
                std::ios::binary | std::ios::in | std::ios::out
            );

            if (g_LipSyncState.Stream->is_open()) {
                if (LipSyncCompiler::CompileLipSyncFromState(g_LipSyncState)) {
                    SyncBankEnums(bank);
                    g_BankStatus = "Dialogue Bank Recompiled Successfully.";
                    g_LipSyncState.PendingAdds.clear();
                    g_LipSyncState.PendingDeletes.clear();
                    g_LipSyncState.CachedSubBankIndex = -1;
                    g_LipSyncState.Stream->close();
                    ReloadBankInPlace(bank);
                    g_ShowSuccessPopup = true;
                    g_SuccessMessage = "Dialogue Bank Compiled Successfully!";
                }
            }
            else {
                g_BankStatus = "Error: Could not open dialogue.big for writing!";
                ReloadBankInPlace(bank);
            }
        }
    }
    else if (bank->Type == EBankType::Audio) {
        SaveAudioBank(bank);
    }
    else if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend || bank->Type == EBankType::Graphics || bank->Type == EBankType::Effects || bank->Type == EBankType::Fonts || bank->Type == EBankType::Shaders) {
        if (BigBankCompiler::Compile(bank)) {
            SyncBankEnums(bank);
            g_BankStatus = "Bank Recompiled Successfully.";
            ReloadBankInPlace(bank);
            g_ShowSuccessPopup = true;
            g_SuccessMessage = "Bank & Headers Compiled Successfully!";
        }
        else {
            g_BankStatus = "Error: Failed to compile bank.";
        }
    }
}

inline void SaveEntryChanges(LoadedBank* bank) {
    if (!bank || bank->SelectedEntryIndex == -1) return;
    BankEntry& e = bank->Entries[bank->SelectedEntryIndex];

    StagedEntry staged;

    // --- ANIMATIONS (Types 6, 7, 9) ---
    if (bank->Type == EBankType::Graphics && (e.Type == 6 || e.Type == 7 || e.Type == 9)) {
        if (g_AnimParser.Data.IsParsed) {
            staged.Anim = std::make_shared<C3DAnimationInfo>(g_AnimParser.Data);
            g_BankStatus = "Animation staged for compilation.";
        }
    }
    // --- PHYSICS MESH (Type 3) ---
    else if (bank->Type == EBankType::Graphics && e.Type == 3) {
        if (g_BBMParser.IsParsed) {
            staged.Physics = std::make_shared<CBBMParser>(g_BBMParser);
            g_BankStatus = "Physics Mesh staged for compilation.";
        }
    }
    // --- MESHES (Types 1, 2, 4, 5) ---
    else if (bank->Type == EBankType::Graphics && IsSupportedMesh(e.Type)) {
        if (g_ActiveMeshContent.IsParsed) {
            // If it's already staged, just grab it to update the active LOD
            if (bank->StagedEntries.count(bank->SelectedEntryIndex)) {
                staged = bank->StagedEntries[bank->SelectedEntryIndex];
            }
            else {
                // First time staging! We must extract ALL LODs from binary into C++ objects.
                staged.MeshMeta = g_ActiveMeshContent.EntryMeta;
                size_t offset = 0;
                for (uint32_t i = 0; i < staged.MeshMeta.LODCount; i++) {
                    size_t sz = staged.MeshMeta.LODSizes[i];
                    std::vector<uint8_t> slice(sz);
                    if (offset + sz <= bank->CurrentEntryRawData.size()) {
                        memcpy(slice.data(), bank->CurrentEntryRawData.data() + offset, sz);
                    }
                    auto lodMesh = std::make_shared<C3DMeshContent>();
                    lodMesh->ParseEntryMetadata(bank->SubheaderCache[bank->SelectedEntryIndex]);
                    lodMesh->Parse(slice, e.Type);
                    staged.MeshLODs.push_back(lodMesh);
                    offset += sz;
                }
            }

            // Overwrite the specific LOD we are currently editing
            if (bank->SelectedLOD < staged.MeshLODs.size()) {
                staged.MeshLODs[bank->SelectedLOD] = std::make_shared<C3DMeshContent>(g_ActiveMeshContent);
            }
            g_BankStatus = "Mesh LOD staged for compilation.";
        }
    }
    // --- TEXTURES ---
    else if (bank->Type == EBankType::Textures || bank->Type == EBankType::Frontend || bank->Type == EBankType::Effects) {
        if (!g_TextureParser.PendingName.empty() && g_TextureParser.PendingName != e.Name) {
            RenameTextureEntry(bank, bank->SelectedEntryIndex, g_TextureParser.PendingName);
        }
        if (g_TextureParser.IsParsed) {
            if (bank->StagedEntries.count(bank->SelectedEntryIndex)) {
                staged = bank->StagedEntries[bank->SelectedEntryIndex];
            }
            else {
                // First time staging an existing binary texture: Extract ALL DXT frames to Raw RGBA!
                auto texInfo = std::make_shared<StagedTextureInfo>();
                texInfo->Header = g_TextureParser.Header;
                texInfo->TargetFormat = g_TextureParser.DecodedFormat;

                uint32_t w = texInfo->Header.Width ? texInfo->Header.Width : texInfo->Header.FrameWidth;
                uint32_t h = texInfo->Header.Height ? texInfo->Header.Height : texInfo->Header.FrameHeight;
                uint32_t frames = (texInfo->Header.FrameCount > 0) ? texInfo->Header.FrameCount : 1;

                for (uint32_t f = 0; f < frames; f++) {
                    const uint8_t* src = g_TextureParser.DecodedPixels.data() + (f * g_TextureParser.TrueFrameStride);
                    texInfo->RawFrames.push_back(TextureUtils::DecompressFrameToRGBA(src, w, h, texInfo->TargetFormat));
                }
                staged.Texture = texInfo;
            }
            g_BankStatus = "Texture staged for compilation.";
        }
    }
    // --- TEXT ---
    else if (bank->Type == EBankType::Text) {
        if (g_TextParser.IsGroup) {
            staged.TextGroup = std::make_shared<CTextGroup>(g_TextParser.GroupData);
        }
        else if (g_TextParser.IsNarratorList) {
            staged.NarratorList = std::make_shared<std::vector<std::string>>(g_TextParser.NarratorStrings);
        }
        else {
            g_TextParser.TextData.SpeechBank = EnforceLugExtension(g_TextParser.TextData.SpeechBank);
            if (!g_TextParser.TextData.SpeechBank.empty()) SaveAssociatedHeader(g_TextParser.TextData.SpeechBank);
            staged.Text = std::make_shared<CTextEntry>(g_TextParser.TextData);
        }
        g_IsTextDirty = false;
        g_BankStatus = "Text Entry staged for compilation.";
    }
    // --- DIALOGUE ---
    else if (bank->Type == EBankType::Dialogue && g_LipSyncParser.IsParsed) {
        staged.LipSync = std::make_shared<CLipSyncData>(g_LipSyncParser.Data);
        g_BankStatus = "LipSync Entry staged for compilation.";
    }
    // --- AUDIO ---
    else if (bank->Type == EBankType::Audio && bank->LugParserPtr) {
        bank->LugParserPtr->IsDirty = true;
        g_BankStatus = "Audio Metadata staged to RAM.";
        return; // Audio uses its own bespoke backend, so no need to put in Staged map
    }

    // Insert into map if valid
    if (!staged.MeshLODs.empty() || staged.Anim || staged.Physics || staged.Texture || staged.Text || staged.TextGroup || staged.NarratorList || staged.LipSync) {
        bank->StagedEntries[bank->SelectedEntryIndex] = staged;
    }

    UpdateFilter(*bank);
}

inline void DeleteLinkedMedia(const std::string& speechBankName, const std::string& identifier) {
    int32_t soundID = ResolveAudioID(speechBankName, identifier);
    if (soundID == -1) {
        g_BankStatus = "Error: Could not resolve Sound ID for deletion.";
        return;
    }

    RemoveHeaderDefinition(speechBankName, identifier);

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
            audioParser->DeleteEntry(idx);
            for (auto& b : g_OpenBanks) {
                if (b.Type == EBankType::Audio && b.AudioParser == audioParser) {
                    if (idx < b.Entries.size()) {
                        b.Entries.erase(b.Entries.begin() + idx);
                        if (b.SelectedEntryIndex == idx) b.SelectedEntryIndex = -1;
                        UpdateFilter(b);
                    }
                    break;
                }
            }
        }
    }

    if (LoadDialogueBankInBackground()) {
        std::string subName = GetSubBankNameForSpeechBank(speechBankName);
        if (!subName.empty() && g_LipSyncState.SubBankMap.count(subName)) {
            int sbIdx = g_LipSyncState.SubBankMap[subName];
            g_LipSyncState.PendingDeletes[sbIdx].insert((uint32_t)soundID);
            if (g_LipSyncState.PendingAdds.count(sbIdx)) {
                g_LipSyncState.PendingAdds[sbIdx].erase((uint32_t)soundID);
            }
        }
    }

    g_BankStatus = "Linked Media Marked for Deletion (Pending Recompile)...";
}