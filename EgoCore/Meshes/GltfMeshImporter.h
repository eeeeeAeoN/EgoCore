#pragma once
#include "MeshParser.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <map>
#include <algorithm>
#include <filesystem>
#include <queue>

namespace GltfMeshImporter {

    static std::string ExtractBlock(const std::string& json, const std::string& key, size_t startPos = 0) {
        size_t pos = json.find("\"" + key + "\"", startPos);
        if (pos == std::string::npos) return "";
        pos = json.find_first_of("[{", pos);
        if (pos == std::string::npos) return "";
        int depth = 0; char openChar = json[pos]; char closeChar = (openChar == '[') ? ']' : '}';
        size_t end = pos; bool inString = false;
        for (; end < json.length(); ++end) {
            if (json[end] == '"' && (end == 0 || json[end - 1] != '\\')) inString = !inString;
            if (inString) continue;
            if (json[end] == openChar) depth++;
            else if (json[end] == closeChar) { depth--; if (depth == 0) return json.substr(pos, end - pos + 1); }
        }
        return "";
    }

    static bool ExtractBool(const std::string& json, const std::string& key, bool defaultVal = false) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos + 4 <= json.length() && json.compare(pos, 4, "true") == 0) return true;
        if (pos + 5 <= json.length() && json.compare(pos, 5, "false") == 0) return false;
        return defaultVal;
    }

    static float ExtractFloatClean(const std::string& json, const std::string& key, float defaultVal = 0.0f) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return defaultVal;
        pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try {
            std::stringstream ss(json.substr(pos));
            ss.imbue(std::locale("C"));
            float f; if (ss >> f) return f;
            return defaultVal;
        }
        catch (...) { return defaultVal; }
    }

    static std::string ExtractStringClean(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos = json.find('\"', pos);
        if (pos == std::string::npos) return "";
        size_t endPos = json.find('\"', pos + 1);
        if (endPos == std::string::npos) return "";
        return json.substr(pos + 1, endPos - pos - 1);
    }

    static std::vector<float> ExtractFloatArray(const std::string& json, const std::string& key) {
        std::vector<float> res;
        std::string arr = ExtractBlock(json, key);
        if (arr.empty() || arr[0] != '[') return res;
        size_t start = 1;
        while (start < arr.length() && arr[start] != ']') {
            size_t next = arr.find_first_of(",]", start);
            if (next == std::string::npos) break;
            std::string val = arr.substr(start, next - start);
            try { res.push_back(std::stof(val)); }
            catch (...) { res.push_back(0.0f); }
            start = next + 1;
        }
        return res;
    }

    static std::vector<float> GetJsonFloatArray(const std::string& j, const std::string& key) {
        std::vector<float> res; size_t pos = j.find("\"" + key + "\""); if (pos == std::string::npos) return res;
        pos = j.find(':', pos); if (pos == std::string::npos) return res; pos = j.find('[', pos); if (pos == std::string::npos) return res;
        size_t end = j.find(']', pos); if (end == std::string::npos) return res;
        std::string arr = j.substr(pos + 1, end - pos - 1);
        for (char& c : arr) if (c == ',' || c == '\n' || c == '\r' || c == '\t') c = ' ';
        std::stringstream ss(arr);
        ss.imbue(std::locale("C"));
        float f; while (ss >> f) res.push_back(f);
        return res;
    }

    static std::vector<std::string> SplitArray(const std::string& jsonArray) {
        std::vector<std::string> items;
        if (jsonArray.empty() || jsonArray.front() != '[') return items;
        size_t pos = 1;
        while (pos < jsonArray.length()) {
            pos = jsonArray.find_first_of("{", pos);
            if (pos == std::string::npos) break;
            int depth = 0; size_t end = pos; bool inStr = false;
            for (; end < jsonArray.length(); ++end) {
                if (jsonArray[end] == '"' && (end == 0 || jsonArray[end - 1] != '\\')) inStr = !inStr;
                if (!inStr) {
                    if (jsonArray[end] == '{') depth++;
                    else if (jsonArray[end] == '}') { depth--; if (depth == 0) { items.push_back(jsonArray.substr(pos, end - pos + 1)); pos = end + 1; break; } }
                }
            }
        }
        return items;
    }

    static float ExtractFloat(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\""); if (pos == std::string::npos) return 0.0f;
        pos = json.find(':', pos); if (pos == std::string::npos) return 0.0f; pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
        size_t end = pos;
        while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-' || json[end] == '+' || json[end] == 'e' || json[end] == 'E')) end++;
        if (pos == end) return 0.0f;
        try { return std::stof(json.substr(pos, end - pos)); }
        catch (...) { return 0.0f; }
    }

    static std::string ExtractString(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\""); if (pos == std::string::npos) return "";
        pos = json.find(':', pos); if (pos == std::string::npos) return "";
        pos = json.find('"', pos); if (pos == std::string::npos) return ""; pos++; size_t end = pos;
        while (end < json.length()) { if (json[end] == '"' && json[end - 1] != '\\') break; end++; }
        return json.substr(pos, end - pos);
    }

    static uint32_t PackPOSPACKED3(float x, float y, float z, const float* scale, const float* offset) {
        auto pack11 = [](float v, float o, float s) -> uint32_t {
            if (s < 0.000001f) return 0;
            float norm = (v - o) / (s * 0.0009775171f);
            int32_t val = (int32_t)std::round(norm);
            return (uint32_t)(std::clamp(val, -1024, 1023) & 0x7FF);
            };
        auto pack10 = [](float v, float o, float s) -> uint32_t {
            if (s < 0.000001f) return 0;
            float norm = (v - o) / (s * 0.0019569471f);
            int32_t val = (int32_t)std::round(norm);
            return (uint32_t)(std::clamp(val, -512, 511) & 0x3FF);
            };
        uint32_t ix = pack11(x, offset[0], scale[0]);
        uint32_t iy = pack11(y, offset[1], scale[1]);
        uint32_t iz = pack10(z, offset[2], scale[2]);
        return ix | (iy << 11) | (iz << 22);
    }

    static uint32_t PackNormal(float x, float y, float z) {
        float len = std::sqrt(x * x + y * y + z * z);
        if (len > 0.00001f) { x /= len; y /= len; z /= len; }

        x = std::clamp(x, -1.0f, 1.0f);
        y = std::clamp(y, -1.0f, 1.0f);
        z = std::clamp(z, -1.0f, 1.0f);

        int32_t ix = (int32_t)std::round(x * 1023.0f);
        int32_t iy = (int32_t)std::round(y * 1023.0f);
        int32_t iz = (int32_t)std::round(z * 511.0f);

        uint32_t ux = (uint32_t)ix & 0x7FF;
        uint32_t uy = ((uint32_t)iy & 0x7FF) << 11;
        uint32_t uz = ((uint32_t)iz & 0x3FF) << 22;

        return ux | uy | uz;
    }

    static int16_t CompressUV(float uv) {
        return (int16_t)std::clamp(std::round((uv + 8.0f) * 2048.0f), -32768.0f, 32767.0f);
    }

    static uint32_t ExtractCRC(const std::string& json) {
        size_t pos = json.find("\"crc\"");
        if (pos == std::string::npos) return 0;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return 0;
        pos++;

        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

        if (pos < json.length() && json[pos] == '\"') {
            pos++;
            size_t endPos = json.find('\"', pos);
            if (endPos == std::string::npos) return 0;
            std::string val = json.substr(pos, endPos - pos);
            try { return (uint32_t)std::stoull(val); }
            catch (...) { return 0; }
        }
        else {
            size_t endPos = pos;
            while (endPos < json.length() && (isdigit(json[endPos]) || json[endPos] == '.')) endPos++;
            std::string val = json.substr(pos, endPos - pos);
            try { return (uint32_t)std::stoull(val); }
            catch (...) { return 0; }
        }
    }


    struct Mat4 { float m[16]; };

    static Mat4 Identity() { return { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }; }

    static Mat4 TransformToMat4(const std::vector<float>& t, const std::vector<float>& r, const std::vector<float>& s) {
        Mat4 m = Identity();
        float tx = t.size() >= 3 ? t[0] : 0, ty = t.size() >= 3 ? t[1] : 0, tz = t.size() >= 3 ? t[2] : 0;
        float qx = r.size() >= 4 ? r[0] : 0, qy = r.size() >= 4 ? r[1] : 0, qz = r.size() >= 4 ? r[2] : 0, qw = r.size() >= 4 ? r[3] : 1;
        float sx = s.size() >= 3 ? s[0] : 1, sy = s.size() >= 3 ? s[1] : 1, sz = s.size() >= 3 ? s[2] : 1;

        float xx = qx * qx, yy = qy * qy, zz = qz * qz;
        float xy = qx * qy, xz = qx * qz, yz = qy * qz;
        float wx = qw * qx, wy = qw * qy, wz = qw * qz;

        m.m[0] = (1.0f - 2.0f * (yy + zz)) * sx; m.m[1] = (2.0f * (xy + wz)) * sx;       m.m[2] = (2.0f * (xz - wy)) * sx;
        m.m[4] = (2.0f * (xy - wz)) * sy;       m.m[5] = (1.0f - 2.0f * (xx + zz)) * sy; m.m[6] = (2.0f * (yz + wx)) * sy;
        m.m[8] = (2.0f * (xz + wy)) * sz;       m.m[9] = (2.0f * (yz - wx)) * sz;       m.m[10] = (1.0f - 2.0f * (xx + yy)) * sz;
        m.m[12] = tx; m.m[13] = ty; m.m[14] = tz;
        return m;
    }

    struct Acc { int view, count, compType; size_t offset; };
    struct BView { size_t offset, length; };

    static void ParseNodes(const std::string& json, C3DMeshContent& outMesh) {
        std::string nodesBlock;
        size_t searchPos = 0;
        while (true) {
            size_t foundIdx = json.find("\"nodes\"", searchPos);
            if (foundIdx == std::string::npos) break;

            std::string block = ExtractBlock(json, "nodes", searchPos);
            if (!block.empty() && block.find('{') != std::string::npos) {
                nodesBlock = block;
                break;
            }
            searchPos = foundIdx + 7;
        }

        std::vector<std::string> nodes = SplitArray(nodesBlock);
        if (nodes.empty()) return;

        std::map<int, int> nodeParentMap;
        for (int i = 0; i < nodes.size(); i++) {
            std::vector<float> children = GetJsonFloatArray(nodes[i], "children");
            for (float c : children) nodeParentMap[(int)c] = i;
        }

        std::map<int, int> gltfNodeToFableBone;
        std::string skinsBlock = ExtractBlock(json, "skins");
        if (!skinsBlock.empty()) {
            std::vector<std::string> skinObjs = SplitArray(skinsBlock);
            if (!skinObjs.empty()) {
                std::vector<float> joints = GetJsonFloatArray(skinObjs[0], "joints");
                for (int i = 0; i < joints.size(); i++) {
                    gltfNodeToFableBone[(int)joints[i]] = i;
                }
            }
        }

        std::vector<std::string> tempHelperNames;
        std::vector<std::string> tempDummyNames;

        for (int i = 0; i < nodes.size(); i++) {
            const auto& node = nodes[i];
            std::string extras = ExtractBlock(node, "extras");
            std::string type = ExtractStringClean(extras, "type");
            std::string name = ExtractStringClean(node, "name");

            if (type.empty()) {
                if (name.find("HPNT_") == 0) type = "Helper";
                else if (name.find("HDMY_") == 0) type = "Dummy";
                else if (name.find("GEN_") == 0) type = "Generator";
                else if (name.find("VOL_") == 0) type = "Volume";
            }

            if (type.empty()) continue;

            std::string exportName = name;
            uint32_t nameCrc = 0;

            size_t dotPos = exportName.find_last_of('.');
            if (dotPos != std::string::npos && dotPos + 4 == exportName.length() && isdigit(exportName[dotPos + 1])) {
                exportName = exportName.substr(0, dotPos);
            }

            if (type == "Helper" || type == "Dummy") {
                nameCrc = ExtractCRC(extras);

                if (nameCrc == 0) {
                    if (exportName.find("HPNT_") == 0 || exportName.find("HDMY_") == 0) {
                        std::string crcStrFallback = exportName.substr(5);
                        size_t usPos = crcStrFallback.find('_');
                        if (usPos != std::string::npos) crcStrFallback = crcStrFallback.substr(0, usPos);
                        try { nameCrc = (uint32_t)std::stoull(crcStrFallback); }
                        catch (...) {}
                        exportName = "";
                    }
                    else {
                        nameCrc = CalculateFableCRC(exportName);
                    }
                }
            }
            else if (type == "Generator") {
                if (exportName.find("GEN_") == 0) exportName = exportName.substr(4);
            }

            bool isParentedToBone = false;
            int resolvedBoneIdx = -1;

            int currNode = i;
            while (nodeParentMap.count(currNode)) {
                int parentNode = nodeParentMap[currNode];
                if (gltfNodeToFableBone.count(parentNode)) {
                    resolvedBoneIdx = gltfNodeToFableBone[parentNode];
                    isParentedToBone = true;
                    break;
                }
                currNode = parentNode;
            }

            if (!isParentedToBone && extras.find("\"boneId\"") != std::string::npos) {
                int staleBoneId = (int)ExtractFloatClean(extras, "boneId", -1);

                if (staleBoneId >= 0 && outMesh.BoneCount > 0 && outMesh.BoneIndices.size() == outMesh.BoneCount) {
                    std::vector<uint16_t> sortedFableIDs = outMesh.BoneIndices;
                    std::sort(sortedFableIDs.begin(), sortedFableIDs.end());

                    if (staleBoneId < sortedFableIDs.size()) {
                        uint16_t targetFableID = sortedFableIDs[staleBoneId];

                        for (int bIdx = 0; bIdx < outMesh.BoneCount; bIdx++) {
                            if (outMesh.BoneIndices[bIdx] == targetFableID) {
                                resolvedBoneIdx = bIdx;
                                break;
                            }
                        }
                    }
                }
                if (resolvedBoneIdx == -1) resolvedBoneIdx = staleBoneId;
            }

            if (type == "Helper") {
                CHelperPoint h = {};
                h.NameCRC = nameCrc;
                h.BoneIndex = resolvedBoneIdx;
                std::vector<float> trans = GetJsonFloatArray(node, "translation");
                if (trans.size() >= 3) { h.Pos[0] = trans[0]; h.Pos[1] = trans[1]; h.Pos[2] = trans[2]; }
                outMesh.Helpers.push_back(h);

                if (!exportName.empty()) tempHelperNames.push_back(exportName);
            }
            else if (type == "Dummy" || type == "Generator") {
                float Transform[12] = { 1,0,0, 0,1,0, 0,0,1, 0,0,0 };
                std::vector<float> mat = GetJsonFloatArray(node, "matrix");
                if (mat.size() >= 16) {
                    Transform[0] = mat[0]; Transform[1] = mat[1]; Transform[2] = mat[2];
                    Transform[3] = mat[4]; Transform[4] = mat[5]; Transform[5] = mat[6];
                    Transform[6] = mat[8]; Transform[7] = mat[9]; Transform[8] = mat[10];
                    Transform[9] = mat[12]; Transform[10] = mat[13]; Transform[11] = mat[14];
                }
                else {
                    std::vector<float> scale = GetJsonFloatArray(node, "scale");
                    std::vector<float> rot = GetJsonFloatArray(node, "rotation");
                    std::vector<float> trans = GetJsonFloatArray(node, "translation");
                    float sX = 1.0f, sY = 1.0f, sZ = 1.0f;
                    if (scale.size() >= 3) { sX = scale[0]; sY = scale[1]; sZ = scale[2]; }
                    if (rot.size() >= 4) {
                        float x = rot[0], y = rot[1], z = rot[2], w = rot[3];
                        Transform[0] = (1.0f - 2.0f * (y * y + z * z)) * sX; Transform[1] = (2.0f * (x * y + z * w)) * sX; Transform[2] = (2.0f * (x * z - y * w)) * sX;
                        Transform[3] = (2.0f * (x * y - z * w)) * sY; Transform[4] = (1.0f - 2.0f * (x * x + z * z)) * sY; Transform[5] = (2.0f * (y * z + x * w)) * sY;
                        Transform[6] = (2.0f * (x * z + y * w)) * sZ; Transform[7] = (2.0f * (y * z - x * w)) * sZ; Transform[8] = (1.0f - 2.0f * (x * x + y * y)) * sZ;
                    }
                    if (trans.size() >= 3) { Transform[9] = trans[0]; Transform[10] = trans[1]; Transform[11] = trans[2]; }
                }

                if (type == "Dummy") {
                    CDummyObject d = {};
                    d.NameCRC = nameCrc;
                    d.BoneIndex = resolvedBoneIdx;
                    memcpy(d.Transform, Transform, 48);
                    outMesh.Dummies.push_back(d);
                    if (!exportName.empty()) tempDummyNames.push_back(exportName);
                }
                else {
                    CMeshGenerator g = {};
                    g.BankIndex = (uint32_t)ExtractFloatClean(extras, "bankId", 0);
                    g.BoneIndex = resolvedBoneIdx;
                    g.UseLocalOrigin = ExtractBool(extras, "useLocalOrigin", false);
                    g.ObjectName = exportName;
                    memcpy(g.Transform, Transform, 48);
                    outMesh.Generators.push_back(g);
                }
            }
            else if (type == "Volume") {
                CMeshVolume v = {};
                v.ID = 0;
                if (name.find("VOL_") == 0) v.Name = name.substr(4); else v.Name = name;
                std::string planesArr = ExtractBlock(extras, "planes");
                std::vector<std::string> planeObjs = SplitArray(planesArr);
                for (const auto& pObj : planeObjs) {
                    CPlane p = {};
                    std::vector<float> n = GetJsonFloatArray(pObj, "n");
                    if (n.size() >= 3) { p.Normal[0] = n[0]; p.Normal[1] = n[1]; p.Normal[2] = n[2]; }
                    p.D = ExtractFloatClean(pObj, "d", 0.0f);
                    v.Planes.push_back(p);
                }
                outMesh.Volumes.push_back(v);
            }
        }

        std::sort(outMesh.Helpers.begin(), outMesh.Helpers.end(), [](const CHelperPoint& a, const CHelperPoint& b) {
            return a.NameCRC < b.NameCRC;
            });

        std::sort(outMesh.Dummies.begin(), outMesh.Dummies.end(), [](const CDummyObject& a, const CDummyObject& b) {
            return a.NameCRC < b.NameCRC;
            });

        std::sort(outMesh.Generators.begin(), outMesh.Generators.end(), [](const CMeshGenerator& a, const CMeshGenerator& b) {
            uint32_t crcA = CalculateFableCRC(a.ObjectName);
            uint32_t crcB = CalculateFableCRC(b.ObjectName);
            return crcA < crcB;
            });

        outMesh.HelperPointCount = (uint16_t)outMesh.Helpers.size();
        outMesh.DummyObjectCount = (uint16_t)outMesh.Dummies.size();
        outMesh.MeshGeneratorCount = (uint16_t)outMesh.Generators.size();
        outMesh.MeshVolumeCount = (uint16_t)outMesh.Volumes.size();

        if (outMesh.HelperPointCount > 0 || outMesh.DummyObjectCount > 0) {
            outMesh.PackNames(tempHelperNames, tempDummyNames);
            outMesh.UnpackNames();
        }
        else {
            outMesh.PackedNamesRaw.clear();
        }
    }

    template <typename T_BV, typename T_Acc>
    static void CompileFableClothFromGLTF(
        const std::string& meshExtras, const std::string& pObj,
        const std::vector<uint8_t>& binData, const std::vector<T_BV>& views, const std::vector<T_Acc>& accessors,
        C3DMeshContent& outMesh, int targetPrim, bool applyBlenderFix)
    {
        CClothPrimitive cp = {};
        cp.PrimitiveIndex = targetPrim;
        cp.MaterialIndex = outMesh.Primitives[targetPrim].MaterialIndex;

        CParticleProgram prog;
        prog.IsParsed = true;
        prog.Version = 0x14D; // Must be 333 (0x14D)
        prog.AveragePatchSize = ExtractFloatClean(meshExtras, "Fable_PatchSize", 1.0f);
        prog.BezierEnable = (int)ExtractFloatClean(meshExtras, "Fable_Bezier", 0);

        auto& sim = prog.InitialSimulation;
        sim.GravityStrength = ExtractFloatClean(meshExtras, "Fable_Gravity", 9.8f);
        sim.WindStrength = ExtractFloatClean(meshExtras, "Fable_Wind", 0.0f);
        sim.GlobalDamping = ExtractFloatClean(meshExtras, "Fable_Damping", 0.1f);
        sim.Timestep = ExtractFloatClean(meshExtras, "Fable_Timestep", 0.0666f); // 15Hz Fable default
        sim.TimestepChanged = 0;
        sim.TimestepMultiplier = ExtractFloatClean(meshExtras, "Fable_TimeMult", 1.0f);
        sim.DraggingEnable = (int)ExtractFloatClean(meshExtras, "Fable_DragEnable", 0);
        sim.DraggingRotational = (int)ExtractFloatClean(meshExtras, "Fable_DragRotational", 0);
        sim.DraggingStrength = ExtractFloatClean(meshExtras, "Fable_DragStrength", 0.0f);
        sim.AccelerationEnable = (int)ExtractFloatClean(meshExtras, "Fable_AccelEnable", 0);

        float structStiffness = ExtractFloatClean(meshExtras, "Fable_StructStiff", 0.8f);

        std::string attr = ExtractBlock(pObj, "attributes");
        int posAccIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
        int uvAccIdx = (int)ExtractFloatClean(attr, "TEXCOORD_0", -1);
        int colAccIdx = (int)ExtractFloatClean(attr, "COLOR_0", -1);
        int jntAccIdx = (int)ExtractFloatClean(attr, "JOINTS_0", -1);
        int wgtAccIdx = (int)ExtractFloatClean(attr, "WEIGHTS_0", -1);
        int indAccIdx = (int)ExtractFloatClean(pObj, "indices", -1);

        if (posAccIdx < 0 || indAccIdx < 0) return;

        T_Acc pAcc = accessors[posAccIdx];
        const float* pData = (const float*)(binData.data() + views[pAcc.view].offset + pAcc.offset);
        const float* uData = uvAccIdx >= 0 ? (const float*)(binData.data() + views[accessors[uvAccIdx].view].offset + accessors[uvAccIdx].offset) : nullptr;
        const float* cData = colAccIdx >= 0 ? (const float*)(binData.data() + views[accessors[colAccIdx].view].offset + accessors[colAccIdx].offset) : nullptr;

        T_Acc iAcc = accessors[indAccIdx];
        const uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;
        auto getVIdx = [&](int offset, int i) -> uint32_t {
            if (iAcc.compType == 5121) return iData[i + offset];
            if (iAcc.compType == 5123) return ((const uint16_t*)iData)[i + offset];
            if (iAcc.compType == 5125) return ((const uint32_t*)iData)[i + offset];
            return 0;
            };

        auto& prim = outMesh.Primitives[targetPrim];

        // ====================================================================
        // 0. ENGINE REQUIREMENT: DECOMPRESS VERTEX BUFFER (BASE VERTS ONLY)
        // ====================================================================
        if ((prim.InitFlags & 4) != 0 && (prim.InitFlags & 0x10) == 0) {
            std::vector<uint8_t> newVB;
            int newStride = prim.VertexStride + 8; // Shift from 20 bytes to 28 bytes
            newVB.resize(prim.VertexCount * newStride);

            for (uint32_t v = 0; v < prim.VertexCount; v++) {
                uint8_t* oldVert = prim.VertexBuffer.data() + v * prim.VertexStride;
                uint8_t* newVert = newVB.data() + v * newStride;

                float px, py, pz;
                UnpackPOSPACKED3(*(uint32_t*)oldVert, prim.Compression.Scale, prim.Compression.Offset, px, py, pz);

                memcpy(newVert, &px, 4);
                memcpy(newVert + 4, &py, 4);
                memcpy(newVert + 8, &pz, 4);

                memcpy(newVert + 12, oldVert + 4, prim.VertexStride - 4);
            }

            prim.VertexBuffer = newVB;
            prim.VertexStride = newStride;
            prim.InitFlags |= 0x10;
            prim.IsCompressed = false;

            for (int i = 0; i < 4; i++) { prim.Compression.Scale[i] = 1.0f; prim.Compression.Offset[i] = 0.0f; }
        }

        // ====================================================================
        // 1. CLASSIFY GLTF VERTICES (0 = Rigid, 1 = Anchor, 2 = Sim)
        // ====================================================================
        std::vector<int> gltfType(pAcc.count, 0);
        std::vector<float> gltfAlpha(pAcc.count, 0.0f);

        for (int i = 0; i < pAcc.count; i++) {
            float g = cData ? cData[i * 4 + 1] : 0.0f;
            if (g > 0.01f) { gltfType[i] = 2; gltfAlpha[i] = g; }
        }

        for (int i = 0; i < iAcc.count; i += 3) {
            uint32_t i0 = getVIdx(0, i), i1 = getVIdx(1, i), i2 = getVIdx(2, i);
            bool hasSim = (gltfType[i0] == 2) || (gltfType[i1] == 2) || (gltfType[i2] == 2);
            if (hasSim) {
                if (gltfType[i0] == 0) gltfType[i0] = 1;
                if (gltfType[i1] == 0) gltfType[i1] = 1;
                if (gltfType[i2] == 0) gltfType[i2] = 1;
            }
        }

        // ====================================================================
        // 2. BUILD UNIQUE TOPOLOGICAL POINTS & DUPLICATE ANCHORS
        // ====================================================================
        struct UPoint { float p[3]; int type; float alpha; uint32_t fableIdx; uint32_t physLocalIdx; };
        std::vector<UPoint> pts;
        std::vector<uint32_t> gltfToUPoint(pAcc.count, 0xFFFFFFFF);

        for (int i = 0; i < pAcc.count; i++) {
            float px = pData[i * 3], py = pData[i * 3 + 1], pz = pData[i * 3 + 2];
            if (applyBlenderFix) { float tmp = py; py = -pz; pz = tmp; }

            int found = -1;
            for (int k = 0; k < pts.size(); k++) {
                if (std::abs(pts[k].p[0] - px) < 0.001f && std::abs(pts[k].p[1] - py) < 0.001f && std::abs(pts[k].p[2] - pz) < 0.001f) {
                    found = k; break;
                }
            }
            if (found == -1) {
                pts.push_back({ {px,py,pz}, gltfType[i], gltfAlpha[i], 0, 0 });
                found = (int)pts.size() - 1;
            }
            else {
                if ((pts[found].type == 0 && gltfType[i] == 2) || (pts[found].type == 2 && gltfType[i] == 0)) {
                    pts[found].type = 1;
                }
                else if (gltfType[i] == 1) {
                    pts[found].type = 1;
                }
                else if (gltfType[i] == 2 && pts[found].type != 1) {
                    pts[found].type = 2;
                }

                if (pts[found].type == 1) pts[found].alpha = 0.0f;
                else if (pts[found].type == 2) pts[found].alpha = (std::max)(pts[found].alpha, gltfAlpha[i]);
            }
            gltfToUPoint[i] = found;
        }

        prog.NonSimCount = 0;
        sim.Size = 0;

        for (auto& pt : pts) {
            if (pt.type == 0 || pt.type == 1) {
                pt.fableIdx = prog.NonSimCount++;
                prog.NonSimPositions.push_back(pt.p[0]); prog.NonSimPositions.push_back(pt.p[1]); prog.NonSimPositions.push_back(pt.p[2]);
            }
        }
        uint32_t nonSimTotal = prog.NonSimCount;

        for (auto& pt : pts) {
            if (pt.type == 1) {
                pt.physLocalIdx = sim.Size++;
                sim.Positions.push_back(pt.p[0]); sim.Positions.push_back(pt.p[1]); sim.Positions.push_back(pt.p[2]);
                sim.SimulationAlphas.push_back(0.0f);
            }
        }
        for (auto& pt : pts) {
            if (pt.type == 2) {
                pt.fableIdx = nonSimTotal + sim.Size;
                pt.physLocalIdx = sim.Size++;
                sim.Positions.push_back(pt.p[0]); sim.Positions.push_back(pt.p[1]); sim.Positions.push_back(pt.p[2]);
                sim.SimulationAlphas.push_back(pt.alpha);
            }
        }

        // ====================================================================
        // 3. GENERATE RENDER MAPPINGS
        // ====================================================================
        for (uint32_t i = 0; i < pts.size(); i++) {
            C3DClothRenderVertex rv;
            rv.PositionIndex = pts[i].fableIdx;
            rv.TexCoordIndex = pts[i].fableIdx;
            prog.RenderVertices.push_back(rv);
        }

        std::set<std::pair<uint32_t, uint32_t>> uniqueEdges;

        for (int i = 0; i < iAcc.count; i += 3) {
            uint32_t u0 = gltfToUPoint[getVIdx(0, i)];
            uint32_t u1 = gltfToUPoint[getVIdx(1, i)];
            uint32_t u2 = gltfToUPoint[getVIdx(2, i)];

            if (pts[u0].type == 2 || pts[u1].type == 2 || pts[u2].type == 2) {
                C3DTriangle2 tri;
                tri.Indices[0] = u0;
                tri.Indices[1] = u1;
                tri.Indices[2] = u2;
                prog.RenderTris.push_back(tri);

                auto addEdge = [&](uint32_t pA, uint32_t pB) {
                    if (pts[pA].type != 2 && pts[pB].type != 2) return;
                    uniqueEdges.insert({ (std::min)(pA, pB), (std::max)(pA, pB) });
                    };
                addEdge(u0, u1); addEdge(u1, u2); addEdge(u2, u0);
            }
        }

        // ====================================================================
        // 4. TOPOLOGICAL BFS SORT (SPRING GENERATION)
        // ====================================================================
        std::vector<std::vector<uint32_t>> adj(sim.Size);
        std::vector<std::pair<uint32_t, uint32_t>> edgeList;
        for (const auto& edge : uniqueEdges) {
            uint32_t phys1 = pts[edge.first].physLocalIdx;
            uint32_t phys2 = pts[edge.second].physLocalIdx;
            adj[phys1].push_back(phys2); adj[phys2].push_back(phys1);
            edgeList.push_back({ phys1, phys2 });
        }

        std::vector<int> distances(sim.Size, -1);
        std::queue<uint32_t> bfsQueue;

        for (uint32_t i = 0; i < sim.Size; ++i) {
            if (sim.SimulationAlphas[i] < 0.0001f) { distances[i] = 0; bfsQueue.push(i); }
        }

        while (!bfsQueue.empty()) {
            uint32_t curr = bfsQueue.front(); bfsQueue.pop();
            for (uint32_t neighbor : adj[curr]) {
                if (distances[neighbor] == -1) { distances[neighbor] = distances[curr] + 1; bfsQueue.push(neighbor); }
            }
        }

        struct SortedEdge { uint32_t p1, p2; int sortKey; float restLength; };
        std::vector<SortedEdge> sortedEdges;

        for (const auto& edge : edgeList) {
            int d1 = distances[edge.first] == -1 ? 9999 : distances[edge.first];
            int d2 = distances[edge.second] == -1 ? 9999 : distances[edge.second];
            float x1 = sim.Positions[edge.first * 3], y1 = sim.Positions[edge.first * 3 + 1], z1 = sim.Positions[edge.first * 3 + 2];
            float x2 = sim.Positions[edge.second * 3], y2 = sim.Positions[edge.second * 3 + 1], z2 = sim.Positions[edge.second * 3 + 2];
            float restLength = std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2) + std::pow(z1 - z2, 2));
            sortedEdges.push_back({ edge.first, edge.second, d1 + d2, restLength });
        }

        std::sort(sortedEdges.begin(), sortedEdges.end(), [](const SortedEdge& a, const SortedEdge& b) { return a.sortKey < b.sortKey; });

        prog.ParsedInstructions.clear();
        for (const auto& edge : sortedEdges) {
            CConstraintInstruction distConst;
            distConst.Opcode = 2; distConst.Count = 1; distConst.PayloadSize = 16;
            distConst.Payload.resize(16);
            memcpy(distConst.Payload.data(), &edge.p1, 4);
            memcpy(distConst.Payload.data() + 4, &edge.p2, 4);
            memcpy(distConst.Payload.data() + 8, &edge.restLength, 4);
            memcpy(distConst.Payload.data() + 12, &structStiffness, 4);
            prog.ParsedInstructions.push_back(distConst);
        }

        // ====================================================================
        // 5. BIND FABLE MEMORY ARRAYS
        // ====================================================================
        uint32_t fableTotalMemorySize = prog.NonSimCount + sim.Size;
        prog.IndexedTextureCoords.resize(fableTotalMemorySize, { 0.0f, 0.0f });

        for (int i = 0; i < pAcc.count; i++) {
            UPoint& pt = pts[gltfToUPoint[i]];
            if (uData && prog.IndexedTextureCoords[pt.fableIdx].u == 0.0f) {
                prog.IndexedTextureCoords[pt.fableIdx].u = uData[i * 2];
                prog.IndexedTextureCoords[pt.fableIdx].v = uData[i * 2 + 1];
                if (pt.type == 1) {
                    prog.IndexedTextureCoords[nonSimTotal + pt.physLocalIdx].u = uData[i * 2];
                    prog.IndexedTextureCoords[nonSimTotal + pt.physLocalIdx].v = uData[i * 2 + 1];
                }
            }
        }

        prog.ParticleIndices.resize(prim.VertexCount, 0xFFFFFFFF);
        prog.VertexIndices.resize(fableTotalMemorySize, 0xFFFFFFFF);

        // MAP ONLY BASE VERTICES
        for (uint32_t v = 0; v < prim.VertexCount; v++) {
            size_t off = v * prim.VertexStride;
            float* fp = (float*)(prim.VertexBuffer.data() + off);
            float px = fp[0], py = fp[1], pz = fp[2];

            uint32_t bestGltf = 0xFFFFFFFF; float bestDist = 1e9f;
            for (int i = 0; i < pAcc.count; i++) {
                float gx = pData[i * 3], gy = pData[i * 3 + 1], gz = pData[i * 3 + 2];
                if (applyBlenderFix) { float tmp = gy; gy = -gz; gz = tmp; }
                float d = std::pow(px - gx, 2) + std::pow(py - gy, 2) + std::pow(pz - gz, 2);
                if (d < bestDist) { bestDist = d; bestGltf = i; }
            }
            if (bestGltf != 0xFFFFFFFF && bestDist < 0.05f) {
                UPoint& pt = pts[gltfToUPoint[bestGltf]];
                prog.ParticleIndices[v] = pt.fableIdx;

                prog.VertexIndices[pt.fableIdx] = v;
                if (pt.type == 1 || pt.type == 2) {
                    prog.VertexIndices[nonSimTotal + pt.physLocalIdx] = v;
                }
            }
        }

        if (jntAccIdx >= 0 && wgtAccIdx >= 0) {
            T_Acc jAcc = accessors[jntAccIdx], wAcc = accessors[wgtAccIdx];
            const uint16_t* jData16 = (jAcc.compType == 5123) ? (const uint16_t*)(binData.data() + views[jAcc.view].offset + jAcc.offset) : nullptr;
            const uint8_t* jData8 = (jAcc.compType == 5121) ? (const uint8_t*)(binData.data() + views[jAcc.view].offset + jAcc.offset) : nullptr;
            const float* wData = (const float*)(binData.data() + views[wAcc.view].offset + wAcc.offset);

            std::map<uint32_t, std::vector<C3DVertexBlend2>> simBoneMap, nonSimBoneMap;
            std::vector<bool> processedBones(fableTotalMemorySize, false);

            for (int i = 0; i < pAcc.count; i++) {
                UPoint& pt = pts[gltfToUPoint[i]];

                for (int pass = 0; pass < 2; pass++) {
                    if (pass == 1 && pt.type != 1) continue;

                    uint32_t fIdx = (pass == 0) ? pt.fableIdx : (nonSimTotal + pt.physLocalIdx);
                    if (processedBones[fIdx]) continue;
                    processedBones[fIdx] = true;

                    for (int k = 0; k < 4; k++) {
                        float w = wData[i * 4 + k];
                        if (w > 0.001f) {
                            uint32_t bone = jData16 ? jData16[i * 4 + k] : (jData8 ? jData8[i * 4 + k] : 0);
                            uint32_t fBoneId = bone < outMesh.BoneIndices.size() ? outMesh.BoneIndices[bone] : bone;

                            if (pt.type == 2 || pass == 1) simBoneMap[fBoneId].push_back({ pt.physLocalIdx, w });
                            else nonSimBoneMap[fBoneId].push_back({ pt.fableIdx, w });
                        }
                    }
                }
            }
            for (auto& kv : nonSimBoneMap) { C3DGroup2 g; g.BoneIndex = kv.first; g.VertexBlends = kv.second; prog.NonSimGroups.push_back(g); }
            for (auto& kv : simBoneMap) { C3DGroup2 g; g.BoneIndex = kv.first; g.VertexBlends = kv.second; prog.SimGroups.push_back(g); }
        }

        cp.Program = prog;
        outMesh.Primitives[targetPrim].ClothPrimitives.push_back(cp);
        outMesh.ClothFlag = true;
    }

    static std::string ImportType1(const std::string& gltfPath, const std::string& originalName, C3DMeshContent& outMesh, bool forceRecalculate = false) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Could not open glTF file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of('.')) + ".bin";
        std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
        if (!binFile.is_open()) return "Could not open associated .bin file.";
        std::streamsize size = binFile.tellg();
        binFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> binData((size_t)size);
        if (!binFile.read((char*)binData.data(), size)) return "Failed to read .bin data.";

        outMesh = C3DMeshContent();
        outMesh.MeshName = originalName;
        outMesh.MeshType = 1;
        outMesh.AnimatedFlag = 0;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        std::string nodesBlock = ExtractBlock(json, "nodes");
        std::vector<std::string> nodeObjs = SplitArray(nodesBlock);
        for (const auto& node : nodeObjs) {
            std::string nName = ExtractStringClean(node, "name");
            if (nName == "Scene_Root" || nName == "Scene Root") {
                std::vector<float> mat = GetJsonFloatArray(node, "matrix");
                if (mat.size() >= 16) {
                    outMesh.RootMatrix[0] = mat[0]; outMesh.RootMatrix[1] = mat[1]; outMesh.RootMatrix[2] = mat[2];
                    outMesh.RootMatrix[3] = mat[4]; outMesh.RootMatrix[4] = mat[5]; outMesh.RootMatrix[5] = mat[6];
                    outMesh.RootMatrix[6] = mat[8]; outMesh.RootMatrix[7] = mat[9]; outMesh.RootMatrix[8] = mat[10];
                    outMesh.RootMatrix[9] = mat[12]; outMesh.RootMatrix[10] = mat[13]; outMesh.RootMatrix[11] = mat[14];
                }
                else {
                    std::vector<float> t = GetJsonFloatArray(node, "translation");
                    std::vector<float> r = GetJsonFloatArray(node, "rotation");
                    std::vector<float> s = GetJsonFloatArray(node, "scale");
                    Mat4 rMat = TransformToMat4(t, r, s);

                    outMesh.RootMatrix[0] = rMat.m[0]; outMesh.RootMatrix[1] = rMat.m[1]; outMesh.RootMatrix[2] = rMat.m[2];
                    outMesh.RootMatrix[3] = rMat.m[4]; outMesh.RootMatrix[4] = rMat.m[5]; outMesh.RootMatrix[5] = rMat.m[6];
                    outMesh.RootMatrix[6] = rMat.m[8]; outMesh.RootMatrix[7] = rMat.m[9]; outMesh.RootMatrix[8] = rMat.m[10];
                    outMesh.RootMatrix[9] = rMat.m[12]; outMesh.RootMatrix[10] = rMat.m[13]; outMesh.RootMatrix[11] = rMat.m[14];
                }
                break;
            }
        }

        std::vector<std::string> viewObjs = SplitArray(ExtractBlock(json, "bufferViews"));
        struct BV { int offset, length; }; std::vector<BV> views;
        for (const auto& v : viewObjs) views.push_back({ (int)ExtractFloatClean(v, "byteOffset", 0), (int)ExtractFloatClean(v, "byteLength", 0) });

        std::vector<std::string> accObjs = SplitArray(ExtractBlock(json, "accessors"));
        struct Acc { int view, offset, count, compType; }; std::vector<Acc> accessors;
        for (const auto& a : accObjs) accessors.push_back({ (int)ExtractFloatClean(a, "bufferView", 0), (int)ExtractFloatClean(a, "byteOffset", 0), (int)ExtractFloatClean(a, "count", 0), (int)ExtractFloatClean(a, "componentType", 0) });

        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outMesh.Materials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                C3DMaterial m = {}; m.ID = i; m.Name = ExtractStringClean(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);

                std::string extras = ExtractBlock(matObjs[i], "extras");
                if (!extras.empty()) {
                    m.DiffuseMapID = (int)ExtractFloatClean(extras, "DiffuseMapID", 0);
                    m.BumpMapID = (int)ExtractFloatClean(extras, "BumpMapID", 0);
                    m.ReflectionMapID = (int)ExtractFloatClean(extras, "ReflectionMapID", 0);
                    m.IlluminationMapID = (int)ExtractFloatClean(extras, "IlluminationMapID", 0);
                    m.MapFlags = (int)ExtractFloatClean(extras, "MapFlags", 0);
                    m.IsTwoSided = ExtractBool(extras, "IsTwoSided", true);
                    m.IsTransparent = ExtractBool(extras, "IsTransparent", false);
                    m.BooleanAlpha = ExtractBool(extras, "BooleanAlpha", false);
                    m.DegenerateTriangles = ExtractBool(extras, "DegenerateTriangles", false);
                }
                else { m.IsTwoSided = true; m.IsTransparent = false; }
                outMesh.Materials.push_back(m);
            }
        }
        else {
            C3DMaterial defMat = {}; defMat.ID = 0; defMat.Name = "Default_Material"; defMat.IsTwoSided = true;
            outMesh.Materials.push_back(defMat);
        }
        outMesh.MaterialCount = (int32_t)outMesh.Materials.size();

        float globalMin[3] = { 1e9f, 1e9f, 1e9f };
        float globalMax[3] = { -1e9f, -1e9f, -1e9f };
        std::vector<std::string> preMeshObjs = SplitArray(ExtractBlock(json, "meshes"));
        for (const auto& meshObj : preMeshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            for (const auto& pObj : primObjs) {
                std::string attr = ExtractBlock(pObj, "attributes");
                int pIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                if (pIdx < 0 || pIdx >= accessors.size()) continue;
                Acc pAcc = accessors[pIdx];
                float* pData = (float*)(binData.data() + views[pAcc.view].offset + pAcc.offset);
                for (int v = 0; v < pAcc.count; v++) {
                    for (int j = 0; j < 3; j++) {
                        float val = pData[v * 3 + j];
                        if (val < globalMin[j]) globalMin[j] = val;
                        if (val > globalMax[j]) globalMax[j] = val;
                    }
                }
            }
        }

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        for (int mIdx = 0; mIdx < meshObjs.size(); mIdx++) {
            const auto& meshObj = meshObjs[mIdx];
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            std::string meshExtras = ExtractBlock(meshObj, "extras");
            //if (ExtractBool(meshExtras, "FableCloth", false)) continue; // Processed later!

            C3DPrimitive outPrim = {};
            outPrim.InitFlags = 4;
            outPrim.IsCompressed = true;
            outPrim.BufferType = 0;
            outPrim.VertexStride = 12;
            outPrim.MaterialIndex = -1;

            outPrim.AvgTextureStretch = ExtractFloatClean(meshExtras, "AvgTextureStretch", 0.1f);
            outPrim.SphereRadius = ExtractFloatClean(meshExtras, "SphereRadius", 0.0f);
            memset(outPrim.SphereCenter, 0, 12);

            struct BaseVertex { float p[3], n[3], u[2]; };
            std::vector<BaseVertex> uniqueVerts;
            std::vector<uint32_t> mergedBaseIndices;
            std::vector<CStaticBlock> baseBlocks;

            for (const auto& pObj : primObjs) {
                int matIdx = (int)ExtractFloatClean(pObj, "material", 0);
                if (matIdx < 0 || matIdx >= outMesh.MaterialCount) matIdx = 0;
                if (outPrim.MaterialIndex == -1) outPrim.MaterialIndex = matIdx;

                std::string attr = ExtractBlock(pObj, "attributes");
                int posAccIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                int normAccIdx = (int)ExtractFloatClean(attr, "NORMAL", -1);
                int uvAccIdx = (int)ExtractFloatClean(attr, "TEXCOORD_0", -1);
                int indAccIdx = (int)ExtractFloatClean(pObj, "indices", -1);

                if (posAccIdx < 0 || indAccIdx < 0) continue;

                Acc posAcc = accessors[posAccIdx];
                Acc iAcc = accessors[indAccIdx];
                float* pData = (float*)(binData.data() + views[posAcc.view].offset + posAcc.offset);
                float* nData = normAccIdx >= 0 ? (float*)(binData.data() + views[accessors[normAccIdx].view].offset + accessors[normAccIdx].offset) : nullptr;
                float* uData = uvAccIdx >= 0 ? (float*)(binData.data() + views[accessors[uvAccIdx].view].offset + accessors[uvAccIdx].offset) : nullptr;
                uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;

                uint32_t startIdx = (uint32_t)mergedBaseIndices.size();

                for (int i = 0; i < iAcc.count; i += 3) {
                    auto getVIdx = [&](int offset) -> uint32_t {
                        if (i + offset >= iAcc.count) return 0xFFFFFFFF;
                        if (iAcc.compType == 5121) return iData[i + offset];
                        if (iAcc.compType == 5123) return ((uint16_t*)iData)[i + offset];
                        if (iAcc.compType == 5125) return ((uint32_t*)iData)[i + offset];
                        return 0xFFFFFFFF;
                        };

                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) };

                    for (int j = 0; j < 3; j++) {
                        uint32_t vIdx = idxs[j];
                        if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                        BaseVertex v = {};
                        v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                        if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                        else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; }
                        if (uData) { v.u[0] = uData[vIdx * 2]; v.u[1] = uData[vIdx * 2 + 1]; }

                        bool found = false;
                        for (size_t k = 0; k < uniqueVerts.size(); ++k) {
                            if (memcmp(&uniqueVerts[k], &v, sizeof(BaseVertex)) == 0) {
                                mergedBaseIndices.push_back((uint32_t)k);
                                found = true; break;
                            }
                        }
                        if (!found) {
                            mergedBaseIndices.push_back((uint32_t)uniqueVerts.size());
                            uniqueVerts.push_back(v);
                        }
                    }
                }
                if (iAcc.count > 0) {
                    CStaticBlock sb = {}; sb.PrimitiveCount = iAcc.count / 3; sb.StartIndex = startIdx; sb.MaterialIndex = matIdx;
                    baseBlocks.push_back(sb);
                }
            }

            if (uniqueVerts.empty()) continue;

            std::vector<float> savedScale = GetJsonFloatArray(meshExtras, "compScale");
            std::vector<float> savedOffset = GetJsonFloatArray(meshExtras, "compOffset");

            if (!forceRecalculate && savedScale.size() >= 3 && savedOffset.size() >= 3) {
                outPrim.Compression.Scale[0] = savedScale[0];
                outPrim.Compression.Scale[1] = savedScale[1];
                outPrim.Compression.Scale[2] = savedScale[2];
                outPrim.Compression.Offset[0] = savedOffset[0];
                outPrim.Compression.Offset[1] = savedOffset[1];
                outPrim.Compression.Offset[2] = savedOffset[2];
            }
            else {
                for (int j = 0; j < 3; j++) {
                    outPrim.Compression.Offset[j] = (globalMax[j] + globalMin[j]) * 0.5f;
                    outPrim.Compression.Scale[j] = (globalMax[j] - globalMin[j]) * 0.505f;
                    if (outPrim.Compression.Scale[j] < 0.0001f) outPrim.Compression.Scale[j] = 0.0001f;
                }
            }
            outPrim.Compression.Scale[3] = 1.0f;
            outPrim.Compression.Offset[3] = 0.0f;
            outPrim.VertexCount = (uint32_t)uniqueVerts.size();
            outPrim.VertexBuffer.resize(outPrim.VertexCount * outPrim.VertexStride, 0);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (uint32_t v = 0; v < outPrim.VertexCount; v++) {
                uint32_t pP = PackPOSPACKED3(uniqueVerts[v].p[0], uniqueVerts[v].p[1], uniqueVerts[v].p[2], outPrim.Compression.Scale, outPrim.Compression.Offset);
                uint32_t pN = PackNormal(uniqueVerts[v].n[0], uniqueVerts[v].n[1], uniqueVerts[v].n[2]);
                int16_t cU = CompressUV(uniqueVerts[v].u[0]), cV = CompressUV(uniqueVerts[v].u[1]);

                memcpy(vDest + 0, &pP, 4); memcpy(vDest + 4, &pN, 4);
                memcpy(vDest + 8, &cU, 2); memcpy(vDest + 10, &cV, 2);
                vDest += outPrim.VertexStride;
            }

            outPrim.IndexCount = (uint32_t)mergedBaseIndices.size();
            outPrim.TriangleCount = outPrim.IndexCount / 3;
            outPrim.IndexBuffer.resize(outPrim.IndexCount);
            for (size_t i = 0; i < mergedBaseIndices.size(); i++) outPrim.IndexBuffer[i] = (uint16_t)mergedBaseIndices[i];

            outPrim.StaticBlocks = baseBlocks;
            outPrim.StaticBlockCount = (uint32_t)outPrim.StaticBlocks.size();
            outPrim.StaticBlockCount_2 = outPrim.StaticBlockCount;

            outMesh.Primitives.push_back(outPrim);

            if (ExtractBool(meshExtras, "FableCloth", false)) {
                int targetPrim = (int)outMesh.Primitives.size() - 1;
                // Type 1 usually doesn't need the axis fix, so we pass 'false'
                CompileFableClothFromGLTF(meshExtras, primObjs[0], binData, views, accessors, outMesh, targetPrim, false);
            }

        }

        std::sort(outMesh.Primitives.begin(), outMesh.Primitives.end(), [&](const C3DPrimitive& a, const C3DPrimitive& b) {
            bool aT = false, bT = false;
            if (a.MaterialIndex >= 0 && a.MaterialIndex < outMesh.MaterialCount) aT = outMesh.Materials[a.MaterialIndex].IsTransparent || outMesh.Materials[a.MaterialIndex].BooleanAlpha;
            if (b.MaterialIndex >= 0 && b.MaterialIndex < outMesh.MaterialCount) bT = outMesh.Materials[b.MaterialIndex].IsTransparent || outMesh.Materials[b.MaterialIndex].BooleanAlpha;
            return aT < bT;
            });

        outMesh.TotalStaticBlocks = 0;
        for (const auto& prim : outMesh.Primitives)
            outMesh.TotalStaticBlocks += (uint16_t)prim.StaticBlockCount;

        ParseNodes(json, outMesh);

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Type 1 Static Mesh";

        return "";
    }

    static std::string ImportType2(const std::string& gltfPath, const std::string& bankEntryName, C3DMeshContent& outMesh, int reps = 32) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Failed to open .gltf file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string buffersBlock = ExtractBlock(json, "buffers");
        std::vector<std::string> bufferObjs = SplitArray(buffersBlock);
        if (bufferObjs.empty()) return "No 'buffers' array found.";
        std::string uri = ExtractString(bufferObjs[0], "uri");

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of("\\/") + 1) + uri;
        std::ifstream binFile(binPath, std::ios::binary);
        if (!binFile.is_open()) return "Failed to open associated .bin file: " + uri;
        std::vector<uint8_t> binData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());

        std::vector<BView> views;
        for (const auto& obj : SplitArray(ExtractBlock(json, "bufferViews")))
            views.push_back({ (size_t)ExtractFloat(obj, "byteOffset"), (size_t)ExtractFloat(obj, "byteLength") });

        std::vector<Acc> accessors;
        for (const auto& obj : SplitArray(ExtractBlock(json, "accessors"))) {
            Acc a; a.view = (int)ExtractFloat(obj, "bufferView"); a.count = (int)ExtractFloat(obj, "count");
            a.compType = (int)ExtractFloat(obj, "componentType"); a.offset = (size_t)ExtractFloat(obj, "byteOffset");
            accessors.push_back(a);
        }

        outMesh = C3DMeshContent();
        outMesh.MeshName = bankEntryName;
        outMesh.MeshType = 2;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outMesh.Materials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                C3DMaterial m = {};
                m.ID = i;
                m.Name = ExtractString(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);

                std::string extras = ExtractBlock(matObjs[i], "extras");
                if (!extras.empty()) {
                    m.DiffuseMapID = (int)ExtractFloat(extras, "DiffuseMapID");
                    m.BumpMapID = (int)ExtractFloat(extras, "BumpMapID");
                    m.ReflectionMapID = (int)ExtractFloat(extras, "ReflectionMapID");
                    m.IlluminationMapID = (int)ExtractFloat(extras, "IlluminationMapID");
                    m.MapFlags = (int)ExtractFloat(extras, "MapFlags");
                    m.IsTwoSided = ExtractBool(extras, "IsTwoSided", true);
                    m.IsTransparent = ExtractBool(extras, "IsTransparent", false);
                    m.BooleanAlpha = ExtractBool(extras, "BooleanAlpha", false);
                    m.DegenerateTriangles = ExtractBool(extras, "DegenerateTriangles", false);
                }
                else {
                    m.IsTwoSided = true;
                    m.IsTransparent = true;
                }
                outMesh.Materials.push_back(m);
            }
        }
        else {
            C3DMaterial defMat = {};
            defMat.ID = 0; defMat.Name = "Grass_Material"; defMat.IsTwoSided = true; defMat.IsTransparent = true;
            outMesh.Materials.push_back(defMat);
        }
        outMesh.MaterialCount = (int32_t)outMesh.Materials.size();

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        if (meshObjs.empty()) return "No meshes found in glTF.";

        for (const auto& meshObj : meshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            C3DPrimitive outPrim = {};
            outPrim.RepeatingMeshReps = reps;
            outPrim.VertexStride = 36;
            outPrim.InitFlags = 4;
            outPrim.MaterialIndex = -1;

            struct BaseVertex { float p[3], n[3], u[2]; };
            std::vector<BaseVertex> uniqueVerts;
            std::vector<uint32_t> mergedBaseIndices;
            std::vector<CStaticBlock> baseBlocks;

            auto FindOrAddVertex = [&](const BaseVertex& v) -> uint32_t {
                for (size_t i = 0; i < uniqueVerts.size(); ++i) {
                    const auto& uv = uniqueVerts[i];
                    if (std::abs(uv.p[0] - v.p[0]) < 0.002f && std::abs(uv.p[1] - v.p[1]) < 0.002f && std::abs(uv.p[2] - v.p[2]) < 0.002f &&
                        std::abs(uv.n[0] - v.n[0]) < 0.01f && std::abs(uv.n[1] - v.n[1]) < 0.01f && std::abs(uv.n[2] - v.n[2]) < 0.01f &&
                        std::abs(uv.u[0] - v.u[0]) < 0.002f && std::abs(uv.u[1] - v.u[1]) < 0.002f) {
                        return (uint32_t)i;
                    }
                }
                uniqueVerts.push_back(v);
                return (uint32_t)(uniqueVerts.size() - 1);
                };

            for (const auto& pObj : primObjs) {
                int matIdx = (int)ExtractFloatClean(pObj, "material", 0);
                if (matIdx < 0 || matIdx >= outMesh.MaterialCount) matIdx = 0;
                if (outPrim.MaterialIndex == -1) outPrim.MaterialIndex = matIdx;

                std::string attr = ExtractBlock(pObj, "attributes");
                int posAccIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                int normAccIdx = (int)ExtractFloatClean(attr, "NORMAL", -1);
                int uvAccIdx = (int)ExtractFloatClean(attr, "TEXCOORD_0", -1);
                int indAccIdx = (int)ExtractFloatClean(pObj, "indices", -1);

                if (posAccIdx < 0 || indAccIdx < 0) continue;

                Acc posAcc = accessors[posAccIdx];
                Acc iAcc = accessors[indAccIdx];

                float* pData = (float*)(binData.data() + views[posAcc.view].offset + posAcc.offset);
                float* nData = normAccIdx >= 0 ? (float*)(binData.data() + views[accessors[normAccIdx].view].offset + accessors[normAccIdx].offset) : nullptr;
                float* uData = uvAccIdx >= 0 ? (float*)(binData.data() + views[accessors[uvAccIdx].view].offset + accessors[uvAccIdx].offset) : nullptr;
                uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;

                uint32_t startIdx = (uint32_t)mergedBaseIndices.size();

                for (int i = 0; i < iAcc.count; i += 3) {
                    auto getVIdx = [&](int offset) -> uint32_t {
                        if (i + offset >= iAcc.count) return 0xFFFFFFFF;
                        if (iAcc.compType == 5121) return iData[i + offset];
                        if (iAcc.compType == 5123) return ((uint16_t*)iData)[i + offset];
                        if (iAcc.compType == 5125) return ((uint32_t*)iData)[i + offset];
                        return 0xFFFFFFFF;
                        };

                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) };

                    for (int j = 0; j < 3; j++) {
                        uint32_t vIdx = idxs[j];
                        if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                        BaseVertex v = {};
                        v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                        if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                        else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; }

                        if (uData) { v.u[0] = uData[vIdx * 2]; v.u[1] = uData[vIdx * 2 + 1]; }
                        else { v.u[0] = 0; v.u[1] = 0; }

                        mergedBaseIndices.push_back(FindOrAddVertex(v));
                    }
                }

                if (iAcc.count > 0) {
                    CStaticBlock sb = {};
                    sb.PrimitiveCount = iAcc.count / 3;
                    sb.StartIndex = startIdx;
                    sb.MaterialIndex = matIdx;
                    sb.IsStrip = false;
                    baseBlocks.push_back(sb);
                }
            }

            uint32_t baseVertCount = (uint32_t)uniqueVerts.size();
            if (baseVertCount == 0) continue;
            if (baseVertCount * reps > 65535) return "Import Error: Vertex overflow.";
            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * reps * 36);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (int r = 0; r < reps; r++) {
                for (uint32_t v = 0; v < baseVertCount; v++) {
                    memcpy(vDest, &uniqueVerts[v].p, 12); vDest += 12;
                    memcpy(vDest, &uniqueVerts[v].n, 12); vDest += 12;
                    memcpy(vDest, &uniqueVerts[v].u, 8);  vDest += 8;
                    uint32_t instID = r;
                    memcpy(vDest, &instID, 4); vDest += 4;
                }
            }

            outPrim.IndexCount = (uint32_t)mergedBaseIndices.size();
            outPrim.TriangleCount = outPrim.IndexCount / 3;
            outPrim.IndexBuffer.resize(outPrim.IndexCount * reps);

            uint32_t outIdx = 0;
            for (int r = 0; r < reps; r++) {
                for (size_t i = 0; i < mergedBaseIndices.size(); i++) {
                    outPrim.IndexBuffer[outIdx++] = (uint16_t)(mergedBaseIndices[i] + (r * baseVertCount));
                }
            }

            for (const auto& baseSb : baseBlocks) {
                CStaticBlock finalSb = baseSb;
                finalSb.StartIndex = baseSb.StartIndex + (mergedBaseIndices.size() * (reps - 1));
                outPrim.StaticBlocks.push_back(finalSb);
                outMesh.TotalStaticBlocks++;
            }

            outPrim.StaticBlockCount = (uint32_t)outPrim.StaticBlocks.size();
            outPrim.StaticBlockCount_2 = outPrim.StaticBlockCount;
            outMesh.Primitives.push_back(outPrim);
        }

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Type 2 Mesh";

        return "";
    }

    static std::string ImportType4(const std::string& gltfPath, const std::string& originalName, C3DMeshContent& outMesh) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Could not open glTF file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of('.')) + ".bin";
        std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
        if (!binFile.is_open()) return "Could not open associated .bin file.";
        std::streamsize size = binFile.tellg();
        binFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> binData((size_t)size);
        if (!binFile.read((char*)binData.data(), size)) return "Failed to read .bin data.";

        outMesh = C3DMeshContent();
        outMesh.MeshName = originalName;
        outMesh.MeshType = 4;
        outMesh.AnimatedFlag = 0;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        auto ExtractStringParam = [](const std::string& json, const std::string& key) -> std::string {
            size_t pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = json.find(':', pos);
            if (pos == std::string::npos) return "";
            pos = json.find('\"', pos);
            if (pos == std::string::npos) return "";
            size_t end = json.find('\"', pos + 1);
            if (end == std::string::npos) return "";
            return json.substr(pos + 1, end - pos - 1);
            };

        std::string nodesBlock = ExtractBlock(json, "nodes");
        std::vector<std::string> nodeObjs = SplitArray(nodesBlock);
        for (const auto& node : nodeObjs) {
            std::string nName = ExtractStringParam(node, "name");
            if (nName == "Scene_Root" || nName == "Scene Root") {
                std::vector<float> mat = GetJsonFloatArray(node, "matrix");
                if (mat.size() >= 16) {
                    outMesh.RootMatrix[0] = mat[0]; outMesh.RootMatrix[1] = mat[1]; outMesh.RootMatrix[2] = mat[2];
                    outMesh.RootMatrix[3] = mat[4]; outMesh.RootMatrix[4] = mat[5]; outMesh.RootMatrix[5] = mat[6];
                    outMesh.RootMatrix[6] = mat[8]; outMesh.RootMatrix[7] = mat[9]; outMesh.RootMatrix[8] = mat[10];
                    outMesh.RootMatrix[9] = mat[12]; outMesh.RootMatrix[10] = mat[13]; outMesh.RootMatrix[11] = mat[14];
                }
                else {
                    std::vector<float> t = GetJsonFloatArray(node, "translation");
                    std::vector<float> r = GetJsonFloatArray(node, "rotation");
                    std::vector<float> s = GetJsonFloatArray(node, "scale");
                    Mat4 rMat = TransformToMat4(t, r, s);

                    outMesh.RootMatrix[0] = rMat.m[0]; outMesh.RootMatrix[1] = rMat.m[1]; outMesh.RootMatrix[2] = rMat.m[2];
                    outMesh.RootMatrix[3] = rMat.m[4]; outMesh.RootMatrix[4] = rMat.m[5]; outMesh.RootMatrix[5] = rMat.m[6];
                    outMesh.RootMatrix[6] = rMat.m[8]; outMesh.RootMatrix[7] = rMat.m[9]; outMesh.RootMatrix[8] = rMat.m[10];
                    outMesh.RootMatrix[9] = rMat.m[12]; outMesh.RootMatrix[10] = rMat.m[13]; outMesh.RootMatrix[11] = rMat.m[14];
                }
                break;
            }
        }

        std::vector<std::string> viewObjs = SplitArray(ExtractBlock(json, "bufferViews"));
        struct BV { int offset, length; }; std::vector<BV> views;
        for (const auto& v : viewObjs) views.push_back({ (int)ExtractFloatClean(v, "byteOffset", 0), (int)ExtractFloatClean(v, "byteLength", 0) });

        std::vector<std::string> accObjs = SplitArray(ExtractBlock(json, "accessors"));
        struct Acc { int view, offset, count, compType; }; std::vector<Acc> accessors;
        for (const auto& a : accObjs) accessors.push_back({ (int)ExtractFloatClean(a, "bufferView", 0), (int)ExtractFloatClean(a, "byteOffset", 0), (int)ExtractFloatClean(a, "count", 0), (int)ExtractFloatClean(a, "componentType", 0) });

        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outMesh.Materials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                C3DMaterial m = {}; m.ID = i; m.Name = ExtractStringParam(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);
                std::string extras = ExtractBlock(matObjs[i], "extras");
                if (!extras.empty()) {
                    m.DiffuseMapID = (int)ExtractFloatClean(extras, "DiffuseMapID", 0);
                    m.BumpMapID = (int)ExtractFloatClean(extras, "BumpMapID", 0);
                    m.ReflectionMapID = (int)ExtractFloatClean(extras, "ReflectionMapID", 0);
                    m.IlluminationMapID = (int)ExtractFloatClean(extras, "IlluminationMapID", 0);
                    m.MapFlags = (int)ExtractFloatClean(extras, "MapFlags", 0);
                    m.IsTwoSided = ExtractBool(extras, "IsTwoSided", true);
                    m.IsTransparent = ExtractBool(extras, "IsTransparent", false);
                    m.BooleanAlpha = ExtractBool(extras, "BooleanAlpha", false);
                    m.DegenerateTriangles = ExtractBool(extras, "DegenerateTriangles", false);
                }
                else { m.IsTwoSided = true; m.IsTransparent = false; }
                outMesh.Materials.push_back(m);
            }
        }
        else {
            C3DMaterial defMat = {}; defMat.ID = 0; defMat.Name = "Default_Material"; defMat.IsTwoSided = true;
            outMesh.Materials.push_back(defMat);
        }
        outMesh.MaterialCount = (int32_t)outMesh.Materials.size();

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        if (meshObjs.empty()) return "No meshes found in glTF.";

        for (int mIdx = 0; mIdx < meshObjs.size(); mIdx++) {
            const auto& meshObj = meshObjs[mIdx];
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            C3DPrimitive outPrim = {};
            outPrim.InitFlags = 4;
            outPrim.IsCompressed = false;
            outPrim.BufferType = 0;
            outPrim.VertexStride = 36;
            outPrim.MaterialIndex = -1;
            outPrim.RepeatingMeshReps = 1;

            struct BaseVertex { float p[3], n[3], u[2]; };
            std::vector<BaseVertex> uniqueVerts;
            std::vector<uint32_t> mergedBaseIndices;
            std::vector<CStaticBlock> baseBlocks;

            for (const auto& pObj : primObjs) {
                int matIdx = (int)ExtractFloatClean(pObj, "material", 0);
                if (matIdx < 0 || matIdx >= outMesh.MaterialCount) matIdx = 0;
                if (outPrim.MaterialIndex == -1) outPrim.MaterialIndex = matIdx;

                std::string attr = ExtractBlock(pObj, "attributes");
                int posAccIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                int normAccIdx = (int)ExtractFloatClean(attr, "NORMAL", -1);
                int uvAccIdx = (int)ExtractFloatClean(attr, "TEXCOORD_0", -1);
                int indAccIdx = (int)ExtractFloatClean(pObj, "indices", -1);

                if (posAccIdx < 0 || indAccIdx < 0) continue;

                auto FindOrAddVertex = [&](const BaseVertex& v) -> uint32_t {
                    for (size_t i = 0; i < uniqueVerts.size(); ++i) {
                        const auto& uv = uniqueVerts[i];
                        if (std::abs(uv.p[0] - v.p[0]) < 0.002f && std::abs(uv.p[1] - v.p[1]) < 0.002f && std::abs(uv.p[2] - v.p[2]) < 0.002f &&
                            std::abs(uv.n[0] - v.n[0]) < 0.01f && std::abs(uv.n[1] - v.n[1]) < 0.01f && std::abs(uv.n[2] - v.n[2]) < 0.01f &&
                            std::abs(uv.u[0] - v.u[0]) < 0.002f && std::abs(uv.u[1] - v.u[1]) < 0.002f) {
                            return (uint32_t)i;
                        }
                    }
                    uniqueVerts.push_back(v);
                    return (uint32_t)(uniqueVerts.size() - 1);
                    };

                Acc posAcc = accessors[posAccIdx];
                Acc iAcc = accessors[indAccIdx];

                float* pData = (float*)(binData.data() + views[posAcc.view].offset + posAcc.offset);
                float* nData = normAccIdx >= 0 ? (float*)(binData.data() + views[accessors[normAccIdx].view].offset + accessors[normAccIdx].offset) : nullptr;
                float* uData = uvAccIdx >= 0 ? (float*)(binData.data() + views[accessors[uvAccIdx].view].offset + accessors[uvAccIdx].offset) : nullptr;
                uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;

                uint32_t startIdx = (uint32_t)mergedBaseIndices.size();

                for (int i = 0; i < iAcc.count; i += 3) {
                    auto getVIdx = [&](int offset) -> uint32_t {
                        if (i + offset >= iAcc.count) return 0xFFFFFFFF;
                        if (iAcc.compType == 5121) return iData[i + offset];
                        if (iAcc.compType == 5123) return ((uint16_t*)iData)[i + offset];
                        if (iAcc.compType == 5125) return ((uint32_t*)iData)[i + offset];
                        return 0xFFFFFFFF;
                        };

                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) };

                    for (int j = 0; j < 3; j++) {
                        uint32_t vIdx = idxs[j];
                        if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                        BaseVertex v = {};
                        v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                        if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                        else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; }

                        if (uData) { v.u[0] = uData[vIdx * 2]; v.u[1] = uData[vIdx * 2 + 1]; }
                        else { v.u[0] = 0; v.u[1] = 0; }

                        mergedBaseIndices.push_back(FindOrAddVertex(v));
                    }
                }

                if (iAcc.count > 0) {
                    CStaticBlock sb = {};
                    sb.PrimitiveCount = iAcc.count / 3;
                    sb.StartIndex = startIdx;
                    sb.MaterialIndex = matIdx;
                    baseBlocks.push_back(sb);
                }
            }

            uint32_t baseVertCount = (uint32_t)uniqueVerts.size();
            if (baseVertCount == 0) continue;
            if (baseVertCount > 65535) return "Import Error: Vertex overflow. Fable limit is 65,535.";

            for (int j = 0; j < 4; j++) {
                outPrim.Compression.Scale[j] = 1.0f;
                outPrim.Compression.Offset[j] = 0.0f;
            }

            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * outPrim.VertexStride, 0);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (uint32_t v = 0; v < baseVertCount; v++) {
                float pos[3] = { uniqueVerts[v].p[0], uniqueVerts[v].p[1], uniqueVerts[v].p[2] };
                float norm[3] = { uniqueVerts[v].n[0], uniqueVerts[v].n[1], uniqueVerts[v].n[2] };
                float uv[2] = { uniqueVerts[v].u[0], uniqueVerts[v].u[1] };

                memcpy(vDest + 0, pos, 12);
                memcpy(vDest + 12, norm, 12);
                memcpy(vDest + 24, uv, 8);

                vDest += outPrim.VertexStride;
            }

            outPrim.IndexCount = (uint32_t)mergedBaseIndices.size();
            outPrim.TriangleCount = outPrim.IndexCount / 3;
            outPrim.IndexBuffer.resize(outPrim.IndexCount);
            for (size_t i = 0; i < mergedBaseIndices.size(); i++) outPrim.IndexBuffer[i] = (uint16_t)mergedBaseIndices[i];

            outPrim.StaticBlocks = baseBlocks;
            outPrim.StaticBlockCount = (uint32_t)outPrim.StaticBlocks.size();
            outPrim.StaticBlockCount_2 = outPrim.StaticBlockCount;

            outMesh.Primitives.push_back(outPrim);
        }

        ParseNodes(json, outMesh);

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Type 4 Particle Mesh";

        return "";
    }

    static std::string ImportType3(const std::string& gltfPath, const std::string& bankEntryName, CBBMParser& outBBM) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Could not open glTF file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of('.')) + ".bin";
        std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
        if (!binFile.is_open()) return "Could not open associated .bin file.";
        std::streamsize size = binFile.tellg();
        binFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> binData((size_t)size);
        if (!binFile.read((char*)binData.data(), size)) return "Failed to read .bin data.";

        outBBM = CBBMParser();
        outBBM.IsParsed = true;
        outBBM.FileVersion = 100;
        outBBM.FileComment = "Copyright Big Blue Box Studios Ltd.";
        outBBM.PhysicsMaterialIndex = 0;

        bool isBlender = json.find("Khronos glTF Blender") != std::string::npos;
        bool hasSceneRoot = json.find("\"name\":\"Scene_Root\"") != std::string::npos || json.find("\"name\": \"Scene_Root\"") != std::string::npos;
        bool applyBlenderFix = isBlender && hasSceneRoot;

        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outBBM.ParsedMaterials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                CBBMParser::BBMMaterial m = {};
                m.Index = i;
                m.Name = ExtractStringClean(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);

                m.Ambient = 0xFFFFFFFF;
                m.Diffuse = 0xFFFFFFFF;
                m.Specular = 0xFFFFFFFF;
                m.Shiny = 0.0f;
                m.ShinyStrength = 0.0f;
                m.Transparency = 0.0f;
                m.TwoSided = true;

                std::string extras = ExtractBlock(matObjs[i], "extras");
                if (!extras.empty()) {
                    m.DiffuseBank = (int)ExtractFloatClean(extras, "DiffuseMapID", 0);
                    m.BumpBank = (int)ExtractFloatClean(extras, "BumpMapID", 0);
                    m.ReflectBank = (int)ExtractFloatClean(extras, "ReflectionMapID", 0);
                    m.IllumBank = (int)ExtractFloatClean(extras, "IlluminationMapID", 0);
                    m.TwoSided = ExtractBool(extras, "IsTwoSided", true);
                    m.Transparent = ExtractBool(extras, "IsTransparent", false);
                    m.BooleanAlpha = ExtractBool(extras, "BooleanAlpha", false);
                    m.DegenerateTriangles = ExtractBool(extras, "DegenerateTriangles", false);
                    m.SelfIllumination = (uint32_t)ExtractFloatClean(extras, "SelfIllumination", 0);
                }
                outBBM.ParsedMaterials.push_back(m);
            }
        }
        else {
            CBBMParser::BBMMaterial defMat = {};
            defMat.Index = 0;
            defMat.Name = "DefaultPhysicsMat";
            defMat.TwoSided = true;
            defMat.Ambient = 0xFFFFFFFF;
            defMat.Diffuse = 0xFFFFFFFF;
            defMat.Specular = 0xFFFFFFFF;
            outBBM.ParsedMaterials.push_back(defMat);
        }

        std::vector<BView> views;
        for (const auto& obj : SplitArray(ExtractBlock(json, "bufferViews")))
            views.push_back({ (size_t)ExtractFloat(obj, "byteOffset"), (size_t)ExtractFloat(obj, "byteLength") });

        std::vector<Acc> accessors;
        for (const auto& obj : SplitArray(ExtractBlock(json, "accessors"))) {
            Acc a; a.view = (int)ExtractFloat(obj, "bufferView"); a.count = (int)ExtractFloat(obj, "count");
            a.compType = (int)ExtractFloat(obj, "componentType"); a.offset = (size_t)ExtractFloat(obj, "byteOffset");
            accessors.push_back(a);
        }

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        if (!meshObjs.empty()) {
            for (const auto& meshObj : meshObjs) {
                std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
                for (const auto& pObj : primObjs) {
                    std::string attr = ExtractBlock(pObj, "attributes");
                    int posAccIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                    int normAccIdx = (int)ExtractFloatClean(attr, "NORMAL", -1);
                    int indAccIdx = (int)ExtractFloatClean(pObj, "indices", -1);

                    if (posAccIdx < 0 || indAccIdx < 0) continue;

                    Acc posAcc = accessors[posAccIdx];
                    Acc iAcc = accessors[indAccIdx];

                    float* pData = (float*)(binData.data() + views[posAcc.view].offset + posAcc.offset);
                    float* nData = normAccIdx >= 0 ? (float*)(binData.data() + views[accessors[normAccIdx].view].offset + accessors[normAccIdx].offset) : nullptr;
                    uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;

                    auto FindOrAddVertex = [&](const CBBMParser::C3DVertex2& v) -> uint32_t {
                        for (size_t i = 0; i < outBBM.ParsedVertices.size(); ++i) {
                            const auto& uv = outBBM.ParsedVertices[i];
                            if (std::abs(uv.Position.x - v.Position.x) < 0.001f &&
                                std::abs(uv.Position.y - v.Position.y) < 0.001f &&
                                std::abs(uv.Position.z - v.Position.z) < 0.001f &&
                                std::abs(uv.Normal.x - v.Normal.x) < 0.01f &&
                                std::abs(uv.Normal.y - v.Normal.y) < 0.01f &&
                                std::abs(uv.Normal.z - v.Normal.z) < 0.01f) {
                                return (uint32_t)i;
                            }
                        }
                        outBBM.ParsedVertices.push_back(v);
                        return (uint32_t)(outBBM.ParsedVertices.size() - 1);
                        };

                    for (int i = 0; i < iAcc.count; i += 3) {
                        auto getVIdx = [&](int offset) -> uint32_t {
                            if (i + offset >= iAcc.count) return 0xFFFFFFFF;
                            if (iAcc.compType == 5121) return iData[i + offset];
                            if (iAcc.compType == 5123) return ((uint16_t*)iData)[i + offset];
                            if (iAcc.compType == 5125) return ((uint32_t*)iData)[i + offset];
                            return 0xFFFFFFFF;
                            };

                        uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) };

                        for (int j = 0; j < 3; j++) {
                            uint32_t vIdx = idxs[j];
                            if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                            CBBMParser::C3DVertex2 v = {};
                            v.Position.x = pData[vIdx * 3]; v.Position.y = pData[vIdx * 3 + 1]; v.Position.z = pData[vIdx * 3 + 2];

                            if (nData) {
                                v.Normal.x = nData[vIdx * 3]; v.Normal.y = nData[vIdx * 3 + 1]; v.Normal.z = nData[vIdx * 3 + 2];
                            }
                            else {
                                v.Normal.x = 0; v.Normal.y = 1; v.Normal.z = 0;
                            }

                            v.UV.u = 0; v.UV.v = 0;

                            outBBM.ParsedIndices.push_back((uint16_t)FindOrAddVertex(v));
                        }
                    }
                }
            }
        }

        C3DMeshContent tempMesh;
        ParseNodes(json, tempMesh);

        for (const auto& h : tempMesh.Helpers) {
            CBBMParser::HelperPoint hp;
            hp.Name = tempMesh.GetNameFromCRC(h.NameCRC);
            if (hp.Name.empty()) hp.Name = "HPNT_UNKNOWN";

            hp.SubMeshIndex = -1;
            size_t subPos = hp.Name.find("_Sub");
            if (subPos != std::string::npos) {
                try { hp.SubMeshIndex = std::stoi(hp.Name.substr(subPos + 4)); }
                catch (...) {}
                hp.Name = hp.Name.substr(0, subPos);
            }

            hp.Position = { h.Pos[0], h.Pos[1], h.Pos[2] };
            hp.BoneIndex = h.BoneIndex;
            outBBM.Helpers.push_back(hp);
        }

        for (const auto& d : tempMesh.Dummies) {
            CBBMParser::DummyObject dum;
            dum.Name = tempMesh.GetNameFromCRC(d.NameCRC);
            if (dum.Name.empty()) dum.Name = "HDMY_UNKNOWN";

            size_t locPos = dum.Name.find("_LOC");
            if (locPos != std::string::npos && locPos == dum.Name.length() - 4) {
                dum.Name = dum.Name.substr(0, locPos);
                dum.UseLocalOrigin = true;
            }
            else {
                dum.UseLocalOrigin = false;
            }

            memcpy(dum.Transform, d.Transform, 48);
            dum.BoneIndex = d.BoneIndex;
            dum.SubMeshIndex = -1;
            dum.Direction = { d.Transform[6], d.Transform[7], d.Transform[8] };
            dum.Position = { d.Transform[9], d.Transform[10], d.Transform[11] };
            outBBM.Dummies.push_back(dum);
        }

        for (const auto& v : tempMesh.Volumes) {
            CBBMParser::Volume vol;
            vol.Name = v.Name;
            vol.ID = v.ID;
            for (const auto& pl : v.Planes) {
                CBBMParser::BBMPlane bbmPlane = { {pl.Normal[0], pl.Normal[1], pl.Normal[2]}, pl.D };
                vol.Planes.push_back(bbmPlane);
            }
            vol.PlaneCount = (uint32_t)vol.Planes.size();
            outBBM.Volumes.push_back(vol);
        }
        return "";
    }

    static std::string ImportType5(const std::string& gltfPath, const std::string& originalName, C3DMeshContent& outMesh, bool forceRecalculate = false) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Could not open glTF file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of('.')) + ".bin";
        std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
        if (!binFile.is_open()) return "Could not open associated .bin file.";
        std::streamsize size = binFile.tellg();
        binFile.seekg(0, std::ios::beg);
        std::vector<uint8_t> binData((size_t)size);
        if (!binFile.read((char*)binData.data(), size)) return "Failed to read .bin data.";

        outMesh = C3DMeshContent();
        outMesh.MeshName = originalName;
        outMesh.MeshType = 5;
        outMesh.AnimatedFlag = true;
        outMesh.TotalAnimatedBlocks = 0;

        bool isBlender = json.find("Khronos glTF Blender") != std::string::npos;
        bool hasSceneRoot = json.find("\"name\":\"Scene_Root\"") != std::string::npos || json.find("\"name\": \"Scene_Root\"") != std::string::npos;
        bool applyBlenderFix = isBlender && hasSceneRoot;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        std::vector<std::string> viewObjs = SplitArray(ExtractBlock(json, "bufferViews"));
        struct BV { int offset, length; }; std::vector<BV> views;
        for (const auto& v : viewObjs) views.push_back({ (int)ExtractFloatClean(v, "byteOffset", 0), (int)ExtractFloatClean(v, "byteLength", 0) });

        std::vector<std::string> accObjs = SplitArray(ExtractBlock(json, "accessors"));
        struct Acc { int view, offset, count, compType; }; std::vector<Acc> accessors;
        for (const auto& a : accObjs) accessors.push_back({ (int)ExtractFloatClean(a, "bufferView", 0), (int)ExtractFloatClean(a, "byteOffset", 0), (int)ExtractFloatClean(a, "count", 0), (int)ExtractFloatClean(a, "componentType", 0) });

        std::string skinsBlock = ExtractBlock(json, "skins");
        std::vector<std::string> skinObjs = SplitArray(skinsBlock);
        if (skinObjs.empty()) return "Error: No skin/armature found. Type 5 must be rigged.";

        std::vector<float> jointsArr = GetJsonFloatArray(skinObjs[0], "joints");
        int ibmAccIdx = (int)ExtractFloatClean(skinObjs[0], "inverseBindMatrices", -1);
        if (jointsArr.empty() || ibmAccIdx < 0) return "Error: Skin is missing joints or InverseBindMatrices.";

        outMesh.BoneCount = (int32_t)jointsArr.size();
        outMesh.BoneTransformsRaw.resize(outMesh.BoneCount * 64, 0);
        outMesh.BoneKeyframesRaw.resize(outMesh.BoneCount * 48, 0);

        std::map<int, int> nodeParentMap;

        std::string nodesBlock;
        size_t searchPos = 0;
        while (true) {
            size_t foundIdx = json.find("\"nodes\"", searchPos);
            if (foundIdx == std::string::npos) break;

            std::string block = ExtractBlock(json, "nodes", searchPos);
            if (!block.empty() && block.find('{') != std::string::npos) {
                nodesBlock = block;
                break;
            }
            searchPos = foundIdx + 7;
        }

        std::vector<std::string> nodeObjs = SplitArray(nodesBlock);

        for (int i = 0; i < (int)nodeObjs.size(); i++) {
            std::vector<float> children = GetJsonFloatArray(nodeObjs[i], "children");
            for (float c : children) nodeParentMap[(int)c] = i;
        }

        Acc ibmAcc = accessors[ibmAccIdx];
        float* ibmData = (float*)(binData.data() + views[ibmAcc.view].offset + ibmAcc.offset);
        float* fableIbmDest = (float*)outMesh.BoneTransformsRaw.data();

        for (int i = 0; i < outMesh.BoneCount; i++) {
            float* srcMat = ibmData + (i * 16);
            float* dstMat = fableIbmDest + (i * 16);

            dstMat[0] = srcMat[0]; dstMat[1] = srcMat[4]; dstMat[2] = srcMat[8];  dstMat[3] = srcMat[12];
            dstMat[4] = srcMat[1]; dstMat[5] = srcMat[5]; dstMat[6] = srcMat[9];  dstMat[7] = srcMat[13];
            dstMat[8] = srcMat[2]; dstMat[9] = srcMat[6]; dstMat[10] = srcMat[10]; dstMat[11] = srcMat[14];
            dstMat[12] = srcMat[3]; dstMat[13] = srcMat[7]; dstMat[14] = srcMat[11]; dstMat[15] = srcMat[15];

            if (applyBlenderFix) {
                for (int r = 0; r < 3; r++) {
                    float c1 = dstMat[r * 4 + 1];
                    float c2 = dstMat[r * 4 + 2];
                    dstMat[r * 4 + 1] = -c2;
                    dstMat[r * 4 + 2] = c1;
                }
            }
        }

        for (int i = 0; i < outMesh.BoneCount; i++) {
            if (i >= (int)jointsArr.size()) break;
            int nodeIdx = (int)jointsArr[i];
            if (nodeIdx < 0 || nodeIdx >= (int)nodeObjs.size()) continue;

            std::string boneName = ExtractStringClean(nodeObjs[nodeIdx], "name");
            if (boneName == "Scene_Root") boneName = "Scene Root";
            outMesh.BoneNames.push_back(boneName);

            int fableId = i;
            std::string extras = ExtractBlock(nodeObjs[nodeIdx], "extras");
            if (extras.find("\"FableID\"") != std::string::npos) {
                fableId = (int)ExtractFloatClean(extras, "FableID", i);
            }
            outMesh.BoneIndices.push_back((uint16_t)fableId);

            C3DBone b = {};
            b.NameCRC = CalculateFableCRC(boneName);
            b.OriginalNoChildren = 0;
            b.ParentIndex = -1;
            if (nodeParentMap.count(nodeIdx)) {
                int pNode = nodeParentMap[nodeIdx];
                for (int j = 0; j < outMesh.BoneCount; j++) {
                    if ((int)jointsArr[j] == pNode) { b.ParentIndex = j; break; }
                }
            }

            float* bt = fableIbmDest + (i * 16);

            b.LocalizationMatrix[0] = bt[0];  b.LocalizationMatrix[1] = bt[4];  b.LocalizationMatrix[2] = bt[8];
            b.LocalizationMatrix[3] = bt[1];  b.LocalizationMatrix[4] = bt[5];  b.LocalizationMatrix[5] = bt[9];
            b.LocalizationMatrix[6] = bt[2];  b.LocalizationMatrix[7] = bt[6];  b.LocalizationMatrix[8] = bt[10];
            b.LocalizationMatrix[9] = bt[3];  b.LocalizationMatrix[10] = bt[7];  b.LocalizationMatrix[11] = bt[11];

            float kx = -bt[3], ky = -bt[7], kz = -bt[11];
            if (applyBlenderFix) {
                float tmp = ky;
                ky = -kz;
                kz = tmp;
            }

            float kf[12] = {};
            kf[0] = 0.0f;    kf[1] = 0.0f;    kf[2] = 0.0f;    kf[3] = 1.0f;
            kf[4] = kx;      kf[5] = ky;      kf[6] = kz;
            kf[7] = 0.0f;
            kf[8] = 1.0f;    kf[9] = 1.0f;    kf[10] = 1.0f;
            kf[11] = 0.0f;
            memcpy(outMesh.BoneKeyframesRaw.data() + i * 48, kf, 48);

            outMesh.Bones.push_back(b);
        }

        for (int i = 0; i < outMesh.BoneCount; i++) {
            outMesh.Bones[i].OriginalNoChildren = 0;
            for (int j = 0; j < outMesh.BoneCount; j++) {
                if (outMesh.Bones[j].ParentIndex == i) {
                    outMesh.Bones[i].OriginalNoChildren++;
                }
            }
        }

        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outMesh.Materials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < (int)matObjs.size(); ++i) {
                C3DMaterial m = {}; m.ID = i; m.Name = ExtractStringClean(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);
                std::string extras = ExtractBlock(matObjs[i], "extras");
                if (!extras.empty()) {
                    m.DiffuseMapID = (int)ExtractFloatClean(extras, "DiffuseMapID", 0);
                    m.BumpMapID = (int)ExtractFloatClean(extras, "BumpMapID", 0);
                    m.ReflectionMapID = (int)ExtractFloatClean(extras, "ReflectionMapID", 0);
                    m.IlluminationMapID = (int)ExtractFloatClean(extras, "IlluminationMapID", 0);
                    m.MapFlags = (int)ExtractFloatClean(extras, "MapFlags", 0);
                    m.IsTwoSided = ExtractBool(extras, "IsTwoSided", true);
                    m.IsTransparent = ExtractBool(extras, "IsTransparent", false);
                    m.BooleanAlpha = ExtractBool(extras, "BooleanAlpha", false);
                    m.DegenerateTriangles = ExtractBool(extras, "DegenerateTriangles", false);
                }
                else { m.IsTwoSided = true; m.IsTransparent = false; }
                outMesh.Materials.push_back(m);
            }
        }
        else {
            C3DMaterial defMat = {}; defMat.ID = 0; defMat.Name = "Default_Material"; defMat.IsTwoSided = true;
            outMesh.Materials.push_back(defMat);
        }
        outMesh.MaterialCount = (int32_t)outMesh.Materials.size();

        float globalMin[3] = { 1e9f, 1e9f, 1e9f };
        float globalMax[3] = { -1e9f, -1e9f, -1e9f };
        std::vector<std::string> preMeshObjs = SplitArray(ExtractBlock(json, "meshes"));
        for (const auto& meshObj : preMeshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            for (const auto& pObj : primObjs) {
                std::string attr = ExtractBlock(pObj, "attributes");
                int pIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                if (pIdx < 0 || pIdx >= accessors.size()) continue;
                Acc pAcc = accessors[pIdx];
                float* pData = (float*)(binData.data() + views[pAcc.view].offset + pAcc.offset);

                for (int v = 0; v < pAcc.count; v++) {
                    float px = pData[v * 3];
                    float py = pData[v * 3 + 1];
                    float pz = pData[v * 3 + 2];

                    if (applyBlenderFix) {
                        float tmp = py;
                        py = -pz;
                        pz = tmp;
                    }

                    if (px < globalMin[0]) globalMin[0] = px;
                    if (px > globalMax[0]) globalMax[0] = px;
                    if (py < globalMin[1]) globalMin[1] = py;
                    if (py > globalMax[1]) globalMax[1] = py;
                    if (pz < globalMin[2]) globalMin[2] = pz;
                    if (pz > globalMax[2]) globalMax[2] = pz;
                }
            }
        }

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        for (const auto& meshObj : meshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            std::string meshExtras = ExtractBlock(meshObj, "extras");
            //if (ExtractBool(meshExtras, "FableCloth", false)) continue; // Processed later!

            C3DPrimitive outPrim = {};
            outPrim.RepeatingMeshReps = 0;
            outPrim.InitFlags = 4;
            outPrim.IsCompressed = true;
            outPrim.BufferType = 0;
            outPrim.VertexStride = 20;
            outPrim.MaterialIndex = -1;

            outPrim.AvgTextureStretch = ExtractFloatClean(meshExtras, "AvgTextureStretch", 0.1f);
            outPrim.SphereRadius = ExtractFloatClean(meshExtras, "SphereRadius", 0.0f);
            memset(outPrim.SphereCenter, 0, 12);

            struct RawVertex { float p[3], n[3], u[2], w[4]; uint8_t j[4]; };
            std::vector<RawVertex> rawVerts;
            std::vector<uint32_t> rawIndices;

            for (const auto& pObj : primObjs) {
                int matIdx = (int)ExtractFloatClean(pObj, "material", 0);
                if (matIdx < 0 || matIdx >= outMesh.MaterialCount) matIdx = 0;
                if (outPrim.MaterialIndex == -1) outPrim.MaterialIndex = matIdx;

                std::string attr = ExtractBlock(pObj, "attributes");
                int pIdx = (int)ExtractFloatClean(attr, "POSITION", -1);
                int nIdx = (int)ExtractFloatClean(attr, "NORMAL", -1);
                int uIdx = (int)ExtractFloatClean(attr, "TEXCOORD_0", -1);
                int jIdx = (int)ExtractFloatClean(attr, "JOINTS_0", -1);
                int wIdx = (int)ExtractFloatClean(attr, "WEIGHTS_0", -1);
                int iIdx = (int)ExtractFloatClean(pObj, "indices", -1);

                if (pIdx < 0 || jIdx < 0 || wIdx < 0 || iIdx < 0) continue;

                Acc pAcc = accessors[pIdx], iAcc = accessors[iIdx], jAcc = accessors[jIdx], wAcc = accessors[wIdx];
                float* pData = (float*)(binData.data() + views[pAcc.view].offset + pAcc.offset);
                float* nData = nIdx >= 0 ? (float*)(binData.data() + views[accessors[nIdx].view].offset + accessors[nIdx].offset) : nullptr;
                float* uData = uIdx >= 0 ? (float*)(binData.data() + views[accessors[uIdx].view].offset + accessors[uIdx].offset) : nullptr;
                uint8_t* indData = binData.data() + views[iAcc.view].offset + iAcc.offset;

                uint32_t vOffset = (uint32_t)rawVerts.size();
                for (int v = 0; v < pAcc.count; v++) {
                    RawVertex rv = {};

                    rv.p[0] = pData[v * 3]; rv.p[1] = pData[v * 3 + 1]; rv.p[2] = pData[v * 3 + 2];
                    if (nData) { rv.n[0] = nData[v * 3]; rv.n[1] = nData[v * 3 + 1]; rv.n[2] = nData[v * 3 + 2]; }
                    else { rv.n[0] = 0; rv.n[1] = 1; rv.n[2] = 0; }
                    if (uData) { rv.u[0] = uData[v * 2]; rv.u[1] = uData[v * 2 + 1]; }

                    if (applyBlenderFix) {
                        float tmp;

                        tmp = rv.p[1];
                        rv.p[1] = -rv.p[2];
                        rv.p[2] = tmp;

                        tmp = rv.n[1];
                        rv.n[1] = -rv.n[2];
                        rv.n[2] = tmp;
                    }

                    if (wAcc.compType == 5126) {
                        float* wData = (float*)(binData.data() + views[wAcc.view].offset + wAcc.offset);
                        rv.w[0] = wData[v * 4]; rv.w[1] = wData[v * 4 + 1]; rv.w[2] = wData[v * 4 + 2]; rv.w[3] = wData[v * 4 + 3];
                    }
                    else if (wAcc.compType == 5123) {
                        uint16_t* wData = (uint16_t*)(binData.data() + views[wAcc.view].offset + wAcc.offset);
                        rv.w[0] = wData[v * 4] / 65535.0f; rv.w[1] = wData[v * 4 + 1] / 65535.0f; rv.w[2] = wData[v * 4 + 2] / 65535.0f; rv.w[3] = wData[v * 4 + 3] / 65535.0f;
                    }
                    else if (wAcc.compType == 5121) {
                        uint8_t* wData = (uint8_t*)(binData.data() + views[wAcc.view].offset + wAcc.offset);
                        rv.w[0] = wData[v * 4] / 255.0f; rv.w[1] = wData[v * 4 + 1] / 255.0f; rv.w[2] = wData[v * 4 + 2] / 255.0f; rv.w[3] = wData[v * 4 + 3] / 255.0f;
                    }

                    if (jAcc.compType == 5121) {
                        uint8_t* jData = binData.data() + views[jAcc.view].offset + jAcc.offset;
                        rv.j[0] = jData[v * 4]; rv.j[1] = jData[v * 4 + 1]; rv.j[2] = jData[v * 4 + 2]; rv.j[3] = jData[v * 4 + 3];
                    }
                    else if (jAcc.compType == 5123) {
                        uint16_t* jData = (uint16_t*)(binData.data() + views[jAcc.view].offset + jAcc.offset);
                        rv.j[0] = (uint8_t)jData[v * 4]; rv.j[1] = (uint8_t)jData[v * 4 + 1]; rv.j[2] = (uint8_t)jData[v * 4 + 2]; rv.j[3] = (uint8_t)jData[v * 4 + 3];
                    }

                    rawVerts.push_back(rv);
                }

                for (int i = 0; i < iAcc.count; i += 3) {
                    auto getVIdx = [&](int off) -> uint32_t {
                        if (iAcc.compType == 5121) return indData[i + off];
                        if (iAcc.compType == 5123) return ((uint16_t*)indData)[i + off];
                        if (iAcc.compType == 5125) return ((uint32_t*)indData)[i + off];
                        return 0;
                        };
                    rawIndices.push_back(getVIdx(0) + vOffset);
                    rawIndices.push_back(getVIdx(2) + vOffset);
                    rawIndices.push_back(getVIdx(1) + vOffset);
                }
            }

            std::vector<float> savedScale = GetJsonFloatArray(meshExtras, "compScale");
            std::vector<float> savedOffset = GetJsonFloatArray(meshExtras, "compOffset");

            if (!forceRecalculate && savedScale.size() >= 3 && savedOffset.size() >= 3) {
                outPrim.Compression.Scale[0] = savedScale[0];
                outPrim.Compression.Scale[1] = savedScale[1];
                outPrim.Compression.Scale[2] = savedScale[2];
                outPrim.Compression.Offset[0] = savedOffset[0];
                outPrim.Compression.Offset[1] = savedOffset[1];
                outPrim.Compression.Offset[2] = savedOffset[2];
            }
            else {
                for (int j = 0; j < 3; j++) {
                    outPrim.Compression.Offset[j] = (globalMax[j] + globalMin[j]) * 0.5f;
                    outPrim.Compression.Scale[j] = (globalMax[j] - globalMin[j]) * 0.505f;
                    if (outPrim.Compression.Scale[j] < 0.0001f) outPrim.Compression.Scale[j] = 0.0001f;
                }
            }
            outPrim.Compression.Scale[3] = 1.0f;
            outPrim.Compression.Offset[3] = 0.0f;

            struct AnimBlockTemp { std::vector<uint32_t> faces; std::vector<uint8_t> palette; };
            std::vector<AnimBlockTemp> blocks;
            AnimBlockTemp currentBlock;

            for (size_t f = 0; f < rawIndices.size(); f += 3) {
                uint32_t v0 = rawIndices[f], v1 = rawIndices[f + 1], v2 = rawIndices[f + 2];
                std::vector<uint8_t> faceJoints;

                auto addJ = [&](uint32_t idx) {
                    for (int k = 0; k < 4; k++) {
                        if (rawVerts[idx].w[k] > 0.0f) {
                            uint8_t jnt = rawVerts[idx].j[k];
                            if (std::find(faceJoints.begin(), faceJoints.end(), jnt) == faceJoints.end()) {
                                faceJoints.push_back(jnt);
                            }
                        }
                    }
                    };

                addJ(v0); addJ(v1); addJ(v2);
                if (faceJoints.empty()) faceJoints.push_back(0);

                std::vector<uint8_t> testPalette = currentBlock.palette;
                for (auto j : faceJoints) {
                    if (std::find(testPalette.begin(), testPalette.end(), j) == testPalette.end()) {
                        testPalette.push_back(j);
                    }
                }

                if (testPalette.size() > 16) {
                    blocks.push_back(currentBlock);
                    currentBlock = AnimBlockTemp();
                    currentBlock.faces = { v0, v1, v2 };
                    currentBlock.palette = faceJoints;
                }
                else {
                    currentBlock.faces.push_back(v0); currentBlock.faces.push_back(v1); currentBlock.faces.push_back(v2);
                    currentBlock.palette = testPalette;
                }
            }
            if (!currentBlock.faces.empty()) blocks.push_back(currentBlock);

            uint32_t currentGlobalVertexOffset = 0;
            for (auto& b : blocks) {
                CAnimatedBlock fBlock = {};
                fBlock.PrimitiveCount = (uint32_t)b.faces.size() / 3;
                fBlock.StartIndex = (uint32_t)outPrim.IndexBuffer.size();
                fBlock.BonesPerVertex = 3;
                fBlock.PalettedFlag = true;
                fBlock.Groups = b.palette;

                std::vector<RawVertex> blockVerts;
                std::map<uint32_t, uint32_t> oldToNewV;

                for (uint32_t oldIdx : b.faces) {
                    if (!oldToNewV.count(oldIdx)) {
                        oldToNewV[oldIdx] = (uint32_t)blockVerts.size();
                        blockVerts.push_back(rawVerts[oldIdx]);
                    }
                    outPrim.IndexBuffer.push_back(oldToNewV[oldIdx] + currentGlobalVertexOffset);
                }

                fBlock.VertexCount = (uint32_t)blockVerts.size();
                size_t vBufStart = outPrim.VertexBuffer.size();
                outPrim.VertexBuffer.resize(vBufStart + fBlock.VertexCount * outPrim.VertexStride);
                uint8_t* dest = outPrim.VertexBuffer.data() + vBufStart;

                for (const auto& v : blockVerts) {
                    uint32_t pP = PackPOSPACKED3(v.p[0], v.p[1], v.p[2], outPrim.Compression.Scale, outPrim.Compression.Offset);
                    uint32_t pN = PackNormal(v.n[0], v.n[1], v.n[2]);
                    int16_t cU = CompressUV(v.u[0]), cV = CompressUV(v.u[1]);

                    struct Influence { float w; uint8_t j; };
                    Influence infs[4] = {
                        { v.w[0], v.j[0] }, { v.w[1], v.j[1] },
                        { v.w[2], v.j[2] }, { v.w[3], v.j[3] }
                    };

                    std::sort(infs, infs + 4, [](const Influence& a, const Influence& b) {
                        return a.w > b.w;
                        });

                    infs[3].w = 0.0f;
                    infs[3].j = 0;

                    uint8_t fw[4] = { 0, 0, 0, 0 };
                    float sum = infs[0].w + infs[1].w + infs[2].w;

                    if (sum < 0.001f) {
                        fw[0] = 255;
                    }
                    else {
                        fw[0] = (uint8_t)std::clamp((int)std::round((infs[0].w / sum) * 255.0f), 0, 255);
                        fw[1] = (uint8_t)std::clamp((int)std::round((infs[1].w / sum) * 255.0f), 0, 255);
                        fw[2] = (uint8_t)std::clamp((int)std::round((infs[2].w / sum) * 255.0f), 0, 255);

                        int total = fw[0] + fw[1] + fw[2];
                        if (total != 255 && fw[0] > 0) {
                            int remainder = 255 - total;
                            fw[0] = (uint8_t)std::clamp((int)fw[0] + remainder, 0, 255);
                        }
                    }

                    uint8_t fj[4] = { 0, 0, 0, 0 };
                    for (int k = 0; k < 3; k++) {
                        if (fw[k] == 0) continue;
                        for (size_t p = 0; p < b.palette.size(); p++) {
                            if (b.palette[p] == infs[k].j) {
                                fj[k] = (uint8_t)(p * 3);
                                break;
                            }
                        }
                    }

                    memcpy(dest + 0, &pP, 4);
                    memcpy(dest + 4, fj, 4);
                    memcpy(dest + 8, fw, 4);
                    memcpy(dest + 12, &pN, 4);
                    memcpy(dest + 16, &cU, 2);
                    memcpy(dest + 18, &cV, 2);
                    dest += 20;
                }

                currentGlobalVertexOffset += fBlock.VertexCount;
                outPrim.AnimatedBlocks.push_back(fBlock);
            }

            outPrim.VertexCount = currentGlobalVertexOffset;
            outPrim.IndexCount = (uint32_t)outPrim.IndexBuffer.size();
            outPrim.TriangleCount = outPrim.IndexCount / 3;
            outPrim.AnimatedBlockCount = (uint32_t)outPrim.AnimatedBlocks.size();
            outPrim.AnimatedBlockCount_2 = outPrim.AnimatedBlockCount;

            outMesh.TotalAnimatedBlocks += outPrim.AnimatedBlockCount;
            outMesh.Primitives.push_back(outPrim);

            if (ExtractBool(meshExtras, "FableCloth", false)) {
                int targetPrim = (int)outMesh.Primitives.size() - 1;
                // Type 5 (characters) needs the Blender axis fix
                CompileFableClothFromGLTF(meshExtras, primObjs[0], binData, views, accessors, outMesh, targetPrim, applyBlenderFix);
            }

        }

        std::sort(outMesh.Primitives.begin(), outMesh.Primitives.end(), [&](const C3DPrimitive& a, const C3DPrimitive& b) {
            bool aTrans = false, bTrans = false;

            if (a.MaterialIndex >= 0 && a.MaterialIndex < outMesh.MaterialCount) {
                aTrans = outMesh.Materials[a.MaterialIndex].IsTransparent || outMesh.Materials[a.MaterialIndex].BooleanAlpha;
            }
            if (b.MaterialIndex >= 0 && b.MaterialIndex < outMesh.MaterialCount) {
                bTrans = outMesh.Materials[b.MaterialIndex].IsTransparent || outMesh.Materials[b.MaterialIndex].BooleanAlpha;
            }

            return aTrans < bTrans;
            });


        ParseNodes(json, outMesh);

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Type 5 Animated Mesh (With Override Support)";

        return "";
    }
}