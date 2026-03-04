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
        if (pos < json.length() && json[pos] == '-') end++; // handle negatives
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

    static DirectX::XMVECTOR Slerp(DirectX::XMVECTOR a, DirectX::XMVECTOR b, float t) { return DirectX::XMQuaternionSlerp(a, b, t); }
    static DirectX::XMVECTOR Lerp(DirectX::XMVECTOR a, DirectX::XMVECTOR b, float t) { return DirectX::XMVectorLerp(a, b, t); }

    // --- TRUE TRANSPILER ---
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

        // 1. Extract Fable Hex Meta
        std::string fableHex = ExtractString(targetAnim, "FableAnimData");
        if (!fableHex.empty()) {
            std::vector<uint8_t> meta = HexToBytes(fableHex);
            size_t c = 0;
            memcpy(&outAnimType, meta.data() + c, 4); c += 4;
            memcpy(&outAnim.Duration, meta.data() + c, 4); c += 4;
            memcpy(&outAnim.NonLoopingDuration, meta.data() + c, 4); c += 4;
            memcpy(&outAnim.MovementVector, meta.data() + c, 12); c += 12;
            memcpy(&outAnim.Rotation, meta.data() + c, 4); c += 4;
            outAnim.IsCyclic = meta[c++];
            outAnim.HasHelper = meta[c++];

            uint32_t evCount = *(uint32_t*)(meta.data() + c); c += 4;
            outAnim.TimeEvents.clear();
            for (uint32_t i = 0; i < evCount; i++) {
                uint32_t nl = *(uint32_t*)(meta.data() + c); c += 4;
                TimeEvent ev;
                ev.Name = std::string((char*)meta.data() + c, nl); c += nl;
                memcpy(&ev.Time, meta.data() + c, 4); c += 4;
                outAnim.TimeEvents.push_back(ev);
            }
            uint32_t maskCount = *(uint32_t*)(meta.data() + c); c += 4;
            outAnim.BoneMaskBits.clear();
            if (maskCount > 0) {
                outAnim.BoneMaskBits.resize(maskCount);
                memcpy(outAnim.BoneMaskBits.data(), meta.data() + c, maskCount * 4);
            }
        }
        else {
            outAnim.Duration = 0.0f; outAnim.IsCyclic = true;
        }

        std::string aName = ExtractString(targetAnim, "name");
        if (!aName.empty() && aName != "ExportedAnim") outAnim.ObjectName = aName;

        // 2. Preserve original tracks to act as a Transpiler mapping
        std::vector<AnimTrack> originalTracks = outAnim.Tracks;
        outAnim.Tracks.clear();

        // 3. Map glTF Nodes
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
        std::vector<std::string> gltfNodeNames;
        for (const auto& obj : nodeObjs) gltfNodeNames.push_back(ExtractString(obj, "name"));

        // 4. Open Binary Buffer
        std::string buffersBlock = ExtractBlock(json, "buffers");
        std::vector<std::string> bufferObjs = SplitArray(buffersBlock);
        if (bufferObjs.empty()) return "No 'buffers' array found.";
        std::string uri = ExtractString(bufferObjs[0], "uri");
        if (uri.find("data:application") == 0) return "Base64 buffers not supported. Export as 'glTF Separate (.gltf + .bin)'.";

        std::string binPath = gltfPath.substr(0, gltfPath.find_last_of("\\/") + 1) + uri;
        std::ifstream binFile(binPath, std::ios::binary);
        if (!binFile.is_open()) return "Failed to open associated .bin file: " + uri;
        std::vector<uint8_t> binData((std::istreambuf_iterator<char>(binFile)), std::istreambuf_iterator<char>());

        // 5. Read Accessors & BufferViews
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

        // 6. DELTA CHECK (Only required if outAnimType is 7)
        std::vector<DirectX::XMMATRIX> dxBindLocal;
        if (outAnimType == 7) {
            if (!mesh.IsParsed) return "Delta Animations (Type 7) mathematically require the skeleton. Please open the character's mesh before importing.";

            dxBindLocal.resize(mesh.BoneCount);
            std::vector<DirectX::XMMATRIX> dxIBM(mesh.BoneCount);
            std::vector<DirectX::XMMATRIX> dxBindGlobal(mesh.BoneCount);

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
        }

        // 7. Parse Samplers & Channels
        std::string sampBlock = ExtractBlock(targetAnim, "samplers");
        struct Samp { int inAcc; int outAcc; };
        std::vector<Samp> samplers;
        for (const auto& obj : SplitArray(sampBlock)) samplers.push_back({ (int)ExtractFloat(obj, "input"), (int)ExtractFloat(obj, "output") });

        std::string chanBlock = ExtractBlock(targetAnim, "channels");

        // FIX: Independent time arrays to survive Blender's keyframe optimization
        struct LoadedTrack {
            std::vector<float> posTimes;
            std::vector<DirectX::XMVECTOR> positions;
            std::vector<float> rotTimes;
            std::vector<DirectX::XMVECTOR> rotations;
        };
        std::map<int, LoadedTrack> mappedTracks;

        auto ExtractInt32 = [](const std::string& json, const std::string& key) -> int32_t {
            size_t pos = json.find("\"" + key + "\""); if (pos == std::string::npos) return 0;
            pos = json.find(':', pos); if (pos == std::string::npos) return 0; pos++;
            while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
            size_t end = pos; if (pos < json.length() && json[pos] == '-') end++;
            while (end < json.length() && isdigit(json[end])) end++;
            if (pos == end) return 0;
            try { return (int32_t)std::stoll(json.substr(pos, end - pos)); }
            catch (...) { return 0; }
            };

        auto CleanName = [](std::string s) {
            std::string r;
            for (char c : s) if (isalnum((unsigned char)c)) r += tolower((unsigned char)c);
            return r;
            };

        for (const auto& obj : SplitArray(chanBlock)) {
            int sampIdx = (int)ExtractFloat(obj, "sampler");
            std::string targetBlock = ExtractBlock(obj, "target");
            int gltfNodeIdx = (int)ExtractFloat(targetBlock, "node");
            std::string path = ExtractString(targetBlock, "path");

            if (gltfNodeIdx >= nodeObjs.size()) continue;

            std::string nodeObj = nodeObjs[gltfNodeIdx];
            std::string nodeName = ExtractString(nodeObj, "name");
            std::string extrasBlock = ExtractBlock(nodeObj, "extras");

            int targetOrigIdx = -1;

            if (!extrasBlock.empty() && extrasBlock.find("FableID") != std::string::npos) {
                uint32_t tBoneIdx = (uint32_t)ExtractInt32(extrasBlock, "FableID");
                for (size_t i = 0; i < originalTracks.size(); i++) {
                    if (originalTracks[i].BoneIndex == tBoneIdx) { targetOrigIdx = (int)i; break; }
                }
            }

            if (targetOrigIdx == -1) {
                std::string cleanNode = CleanName(nodeName);
                for (size_t i = 0; i < originalTracks.size(); i++) {
                    std::string cleanOrig = CleanName(originalTracks[i].BoneName);
                    if (!cleanNode.empty() && cleanNode == cleanOrig) {
                        targetOrigIdx = (int)i; break;
                    }
                }
            }

            if (targetOrigIdx == -1) continue;

            auto& track = mappedTracks[targetOrigIdx];
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
            for (const auto& [idx, t] : mappedTracks) {
                if (!t.posTimes.empty() && t.posTimes.back() > maxT) maxT = t.posTimes.back();
                if (!t.rotTimes.empty() && t.rotTimes.back() > maxT) maxT = t.rotTimes.back();
            }
            outAnim.Duration = maxT;
        }

        int totalFrames = (int)(outAnim.Duration * 30.0f);
        if (totalFrames < 2) totalFrames = 2;

        outAnim.Tracks.clear();

        for (size_t origIdx = 0; origIdx < originalTracks.size(); origIdx++) {
            AnimTrack t = originalTracks[origIdx];

            t.FrameCount = totalFrames;
            t.SamplesPerSecond = 30.0f;
            t.PositionFactor = 1.0f;
            t.ScalingFactor = 1.0f;

            t.PositionTrack.clear();
            t.RotationTrack.clear();
            t.PalettedPositions.clear();
            t.PalettedRotations.clear();

            if (mappedTracks.count((int)origIdx) > 0) {
                auto& lTrack = mappedTracks[(int)origIdx];

                int meshLocalIdx = -1;
                if (outAnimType == 7 && mesh.BoneCount > 0) {
                    for (int m = 0; m < mesh.BoneCount; m++) {
                        if (mesh.BoneIndices[m] == t.BoneIndex) { meshLocalIdx = m; break; }
                    }
                }

                for (int f = 0; f < totalFrames; f++) {
                    float targetTime = f / 30.0f;

                    // --- Independent Position Lerp ---
                    DirectX::XMVECTOR pVec = DirectX::XMVectorZero();
                    if (lTrack.positions.empty()) {
                        pVec = DirectX::XMVectorZero();
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

                    // --- Independent Rotation Slerp ---
                    DirectX::XMVECTOR rVec = DirectX::XMVectorSet(0, 0, 0, 1);
                    if (lTrack.rotations.empty()) {
                        rVec = DirectX::XMVectorSet(0, 0, 0, 1);
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

                    if (outAnimType == 7 && meshLocalIdx != -1 && meshLocalIdx < dxBindLocal.size()) {
                        DirectX::XMMATRIX mExport = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationQuaternion(rVec), DirectX::XMMatrixTranslationFromVector(pVec));
                        DirectX::XMMATRIX mInvBind = DirectX::XMMatrixInverse(nullptr, dxBindLocal[meshLocalIdx]);
                        DirectX::XMMATRIX mDelta = DirectX::XMMatrixMultiply(mExport, mInvBind);
                        DirectX::XMVECTOR s, r, t_vec;
                        DirectX::XMMatrixDecompose(&s, &r, &t_vec, mDelta);
                        pVec = t_vec;
                        rVec = DirectX::XMQuaternionConjugate(r);
                    }
                    else {
                        rVec = DirectX::XMQuaternionConjugate(rVec);
                    }

                    t.PositionTrack.push_back({ DirectX::XMVectorGetX(pVec), DirectX::XMVectorGetY(pVec), DirectX::XMVectorGetZ(pVec) });
                    t.RotationTrack.push_back({ DirectX::XMVectorGetX(rVec), DirectX::XMVectorGetY(rVec), DirectX::XMVectorGetZ(rVec), DirectX::XMVectorGetW(rVec) });
                }
            }
            else {
                Vec3 basePos = { 0,0,0 };
                Vec4 baseRot = { 0,0,0,1 };

                if (!originalTracks[origIdx].PositionTrack.empty()) {
                    auto p = originalTracks[origIdx].PositionTrack[0];
                    basePos = { p.x, p.y, p.z };
                }
                if (!originalTracks[origIdx].RotationTrack.empty()) {
                    auto r = originalTracks[origIdx].RotationTrack[0];
                    baseRot = { r.x, r.y, r.z, r.w };
                }

                for (int f = 0; f < totalFrames; f++) {
                    t.PositionTrack.push_back({ basePos.x, basePos.y, basePos.z });
                    t.RotationTrack.push_back({ baseRot.x, baseRot.y, baseRot.z, baseRot.w });
                }
            }

            for (int f = 0; f < totalFrames; f++) {
                t.PalettedPositions.push_back((uint8_t)f);
                t.PalettedRotations.push_back((uint8_t)f);
            }

            outAnim.Tracks.push_back(t);
        }

        outAnim.IsParsed = true;
        return ""; // Success!
    }
}