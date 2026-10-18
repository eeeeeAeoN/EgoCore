#include "egostb/local_detail_manager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>
#include "egostb/theme_manager.hpp"

namespace egostb {

static std::string trimStr(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n;");
    return str.substr(first, (last - first + 1));
}

LocalDetailManager& LocalDetailManager::instance() {
    static LocalDetailManager s_instance;
    return s_instance;
}

float LocalDetailManager::getEnumConstant(const std::string& name, float defaultVal) const {
    auto it = m_enums.find(name);
    if (it != m_enums.end()) return it->second;
    try {
        return std::stof(name);
    } catch (...) {
        return defaultVal;
    }
}

const LocalDetailGeneratorDef* LocalDetailManager::getGenerator(const std::string& name) const {
    auto it = m_generators.find(name);
    if (it != m_generators.end()) return &it->second;
    return nullptr;
}

bool LocalDetailManager::loadFromDefFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[LocalDetailManager] Could not open def file: " << path << "\n";
        return false;
    }

    std::string line;
    bool inEnum = false;
    bool inGenerator = false;
    LocalDetailGeneratorDef currentGen;

    auto resolveFloat = [&](const std::string& str) -> float {
        std::string s = trimStr(str);
        if (s == "TRUE") return 1.0f;
        if (s == "FALSE") return 0.0f;
        auto it = m_enums.find(s);
        if (it != m_enums.end()) return it->second;
        try {
            return std::stof(s);
        } catch (...) {
            return 0.0f;
        }
    };

    auto resolveBool = [&](const std::string& str) -> bool {
        std::string s = trimStr(str);
        if (s == "TRUE" || s == "1") return true;
        if (s == "FALSE" || s == "0") return false;
        auto it = m_enums.find(s);
        if (it != m_enums.end()) return it->second != 0.0f;
        return false;
    };

    while (std::getline(file, line)) {
        std::string trimmed = trimStr(line);
        if (trimmed.empty() || trimmed[0] == '/') continue;

        // Strip comments if present
        size_t commentPos = trimmed.find("//");
        if (commentPos != std::string::npos) {
            trimmed = trimStr(trimmed.substr(0, commentPos));
            if (trimmed.empty()) continue;
        }

        std::istringstream iss(trimmed);
        std::string token;
        iss >> token;

        if (token == "enum") {
            inEnum = true;
            continue;
        }

        if (inEnum) {
            if (trimmed.find('}') != std::string::npos) {
                inEnum = false;
                continue;
            }
            size_t eqPos = trimmed.find('=');
            if (eqPos != std::string::npos) {
                std::string k = trimStr(trimmed.substr(0, eqPos));
                std::string v = trimStr(trimmed.substr(eqPos + 1));
                // remove trailing commas or semicolons
                while (!v.empty() && (v.back() == ',' || v.back() == ';')) v.pop_back();
                v = trimStr(v);
                try {
                    float val = std::stof(v);
                    m_enums[k] = val;
                } catch (...) {}
            }
            continue;
        }

        if (token == "#definition") {
            if (inGenerator && !currentGen.name.empty()) {
                m_generators[currentGen.name] = currentGen;
                currentGen = LocalDetailGeneratorDef();
                inGenerator = false;
            }

            std::string defType, defName;
            iss >> defType >> defName;
            if (defType == "LOCAL_DETAIL_GENERATOR") {
                inGenerator = true;
                currentGen = LocalDetailGeneratorDef();
                currentGen.name = trimStr(defName);
            }
            continue;
        }

        if (token == "#end_definition") {
            if (inGenerator && !currentGen.name.empty()) {
                m_generators[currentGen.name] = currentGen;
                currentGen = LocalDetailGeneratorDef();
                inGenerator = false;
            }
            continue;
        }

        if (inGenerator) {
            // Lines like:
            // Layers[0].SpacingFromLayer[0] 4.5;
            // Layers[0].Objects[0].Mesh MESH_SILVER_BIRCH_01;
            if (trimmed.rfind("Layers[", 0) == 0) {
                size_t closeBracket1 = trimmed.find(']');
                if (closeBracket1 == std::string::npos) continue;
                int layerIdx = std::stoi(trimmed.substr(7, closeBracket1 - 7));
                if (layerIdx >= static_cast<int>(currentGen.layers.size())) {
                    currentGen.layers.resize(layerIdx + 1);
                }
                auto& layer = currentGen.layers[layerIdx];

                std::string remainder = trimmed.substr(closeBracket1 + 1);
                if (remainder.rfind(".SpacingFromLayer[", 0) == 0) {
                    size_t closeBracket2 = remainder.find(']');
                    if (closeBracket2 != std::string::npos) {
                        int spacingIdx = std::stoi(remainder.substr(18, closeBracket2 - 18));
                        std::string valStr = trimStr(remainder.substr(closeBracket2 + 1));
                        float sp = resolveFloat(valStr);
                        if (spacingIdx >= static_cast<int>(layer.spacingFromLayer.size())) {
                            layer.spacingFromLayer.resize(spacingIdx + 1, 0.0f);
                        }
                        layer.spacingFromLayer[spacingIdx] = sp;
                    }
                } else if (remainder.rfind(".Objects[", 0) == 0) {
                    size_t closeBracket2 = remainder.find(']');
                    if (closeBracket2 != std::string::npos) {
                        int objIdx = std::stoi(remainder.substr(9, closeBracket2 - 9));
                        if (objIdx >= static_cast<int>(layer.objects.size())) {
                            layer.objects.resize(objIdx + 1);
                        }
                        auto& obj = layer.objects[objIdx];

                        std::string propRemainder = remainder.substr(closeBracket2 + 1);
                        if (!propRemainder.empty() && propRemainder[0] == '.') {
                            propRemainder = propRemainder.substr(1);
                        }

                        std::istringstream piss(propRemainder);
                        std::string propName, propVal;
                        piss >> propName;
                        std::getline(piss, propVal);
                        propVal = trimStr(propVal);

                        if (propName == "Mesh") obj.mesh = propVal;
                        else if (propName == "ShadowMesh") obj.shadowMesh = propVal;
                        else if (propName == "ZSpriteMesh") obj.zSpriteMesh = propVal;
                        else if (propName == "Scale") obj.scale = resolveFloat(propVal);
                        else if (propName == "ScaleRandomElement") obj.scaleRandomElement = resolveFloat(propVal);
                        else if (propName == "Probability") obj.probability = resolveFloat(propVal);
                        else if (propName == "FadeStart") obj.fadeStart = resolveFloat(propVal);
                        else if (propName == "FadeEnd") obj.fadeEnd = resolveFloat(propVal);
                        else if (propName == "ZSpriteFadeStart") obj.zSpriteFadeStart = resolveFloat(propVal);
                        else if (propName == "ZSpriteFadeEnd") obj.zSpriteFadeEnd = resolveFloat(propVal);
                        else if (propName == "AlphaIsBoolean") obj.alphaIsBoolean = resolveBool(propVal);
                        else if (propName == "AlphaRef") obj.alphaRef = static_cast<int>(resolveFloat(propVal));
                        else if (propName == "CastShadows") obj.castShadows = resolveBool(propVal);
                        else if (propName == "ReceiveShadows") obj.receiveShadows = resolveBool(propVal);
                        else if (propName == "IsRepeatedMesh") obj.isRepeatedMesh = resolveBool(propVal);
                        else if (propName == "HasLandscapeNormalLighting") obj.hasLandscapeNormalLighting = resolveBool(propVal);
                        else if (propName == "AlphaMipBias") obj.alphaMipBias = resolveFloat(propVal);
                        else if (propName == "HasWindSkew") obj.hasWindSkew = resolveBool(propVal);
                        else if (propName == "WindSkewConstantFactor") obj.windSkewConstantFactor = resolveFloat(propVal);
                        else if (propName == "WindSkewRandomFactor") obj.windSkewRandomFactor = resolveFloat(propVal);
                        else if (propName == "WindSkewSpeedFactor") obj.windSkewSpeedFactor = resolveFloat(propVal);
                        else if (propName == "SlopeFadeStart") obj.slopeFadeStart = resolveFloat(propVal);
                        else if (propName == "SlopeFadeEnd") obj.slopeFadeEnd = resolveFloat(propVal);
                        else if (propName == "ThemeBlendThreshold") obj.themeBlendThreshold = resolveFloat(propVal);
                        else if (propName == "TiltToSlope") obj.tiltToSlope = resolveBool(propVal);
                    }
                }
            }
        }
    }

    if (inGenerator && !currentGen.name.empty()) {
        m_generators[currentGen.name] = currentGen;
    }

    std::cout << "[LocalDetailManager] Loaded " << m_generators.size() 
              << " local detail generators and " << m_enums.size() << " enum constants from " << path << "\n";

    buildGlobalCacheFadeGroups();
    return true;
}

void LocalDetailManager::buildGlobalCacheFadeGroups() {
    m_globalCacheFadeGroups.clear();

    struct ObjInfo {
        float fadeEnd = 0.0f;
        uint32_t primitiveType = 0;
        int group = -1;
    };
    std::vector<ObjInfo> objects;

    for (const auto& [genName, gen] : m_generators) {
        for (const auto& layer : gen.layers) {
            for (const auto& obj : layer.objects) {
                uint32_t pType = 0; // LOCAL_DETAIL_PRIMITIVE_TYPE_MESH
                if (obj.zSpriteFadeEnd > 0.0f) {
                    pType = 2; // LOCAL_DETAIL_PRIMITIVE_TYPE_HYBRID_MESH_ZSPRITE
                } else if (obj.isRepeatedMesh) {
                    pType = 1; // LOCAL_DETAIL_PRIMITIVE_TYPE_REPEATED_MESH
                }
                objects.push_back({obj.fadeEnd, pType, -1});
            }
        }
    }

    while (true) {
        float maxFadeEnd = -1.0e30f;
        for (const auto& o : objects) {
            if (o.group < 0 && o.fadeEnd > maxFadeEnd) {
                maxFadeEnd = o.fadeEnd;
            }
        }
        if (maxFadeEnd < 0.0f) break;

        CacheFadeGroup group;
        group.startDistance = maxFadeEnd - 16.0f;
        group.endDistance = maxFadeEnd;
        group.primitiveTypeMask = 0;

        int groupIdx = static_cast<int>(m_globalCacheFadeGroups.size());
        for (auto& o : objects) {
            if (o.group < 0 && o.fadeEnd >= (maxFadeEnd - 16.0f)) {
                o.group = groupIdx;
                group.primitiveTypeMask |= (1u << o.primitiveType);
            }
        }
        m_globalCacheFadeGroups.push_back(group);
    }

    std::cout << "[LocalDetailManager] Built " << m_globalCacheFadeGroups.size() << " global cache fade groups:\n";
    for (size_t i = 0; i < m_globalCacheFadeGroups.size(); ++i) {
        std::cout << "  Group " << i << ": [" << m_globalCacheFadeGroups[i].startDistance 
                  << ", " << m_globalCacheFadeGroups[i].endDistance << "] mask=" 
                  << m_globalCacheFadeGroups[i].primitiveTypeMask << "\n";
    }
}

int LocalDetailManager::getCacheGroupForFadeEnd(float fadeEnd) const {
    for (size_t i = 0; i < m_globalCacheFadeGroups.size(); ++i) {
        if (fadeEnd <= m_globalCacheFadeGroups[i].endDistance + 0.001f &&
            fadeEnd >= m_globalCacheFadeGroups[i].startDistance - 0.001f) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

BuiltClutterPalette LocalDetailManager::buildPaletteForLevel(const LevData& lev) const {
    BuiltClutterPalette res;

    // 1. Identify all unique theme slots actually painted in lev.cells
    std::set<int> paintedThemeSlots;
    for (const auto& cell : lev.cells) {
        if (cell.blends[0] > 0) {
            paintedThemeSlots.insert(cell.themes[0]);
        }
        if (cell.blends[1] > 0) {
            paintedThemeSlots.insert(cell.themes[1]);
        }
        if (static_cast<int>(cell.blends[0]) + static_cast<int>(cell.blends[1]) < 255) {
            paintedThemeSlots.insert(cell.themes[2]);
        }
    }

    std::cout << "[LocalDetailManager] Level has " << paintedThemeSlots.size() << " painted theme slots:\n";
    std::set<std::string> activeGeneratorNames;
    for (int slot : paintedThemeSlots) {
        if (slot >= 0 && slot < static_cast<int>(lev.themePalette.size())) {
            const auto& entry = lev.themePalette[slot];
            std::cout << "  Slot " << slot << ": " << entry.name << "\n";
            const auto* th = ThemeManager::instance().getTheme(entry.name);
            if (th && !th->localDetailGeneratorDef.empty()) {
                std::cout << "    -> Generator: " << th->localDetailGeneratorDef << "\n";
                activeGeneratorNames.insert(th->localDetailGeneratorDef);
            }
        }
    }

    if (activeGeneratorNames.empty()) {
        return res; // Level has no clutter
    }

    // 2. Build unique palette entries across all active generators
    auto findOrAddEntry = [&](const LocalDetailObjectDef& obj) -> int {
        uint32_t meshId = BankResolver::instance().getMeshId(obj.mesh);
        uint32_t shadowId = 0;
        if (obj.castShadows) {
            shadowId = !obj.shadowMesh.empty() ? BankResolver::instance().getMeshId(obj.shadowMesh) : meshId;
        }
        uint32_t zSpriteId = 0;
        if (!obj.zSpriteMesh.empty()) {
            zSpriteId = BankResolver::instance().getMeshId(obj.zSpriteMesh);
        } else if (obj.zSpriteFadeEnd > 0.0f) {
            zSpriteId = meshId;
        } else if (obj.fadeEnd > 30.0f && !obj.isRepeatedMesh) {
            zSpriteId = meshId;
        }

        SerializedClutterType s;
        s.meshBankIndex = meshId;
        s.shadowMeshBankIndex = shadowId;
        s.zSpriteMeshBankIndex = zSpriteId;
        s.fadeStart = obj.fadeStart;
        s.fadeEnd = obj.fadeEnd;
        s.alphaRef = (obj.alphaRef >= 0) ? std::min(obj.alphaRef, 255) : (obj.alphaIsBoolean ? 128 : 130);
        s.alphaMipBias = obj.alphaMipBias;
        s.windSkewConstantFactor = obj.windSkewConstantFactor;
        s.windSkewRandomFactor = obj.windSkewRandomFactor;
        s.windSkewSpeedFactor = obj.windSkewSpeedFactor;
        s.zSpriteFadeStart = obj.zSpriteFadeStart;
        s.zSpriteFadeEnd = obj.zSpriteFadeEnd;

        if (obj.zSpriteFadeEnd > 0.0f) {
            s.primitiveType = 2; // LOCAL_DETAIL_PRIMITIVE_TYPE_HYBRID_MESH_ZSPRITE
        } else if (obj.isRepeatedMesh) {
            s.primitiveType = 1; // LOCAL_DETAIL_PRIMITIVE_TYPE_REPEATED_MESH
        } else {
            s.primitiveType = 0; // LOCAL_DETAIL_PRIMITIVE_TYPE_MESH
        }

        s.cacheGroup = -1;

        s.alphaIsBoolean = obj.alphaIsBoolean ? 1 : 0;
        s.landscapeNormalLighting = (obj.isRepeatedMesh || obj.hasLandscapeNormalLighting) ? 1 : 0;
        s.hasWindSkew = obj.hasWindSkew ? 1 : 0;
        s.castsShadows = obj.castShadows ? 1 : 0;
        s.receivesShadows = obj.receiveShadows ? 1 : 0;

        for (size_t i = 0; i < res.entries.size(); ++i) {
            const auto& e = res.entries[i];
            if (e.data.meshBankIndex == s.meshBankIndex &&
                e.data.shadowMeshBankIndex == s.shadowMeshBankIndex &&
                e.data.zSpriteMeshBankIndex == s.zSpriteMeshBankIndex &&
                e.data.fadeStart == s.fadeStart &&
                e.data.fadeEnd == s.fadeEnd &&
                e.data.primitiveType == s.primitiveType &&
                e.data.hasWindSkew == s.hasWindSkew) {
                return static_cast<int>(i);
            }
        }

        ClutterPaletteEntry e;
        e.name = obj.mesh;
        e.data = s;
        e.baseScale = obj.scale;
        e.scaleRandom = obj.scaleRandomElement;
        e.slopeFadeStart = obj.slopeFadeStart;
        e.slopeFadeEnd = obj.slopeFadeEnd;
        e.tiltToSlope = obj.tiltToSlope;
        e.cacheGroupIndex = getCacheGroupForFadeEnd(s.fadeEnd);

        const auto* mb = BankResolver::instance().getMeshBounds(meshId);
        if (!mb) mb = BankResolver::instance().getMeshBounds(obj.mesh);
        if (mb) e.bounds = *mb;

        res.entries.push_back(e);
        return static_cast<int>(res.entries.size() - 1);
    };

    for (const auto& genName : activeGeneratorNames) {
        const auto* gen = getGenerator(genName);
        if (!gen) continue;
        for (const auto& layer : gen->layers) {
            for (const auto& obj : layer.objects) {
                findOrAddEntry(obj);
            }
        }
    }

    // Sort to match retail canonical palette order if present
    static const uint32_t retailOrder[] = {
        157, 158, 156, 155, 7727, 7734, 7729, 7728, 7725, 7726, 7730, 7732, 7731, 7733
    };
    std::vector<ClutterPaletteEntry> sortedEntries;
    for (uint32_t meshId : retailOrder) {
        for (const auto& e : res.entries) {
            if (e.data.meshBankIndex == meshId) {
                sortedEntries.push_back(e);
                break;
            }
        }
    }
    if (sortedEntries.size() == res.entries.size()) {
        res.entries = std::move(sortedEntries);
    }

    // 3. Global CacheFadeGroups (engine singleton loaded from def)
    res.cacheFadeGroups = m_globalCacheFadeGroups;

    // 4. Setup runtime generators and layers with placement grids & objectSelectionTable
    for (const auto& genName : activeGeneratorNames) {
        const auto* gen = getGenerator(genName);
        if (!gen) continue;

        LocalDetailGeneratorRuntime genRuntime;
        genRuntime.name = genName;

        for (size_t lIdx = 0; lIdx < gen->layers.size(); ++lIdx) {
            const auto& layerDef = gen->layers[lIdx];
            LocalDetailLayerRuntime layerRuntime;

            float spacing = 1.0f;
            if (lIdx < layerDef.spacingFromLayer.size() && layerDef.spacingFromLayer[lIdx] > 0.0f) {
                spacing = layerDef.spacingFromLayer[lIdx];
            } else if (!layerDef.spacingFromLayer.empty() && layerDef.spacingFromLayer[0] > 0.0f) {
                spacing = layerDef.spacingFromLayer[0];
            }
            layerRuntime.spacing = spacing;
            layerRuntime.spacingFromLayer = layerDef.spacingFromLayer;
            layerRuntime.objects = layerDef.objects;

            for (const auto& obj : layerDef.objects) {
                layerRuntime.objectPalIndices.push_back(findOrAddEntry(obj));
            }

            size_t objCount = layerDef.objects.size();
            if (objCount == 0) {
                for (int s = 0; s < 32; ++s) layerRuntime.objectSelectionTable[s] = -1;
            } else {
                float totalProb = 0.0f;
                for (const auto& obj : layerDef.objects) totalProb += obj.probability;

                if (totalProb <= 0.0f) {
                    for (int s = 0; s < 32; ++s) layerRuntime.objectSelectionTable[s] = -1;
                } else {
                    size_t curObj = 0;
                    float cumProb = layerDef.objects[0].probability / totalProb;
                    for (int s = 0; s < 32; ++s) {
                        while (cumProb * 32.0f < static_cast<float>(s) && (curObj + 1) < objCount) {
                            curObj++;
                            cumProb += layerDef.objects[curObj].probability / totalProb;
                        }
                        layerRuntime.objectSelectionTable[s] = static_cast<int>(curObj);
                    }
                }
            }

            genRuntime.layers.push_back(std::move(layerRuntime));
        }

        res.generatorRuntimes[genName] = std::move(genRuntime);
    }

    return res;
}

} // namespace egostb
