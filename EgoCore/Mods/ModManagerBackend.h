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
#include "CompilerBackend.h"

namespace fs = std::filesystem;

inline int g_LaunchState = 0;

class ModManagerBackend {
public:
    static inline std::vector<ModEntry> g_LoadedMods;

    static void InitializeAndLoad() {
        g_LoadedMods.clear();
        std::string modsDir = g_AppConfig.GameRootPath + "\\Mods";

        if (!fs::exists(modsDir)) fs::create_directories(modsDir);

        // Load order now comes from the [ModOrder] section of the config file,
        // populated into g_SavedModOrder by LoadConfig().
        std::vector<std::pair<std::string, bool>>& savedOrder = g_SavedModOrder;

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

                // Reset flags before deep scan
                mod.IsAssetMod = false;
                mod.IsDefMod = false;

                // Unified deep scan for .resource, .def, and event files
                try {
                    for (const auto& file : fs::recursive_directory_iterator(mod.ModFolderPath)) {
                        if (file.is_regular_file()) {
                            std::string ext = file.path().extension().string();
                            std::string filename = file.path().filename().string();
                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

                            // Asset Mod Condition
                            if (ext == ".resource") {
                                mod.IsAssetMod = true;
                            }

                            // Def Mod Condition
                            if (ext == ".def" ||
                                filename.find("sound_animation_events") != std::string::npos ||
                                filename.find("game_animation_events") != std::string::npos) {
                                mod.IsDefMod = true;
                            }

                            // Optimization: If we found both, no need to keep scanning this mod's folders
                            if (mod.IsAssetMod && mod.IsDefMod) {
                                break;
                            }
                        }
                    }
                }
                catch (...) {}

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
        // Update the staging vector then flush everything to the config file.
        g_SavedModOrder.clear();
        for (const auto& mod : g_LoadedMods)
            g_SavedModOrder.push_back({ mod.Name, mod.IsEnabled });
        SaveConfig();
    }

    static void DeleteMod(int index) {
        if (index < 0 || index >= g_LoadedMods.size()) return;
        if (g_LoadedMods[index].IsCoreMod) return;

        bool wasAsset = g_LoadedMods[index].IsAssetMod;
        bool wasDef = g_LoadedMods[index].IsDefMod;

        std::string path = g_LoadedMods[index].ModFolderPath;
        try { if (fs::exists(path)) fs::remove_all(path); }
        catch (...) {}

        g_LoadedMods.erase(g_LoadedMods.begin() + index);
        SaveLoadOrder();
        UpdateGlobalModsIni();

        if (wasAsset) g_AppConfig.ModSystemDirty = true;
        if (wasDef) g_AppConfig.DefSystemDirty = true;
        if (wasAsset || wasDef) SaveConfig();
    }

    static void MergeDefFile(const std::string& modFile, const std::string& targetFile) {
        std::ifstream tIn(targetFile, std::ios::binary); std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>()); tIn.close();
        std::ifstream mIn(modFile, std::ios::binary); std::string mContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>()); mIn.close();

        std::string maskedTContent = CreateCommentMaskedString(tContent);
        std::string maskedMContent = CreateCommentMaskedString(mContent);

        size_t mCursor = 0;
        while (true) {
            size_t defStart = maskedMContent.find("#definition", mCursor);
            if (defStart == std::string::npos) break;
            size_t defEnd = maskedMContent.find("#end_definition", defStart);
            if (defEnd == std::string::npos) break;
            defEnd += 15;

            std::string modBlock = mContent.substr(defStart, defEnd - defStart);
            std::string headerLine = mContent.substr(defStart, mContent.find('\n', defStart) - defStart);

            std::stringstream ss(headerLine);
            std::string dummy, type, name;
            ss >> dummy >> type >> name;

            if (!type.empty() && !name.empty()) {
                // Match any whitespace between the tokens instead of assuming spaces
                std::regex defRegex(
                    "#definition[ \\t]+" + type + "[ \\t]+" + name + "[ \\t\\r\\n]"
                );

                std::smatch defMatch;
                bool found = std::regex_search(maskedTContent, defMatch, defRegex);

                if (found) {
                    size_t tStart = (size_t)defMatch.position();
                    size_t tEnd = maskedTContent.find("#end_definition", tStart);
                    if (tEnd != std::string::npos) {
                        tContent.replace(tStart, (tEnd + 15) - tStart, modBlock);
                        maskedTContent = CreateCommentMaskedString(tContent);
                    }
                }
                else {
                    tContent += "\n\n" + modBlock;
                    maskedTContent = CreateCommentMaskedString(tContent);
                }
            }
            mCursor = defEnd;
        }
        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc); out << tContent;
    }

    static void MergeHeaderFile(const std::string& modFile, const std::string& targetFile) {
        std::ifstream tIn(targetFile, std::ios::binary); std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>()); tIn.close();
        std::ifstream mIn(modFile, std::ios::binary); std::string mContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>()); mIn.close();

        std::string maskedTContent = CreateCommentMaskedString(tContent);
        std::string maskedMContent = CreateCommentMaskedString(mContent);

        std::regex enumRegex(R"(enum\s+(\w+)\s*\{([\s\S]*?)\};)");
        std::regex wordRegex(R"(([A-Za-z0-9_]+))");

        // Scan Mod file for Enums (Skips commented ones)
        auto mBegin = std::sregex_iterator(maskedMContent.begin(), maskedMContent.end(), enumRegex);
        auto mEnd = std::sregex_iterator();

        for (auto it = mBegin; it != mEnd; ++it) {
            std::string enumName = (*it)[1].str();

            size_t bodyStart = mContent.find('{', it->position()) + 1;
            size_t bodyEnd = mContent.find('}', bodyStart);
            std::string mBodyOriginal = mContent.substr(bodyStart, bodyEnd - bodyStart);

            std::regex targetEnumRegex("enum\\s+" + enumName + "\\s*\\{([\\s\\S]*?)\\};");
            std::smatch tMatch;

            if (std::regex_search(maskedTContent, tMatch, targetEnumRegex)) {
                size_t tBodyStart = tContent.find('{', tMatch.position()) + 1;
                size_t tBodyEnd = tContent.find('}', tBodyStart);
                std::string tBodyMasked = tMatch[1].str();

                std::set<std::string> existingMembers;
                std::sregex_iterator tWordBegin(tBodyMasked.begin(), tBodyMasked.end(), wordRegex);
                for (auto wit = tWordBegin; wit != mEnd; ++wit) existingMembers.insert((*wit)[1].str());

                std::stringstream ss(mBodyOriginal); std::string line; std::string toAppend = "";
                while (std::getline(ss, line)) {
                    // Ghost line check prevents injecting commented-out members
                    std::string maskedLine = CreateCommentMaskedString(line);
                    std::smatch lineMatch;

                    if (std::regex_search(maskedLine, lineMatch, wordRegex)) {
                        std::string word = lineMatch[1].str();
                        if (word != "force_dword" && word != "FORCE_DWORD") {
                            if (existingMembers.find(word) == existingMembers.end()) {
                                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                                toAppend += "    " + line + "\n";
                                existingMembers.insert(word);
                            }
                        }
                    }
                }
                if (!toAppend.empty()) {
                    tContent.insert(tBodyEnd, toAppend);
                    maskedTContent = CreateCommentMaskedString(tContent);
                }
            }
            else {
                size_t enumStart = it->position();
                size_t enumLength = it->length();
                tContent += "\n" + mContent.substr(enumStart, enumLength) + "\n";
                maskedTContent = CreateCommentMaskedString(tContent);
            }
        }
        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc);
        out << tContent;
        out.close();
    }

    static void MergeEventFile(const std::string& modFile, const std::string& targetFile) {
        std::ifstream tIn(targetFile, std::ios::binary); std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>()); tIn.close();
        std::ifstream mIn(modFile, std::ios::binary); std::string mContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>()); mIn.close();

        size_t mCursor = 0;
        while (true) {
            size_t evStart = mContent.find("BEGIN_EVENTS:", mCursor);
            if (evStart == std::string::npos) break;
            size_t evEnd = mContent.find("END_EVENTS", evStart);
            if (evEnd == std::string::npos) break;
            evEnd += 10;

            std::string modBlock = mContent.substr(evStart, evEnd - evStart);
            std::string headerLine = mContent.substr(evStart, mContent.find('\n', evStart) - evStart);

            std::string name = headerLine.substr(13);
            name.erase(0, name.find_first_not_of(" \t\r")); name.erase(name.find_last_not_of(" \t\r") + 1);

            if (!name.empty()) {
                std::string searchStr = "BEGIN_EVENTS: " + name;

                // Exact word-boundary matching for events
                size_t tStart = 0;
                bool found = false;
                while ((tStart = tContent.find(searchStr, tStart)) != std::string::npos) {
                    size_t nextIdx = tStart + searchStr.length();
                    if (nextIdx >= tContent.length() || std::isspace((unsigned char)tContent[nextIdx])) {
                        found = true;
                        break;
                    }
                    tStart += searchStr.length();
                }

                if (found) {
                    size_t tEnd = tContent.find("END_EVENTS", tStart);
                    if (tEnd != std::string::npos) {
                        tContent.replace(tStart, (tEnd + 10) - tStart, modBlock);
                    }
                }
                else {
                    size_t insertPos = tContent.rfind("END_ANIMATION_EVENTS");
                    if (insertPos != std::string::npos) tContent.insert(insertPos, modBlock + "\n\n");
                    else tContent += "\n" + modBlock;
                }
            }
            mCursor = evEnd;
        }
        std::ofstream out(targetFile, std::ios::binary | std::ios::trunc); out << tContent;
    }

    static void ProcessModsAndLaunch() {

        bool hasActiveAssetMods = false;
        bool hasActiveDefMods = false;
        std::set<std::string> neededBankFiles;

        for (const auto& mod : g_LoadedMods) {
            if (!mod.IsEnabled) continue;
            if (mod.IsAssetMod) {
                hasActiveAssetMods = true;
                BuildNeedListForMod(mod, neededBankFiles);
            }
            if (mod.IsDefMod) {
                hasActiveDefMods = true;
            }
        }

        if (!hasActiveAssetMods && !hasActiveDefMods) {

            bool wasBankDirty = g_AppConfig.ModSystemDirty;
            bool wasDefDirty = g_AppConfig.DefSystemDirty;

            RestoreAllTmpBackups();

            g_AppConfig.ModSystemDirty = false;
            g_AppConfig.DefSystemDirty = false;
            SaveConfig();

            if (wasDefDirty) {
                g_PendingGameLaunch = true;
                CompileAllDefs_Stealth();
                return;
            }

            LaunchGame();
            return;
        }

        if (!g_AppConfig.ModSystemDirty && !g_AppConfig.DefSystemDirty) {
            LaunchGame();
            return;
        }

        for (auto& bank : g_OpenBanks) {
            if (bank.Stream && bank.Stream->is_open()) bank.Stream->close();
        }

        ModBankPatcher::BuildMasterModIndex(g_LoadedMods);

        RestoreVanillaFiles(g_AppConfig.ModSystemDirty, g_AppConfig.DefSystemDirty);
        if (g_AppConfig.ModSystemDirty) {
            BackupNeededBankFiles(neededBankFiles);
        }
        if (g_AppConfig.DefSystemDirty) {
            BackupAffectedDefFiles();
        }
        if (g_AppConfig.ModSystemDirty && hasActiveAssetMods) {
            EnsureNeededBanksAreLoaded(neededBankFiles);
            LoadHeadersFromDir(g_AppConfig.GameRootPath);

            for (auto& bank : g_OpenBanks) {
                std::string bankFileName = fs::path(bank.FullPath).filename().string();
                std::transform(bankFileName.begin(), bankFileName.end(), bankFileName.begin(), ::tolower);

                if (neededBankFiles.find(bankFileName) != neededBankFiles.end()) {
                    if (bank.Type == EBankType::Audio) {
                        bank.ModifiedEntryData.clear();
                        ImplantAudioMods(&bank);
                        if (bank.IsDirty) { SaveAudioBank(&bank); bank.IsDirty = false; }
                    }
                    else {
                        if (ModBankPatcher::PatchBankForLaunch(&bank)) ReloadBankInPlace(&bank);
                    }
                }
                else {
                    ReloadBankInPlace(&bank);
                }
            }
        }

        g_AppConfig.ModSystemDirty = false;

        if (g_AppConfig.DefSystemDirty && hasActiveDefMods) {
            CompileAudioBinFiles();

            std::string dataPath = g_AppConfig.GameRootPath + "\\Data";

            for (const auto& mod : g_LoadedMods) {
                if (!mod.IsEnabled)  continue;
                if (!mod.IsDefMod)   continue;

                std::string modDataPath = mod.ModFolderPath + "\\Data";
                if (!fs::exists(modDataPath)) continue;

                for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                    if (!entry.is_regular_file()) continue;

                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    if (ext != ".def" && ext != ".txt" && ext != ".h" && ext != ".bin") continue;

                    std::string relPath = entry.path().string().substr(modDataPath.length());
                    std::string targetPath = dataPath + relPath;

                    fs::create_directories(fs::path(targetPath).parent_path());

                    if (!fs::exists(targetPath)) {
                        fs::copy_file(entry.path(), targetPath);
                        continue;
                    }
                    if (ext == ".def") MergeDefFile(entry.path().string(), targetPath);
                    else if (ext == ".txt") MergeEventFile(entry.path().string(), targetPath);
                    else if (ext == ".h")   MergeHeaderFile(entry.path().string(), targetPath);
                    else if (ext == ".bin") fs::copy_file(entry.path(), targetPath,
                        fs::copy_options::overwrite_existing);
                }
            }

            g_AppConfig.DefSystemDirty = false;
            SaveConfig();

            g_PendingGameLaunch = true;
            CompileAllDefs_Stealth();
            return;
        }
        g_AppConfig.DefSystemDirty = false;
        SaveConfig();
        LaunchGame();
    }

    static void RestoreVanillaFiles(bool restoreBanks, bool restoreDefs) {
        std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
        if (!fs::exists(dataPath)) return;

        for (const auto& entry : fs::recursive_directory_iterator(dataPath)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".tmp") continue;

            std::string tmpPath = entry.path().string();
            std::string originalPath = tmpPath.substr(0, tmpPath.length() - 4);
            std::string origExt = fs::path(originalPath).extension().string();
            std::transform(origExt.begin(), origExt.end(), origExt.begin(), ::tolower);

            bool isBank = (origExt == ".big" || origExt == ".lut" || origExt == ".lug");
            bool isDef = (origExt == ".def" || origExt == ".txt" || origExt == ".h" || origExt == ".bin");

            if ((restoreBanks && isBank) || (restoreDefs && isDef)) {
                try {
                    if (fs::exists(originalPath)) fs::remove(originalPath);
                    fs::rename(tmpPath, originalPath);
                }
                catch (...) {}
            }
        }
    }

    static void RestoreAllTmpBackups() {
        RestoreVanillaFiles(true, true);
    }

    static void BackupNeededBankFiles(const std::set<std::string>& neededBankFileNames) {
        std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
        if (!fs::exists(dataPath)) return;

        for (const auto& entry : fs::recursive_directory_iterator(dataPath)) {
            if (!entry.is_regular_file()) continue;

            std::string fName = entry.path().filename().string();
            std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);

            if (neededBankFileNames.count(fName)) {
                std::string tmpPath = entry.path().string() + ".tmp";
                if (!fs::exists(tmpPath)) {
                    try { fs::copy_file(entry.path(), tmpPath); }
                    catch (...) {}
                }
            }
        }
    }

    static void BackupAffectedDefFiles() {
        std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
        if (!fs::exists(dataPath)) return;

        for (const auto& mod : g_LoadedMods) {
            if (!mod.IsEnabled || !mod.IsDefMod) continue;

            std::string modDataPath = mod.ModFolderPath + "\\Data";
            if (!fs::exists(modDataPath)) continue;

            for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                if (!entry.is_regular_file()) continue;

                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".def" && ext != ".txt" && ext != ".h" && ext != ".bin") continue;

                std::string relPath = entry.path().string().substr(modDataPath.length());
                std::string targetPath = dataPath + relPath;

                // Only backup files that actually exist in the game directory.
                // New files (vanilla doesn't have them) don't need a backup.
                if (!fs::exists(targetPath)) continue;

                std::string tmpPath = targetPath + ".tmp";
                if (!fs::exists(tmpPath)) {
                    try { fs::copy_file(targetPath, tmpPath); }
                    catch (...) {}
                }
            }
        }
    }

private:
    static void LaunchGame() {
        std::string exePath = g_AppConfig.GameRootPath + "\\FableLauncher.exe";
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, g_AppConfig.GameRootPath.c_str(), SW_SHOWDEFAULT);
        exit(0);
    }

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