#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "egostb/lev_reader.hpp"
#include "egostb/mesh_builder.hpp"

namespace egostb {

struct EngineThemeDef {
    std::string name;
    std::string baseTexture;
    std::string backgroundTexture;
    std::string baseBumpMap;
    std::string cliffBaseTexture;
    std::string cliffBackgroundTexture;
    std::string cliffBumpMap;
    float baseSelfIllumination = 0.0f;
    float cliffSelfIllumination = 0.0f;
    float waterHeight = 0.0f;
    int waterType = 0; // 0 = none, 1 = lake, 2 = river, 3 = sea, 4 = reflective sea, 5 = non-reflective sea, 6 = old, 8 = ice
    std::string localDetailGeneratorDef;
};

class ThemeManager {
public:
    static ThemeManager& instance();

    bool loadFromDefFile(const std::string& path);
    const EngineThemeDef* getTheme(const std::string& name) const;

    std::vector<ThemeMaterial> buildMaterials(
        const LevData& lev,
        bool retailMode,
        std::vector<std::string>& localPalette) const;

private:
    std::unordered_map<std::string, EngineThemeDef> m_themes;
};

} // namespace egostb
