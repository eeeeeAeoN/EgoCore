#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <set>
#include <windows.h>
#include <shellapi.h>
#include "ModManagerCompiler.h"
#include "BankEditor.h"

namespace fs = std::filesystem;

inline int g_LaunchState = 0;

class ModManagerBackend {
public:
    static inline std::vector<ModEntry> g_LoadedMods;
    static inline std::string g_EgoCoreModOrderFile = "egocore_modorder.txt";

    static void InitializeAndLoad() {
        g_LoadedMods.clear();
        std::string modsDir = g_AppConfig.GameRootPath + "\\Mods";

        if (!fs::exists(modsDir)) fs::create_directories(modsDir);

        std::vector<std::pair<std::string, bool>> savedOrder;
        std::ifstream orderFile(g_EgoCoreModOrderFile);
        if (orderFile.is_open()) {
            std::string line;
            while (std::getline(orderFile, line)) {
                if (line.empty()) continue;

                size_t delim = line.find_last_of('|');
                if (delim != std::string::npos) {
                    std::string name = line.substr(0, delim);
                    bool isEnabled = (line.substr(delim + 1) == "1");
                    savedOrder.push_back({ name, isEnabled });
                }
                else {
                    savedOrder.push_back({ line, false });
                }
            }
        }

        std::vector<ModEntry> discoveredMods;

        if (fs::exists(g_AppConfig.GameRootPath + "\\FableScriptExtender.dll")) {
            ModEntry fse;
            fse.Name = "FableScriptExtender";
            fse.Description = "Core Script Extender for Fable. Required for advanced DLL mods to function.";
            fse.HasDll = true;
            fse.IsCoreMod = true;
            fse.ModFolderPath = g_AppConfig.GameRootPath;
            discoveredMods.push_back(fse);
        }

        for (const auto& entry : fs::directory_iterator(modsDir)) {
            if (entry.is_directory()) {
                ModEntry mod;
                mod.Name = entry.path().filename().string();
                mod.ModFolderPath = entry.path().string();

                std::string infoPath = mod.ModFolderPath + "\\" + mod.Name + "_Info.txt";
                if (fs::exists(infoPath)) {
                    std::ifstream infoFile(infoPath);
                    std::string content((std::istreambuf_iterator<char>(infoFile)), std::istreambuf_iterator<char>());
                    mod.Description = content;
                }
                else {
                    mod.Description = "No description provided.";
                }

                mod.HasDll = fs::exists(mod.ModFolderPath + "\\" + mod.Name + ".dll");
                mod.IsAssetMod = fs::exists(mod.ModFolderPath + "\\Data");

                mod.SettingsIniPath = mod.ModFolderPath + "\\" + mod.Name + ".ini";
                if (fs::exists(mod.SettingsIniPath)) LoadModSettings(mod);

                discoveredMods.push_back(mod);
            }
        }

        SyncWithGlobalModsIni(discoveredMods);

        for (const auto& savedMod : savedOrder) {
            auto it = std::find_if(discoveredMods.begin(), discoveredMods.end(), [&](const ModEntry& m) { return m.Name == savedMod.first; });
            if (it != discoveredMods.end()) {
                if (!it->HasDll && !it->IsCoreMod) {
                    it->IsEnabled = savedMod.second;
                }
                g_LoadedMods.push_back(*it);
                discoveredMods.erase(it);
            }
        }
        for (const auto& newMod : discoveredMods) g_LoadedMods.push_back(newMod);

        SaveLoadOrder();
        UpdateGlobalModsIni();
    }

    static void SyncWithGlobalModsIni(std::vector<ModEntry>& modsList) {
        std::string modsIniPath = g_AppConfig.GameRootPath + "\\mods.ini";
        if (!fs::exists(modsIniPath)) return;

        std::ifstream file(modsIniPath);
        std::string line;
        while (std::getline(file, line)) {
            size_t first = line.find_first_not_of(" \t");
            if (first != std::string::npos && line[first] == ';') continue;

            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);

                if (key == "FableScriptExtender.dll") {
                    for (auto& m : modsList) if (m.IsCoreMod) m.IsEnabled = (val.find('1') != std::string::npos);
                    continue;
                }

                size_t slash = key.find('\\');
                if (slash != std::string::npos) {
                    std::string modName = key.substr(0, slash);
                    for (auto& m : modsList) {
                        if (m.Name == modName && !m.IsCoreMod) m.IsEnabled = (val.find('1') != std::string::npos);
                    }
                }
            }
        }
    }

    static void UpdateGlobalModsIni() {
        std::string modsIniPath = g_AppConfig.GameRootPath + "\\mods.ini";
        std::vector<std::string> newIniLines;
        bool inMods = false;
        bool sectionFound = false;

        if (fs::exists(modsIniPath)) {
            std::ifstream in(modsIniPath);
            std::string line;
            while (std::getline(in, line)) {
                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));

                if (trimmed.find("[Mods]") == 0) {
                    inMods = true; sectionFound = true;
                    newIniLines.push_back(line); continue;
                }
                if (inMods && !trimmed.empty() && trimmed[0] == '[') inMods = false;
                if (inMods) {
                    if (!trimmed.empty() && trimmed[0] == ';') { newIniLines.push_back(line); continue; }
                    if (trimmed.find(".dll") != std::string::npos) continue;
                }
                newIniLines.push_back(line);
            }
        }

        if (!sectionFound) newIniLines.push_back("[Mods]");

        auto it = std::find_if(newIniLines.begin(), newIniLines.end(), [](const std::string& s) { return s.find("[Mods]") != std::string::npos; });
        if (it != newIniLines.end()) {
            std::vector<std::string> injectLines;
            for (const auto& mod : g_LoadedMods) {
                if (mod.HasDll) {
                    if (mod.IsCoreMod) injectLines.push_back("FableScriptExtender.dll=" + std::string(mod.IsEnabled ? "1" : "0"));
                    else injectLines.push_back(mod.Name + "\\" + mod.Name + ".dll=" + (mod.IsEnabled ? "1" : "0"));
                }
            }
            newIniLines.insert(it + 1, injectLines.begin(), injectLines.end());
        }

        std::ofstream out(modsIniPath);
        for (const auto& l : newIniLines) out << l << "\n";
    }

    static void LoadModSettings(ModEntry& mod) {
        mod.SettingsLines.clear();
        std::ifstream file(mod.SettingsIniPath);
        std::string line;
        while (std::getline(file, line)) {
            IniLine il;
            il.Raw = line;
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first != std::string::npos && line[first] != ';' && line[first] != '[') {
                size_t eqPos = line.find('=');
                if (eqPos != std::string::npos) {
                    il.IsKeyValue = true;
                    il.Key = line.substr(first, eqPos - first);
                    il.Value = line.substr(eqPos + 1);
                    il.Key.erase(il.Key.find_last_not_of(" \t\r\n") + 1);
                    il.Value.erase(0, il.Value.find_first_not_of(" \t\r\n"));
                    il.Value.erase(il.Value.find_last_not_of(" \t\r\n") + 1);
                }
            }
            mod.SettingsLines.push_back(il);
        }
    }

    static void SaveModSettings(const ModEntry& mod) {
        if (mod.SettingsIniPath.empty()) return;
        std::ofstream file(mod.SettingsIniPath);
        for (const auto& il : mod.SettingsLines) {
            if (il.IsKeyValue) file << il.Key << "=" << il.Value << "\n";
            else file << il.Raw << "\n";
        }
    }

    static void SaveLoadOrder() {
        std::ofstream orderFile(g_EgoCoreModOrderFile);
        for (const auto& mod : g_LoadedMods) orderFile << mod.Name << "|" << (mod.IsEnabled ? "1" : "0") << "\n";
    }

    static void DeleteMod(int index) {
        if (index < 0 || index >= g_LoadedMods.size()) return;
        if (g_LoadedMods[index].IsCoreMod) return;

        std::string path = g_LoadedMods[index].ModFolderPath;
        try { if (fs::exists(path)) fs::remove_all(path); }
        catch (...) {}
        g_LoadedMods.erase(g_LoadedMods.begin() + index);
        SaveLoadOrder(); UpdateGlobalModsIni();
    }

    static void ProcessModsAndLaunch() {
        std::set<std::string> neededFiles;
        for (const auto& mod : g_LoadedMods) {
            if (mod.IsEnabled && mod.IsAssetMod) BuildNeedListForMod(mod, neededFiles);
        }

        if (g_AppConfig.ModSystemDirty) {
            ModBankPatcher::BuildMasterModIndex(g_LoadedMods);

            // --- 0. FORCE CLOSE ALL STREAMS ---
            for (auto& bank : g_OpenBanks) {
                if (bank.Stream && bank.Stream->is_open()) {
                    bank.Stream->close();
                }
            }

            // --- 1. GLOBAL VANILLA RESTORE (Banks & Loose Files) ---
            // This robust global sweep replaces the old g_DefWorkspace loop, catching ALL files safely
            std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
            if (fs::exists(dataPath)) {
                for (const auto& entry : fs::recursive_directory_iterator(dataPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".tmp") {
                        std::string tmpPath = entry.path().string();
                        std::string originalPath = tmpPath.substr(0, tmpPath.length() - 4);

                        try {
                            if (fs::exists(originalPath)) fs::remove(originalPath);
                            fs::rename(tmpPath, originalPath);
                        }
                        catch (...) {}
                    }
                }
            }

            // --- 2. LOAD DEPS ---
            EnsureNeededBanksAreLoaded(neededFiles);
            LoadHeadersFromDir(g_AppConfig.GameRootPath);

            // --- 3. CREATE NEW BACKUPS FOR ACTIVE BANKS ---
            for (auto& bank : g_OpenBanks) {
                std::string bankFileName = fs::path(bank.FullPath).filename().string();
                std::transform(bankFileName.begin(), bankFileName.end(), bankFileName.begin(), ::tolower);

                if (neededFiles.find(bankFileName) != neededFiles.end()) {
                    std::string tmpPath = bank.FullPath + ".tmp";
                    if (!fs::exists(tmpPath)) fs::copy_file(bank.FullPath, tmpPath);
                }
            }

            // --- 4. INJECT MOD BANKS (Audio & Big) ---
            for (auto& bank : g_OpenBanks) {
                std::string bankFileName = fs::path(bank.FullPath).filename().string();
                std::transform(bankFileName.begin(), bankFileName.end(), bankFileName.begin(), ::tolower);

                if (neededFiles.find(bankFileName) != neededFiles.end()) {
                    if (bank.Stream && bank.Stream->is_open()) bank.Stream->close();

                    if (bank.Type == EBankType::Audio) {
                        bank.ModifiedEntryData.clear();
                        ImplantAudioMods(&bank);
                        if (bank.IsDirty) {
                            SaveAudioBank(&bank);
                            bank.IsDirty = false;
                        }
                    }
                    else {
                        if (ModBankPatcher::PatchBankForLaunch(&bank)) {
                            ReloadBankInPlace(&bank);
                        }
                    }
                }
                else {
                    ReloadBankInPlace(&bank);
                }
            }

            // --- 5. INJECT LOOSE FILES (.h, .bin) DIRECTLY FROM MOD FOLDERS ---
            for (const auto& mod : g_LoadedMods) {
                if (!mod.IsEnabled || !mod.IsAssetMod) continue;
                std::string modDataPath = mod.ModFolderPath + "\\Data";

                if (fs::exists(modDataPath)) {
                    for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                        if (entry.is_regular_file()) {
                            std::string ext = entry.path().extension().string();
                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                            // Copy standalone definition files matching the structure directly to the Game Root
                            if (ext == ".h" || ext == ".bin") {
                                std::string relPath = entry.path().string().substr(modDataPath.length());
                                std::string targetGamePath = dataPath + relPath;

                                fs::create_directories(fs::path(targetGamePath).parent_path());

                                std::string tmpPath = targetGamePath + ".tmp";
                                if (fs::exists(targetGamePath) && !fs::exists(tmpPath)) {
                                    fs::copy_file(targetGamePath, tmpPath);
                                }

                                fs::copy_file(entry.path(), targetGamePath, fs::copy_options::overwrite_existing);
                            }
                        }
                    }
                }
            }

            // --- 6. COMPILE GLOBAL .BIN FILES ---
            CompileAudioBinFiles();

            ResetAudioPlayers();
            g_AppConfig.ModSystemDirty = false;
            SaveConfig();
        }
        else {
            EnsureNeededBanksAreLoaded(neededFiles);
            LoadHeadersFromDir(g_AppConfig.GameRootPath);
        }

        std::string exePath = g_AppConfig.GameRootPath + "\\FableLauncher.exe";
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, g_AppConfig.GameRootPath.c_str(), SW_SHOWDEFAULT);
        exit(0);
    }

private:
    static void CompileAudioBinFiles() {
        std::string defsDir = g_AppConfig.GameRootPath + "\\Data\\Defs";
        if (!fs::exists(defsDir)) return;

        std::vector<std::string> targetHeaders = {
            "gamesnds.h", "dialoguesnds.h", "dialoguesnds2.h",
            "scriptdialoguesnds.h", "scriptdialoguesnds2.h"
        };

        std::regex entryRegex(R"(([A-Za-z0-9_]+)\s*=\s*(\d+))");

        for (const auto& header : targetHeaders) {
            std::string headerPath = defsDir + "\\" + header;
            if (!fs::exists(headerPath)) continue;

            std::ifstream inFile(headerPath);
            if (!inFile.is_open()) continue;

            std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();

            struct BinEntry { uint32_t ID; uint32_t CRC; };
            std::vector<BinEntry> entries;

            std::sregex_iterator begin(content.begin(), content.end(), entryRegex);
            std::sregex_iterator end;
            for (auto it = begin; it != end; ++it) {
                BinEntry be;
                be.ID = std::stoul((*it)[2].str());
                be.CRC = BinaryParser::CalculateCRC32_Fable((*it)[1].str());
                entries.push_back(be);
            }

            if (entries.empty()) continue;

            std::sort(entries.begin(), entries.end(), [](const BinEntry& a, const BinEntry& b) {
                return a.CRC < b.CRC;
                });

            std::string binPath = defsDir + "\\" + fs::path(header).stem().string() + ".bin";
            std::ofstream outFile(binPath, std::ios::binary);
            if (outFile.is_open()) {
                uint32_t count = (uint32_t)entries.size();
                outFile.write((char*)&count, 4);
                for (const auto& e : entries) {
                    outFile.write((char*)&e.CRC, 4);
                    outFile.write((char*)&e.ID, 4);
                }
                outFile.close();
            }
        }
    }

    static void EnsureNeededBanksAreLoaded(const std::set<std::string>& neededFiles) {
        std::set<std::string> missingBanks;
        for (const auto& needed : neededFiles) {
            if (needed.find(".big") == std::string::npos && needed.find(".lut") == std::string::npos && needed.find(".lug") == std::string::npos) continue;

            bool found = false;
            for (const auto& bank : g_OpenBanks) {
                std::string bName = fs::path(bank.FullPath).filename().string();
                std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
                if (bName == needed) { found = true; break; }
            }
            if (!found) missingBanks.insert(needed);
        }

        if (missingBanks.empty()) return;
        std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
        if (!fs::exists(dataPath)) return;

        for (const auto& entry : fs::recursive_directory_iterator(dataPath)) {
            if (entry.is_regular_file()) {
                std::string fName = entry.path().filename().string();
                std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

                if (missingBanks.count(fName)) {
                    LoadBank(entry.path().string());
                    missingBanks.erase(fName);
                    if (missingBanks.empty()) break;
                }
            }
        }
    }

    static void ImplantAudioMods(LoadedBank* bank) {
        std::string lowerBankName = bank->FileName;
        std::transform(lowerBankName.begin(), lowerBankName.end(), lowerBankName.begin(), ::tolower);
        bool audioInjected = false;

        for (size_t i = 0; i < bank->Entries.size(); i++) {
            std::string entryNameLower = bank->Entries[i].Name;
            std::transform(entryNameLower.begin(), entryNameLower.end(), entryNameLower.begin(), ::tolower);
            std::string key = lowerBankName + "/N/A/" + entryNameLower;

            if (ModBankPatcher::g_ActiveModAssets.count(key)) {
                auto& asset = ModBankPatcher::g_ActiveModAssets[key];
                std::ifstream resFile(asset.ResourcePath, std::ios::binary | std::ios::ate);

                if (resFile.is_open()) {
                    std::streamsize size = resFile.tellg();
                    resFile.seekg(0, std::ios::beg);
                    std::vector<uint8_t> buffer(size);
                    if (resFile.read((char*)buffer.data(), size)) {
                        bank->ModifiedEntryData[i] = buffer;
                        bank->Entries[i].Size = (uint32_t)size;
                        audioInjected = true;
                    }
                }
                asset.IsHandled = true;
            }
        }

        if (audioInjected) {
            if (bank->LugParserPtr) bank->LugParserPtr->IsDirty = true;
            bank->IsDirty = true;
        }
    }

    static void BuildNeedListForMod(const ModEntry& mod, std::set<std::string>& outNeedList) {
        std::string modDataPath = mod.ModFolderPath + "\\Data";
        if (!fs::exists(modDataPath)) return;

        for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
            if (entry.is_directory()) {
                std::string dirName = entry.path().filename().string();
                if (dirName.size() > 4) {
                    std::string ext = dirName.substr(dirName.size() - 4);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".big" || ext == ".lut" || ext == ".lug") {
                        std::transform(dirName.begin(), dirName.end(), dirName.begin(), ::tolower);
                        outNeedList.insert(dirName);
                    }
                }
            }
            else if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".h" || ext == ".bin") {
                    std::string fName = entry.path().filename().string();
                    std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);
                    outNeedList.insert(fName);
                }
            }
        }
    }
};