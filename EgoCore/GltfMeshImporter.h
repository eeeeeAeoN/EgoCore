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

    struct Acc { int view, count, compType; size_t offset; };
    struct BView { size_t offset, length; };

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
                for (int i = 0; i < iAcc.count; i++) {
                    uint32_t vIdx = 0;
                    if (iAcc.compType == 5121) vIdx = iData[i];
                    else if (iAcc.compType == 5123) vIdx = ((uint16_t*)iData)[i];
                    else if (iAcc.compType == 5125) vIdx = ((uint32_t*)iData)[i];

                    if (vIdx >= posAcc.count) continue;

                    BaseVertex v = {};
                    v.p[0] = pData[vIdx * 3]; v.p[1] = pData[vIdx * 3 + 1]; v.p[2] = pData[vIdx * 3 + 2];
                    if (nData) { v.n[0] = nData[vIdx * 3]; v.n[1] = nData[vIdx * 3 + 1]; v.n[2] = nData[vIdx * 3 + 2]; }
                    else { v.n[0] = 0; v.n[1] = 1; v.n[2] = 0; } // Fallback normal
                    if (uData) { v.u[0] = uData[vIdx * 2]; v.u[1] = uData[vIdx * 2 + 1]; }
                    else { v.u[0] = 0; v.u[1] = 0; } // Fallback UV

                    mergedBaseIndices.push_back(FindOrAddVertex(v));
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