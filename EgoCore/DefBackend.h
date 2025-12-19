#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <shlobj.h> 
#include "TextEditor.h"

namespace fs = std::filesystem;

struct DefEntry {
    std::string Type;
    std::string Name;
    std::string SourceFile;
    size_t StartOffset = 0;
    size_t EndOffset = 0;
};

struct DefWorkspace {
    std::string RootPath;
    bool IsLoaded = false;

    std::map<std::string, std::vector<DefEntry>> CategorizedDefs;

    std::string SelectedType;
    int SelectedEntryIndex = -1;
    char FilterText[128] = "";
    float EditorFontScale = 1.0f;

    bool IsSearchOpen = false;
    char SearchBuffer[128] = "";
    bool SearchCaseSensitive = false;

    TextEditor Editor;

    DefWorkspace() {
        auto lang = TextEditor::LanguageDefinition::CPlusPlus();
        Editor.SetLanguageDefinition(lang);
        Editor.SetPalette(TextEditor::GetDarkPalette());
        Editor.SetShowWhitespaces(false);
    }
};

static DefWorkspace g_DefWorkspace;

static std::string OpenFolderDialog() {
    char path[MAX_PATH];
    BROWSEINFOA bi = { 0 }; bi.lpszTitle = "Select Game Root Folder"; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) { SHGetPathFromIDListA(pidl, path); CoTaskMemFree(pidl); return std::string(path); }
    return "";
}

static void ScanFileForDefs(const fs::path& filePath) {
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

static void LoadDefsFromFolder(const std::string& rootPath) {
    g_DefWorkspace.IsLoaded = false;
    g_DefWorkspace.CategorizedDefs.clear();
    g_DefWorkspace.RootPath = rootPath;

    std::vector<std::string> searchPaths = { rootPath + "\\Data\\Defs" };
    std::vector<std::string> visitedPaths;

    for (const auto& searchDir : searchPaths) {
        if (!fs::exists(searchDir)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(searchDir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".def" || ext == ".tpl") {
                    std::string fullPath = entry.path().string();
                    bool alreadyScanned = false;
                    for (const auto& v : visitedPaths) { if (v == fullPath) { alreadyScanned = true; break; } }
                    if (!alreadyScanned) { visitedPaths.push_back(fullPath); ScanFileForDefs(entry.path()); }
                }
            }
        }
    }
    g_DefWorkspace.IsLoaded = true;
}

static void SaveDefEntry(DefEntry& entry) {
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
    entry.EndOffset += sizeDiff;
}

static void LoadDefContent(DefEntry& entry) {
    std::ifstream file(entry.SourceFile, std::ios::binary);
    if (!file.is_open()) return;
    file.seekg(entry.StartOffset);
    size_t len = entry.EndOffset - entry.StartOffset;
    std::string buffer(len, '\0');
    file.read(&buffer[0], len);
    g_DefWorkspace.Editor.SetText(buffer);
}

// --- YOUR EXACT SEARCH IMPLEMENTATION ---
static void FindNextInEditor() {
    if (!g_DefWorkspace.IsLoaded) return;

    std::string query = g_DefWorkspace.SearchBuffer;
    if (query.empty()) return;

    auto lines = g_DefWorkspace.Editor.GetTextLines();
    auto cursor = g_DefWorkspace.Editor.GetCursorPosition();

    int startLine = cursor.mLine;
    int startCol = cursor.mColumn + 1; // Start search from next character

    // Case-insensitive helper
    auto charsMatch = [](char a, char b, bool caseSensitive) {
        if (caseSensitive) return a == b;
        return std::tolower(a) == std::tolower(b);
        };

    // Search function
    auto searchInLine = [&](const std::string& line, size_t fromCol) -> int {
        if (fromCol >= line.length()) return -1;
        if (line.length() - fromCol < query.length()) return -1;

        for (size_t i = fromCol; i <= line.length() - query.length(); i++) {
            bool match = true;
            for (size_t j = 0; j < query.length(); j++) {
                if (!charsMatch(line[i + j], query[j], g_DefWorkspace.SearchCaseSensitive)) {
                    match = false;
                    break;
                }
            }
            if (match) return (int)i;
        }
        return -1;
        };

    // Two-pass search
    for (int pass = 0; pass < 2; pass++) {
        int beginLine = (pass == 0) ? startLine : 0;
        int endLine = (pass == 0) ? (int)lines.size() : startLine + 1;

        for (int i = beginLine; i < endLine; i++) {
            if (i >= (int)lines.size()) continue;

            int searchFrom = (pass == 0 && i == startLine) ? startCol : 0;
            int foundCol = searchInLine(lines[i], searchFrom);

            if (foundCol != -1) {
                // Just move cursor, no selection
                TextEditor::Coordinates pos(i, foundCol);
                g_DefWorkspace.Editor.SetCursorPosition(pos);
                return;
            }
        }
    }
}