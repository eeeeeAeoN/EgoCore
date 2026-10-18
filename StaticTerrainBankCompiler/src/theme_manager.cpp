#include "egostb/theme_manager.hpp"
#include "egostb/bank_resolver.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <set>

namespace egostb {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n;");
    return str.substr(first, (last - first + 1));
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    if (s_instance.m_themes.empty()) {
        EngineThemeDef fallback;
        fallback.name = "INVALID_THEME_STANDIN";
        fallback.baseTexture = "LANDSCAPE_INVALID_THEME_TEXTURE";
        fallback.backgroundTexture = "LANDSCAPE_INVALID_THEME_TEXTURE";
        fallback.cliffBaseTexture = "LANDSCAPE_INVALID_THEME_TEXTURE";
        fallback.cliffBackgroundTexture = "LANDSCAPE_INVALID_THEME_TEXTURE";
        s_instance.m_themes["INVALID_THEME_STANDIN"] = fallback;
    }
    return s_instance;
}

bool ThemeManager::loadFromDefFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open def file: " << path << "\n";
        return false;
    }

    std::string line;
    EngineThemeDef currentTheme;
    bool inTheme = false;

    while (std::getline(file, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '/') continue;

        std::istringstream iss(t);
        std::string token;
        iss >> token;

        if (token == "#definition") {
            std::string defType, defName;
            iss >> defType >> defName;
            if (defType == "ENGINE_THEME") {
                inTheme = true;
                currentTheme = EngineThemeDef();
                currentTheme.name = trim(defName);
            }
        } else if (token == "#end_definition") {
            if (inTheme) {
                if (currentTheme.backgroundTexture.empty()) {
                    currentTheme.backgroundTexture = currentTheme.baseTexture;
                }
                m_themes[currentTheme.name] = currentTheme;
                inTheme = false;
            }
        } else if (inTheme) {
            std::string value;
            std::getline(iss, value);
            value = trim(value);

            if (token == "BaseTexture") {
                currentTheme.baseTexture = value;
            } else if (token == "BackgroundTexture") {
                currentTheme.backgroundTexture = value;
            } else if (token == "BaseBumpMap") {
                currentTheme.baseBumpMap = value;
            } else if (token == "CliffBaseTexture") {
                currentTheme.cliffBaseTexture = value;
            } else if (token == "CliffBackgroundTexture") {
                currentTheme.cliffBackgroundTexture = value;
            } else if (token == "CliffBumpMap") {
                currentTheme.cliffBumpMap = value;
            } else if (token == "BaseSelfIllumination") {
                try { currentTheme.baseSelfIllumination = std::stof(value); } catch (...) {}
            } else if (token == "CliffSelfIllumination") {
                try { currentTheme.cliffSelfIllumination = std::stof(value); } catch (...) {}
            } else if (token == "WaterHeight") {
                try { currentTheme.waterHeight = std::stof(value); } catch (...) {}
            } else if (token == "WaterType") {
                if (value.find("WATER_TYPE_LAKE") != std::string::npos) currentTheme.waterType = 1;
                else if (value.find("WATER_TYPE_RIVER") != std::string::npos) currentTheme.waterType = 2;
                else if (value.find("WATER_TYPE_NON_REFLECTIVE_SEA") != std::string::npos) currentTheme.waterType = 5;
                else if (value.find("WATER_TYPE_REFLECTIVE_SEA") != std::string::npos) currentTheme.waterType = 4;
                else if (value.find("WATER_TYPE_SEA") != std::string::npos) currentTheme.waterType = 3;
                else if (value.find("WATER_TYPE_OLD") != std::string::npos) currentTheme.waterType = 6;
                else if (value.find("WATER_TYPE_ICE") != std::string::npos) currentTheme.waterType = 8;
            } else if (token == "LocalDetailGeneratorDef") {
                currentTheme.localDetailGeneratorDef = value;
            }
        }
    }

    std::cout << "Loaded " << m_themes.size() << " themes from " << path << "\n";
    return true;
}

const EngineThemeDef* ThemeManager::getTheme(const std::string& name) const {
    if (name.empty()) return nullptr;
    auto it = m_themes.find(name);
    if (it != m_themes.end()) {
        return &it->second;
    }
    it = m_themes.find("INVALID_THEME_STANDIN");
    if (it != m_themes.end()) {
        static std::set<std::string> s_warned;
        if (s_warned.insert(name).second) {
            std::cout << "  [ThemeManager] Theme '" << name << "' not found, defaulting to INVALID_THEME_STANDIN\n";
        }
        return &it->second;
    }
    return nullptr;
}

std::vector<ThemeMaterial> ThemeManager::buildMaterials(
    const LevData& lev,
    bool retailMode,
    std::vector<std::string>& localPalette) const
{
    std::vector<ThemeMaterial> materials(256);

    if (!retailMode) {
        if (localPalette.empty()) {
            localPalette.push_back(""); // Index 0 is null/empty
        }
    }

    auto resolveTexture = [&](const std::string& name) -> uint32_t {
        if (name.empty()) return 0;
        if (retailMode) {
            return BankResolver::instance().getTextureId(name);
        } else {
            for (size_t i = 1; i < localPalette.size(); ++i) {
                if (localPalette[i] == name) return static_cast<uint32_t>(i);
            }
            localPalette.push_back(name);
            return static_cast<uint32_t>(localPalette.size() - 1);
        }
    };

    // Pass 1: resolve base textures for all active slots
    for (size_t slot = 0; slot < lev.themePalette.size() && slot < 256; ++slot) {
        const auto& tName = lev.themePalette[slot].name;
        if (tName.empty()) continue;
        const auto* def = getTheme(tName);
        if (!def) continue;

        materials[slot].available = true;

        std::string baseFg = def->baseTexture;
        std::string baseBg = def->backgroundTexture.empty() ? def->baseTexture : def->backgroundTexture;
        std::string baseBump = def->baseBumpMap;

        materials[slot].base.textures[0] = resolveTexture(baseFg);
        materials[slot].base.textures[1] = resolveTexture(baseBg);
        materials[slot].base.textures[2] = resolveTexture(baseBump);
        materials[slot].base.selfIllumination = def->baseSelfIllumination;
    }

    // Pass 2: resolve cliff textures for all active slots
    for (size_t slot = 0; slot < lev.themePalette.size() && slot < 256; ++slot) {
        const auto& tName = lev.themePalette[slot].name;
        if (tName.empty()) continue;
        const auto* def = getTheme(tName);
        if (!def) continue;

        std::string cliffFg = def->cliffBaseTexture;
        std::string cliffBg = def->cliffBackgroundTexture.empty() ? def->cliffBaseTexture : def->cliffBackgroundTexture;
        std::string cliffBump = def->cliffBumpMap;

        materials[slot].cliff.textures[0] = resolveTexture(cliffFg);
        materials[slot].cliff.textures[1] = resolveTexture(cliffBg);
        materials[slot].cliff.textures[2] = resolveTexture(cliffBump);
        materials[slot].cliff.selfIllumination = def->cliffSelfIllumination;
    }

    return materials;
}

} // namespace egostb
