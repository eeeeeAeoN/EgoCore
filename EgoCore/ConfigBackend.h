#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "BankBackend.h"
#include "DefBackend.h"

namespace fs = std::filesystem;

struct AppConfig {
    std::string GameRootPath;
    std::vector<std::string> AutoLoadBanks;
    bool IsConfigured = false;
};

static AppConfig g_AppConfig;
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

static void SaveConfig() {
    std::ofstream file(CONFIG_FILENAME);
    if (file.is_open()) {
        file << "Root=" << g_AppConfig.GameRootPath << "\n";
        for (const auto& path : g_AppConfig.AutoLoadBanks) {
            file << "Bank=" << path << "\n";
        }
        file.close();
    }
}

static void LoadConfig() {
    std::ifstream file(CONFIG_FILENAME);
    g_AppConfig.AutoLoadBanks.clear();
    g_AppConfig.IsConfigured = false;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("Root=") == 0) {
                g_AppConfig.GameRootPath = line.substr(5);
                if (!g_AppConfig.GameRootPath.empty() && fs::exists(g_AppConfig.GameRootPath)) {
                    g_AppConfig.IsConfigured = true;
                }
            }
            else if (line.find("Bank=") == 0) {
                std::string path = line.substr(5);
                if (!path.empty()) g_AppConfig.AutoLoadBanks.push_back(path);
            }
        }
        file.close();
    }
}

static void PerformAutoLoad() {
    if (!g_AppConfig.IsConfigured) return;

    LoadDefsFromFolder(g_AppConfig.GameRootPath);

    for (const auto& relativePath : g_AppConfig.AutoLoadBanks) {
        std::string fullPath = g_AppConfig.GameRootPath + relativePath;
        if (fs::exists(fullPath)) {
            LoadBank(fullPath);
        }
    }
}

static void InitializeSetup(const std::string& selectedPath) {
    g_AppConfig.GameRootPath = selectedPath;
    g_AppConfig.AutoLoadBanks = DEFAULT_BANKS;
    g_AppConfig.IsConfigured = true;
    SaveConfig();
    PerformAutoLoad();
}