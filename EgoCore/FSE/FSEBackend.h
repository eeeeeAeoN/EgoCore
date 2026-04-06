#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include "TextEditor.h"
#include "ConfigBackend.h"

namespace fs = std::filesystem;

enum class EFSEItemType { None, QuestMain, Entity, ExtraScript };

struct FSEEntity {
    std::string Name;
    std::string File;
    int ID;
};

struct FSEQuest {
    std::string Name;
    std::string File;
    int ID;
    std::vector<FSEEntity> Entities;
    std::vector<std::string> ExtraScripts;
};

struct FSEWorkspace {
    bool IsInstalled = false;
    std::string FSERoot;
    std::vector<FSEQuest> Quests;

    TextEditor Editor;
    std::string ActiveFilePath;
    std::string OriginalContent;
    bool IsLoaded = false;

    float EditorFontScale = 1.0f;
    bool IsSearchOpen = false;
    char SearchBuffer[128] = "";
    bool SearchCaseSensitive = false;

    EFSEItemType ActiveItemType = EFSEItemType::None;
    int ActiveQuestIdx = -1;
    int ActiveEntityIdx = -1;
    std::string ActiveExtraScriptName = "";

    FSEWorkspace() {
        auto lang = TextEditor::LanguageDefinition::Lua();
        Editor.SetLanguageDefinition(lang);
        Editor.SetPalette(TextEditor::GetDarkPalette());
        Editor.SetShowWhitespaces(false);
    }

    bool IsDirty() const {
        if (ActiveFilePath.empty()) return false;
        return Editor.GetText() != OriginalContent;
    }
};

inline FSEWorkspace g_FSEWorkspace;

inline bool CheckFSEInstalled(const std::string& gameRoot) {
    if (gameRoot.empty()) return false;
    fs::path dllPath = fs::path(gameRoot) / "FableScriptExtender.dll";
    g_FSEWorkspace.FSERoot = (fs::path(gameRoot) / "FSE").string();
    g_FSEWorkspace.IsInstalled = fs::exists(dllPath) && fs::exists(g_FSEWorkspace.FSERoot);
    return g_FSEWorkspace.IsInstalled;
}

inline void ScanExtraScripts(FSEQuest& quest) {
    quest.ExtraScripts.clear();
    fs::path questDir = fs::path(g_FSEWorkspace.FSERoot) / quest.Name;
    if (!fs::exists(questDir)) return;

    for (const auto& entry : fs::directory_iterator(questDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filename = entry.path().stem().string();
            if (filename != quest.Name) {
                quest.ExtraScripts.push_back(filename);
            }
        }
    }
}

inline void LoadQuestsLua() {
    g_FSEWorkspace.Quests.clear();
    g_FSEWorkspace.IsLoaded = true;

    if (!g_FSEWorkspace.IsInstalled) return;

    std::string questsFile = (fs::path(g_FSEWorkspace.FSERoot) / "quests.lua").string();
    if (!fs::exists(questsFile)) return;

    std::ifstream file(questsFile);
    std::string line;

    FSEQuest currentQuest;
    bool inQuest = false;
    bool inEntities = false;

    std::regex questStartRegex("([a-zA-Z0-9_]+)\\s*=\\s*\\{");
    std::regex propNameRegex("name\\s*=\\s*\"([^\"]+)\"");
    std::regex propFileRegex("file\\s*=\\s*\"([^\"]+)\"");
    std::regex propIdRegex("id\\s*=\\s*(\\d+)");
    std::regex entityScriptsStartRegex("entity_scripts\\s*=\\s*\\{");
    std::regex entityRegex("\\{\\s*name\\s*=\\s*\"([^\"]+)\",\\s*file\\s*=\\s*\"([^\"]+)\",\\s*id\\s*=\\s*(\\d+)\\s*\\}");

    while (std::getline(file, line)) {
        std::smatch match;
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));

        if (!inQuest) {
            if (trimmed.find("Quests") != std::string::npos && trimmed.find("=") != std::string::npos) continue;

            if (std::regex_search(line, match, questStartRegex)) {
                inQuest = true;
                currentQuest = FSEQuest();
            }
        }
        else if (!inEntities) {
            if (std::regex_search(line, match, propNameRegex)) currentQuest.Name = match[1].str();
            else if (std::regex_search(line, match, propFileRegex)) currentQuest.File = match[1].str();
            else if (std::regex_search(line, match, propIdRegex)) currentQuest.ID = std::stoi(match[1].str());
            else if (std::regex_search(line, match, entityScriptsStartRegex)) {
                inEntities = true;
            }
            else if (trimmed == "}," || trimmed == "}") {
                if (!currentQuest.Name.empty()) {
                    ScanExtraScripts(currentQuest);
                    g_FSEWorkspace.Quests.push_back(currentQuest);
                }
                inQuest = false;
            }
        }
        else {
            if (std::regex_search(line, match, entityRegex)) {
                FSEEntity e;
                e.Name = match[1].str();
                e.File = match[2].str();
                e.ID = std::stoi(match[3].str());
                currentQuest.Entities.push_back(e);
            }
            else if (trimmed == "}" || trimmed == "},") {
                inEntities = false;
            }
        }
    }
}

inline void SaveQuestsLua() {
    if (!g_FSEWorkspace.IsInstalled) return;
    std::string questsFile = (fs::path(g_FSEWorkspace.FSERoot) / "quests.lua").string();

    std::ofstream out(questsFile);
    out << "Quests = {\n\n";
    for (size_t i = 0; i < g_FSEWorkspace.Quests.size(); ++i) {
        const auto& q = g_FSEWorkspace.Quests[i];
        out << "    " << q.Name << " = {\n";
        out << "        name = \"" << q.Name << "\",\n";
        out << "        file = \"" << q.File << "\",\n";
        out << "        id = " << q.ID << ",\n\n";
        out << "        entity_scripts = {\n";

        for (size_t j = 0; j < q.Entities.size(); ++j) {
            const auto& e = q.Entities[j];
            out << "            { name = \"" << e.Name << "\", file = \"" << e.File << "\", id = " << e.ID << " }";
            if (j < q.Entities.size() - 1) out << ",\n";
            else out << "\n";
        }

        out << "        }\n    }";
        if (i < g_FSEWorkspace.Quests.size() - 1) out << ",\n\n";
        else out << "\n";
    }
    out << "}\n";
    out.close();
}

inline int GetNextQuestID() {
    int maxId = 4999;
    for (const auto& q : g_FSEWorkspace.Quests) {
        if (q.ID > maxId) maxId = q.ID;
    }
    return maxId + 1;
}

inline int GetNextEntityID() {
    int maxId = 1999;
    for (const auto& q : g_FSEWorkspace.Quests) {
        for (const auto& e : q.Entities) {
            if (e.ID > maxId) maxId = e.ID;
        }
    }
    return maxId + 1;
}

inline void CreateFSEFileIfMissing(const std::string& path, const std::string& defaultContent) {
    if (!fs::exists(path)) {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream file(path);
        file << defaultContent;
        file.close();
    }
}

inline void CreateFSEQuest(const std::string& name) {
    FSEQuest q;
    q.Name = name;
    q.File = name + "/" + name;
    q.ID = GetNextQuestID();
    g_FSEWorkspace.Quests.push_back(q);
    SaveQuestsLua();

    std::string path = (fs::path(g_FSEWorkspace.FSERoot) / name / (name + ".lua")).string();
    std::string content = "-- Quest: " + name + "\n\nfunction Init(Quest)\nend\n\nfunction Main(Quest)\nend\n\nfunction OnPersist(Quest, Context)\nend\n";
    CreateFSEFileIfMissing(path, content);
}

inline void CreateFSEEntity(int questIdx, const std::string& name) {
    if (questIdx < 0 || questIdx >= g_FSEWorkspace.Quests.size()) return;
    FSEQuest& q = g_FSEWorkspace.Quests[questIdx];

    FSEEntity e;
    e.Name = name;
    e.File = q.Name + "/Entities/" + name;
    e.ID = GetNextEntityID();
    q.Entities.push_back(e);
    SaveQuestsLua();

    std::string path = (fs::path(g_FSEWorkspace.FSERoot) / q.Name / "Entities" / (name + ".lua")).string();
    std::string content = "-- Entity: " + name + "\n\nfunction Init(questObject, meObject)\nend\n\nfunction Main(questObject, meObject)\nend\n";
    CreateFSEFileIfMissing(path, content);
}

inline void CreateFSEScript(int questIdx, const std::string& name) {
    if (questIdx < 0 || questIdx >= g_FSEWorkspace.Quests.size()) return;
    FSEQuest& q = g_FSEWorkspace.Quests[questIdx];

    std::string path = (fs::path(g_FSEWorkspace.FSERoot) / q.Name / (name + ".lua")).string();
    std::string content = "-- Script: " + name + "\n\nfunction Init(Quest)\nend\n";
    CreateFSEFileIfMissing(path, content);

    q.ExtraScripts.push_back(name);
}

inline void LoadFSEScriptContent(const std::string& relativePath, EFSEItemType type, int questIdx, int entityIdx = -1, const std::string& extraName = "") {
    std::string fullPath = (fs::path(g_FSEWorkspace.FSERoot) / (relativePath + ".lua")).string();

    g_FSEWorkspace.ActiveItemType = type;
    g_FSEWorkspace.ActiveQuestIdx = questIdx;
    g_FSEWorkspace.ActiveEntityIdx = entityIdx;
    g_FSEWorkspace.ActiveExtraScriptName = extraName;
    g_FSEWorkspace.ActiveFilePath = fullPath;

    if (fs::exists(fullPath)) {
        std::ifstream file(fullPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        g_FSEWorkspace.Editor.SetText(content);
        g_FSEWorkspace.OriginalContent = g_FSEWorkspace.Editor.GetText();
    }
    else {
        g_FSEWorkspace.Editor.SetText("-- File not found: " + relativePath);
        g_FSEWorkspace.OriginalContent = "";
    }
}

inline void SaveActiveFSEScript() {
    if (g_FSEWorkspace.ActiveFilePath.empty()) return;
    std::ofstream out(g_FSEWorkspace.ActiveFilePath);
    out << g_FSEWorkspace.Editor.GetText();
    out.close();
    g_FSEWorkspace.OriginalContent = g_FSEWorkspace.Editor.GetText();
}

inline void DeleteActiveFSEItem() {
    if (g_FSEWorkspace.ActiveItemType == EFSEItemType::None) return;

    std::error_code ec;

    if (g_FSEWorkspace.ActiveItemType == EFSEItemType::QuestMain) {
        if (g_FSEWorkspace.ActiveQuestIdx >= 0 && g_FSEWorkspace.ActiveQuestIdx < g_FSEWorkspace.Quests.size()) {
            auto& q = g_FSEWorkspace.Quests[g_FSEWorkspace.ActiveQuestIdx];
            fs::remove_all(fs::path(g_FSEWorkspace.FSERoot) / q.Name, ec);
            g_FSEWorkspace.Quests.erase(g_FSEWorkspace.Quests.begin() + g_FSEWorkspace.ActiveQuestIdx);
            SaveQuestsLua();
        }
    }
    else if (g_FSEWorkspace.ActiveItemType == EFSEItemType::Entity) {
        if (g_FSEWorkspace.ActiveQuestIdx >= 0 && g_FSEWorkspace.ActiveEntityIdx >= 0) {
            auto& q = g_FSEWorkspace.Quests[g_FSEWorkspace.ActiveQuestIdx];
            auto& e = q.Entities[g_FSEWorkspace.ActiveEntityIdx];
            fs::remove(fs::path(g_FSEWorkspace.FSERoot) / (e.File + ".lua"), ec);
            q.Entities.erase(q.Entities.begin() + g_FSEWorkspace.ActiveEntityIdx);
            SaveQuestsLua();
        }
    }
    else if (g_FSEWorkspace.ActiveItemType == EFSEItemType::ExtraScript) {
        if (g_FSEWorkspace.ActiveQuestIdx >= 0) {
            auto& q = g_FSEWorkspace.Quests[g_FSEWorkspace.ActiveQuestIdx];
            fs::remove(fs::path(g_FSEWorkspace.FSERoot) / q.Name / (g_FSEWorkspace.ActiveExtraScriptName + ".lua"), ec);
            auto it = std::find(q.ExtraScripts.begin(), q.ExtraScripts.end(), g_FSEWorkspace.ActiveExtraScriptName);
            if (it != q.ExtraScripts.end()) q.ExtraScripts.erase(it);
        }
    }

    g_FSEWorkspace.ActiveFilePath = "";
    g_FSEWorkspace.Editor.SetText("");
    g_FSEWorkspace.ActiveItemType = EFSEItemType::None;
}

inline void FindNextInFSEEditor() {
    if (!g_FSEWorkspace.IsLoaded) return;
    std::string query = g_FSEWorkspace.SearchBuffer;
    if (query.empty()) return;
    auto lines = g_FSEWorkspace.Editor.GetTextLines();
    auto cursor = g_FSEWorkspace.Editor.GetCursorPosition();
    int startLine = cursor.mLine;
    int startCol = cursor.mColumn + 1;
    auto charsMatch = [](char a, char b, bool caseSensitive) { if (caseSensitive) return a == b; return std::tolower(a) == std::tolower(b); };
    auto searchInLine = [&](const std::string& line, size_t fromCol) -> int {
        if (fromCol >= line.length()) return -1;
        if (line.length() - fromCol < query.length()) return -1;
        for (size_t i = fromCol; i <= line.length() - query.length(); i++) {
            bool match = true;
            for (size_t j = 0; j < query.length(); j++) { if (!charsMatch(line[i + j], query[j], g_FSEWorkspace.SearchCaseSensitive)) { match = false; break; } }
            if (match) return (int)i;
        } return -1;
        };
    for (int pass = 0; pass < 2; pass++) {
        int beginLine = (pass == 0) ? startLine : 0;
        int endLine = (pass == 0) ? (int)lines.size() : startLine + 1;
        for (int i = beginLine; i < endLine; i++) {
            if (i >= (int)lines.size()) continue;
            int searchFrom = (pass == 0 && i == startLine) ? startCol : 0;
            int foundCol = searchInLine(lines[i], searchFrom);
            if (foundCol != -1) { TextEditor::Coordinates pos(i, foundCol); g_FSEWorkspace.Editor.SetCursorPosition(pos); return; }
        }
    }
}