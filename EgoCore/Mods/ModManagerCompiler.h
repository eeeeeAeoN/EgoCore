#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <regex>
#include "ConfigBackend.h"
#include "BankBackend.h"
#include "BigBankCompiler.h"
#include "DefBackend.h"
#include "BinaryParser.h"

namespace fs = std::filesystem;

enum class EModAssetCategory {
    BankEntry,
    Definition,
    Header,
    AnimationEvent
};

struct IniLine {
    std::string Raw;
    std::string Key;
    std::string Value;
    bool IsKeyValue = false;
};

struct ModEntry {
    std::string Name;
    std::string Description;
    bool IsEnabled = false;
    bool HasDll = false;
    bool IsCoreMod = false;
    bool IsAssetMod = false;
    bool IsDefMod = false;
    bool IsTngMod = false;
    std::string ModFolderPath;
    std::string SettingsIniPath;
    std::vector<IniLine> SettingsLines;
};

struct ModAssetOverride {
    std::string ResourcePath;
    std::string HeaderPath;
    bool IsHandled = false;
};

struct StagedModPackageEntry {
    EModAssetCategory Category = EModAssetCategory::BankEntry;
    uint32_t EntryID = 0;
    std::string EntryName;
    int32_t EntryType = 0;
    EBankType BankType = (EBankType)0;
    std::string TypeName;
    std::string BankName;
    std::string SubBankName;
    std::string SourceFullPath; 
};

inline std::string GetEntryTypeName(EBankType bankType, int32_t type, const std::string& bankName) {
    if (bankType == EBankType::Graphics || bankType == EBankType::XboxGraphics) {
        switch (type) {
        case 1: return "Static Mesh"; case 2: return "Repeated Mesh";
        case 3: return "Physics (BBM)"; case 4: return "Particle Mesh";
        case 5: return "Animated Mesh"; case 6: return "Animation";
        case 7: return "Delta Animation"; case 8: return "Lipsync Animation";
        case 9: return "Partial Animation"; case 10: return "Relative Animation";
        default: return "Gfx " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Textures || bankType == EBankType::Frontend) {
        switch (type) {
        case 0: return "Graphic Single"; case 1: return "Graphic Sequence";
        case 2: return "Bumpmap"; case 3: return "Bumpmap Sequence";
        case 4: return "Volume Texture"; case 5: return "Sprite Sheet";
        default: return "Tex " + std::to_string(type);
        }
    }
    if (bankType == EBankType::Shaders) {
        switch (type) { case 0: return "Vertex Shader"; case 1: return "Pixel Shader"; default: return "Shader " + std::to_string(type); }
    }
    if (bankType == EBankType::Effects) {
        switch (type) { case 0: return "Particle Entry"; default: return "Effect " + std::to_string(type); }
    }
    if (bankType == EBankType::Dialogue) {
        switch (type) { case 1: return "Lipsync Entry"; default: return "Dialogue " + std::to_string(type); }
    }
    if (bankType == EBankType::Fonts) {
        switch (type) { case 0: return "PC Font"; case 1: return "Xbox Font"; case 2: return "GlyphData"; default: return "Font " + std::to_string(type); }
    }
    if (bankType == EBankType::Text) {
        switch (type) { case 0: return "Text Entry"; case 1: return "Group Text Entry"; case 2: return "Narrator List"; default: return "Text " + std::to_string(type); }
    }
    if (bankType == EBankType::Audio) {
        std::string lowerName = bankName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(".lut") != std::string::npos) return "Audio Clip (.lut)";
        if (lowerName.find(".lug") != std::string::npos) return "Scripted Audio (.lug)";
        return "Audio Entry";
    }
    return "Type " + std::to_string(type);
}

class ModPackageTracker {
public:
    inline static std::vector<StagedModPackageEntry> g_MarkedEntries;

    static void LoadMarkedState() {
        g_MarkedEntries.clear();
        for (const auto& raw : g_SavedMarkedEntries) {
            StagedModPackageEntry e;
            e.EntryID = raw.EntryID;
            e.EntryName = raw.EntryName;
            e.EntryType = raw.EntryType;
            e.BankType = (EBankType)raw.BankType;
            e.TypeName = raw.TypeName;
            e.BankName = raw.BankName;
            e.SubBankName = raw.SubBankName;
            e.SourceFullPath = raw.SourceFullPath;
            g_MarkedEntries.push_back(e);
        }
    }

    static void SaveMarkedState() {
        g_SavedMarkedEntries.clear();
        for (const auto& e : g_MarkedEntries) {
            RawMarkedEntry raw;
            raw.EntryID = e.EntryID;
            raw.EntryName = e.EntryName;
            raw.EntryType = e.EntryType;
            raw.BankType = (int32_t)e.BankType;
            raw.TypeName = e.TypeName;
            raw.BankName = e.BankName;
            raw.SubBankName = e.SubBankName;
            raw.SourceFullPath = e.SourceFullPath;
            g_SavedMarkedEntries.push_back(raw);
        }
        SaveConfig();
    }

    static bool IsMarked(const std::string& bankName, const std::string& entryName) {
        for (const auto& existing : g_MarkedEntries) {
            if (existing.EntryName == entryName && existing.BankName == bankName) return true;
        }
        return false;
    }

    static void ToggleMark(const StagedModPackageEntry& entry) {
        for (size_t i = 0; i < g_MarkedEntries.size(); i++) {
            if (g_MarkedEntries[i].EntryName == entry.EntryName && g_MarkedEntries[i].BankName == entry.BankName) {
                g_MarkedEntries.erase(g_MarkedEntries.begin() + i);
                SaveMarkedState();
                return;
            }
        }
        g_MarkedEntries.push_back(entry);
        SaveMarkedState();
    }

    static void ClearAll() { g_MarkedEntries.clear(); SaveMarkedState(); }
};

class ModBankPatcher {
public:
    static inline std::map<std::string, ModAssetOverride> g_ActiveModAssets;

    static void BuildMasterModIndex(const std::vector<ModEntry>& loadedMods) {
        g_ActiveModAssets.clear();
        for (auto it = loadedMods.rbegin(); it != loadedMods.rend(); ++it) {
            const auto& mod = *it;
            if (!mod.IsEnabled || !mod.IsAssetMod) continue;
            std::string modDataPath = mod.ModFolderPath + "\\Data";
            if (!fs::exists(modDataPath)) continue;

            for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".resource") {
                    std::string resPath = entry.path().string();
                    std::string hdrPath = resPath.substr(0, resPath.find_last_of('.')) + ".header";
                    std::string entryName = entry.path().stem().string();
                    std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::tolower);

                    std::string bankName = "";
                    std::string subBankName = "N/A";
                    fs::path parent = entry.path().parent_path();
                    while (parent != modDataPath && !parent.empty()) {
                        std::string pName = parent.filename().string();
                        std::string pNameLower = pName;
                        std::transform(pNameLower.begin(), pNameLower.end(), pNameLower.begin(), ::tolower);
                        if (pNameLower.find(".big") != std::string::npos || pNameLower.find(".lut") != std::string::npos || pNameLower.find(".lug") != std::string::npos) {
                            bankName = pNameLower;
                            if (entry.path().parent_path() != parent) subBankName = entry.path().parent_path().filename().string();
                            break;
                        }
                        parent = parent.parent_path();
                    }

                    if (!bankName.empty()) {
                        std::string lookupKey = bankName + "/" + subBankName + "/" + entryName;
                        ModAssetOverride asset;
                        asset.ResourcePath = resPath;
                        if (fs::exists(hdrPath)) asset.HeaderPath = hdrPath;
                        g_ActiveModAssets[lookupKey] = asset;
                    }
                }
            }
        }
    }

    struct PatchSubBank {
        InternalBankInfo Info;
        std::vector<BankEntry> Entries;
        std::vector<std::vector<uint8_t>> MetaList;
        std::vector<bool> IsModded;
        std::vector<std::string> ModResPath;
    };

    static bool PatchBankForLaunch(LoadedBank* bank) {
        std::string tmpPath = bank->FullPath + ".tmp";
        if (!fs::exists(tmpPath)) return false;

        std::ifstream in(tmpPath, std::ios::binary);
        std::ofstream out(bank->FullPath, std::ios::binary);
        if (!in.is_open() || !out.is_open()) return false;

        std::string lowerBankName = bank->FileName;
        std::transform(lowerBankName.begin(), lowerBankName.end(), lowerBankName.begin(), ::tolower);

        std::vector<char> zeroBuffer(2048, 0);
        struct BIGHeader { char m[4]; uint32_t v; uint32_t fOff; uint32_t fSize; } header;
        in.read((char*)&header, sizeof(header));
        out.write((char*)&header, sizeof(header));

        uint32_t pos = (uint32_t)out.tellp();
        if (pos < 2048) out.write(zeroBuffer.data(), 2048 - pos);
        uint32_t currentDataOffset = (uint32_t)out.tellp();

        in.seekg(header.fOff, std::ios::beg);
        uint32_t bankCount = 0;
        in.read((char*)&bankCount, 4);

        std::vector<PatchSubBank> subBanks;
        uint32_t highestID = 0;

        for (uint32_t i = 0; i < bankCount; i++) {
            PatchSubBank psb;
            std::getline(in, psb.Info.Name, '\0');
            in.read((char*)&psb.Info.Version, 4);
            in.read((char*)&psb.Info.EntryCount, 4);
            in.read((char*)&psb.Info.Offset, 4);
            in.read((char*)&psb.Info.Size, 4);
            in.read((char*)&psb.Info.Align, 4);
            subBanks.push_back(psb);
        }

        for (auto& psb : subBanks) {
            in.seekg(psb.Info.Offset, std::ios::beg);
            uint32_t numTypes = 0;
            in.read((char*)&numTypes, 4);
            if (numTypes < 1000) in.seekg(numTypes * 8, std::ios::cur); else in.seekg(-4, std::ios::cur);

            for (uint32_t e = 0; e < psb.Info.EntryCount; ++e) {
                BankEntry entry;
                uint32_t magic;
                in.read((char*)&magic, 4); in.read((char*)&entry.ID, 4); in.read((char*)&entry.Type, 4);
                in.read((char*)&entry.Size, 4); in.read((char*)&entry.Offset, 4); in.read((char*)&entry.CRC, 4);

                uint32_t nameLen = 0; in.read((char*)&nameLen, 4);
                if (nameLen > 0) {
                    entry.Name.resize(nameLen, '\0');
                    in.read(&entry.Name[0], nameLen);
                    entry.Name.erase(std::remove(entry.Name.begin(), entry.Name.end(), '\0'), entry.Name.end());
                }

                in.read((char*)&entry.Timestamp, 4);
                uint32_t depCount = 0; in.read((char*)&depCount, 4);
                for (uint32_t d = 0; d < depCount; d++) {
                    uint32_t dLen = 0; in.read((char*)&dLen, 4);
                    if (dLen > 0) {
                        std::string dep(dLen, '\0'); in.read(&dep[0], dLen);
                        dep.erase(std::remove(dep.begin(), dep.end(), '\0'), dep.end());
                        entry.Dependencies.push_back(dep);
                    }
                }

                in.read((char*)&entry.InfoSize, 4);
                std::vector<uint8_t> metaData(entry.InfoSize);
                if (entry.InfoSize > 0) in.read((char*)metaData.data(), entry.InfoSize);
                if (entry.ID > highestID) highestID = entry.ID;

                std::string entryNameLower = entry.Name;
                std::transform(entryNameLower.begin(), entryNameLower.end(), entryNameLower.begin(), ::tolower);
                std::string key = lowerBankName + "/" + psb.Info.Name + "/" + entryNameLower;

                bool isModded = false;
                std::string modResPath = "";

                if (g_ActiveModAssets.count(key)) {
                    isModded = true;
                    modResPath = g_ActiveModAssets[key].ResourcePath;
                    entry.Size = fs::file_size(modResPath);
                    if (!g_ActiveModAssets[key].HeaderPath.empty()) {
                        entry.InfoSize = (uint32_t)fs::file_size(g_ActiveModAssets[key].HeaderPath);
                        metaData.resize(entry.InfoSize);
                        std::ifstream hdrIn(g_ActiveModAssets[key].HeaderPath, std::ios::binary);
                        hdrIn.read((char*)metaData.data(), entry.InfoSize);

                        if (metaData.size() >= sizeof(CGraphicHeader)) {
                            CGraphicHeader* hdr = (CGraphicHeader*)metaData.data();
                            hdr->MipSize0 = 0;
                        }
                    }
                    else {
                        entry.InfoSize = 0; metaData.clear();
                    }
                    g_ActiveModAssets[key].IsHandled = true;
                }
                psb.Entries.push_back(entry);
                psb.MetaList.push_back(metaData);
                psb.IsModded.push_back(isModded);
                psb.ModResPath.push_back(modResPath);
            }
        }
        for (auto& [key, asset] : g_ActiveModAssets) {
            if (asset.IsHandled) continue;
            size_t firstSlash = key.find('/'); size_t lastSlash = key.find_last_of('/');
            if (firstSlash == std::string::npos || lastSlash == std::string::npos || firstSlash == lastSlash) continue;

            std::string bName = key.substr(0, firstSlash);
            if (bName != lowerBankName) continue;
            std::string sbName = key.substr(firstSlash + 1, lastSlash - firstSlash - 1);

            PatchSubBank* targetPSB = nullptr;
            for (auto& psb : subBanks) { if (psb.Info.Name == sbName) { targetPSB = &psb; break; } }
            if (!targetPSB) continue;

            highestID++;
            BankEntry newEntry;
            newEntry.ID = highestID;
            newEntry.Name = fs::path(asset.ResourcePath).stem().string();
            if (lowerBankName == "graphics.big" || lowerBankName == "xboxgraphics.big") newEntry.Type = 1; else newEntry.Type = 0;
            newEntry.Size = fs::file_size(asset.ResourcePath);
            newEntry.Offset = 0; newEntry.CRC = 0; newEntry.Timestamp = 0;

            std::vector<uint8_t> newMeta;
            if (!asset.HeaderPath.empty() && fs::exists(asset.HeaderPath)) {
                newEntry.InfoSize = fs::file_size(asset.HeaderPath);
                newMeta.resize(newEntry.InfoSize);
                std::ifstream hdrIn(asset.HeaderPath, std::ios::binary);
                hdrIn.read((char*)newMeta.data(), newEntry.InfoSize);
            }
            else { newEntry.InfoSize = 0; }

            targetPSB->Entries.push_back(newEntry);
            targetPSB->MetaList.push_back(newMeta);
            targetPSB->IsModded.push_back(true);
            targetPSB->ModResPath.push_back(asset.ResourcePath);
            asset.IsHandled = true;
        }

        for (auto& psb : subBanks) {
            uint32_t sbAlign = (psb.Info.Align == 0) ? 1 : psb.Info.Align;
            for (size_t i = 0; i < psb.Entries.size(); ++i) {
                auto& entry = psb.Entries[i];
                if (sbAlign > 1) {
                    uint32_t p = (uint32_t)out.tellp(); uint32_t r = p % sbAlign;
                    if (r != 0) { out.write(zeroBuffer.data(), sbAlign - r); currentDataOffset += (sbAlign - r); }
                }

                uint32_t savedOriginalOffset = entry.Offset;
                entry.Offset = currentDataOffset;
                if (entry.Size > 0) {
                    if (psb.IsModded[i]) {
                        std::ifstream resIn(psb.ModResPath[i], std::ios::binary);
                        std::vector<char> buffer(entry.Size);
                        resIn.read(buffer.data(), entry.Size);
                        out.write(buffer.data(), entry.Size);
                    }
                    else {
                        std::streampos returnPos = in.tellg();
                        in.seekg(savedOriginalOffset, std::ios::beg);
                        std::vector<char> buffer(entry.Size);
                        in.read(buffer.data(), entry.Size);
                        out.write(buffer.data(), entry.Size);
                        in.seekg(returnPos, std::ios::beg);
                    }
                }
                currentDataOffset += entry.Size;
            }
        }

        uint32_t tocPos = (uint32_t)out.tellp();
        if (tocPos % 2048 != 0) out.write(zeroBuffer.data(), 2048 - (tocPos % 2048));

        for (auto& psb : subBanks) {
            psb.Info.Offset = (uint32_t)out.tellp();
            psb.Info.EntryCount = (uint32_t)psb.Entries.size();

            std::map<uint32_t, uint32_t> typeCounts;
            for (const auto& e : psb.Entries) typeCounts[e.Type]++;
            uint32_t nTypes = (uint32_t)typeCounts.size();
            out.write((char*)&nTypes, 4);
            for (const auto& [type, count] : typeCounts) {
                uint32_t t = type; uint32_t c = count;
                out.write((char*)&t, 4); out.write((char*)&c, 4);
            }

            for (size_t i = 0; i < psb.Entries.size(); i++) {
                const auto& e = psb.Entries[i];
                uint32_t magic = 42; out.write((char*)&magic, 4);
                out.write((char*)&e.ID, 4); out.write((char*)&e.Type, 4); out.write((char*)&e.Size, 4);
                out.write((char*)&e.Offset, 4); out.write((char*)&e.CRC, 4);
                uint32_t nameLen = (uint32_t)e.Name.length(); out.write((char*)&nameLen, 4);
                if (nameLen > 0) out.write(e.Name.c_str(), nameLen);
                out.write((char*)&e.Timestamp, 4);

                uint32_t dCount = (uint32_t)e.Dependencies.size(); out.write((char*)&dCount, 4);
                for (const auto& dep : e.Dependencies) {
                    uint32_t dLen = (uint32_t)dep.length(); out.write((char*)&dLen, 4);
                    if (dLen > 0) out.write(dep.c_str(), dLen);
                }
                out.write((char*)&e.InfoSize, 4);
                if (e.InfoSize > 0) out.write((char*)psb.MetaList[i].data(), e.InfoSize);
            }
            psb.Info.Size = (uint32_t)out.tellp() - psb.Info.Offset;
        }

        uint32_t footerStart = (uint32_t)out.tellp();
        uint32_t outBankCount = (uint32_t)subBanks.size();
        out.write((char*)&outBankCount, 4);

        for (const auto& psb : subBanks) {
            out.write(psb.Info.Name.c_str(), psb.Info.Name.length() + 1);
            out.write((char*)&psb.Info.Version, 4);
            out.write((char*)&psb.Info.EntryCount, 4);
            out.write((char*)&psb.Info.Offset, 4);
            out.write((char*)&psb.Info.Size, 4);
            out.write((char*)&psb.Info.Align, 4);
        }

        uint32_t footerSize = (uint32_t)out.tellp() - footerStart;
        out.seekp(0, std::ios::beg);
        header.fOff = footerStart; header.fSize = footerSize;
        out.write((char*)&header, sizeof(header));

        in.close(); out.close();
        return true;
    }
};

class ModManagerCompiler {
public:
    static std::string ExtractLanguageFromPath(const std::string& fullPath) {
        std::string lowerPath = fullPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
        size_t langPos = lowerPath.find("\\lang\\");
        if (langPos == std::string::npos) langPos = lowerPath.find("/lang/");
        if (langPos != std::string::npos) {
            size_t start = langPos + 6;
            size_t end = fullPath.find_first_of("\\/", start);
            if (end != std::string::npos) return fullPath.substr(start, end - start);
        }
        return "English";
    }

    static std::string GetTargetDirectoryForEntry(const StagedModPackageEntry& entry, const std::string& modRoot) {
        std::string basePath = "";
        std::string bankCategory = entry.BankName;
        std::string finalSubBank = entry.SubBankName;
        if (finalSubBank != "N/A" && !finalSubBank.empty()) {
            if (finalSubBank == "GBANK_MAIN") finalSubBank = "GBANK_MAIN_PC";
            else if (finalSubBank == "GBANK_GUI") finalSubBank = "GBANK_GUI_PC";
            else if (finalSubBank == "GBANK_FRONT_END") finalSubBank = "GBANK_FRONT_END_PC";
            else if (finalSubBank == "PARTICLE_MAIN") finalSubBank = "PARTICLE_MAIN_PC";
        }

        if (entry.BankType == EBankType::Graphics || entry.BankType == EBankType::XboxGraphics) {
            if (finalSubBank == "GBANK_MAIN_PC" || finalSubBank == "GBANK_GUI_PC") basePath = "Data\\Graphics\\pc";
            else basePath = "Data\\Graphics";
        }
        else if (entry.BankType == EBankType::Textures || entry.BankType == EBankType::Frontend) basePath = "Data\\Graphics\\pc";
        else if (entry.BankType == EBankType::Effects) basePath = "Data\\Misc\\pc";
        else if (entry.BankType == EBankType::Shaders) basePath = "Data\\Shaders\\pc";
        else if (entry.BankType == EBankType::Audio) {
            std::string lowerBank = entry.BankName;
            std::transform(lowerBank.begin(), lowerBank.end(), lowerBank.begin(), ::tolower);
            if (lowerBank.find(".lut") != std::string::npos) basePath = "Data\\Lang\\" + ExtractLanguageFromPath(entry.SourceFullPath);
            else basePath = "Data\\Sound";
        }
        else if (entry.BankType == EBankType::Dialogue || entry.BankType == EBankType::Text || entry.BankType == EBankType::Fonts) basePath = "Data\\Lang\\" + ExtractLanguageFromPath(entry.SourceFullPath);
        else basePath = "Data\\Unknown";

        if (finalSubBank != "N/A" && !finalSubBank.empty()) return modRoot + "\\" + basePath + "\\" + bankCategory + "\\" + finalSubBank;
        return modRoot + "\\" + basePath + "\\" + bankCategory;
    }

    static std::string SanitizeForEnum(const std::string& name, bool forceUpper = true) {
        std::string out = name;
        for (char& c : out) {
            if (!std::isalnum(c)) c = '_';
            else if (forceUpper) c = (char)std::toupper(c);
        }
        if (!out.empty() && std::isdigit(out[0])) out = "_" + out;
        if (out.empty()) out = "UNKNOWN_ENTRY";
        return out;
    }

    static void GenerateEnumHeaders(const std::string& modName, const std::vector<StagedModPackageEntry>& entries) {
        std::string baseHeaderPath = fs::current_path().string() + "\\ExportedMods\\" + modName + "\\Data\\Defs\\RetailHeaders";
        std::string pcHeaderPath = baseHeaderPath + "\\pc";

        std::map<std::string, std::map<std::string, std::vector<StagedModPackageEntry>>> filesToGenerate;

        for (const auto& entry : entries) {
            std::string targetFile = ""; std::string enumName = ""; bool isPcPath = false;
            if (entry.BankType == EBankType::Fonts || entry.BankType == EBankType::Shaders || entry.BankType == EBankType::Audio) continue;

            if (entry.BankType == EBankType::Graphics || entry.BankType == EBankType::XboxGraphics) {
                if (entry.SubBankName == "MBANK_ALLMESHES") {
                    targetFile = "meshdata.h";
                    if (entry.EntryType >= 1 && entry.EntryType <= 5) enumName = "EMeshType2";
                    else if (entry.EntryType >= 6 && entry.EntryType <= 10) enumName = "EAnimType2";
                }
            }
            else if (entry.BankType == EBankType::Text) {
                if (entry.EntryType == 0) { targetFile = "text.h"; enumName = "EGameText"; }
                else if (entry.EntryType == 1) { targetFile = "text.h"; enumName = "ETextGroup"; }
            }
            else if (entry.BankType == EBankType::Dialogue && entry.EntryType == 1) {
                std::string sub = entry.SubBankName;
                if (sub.find("_MAIN_2") != std::string::npos) { targetFile = "dialogue_lipsync2.h"; enumName = "ELipSync3"; }
                else if (sub.find("_MAIN") != std::string::npos) { targetFile = "dialogue_lipsync.h"; enumName = "ELipSync"; }
                else if (sub.find("_SCRIPT_2") != std::string::npos) { targetFile = "script_lipsync2.h"; enumName = "ELipSync4"; }
                else if (sub.find("_SCRIPT") != std::string::npos) { targetFile = "script_lipsync.h"; enumName = "ELipSync2"; }
            }
            else if (entry.BankType == EBankType::Effects) {
                targetFile = "particles.h"; enumName = "EParticleEmitter"; isPcPath = true;
            }
            else if (entry.BankType == EBankType::Textures || entry.BankType == EBankType::Frontend) {
                if (entry.EntryName.size() >= 2 && entry.EntryName.substr(0, 2) == "[\\") continue;
                isPcPath = true; std::string sub = entry.SubBankName;
                if (sub == "GBANK_MAIN_PC" || sub == "GBANK_MAIN") { targetFile = "textures.h"; enumName = "EEngineGraphic"; }
                else if (sub == "GBANK_GUI_PC" || sub == "GBANK_GUI") { targetFile = "gui_bank.h"; enumName = "EGuiGraphicBank"; }
                else if (sub == "GBANK_FRONT_END" || sub == "GBANK_FRONT_END_PC") { targetFile = "front_end_bank.h"; enumName = "EFrontEndGraphicBank"; }
            }

            if (!targetFile.empty() && !enumName.empty()) {
                std::string fullPath = (isPcPath ? pcHeaderPath : baseHeaderPath) + "\\" + targetFile;
                filesToGenerate[fullPath][enumName].push_back(entry);
            }
        }

        if (!fs::exists(baseHeaderPath)) fs::create_directories(baseHeaderPath);
        if (!fs::exists(pcHeaderPath)) fs::create_directories(pcHeaderPath);

        for (const auto& [filePath, enumsMap] : filesToGenerate) {
            std::ofstream outFile(filePath);
            if (!outFile.is_open()) continue;
            outFile << "#pragma once\n\n";
            for (const auto& [name, enumEntries] : enumsMap) {
                outFile << "enum " << name << "\n{\n";
                bool isLipSync = (name.find("LipSync") != std::string::npos);
                for (const auto& e : enumEntries) outFile << "    " << SanitizeForEnum(e.EntryName, !isLipSync) << " = " << e.EntryID << ",\n";
                outFile << "};\n\n";
            }
            outFile.close();
        }
    }

    static void GenerateAudioDefs(const std::string& modName, const std::vector<StagedModPackageEntry>& entries) {
        std::string defsDir = fs::current_path().string() + "\\ExportedMods\\" + modName + "\\Data\\Defs";
        std::map<std::string, std::vector<StagedModPackageEntry>> audioMap;

        for (const auto& e : entries) {
            if (e.BankType == EBankType::Audio) {
                std::string lowerBank = e.BankName;
                std::transform(lowerBank.begin(), lowerBank.end(), lowerBank.begin(), ::tolower);
                std::string targetHeader = "";

                if (lowerBank == "dialogue.lut") targetHeader = "dialoguesnds.h";
                else if (lowerBank == "dialogue2.lut") targetHeader = "dialoguesnds2.h";
                else if (lowerBank == "scriptdialogue.lut") targetHeader = "scriptdialoguesnds.h";
                else if (lowerBank == "scriptdialogue2.lut") targetHeader = "scriptdialoguesnds2.h";
                else if (lowerBank == "ingame.lug") targetHeader = "gamesnds.h";

                if (!targetHeader.empty()) {
                    audioMap[targetHeader].push_back(e);
                }
            }
        }

        if (audioMap.empty()) return;
        if (!fs::exists(defsDir)) fs::create_directories(defsDir);

        std::regex entryRegex(R"(([A-Za-z0-9_]+)\s*=\s*(\d+))");

        for (const auto& [headerName, modEntries] : audioMap) {
            std::string vanillaContent = "";

            std::string sourcePath = g_AppConfig.GameRootPath + "\\Data\\Defs\\" + headerName;
            std::ifstream inFile(sourcePath, std::ios::binary);

            if (inFile.is_open()) {
                vanillaContent = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
                inFile.close();
            }
            else {
                for (const auto& enumEntry : g_DefWorkspace.AllEnums) {
                    if (fs::path(enumEntry.FilePath).filename().string() == headerName) {
                        vanillaContent = enumEntry.FullContent;
                        break;
                    }
                }
            }

            if (vanillaContent.empty()) continue;

            std::set<std::string> existingNames;
            std::sregex_iterator begin(vanillaContent.begin(), vanillaContent.end(), entryRegex);
            std::sregex_iterator end;
            for (auto it = begin; it != end; ++it) existingNames.insert((*it)[1].str());

            std::string newEntriesStr = "";
            for (const auto& e : modEntries) {
                std::string sanitized = "";

                if (headerName == "gamesnds.h") {
                    std::string rawName = e.EntryName;
                    size_t extPos = rawName.find(".wav");
                    if (extPos != std::string::npos) rawName = rawName.substr(0, extPos);
                    sanitized = "SND_" + rawName;
                    std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(), ::toupper);
                    sanitized = SanitizeForEnum(sanitized, true);
                }
                else {
                    std::regex specificIdRegex(R"(([A-Za-z0-9_]+)\s*=\s*)" + std::to_string(e.EntryID) + R"(\b)");
                    std::sregex_iterator idBegin(vanillaContent.begin(), vanillaContent.end(), specificIdRegex);

                    if (idBegin != end) sanitized = (*idBegin)[1].str();
                    else sanitized = "SND_" + SanitizeForEnum(e.EntryName, true);
                }

                if (existingNames.find(sanitized) == existingNames.end()) {
                    newEntriesStr += "    " + sanitized + " = " + std::to_string(e.EntryID) + ",\n";
                    existingNames.insert(sanitized);
                }
            }

            if (!newEntriesStr.empty()) {
                size_t insertPos = vanillaContent.find_last_of('}');
                if (insertPos != std::string::npos) {
                    vanillaContent.insert(insertPos, newEntriesStr);
                }
            }

            std::string outHeaderPath = defsDir + "\\" + headerName;
            std::ofstream hFile(outHeaderPath);
            if (hFile.is_open()) {
                hFile << vanillaContent;
                hFile.close();
            }
        }
    }

    static void BuildPackageStructure(const std::string& modName, const std::vector<StagedModPackageEntry>& entries) {
        if (modName.empty() || entries.empty()) return;

        for (auto& bank : g_OpenBanks) {
            if (!bank.StagedEntries.empty() || !bank.ModifiedEntryData.empty()) {
                if (bank.Type == EBankType::Audio) SaveAudioBank(&bank);
                else SaveBigBank(&bank);
            }
        }

        std::string modRoot = fs::current_path().string() + "\\ExportedMods\\" + modName;
        if (!fs::exists(modRoot)) fs::create_directories(modRoot);

        std::map<std::string, std::string> defFiles;
        std::map<std::string, std::string> headerFiles;
        std::map<std::string, std::string> eventFiles;

        for (const auto& entry : entries) {
            if (entry.Category != EModAssetCategory::BankEntry) {

                std::string sourcePath = entry.SourceFullPath;
                std::replace(sourcePath.begin(), sourcePath.end(), '/', '\\');

                std::string lowerPath = sourcePath;
                std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

                size_t dataPos = lowerPath.find("\\data\\");
                if (dataPos != std::string::npos) {

                    std::string relativePath = sourcePath.substr(dataPos);
                    if (relativePath.length() > 0 && relativePath[0] == '\\') relativePath = relativePath.substr(1);

                    std::string targetFilePath = modRoot + "\\" + relativePath;

                    if (entry.Category == EModAssetCategory::Definition) {
                        auto ExtractDefFromMap = [&](const std::map<std::string, std::vector<DefEntry>>& mapToSearch) -> bool {
                            for (const auto& [type, defList] : mapToSearch) {
                                for (const auto& def : defList) {
                                    if (def.Name == entry.EntryName) {
                                        // Strict Case-Insensitive Path Match prevents grabbing GameDefs instead of ScriptDefs
                                        std::string defSrc = def.SourceFile;
                                        std::string entSrc = entry.SourceFullPath;
                                        std::transform(defSrc.begin(), defSrc.end(), defSrc.begin(), ::tolower);
                                        std::transform(entSrc.begin(), entSrc.end(), entSrc.begin(), ::tolower);
                                        std::replace(defSrc.begin(), defSrc.end(), '/', '\\');
                                        std::replace(entSrc.begin(), entSrc.end(), '/', '\\');

                                        if (defSrc == entSrc) {
                                            std::ifstream inFile(def.SourceFile, std::ios::binary | std::ios::ate);
                                            if (inFile.is_open()) {
                                                std::streamsize fileSize = inFile.tellg();
                                                if (def.StartOffset >= 0 && def.EndOffset <= fileSize && def.EndOffset > def.StartOffset) {
                                                    inFile.seekg(def.StartOffset, std::ios::beg);
                                                    std::string buffer(def.EndOffset - def.StartOffset, '\0');
                                                    if (inFile.read(&buffer[0], buffer.size())) {
                                                        defFiles[targetFilePath] += buffer + "\n\n";
                                                    }
                                                }
                                                inFile.close();
                                            }
                                            return true;
                                        }
                                    }
                                }
                            }
                            return false;
                            };

                        if (!ExtractDefFromMap(g_DefWorkspace.CategorizedDefs)) {
                            for (const auto& context : g_DefWorkspace.Contexts) {
                                if (ExtractDefFromMap(context.CategorizedDefs)) break;
                            }
                        }
                    }
                    else if (entry.Category == EModAssetCategory::Header) {
                        for (const auto& enumEntry : g_DefWorkspace.AllEnums) {
                            if (enumEntry.Name == entry.EntryName) {
                                headerFiles[targetFilePath] += enumEntry.FullContent + "\n\n";
                                break;
                            }
                        }
                    }
                    else if (entry.Category == EModAssetCategory::AnimationEvent) {
                        EventFile* fileToSearch = (entry.TypeName == "Sound Event") ? &g_EventWorkspace.SoundEvents : &g_EventWorkspace.GameEvents;
                        for (const auto& ev : fileToSearch->Events) {
                            if (ev.AnimName == entry.EntryName) {
                                eventFiles[targetFilePath] += "BEGIN_EVENTS: " + ev.AnimName + "\n" + ev.Content;
                                if (!ev.Content.empty() && ev.Content.back() != '\n') eventFiles[targetFilePath] += "\n";
                                eventFiles[targetFilePath] += "END_EVENTS\n\n";
                                break;
                            }
                        }
                    }
                }
                continue;
            }

            std::string targetDir = GetTargetDirectoryForEntry(entry, modRoot);
            if (!fs::exists(targetDir)) fs::create_directories(targetDir);

            LoadedBank* targetBank = nullptr;
            for (auto& b : g_OpenBanks) { if (b.FileName == entry.BankName) { targetBank = &b; break; } }
            if (!targetBank) continue;

            int entryIndex = -1;
            for (int i = 0; i < (int)targetBank->Entries.size(); ++i) {
                if (targetBank->Entries[i].ID == entry.EntryID) { entryIndex = i; break; }
            }
            if (entryIndex == -1) continue;

            const auto& bankEntry = targetBank->Entries[entryIndex];
            std::string resourcePath = targetDir + "\\" + entry.EntryName + ".resource";
            std::ofstream resFile(resourcePath, std::ios::binary);

            if (resFile.is_open()) {
                if (targetBank->ModifiedEntryData.count(entryIndex)) {
                    const auto& data = targetBank->ModifiedEntryData[entryIndex];
                    resFile.write((const char*)data.data(), data.size());
                }
                else if (bankEntry.Size > 0) {
                    std::ifstream diskStream;
                    std::istream* activeStream = nullptr;

                    if (targetBank->Stream && targetBank->Stream->is_open()) activeStream = targetBank->Stream.get();
                    else { diskStream.open(targetBank->FullPath, std::ios::binary); if (diskStream.is_open()) activeStream = &diskStream; }

                    if (activeStream) {
                        activeStream->clear();
                        activeStream->seekg(bankEntry.Offset, std::ios::beg);
                        std::vector<char> buffer(bankEntry.Size);
                        activeStream->read(buffer.data(), bankEntry.Size);
                        resFile.write(buffer.data(), bankEntry.Size);
                    }
                    if (diskStream.is_open()) diskStream.close();
                }
                resFile.close();
            }

            if (bankEntry.InfoSize > 0 && targetBank->SubheaderCache.count(entryIndex)) {
                std::string headerPath = targetDir + "\\" + entry.EntryName + ".header";
                std::ofstream hdrFile(headerPath, std::ios::binary);
                if (hdrFile.is_open()) {
                    std::vector<uint8_t> metaData = targetBank->SubheaderCache[entryIndex];
                    if (metaData.size() >= sizeof(CGraphicHeader)) {
                        CGraphicHeader* hdr = (CGraphicHeader*)metaData.data();
                        hdr->MipSize0 = 0;
                    }

                    hdrFile.write((const char*)metaData.data(), metaData.size());
                    hdrFile.close();
                }
            }
        }

        auto WriteMappedFiles = [](const std::map<std::string, std::string>& fileMap, const std::string& header, const std::string& footer) {
            for (const auto& [filePath, content] : fileMap) {
                if (content.empty()) continue;

                std::string finalDir = std::filesystem::path(filePath).parent_path().string();
                if (!std::filesystem::exists(finalDir)) {
                    std::filesystem::create_directories(finalDir);
                }

                std::ofstream outFile(filePath, std::ios::binary | std::ios::trunc);
                if (outFile.is_open()) {
                    if (!header.empty()) outFile << header;
                    outFile << content;
                    if (!footer.empty()) outFile << footer;
                    outFile.close();
                }
            }
            };

        WriteMappedFiles(defFiles, "", "");
        WriteMappedFiles(headerFiles, "#pragma once\n\n", "");
        WriteMappedFiles(eventFiles, "BEGIN_ANIMATION_EVENTS\n\n", "END_ANIMATION_EVENTS\n");

        GenerateEnumHeaders(modName, entries);
        GenerateAudioDefs(modName, entries);
    }
};