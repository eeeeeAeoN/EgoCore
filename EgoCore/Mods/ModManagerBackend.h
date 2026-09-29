#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <set>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <shellapi.h>
#include "ModManagerCompiler.h"
#include "BankEditor.h"
#include "CompilerBackend.h"
#include "TngMerger.h"

namespace fs = std::filesystem;

inline int g_LaunchState = 0;
inline bool g_ShowFallbackLaunchPopup = false;

// --- Launch (ProcessModsAndLaunch) background state ---
// Patching banks, merging defs, and restoring/backing up files can take a
// noticeable moment on disk. This runs on a worker thread so the "Launching
// Fable" popup can keep animating instead of freezing the UI.
inline std::atomic<bool> g_IsProcessingLaunch = false;
inline std::mutex g_LaunchStatusMutex;
inline std::string g_LaunchStatus = "Preparing to launch Fable...";

inline void SetLaunchStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(g_LaunchStatusMutex);
    g_LaunchStatus = status;
}

inline std::string GetLaunchStatus() {
    std::lock_guard<std::mutex> lock(g_LaunchStatusMutex);
    return g_LaunchStatus;
}

class ModManagerBackend {
public:
    static inline std::vector<ModEntry> g_LoadedMods;

    static void InitializeAndLoad() {
        g_LoadedMods.clear();
        std::string modsDir = g_AppConfig.GameRootPath + "\\Mods";

        if (!fs::exists(modsDir)) fs::create_directories(modsDir);

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
                mod.IsTngMod = false;
                mod.IsFSEMod = false;

                // FSE detection: simple folder check (must have "FSE" directory)
                if (fs::exists(mod.ModFolderPath + "\\FSE") && fs::is_directory(mod.ModFolderPath + "\\FSE")) {
                    mod.IsFSEMod = true;
                }

                try {
                    for (const auto& file : fs::recursive_directory_iterator(mod.ModFolderPath)) {
                        if (file.is_regular_file()) {
                            std::string ext = file.path().extension().string();
                            std::string filename = file.path().filename().string();
                            std::string relPath = file.path().string().substr(mod.ModFolderPath.length());

                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

                            std::string lowerRelPath = relPath;
                            std::transform(lowerRelPath.begin(), lowerRelPath.end(), lowerRelPath.begin(), ::tolower);

                            // Asset Mod Condition (Binary Banks)
                            if (ext == ".resource") {
                                mod.IsAssetMod = true;
                            }

                            // Def Mod Condition
                            if (ext == ".def" ||
                                filename.find("sound_animation_events") != std::string::npos ||
                                filename.find("game_animation_events") != std::string::npos) {
                                mod.IsDefMod = true;
                            }

                            // Tng Mod Condition
                            if (ext == ".tng") {
                                mod.IsTngMod = true;
                            }

                            // Loose Asset Mod Condition
                            if (!mod.IsAssetMod) {
                                if (lowerRelPath.find("\\data\\bones\\") != std::string::npos ||
                                    lowerRelPath.find("\\data\\enginecache\\") != std::string::npos ||
                                    (lowerRelPath.find("\\data\\levels\\") != std::string::npos && ext != ".tng") ||
                                    lowerRelPath.find("\\data\\lightingtable\\") != std::string::npos ||
                                    lowerRelPath.find("\\data\\video\\") != std::string::npos ||
                                    lowerRelPath.find("\\data\\tattoos\\") != std::string::npos) {
                                    mod.IsAssetMod = true;
                                }
                                else if (lowerRelPath.find("\\data\\misc\\") != std::string::npos &&
                                    lowerRelPath.find("\\data\\misc\\pc\\") == std::string::npos) {
                                    if (filename != "game_animation_events.txt" && filename != "sound_animation_events.txt" &&
                                        filename != "game_animation_events.bin" && filename != "sound_animation_events.bin" &&
                                        filename != "stars.dat") {
                                        mod.IsAssetMod = true;
                                    }
                                }
                                else if (lowerRelPath.find("\\data\\sound\\") != std::string::npos) {
                                    if (ext != ".lug" && ext != ".met") mod.IsAssetMod = true;
                                }
                                else if (lowerRelPath.find("\\") == lowerRelPath.find_last_of("\\")) {
                                    if (filename == "user.ini" || filename == "userst.ini" ||
                                        filename == "dbug.ini" || filename == "dbust.ini") {
                                        mod.IsAssetMod = true;
                                    }
                                }
                            }

                            // Optimization: Break early only if all three are found
                            if (mod.IsAssetMod && mod.IsDefMod && mod.IsTngMod) {
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
        std::string modsIniPath = g_AppConfig.GameRootPath + "\\Mods.ini";
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
        std::string modsIniPath = g_AppConfig.GameRootPath + "\\Mods.ini";
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
        bool wasTng = g_LoadedMods[index].IsTngMod;
        bool wasFSE = g_LoadedMods[index].IsFSEMod;

        std::string path = g_LoadedMods[index].ModFolderPath;
        try { if (fs::exists(path)) fs::remove_all(path); }
        catch (...) {}

        g_LoadedMods.erase(g_LoadedMods.begin() + index);
        SaveLoadOrder();
        UpdateGlobalModsIni();

        if (wasAsset) g_AppConfig.ModSystemDirty = true;
        if (wasDef) g_AppConfig.DefSystemDirty = true;
        if (wasTng) g_AppConfig.TngSystemDirty = true;
        if (wasFSE) g_AppConfig.FSESystemDirty = true;
        if (wasAsset || wasDef || wasTng || wasFSE) SaveConfig();
    }

    static void MergeDefFile(const std::string& modFile, const std::string& targetFile) {
        std::ifstream tIn(targetFile, std::ios::binary); std::string tContent((std::istreambuf_iterator<char>(tIn)), std::istreambuf_iterator<char>()); tIn.close();
        std::ifstream mIn(modFile, std::ios::binary); std::string mContent((std::istreambuf_iterator<char>(mIn)), std::istreambuf_iterator<char>()); mIn.close();

        std::string maskedTContent = CreateCommentMaskedString(tContent);
        std::string maskedMContent = CreateCommentMaskedString(mContent);

        size_t mCursor = 0;
        while (true) {
            // Catches both "#definition" and "#definition_template"
            size_t defStart = maskedMContent.find("#definition", mCursor);
            if (defStart == std::string::npos) break;

            size_t defEnd = maskedMContent.find("#end_definition", defStart);
            if (defEnd == std::string::npos) break;
            defEnd += 15;

            std::string modBlock = mContent.substr(defStart, defEnd - defStart);
            std::string headerLine = mContent.substr(defStart, mContent.find('\n', defStart) - defStart);

            headerLine.erase(std::remove(headerLine.begin(), headerLine.end(), '\r'), headerLine.end());

            std::stringstream ss(headerLine);
            std::string dummy, type, name;
            ss >> dummy >> type >> name;

            if (!type.empty() && !name.empty()) {
                std::regex defRegex(
                    "#definition(?:_template)?[ \\t]+" + type + "[ \\t]+" + name + "[ \\t\\r\\n]"
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

        // --- Merge enum blocks (full block replacement for .def enums) ---
        std::regex enumRegex(R"(enum\s+(\w+)\s*\{[\s\S]*?\};)");
        auto mEnumBegin = std::sregex_iterator(maskedMContent.begin(), maskedMContent.end(), enumRegex);
        auto mEnumEnd = std::sregex_iterator();

        for (auto it = mEnumBegin; it != mEnumEnd; ++it) {
            std::string enumName = (*it)[1].str();
            size_t modEnumStart = it->position();
            size_t modEnumLen = it->length();

            // Use original (unmasked) content so comments inside the enum are preserved
            std::string modEnumBlock = mContent.substr(modEnumStart, modEnumLen);

            // Search target for an enum with the same name
            std::regex targetEnumRegex("enum\\s+" + enumName + "\\s*\\{[\\s\\S]*?\\};");
            std::smatch tMatch;

            if (std::regex_search(maskedTContent, tMatch, targetEnumRegex)) {
                // Replace the entire enum block in the target
                tContent.replace(tMatch.position(), tMatch.length(), modEnumBlock);
                maskedTContent = CreateCommentMaskedString(tContent);
            }
            else {
                // New enum: insert before the first #definition, or append at end
                size_t insertPos = maskedTContent.find("#definition");
                if (insertPos != std::string::npos) {
                    tContent.insert(insertPos, modEnumBlock + "\n\n");
                }
                else {
                    tContent += "\n\n" + modEnumBlock;
                }
                maskedTContent = CreateCommentMaskedString(tContent);
            }
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
                    std::string maskedLine = CreateCommentMaskedString(line);
                    std::smatch lineMatch;

                    if (std::regex_search(maskedLine, lineMatch, wordRegex)) {
                        std::string word = lineMatch[1].str();
                        if (word != "force_dword" && word != "FORCE_DWORD") {
                            if (existingMembers.find(word) == existingMembers.end()) {

                                uint32_t finalID = 0;
                                bool useLiveID = false;

                                std::string upperWord = word;
                                std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);

                                if (g_LiveBankIDs.count(word)) {
                                    finalID = g_LiveBankIDs[word];
                                    useLiveID = true;
                                }
                                else if (g_LiveBankIDs.count(upperWord)) {
                                    finalID = g_LiveBankIDs[upperWord];
                                    useLiveID = true;
                                }

                                if (useLiveID) {
                                    // Override the modder's text with the true dynamic ID
                                    toAppend += "    " + word + " = " + std::to_string(finalID) + ",\n";
                                }
                                else {
                                    // Fallback for non-bank enums (like animation events)
                                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                                    toAppend += "    " + line + "\n";
                                }

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

    // ========================================================================
    //  FSE MOD INSTALLATION HELPERS
    // ========================================================================

    struct ModParsedQuest {
        std::string Name;
        std::string File;
        std::vector<FSEEntity> Entities;
    };

    // Robust parser for a mod's quests.lua
    static std::vector<ModParsedQuest> ParseModQuestsLua(const std::string& filePath) {
        std::vector<ModParsedQuest> result;
        if (!fs::exists(filePath)) return result;

        std::ifstream file(filePath);
        if (!file.is_open()) return result;

        std::string line;
        ModParsedQuest currentQuest;
        bool inQuest = false;
        bool inEntities = false;

        std::regex questStartRegex(R"(([a-zA-Z0-9_]+)\s*=\s*\{)");
        std::regex propNameRegex(R"(name\s*=\s*["']([^"']+)["'])");
        std::regex propFileRegex(R"(file\s*=\s*["']([^"']+)["'])");
        std::regex entityScriptsStartRegex(R"(entity_scripts\s*=\s*\{)");
        std::regex entityBlockRegex(R"(\{([^}]+)\})");

        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // Strip Lua comments (-- ...)
            size_t commentPos = line.find("--");
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
            if (trimmed.empty()) continue;

            if (!inQuest) {
                if (trimmed.find("Quests") != std::string::npos && trimmed.find('=') != std::string::npos) continue;
                std::smatch match;
                if (std::regex_search(line, match, questStartRegex)) {
                    inQuest = true;
                    inEntities = false;
                    currentQuest = ModParsedQuest();
                    currentQuest.Name = match[1].str();
                }
            }
            else if (!inEntities) {
                std::smatch match;
                if (std::regex_search(line, match, propNameRegex)) {
                    currentQuest.Name = match[1].str();
                }
                else if (std::regex_search(line, match, propFileRegex)) {
                    currentQuest.File = match[1].str();
                }
                else if (std::regex_search(line, match, entityScriptsStartRegex)) {
                    // Check if closing brace is on the same line (e.g. entity_scripts = {})
                    size_t bracePos = line.find('{');
                    size_t closeBracePos = line.find('}', bracePos);
                    if (closeBracePos != std::string::npos) {
                        std::string inside = line.substr(bracePos + 1, closeBracePos - bracePos - 1);
                        std::sregex_iterator it(inside.begin(), inside.end(), entityBlockRegex);
                        std::sregex_iterator end;
                        for (; it != end; ++it) {
                            std::string entContent = (*it)[1].str();
                            std::smatch eNameMatch, eFileMatch;
                            if (std::regex_search(entContent, eNameMatch, propNameRegex) &&
                                std::regex_search(entContent, eFileMatch, propFileRegex)) {
                                FSEEntity ent;
                                ent.Name = eNameMatch[1].str();
                                ent.File = eFileMatch[1].str();
                                ent.ID = 0;
                                currentQuest.Entities.push_back(ent);
                            }
                        }
                    }
                    else {
                        inEntities = true;
                    }
                }
                else if (trimmed == "}," || trimmed == "}" || trimmed.find('}') == 0) {
                    if (!currentQuest.Name.empty()) {
                        result.push_back(currentQuest);
                    }
                    inQuest = false;
                }
            }
            else { // inEntities == true
                std::sregex_iterator it(line.begin(), line.end(), entityBlockRegex);
                std::sregex_iterator end;
                for (; it != end; ++it) {
                    std::string entContent = (*it)[1].str();
                    std::smatch eNameMatch, eFileMatch;
                    if (std::regex_search(entContent, eNameMatch, propNameRegex) &&
                        std::regex_search(entContent, eFileMatch, propFileRegex)) {
                        FSEEntity ent;
                        ent.Name = eNameMatch[1].str();
                        ent.File = eFileMatch[1].str();
                        ent.ID = 0;
                        currentQuest.Entities.push_back(ent);
                    }
                }

                if (trimmed == "}" || trimmed == "}," || trimmed.find('}') != std::string::npos) {
                    inEntities = false;
                }
            }
        }

        if (inQuest && !currentQuest.Name.empty()) {
            result.push_back(currentQuest);
        }

        return result;
    }

    // Merge parsed quests from a mod into the workspace quests.
    // Higher-priority mods run first in forward pass, so claimedQuestNames prevents lower-priority overrides.
    static void PatchQuestsLua(const ModEntry& mod, std::set<std::string>& claimedQuestNames) {
        std::string modQuestsFile = mod.ModFolderPath + "\\FSE\\quests.lua";
        if (!fs::exists(modQuestsFile)) return;

        auto modQuests = ParseModQuestsLua(modQuestsFile);

        for (const auto& mq : modQuests) {
            // Never overwrite FSE_Master
            if (mq.Name == "FSE_Master") continue;

            // If a higher-priority mod already claimed this quest name, skip it
            if (claimedQuestNames.count(mq.Name)) continue;
            claimedQuestNames.insert(mq.Name);

            // Check if quest already exists in the workspace (e.g. vanilla quest like MazeResearch)
            bool found = false;
            for (auto& existing : g_FSEWorkspace.Quests) {
                if (existing.Name == mq.Name) {
                    existing.File = mq.File;
                    existing.Entities = mq.Entities;
                    found = true;
                    break;
                }
            }

            if (!found) {
                FSEQuest newQuest;
                newQuest.Name = mq.Name;
                newQuest.File = mq.File;
                newQuest.ID = 0;
                newQuest.Entities = mq.Entities;
                g_FSEWorkspace.Quests.push_back(newQuest);
            }
        }
    }

    // Reassign all quest and entity IDs according to specifications:
    // FSE_Master = 1000.
    // Quest IDs start at 1001, sequential.
    // Entity IDs start at 1, sequential across all quests.
    static void FinalizeQuestsLuaIDsAndSave() {
        int nextQuestID = 1001;
        int nextEntityID = 1;
        for (auto& q : g_FSEWorkspace.Quests) {
            if (q.Name == "FSE_Master") {
                q.ID = 1000;
            } else {
                q.ID = nextQuestID++;
            }
            for (auto& e : q.Entities) {
                e.ID = nextEntityID++;
            }
        }
        SaveQuestsLua();
    }

    // Track deployed script folders for clean restoration
    static inline std::set<std::string> g_DeployedFSEScriptFolders;

    // Copy script folders from [Mod]/FSE into [Fable]/FSE.
    // Called in reverse priority order (lowest first, highest overwrites).
    static void DeployFSEScriptFolders(const ModEntry& mod) {
        std::string modFSEPath = mod.ModFolderPath + "\\FSE";
        std::string gameFSEPath = g_AppConfig.GameRootPath + "\\FSE";

        if (!fs::exists(modFSEPath)) return;

        for (const auto& entry : fs::directory_iterator(modFSEPath)) {
            if (!entry.is_directory()) continue;

            std::string folderName = entry.path().filename().string();
            if (folderName == "Master") continue; // Safeguard vanilla Master folder

            g_DeployedFSEScriptFolders.insert(folderName);

            // Record in persistent manifest file inside [Fable]/FSE/
            std::string manifestPath = gameFSEPath + "\\.egocore_deployed_folders.txt";
            try {
                std::ofstream mf(manifestPath, std::ios::app);
                if (mf.is_open()) {
                    mf << folderName << "\n";
                }
            }
            catch (...) {}

            std::string targetDir = gameFSEPath + "\\" + folderName;
            try {
                fs::copy(entry.path(), targetDir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            catch (...) {}
        }
    }

    // Patch text files (FinalAlbion.qst from FSE.qst, user.ini from FSEuser.ini).
    // Supports [Delete] and [Add] sections.
    static void PatchTextFile(const std::string& patchFilePath, const std::string& targetFilePath) {
        if (!fs::exists(patchFilePath) || !fs::exists(targetFilePath)) return;

        std::vector<std::string> deleteLines;
        std::vector<std::string> addLines;

        {
            std::ifstream patchFile(patchFilePath);
            std::string line;
            enum { None, Delete, Add } section = None;
            bool firstLine = true;

            while (std::getline(patchFile, line)) {
                if (firstLine) {
                    firstLine = false;
                    if (line.size() >= 3 &&
                        (unsigned char)line[0] == 0xEF &&
                        (unsigned char)line[1] == 0xBB &&
                        (unsigned char)line[2] == 0xBF) {
                        line = line.substr(3);
                    }
                }
                if (!line.empty() && line.back() == '\r') line.pop_back();

                std::string trimmed = line;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

                std::string lowerTrimmed = trimmed;
                std::transform(lowerTrimmed.begin(), lowerTrimmed.end(), lowerTrimmed.begin(), ::tolower);

                if (lowerTrimmed == "[delete]") { section = Delete; continue; }
                if (lowerTrimmed == "[add]") { section = Add; continue; }
                if (trimmed.empty()) continue;

                if (section == Delete) deleteLines.push_back(trimmed);
                else if (section == Add) addLines.push_back(line);
            }
        }

        // Read target file lines
        std::vector<std::string> targetLines;
        {
            std::ifstream targetFile(targetFilePath);
            std::string line;
            bool firstLine = true;
            while (std::getline(targetFile, line)) {
                if (firstLine) {
                    firstLine = false;
                    if (line.size() >= 3 &&
                        (unsigned char)line[0] == 0xEF &&
                        (unsigned char)line[1] == 0xBB &&
                        (unsigned char)line[2] == 0xBF) {
                        line = line.substr(3);
                    }
                }
                if (!line.empty() && line.back() == '\r') line.pop_back();
                targetLines.push_back(line);
            }
        }

        // Helper to normalize whitespace and optional trailing semicolon
        auto normalizeLine = [](const std::string& str) -> std::string {
            std::string res;
            bool inSpace = false;
            size_t start = str.find_first_not_of(" \t");
            if (start == std::string::npos) return "";
            size_t end = str.find_last_not_of(" \t");
            for (size_t i = start; i <= end; ++i) {
                char c = str[i];
                if (c == ' ' || c == '\t') {
                    if (!inSpace) { res += ' '; inSpace = true; }
                } else {
                    res += c;
                    inSpace = false;
                }
            }
            if (!res.empty() && res.back() == ';') res.pop_back();
            res.erase(res.find_last_not_of(" \t") + 1);
            return res;
        };

        // Apply deletions
        if (!deleteLines.empty()) {
            std::vector<std::string> filteredLines;
            for (const auto& tLine : targetLines) {
                std::string trimmedTarget = tLine;
                trimmedTarget.erase(0, trimmedTarget.find_first_not_of(" \t"));
                trimmedTarget.erase(trimmedTarget.find_last_not_of(" \t") + 1);
                std::string normTarget = normalizeLine(tLine);

                bool shouldDelete = false;
                for (const auto& dLine : deleteLines) {
                    if (trimmedTarget == dLine || normTarget == normalizeLine(dLine)) {
                        shouldDelete = true;
                        break;
                    }
                }
                if (!shouldDelete) filteredLines.push_back(tLine);
            }
            targetLines = filteredLines;
        }

        // Append [Add] lines (avoid exact duplicates)
        for (const auto& aLine : addLines) {
            std::string trimmedAdd = aLine;
            trimmedAdd.erase(0, trimmedAdd.find_first_not_of(" \t"));
            trimmedAdd.erase(trimmedAdd.find_last_not_of(" \t") + 1);
            if (trimmedAdd.empty()) continue;

            bool alreadyExists = false;
            for (const auto& existing : targetLines) {
                std::string trimmedExist = existing;
                trimmedExist.erase(0, trimmedExist.find_first_not_of(" \t"));
                trimmedExist.erase(trimmedExist.find_last_not_of(" \t") + 1);
                if (trimmedExist == trimmedAdd) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                targetLines.push_back(aLine);
            }
        }

        // Write back with CRLF line endings
        std::ofstream out(targetFilePath, std::ios::trunc | std::ios::binary);
        for (const auto& l : targetLines) {
            out << l << "\r\n";
        }
    }

    // Backup the three files affected by FSE mods:
    //   [Fable]/FSE/quests.lua  ->  quests.lua.tmp
    //   [Fable]/Data/Levels/FinalAlbion.qst  ->  FinalAlbion.qst.tmp
    //   [Fable]/user.ini  ->  user.ini.tmp
    static void BackupFSEFiles() {
        auto backupOne = [](const std::string& path) {
            if (!fs::exists(path)) return;
            std::string tmpPath = path + ".tmp";
            if (!fs::exists(tmpPath)) {
                try { fs::copy_file(path, tmpPath); }
                catch (...) {}
            }
        };

        backupOne(g_AppConfig.GameRootPath + "\\FSE\\quests.lua");
        backupOne(g_AppConfig.GameRootPath + "\\Data\\Levels\\FinalAlbion.qst");
        backupOne(g_AppConfig.GameRootPath + "\\user.ini");
    }

    // Restore the three FSE-affected files from .tmp backups, and delete
    // any script folders that were deployed by FSE mods.
    static void RestoreFSEFiles() {
        auto restoreOne = [](const std::string& path) {
            std::string tmpPath = path + ".tmp";
            if (!fs::exists(tmpPath)) return;
            try {
                if (fs::exists(path)) fs::remove(path);
                fs::copy_file(tmpPath, path);
                fs::remove(tmpPath);
            }
            catch (...) {}
        };

        restoreOne(g_AppConfig.GameRootPath + "\\FSE\\quests.lua");
        restoreOne(g_AppConfig.GameRootPath + "\\Data\\Levels\\FinalAlbion.qst");
        restoreOne(g_AppConfig.GameRootPath + "\\user.ini");

        // Delete deployed script folders from [Fable]/FSE/
        std::string gameFSEPath = g_AppConfig.GameRootPath + "\\FSE";
        std::string manifestPath = gameFSEPath + "\\.egocore_deployed_folders.txt";

        std::set<std::string> foldersToDelete = g_DeployedFSEScriptFolders;

        if (fs::exists(manifestPath)) {
            std::ifstream mf(manifestPath);
            std::string folder;
            while (std::getline(mf, folder)) {
                if (!folder.empty() && folder.back() == '\r') folder.pop_back();
                if (!folder.empty()) foldersToDelete.insert(folder);
            }
            mf.close();
            try { fs::remove(manifestPath); } catch (...) {}
        }

        for (const auto& mod : g_LoadedMods) {
            if (!mod.IsFSEMod) continue;
            std::string modFSEPath = mod.ModFolderPath + "\\FSE";
            if (fs::exists(modFSEPath)) {
                for (const auto& entry : fs::directory_iterator(modFSEPath)) {
                    if (entry.is_directory()) foldersToDelete.insert(entry.path().filename().string());
                }
            }
        }

        for (const auto& folderName : foldersToDelete) {
            if (folderName == "Master") continue; // Never delete vanilla Master folder
            std::string deployedPath = gameFSEPath + "\\" + folderName;
            try {
                if (fs::exists(deployedPath)) fs::remove_all(deployedPath);
            }
            catch (...) {}
        }
        g_DeployedFSEScriptFolders.clear();
    }

    static void ProcessModsAndLaunch() {
        SetLaunchStatus("Reading mod load order...");

        // FIX: Force mod state loading from disk before processing.
        // If launching directly from cold boot, g_LoadedMods is empty and would 
        // otherwise trigger RestoreAllTmpBackups() and wipe active asset mods.
        ModManagerBackend::InitializeAndLoad();

        for (auto& bank : g_OpenBanks) {
            if (bank.Stream && bank.Stream->is_open()) bank.Stream->close();
        }

        bool hasActiveAssetMods = false;
        bool hasActiveDefMods = false;
        bool hasActiveTngMods = false;
        bool hasActiveFSEMods = false;
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
            if (mod.IsTngMod) {
                hasActiveTngMods = true;
            }
            if (mod.IsFSEMod) {
                hasActiveFSEMods = true;
            }
        }

        if (!hasActiveAssetMods && !hasActiveDefMods && !hasActiveTngMods && !hasActiveFSEMods) {
            bool wasBankDirty = g_AppConfig.ModSystemDirty;
            bool wasDefDirty = g_AppConfig.DefSystemDirty;
            bool wasTngDirty = g_AppConfig.TngSystemDirty;
            bool wasFSEDirty = g_AppConfig.FSESystemDirty;

            RestoreAllTmpBackups();
            if (wasFSEDirty) RestoreFSEFiles();

            g_AppConfig.ModSystemDirty = false;
            g_AppConfig.DefSystemDirty = false;
            g_AppConfig.TngSystemDirty = false;
            g_AppConfig.FSESystemDirty = false;
            SaveConfig();

            if (wasDefDirty) {
                g_PendingGameLaunch = true;
                CompileAllDefs_Native();
                return;
            }

            LaunchGame();
            return;
        }

        if (!g_AppConfig.ModSystemDirty && !g_AppConfig.DefSystemDirty && !g_AppConfig.TngSystemDirty && !g_AppConfig.FSESystemDirty) {
            LaunchGame();
            return;
        }

        ModBankPatcher::BuildMasterModIndex(g_LoadedMods);

        SetLaunchStatus("Restoring vanilla files...");
        RestoreVanillaFiles(g_AppConfig.ModSystemDirty, g_AppConfig.DefSystemDirty, g_AppConfig.TngSystemDirty);

        SetLaunchStatus("Backing up affected files...");
        if (g_AppConfig.ModSystemDirty) BackupNeededBankFiles(neededBankFiles);
        if (g_AppConfig.DefSystemDirty) BackupAffectedDefFiles();
        if (g_AppConfig.TngSystemDirty) BackupAffectedTngFiles();

        if (g_AppConfig.ModSystemDirty && hasActiveAssetMods) {
            SetLaunchStatus("Patching banks...");

            if (neededBankFiles.count("effects.big")) {
                neededBankFiles.insert("graphics.big");
                neededBankFiles.insert("textures.big");
            }
            if (neededBankFiles.count("graphics.big")) {
                neededBankFiles.insert("textures.big");
            }
            if (neededBankFiles.count("xboxeffects.big")) {
                neededBankFiles.insert("xboxgraphics.big");
            }
            if (neededBankFiles.count("xboxgraphics.big")) {
                neededBankFiles.insert("textures.big");
            }

            EnsureNeededBanksAreLoaded(neededBankFiles);
            LoadHeadersFromDir(g_AppConfig.GameRootPath);

            // 2. SORT BANKS: Force Textures -> Graphics -> Effects
            std::sort(g_OpenBanks.begin(), g_OpenBanks.end(), [](const LoadedBank& a, const LoadedBank& b) {
                auto getOrder = [](const std::string& name) {
                    std::string n = name; std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    if (n.find("textures") != std::string::npos) return 0;
                    if (n.find("graphics") != std::string::npos) return 1;
                    if (n.find("effects") != std::string::npos) return 2;
                    return 3;
                    };
                int orderA = getOrder(a.FileName);
                int orderB = getOrder(b.FileName);
                if (orderA != orderB) return orderA < orderB;
                return a.FileName < b.FileName;
                });

            g_LiveBankIDs.clear();

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
            SetLaunchStatus("Deploying loose asset mods...");
            DeployLooseAssetMods();
        }
        g_AppConfig.ModSystemDirty = false;

        if (g_AppConfig.TngSystemDirty) {
            if (hasActiveTngMods) {
                SetLaunchStatus("Merging level (.tng) mods...");
                std::string dataPath = g_AppConfig.GameRootPath + "\\Data";

                for (auto it = g_LoadedMods.rbegin(); it != g_LoadedMods.rend(); ++it) {
                    const auto& mod = *it;
                    if (!mod.IsEnabled || !mod.IsTngMod) continue;

                    std::string modDataPath = mod.ModFolderPath + "\\Data\\Levels";
                    if (!fs::exists(modDataPath)) continue;

                    for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".tng") {
                            std::string relPath = entry.path().string().substr(mod.ModFolderPath.length() + 5);
                            std::string targetPath = dataPath + relPath;

                            if (!fs::exists(targetPath)) {
                                fs::create_directories(fs::path(targetPath).parent_path());
                                fs::copy_file(entry.path(), targetPath);
                            }
                            else {
                                TngMerger::ProcessTngMod(entry.path().string(), targetPath);
                            }
                        }
                    }
                }
            }
            g_AppConfig.TngSystemDirty = false;
        }

        // --- FSE SCRIPT EXTENDER MODS ---
        if (g_AppConfig.FSESystemDirty) {
            if (hasActiveFSEMods) {
                SetLaunchStatus("Patching FSE script mods...");

                // Restore previous FSE state (files + delete deployed script folders)
                RestoreFSEFiles();

                // Create fresh backups before patching
                BackupFSEFiles();

                // Reload the (now-vanilla) quests.lua into the workspace
                CheckFSEInstalled(g_AppConfig.GameRootPath);
                LoadQuestsLua();

                // 1. Script folder deployment and text patches (reverse priority order: lowest first, highest overwrites)
                for (auto it = g_LoadedMods.rbegin(); it != g_LoadedMods.rend(); ++it) {
                    const auto& mod = *it;
                    if (!mod.IsEnabled || !mod.IsFSEMod) continue;

                    DeployFSEScriptFolders(mod);

                    // Patch FinalAlbion.qst with FSE.qst
                    std::string fseQstPath = mod.ModFolderPath + "\\FSE\\FSE.qst";
                    std::string finalAlbionPath = g_AppConfig.GameRootPath + "\\Data\\Levels\\FinalAlbion.qst";
                    if (fs::exists(fseQstPath)) PatchTextFile(fseQstPath, finalAlbionPath);

                    // Patch user.ini with FSEUser.ini (or FSEuser.ini)
                    std::string fseUserPath = mod.ModFolderPath + "\\FSE\\FSEUser.ini";
                    if (!fs::exists(fseUserPath)) fseUserPath = mod.ModFolderPath + "\\FSE\\FSEuser.ini";
                    std::string userIniPath = g_AppConfig.GameRootPath + "\\user.ini";
                    if (fs::exists(fseUserPath)) PatchTextFile(fseUserPath, userIniPath);
                }

                // 2. Quests.lua patching (forward priority order: index 0 = highest priority claims first)
                std::set<std::string> claimedQuestNames;
                for (const auto& mod : g_LoadedMods) {
                    if (!mod.IsEnabled || !mod.IsFSEMod) continue;
                    PatchQuestsLua(mod, claimedQuestNames);
                }

                // 3. Reassign IDs and save quests.lua
                FinalizeQuestsLuaIDsAndSave();
            } else {
                // No active FSE mods but dirty â€” just restore
                RestoreFSEFiles();
            }
            g_AppConfig.FSESystemDirty = false;
        }

        if (g_AppConfig.DefSystemDirty) {
            if (hasActiveDefMods) {
                SetLaunchStatus("Merging definition mods...");
                std::string dataPath = g_AppConfig.GameRootPath + "\\Data";

                for (auto it = g_LoadedMods.rbegin(); it != g_LoadedMods.rend(); ++it) {
                    const auto& mod = *it;
                    if (!mod.IsEnabled)   continue;
                    if (!mod.IsDefMod)    continue;

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
                        else if (ext == ".bin") fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                    }
                }
                CompileAudioBinFiles();
            }

            g_AppConfig.DefSystemDirty = false;
            SaveConfig();

            SetLaunchStatus("Compiling definitions...");
            g_PendingGameLaunch = true;
            CompileAllDefs_Native();
            return;
        }

        g_AppConfig.DefSystemDirty = false;
        SaveConfig();
        SetLaunchStatus("Launching game...");
        LaunchGame();
    }

    static void RestoreVanillaFiles(bool restoreBanks, bool restoreDefs, bool restoreTngs) {
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
            bool isTng = (origExt == ".tng");
            bool isLooseAsset = (!isBank && !isDef && !isTng && origExt != ".qst" && origExt != ".lua" && origExt != ".ini");

            // Group Loose Assets with Bank restoration (Asset Mods)
            if ((restoreBanks && (isBank || isLooseAsset)) || (restoreDefs && isDef) || (restoreTngs && isTng)) {
                try {
                    if (fs::exists(originalPath)) fs::remove(originalPath);
                    fs::rename(tmpPath, originalPath);
                }
                catch (...) {}
            }
        }
    }

    static void RestoreAllTmpBackups() {
        RestoreVanillaFiles(true, true, true);
        RestoreFSEFiles();
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

    static void BackupAffectedTngFiles() {
        std::string dataPath = g_AppConfig.GameRootPath + "\\Data";
        if (!fs::exists(dataPath)) return;

        for (const auto& mod : g_LoadedMods) {
            if (!mod.IsEnabled || !mod.IsTngMod) continue;

            std::string modDataPath = mod.ModFolderPath + "\\Data\\Levels";
            if (!fs::exists(modDataPath)) continue;

            for (const auto& entry : fs::recursive_directory_iterator(modDataPath)) {
                if (!entry.is_regular_file()) continue;

                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".tng") continue;
                std::string relPath = entry.path().string().substr(mod.ModFolderPath.length() + 5);
                std::string targetPath = dataPath + relPath;

                if (!fs::exists(targetPath)) continue;

                std::string tmpPath = targetPath + ".tmp";
                if (!fs::exists(tmpPath)) {
                    try { fs::copy_file(targetPath, tmpPath); }
                    catch (...) {}
                }
            }
        }
    }

    static void DeployLooseAssetMods() {
        std::string gameRoot = g_AppConfig.GameRootPath;

        for (auto it = g_LoadedMods.rbegin(); it != g_LoadedMods.rend(); ++it) {
            const auto& mod = *it;
            if (!mod.IsEnabled || !mod.IsAssetMod) continue;

            try {
                for (const auto& file : fs::recursive_directory_iterator(mod.ModFolderPath)) {
                    if (!file.is_regular_file()) continue;

                    std::string modFilePath = file.path().string();
                    std::string relPath = modFilePath.substr(mod.ModFolderPath.length());

                    std::string lowerRelPath = relPath;
                    std::transform(lowerRelPath.begin(), lowerRelPath.end(), lowerRelPath.begin(), ::tolower);

                    std::string filename = file.path().filename().string();
                    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

                    std::string ext = file.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    // --- Gatekeeper Logic ---
                    bool isAllowed = false;

                    if (lowerRelPath.find("\\data\\bones\\") != std::string::npos ||
                        lowerRelPath.find("\\data\\enginecache\\") != std::string::npos ||
                        (lowerRelPath.find("\\data\\levels\\") != std::string::npos && ext != ".tng") ||
                        lowerRelPath.find("\\data\\lightingtable\\") != std::string::npos ||
                        lowerRelPath.find("\\data\\video\\") != std::string::npos ||
                        lowerRelPath.find("\\data\\tattoos\\") != std::string::npos) {
                        isAllowed = true;
                    }
                    else if (lowerRelPath.find("\\data\\misc\\") != std::string::npos &&
                        lowerRelPath.find("\\data\\misc\\pc\\") == std::string::npos) {
                        if (filename != "game_animation_events.txt" && filename != "sound_animation_events.txt" &&
                            filename != "game_animation_events.bin" && filename != "sound_animation_events.bin" &&
                            filename != "stars.dat") {
                            isAllowed = true;
                        }
                    }
                    else if (lowerRelPath.find("\\data\\sound\\") != std::string::npos) {
                        if (ext != ".lug" && ext != ".met") isAllowed = true;
                    }
                    else if (lowerRelPath.find("\\") == lowerRelPath.find_last_of("\\")) {
                        if (filename == "user.ini" || filename == "userst.ini" ||
                            filename == "dbug.ini" || filename == "dbust.ini") {
                            isAllowed = true;
                        }
                    }

                    if (!isAllowed) continue;

                    // --- The Replacement Logic ---
                    std::string targetPath = gameRoot + relPath;
                    std::string tmpPath = targetPath + ".tmp";

                    fs::create_directories(fs::path(targetPath).parent_path());

                    if (fs::exists(targetPath)) {
                        if (fs::exists(tmpPath)) {
                            fs::remove(targetPath);
                            fs::copy_file(tmpPath, targetPath);
                            fs::copy_file(targetPath, tmpPath, fs::copy_options::overwrite_existing);
                        }
                        else {
                            fs::copy_file(targetPath, tmpPath);
                        }
                        fs::copy_file(modFilePath, targetPath, fs::copy_options::overwrite_existing);
                    }
                    else {
                        fs::copy_file(modFilePath, targetPath, fs::copy_options::overwrite_existing);
                    }
                }
            }
            catch (...) {}
        }
    }

    static void LaunchGame() {
        std::string exePath = g_AppConfig.GameRootPath + "\\Fable.exe";
        if (fs::exists(exePath)) {
            ShellExecuteA(NULL, "open", exePath.c_str(), NULL, g_AppConfig.GameRootPath.c_str(), SW_SHOWDEFAULT);
            exit(0);
        }
        else {
            g_ShowFallbackLaunchPopup = true;
        }
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

        for (int i = 0; i < (int)bank->Entries.size(); i++) {
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