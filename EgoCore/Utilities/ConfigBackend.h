#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

struct AppConfig {
    std::string GameRootPath;
    std::vector<std::string> AutoLoadBanks;
    bool IsConfigured = false;

    // --- FLAGS ---
    bool ShowDeleteConfirm = true;
    bool ShowAddConfirm = true;
    bool ShowBankDeleteConfirm = true;
    bool ShowUnsavedChangesWarning = true;
};

// FIX: 'inline' ensures only ONE config exists across the whole app
inline AppConfig g_AppConfig;
static const std::string CONFIG_FILENAME = "egocore_config.ini";

static const std::vector<std::string> DEFAULT_BANKS = {
    "\\Data\\graphics\\graphics.big",
    "\\Data\\graphics\\pc\\textures.big",
    "\\Data\\graphics\\pc\\frontend.big",
    "\\Data\\lang\\English\\text.big",
    "\\Data\\lang\\English\\fonts.big",
    "\\Data\\lang\\English\\dialogue.big",
    "\\Data\\misc\\pc\\effects.big",
    "\\Data\\shaders\\pc\\shaders.big"
};

inline void SaveConfig() {
    std::ofstream file(CONFIG_FILENAME);
    if (file.is_open()) {
        file << "Root=" << g_AppConfig.GameRootPath << "\n";
        for (const auto& path : g_AppConfig.AutoLoadBanks) {
            file << "Bank=" << path << "\n";
        }
        file << "ShowDeleteConfirm=" << (g_AppConfig.ShowDeleteConfirm ? "1" : "0") << "\n";
        file << "ShowAddConfirm=" << (g_AppConfig.ShowAddConfirm ? "1" : "0") << "\n";
        file << "ShowBankDeleteConfirm=" << (g_AppConfig.ShowBankDeleteConfirm ? "1" : "0") << "\n";
        file << "ShowUnsavedChangesWarning=" << (g_AppConfig.ShowUnsavedChangesWarning ? "1" : "0") << "\n";
        file.close();
    }
}

inline void LoadConfig() {
    std::ifstream file(CONFIG_FILENAME);
    g_AppConfig.AutoLoadBanks.clear();
    g_AppConfig.IsConfigured = false;

    // Defaults
    g_AppConfig.ShowDeleteConfirm = true;
    g_AppConfig.ShowAddConfirm = true;
    g_AppConfig.ShowBankDeleteConfirm = true;
    g_AppConfig.ShowUnsavedChangesWarning = true;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.find("Root=") == 0) {
                g_AppConfig.GameRootPath = line.substr(5);
                if (!g_AppConfig.GameRootPath.empty() && fs::exists(g_AppConfig.GameRootPath)) {
                    g_AppConfig.IsConfigured = true;
                }
            }
            else if (line.find("Bank=") == 0) g_AppConfig.AutoLoadBanks.push_back(line.substr(5));
            else if (line.find("ShowDeleteConfirm=") == 0) g_AppConfig.ShowDeleteConfirm = (line.substr(18) == "1");
            else if (line.find("ShowAddConfirm=") == 0) g_AppConfig.ShowAddConfirm = (line.substr(15) == "1");
            else if (line.find("ShowBankDeleteConfirm=") == 0) g_AppConfig.ShowBankDeleteConfirm = (line.substr(22) == "1");
            else if (line.find("ShowUnsavedChangesWarning=") == 0) g_AppConfig.ShowUnsavedChangesWarning = (line.substr(26) == "1");
        }
        file.close();
    }
}