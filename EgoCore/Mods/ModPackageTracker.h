#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "ConfigBackend.h"
#include "BankTypes.h"

enum class EModAssetCategory {
    BankEntry,
    Definition,
    Header,
    AnimationEvent,
    FSEScript
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

inline std::vector<StagedModPackageEntry> g_ModPackageEntries;

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
            if (e.TypeName == "Definition") e.Category = EModAssetCategory::Definition;
            else if (e.TypeName == "C++ Header") e.Category = EModAssetCategory::Header;
            else if (e.TypeName == "Sound Event" || e.TypeName == "Game Event") e.Category = EModAssetCategory::AnimationEvent;
            else if (e.BankName == "FSE") e.Category = EModAssetCategory::FSEScript;
            else e.Category = EModAssetCategory::BankEntry;
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

    static void RemoveMark(const std::string& bankName, const std::string& entryName) {
        bool changed = false;
        for (size_t i = 0; i < g_MarkedEntries.size();) {
            if (g_MarkedEntries[i].BankName == bankName && g_MarkedEntries[i].EntryName == entryName) {
                g_MarkedEntries.erase(g_MarkedEntries.begin() + i);
                changed = true;
            }
            else {
                ++i;
            }
        }
        if (changed) SaveMarkedState();

        for (size_t i = 0; i < g_ModPackageEntries.size();) {
            if (g_ModPackageEntries[i].BankName == bankName && g_ModPackageEntries[i].EntryName == entryName) {
                g_ModPackageEntries.erase(g_ModPackageEntries.begin() + i);
            }
            else {
                ++i;
            }
        }
    }

    static void RemoveMarksForQuest(const std::string& questName) {
        bool changed = false;
        for (size_t i = 0; i < g_MarkedEntries.size();) {
            if (g_MarkedEntries[i].BankName == "FSE" &&
                (g_MarkedEntries[i].SubBankName == questName || g_MarkedEntries[i].SubBankName == questName + "\\Entities")) {
                g_MarkedEntries.erase(g_MarkedEntries.begin() + i);
                changed = true;
            }
            else {
                ++i;
            }
        }
        if (changed) SaveMarkedState();

        for (size_t i = 0; i < g_ModPackageEntries.size();) {
            if (g_ModPackageEntries[i].BankName == "FSE" &&
                (g_ModPackageEntries[i].SubBankName == questName || g_ModPackageEntries[i].SubBankName == questName + "\\Entities")) {
                g_ModPackageEntries.erase(g_ModPackageEntries.begin() + i);
            }
            else {
                ++i;
            }
        }
    }
};
