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

        // Failsafe Material
        C3DMaterial defMat = {};
        defMat.ID = 0; defMat.Name = "Grass_Material"; defMat.IsTwoSided = true; defMat.IsTransparent = true;
        outMesh.Materials.push_back(defMat);
        outMesh.MaterialCount = 1;

        std::vector<std::string> meshObjs = SplitArray(ExtractBlock(json, "meshes"));
        if (meshObjs.empty()) return "No meshes found in glTF.";

        std::vector<std::string> primObjs = SplitArray(ExtractBlock(meshObjs[0], "primitives"));

        for (const auto& pObj : primObjs) {
            C3DPrimitive prim = {};
            prim.MaterialIndex = 0;
            prim.RepeatingMeshReps = reps;
            prim.VertexStride = 36;
            prim.InitFlags = 4; // Type 2 is uncompressed floats, despite InitFlags=4

            std::string attrBlock = ExtractBlock(pObj, "attributes");
            int posAccIdx = (int)ExtractFloat(attrBlock, "POSITION");
            int normAccIdx = (int)ExtractFloat(attrBlock, "NORMAL");
            int uvAccIdx = (int)ExtractFloat(attrBlock, "TEXCOORD_0");
            int indAccIdx = (int)ExtractFloat(pObj, "indices");

            if (posAccIdx < 0 || posAccIdx >= accessors.size()) continue;

            Acc posAcc = accessors[posAccIdx];
            uint32_t baseVertCount = posAcc.count;

            struct BaseV { float p[3]; float n[3]; float u[2]; };
            std::vector<BaseV> baseVerts(baseVertCount);

            float* pData = (float*)(binData.data() + views[posAcc.view].offset + posAcc.offset);
            for (int i = 0; i < baseVertCount; i++) {
                baseVerts[i].p[0] = pData[i * 3 + 0]; baseVerts[i].p[1] = pData[i * 3 + 1]; baseVerts[i].p[2] = pData[i * 3 + 2];
            }

            if (normAccIdx >= 0) {
                float* nData = (float*)(binData.data() + views[accessors[normAccIdx].view].offset + accessors[normAccIdx].offset);
                for (int i = 0; i < baseVertCount; i++) { baseVerts[i].n[0] = nData[i * 3]; baseVerts[i].n[1] = nData[i * 3 + 1]; baseVerts[i].n[2] = nData[i * 3 + 2]; }
            }
            if (uvAccIdx >= 0) {
                float* uData = (float*)(binData.data() + views[accessors[uvAccIdx].view].offset + accessors[uvAccIdx].offset);
                for (int i = 0; i < baseVertCount; i++) { baseVerts[i].u[0] = uData[i * 2]; baseVerts[i].u[1] = uData[i * 2 + 1]; }
            }

            // Expand Vertices
            prim.VertexCount = baseVertCount;
            uint32_t totalVerts = baseVertCount * reps;
            prim.VertexBuffer.resize(totalVerts * 36);
            uint8_t* vDest = prim.VertexBuffer.data();

            for (int r = 0; r < reps; r++) {
                for (int v = 0; v < baseVertCount; v++) {
                    memcpy(vDest, baseVerts[v].p, 12); vDest += 12;
                    memcpy(vDest, baseVerts[v].n, 12); vDest += 12;
                    memcpy(vDest, baseVerts[v].u, 8);  vDest += 8;
                    uint32_t instID = r;
                    memcpy(vDest, &instID, 4);         vDest += 4;
                }
            }

            // Read Base Indices
            std::vector<uint32_t> baseIndices;
            if (indAccIdx >= 0) {
                Acc iAcc = accessors[indAccIdx];
                uint8_t* iData = binData.data() + views[iAcc.view].offset + iAcc.offset;
                for (int i = 0; i < iAcc.count; i++) {
                    if (iAcc.compType == 5121) baseIndices.push_back(iData[i]);
                    else if (iAcc.compType == 5123) baseIndices.push_back(((uint16_t*)iData)[i]);
                    else if (iAcc.compType == 5125) baseIndices.push_back(((uint32_t*)iData)[i]);
                }
            }

            // Expand Indices
            prim.IndexCount = (uint32_t)baseIndices.size();
            prim.TriangleCount = prim.IndexCount / 3;
            prim.IndexBuffer.resize(prim.IndexCount * reps);

            uint32_t outIdx = 0;
            for (int r = 0; r < reps; r++) {
                for (int i = 0; i < prim.IndexCount; i++) {
                    prim.IndexBuffer[outIdx++] = (uint16_t)(baseIndices[i] + (r * baseVertCount));
                }
            }

            // Create Required Static Block for Type 2
            CStaticBlock sb = {};
            sb.PrimitiveCount = prim.TriangleCount;
            sb.StartIndex = prim.IndexCount * (reps - 1); // Fable uses the start of the LAST repetition as stride marker
            sb.IsStrip = false;
            prim.StaticBlocks.push_back(sb);
            prim.StaticBlockCount = 1;
            prim.StaticBlockCount_2 = 1;
            outMesh.TotalStaticBlocks++;

            outMesh.Primitives.push_back(prim);
        }

        outMesh.PrimitiveCount = (int32_t)outMesh.Primitives.size();
        outMesh.AutoCalculateBounds();
        outMesh.IsParsed = true;
        outMesh.DebugStatus = "Successfully Synthesized Type 2 Mesh";

        return "";
    }
}