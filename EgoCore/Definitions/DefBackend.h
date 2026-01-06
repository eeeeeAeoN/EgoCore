#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <set>
#include <shlobj.h> 
#include "TextEditor.h"

namespace fs = std::filesystem;

// --- Enums & Navigation Structures ---
enum class DefAction {
    None,
    SwitchToDef,
    SwitchToHeader,
    ExitProgram
};

struct PendingNavigation {
    DefAction Action = DefAction::None;
    std::string TargetType;
    int TargetIndex = -1;
};

// --- Data Structures ---
struct DefEntry {
    std::string Type;
    std::string Name;
    std::string SourceFile;
    size_t StartOffset = 0;
    size_t EndOffset = 0;
};

struct EnumEntry {
    std::string Name;
    std::string FullContent;
    std::string FilePath;
    size_t StartOffset = 0;
    size_t EndOffset = 0;
};

struct DefContext {
    std::string Name;
    std::string RelativePath;
    std::string AddonFileName;

    // Constructor to satisfy vector initialization
    DefContext(std::string n, std::string r, std::string a)
        : Name(n), RelativePath(r), AddonFileName(a) {
    }
    DefContext() {}
};

struct DefWorkspace {
    std::string RootPath;
    bool IsLoaded = false;

    int ActiveContextIndex = 0;
    std::vector<DefContext> Contexts;

    // --- DEFS STATE ---
    std::map<std::string, std::vector<DefEntry>> CategorizedDefs;
    std::string SelectedType;
    int SelectedEntryIndex = -1;
    char FilterText[128] = "";

    std::vector<EnumEntry> AllEnums;
    int SelectedEnumIndex = -1;
    char HeaderFilter[128] = "";

    PendingNavigation PendingNav;
    bool TriggerUnsavedPopup = false;
    bool ShowDefsMode = true;

    std::string OriginalContent;
    float EditorFontScale = 1.0f;
    bool IsSearchOpen = false;
    char SearchBuffer[128] = "";
    bool SearchCaseSensitive = false;

    TextEditor Editor;

    DefWorkspace() {
        Contexts = {
            { "Game Defs",     "Defs",              "EgoCoreDefs_Addons.def" },
            { "Script Defs",   "Defs\\ScriptDefs",   "EgoCoreScriptDefs_Addons.def" },
            { "Frontend Defs", "Defs\\FrontEndDefs", "EgoCoreFrontEndDefs_Addons.def" }
        };

        auto lang = TextEditor::LanguageDefinition::CPlusPlus();
        Editor.SetLanguageDefinition(lang);
        Editor.SetPalette(TextEditor::GetDarkPalette());
        Editor.SetShowWhitespaces(false);
    }

    bool IsDirty() const {
        if (ShowDefsMode) {
            if (SelectedType.empty() || SelectedEntryIndex == -1) return false;
        }
        else {
            if (SelectedEnumIndex == -1) return false;
        }
        return Editor.GetText() != OriginalContent;
    }
};

// --- GLOBALS (Must be inline to be shared) ---
inline DefWorkspace g_DefWorkspace;
inline std::vector<std::string> g_AvailableSoundBanks;

// --- FUNCTIONS (Must be inline to avoid multiple definition errors) ---

inline void ScanSoundBanks() {
    g_AvailableSoundBanks.clear();
    if (g_DefWorkspace.CategorizedDefs.count("SOUND_SETUP")) {
        const auto& entries = g_DefWorkspace.CategorizedDefs["SOUND_SETUP"];
        for (const auto& entry : entries) {
            if (entry.Name == "ENGLISH_SOUND_SETUP") {
                std::ifstream file(entry.SourceFile, std::ios::binary);
                if (file.is_open()) {
                    file.seekg(entry.StartOffset);
                    size_t len = entry.EndOffset - entry.StartOffset;
                    std::string content(len, '\0');
                    file.read(&content[0], len);
                    file.close();

                    std::regex lutRegex(R"(\"([a-zA-Z0-9_\-\.]+\.lut)\")", std::regex::icase);
                    auto begin = std::sregex_iterator(content.begin(), content.end(), lutRegex);
                    auto end = std::sregex_iterator();

                    for (std::sregex_iterator i = begin; i != end; ++i) {
                        std::smatch match = *i;
                        std::string val = match[1].str();
                        bool exists = false;
                        for (const auto& s : g_AvailableSoundBanks) if (s == val) exists = true;
                        if (!exists) g_AvailableSoundBanks.push_back(val);
                    }
                }
                break;
            }
        }
    }
    if (g_AvailableSoundBanks.empty()) {
        g_AvailableSoundBanks.push_back("dialogue.lut");
        g_AvailableSoundBanks.push_back("scriptdialogue.lut");
    }
}

inline void LoadHeadersFromDir(const std::string& rootPath) {
    g_DefWorkspace.AllEnums.clear();
    std::set<std::string> visitedFiles;
    std::regex enumRegex(R"(enum\s+(\w+)\s*(\{[\s\S]*?\};))");
    std::string searchPath = rootPath + "\\Data\\Defs";
    try {
        if (!fs::exists(searchPath)) return;
        for (const auto& entry : fs::recursive_directory_iterator(searchPath)) {
            std::string pathStr = entry.path().string();
            std::string pathLower = pathStr;
            std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
            if (pathLower.find("devheaders") != std::string::npos) continue;
            if (pathLower.find("retailheaders") != std::string::npos && pathLower.find("xbox") != std::string::npos) continue;
            if (entry.path().extension() == ".h") {
                if (visitedFiles.count(pathStr)) continue;
                visitedFiles.insert(pathStr);
                std::ifstream file(entry.path(), std::ios::binary);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string content = buffer.str();
                    auto words_begin = std::sregex_iterator(content.begin(), content.end(), enumRegex);
                    auto words_end = std::sregex_iterator();
                    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                        std::smatch match = *i;
                        EnumEntry newEnum;
                        newEnum.Name = match[1].str();
                        newEnum.FullContent = "enum " + newEnum.Name + "\n" + match[2].str();
                        newEnum.FilePath = pathStr;
                        newEnum.StartOffset = match.position(0);
                        newEnum.EndOffset = newEnum.StartOffset + match.length(0);
                        g_DefWorkspace.AllEnums.push_back(newEnum);
                    }
                }
            }
        }
    }
    catch (...) {}
    std::sort(g_DefWorkspace.AllEnums.begin(), g_DefWorkspace.AllEnums.end(), [](const EnumEntry& a, const EnumEntry& b) { return a.Name < b.Name; });
}

inline std::vector<std::string> GetEnumMembers(const std::string& enumName) {
    std::vector<std::string> members;

    int idx = -1;
    for (int i = 0; i < (int)g_DefWorkspace.AllEnums.size(); i++) {
        if (g_DefWorkspace.AllEnums[i].Name == enumName) {
            idx = i;
            break;
        }
    }
    if (idx == -1) return members;

    std::string content = g_DefWorkspace.AllEnums[idx].FullContent;

    size_t openBrace = content.find('{');
    size_t closeBrace = content.rfind('}');
    if (openBrace == std::string::npos || closeBrace == std::string::npos) return members;

    std::string body = content.substr(openBrace + 1, closeBrace - openBrace - 1);

    std::regex re(R"(([A-Z0-9_]+)\s*(?:=.*?)?(?:,|$))");
    auto begin = std::sregex_iterator(body.begin(), body.end(), re);
    auto end = std::sregex_iterator();

    for (auto i = begin; i != end; ++i) {
        std::smatch match = *i;
        std::string val = match[1].str();
        if (val != "force_dword" && val != "FORCE_DWORD") {
            members.push_back(val);
        }
    }
    return members;
}

inline void ScanFileForDefs(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t cursor = 0;
    while (true) {
        size_t defStart = content.find("#definition", cursor);
        if (defStart == std::string::npos) break;
        size_t defEnd = content.find("#end_definition", defStart);
        if (defEnd == std::string::npos) break;
        defEnd += 15;
        size_t lineEnd = content.find('\n', defStart);
        std::string headerLine = content.substr(defStart, lineEnd - defStart);
        std::stringstream ss(headerLine);
        std::string token, type, name;
        ss >> token >> type >> name;
        if (!type.empty() && !name.empty()) {
            DefEntry entry;
            entry.Type = type; entry.Name = name; entry.SourceFile = filePath.string();
            entry.StartOffset = defStart; entry.EndOffset = defEnd;
            g_DefWorkspace.CategorizedDefs[type].push_back(entry);
        }
        cursor = defEnd;
    }
}

inline void LoadDefsFromFolder(const std::string& rootPath) {
    g_DefWorkspace.IsLoaded = false;
    g_DefWorkspace.CategorizedDefs.clear();
    g_DefWorkspace.RootPath = rootPath;
    if (g_DefWorkspace.ActiveContextIndex < 0 || g_DefWorkspace.ActiveContextIndex >= g_DefWorkspace.Contexts.size()) g_DefWorkspace.ActiveContextIndex = 0;
    DefContext& ctx = g_DefWorkspace.Contexts[g_DefWorkspace.ActiveContextIndex];
    std::string searchPath = rootPath + "\\Data\\" + ctx.RelativePath;
    std::vector<std::string> visitedPaths;
    if (fs::exists(searchPath)) {
        for (const auto& entry : fs::recursive_directory_iterator(searchPath)) {
            std::string pathStr = entry.path().string();
            std::string pathLower = pathStr;
            std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
            if (pathLower.find("devheaders") != std::string::npos) continue;
            if (g_DefWorkspace.ActiveContextIndex == 0) {
                if (pathLower.find("\\scriptdefs") != std::string::npos || pathLower.find("/scriptdefs") != std::string::npos) continue;
                if (pathLower.find("\\frontenddefs") != std::string::npos || pathLower.find("/frontenddefs") != std::string::npos) continue;
            }
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".def" || ext == ".tpl") {
                    bool alreadyScanned = false;
                    for (const auto& v : visitedPaths) { if (v == pathStr) { alreadyScanned = true; break; } }
                    if (!alreadyScanned) { visitedPaths.push_back(pathStr); ScanFileForDefs(entry.path()); }
                }
            }
        }
    }
    LoadHeadersFromDir(rootPath);
    ScanSoundBanks();
    g_DefWorkspace.SelectedType = ""; g_DefWorkspace.SelectedEntryIndex = -1; g_DefWorkspace.Editor.SetText("");
    g_DefWorkspace.IsLoaded = true;
}

inline void CreateNewDef(const std::string& type) {
    if (g_DefWorkspace.RootPath.empty()) return;
    std::string baseName = "New_Entry";
    std::string candidateName = baseName;
    int counter = 1;
    auto& entries = g_DefWorkspace.CategorizedDefs[type];
    bool exists;
    do {
        exists = false;
        for (const auto& e : entries) { if (e.Name == candidateName) { exists = true; break; } }
        if (exists) { candidateName = baseName + "_" + std::to_string(counter++); }
    } while (exists);
    DefContext& ctx = g_DefWorkspace.Contexts[g_DefWorkspace.ActiveContextIndex];
    std::string targetFile = g_DefWorkspace.RootPath + "\\Data\\" + ctx.RelativePath + "\\" + ctx.AddonFileName;
    fs::path p(targetFile);
    if (!fs::exists(p.parent_path())) fs::create_directories(p.parent_path());
    std::ofstream file(targetFile, std::ios::app);
    if (file.is_open()) {
        file << "\n\n";
        file << "#definition " << type << " " << candidateName << "\n\n";
        file << "// Contents\n\n";
        file << "#end_definition\n";
        file.close();
        LoadDefsFromFolder(g_DefWorkspace.RootPath);
    }
}

inline void DeleteDefEntry(const DefEntry& entry) {
    std::ifstream inFile(entry.SourceFile, std::ios::binary);
    if (!inFile.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();
    if (content.size() < entry.EndOffset) return;
    size_t cutStart = entry.StartOffset;
    size_t cutEnd = entry.EndOffset;
    while (cutStart > 0) {
        size_t lineStart = content.rfind('\n', cutStart - 1);
        if (lineStart == std::string::npos) lineStart = 0; else lineStart += 1;
        if (lineStart >= cutStart) break;
        size_t prevLineEnd = (lineStart > 0) ? lineStart - 1 : 0;
        size_t prevLineStart = content.rfind('\n', (prevLineEnd > 0 ? prevLineEnd - 1 : 0));
        if (prevLineStart == std::string::npos) prevLineStart = 0; else prevLineStart += 1;
        std::string line = content.substr(prevLineStart, prevLineEnd - prevLineStart);
        size_t firstNonSpace = line.find_first_not_of(" \t\r");
        bool isDeletable = false;
        if (firstNonSpace == std::string::npos) isDeletable = true;
        else {
            if (line.substr(firstNonSpace, 2) == "//") isDeletable = true;
            if (line.find("#end_definition") != std::string::npos) isDeletable = false;
            if (line.find("#definition") != std::string::npos) isDeletable = false;
        }
        if (isDeletable) { cutStart = prevLineStart; if (cutStart == 0) break; }
        else break;
    }
    while (cutEnd < content.size()) {
        char c = content[cutEnd];
        if (isspace((unsigned char)c)) { cutEnd++; continue; }
        if (content.compare(cutEnd, 2, "//") == 0) {
            size_t nextLine = content.find('\n', cutEnd);
            if (nextLine == std::string::npos) cutEnd = content.size(); else cutEnd = nextLine + 1;
            continue;
        }
        break;
    }
    std::string pre = content.substr(0, cutStart);
    std::string post = content.substr(cutEnd);
    std::ofstream outFile(entry.SourceFile, std::ios::binary);
    outFile << pre << post;
    outFile.close();
    LoadDefsFromFolder(g_DefWorkspace.RootPath);
    g_DefWorkspace.SelectedEntryIndex = -1; g_DefWorkspace.SelectedType = ""; g_DefWorkspace.Editor.SetText("");
}

inline void SaveDefEntry(DefEntry& entry) {
    std::string newContent = g_DefWorkspace.Editor.GetText();
    std::ifstream inFile(entry.SourceFile, std::ios::binary);
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();
    if (fileContent.size() < entry.EndOffset) return;
    std::string pre = fileContent.substr(0, entry.StartOffset);
    std::string post = fileContent.substr(entry.EndOffset);
    std::string finalContent = pre + newContent + post;
    std::ofstream outFile(entry.SourceFile, std::ios::binary);
    outFile << finalContent; outFile.close();
    long long sizeDiff = (long long)newContent.size() - (long long)(entry.EndOffset - entry.StartOffset);
    for (auto& [t, list] : g_DefWorkspace.CategorizedDefs) {
        for (auto& other : list) {
            if (other.SourceFile == entry.SourceFile && other.StartOffset > entry.StartOffset) {
                other.StartOffset += sizeDiff; other.EndOffset += sizeDiff;
            }
        }
    }
    entry.EndOffset += sizeDiff;
    g_DefWorkspace.Editor.SetText(newContent);
    g_DefWorkspace.OriginalContent = g_DefWorkspace.Editor.GetText();
    std::stringstream ss(newContent);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find("#definition") != std::string::npos) {
            std::stringstream ls(line);
            std::string temp, type, name;
            ls >> temp >> type >> name;
            if (!name.empty()) { entry.Name = name; } break;
        }
    }
}

inline void SaveHeaderEntry(EnumEntry& entry) {
    std::string newContent = g_DefWorkspace.Editor.GetText();
    std::ifstream inFile(entry.FilePath, std::ios::binary);
    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();
    if (fileContent.size() < entry.EndOffset) return;
    std::string pre = fileContent.substr(0, entry.StartOffset);
    std::string post = fileContent.substr(entry.EndOffset);
    std::string finalContent = pre + newContent + post;
    std::ofstream outFile(entry.FilePath, std::ios::binary);
    outFile << finalContent; outFile.close();
    long long sizeDiff = (long long)newContent.size() - (long long)(entry.EndOffset - entry.StartOffset);
    for (auto& other : g_DefWorkspace.AllEnums) {
        if (other.FilePath == entry.FilePath && other.StartOffset > entry.StartOffset) {
            other.StartOffset += sizeDiff; other.EndOffset += sizeDiff;
        }
    }
    entry.EndOffset += sizeDiff;
    entry.FullContent = newContent;
    g_DefWorkspace.Editor.SetText(newContent);
    g_DefWorkspace.OriginalContent = g_DefWorkspace.Editor.GetText();
}

inline void LoadDefContent(DefEntry& entry) {
    std::ifstream file(entry.SourceFile, std::ios::binary);
    if (!file.is_open()) return;
    file.seekg(entry.StartOffset);
    size_t len = entry.EndOffset - entry.StartOffset;
    std::string buffer(len, '\0');
    file.read(&buffer[0], len);
    g_DefWorkspace.Editor.SetText(buffer);
    g_DefWorkspace.OriginalContent = g_DefWorkspace.Editor.GetText();
}

inline void LoadHeaderContent(EnumEntry& entry) {
    g_DefWorkspace.Editor.SetText(entry.FullContent);
    g_DefWorkspace.OriginalContent = g_DefWorkspace.Editor.GetText();
}

inline void FindNextInEditor() {
    if (!g_DefWorkspace.IsLoaded) return;
    std::string query = g_DefWorkspace.SearchBuffer;
    if (query.empty()) return;
    auto lines = g_DefWorkspace.Editor.GetTextLines();
    auto cursor = g_DefWorkspace.Editor.GetCursorPosition();
    int startLine = cursor.mLine;
    int startCol = cursor.mColumn + 1;
    auto charsMatch = [](char a, char b, bool caseSensitive) { if (caseSensitive) return a == b; return std::tolower(a) == std::tolower(b); };
    auto searchInLine = [&](const std::string& line, size_t fromCol) -> int {
        if (fromCol >= line.length()) return -1;
        if (line.length() - fromCol < query.length()) return -1;
        for (size_t i = fromCol; i <= line.length() - query.length(); i++) {
            bool match = true;
            for (size_t j = 0; j < query.length(); j++) { if (!charsMatch(line[i + j], query[j], g_DefWorkspace.SearchCaseSensitive)) { match = false; break; } }
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
            if (foundCol != -1) { TextEditor::Coordinates pos(i, foundCol); g_DefWorkspace.Editor.SetCursorPosition(pos); return; }
        }
    }
}