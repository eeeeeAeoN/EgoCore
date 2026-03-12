// --- START OF FILE GltfMeshImporter.h ---

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
        try { return std::stof(json.substr(pos)); }
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

    static std::vector<std::string> SplitArray(const std::string& jsonArray) {
        std::vector<std::string> items;
        if (jsonArray.empty() || jsonArray.front() != '[') return items;
        size_t pos = 1;
        while (pos < jsonArray.length()) {
            pos = jsonArray.find_first_of("{", pos);
            if (pos == std::string::npos) break;
            int depth = 0; size_t end = pos; bool inStr = false;
            for (; end < jsonArray.length(); ++end) {
                if (jsonArray[end] == '"' && jsonArray[end - 1] != '\\') inStr = !inStr;
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
        // 1. Force normalize to guarantee length is exactly 1.0 to prevent artifacting
        float len = std::sqrt(x * x + y * y + z * z);
        if (len > 0.00001f) { x /= len; y /= len; z /= len; }

        // 2. Clamp strictly to [-1.0, 1.0] to prevent catastrophic bit-overflow wrapper
        x = std::clamp(x, -1.0f, 1.0f);
        y = std::clamp(y, -1.0f, 1.0f);
        z = std::clamp(z, -1.0f, 1.0f);

        // 3. Scale to Fable's exact bit-depth maximums (11 bits = 1023, 10 bits = 511)
        int32_t ix = (int32_t)std::round(x * 1023.0f);
        int32_t iy = (int32_t)std::round(y * 1023.0f);
        int32_t iz = (int32_t)std::round(z * 511.0f);

        // 4. Mask and pack into the 11-11-10 DWORD format Fable expects
        uint32_t ux = (uint32_t)ix & 0x7FF;
        uint32_t uy = ((uint32_t)iy & 0x7FF) << 11;
        uint32_t uz = ((uint32_t)iz & 0x3FF) << 22;

        return ux | uy | uz;
    }

    static int16_t CompressUV(float uv) {
        // EXACT INVERSE OF: return ((float)v / 2048.0f) - 8.0f;
        return (int16_t)std::clamp(std::round((uv + 8.0f) * 2048.0f), -32768.0f, 32767.0f);
    }

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

        std::vector<std::string> tempHelperNames;
        std::vector<std::string> tempDummyNames;

        for (const auto& node : nodes) {
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

            // Safely strip Blender suffixes like ".001", ".002" before calculating the CRC
            size_t dotPos = exportName.find_last_of('.');
            if (dotPos != std::string::npos && dotPos + 4 == exportName.length() && isdigit(exportName[dotPos + 1])) {
                exportName = exportName.substr(0, dotPos);
            }

            // Calculate the exact Engine CRC natively from the string, bypassing JSON float corruption
            if (type == "Helper") {
                if (exportName.find("HPNT_") == 0) {
                    std::string crcStr = exportName.substr(5);
                    size_t usPos = crcStr.find('_');
                    if (usPos != std::string::npos) crcStr = crcStr.substr(0, usPos);
                    try { nameCrc = (uint32_t)std::stoull(crcStr); }
                    catch (...) {}
                    exportName = "";
                }
                else {
                    nameCrc = CalculateFableCRC(exportName);
                }
            }
            else if (type == "Dummy") {
                if (exportName.find("HDMY_") == 0) {
                    std::string crcStr = exportName.substr(5);
                    size_t usPos = crcStr.find('_');
                    if (usPos != std::string::npos) crcStr = crcStr.substr(0, usPos);
                    try { nameCrc = (uint32_t)std::stoull(crcStr); }
                    catch (...) {}
                    exportName = "";
                }
                else {
                    nameCrc = CalculateFableCRC(exportName);
                }
            }
            else if (type == "Generator") {
                if (exportName.find("GEN_") == 0) exportName = exportName.substr(4);
            }

            if (type == "Helper") {
                CHelperPoint h = {};
                h.NameCRC = nameCrc;
                h.BoneIndex = -1;
                std::vector<float> trans = ExtractFloatArray(node, "translation");
                if (trans.size() >= 3) { h.Pos[0] = trans[0]; h.Pos[1] = trans[1]; h.Pos[2] = trans[2]; }
                outMesh.Helpers.push_back(h);

                if (!exportName.empty()) tempHelperNames.push_back(exportName);
            }
            else if (type == "Dummy" || type == "Generator") {
                float Transform[12] = { 1,0,0, 0,1,0, 0,0,1, 0,0,0 };

                std::vector<float> mat = ExtractFloatArray(node, "matrix");
                if (mat.size() >= 16) {
                    Transform[0] = mat[0]; Transform[1] = mat[1]; Transform[2] = mat[2];
                    Transform[3] = mat[4]; Transform[4] = mat[5]; Transform[5] = mat[6];
                    Transform[6] = mat[8]; Transform[7] = mat[9]; Transform[8] = mat[10];
                    Transform[9] = mat[12]; Transform[10] = mat[13]; Transform[11] = mat[14];
                }
                else {
                    std::vector<float> scale = ExtractFloatArray(node, "scale");
                    std::vector<float> rot = ExtractFloatArray(node, "rotation");
                    std::vector<float> trans = ExtractFloatArray(node, "translation");

                    float sX = 1.0f, sY = 1.0f, sZ = 1.0f;
                    if (scale.size() >= 3) { sX = scale[0]; sY = scale[1]; sZ = scale[2]; }

                    if (rot.size() >= 4) {
                        float x = rot[0], y = rot[1], z = rot[2], w = rot[3];
                        Transform[0] = (1.0f - 2.0f * (y * y + z * z)) * sX; Transform[1] = (2.0f * (x * y + z * w)) * sX; Transform[2] = (2.0f * (x * z - y * w)) * sX;
                        Transform[3] = (2.0f * (x * y - z * w)) * sY; Transform[4] = (1.0f - 2.0f * (x * x + z * z)) * sY; Transform[5] = (2.0f * (y * z + x * w)) * sY;
                        Transform[6] = (2.0f * (x * z + y * w)) * sZ; Transform[7] = (2.0f * (y * z - x * w)) * sZ; Transform[8] = (1.0f - 2.0f * (x * x + y * y)) * sZ;
                    }
                    if (trans.size() >= 3) {
                        Transform[9] = trans[0]; Transform[10] = trans[1]; Transform[11] = trans[2];
                    }
                }

                if (type == "Dummy") {
                    CDummyObject d = {};
                    d.NameCRC = nameCrc;
                    d.BoneIndex = -1;
                    memcpy(d.Transform, Transform, 48);
                    outMesh.Dummies.push_back(d);

                    if (!exportName.empty()) tempDummyNames.push_back(exportName);
                }
                else {
                    CMeshGenerator g = {};
                    g.BankIndex = (uint32_t)ExtractFloatClean(extras, "bankId", 0);
                    g.BoneIndex = -1;
                    g.UseLocalOrigin = false;
                    g.ObjectName = exportName;
                    memcpy(g.Transform, Transform, 48);
                    outMesh.Generators.push_back(g);
                }
            }
            else if (type == "Volume") {
                CMeshVolume v = {};
                v.ID = 0; // Fixed: Version tags are ignored from the JSON
                if (name.find("VOL_") == 0) v.Name = name.substr(4);
                else v.Name = name;

                std::string planesArr = ExtractBlock(extras, "planes");
                std::vector<std::string> planeObjs = SplitArray(planesArr);
                for (const auto& pObj : planeObjs) {
                    CPlane p = {};
                    std::vector<float> n = ExtractFloatArray(pObj, "n");
                    if (n.size() >= 3) { p.Normal[0] = n[0]; p.Normal[1] = n[1]; p.Normal[2] = n[2]; }
                    p.D = ExtractFloatClean(pObj, "d", 0.0f);
                    v.Planes.push_back(p);
                }
                outMesh.Volumes.push_back(v);
            }
        }

        outMesh.HelperPointCount = (uint16_t)outMesh.Helpers.size();
        outMesh.DummyObjectCount = (uint16_t)outMesh.Dummies.size();
        outMesh.MeshGeneratorCount = (uint16_t)outMesh.Generators.size();
        outMesh.MeshVolumeCount = (uint16_t)outMesh.Volumes.size();

        outMesh.PackNames(tempHelperNames, tempDummyNames);
        outMesh.UnpackNames();
    }

    struct Acc { int view, count, compType; size_t offset; };
    struct BView { size_t offset, length; };

    static std::string ImportType1(const std::string& gltfPath, const std::string& originalName, C3DMeshContent& outMesh) {
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
        outMesh.AnimatedFlag = 0; // Strictly Static

        // TARGETED BLENDER FIX:
        // EgoCore writes an artificial 'Scene_Root' to make Z-up meshes look Y-up in viewers.
        // Blender "helpfully" bakes this coordinate conversion permanently into the binary vertex buffer, 
        // physically swapping Y and Z, but it leaves the local helper/bone translations completely alone!
        // This is why complex matrix multiplication broke your helpers. We just need to un-swap the vertices.
        bool isBlender = json.find("Khronos glTF Blender") != std::string::npos;
        bool hasSceneRoot = json.find("\"name\":\"Scene_Root\"") != std::string::npos || json.find("\"name\": \"Scene_Root\"") != std::string::npos;
        bool applyBlenderFix = isBlender && hasSceneRoot;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        std::string nodesBlock = ExtractBlock(json, "nodes");
        std::vector<std::string> nodeObjs = SplitArray(nodesBlock);
        for (const auto& node : nodeObjs) {
            if (ExtractStringClean(node, "name") == "Scene_Root") {
                std::vector<float> mat = ExtractFloatArray(node, "matrix");
                if (mat.size() >= 16) {
                    outMesh.RootMatrix[0] = mat[0]; outMesh.RootMatrix[1] = mat[1]; outMesh.RootMatrix[2] = mat[2];
                    outMesh.RootMatrix[3] = mat[4]; outMesh.RootMatrix[4] = mat[5]; outMesh.RootMatrix[5] = mat[6];
                    outMesh.RootMatrix[6] = mat[8]; outMesh.RootMatrix[7] = mat[9]; outMesh.RootMatrix[8] = mat[10];
                    outMesh.RootMatrix[9] = mat[12]; outMesh.RootMatrix[10] = mat[13]; outMesh.RootMatrix[11] = mat[14];
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
                C3DMaterial m = {}; m.ID = i; m.Name = ExtractString(matObjs[i], "name");
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
            outPrim.IsCompressed = true;
            outPrim.BufferType = 0;
            outPrim.VertexStride = 24;
            outPrim.MaterialIndex = -1;

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

                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) }; // Swap winding for Fable

                    for (int j = 0; j < 3; j++) {
                        uint32_t vIdx = idxs[j];
                        if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                        BaseVertex v = {};
                        v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                        if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                        else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; }

                        if (uData) { v.u[0] = uData[vIdx * 2]; v.u[1] = uData[vIdx * 2 + 1]; }
                        else { v.u[0] = 0; v.u[1] = 0; }

                        // ONLY apply fix if Blender baked the rotation into the binary buffer
                        if (applyBlenderFix) {
                            float temp_y = v.p[1];
                            v.p[1] = -v.p[2];
                            v.p[2] = temp_y;

                            float temp_ny = v.n[1];
                            v.n[1] = -v.n[2];
                            v.n[2] = temp_ny;
                        }

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

            float minP[3] = { 1e9f, 1e9f, 1e9f }; float maxP[3] = { -1e9f, -1e9f, -1e9f };
            for (const auto& v : uniqueVerts) {
                for (int j = 0; j < 3; j++) {
                    if (v.p[j] < minP[j]) minP[j] = v.p[j];
                    if (v.p[j] > maxP[j]) maxP[j] = v.p[j];
                }
            }
            for (int j = 0; j < 3; j++) {
                outPrim.Compression.Offset[j] = minP[j];
                outPrim.Compression.Scale[j] = maxP[j] - minP[j];
                if (outPrim.Compression.Scale[j] < 0.0001f) outPrim.Compression.Scale[j] = 0.0001f;
            }

            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * outPrim.VertexStride, 0);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (uint32_t v = 0; v < baseVertCount; v++) {
                uint32_t packedPos = PackPOSPACKED3(uniqueVerts[v].p[0], uniqueVerts[v].p[1], uniqueVerts[v].p[2], outPrim.Compression.Scale, outPrim.Compression.Offset);
                uint32_t packedNorm = PackNormal(uniqueVerts[v].n[0], uniqueVerts[v].n[1], uniqueVerts[v].n[2]);
                int16_t compU = CompressUV(uniqueVerts[v].u[0]);
                int16_t compV = CompressUV(uniqueVerts[v].u[1]);

                memcpy(vDest + 0, &packedPos, 4);
                memcpy(vDest + 4, &packedNorm, 4);
                memcpy(vDest + 8, &compU, 2);
                memcpy(vDest + 10, &compV, 2);

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

        ParseNodes(json, outMesh); // Helpers are parsed directly from local JSON translates

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Pure Static Mesh with Targeted Blender Fix";

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

        // Parse Materials from glTF
        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outMesh.Materials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                C3DMaterial m = {};
                m.ID = i;
                m.Name = ExtractString(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);

                // Recover Fable-specific data from the extras block
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
            // True Failsafe if the glTF has absolutely no materials
            C3DMaterial defMat = {};
            defMat.ID = 0; defMat.Name = "Grass_Material"; defMat.IsTwoSided = true; defMat.IsTransparent = true;
            outMesh.Materials.push_back(defMat);
        }
        outMesh.MaterialCount = (int32_t)outMesh.Materials.size();

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        if (meshObjs.empty()) return "No meshes found in glTF.";

        // We process ONE Fable Primitive per glTF Mesh
        for (const auto& meshObj : meshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            C3DPrimitive outPrim = {};
            outPrim.RepeatingMeshReps = reps;
            outPrim.VertexStride = 36;
            outPrim.InitFlags = 4;
            outPrim.MaterialIndex = -1;

            // --- NEW: Vertex Welder Structures ---
            struct BaseVertex { float p[3], n[3], u[2]; };
            std::vector<BaseVertex> uniqueVerts;
            std::vector<uint32_t> mergedBaseIndices;
            std::vector<CStaticBlock> baseBlocks;

            // Simple linear search to deduplicate vertices. 
            // Loosened epsilon catches Blender's floating-point normal drift!
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

                // 1. Map glTF indices to our optimized, unique Fable vertices
                for (int i = 0; i < iAcc.count; i += 3) {
                    auto getVIdx = [&](int offset) -> uint32_t {
                        if (i + offset >= iAcc.count) return 0xFFFFFFFF;
                        if (iAcc.compType == 5121) return iData[i + offset];
                        if (iAcc.compType == 5123) return ((uint16_t*)iData)[i + offset];
                        if (iAcc.compType == 5125) return ((uint32_t*)iData)[i + offset];
                        return 0xFFFFFFFF;
                        };

                    // SWAP index 1 and 2 to convert glTF Counter-Clockwise back to Fable Clockwise
                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) };

                    for (int j = 0; j < 3; j++) {
                        uint32_t vIdx = idxs[j];
                        if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                        BaseVertex v = {};
                        v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                        if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                        else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; }

                        // INVERT the V coordinate back to Fable's Top-Left origin!
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

            // 2. Expand Optimized Vertices for Repetitions
            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * reps * 36);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (int r = 0; r < reps; r++) {
                for (uint32_t v = 0; v < baseVertCount; v++) {
                    memcpy(vDest, &uniqueVerts[v].p, 12); vDest += 12;
                    memcpy(vDest, &uniqueVerts[v].n, 12); vDest += 12;
                    memcpy(vDest, &uniqueVerts[v].u, 8);  vDest += 8;
                    uint32_t instID = r;
                    memcpy(vDest, &instID, 4); vDest += 4; // Instance ID tracking
                }
            }

            // 3. Expand Indices for Repetitions
            outPrim.IndexCount = (uint32_t)mergedBaseIndices.size();
            outPrim.TriangleCount = outPrim.IndexCount / 3;
            outPrim.IndexBuffer.resize(outPrim.IndexCount * reps);

            uint32_t outIdx = 0;
            for (int r = 0; r < reps; r++) {
                for (size_t i = 0; i < mergedBaseIndices.size(); i++) {
                    outPrim.IndexBuffer[outIdx++] = (uint16_t)(mergedBaseIndices[i] + (r * baseVertCount));
                }
            }

            // 4. Map Fable Blocks to the LAST Repetition Layer
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
        outMesh.MeshType = 4; // Type 4 Particle Mesh
        outMesh.AnimatedFlag = 0;

        memset(outMesh.RootMatrix, 0, 48);
        outMesh.RootMatrix[0] = 1.0f; outMesh.RootMatrix[4] = 1.0f; outMesh.RootMatrix[8] = 1.0f;

        auto GetJsonFloatArray = [](const std::string& j, const std::string& key) -> std::vector<float> {
            std::vector<float> res; size_t pos = j.find("\"" + key + "\""); if (pos == std::string::npos) return res;
            pos = j.find(':', pos); if (pos == std::string::npos) return res; pos = j.find('[', pos); if (pos == std::string::npos) return res;
            size_t end = j.find(']', pos); if (end == std::string::npos) return res;
            std::string arr = j.substr(pos + 1, end - pos - 1);
            for (char& c : arr) if (c == ',' || c == '\n' || c == '\r' || c == '\t') c = ' ';
            std::stringstream ss(arr); float f; while (ss >> f) res.push_back(f);
            return res;
            };

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

        // Parse Root Node Matrix directly from either Matrix array or Rotation Quaternion!
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

                    // Assign the calculated 4x4 matrix to Fable's 4x3 RootMatrix format
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
            outPrim.InitFlags = 0;
            outPrim.IsCompressed = false;
            outPrim.BufferType = 0;
            outPrim.VertexStride = 32;    // Back to 32 bytes for Fable's floats!
            outPrim.MaterialIndex = -1;

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

                    uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) }; // Swap winding

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

            float minP[3] = { 1e9f, 1e9f, 1e9f }; float maxP[3] = { -1e9f, -1e9f, -1e9f };
            for (const auto& v : uniqueVerts) {
                for (int j = 0; j < 3; j++) {
                    if (v.p[j] < minP[j]) minP[j] = v.p[j];
                    if (v.p[j] > maxP[j]) maxP[j] = v.p[j];
                }
            }
            for (int j = 0; j < 3; j++) {
                outPrim.Compression.Offset[j] = minP[j];
                outPrim.Compression.Scale[j] = maxP[j] - minP[j];
                if (outPrim.Compression.Scale[j] < 0.0001f) outPrim.Compression.Scale[j] = 0.0001f;
            }

            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * outPrim.VertexStride, 0);
            uint8_t* vDest = outPrim.VertexBuffer.data();

            for (uint32_t v = 0; v < baseVertCount; v++) {
                float pos[3] = { uniqueVerts[v].p[0], uniqueVerts[v].p[1], uniqueVerts[v].p[2] };
                float norm[3] = { uniqueVerts[v].n[0], uniqueVerts[v].n[1], uniqueVerts[v].n[2] };
                float uv[2] = { uniqueVerts[v].u[0], uniqueVerts[v].u[1] }; // Pure uncompressed floats

                memcpy(vDest + 0, pos, 12);
                memcpy(vDest + 12, norm, 12);
                memcpy(vDest + 24, uv, 8); // Strided nicely for 32 bytes

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
        outBBM.FileVersion = 100; // <--- SYNCHRONIZED VERSION (0x64)
        outBBM.FileComment = "Copyright Big Blue Box Studios Ltd."; // <--- SYNCHRONIZED FILE COMMENT
        outBBM.PhysicsMaterialIndex = 0;

        bool isBlender = json.find("Khronos glTF Blender") != std::string::npos;
        bool hasSceneRoot = json.find("\"name\":\"Scene_Root\"") != std::string::npos || json.find("\"name\": \"Scene_Root\"") != std::string::npos;
        bool applyBlenderFix = isBlender && hasSceneRoot;

        // --- FIX ISSUE 6: Parse BBM Materials ---
        std::string materialsBlock = ExtractBlock(json, "materials");
        std::vector<std::string> matObjs = SplitArray(materialsBlock);

        outBBM.ParsedMaterials.clear();
        if (!matObjs.empty()) {
            for (int i = 0; i < matObjs.size(); ++i) {
                CBBMParser::BBMMaterial m = {};
                m.Index = i;
                m.Name = ExtractStringClean(matObjs[i], "name");
                if (m.Name.empty()) m.Name = "Imported_Mat_" + std::to_string(i);

                // --- INJECT FABLE DEFAULTS ---
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
                    int normAccIdx = (int)ExtractFloatClean(attr, "NORMAL", -1); // <--- FIX ISSUE 5
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

                        uint32_t idxs[3] = { getVIdx(0), getVIdx(2), getVIdx(1) }; // Swap winding

                        for (int j = 0; j < 3; j++) {
                            uint32_t vIdx = idxs[j];
                            if (vIdx >= posAcc.count || vIdx == 0xFFFFFFFF) continue;

                            CBBMParser::C3DVertex2 v = {};
                            v.Position.x = pData[vIdx * 3]; v.Position.y = pData[vIdx * 3 + 1]; v.Position.z = pData[vIdx * 3 + 2];

                            // --- FIX ISSUE 5: Parse Normals ---
                            if (nData) {
                                v.Normal.x = nData[vIdx * 3]; v.Normal.y = nData[vIdx * 3 + 1]; v.Normal.z = nData[vIdx * 3 + 2];
                            }
                            else {
                                v.Normal.x = 0; v.Normal.y = 1; v.Normal.z = 0;
                            }

                            v.UV.u = 0; v.UV.v = 0; // Physics meshes ignore UVs

                            outBBM.ParsedIndices.push_back((uint16_t)FindOrAddVertex(v));
                        }
                    }
                }
            }
        }

        // Steal the JSON logic from Type 1/2 to perfectly recover Navmeshes/Helpers from GLTF nodes
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
}