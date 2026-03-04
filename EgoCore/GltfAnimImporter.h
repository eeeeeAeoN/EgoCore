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
        std::string nodesBlock = ExtractBlock(json, "nodes");
        std::vector<std::string> nodeObjs = SplitArray(nodesBlock);
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

        struct LoadedTrack {
            int originalTrackIdx;
            std::vector<float> times;
            std::vector<DirectX::XMVECTOR> positions;
            std::vector<DirectX::XMVECTOR> rotations;
        };
        std::map<int, LoadedTrack> loadedTracks;

        for (const auto& obj : SplitArray(chanBlock)) {
            int sampIdx = (int)ExtractFloat(obj, "sampler");
            std::string targetBlock = ExtractBlock(obj, "target");
            int gltfNodeIdx = (int)ExtractFloat(targetBlock, "node");
            std::string path = ExtractString(targetBlock, "path");

            if (gltfNodeIdx >= gltfNodeNames.size()) continue;

            // --- TRANSPILER MATCHER: Map glTF Node to original AnimTrack Name ---
            std::string nodeName = gltfNodeNames[gltfNodeIdx];
            std::transform(nodeName.begin(), nodeName.end(), nodeName.begin(), ::tolower);

            int originalIdx = -1;
            for (size_t i = 0; i < originalTracks.size(); i++) {
                std::string fName = originalTracks[i].BoneName;
                std::transform(fName.begin(), fName.end(), fName.begin(), ::tolower);
                // Fable names contain garbage. We check if the node name starts with the bone name.
                if (nodeName.find(fName.c_str()) != std::string::npos || fName.find(nodeName.c_str()) != std::string::npos) {
                    originalIdx = i; break;
                }
            }
            if (originalIdx == -1) continue;

            auto& track = loadedTracks[originalIdx];
            track.originalTrackIdx = originalIdx;

            auto& inAcc = accessors[samplers[sampIdx].inAcc];
            auto& outAcc = accessors[samplers[sampIdx].outAcc];

            float* times = (float*)(binData.data() + views[inAcc.view].offset);
            if (track.times.empty()) {
                for (int i = 0; i < inAcc.count; i++) track.times.push_back(times[i]);
            }

            float* vals = (float*)(binData.data() + views[outAcc.view].offset);
            if (path == "translation") {
                for (int i = 0; i < outAcc.count; i++) track.positions.push_back(DirectX::XMVectorSet(vals[i * 3], vals[i * 3 + 1], vals[i * 3 + 2], 1.0f));
            }
            else if (path == "rotation") {
                for (int i = 0; i < outAcc.count; i++) track.rotations.push_back(DirectX::XMVectorSet(vals[i * 4], vals[i * 4 + 1], vals[i * 4 + 2], vals[i * 4 + 3]));
            }
        }

        if (outAnim.Duration <= 0.01f) {
            float maxT = 0.0f;
            for (const auto& [n, t] : loadedTracks) { if (!t.times.empty() && t.times.back() > maxT) maxT = t.times.back(); }
            outAnim.Duration = maxT;
        }

        // 8. Rebuild Fable Track Array
        int totalFrames = (int)(outAnim.Duration * 30.0f);
        if (totalFrames < 2) totalFrames = 2;

        for (size_t origIdx = 0; origIdx < originalTracks.size(); origIdx++) {
            if (loadedTracks.count(origIdx) == 0) {
                // Not animated in glTF? Keep original keyframes exactly as they were!
                outAnim.Tracks.push_back(originalTracks[origIdx]);
                continue;
            }

            auto& lTrack = loadedTracks[origIdx];

            // Transplant exact Fable specific variables from the old track
            AnimTrack t;
            t.BoneIndex = originalTracks[origIdx].BoneIndex;
            t.ParentIndex = originalTracks[origIdx].ParentIndex;
            t.BoneName = originalTracks[origIdx].BoneName; // Preserves binary garbage padding!
            t.PreFPSFlag = originalTracks[origIdx].PreFPSFlag;
            memcpy(t.PostFrameFlags, originalTracks[origIdx].PostFrameFlags, 4);
            t.SamplesPerSecond = 30.0f;
            t.FrameCount = totalFrames;
            t.PositionFactor = 1.0f;
            t.ScalingFactor = 1.0f;

            for (int f = 0; f < totalFrames; f++) {
                float targetTime = f / 30.0f;
                int idx0 = 0, idx1 = 0; float lerpT = 0.0f;
                for (int i = 0; i < lTrack.times.size() - 1; i++) {
                    if (targetTime >= lTrack.times[i] && targetTime <= lTrack.times[i + 1]) {
                        idx0 = i; idx1 = i + 1;
                        lerpT = (targetTime - lTrack.times[i]) / (lTrack.times[i + 1] - lTrack.times[i]);
                        break;
                    }
                    if (i == lTrack.times.size() - 2) { idx0 = idx1 = i + 1; }
                }

                DirectX::XMVECTOR pVec = lTrack.positions.empty() ? DirectX::XMVectorZero() : Lerp(lTrack.positions[idx0], lTrack.positions[idx1], lerpT);
                DirectX::XMVECTOR rVec = lTrack.rotations.empty() ? DirectX::XMVectorSet(0, 0, 0, 1) : Slerp(lTrack.rotations[idx0], lTrack.rotations[idx1], lerpT);

                if (outAnimType == 7 && t.BoneIndex < dxBindLocal.size()) {
                    DirectX::XMMATRIX mExport = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationQuaternion(rVec), DirectX::XMMatrixTranslationFromVector(pVec));
                    DirectX::XMMATRIX mInvBind = DirectX::XMMatrixInverse(nullptr, dxBindLocal[t.BoneIndex]);
                    DirectX::XMMATRIX mDelta = DirectX::XMMatrixMultiply(mExport, mInvBind);
                    DirectX::XMVECTOR s, r, t_vec;
                    DirectX::XMMatrixDecompose(&s, &r, &t_vec, mDelta);
                    pVec = t_vec;
                    rVec = DirectX::XMQuaternionConjugate(r);
                }
                else {
                    rVec = DirectX::XMQuaternionConjugate(rVec); // Convert glTF Rot back to Fable Rot
                }

                t.PositionTrack.push_back({ DirectX::XMVectorGetX(pVec), DirectX::XMVectorGetY(pVec), DirectX::XMVectorGetZ(pVec) });
                t.RotationTrack.push_back({ DirectX::XMVectorGetX(rVec), DirectX::XMVectorGetY(rVec), DirectX::XMVectorGetZ(rVec), DirectX::XMVectorGetW(rVec) });
            }

            // Create 1:1 dummy palettes
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