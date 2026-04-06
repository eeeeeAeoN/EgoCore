#pragma once
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <fstream>
#include <algorithm>
#include <cctype>
#include "imgui.h"
#include "TextEditor.h"

struct FSEMethodSignature {
    std::string Name;
    std::string FullSignature;
    std::vector<std::string> Parameters;
};

namespace FSEAutosuggest {
    inline std::map<std::string, std::vector<FSEMethodSignature>> APIMethods;
    inline std::map<std::string, std::vector<std::string>> APIEnums;

    inline std::string CurrentQuestVar = "Quest";
    inline std::string CurrentThingVar = "Me";

    inline bool IsSuggestPopupOpen = false;
    inline std::string CurrentFilter = "";
    inline std::vector<std::string> FilteredSuggestions;
    inline std::vector<std::string> FilteredSignatures;
    inline int SelectedSuggestionIndex = 0;

    inline int DismissedLine = -1;
    inline ImVec2 PopupPos;

    inline void LoadDictionaries(const std::string& fseApiFile, const std::string& enumsFile) {
        APIMethods.clear();
        APIEnums.clear();

        auto trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            };

        std::regex sectionReg(R"(\[([a-zA-Z0-9_]+)\])");
        std::regex methodReg(R"(([a-zA-Z0-9_]+)\((.*)\))");

        std::ifstream api(fseApiFile);
        std::string line, currentSection;

        while (std::getline(api, line)) {
            trim(line);
            if (line.empty() || line[0] == ';') continue;

            std::smatch match;
            if (std::regex_match(line, match, sectionReg)) {
                currentSection = match[1].str();
            }
            else if (!currentSection.empty() && std::regex_search(line, match, methodReg)) {
                FSEMethodSignature sig;
                sig.Name = match[1].str();
                sig.FullSignature = line;

                std::string paramsStr = match[2].str();

                paramsStr.erase(std::remove(paramsStr.begin(), paramsStr.end(), '['), paramsStr.end());
                paramsStr.erase(std::remove(paramsStr.begin(), paramsStr.end(), ']'), paramsStr.end());

                size_t pos = 0;
                while ((pos = paramsStr.find(',')) != std::string::npos) {
                    std::string param = paramsStr.substr(0, pos);
                    trim(param);
                    if (!param.empty()) sig.Parameters.push_back(param);
                    paramsStr.erase(0, pos + 1);
                }
                trim(paramsStr);
                if (!paramsStr.empty()) sig.Parameters.push_back(paramsStr);

                APIMethods[currentSection].push_back(sig);
            }
        }

        std::ifstream enums(enumsFile);
        currentSection = "";

        while (std::getline(enums, line)) {
            trim(line);
            if (line.empty() || line[0] == ';') continue;

            std::smatch match;
            if (std::regex_match(line, match, sectionReg)) {
                currentSection = match[1].str();
            }
            else if (!currentSection.empty()) {
                APIEnums[currentSection].push_back(line);
            }
        }
    }

    inline void UpdateLocalContext(TextEditor& editor) {
        auto cursor = editor.GetCursorPosition();
        auto lines = editor.GetTextLines();

        std::regex funcReg(R"(function\s+[a-zA-Z0-9_]+\s*\(\s*([a-zA-Z0-9_]+)?\s*(?:,\s*([a-zA-Z0-9_]+))?)");

        for (int i = cursor.mLine; i >= 0; --i) {
            if (i >= lines.size()) continue;
            std::smatch match;
            if (std::regex_search(lines[i], match, funcReg)) {
                if (match[1].matched) CurrentQuestVar = match[1].str();
                if (match[2].matched) CurrentThingVar = match[2].str();
                break;
            }
        }
    }

    inline void ProcessEditorInput(TextEditor& editor) {
        if (!editor.IsFocused()) return;

        UpdateLocalContext(editor);

        auto cursor = editor.GetCursorPosition();
        auto lines = editor.GetTextLines();
        if (cursor.mLine >= lines.size()) return;

        if (cursor.mLine != DismissedLine) {
            DismissedLine = -1;
        }

        std::string currentLine = lines[cursor.mLine];
        std::string textBeforeCursor = currentLine.substr(0, cursor.mColumn);

        auto isQuestVar = [&](const std::string& v) {
            return v == "Quest" || v == "quest" || v == "questObject" || v == CurrentQuestVar;
            };
        auto isThingVar = [&](const std::string& v) {
            return v == "Me" || v == "me" || v == "meObject" || v == CurrentThingVar;
            };

        std::regex methodTypingReg(R"(([a-zA-Z0-9_]+):([a-zA-Z0-9_]*)$)");
        std::regex paramTypingReg(R"(([a-zA-Z0-9_]+):([a-zA-Z0-9_]+)\s*\((.*)$)");

        std::smatch match;

        FilteredSuggestions.clear();
        FilteredSignatures.clear();
        IsSuggestPopupOpen = false;

        if (std::regex_search(textBeforeCursor, match, methodTypingReg)) {
            std::string varName = match[1].str();
            CurrentFilter = match[2].str();
            std::string targetDict = "";

            if (isQuestVar(varName)) targetDict = "CQuestStateType";
            else if (isThingVar(varName)) targetDict = "CScriptThingType";

            if (!targetDict.empty()) {
                std::string filterUpper = CurrentFilter;
                std::transform(filterUpper.begin(), filterUpper.end(), filterUpper.begin(), ::toupper);

                for (const auto& sig : APIMethods[targetDict]) {
                    std::string nameUpper = sig.Name;
                    std::transform(nameUpper.begin(), nameUpper.end(), nameUpper.begin(), ::toupper);

                    if (CurrentFilter.empty() || nameUpper.find(filterUpper) != std::string::npos) {
                        FilteredSuggestions.push_back(sig.Name);
                        FilteredSignatures.push_back(sig.FullSignature);
                        if (FilteredSuggestions.size() >= 15) break;
                    }
                }

                if (!FilteredSuggestions.empty() && cursor.mLine != DismissedLine) {
                    IsSuggestPopupOpen = true;
                }
            }
        }
        else if (std::regex_search(textBeforeCursor, match, paramTypingReg)) {
            std::string varName = match[1].str();
            std::string methodName = match[2].str();
            std::string argsString = match[3].str();

            // Calculate parameter index safely by ignoring inner functions and strings.
            // Also detects if the outer parenthesis has been closed (depth < 0).
            int currentParamIndex = 0;
            int depth = 0;
            bool inString = false;

            for (char c : argsString) {
                if (c == '"') inString = !inString;
                else if (!inString) {
                    if (c == '(') depth++;
                    else if (c == ')') depth--;
                    else if (c == ',' && depth == 0) currentParamIndex++;
                }
            }

            // Only attempt to show suggestions if the main function hasn't been closed
            if (depth >= 0) {
                std::string targetDict = isQuestVar(varName) ? "CQuestStateType" :
                    isThingVar(varName) ? "CScriptThingType" : "";

                if (!targetDict.empty()) {
                    for (const auto& sig : APIMethods[targetDict]) {
                        if (sig.Name == methodName) {
                            if (currentParamIndex < sig.Parameters.size()) {
                                std::string activeParam = sig.Parameters[currentParamIndex];

                                std::regex lastWordReg(R"(([a-zA-Z0-9_]+)$)");
                                std::smatch wordMatch;
                                CurrentFilter = "";
                                if (std::regex_search(argsString, wordMatch, lastWordReg)) {
                                    CurrentFilter = wordMatch[1].str();
                                }

                                for (const auto& [enumName, enumValues] : APIEnums) {
                                    if (activeParam.find(enumName) != std::string::npos) {
                                        std::string filterUpper = CurrentFilter;
                                        std::transform(filterUpper.begin(), filterUpper.end(), filterUpper.begin(), ::toupper);

                                        for (const auto& val : enumValues) {
                                            std::string valUpper = val;
                                            std::transform(valUpper.begin(), valUpper.end(), valUpper.begin(), ::toupper);

                                            if (CurrentFilter.empty() || valUpper.find(filterUpper) != std::string::npos) {
                                                FilteredSuggestions.push_back(val);
                                                FilteredSignatures.push_back(enumName + "::" + val);
                                                if (FilteredSuggestions.size() >= 10) break;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                            if (!FilteredSuggestions.empty() && cursor.mLine != DismissedLine) IsSuggestPopupOpen = true;
                            break;
                        }
                    }
                }
            }
        }
    }
}