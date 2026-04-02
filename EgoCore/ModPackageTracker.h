#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "ModPackageCompiler.h"

class ModPackageTracker {
public:
    inline static std::vector<StagedModPackageEntry> g_MarkedEntries;

    static void LoadMarkedState() {
        g_MarkedEntries.clear();
        std::ifstream file("EgoCore_MarkedEntries.txt");
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            StagedModPackageEntry e;
            e.EntryID = std::stoul(line);
            std::getline(file, e.EntryName);

            std::getline(file, line);
            e.EntryType = std::stoi(line);

            std::getline(file, line);
            e.BankType = (EBankType)std::stoi(line);

            std::getline(file, e.TypeName);
            std::getline(file, e.BankName);
            std::getline(file, e.SubBankName);
            std::getline(file, e.SourceFullPath);

            g_MarkedEntries.push_back(e);
        }
    }

    static void SaveMarkedState() {
        std::ofstream file("EgoCore_MarkedEntries.txt");
        for (const auto& e : g_MarkedEntries) {
            file << e.EntryID << "\n"
                << e.EntryName << "\n"
                << e.EntryType << "\n"
                << (int)e.BankType << "\n"
                << e.TypeName << "\n"
                << e.BankName << "\n"
                << e.SubBankName << "\n"
                << e.SourceFullPath << "\n";
        }
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

    static void RemoveMark(size_t index) {
        if (index < g_MarkedEntries.size()) {
            g_MarkedEntries.erase(g_MarkedEntries.begin() + index);
            SaveMarkedState();
        }
    }

    static void ClearAll() {
        g_MarkedEntries.clear();
        SaveMarkedState();
    }
};