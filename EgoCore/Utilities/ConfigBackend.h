#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "InputManager.h"

namespace fs = std::filesystem;

struct AppConfig {
    std::string GameRootPath;
    std::vector<std::string> AutoLoadBanks;
    bool IsConfigured = false;
    bool SkipFrontend = false;
    bool ModSystemDirty = false;
    bool DefSystemDirty = false;
    bool ShowDeleteConfirm = true;
    bool ShowAddConfirm = true;
    bool ShowBankDeleteConfirm = true;
    bool ShowUnsavedChangesWarning = true;
    bool EnableLookupGeneration = false;
};
inline AppConfig g_AppConfig;
static const std::string CONFIG_FILENAME = "egocore_config.ini";

inline std::vector<std::string> GetDefaultBanks(const std::string& root) {
    const char* langs[] = { "English", "French", "Italian", "Chinese", "German", "Korean", "Japanese", "Spanish" };
    std::string detectedLang = "English";

    for (const char* l : langs) {
        if (fs::exists(fs::path(root) / "Data" / "Lang" / l)) {
            detectedLang = l;
            break;
        }
    }

    return {
        "\\Data\\graphics\\graphics.big",
        "\\Data\\graphics\\pc\\textures.big",
        "\\Data\\graphics\\pc\\frontend.big",
        "\\Data\\lang\\" + detectedLang + "\\text.big",
        "\\Data\\lang\\" + detectedLang + "\\fonts.big",
        "\\Data\\lang\\" + detectedLang + "\\dialogue.big",
        "\\Data\\misc\\pc\\effects.big",
        "\\Data\\shaders\\pc\\shaders.big"
    };
}

inline void SaveConfig() {
    std::ofstream file(CONFIG_FILENAME);
    if (file.is_open()) {
        file << "Root=" << g_AppConfig.GameRootPath << "\n";
        for (const auto& path : g_AppConfig.AutoLoadBanks) file << "Bank=" << path << "\n";
        file << "ShowDeleteConfirm=" << (g_AppConfig.ShowDeleteConfirm ? "1" : "0") << "\n";
        file << "ShowAddConfirm=" << (g_AppConfig.ShowAddConfirm ? "1" : "0") << "\n";
        file << "ShowBankDeleteConfirm=" << (g_AppConfig.ShowBankDeleteConfirm ? "1" : "0") << "\n";
        file << "ShowUnsavedChangesWarning=" << (g_AppConfig.ShowUnsavedChangesWarning ? "1" : "0") << "\n";
        file << "SkipFrontend=" << (g_AppConfig.SkipFrontend ? "1" : "0") << "\n";
        file << "ModSystemDirty=" << (g_AppConfig.ModSystemDirty ? "1" : "0") << "\n";
        file << "DefSystemDirty=" << (g_AppConfig.DefSystemDirty ? "1" : "0") << "\n";
        file << "EnableLookupGeneration=" << (g_AppConfig.EnableLookupGeneration ? "1" : "0") << "\n";

        auto SaveKey = [&](const std::string& name, const ShortcutKey& k) {
            file << name << "=" << (int)k.Key << "," << k.Ctrl << "," << k.Shift << "," << k.Alt << "\n";
            };
        SaveKey("Key_SwitchBankMode", g_Keybinds.SwitchBankMode);
        SaveKey("Key_SwitchDefMode", g_Keybinds.SwitchDefMode);
        SaveKey("Key_SaveEntry", g_Keybinds.SaveEntry);
        SaveKey("Key_Compile", g_Keybinds.Compile);
        SaveKey("Key_NavigateBack", g_Keybinds.NavigateBack);
        SaveKey("Key_NavigateForward", g_Keybinds.NavigateForward);
        SaveKey("Key_DeleteEntry", g_Keybinds.DeleteEntry);
        SaveKey("Key_ToggleLeftPanel", g_Keybinds.ToggleLeftPanel);
        SaveKey("Key_LookupDefinition", g_Keybinds.LookupDefinition);

        file.close();
    }
}

inline void LoadConfig() {
    std::ifstream file(CONFIG_FILENAME);
    g_AppConfig.AutoLoadBanks.clear();
    g_AppConfig.IsConfigured = false;
    g_AppConfig.SkipFrontend = false;
    g_AppConfig.ShowDeleteConfirm = true;
    g_AppConfig.ShowAddConfirm = true;
    g_AppConfig.ShowBankDeleteConfirm = true;
    g_AppConfig.ShowUnsavedChangesWarning = true;
    g_AppConfig.DefSystemDirty = false;
    g_AppConfig.EnableLookupGeneration = false;

    if (file.is_open()) {
        std::string line;

        auto ParseKey = [](const std::string& val, ShortcutKey& k) {
            std::stringstream ss(val);
            std::string item;
            if (std::getline(ss, item, ',')) k.Key = (ImGuiKey)std::stoi(item);
            if (std::getline(ss, item, ',')) k.Ctrl = (item == "1");
            if (std::getline(ss, item, ',')) k.Shift = (item == "1");
            if (std::getline(ss, item, ',')) k.Alt = (item == "1");
            };

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
            else if (line.find("ShowUnsavedChangesWarning=") == 0) g_AppConfig.ShowUnsavedChangesWarning = (line.substr(26) == "1");
            else if (line.find("SkipFrontend=") == 0) g_AppConfig.SkipFrontend = (line.substr(13) == "1");
            else if (line.find("ModSystemDirty=") == 0) g_AppConfig.ModSystemDirty = (line.substr(15) == "1");
            else if (line.find("DefSystemDirty=") == 0) g_AppConfig.DefSystemDirty = (line.substr(15) == "1");
            else if (line.find("EnableLookupGeneration=") == 0) g_AppConfig.EnableLookupGeneration = (line.substr(23) == "1");
            else if (line.find("Key_SwitchBankMode=") == 0) ParseKey(line.substr(19), g_Keybinds.SwitchBankMode);
            else if (line.find("Key_SwitchDefMode=") == 0) ParseKey(line.substr(18), g_Keybinds.SwitchDefMode);
            else if (line.find("Key_SaveEntry=") == 0) ParseKey(line.substr(14), g_Keybinds.SaveEntry);
            else if (line.find("Key_Compile=") == 0) ParseKey(line.substr(12), g_Keybinds.Compile);
            else if (line.find("Key_NavigateBack=") == 0) ParseKey(line.substr(17), g_Keybinds.NavigateBack);
            else if (line.find("Key_NavigateForward=") == 0) ParseKey(line.substr(20), g_Keybinds.NavigateForward);
            else if (line.find("Key_DeleteEntry=") == 0) ParseKey(line.substr(16), g_Keybinds.DeleteEntry);
            else if (line.find("Key_ToggleLeftPanel=") == 0) ParseKey(line.substr(20), g_Keybinds.ToggleLeftPanel);
            else if (line.find("Key_LookupDefinition=") == 0) ParseKey(line.substr(21), g_Keybinds.LookupDefinition);
        }
        file.close();
    }
}