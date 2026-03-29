#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "TextEditor.h"

struct EventEntry {
    std::string AnimName;
    std::string Content;
};

struct EventFile {
    std::string FileName;
    std::string FilePath;
    std::vector<EventEntry> Events;
    bool IsLoaded = false;

    bool Load(const std::string& path) {
        FilePath = path;
        FileName = std::filesystem::path(path).filename().string();
        Events.clear();
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        EventEntry currentEntry;
        bool inEvent = false;

        while (std::getline(file, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
            trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

            if (trimmed.empty() || trimmed == "BEGIN_ANIMATION_EVENTS" || trimmed == "END_ANIMATION_EVENTS") continue;

            if (trimmed.find("BEGIN_EVENTS:") == 0) {
                inEvent = true;
                currentEntry = EventEntry();
                currentEntry.AnimName = trimmed.substr(13);
                currentEntry.AnimName.erase(0, currentEntry.AnimName.find_first_not_of(" \t"));
            }
            else if (trimmed == "END_EVENTS") {
                if (inEvent) {
                    Events.push_back(currentEntry);
                    inEvent = false;
                }
            }
            else if (inEvent) {
                currentEntry.Content += line + "\n";
            }
        }
        IsLoaded = true;
        return true;
    }

    bool Save() {
        if (FilePath.empty()) return false;
        std::ofstream file(FilePath, std::ios::trunc);
        if (!file.is_open()) return false;

        file << "BEGIN_ANIMATION_EVENTS\n\n";
        for (const auto& e : Events) {
            file << "BEGIN_EVENTS: " << e.AnimName << "\n";
            if (!e.Content.empty()) {
                file << e.Content;
                if (e.Content.back() != '\n') file << "\n";
            }
            file << "END_EVENTS\n\n";
        }
        file << "END_ANIMATION_EVENTS\n";
        return true;
    }
};

struct EventWorkspace {
    EventFile SoundEvents;
    EventFile GameEvents;
    int SelectedFileType = 0; // 0 = Sound, 1 = Game
    int SelectedEventIndex = -1;
    TextEditor Editor;
    char FilterText[128] = "";
    std::string OriginalContent;

    EventWorkspace() {
        auto lang = TextEditor::LanguageDefinition::CPlusPlus();
        Editor.SetLanguageDefinition(lang);
        Editor.SetPalette(TextEditor::GetDarkPalette());
        Editor.SetShowWhitespaces(false);
    }

    void LoadAll(const std::string& rootPath) {
        std::string miscPath = rootPath + "\\Data\\Misc\\";
        SoundEvents.Load(miscPath + "sound_animation_events.txt");
        GameEvents.Load(miscPath + "game_animation_events.txt");
        SelectedEventIndex = -1;
        Editor.SetText("");
    }

    EventFile* GetActiveFile() {
        return SelectedFileType == 0 ? &SoundEvents : &GameEvents;
    }

    bool IsDirty() {
        if (SelectedEventIndex == -1) return false;
        return Editor.GetText() != OriginalContent;
    }
};

inline EventWorkspace g_EventWorkspace;