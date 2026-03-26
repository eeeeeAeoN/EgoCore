#pragma once
#include "MeshParser.h"
#include "AnimParser.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <map>
#include <algorithm>
#include <directxmath.h>

namespace GltfAnimImporter {

    static std::vector<uint8_t> HexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
            bytes.push_back(byte);
        }
        return bytes;
    }

    struct GltfBufferView { int offset; int length; };
    struct GltfAccessor { int view; int count; int typeSize; };

    static int32_t ExtractInt32(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\""); if (pos == std::string::npos) return 0;
        pos = json.find(':', pos); if (pos == std::string::npos) return 0; pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
        size_t end = pos;
        if (pos < json.length() && json[pos] == '-') end++;
        while (end < json.length() && isdigit(json[end])) end++;
        if (pos == end) return 0;
        try { return (int32_t)std::stoll(json.substr(pos, end - pos)); }
        catch (...) { return 0; }
    }

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

    static std::string ExtractString(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos); if (pos == std::string::npos) return "";
        pos = json.find('"', pos); if (pos == std::string::npos) return "";
        pos++; size_t end = pos;
        while (end < json.length()) { if (json[end] == '"' && json[end - 1] != '\\') break; end++; }
        return json.substr(pos, end - pos);
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

    static std::vector<float> ExtractFloatArray(const std::string& json, const std::string& key) {
        std::vector<float> res;
        std::string block = ExtractBlock(json, key);
        if (block.empty() || block[0] != '[') return res;
        size_t pos = 1;
        while (pos < block.length() && block[pos] != ']') {
            while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\t' || block[pos] == '\n' || block[pos] == '\r' || block[pos] == ',')) pos++;
            if (pos >= block.length() || block[pos] == ']') break;
            size_t end = pos;
            while (end < block.length() && (isdigit(block[end]) || block[end] == '.' || block[end] == '-' || block[end] == '+' || block[end] == 'e' || block[end] == 'E')) end++;
            if (pos != end) {
                try { res.push_back(std::stof(block.substr(pos, end - pos))); }
                catch (...) {}
            }
            pos = end;
        }
        return res;
    }

    static std::string CleanName(std::string s) {
        std::string r;
        for (char c : s) if (isalnum((unsigned char)c) || c == '_' || c == ' ') r += (char)c;
        return r;
    }

    static DirectX::XMVECTOR Slerp(DirectX::XMVECTOR a, DirectX::XMVECTOR b, float t) { return DirectX::XMQuaternionSlerp(a, b, t); }
    static DirectX::XMVECTOR Lerp(DirectX::XMVECTOR a, DirectX::XMVECTOR b, float t) { return DirectX::XMVectorLerp(a, b, t); }

    struct GltfFableNode {
        int GltfIndex;
        std::string Name;
        uint32_t FableID;
        int32_t ParentID;
        DirectX::XMMATRIX BindLocal;
        bool IsMapped;
    };

    static std::string Import(const std::string& gltfPath, const C3DMeshContent& mesh, C3DAnimationInfo& outAnim, int& outAnimType) {
        std::ifstream file(gltfPath);
        if (!file.is_open()) return "Failed to open .gltf file.";
        std::stringstream buffer; buffer << file.rdbuf();
        std::string json = buffer.str();

        std::string animsBlock = ExtractBlock(json, "animations");
        if (animsBlock.empty() || animsBlock == "[]") return "No 'animations' array found in glTF. Ensure Blender exported an Action.";

        std::vector<std::string> animList = SplitArray(animsBlock);
        if (animList.empty()) return "Animations array is empty.";
        std::string targetAnim = animList[0];

        std::string fableHex = ExtractString(targetAnim, "FableAnimData");
        if (!fableHex.empty()) {
            std::vector<uint8_t> meta = HexToBytes(fableHex);
            size_t c = 0;

            if (meta.size() >= 26) {
                memcpy(&outAnimType, meta.data() + c, 4); c += 4;
                memcpy(&outAnim.Duration, meta.data() + c, 4); c += 4;
                memcpy(&outAnim.NonLoopingDuration, meta.data() + c, 4); c += 4;
                memcpy(&outAnim.MovementVector, meta.data() + c, 12); c += 12;
                memcpy(&outAnim.Rotation, meta.data() + c, 4); c += 4;
                outAnim.IsCyclic = meta[c++];
                outAnim.HasHelper = meta[c++];

                uint32_t evCount = 0;
                if (c + 4 <= meta.size()) { memcpy(&evCount, meta.data() + c, 4); c += 4; }

                outAnim.TimeEvents.clear();
                for (uint32_t i = 0; i < evCount; i++) {
                    if (c + 4 > meta.size()) break;
                    uint32_t nl = *(uint32_t*)(meta.data() + c); c += 4;
                    if (c + nl + 4 > meta.size()) break;
                    TimeEvent ev;
                    ev.Name = std::string((char*)meta.data() + c, nl); c += nl;
                    memcpy(&ev.Time, meta.data() + c, 4); c += 4;
                    outAnim.TimeEvents.push_back(ev);
                }

                uint32_t maskCount = 0;
                if (c + 4 <= meta.size()) { memcpy(&maskCount, meta.data() + c, 4); c += 4; }
                outAnim.BoneMaskBits.clear();
                if (maskCount > 0 && c + maskCount * 4 <= meta.size()) {
                    outAnim.BoneMaskBits.resize(maskCount);
                    memcpy(outAnim.BoneMaskBits.data(), meta.data() + c, maskCount * 4);
                    c += maskCount * 4;
                }

                outAnim.HelperTracks.clear();
                uint32_t helperCount = 0;
                if (c + 4 <= meta.size()) { memcpy(&helperCount, meta.data() + c, 4); c += 4; }
                for (uint32_t i = 0; i < helperCount; i++) {
                    if (c + 4 > meta.size()) break;
                    AnimTrack ht;
                    uint32_t nl = 0; memcpy(&nl, meta.data() + c, 4); c += 4;
                    if (nl > 0 && c + nl <= meta.size()) {
                        ht.BoneName = std::string((char*)meta.data() + c, nl); c += nl;
                    }
                    ht.BoneIndex = 31450; ht.ParentIndex = -1; ht.PreFPSFlag = 0;
                    if (c + 16 <= meta.size()) {
                        memcpy(&ht.SamplesPerSecond, meta.data() + c, 4); c += 4;
                        memcpy(&ht.FrameCount, meta.data() + c, 4); c += 4;
                        memcpy(&ht.PositionFactor, meta.data() + c, 4); c += 4;
                        memcpy(&ht.ScalingFactor, meta.data() + c, 4); c += 4;
                    }
                    uint32_t posCount = 0;
                    if (c + 4 <= meta.size()) { memcpy(&posCount, meta.data() + c, 4); c += 4; }
                    if (posCount > 0 && c + posCount * 12 <= meta.size()) {
                        ht.PositionTrack.resize(posCount);
                        memcpy(ht.PositionTrack.data(), meta.data() + c, posCount * 12);
                        c += posCount * 12;
                    }
                    outAnim.HelperTracks.push_back(ht);
                }
            }
        }
        else {
            outAnim.Duration = 0.0f; outAnim.IsCyclic = true;
        }

        if (outAnim.Duration > 600.0f || outAnim.Duration < 0.0f) outAnim.Duration = 0.0f;

        std::string aName = ExtractString(targetAnim, "name");
        if (!aName.empty() && aName != "ExportedAnim") outAnim.ObjectName = CleanName(aName);

        std::string realNodesBlock = "";
        size_t searchPos = 0;
        while (true) {
            size_t pos = json.find("\"nodes\"", searchPos);
            if (pos == std::string::npos) break;

            size_t arrStart = json.find_first_of("[{", pos);
            if (arrStart != std::string::npos && json[arrStart] == '[') {
                size_t firstContent = json.find_first_not_of(" \t\r\n", arrStart + 1);
                if (firstContent != std::string::npos && json[firstContent] == '{') {
                    int depth = 0;
                    size_t endPos = arrStart;
                    for (; endPos < json.length(); endPos++) {
                        if (json[endPos] == '[' || json[endPos] == '{') depth++;
                        else if (json[endPos] == ']' || json[endPos] == '}') depth--;
                        if (depth == 0) break;
                    }
                    realNodesBlock = json.substr(arrStart, endPos - arrStart + 1);
                    break;
                }
            }
            searchPos = pos + 7;
        }
        std::vector<std::string> nodeObjs = SplitArray(realNodesBlock);

        std::vector<GltfFableNode> fableNodes;
        bool hasFableIDs = false;

        for (size_t nIdx = 0; nIdx < nodeObjs.size(); nIdx++) {
            std::string nodeObj = nodeObjs[nIdx];
            std::string extrasBlock = ExtractBlock(nodeObj, "extras");

            GltfFableNode fn;
            fn.GltfIndex = (int)nIdx;
            fn.Name = CleanName(ExtractString(nodeObj, "name"));
            fn.IsMapped = false;
            fn.FableID = 0;
            fn.ParentID = -1;

            std::vector<float> mat = ExtractFloatArray(nodeObj, "matrix");
            if (mat.size() >= 16) {
                fn.BindLocal = DirectX::XMMATRIX(mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6], mat[7], mat[8], mat[9], mat[10], mat[11], mat[12], mat[13], mat[14], mat[15]);
            }
            else {
                std::vector<float> trans = ExtractFloatArray(nodeObj, "translation");
                std::vector<float> rot = ExtractFloatArray(nodeObj, "rotation");
                std::vector<float> scl = ExtractFloatArray(nodeObj, "scale");

                DirectX::XMVECTOR vT = trans.size() >= 3 ? DirectX::XMVectorSet(trans[0], trans[1], trans[2], 1.0f) : DirectX::XMVectorSet(0, 0, 0, 1);
                DirectX::XMVECTOR vR = rot.size() >= 4 ? DirectX::XMVectorSet(rot[0], rot[1], rot[2], rot[3]) : DirectX::XMQuaternionIdentity();
                DirectX::XMVECTOR vS = scl.size() >= 3 ? DirectX::XMVectorSet(scl[0], scl[1], scl[2], 1.0f) : DirectX::XMVectorSet(1, 1, 1, 1);

                fn.BindLocal = DirectX::XMMatrixScalingFromVector(vS) * DirectX::XMMatrixRotationQuaternion(vR) * DirectX::XMMatrixTranslationFromVector(vT);
            }

            if (!extrasBlock.empty() && extrasBlock.find("FableID") != std::string::npos) {
                fn.FableID = (uint32_t)ExtractInt32(extrasBlock, "FableID");
                fn.ParentID = ExtractInt32(extrasBlock, "ParentID");
                fn.IsMapped = true;
                hasFableIDs = true;
            }

            fableNodes.push_back(fn);
        }

        if (!hasFableIDs) {
            if (!mesh.IsParsed || mesh.BoneCount == 0) {
                return "This glTF does not contain Fable metadata (FableID) and no Mesh is loaded! We cannot map these generic bones to Fable Engine IDs.";
            }

            std::vector<DirectX::XMMATRIX> dxIBM(mesh.BoneCount);
            std::vector<DirectX::XMMATRIX> dxBindGlobal(mesh.BoneCount);
            std::vector<DirectX::XMMATRIX> dxBindLocal(mesh.BoneCount);

            for (int i = 0; i < mesh.BoneCount; i++) {
                if ((i + 1) * 64 <= mesh.BoneTransformsRaw.size()) {
                    float* raw = (float*)(mesh.BoneTransformsRaw.data() + i * 64);
                    DirectX::XMMATRIX rawMatrix = DirectX::XMMATRIX(raw);
                    rawMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                    dxIBM[i] = DirectX::XMMatrixTranspose(rawMatrix);
                    dxBindGlobal[i] = DirectX::XMMatrixInverse(nullptr, dxIBM[i]);
                }
                else { dxIBM[i] = DirectX::XMMatrixIdentity(); dxBindGlobal[i] = DirectX::XMMatrixIdentity(); }
            }
            for (int i = 0; i < mesh.BoneCount; i++) {
                int par = mesh.Bones[i].ParentIndex;
                if (par == -1 || par >= mesh.BoneCount) dxBindLocal[i] = dxBindGlobal[i];
                else dxBindLocal[i] = DirectX::XMMatrixMultiply(dxBindGlobal[i], dxIBM[par]);
            }

            for (auto& fn : fableNodes) {
                std::string cleanNode = fn.Name;
                std::transform(cleanNode.begin(), cleanNode.end(), cleanNode.begin(), ::tolower);

                int targetMeshIdx = -1;
                for (int m = 0; m < mesh.BoneCount; m++) {
                    std::string cleanOrig = CleanName(mesh.BoneNames[m]);
                    std::transform(cleanOrig.begin(), cleanOrig.end(), cleanOrig.begin(), ::tolower);

                    if (!cleanNode.empty() && !cleanOrig.empty()) {
                        if (cleanNode == cleanOrig || cleanNode.find(cleanOrig) != std::string::npos || cleanOrig.find(cleanNode) != std::string::npos) {
                            targetMeshIdx = m;
                            break;
                        }
                    }
                }
                if (targetMeshIdx != -1) {
                    fn.FableID = mesh.BoneIndices[targetMeshIdx];
                    int p = mesh.Bones[targetMeshIdx].ParentIndex;
                    fn.ParentID = (p == -1 || p >= mesh.BoneCount) ? -1 : mesh.BoneIndices[p];
                    fn.BindLocal = dxBindLocal[targetMeshIdx];
                    fn.IsMapped = true;
                }
            }
        }

        outAnim.Tracks.clear();

        std::vector<AnimTrack> newTracks;
        std::map<int, int> gltfNodeToTrackIdx;

        for (const auto& fn : fableNodes) {
            if (!fn.IsMapped) continue;

            AnimTrack t;
            t.BoneIndex = fn.FableID;
            t.ParentIndex = fn.ParentID;
            t.BoneName = fn.Name;
            t.PreFPSFlag = 1;
            t.SamplesPerSecond = 30.0f;
            t.ScalingFactor = 1.0f;
            t.PositionFactor = 0.0f;
            newTracks.push_back(t);
            gltfNodeToTrackIdx[fn.GltfIndex] = (int)newTracks.size() - 1;
        }

        if (newTracks.empty()) return "No valid bones were mapped. Cannot construct animation tracks.";

        std::string buffersBlock = ExtractBlock(json, "buffers");
        std::vector<std::string> bufferObjs = SplitArray(buffersBlock);
        if (bufferObjs.empty()) return "No 'buffers' array found.";
        std::string uri = ExtractString(bufferObjs[0], "uri");
        if (uri.find("data:application") == 0) return "Base64 buffers not supported. Export as 'glTF Separate (.gltf + .bin)'.";

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of("\\/") + 1) + uri;
        std::ifstream binFile(binPath, std::ios::binary);
        if (!binFile.is_open()) return "Failed to open associated .bin file: " + uri;
        std::vector<uint8_t> binData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());

        std::vector<GltfAccessor> accessors;
        std::string accBlock = ExtractBlock(json, "accessors");
        for (const auto& obj : SplitArray(accBlock)) {
            std::string t = ExtractString(obj, "type");
            int sz = (t == "SCALAR") ? 1 : (t == "VEC3" ? 3 : (t == "VEC4" ? 4 : 16));
            accessors.push_back({ (int)ExtractFloat(obj, "bufferView"), (int)ExtractFloat(obj, "count"), sz });
        }
        std::vector<GltfBufferView> views;
        std::string bvBlock = ExtractBlock(json, "bufferViews");
        for (const auto& obj : SplitArray(bvBlock)) views.push_back({ (int)ExtractFloat(obj, "byteOffset"), (int)ExtractFloat(obj, "byteLength") });

        std::string sampBlock = ExtractBlock(targetAnim, "samplers");
        struct Samp { int inAcc; int outAcc; };
        std::vector<Samp> samplers;
        for (const auto& obj : SplitArray(sampBlock)) samplers.push_back({ (int)ExtractFloat(obj, "input"), (int)ExtractFloat(obj, "output") });

        std::string chanBlock = ExtractBlock(targetAnim, "channels");

        struct LoadedTrack {
            std::vector<float> posTimes;
            std::vector<DirectX::XMVECTOR> positions;
            std::vector<float> rotTimes;
            std::vector<DirectX::XMVECTOR> rotations;
        };
        std::map<int, LoadedTrack> mappedKeyframes;

        for (const auto& obj : SplitArray(chanBlock)) {
            int sampIdx = (int)ExtractFloat(obj, "sampler");
            std::string targetBlock = ExtractBlock(obj, "target");
            int gltfNodeIdx = (int)ExtractFloat(targetBlock, "node");
            std::string path = ExtractString(targetBlock, "path");

            if (gltfNodeToTrackIdx.find(gltfNodeIdx) == gltfNodeToTrackIdx.end()) continue;
            int trackIdx = gltfNodeToTrackIdx[gltfNodeIdx];

            auto& track = mappedKeyframes[trackIdx];
            auto& inAcc = accessors[samplers[sampIdx].inAcc];
            auto& outAcc = accessors[samplers[sampIdx].outAcc];

            float* times = (float*)(binData.data() + views[inAcc.view].offset);
            float* vals = (float*)(binData.data() + views[outAcc.view].offset);

            if (path == "translation") {
                if (track.posTimes.empty()) { for (int i = 0; i < inAcc.count; i++) track.posTimes.push_back(times[i]); }
                for (int i = 0; i < outAcc.count; i++) track.positions.push_back(DirectX::XMVectorSet(vals[i * 3], vals[i * 3 + 1], vals[i * 3 + 2], 1.0f));
            }
            else if (path == "rotation") {
                if (track.rotTimes.empty()) { for (int i = 0; i < inAcc.count; i++) track.rotTimes.push_back(times[i]); }
                for (int i = 0; i < outAcc.count; i++) track.rotations.push_back(DirectX::XMVectorSet(vals[i * 4], vals[i * 4 + 1], vals[i * 4 + 2], vals[i * 4 + 3]));
            }
        }

        if (outAnim.Duration <= 0.01f) {
            float maxT = 0.0f;
            for (const auto& [idx, t] : mappedKeyframes) {
                if (!t.posTimes.empty() && t.posTimes.back() > maxT) maxT = t.posTimes.back();
                if (!t.rotTimes.empty() && t.rotTimes.back() > maxT) maxT = t.rotTimes.back();
            }
            outAnim.Duration = maxT;
        }

        int totalFrames = (int)(outAnim.Duration * 30.0f);
        if (totalFrames < 1) totalFrames = 1;

        for (size_t tIdx = 0; tIdx < newTracks.size(); tIdx++) {
            AnimTrack t = newTracks[tIdx];
            t.FrameCount = totalFrames;

            t.PositionTrack.clear();
            t.RotationTrack.clear();
            t.PalettedPositions.clear();
            t.PalettedRotations.clear();

            int gNodeIdx = -1;
            for (const auto& pair : gltfNodeToTrackIdx) {
                if (pair.second == tIdx) { gNodeIdx = pair.first; break; }
            }

            DirectX::XMMATRIX bindLocal = DirectX::XMMatrixIdentity();
            if (gNodeIdx != -1) bindLocal = fableNodes[gNodeIdx].BindLocal;

            DirectX::XMVECTOR s, r_bind, t_bind;
            DirectX::XMMatrixDecompose(&s, &r_bind, &t_bind, bindLocal);

            Vec3 basePos = { DirectX::XMVectorGetX(t_bind), DirectX::XMVectorGetY(t_bind), DirectX::XMVectorGetZ(t_bind) };
            DirectX::XMVECTOR rConj = DirectX::XMQuaternionConjugate(r_bind);
            Vec4 baseRot = { DirectX::XMVectorGetX(rConj), DirectX::XMVectorGetY(rConj), DirectX::XMVectorGetZ(rConj), DirectX::XMVectorGetW(rConj) };

            if (mappedKeyframes.count((int)tIdx) > 0) {
                auto& lTrack = mappedKeyframes[(int)tIdx];

                for (int f = 0; f < totalFrames; f++) {
                    float targetTime = f / 30.0f;

                    if (lTrack.positions.empty() && lTrack.rotations.empty()) {
                        t.PositionTrack.push_back(basePos);
                        t.RotationTrack.push_back(baseRot);
                        continue;
                    }

                    DirectX::XMVECTOR pVec;
                    if (lTrack.positions.empty()) {
                        pVec = DirectX::XMVectorSet(basePos.x, basePos.y, basePos.z, 1.0f);
                    }
                    else if (lTrack.posTimes.size() <= 1) {
                        pVec = lTrack.positions[0];
                    }
                    else {
                        int idx0 = 0, idx1 = 0; float lerpT = 0.0f;
                        for (int i = 0; i < lTrack.posTimes.size() - 1; i++) {
                            if (targetTime >= lTrack.posTimes[i] && targetTime <= lTrack.posTimes[i + 1]) {
                                idx0 = i; idx1 = i + 1;
                                lerpT = (targetTime - lTrack.posTimes[i]) / (lTrack.posTimes[i + 1] - lTrack.posTimes[i]);
                                break;
                            }
                            if (i == lTrack.posTimes.size() - 2) { idx0 = idx1 = i + 1; }
                        }
                        pVec = Lerp(lTrack.positions[idx0], lTrack.positions[idx1], lerpT);
                    }

                    DirectX::XMVECTOR rVec;
                    if (lTrack.rotations.empty()) {
                        rVec = DirectX::XMQuaternionConjugate(DirectX::XMVectorSet(baseRot.x, baseRot.y, baseRot.z, baseRot.w));
                    }
                    else if (lTrack.rotTimes.size() <= 1) {
                        rVec = lTrack.rotations[0];
                    }
                    else {
                        int idx0 = 0, idx1 = 0; float lerpT = 0.0f;
                        for (int i = 0; i < lTrack.rotTimes.size() - 1; i++) {
                            if (targetTime >= lTrack.rotTimes[i] && targetTime <= lTrack.rotTimes[i + 1]) {
                                idx0 = i; idx1 = i + 1;
                                lerpT = (targetTime - lTrack.rotTimes[i]) / (lTrack.rotTimes[i + 1] - lTrack.rotTimes[i]);
                                break;
                            }
                            if (i == lTrack.rotTimes.size() - 2) { idx0 = idx1 = i + 1; }
                        }
                        rVec = Slerp(lTrack.rotations[idx0], lTrack.rotations[idx1], lerpT);
                    }

                    if (outAnimType == 7) {
                        DirectX::XMMATRIX mExport = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationQuaternion(rVec), DirectX::XMMatrixTranslationFromVector(pVec));
                        DirectX::XMMATRIX mInvBind = DirectX::XMMatrixInverse(nullptr, bindLocal);
                        DirectX::XMMATRIX mDelta = DirectX::XMMatrixMultiply(mExport, mInvBind);

                        DirectX::XMVECTOR s_d, r_d, t_d;
                        DirectX::XMMatrixDecompose(&s_d, &r_d, &t_d, mDelta);
                        pVec = t_d;
                        rVec = DirectX::XMQuaternionConjugate(r_d);
                    }
                    else {
                        rVec = DirectX::XMQuaternionConjugate(rVec);
                    }

                    t.PositionTrack.push_back({ DirectX::XMVectorGetX(pVec), DirectX::XMVectorGetY(pVec), DirectX::XMVectorGetZ(pVec) });
                    t.RotationTrack.push_back({ DirectX::XMVectorGetX(rVec), DirectX::XMVectorGetY(rVec), DirectX::XMVectorGetZ(rVec), DirectX::XMVectorGetW(rVec) });
                }
            }
            else {
                for (int f = 0; f < totalFrames; f++) {
                    t.PositionTrack.push_back(basePos);
                    t.RotationTrack.push_back(baseRot);
                }
            }

            float maxExtents = 0.0f;
            for (const auto& pos : t.PositionTrack) {
                if (std::abs(pos.x) > maxExtents) maxExtents = std::abs(pos.x);
                if (std::abs(pos.y) > maxExtents) maxExtents = std::abs(pos.y);
                if (std::abs(pos.z) > maxExtents) maxExtents = std::abs(pos.z);
            }

            if (maxExtents > 32700.0f) {
                t.PositionFactor = maxExtents / 32767.0f;
            }
            else if (maxExtents > 0.0f && maxExtents < 327.0f) {
                t.PositionFactor = 0.01f;
            }
            else {
                t.PositionFactor = 1.0f;
            }

            outAnim.Tracks.push_back(t);
        }

        outAnim.IsParsed = true;
        return "";
    }
}