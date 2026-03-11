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
                v.ID = (uint32_t)ExtractFloatClean(extras, "ID", 0);
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
        outMesh.MeshType = 1; // Type 1 Static Mesh
        outMesh.AnimatedFlag = 0;

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
                C3DMaterial m = {};
                m.ID = i;
                m.Name = ExtractString(matObjs[i], "name");
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
                else {
                    m.IsTwoSided = true; m.IsTransparent = false;
                }
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

        for (const auto& meshObj : meshObjs) {
            std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObj, "primitives"));
            if (primObjs.empty()) continue;

            C3DPrimitive outPrim = {};
            outPrim.InitFlags = 4;       
            outPrim.IsCompressed = true;  
            outPrim.VertexStride = 24;    
            outPrim.BufferType = 0;
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

                    // Swapped glTF indices to fix Fable's Clockwise Winding!
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
            if (baseVertCount > 65535) return "Import Error: Vertex overflow. Fable limit is 65,535.";

            float minP[3] = { 1e9f, 1e9f, 1e9f };
            float maxP[3] = { -1e9f, -1e9f, -1e9f };
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

            // EXACT 24-BYTE PACKING
            outPrim.VertexCount = baseVertCount;
            outPrim.VertexBuffer.resize(baseVertCount * 24, 0);
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

                vDest += 24;
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
}