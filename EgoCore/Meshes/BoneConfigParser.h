#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <directxmath.h>

using namespace DirectX;

struct BoneConfigData {
    std::string CreatureType;
    std::unordered_map<std::string, XMFLOAT3> BoneScales;
    bool IsLoaded = false;
    std::string FileName;
};

class BoneConfigParser {
public:
    static std::string CleanName(const std::string& str) {
        std::string res;
        for (char c : str) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                res += (char)tolower(c);
            }
        }
        return res;
    }

    static bool Parse(const std::string& filePath, BoneConfigData& outData) {
        outData = BoneConfigData();
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        std::string line;
        bool inBoneData = false;

        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);
            if (line.empty()) continue;

            if (line.rfind("Creature_type:", 0) == 0) {
                size_t semi = line.find(';');
                size_t colon = line.find(':');
                if (semi != std::string::npos && semi > colon) {
                    std::string val = line.substr(colon + 1, semi - colon - 1);
                    size_t vStart = val.find_first_not_of(" \t");
                    size_t vEnd = val.find_last_not_of(" \t");
                    if (vStart != std::string::npos && vEnd != std::string::npos) {
                        outData.CreatureType = val.substr(vStart, vEnd - vStart + 1);
                    }
                }
                continue;
            }

            std::string lowerLine = line;
            std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

            if (lowerLine.find("#start_bone_data") != std::string::npos) {
                inBoneData = true;
                continue;
            }
            if (lowerLine.find("#end_bone_data") != std::string::npos) {
                inBoneData = false;
                continue;
            }

            if (inBoneData) {
                size_t colon = line.find(':');
                size_t semi = line.find(';');
                if (colon != std::string::npos && semi != std::string::npos && semi > colon) {
                    std::string boneName = line.substr(0, colon);
                    size_t bStart = boneName.find_first_not_of(" \t");
                    size_t bEnd = boneName.find_last_not_of(" \t");
                    if (bStart != std::string::npos && bEnd != std::string::npos) {
                        boneName = boneName.substr(bStart, bEnd - bStart + 1);
                    }

                    std::string valsStr = line.substr(colon + 1, semi - colon - 1);
                    for (char& c : valsStr) {
                        if (c == ',') c = ' ';
                    }

                    std::stringstream ss(valsStr);
                    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
                    if (ss >> sx >> sy >> sz) {
                        outData.BoneScales[CleanName(boneName)] = XMFLOAT3(sx, sy, sz);
                    }
                }
            }
        }

        outData.IsLoaded = true;
        return true;
    }
};