#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <locale>
#include <directxmath.h>

using namespace DirectX;

struct BoneConfigGroup {
    std::string Name;
    std::vector<std::string> Bones;
};

struct BoneConfigEntry {
    std::string Name;
    XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
};

struct BoneConfigData {
    std::string CreatureType;
    std::vector<BoneConfigGroup> Groups;
    std::vector<BoneConfigEntry> Entries;
    std::unordered_map<std::string, XMFLOAT3> BoneScales;
    bool IsLoaded = false;
    std::string FileName;
    std::string FullPath;

    static std::string CleanName(const std::string& str) {
        std::string res;
        for (char c : str) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                res += (char)tolower((unsigned char)c);
            }
        }
        return res;
    }

    void RebuildScaleMap() {
        BoneScales.clear();
        for (const auto& entry : Entries) {
            std::string key = CleanName(entry.Name);
            if (!key.empty()) {
                BoneScales[key] = entry.Scale;
            }
        }
    }

    int FindEntryIndex(const std::string& boneName) const {
        for (size_t i = 0; i < Entries.size(); ++i) {
            if (Entries[i].Name == boneName) return (int)i;
        }
        std::string target = CleanName(boneName);
        if (target.empty()) return -1;
        for (size_t i = 0; i < Entries.size(); ++i) {
            if (CleanName(Entries[i].Name) == target) return (int)i;
        }
        return -1;
    }

    XMFLOAT3 GetBoneScale(const std::string& boneName) const {
        std::string key = CleanName(boneName);
        auto it = BoneScales.find(key);
        if (it != BoneScales.end()) return it->second;
        return XMFLOAT3(1.0f, 1.0f, 1.0f);
    }

    void SetBoneScale(const std::string& boneName, const XMFLOAT3& scale) {
        if (boneName.empty()) return;
        int idx = FindEntryIndex(boneName);
        if (idx >= 0) {
            Entries[idx].Scale = scale;
        }
        else {
            Entries.push_back({ boneName, scale });
        }
        std::string key = CleanName(boneName);
        if (!key.empty()) {
            BoneScales[key] = scale;
        }
    }

    bool RemoveBone(const std::string& boneName) {
        int idx = FindEntryIndex(boneName);
        if (idx < 0) return false;
        Entries.erase(Entries.begin() + idx);
        RebuildScaleMap();
        return true;
    }

    int PruneDefaultScales(float eps = 0.0005f) {
        int removed = 0;
        for (auto it = Entries.begin(); it != Entries.end(); ) {
            if (std::fabs(it->Scale.x - 1.0f) <= eps &&
                std::fabs(it->Scale.y - 1.0f) <= eps &&
                std::fabs(it->Scale.z - 1.0f) <= eps) {
                it = Entries.erase(it);
                ++removed;
            }
            else {
                ++it;
            }
        }
        if (removed > 0) RebuildScaleMap();
        return removed;
    }
};

class BoneConfigParser {
public:
    static std::string CleanName(const std::string& str) {
        return BoneConfigData::CleanName(str);
    }

    static std::string Trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string FormatFloat(float v) {
        std::ostringstream ss;
        ss.imbue(std::locale::classic());
        if (std::fabs(v) < 0.00005f) v = 0.0f;
        ss << std::fixed << std::setprecision(4) << v;
        std::string s = ss.str();
        size_t dot = s.find('.');
        if (dot != std::string::npos) {
            while (s.size() > dot + 2 && s.back() == '0') {
                s.pop_back();
            }
        }
        return s;
    }

    static std::string GetMirrorBoneName(const std::string& name) {
        if (name.empty()) return "";

        auto tryReplace = [&](const std::string& from, const std::string& to, std::string& out) -> bool {
            size_t pos = name.find(from);
            if (pos != std::string::npos) {
                out = name;
                out.replace(pos, from.size(), to);
                return true;
            }
            return false;
        };

        std::string out;
        if (tryReplace(" L ", " R ", out)) return out;
        if (tryReplace(" R ", " L ", out)) return out;
        if (tryReplace("_L_", "_R_", out)) return out;
        if (tryReplace("_R_", "_L_", out)) return out;
        if (name.rfind("L_", 0) == 0) return "R_" + name.substr(2);
        if (name.rfind("R_", 0) == 0) return "L_" + name.substr(2);
        if (name.size() >= 2 && name.substr(name.size() - 2) == "_L") return name.substr(0, name.size() - 2) + "_R";
        if (name.size() >= 2 && name.substr(name.size() - 2) == "_R") return name.substr(0, name.size() - 2) + "_L";
        if (name == "lhand") return "rhand";
        if (name == "rhand") return "lhand";
        if (name == "lfoot") return "rfoot";
        if (name == "rfoot") return "lfoot";
        return "";
    }

    static bool Parse(const std::string& filePath, BoneConfigData& outData) {
        outData = BoneConfigData();
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        std::string line;
        bool inGroupSettings = false;
        bool inBoneData = false;

        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty()) continue;

            if (line.rfind("Creature_type:", 0) == 0 || line.rfind("creature_type:", 0) == 0) {
                size_t semi = line.find(';');
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    size_t endPos = (semi != std::string::npos && semi > colon) ? semi : line.size();
                    outData.CreatureType = Trim(line.substr(colon + 1, endPos - colon - 1));
                }
                continue;
            }

            std::string lowerLine = line;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), [](unsigned char c) { return (char)::tolower(c); });

            if (lowerLine.find("#start_group_settings") != std::string::npos) {
                inGroupSettings = true;
                inBoneData = false;
                continue;
            }
            if (lowerLine.find("#end_group_settings") != std::string::npos) {
                inGroupSettings = false;
                continue;
            }
            if (lowerLine.find("#start_bone_data") != std::string::npos) {
                inBoneData = true;
                inGroupSettings = false;
                continue;
            }
            if (lowerLine.find("#end_bone_data") != std::string::npos) {
                inBoneData = false;
                continue;
            }

            if (inGroupSettings) {
                size_t colon = line.find(':');
                size_t semi = line.find(';');
                if (colon != std::string::npos) {
                    std::string grpName = Trim(line.substr(0, colon));
                    size_t endPos = (semi != std::string::npos && semi > colon) ? semi : line.size();
                    std::string membersStr = line.substr(colon + 1, endPos - colon - 1);

                    BoneConfigGroup grp;
                    grp.Name = grpName;

                    size_t pos = 0;
                    while (pos < membersStr.size()) {
                        size_t q1 = membersStr.find('"', pos);
                        if (q1 == std::string::npos) break;
                        size_t q2 = membersStr.find('"', q1 + 1);
                        if (q2 == std::string::npos) break;
                        std::string member = Trim(membersStr.substr(q1 + 1, q2 - q1 - 1));
                        if (!member.empty()) grp.Bones.push_back(member);
                        pos = q2 + 1;
                    }

                    if (!grp.Name.empty()) {
                        outData.Groups.push_back(std::move(grp));
                    }
                }
                continue;
            }

            if (inBoneData) {
                size_t colon = line.find(':');
                size_t semi = line.find(';');
                if (colon != std::string::npos) {
                    std::string boneName = Trim(line.substr(0, colon));
                    size_t endPos = (semi != std::string::npos && semi > colon) ? semi : line.size();
                    std::string valsStr = line.substr(colon + 1, endPos - colon - 1);
                    for (char& c : valsStr) {
                        if (c == ',') c = ' ';
                    }

                    std::stringstream ss(valsStr);
                    ss.imbue(std::locale::classic());
                    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
                    if (!boneName.empty() && (ss >> sx >> sy >> sz)) {
                        outData.Entries.push_back({ boneName, XMFLOAT3(sx, sy, sz) });
                    }
                }
            }
        }

        outData.RebuildScaleMap();
        outData.FullPath = filePath;
        outData.FileName = std::filesystem::path(filePath).filename().string();
        outData.IsLoaded = true;
        return true;
    }

    static bool Save(const std::string& filePath, const BoneConfigData& data) {
        std::error_code ec;
        std::filesystem::path p(filePath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path(), ec);
        }

        std::ofstream file(filePath, std::ios::trunc);
        if (!file.is_open()) return false;

        std::string creatureType = Trim(data.CreatureType);
        if (creatureType.empty()) creatureType = "CREATURE_HERO";

        file << "Creature_type: " << creatureType << ";\n";
        file << "#Start_group_settings\n";
        for (const auto& grp : data.Groups) {
            if (grp.Name.empty() || grp.Bones.empty()) continue;
            file << grp.Name << ": ";
            for (size_t i = 0; i < grp.Bones.size(); ++i) {
                if (i > 0) file << ", ";
                file << "\"" << grp.Bones[i] << "\"";
            }
            file << ";\n";
        }
        file << "#End_group_settings\n";
        file << "#Start_Bone_data\n";
        for (const auto& entry : data.Entries) {
            if (entry.Name.empty()) continue;
            file << entry.Name << ": "
                 << FormatFloat(entry.Scale.x) << ", "
                 << FormatFloat(entry.Scale.y) << ", "
                 << FormatFloat(entry.Scale.z) << ";\n";
        }
        file << "#End_bone_data\n";
        file.flush();
        return file.good();
    }
};
