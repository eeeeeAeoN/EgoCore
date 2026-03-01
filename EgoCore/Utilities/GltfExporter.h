#pragma once
#include "MeshParser.h"
#include "BBMParser.h" 
#include "AnimParser.h"
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <map>
#include <algorithm>
#include <directxmath.h> // -- I was forced to include it since delta animation baking was proving difficult 

namespace GltfExporter {

    struct Mat4 { float m[16]; };
    struct Vec3 { float x, y, z; };
    struct Vec4 { float x, y, z, w; };

    static Mat4 Identity() { return { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }; }

    static Mat4 Multiply(const Mat4& A, const Mat4& B) {
        Mat4 R;
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
            R.m[i * 4 + j] = A.m[i * 4 + 0] * B.m[0 * 4 + j] + A.m[i * 4 + 1] * B.m[1 * 4 + j] + A.m[i * 4 + 2] * B.m[2 * 4 + j] + A.m[i * 4 + 3] * B.m[3 * 4 + j];
        return R;
    }

    // MATH HELPER: Transform to Matrix
    static Mat4 TransformToMat4(const ::Vec3& t, const ::Vec4& q) {
        Mat4 m = Identity();
        float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        m.m[0] = 1.0f - 2.0f * (yy + zz); m.m[1] = 2.0f * (xy + wz);       m.m[2] = 2.0f * (xz - wy);
        m.m[4] = 2.0f * (xy - wz);       m.m[5] = 1.0f - 2.0f * (xx + zz); m.m[6] = 2.0f * (yz + wx);
        m.m[8] = 2.0f * (xz + wy);       m.m[9] = 2.0f * (yz - wx);       m.m[10] = 1.0f - 2.0f * (xx + yy);
        m.m[12] = t.x; m.m[13] = t.y; m.m[14] = t.z;
        return m;
    }

    // MATH HELPER: Matrix to Transform
    static void Mat4ToTransform(const Mat4& m, ::Vec3& t, ::Vec4& q) {
        t.x = m.m[12]; t.y = m.m[13]; t.z = m.m[14];
        float tr = m.m[0] + m.m[5] + m.m[10];
        if (tr > 0.0f) {
            float S = sqrtf(tr + 1.0f) * 2.0f;
            q.w = 0.25f * S; q.x = (m.m[6] - m.m[9]) / S; q.y = (m.m[8] - m.m[2]) / S; q.z = (m.m[1] - m.m[4]) / S;
        }
        else if ((m.m[0] > m.m[5]) && (m.m[0] > m.m[10])) {
            float S = sqrtf(1.0f + m.m[0] - m.m[5] - m.m[10]) * 2.0f;
            q.w = (m.m[6] - m.m[9]) / S; q.x = 0.25f * S; q.y = (m.m[1] + m.m[4]) / S; q.z = (m.m[8] + m.m[2]) / S;
        }
        else if (m.m[5] > m.m[10]) {
            float S = sqrtf(1.0f + m.m[5] - m.m[0] - m.m[10]) * 2.0f;
            q.w = (m.m[8] - m.m[2]) / S; q.x = (m.m[1] + m.m[4]) / S; q.y = 0.25f * S; q.z = (m.m[6] + m.m[9]) / S;
        }
        else {
            float S = sqrtf(1.0f + m.m[10] - m.m[0] - m.m[5]) * 2.0f;
            q.w = (m.m[1] - m.m[4]) / S; q.x = (m.m[8] + m.m[2]) / S; q.y = (m.m[6] + m.m[9]) / S; q.z = 0.25f * S;
        }
    }

    static bool Invert(const Mat4& m, Mat4& out) {
        float inv[16], det;
        inv[0] = m.m[5] * m.m[10] * m.m[15] - m.m[5] * m.m[11] * m.m[14] - m.m[9] * m.m[6] * m.m[15] + m.m[9] * m.m[7] * m.m[14] + m.m[13] * m.m[6] * m.m[11] - m.m[13] * m.m[7] * m.m[10];
        inv[4] = -m.m[4] * m.m[10] * m.m[15] + m.m[4] * m.m[11] * m.m[14] + m.m[8] * m.m[6] * m.m[15] - m.m[8] * m.m[7] * m.m[14] - m.m[12] * m.m[6] * m.m[11] + m.m[12] * m.m[7] * m.m[10];
        inv[8] = m.m[4] * m.m[9] * m.m[15] - m.m[4] * m.m[11] * m.m[13] - m.m[8] * m.m[5] * m.m[15] + m.m[8] * m.m[7] * m.m[13] + m.m[12] * m.m[5] * m.m[11] - m.m[12] * m.m[7] * m.m[9];
        inv[12] = -m.m[4] * m.m[9] * m.m[14] + m.m[4] * m.m[10] * m.m[13] + m.m[8] * m.m[5] * m.m[14] - m.m[8] * m.m[6] * m.m[13] - m.m[12] * m.m[5] * m.m[10] + m.m[12] * m.m[6] * m.m[9];
        inv[1] = -m.m[1] * m.m[10] * m.m[15] + m.m[1] * m.m[11] * m.m[14] + m.m[9] * m.m[2] * m.m[15] - m.m[9] * m.m[3] * m.m[14] - m.m[13] * m.m[2] * m.m[11] + m.m[13] * m.m[3] * m.m[10];
        inv[5] = m.m[0] * m.m[10] * m.m[15] - m.m[0] * m.m[11] * m.m[14] - m.m[8] * m.m[2] * m.m[15] + m.m[8] * m.m[3] * m.m[14] + m.m[12] * m.m[2] * m.m[11] - m.m[12] * m.m[3] * m.m[10];
        inv[9] = -m.m[0] * m.m[9] * m.m[15] + m.m[0] * m.m[11] * m.m[13] + m.m[8] * m.m[1] * m.m[15] - m.m[8] * m.m[3] * m.m[13] - m.m[12] * m.m[1] * m.m[11] + m.m[12] * m.m[3] * m.m[9];
        inv[13] = m.m[0] * m.m[9] * m.m[14] - m.m[0] * m.m[10] * m.m[13] - m.m[8] * m.m[1] * m.m[14] + m.m[8] * m.m[2] * m.m[13] + m.m[12] * m.m[1] * m.m[10] - m.m[12] * m.m[2] * m.m[9];
        inv[2] = m.m[1] * m.m[6] * m.m[15] - m.m[1] * m.m[7] * m.m[14] - m.m[5] * m.m[2] * m.m[15] + m.m[5] * m.m[3] * m.m[14] + m.m[13] * m.m[2] * m.m[7] - m.m[13] * m.m[3] * m.m[6];
        inv[6] = -m.m[0] * m.m[6] * m.m[15] + m.m[0] * m.m[7] * m.m[14] + m.m[4] * m.m[2] * m.m[15] - m.m[4] * m.m[3] * m.m[14] - m.m[12] * m.m[2] * m.m[7] + m.m[12] * m.m[3] * m.m[6];
        inv[10] = m.m[0] * m.m[5] * m.m[15] - m.m[0] * m.m[7] * m.m[13] - m.m[4] * m.m[1] * m.m[15] + m.m[4] * m.m[3] * m.m[13] + m.m[12] * m.m[1] * m.m[7] - m.m[12] * m.m[3] * m.m[5];
        inv[14] = -m.m[0] * m.m[5] * m.m[14] + m.m[0] * m.m[6] * m.m[13] + m.m[4] * m.m[1] * m.m[14] - m.m[4] * m.m[2] * m.m[13] - m.m[12] * m.m[1] * m.m[6] + m.m[12] * m.m[2] * m.m[5];
        inv[3] = -m.m[1] * m.m[6] * m.m[11] + m.m[1] * m.m[7] * m.m[10] + m.m[5] * m.m[2] * m.m[11] - m.m[5] * m.m[3] * m.m[10] - m.m[9] * m.m[2] * m.m[7] + m.m[9] * m.m[3] * m.m[6];
        inv[7] = m.m[0] * m.m[6] * m.m[11] - m.m[0] * m.m[7] * m.m[10] - m.m[4] * m.m[2] * m.m[11] + m.m[4] * m.m[3] * m.m[10] + m.m[8] * m.m[2] * m.m[7] - m.m[8] * m.m[3] * m.m[6];
        inv[11] = -m.m[0] * m.m[5] * m.m[11] + m.m[0] * m.m[7] * m.m[9] + m.m[4] * m.m[1] * m.m[11] - m.m[4] * m.m[3] * m.m[9] - m.m[8] * m.m[1] * m.m[7] + m.m[8] * m.m[3] * m.m[5];
        inv[15] = m.m[0] * m.m[5] * m.m[10] - m.m[0] * m.m[6] * m.m[9] - m.m[4] * m.m[1] * m.m[10] + m.m[4] * m.m[2] * m.m[9] + m.m[8] * m.m[1] * m.m[6] - m.m[8] * m.m[2] * m.m[5];
        det = m.m[0] * inv[0] + m.m[1] * inv[4] + m.m[2] * inv[8] + m.m[3] * inv[12];
        if (det == 0) return false;
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) out.m[i] = inv[i] * det;
        return true;
    }

    static void Transpose(const Mat4& src, Mat4& dst) {
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) dst.m[c * 4 + r] = src.m[r * 4 + c];
    }

    struct GltfBuffer {
        std::vector<uint8_t> data;
        void Pad() { while (data.size() % 4 != 0) data.push_back(0); }
        int Write(const void* src, size_t size) { Pad(); size_t s = data.size(); data.resize(s + size); memcpy(data.data() + s, src, size); return (int)s; }
    };

    static std::string Esc(const std::string& s) {
        std::stringstream ss; ss << "\"";
        for (char c : s) {
            switch (c) {
            case '"': ss << "\\\""; break; case '\\': ss << "\\\\"; break; case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break; case '\n': ss << "\\n"; break; case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                else ss << c;
            }
        }
        ss << "\""; return ss.str();
    }

    static std::string ToHex(const std::vector<uint8_t>& data) {
        std::stringstream ss; ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < data.size(); ++i) ss << std::setw(2) << (int)data[i];
        return ss.str();
    }

    static void UnpackNormal(uint32_t pk, float& x, float& y, float& z) {
        int32_t ix = pk & 0x7FF; if (ix & 0x400) ix |= 0xFFFFF800; int32_t iy = (pk >> 11) & 0x7FF; if (iy & 0x400) iy |= 0xFFFFF800; int32_t iz = (pk >> 22); if (iz & 0x200) iz |= 0xFFFFFC00;
        x = (float)ix / 1023.0f; y = (float)iy / 1023.0f; z = (float)iz / 511.0f; float l = sqrtf(x * x + y * y + z * z); if (l > 0) { x /= l; y /= l; z /= l; }
    }

    struct Accessor {
        int view; int count; int compType; std::string type; float min[3]; float max[3]; bool hasBounds;
        Accessor(int v, int c, int ct, std::string t, float* mi, float* ma, bool b) : view(v), count(c), compType(ct), type(t), hasBounds(b) {
            if (mi) memcpy(min, mi, 12); else memset(min, 0, 12);
            if (ma) memcpy(max, ma, 12); else memset(max, 0, 12);
        }
    };
    struct BufferView { int offset; int length; int target; };

    static bool Export(const C3DMeshContent& mesh, const std::string& filename, const AnimParser* anim = nullptr, int animType = 6) {
        if (mesh.Primitives.empty()) return false;

        std::string binFilename = filename.substr(0, filename.find_last_of('.')) + ".bin";
        std::string binUri = binFilename.substr(binFilename.find_last_of("\\/") + 1);

        std::ofstream json(filename);
        if (!json.is_open()) return false;
        json.imbue(std::locale("C"));

        GltfBuffer bin;
        std::vector<Accessor> accessors;
        std::vector<BufferView> bufferViews;
        std::stringstream primitivesJson;

        std::map<uint16_t, uint16_t> globalToLocalBone;
        for (int i = 0; i < mesh.BoneCount; i++) if (i < mesh.BoneIndices.size()) globalToLocalBone[mesh.BoneIndices[i]] = (uint16_t)i;

        std::vector<Mat4> IBMs;
        std::vector<Mat4> WorldTransforms;
        IBMs.resize(mesh.BoneCount);
        WorldTransforms.resize(mesh.BoneCount);

        for (int i = 0; i < mesh.BoneCount; i++) {
            if ((i + 1) * 64 <= mesh.BoneTransformsRaw.size()) {
                Mat4 m; memcpy(m.m, mesh.BoneTransformsRaw.data() + i * 64, 64);
                IBMs[i] = m; Invert(IBMs[i], WorldTransforms[i]);
            }
            else { IBMs[i] = Identity(); WorldTransforms[i] = Identity(); }
        }

        int boneStartIdx = 0; int meshNodeIdx = mesh.BoneCount; int helperStartIdx = meshNodeIdx + 1;
        int dummyStartIdx = helperStartIdx + mesh.Helpers.size(); int genStartIdx = dummyStartIdx + mesh.Dummies.size();
        int volStartIdx = genStartIdx + mesh.Generators.size(); int rootWrapperIdx = volStartIdx + mesh.Volumes.size();
        int totalNodes = rootWrapperIdx + 1;

        std::vector<std::vector<int>> nodeChildren(totalNodes);
        for (int i = 0; i < mesh.BoneCount; i++) { int p = mesh.Bones[i].ParentIndex; if (p != -1) nodeChildren[p].push_back(i); }
        nodeChildren[rootWrapperIdx].push_back(meshNodeIdx);

        auto LinkToParent = [&](int myIdx, int boneGlobalID) {
            int parentIdx = -1;
            if (globalToLocalBone.count((uint16_t)boneGlobalID)) parentIdx = globalToLocalBone[(uint16_t)boneGlobalID];
            if (parentIdx >= 0 && parentIdx < mesh.BoneCount) nodeChildren[parentIdx].push_back(myIdx);
            else nodeChildren[rootWrapperIdx].push_back(myIdx);
            };

        for (size_t i = 0; i < mesh.Helpers.size(); i++) LinkToParent(helperStartIdx + i, mesh.Helpers[i].BoneIndex);
        for (size_t i = 0; i < mesh.Dummies.size(); i++) LinkToParent(dummyStartIdx + i, mesh.Dummies[i].BoneIndex);
        for (size_t i = 0; i < mesh.Generators.size(); i++) LinkToParent(genStartIdx + i, mesh.Generators[i].BoneIndex);
        for (size_t i = 0; i < mesh.Volumes.size(); i++) nodeChildren[rootWrapperIdx].push_back(volStartIdx + i);
        for (int i = 0; i < mesh.BoneCount; i++) if (mesh.Bones[i].ParentIndex == -1) nodeChildren[rootWrapperIdx].push_back(i);

        json << "{\"asset\":{\"version\":\"2.0\",\"generator\":\"FableBankExplorer\"},";
        bool isRepeated = false; bool fP = true;

        for (const auto& prim : mesh.Primitives) {
            if (prim.RepeatingMeshReps > 1) isRepeated = true;
            if (!fP) primitivesJson << ","; fP = false; primitivesJson << "\n{\"attributes\":{";

            std::vector<uint16_t> idx;
            // FIX 1: Triangle Strips loop boundary fixed. 'c' is the number of triangles!
            auto AddIdx = [&](uint32_t c, uint32_t s, bool str) {
                if (str) {
                    for (uint32_t k = 0; k < c; k++) {
                        if (s + k + 2 >= prim.IndexBuffer.size()) break; // Safety Check
                        uint16_t i0 = prim.IndexBuffer[s + k];
                        uint16_t i1 = prim.IndexBuffer[s + k + 1];
                        uint16_t i2 = prim.IndexBuffer[s + k + 2];
                        if (i0 == i1 || i1 == i2 || i0 == i2) continue;
                        if (i0 == 0xFFFF || i1 == 0xFFFF || i2 == 0xFFFF) continue;
                        if (k % 2) { idx.push_back(i0); idx.push_back(i2); idx.push_back(i1); }
                        else { idx.push_back(i0); idx.push_back(i1); idx.push_back(i2); }
                    }
                }
                else {
                    for (uint32_t k = 0; k < c * 3; k++) {
                        if (s + k < prim.IndexBuffer.size()) { uint16_t i = prim.IndexBuffer[s + k]; if (i != 0xFFFF) idx.push_back(i); }
                    }
                }
                };

            if (prim.RepeatingMeshReps > 1) { if (!prim.IndexBuffer.empty()) AddIdx(prim.IndexBuffer.size() / 3, 0, false); }
            else {
                if (!prim.StaticBlocks.empty()) for (const auto& b : prim.StaticBlocks) AddIdx(b.PrimitiveCount, b.StartIndex, b.IsStrip);
                else if (!prim.AnimatedBlocks.empty()) for (const auto& b : prim.AnimatedBlocks) AddIdx(b.PrimitiveCount, b.StartIndex, b.IsStrip);
                else if (!prim.IndexBuffer.empty()) AddIdx(prim.IndexBuffer.size() / 3, 0, false);
            }

            bufferViews.push_back({ bin.Write(idx.data(), idx.size() * 2), (int)idx.size() * 2, 34963 });
            accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)idx.size(), 5123, "SCALAR", 0, 0, false));
            int idxAcc = (int)accessors.size() - 1;

            std::vector<float> posData, normData, uvData, weightData; std::vector<uint16_t> jointData;
            float min[3] = { 1e9,1e9,1e9 }, max[3] = { -1e9,-1e9,-1e9 };
            int blk = 0, proc = 0, limit = (prim.AnimatedBlocks.empty() ? 999999 : prim.AnimatedBlocks[0].VertexCount);

            bool hasBones = (prim.AnimatedBlockCount > 0);
            bool isPosComp = (prim.InitFlags & 4) != 0 && (prim.InitFlags & 0x10) == 0;
            bool isNormComp = (prim.InitFlags & 4) != 0;

            size_t iOff = isPosComp ? 4 : 12; size_t wOff = iOff + 4; size_t normOff = iOff + (hasBones ? 8 : 0); size_t uOff = normOff + (isNormComp ? 4 : 12);

            for (int v = 0; v < prim.VertexCount; v++) {
                if (hasBones && proc >= limit) { blk++; proc = 0; if (blk < prim.AnimatedBlocks.size()) limit = prim.AnimatedBlocks[blk].VertexCount; }
                proc++;

                size_t off = v * prim.VertexStride; if (off + 12 > prim.VertexBuffer.size()) break;
                float x, y, z, nx = 0, ny = 1, nz = 0, u = 0, v_c = 0;

                if (prim.RepeatingMeshReps > 1) {
                    float* p = (float*)(prim.VertexBuffer.data() + off); x = p[0]; y = p[1]; z = p[2];
                    if (off + 24 <= prim.VertexBuffer.size()) { float* n = (float*)(prim.VertexBuffer.data() + off + 12); nx = n[0]; ny = n[1]; nz = n[2]; }
                    if (off + 32 <= prim.VertexBuffer.size()) { float* t = (float*)(prim.VertexBuffer.data() + off + 24); u = t[0]; v_c = t[1]; }
                }
                else {
                    if (isPosComp) { UnpackPOSPACKED3(*(uint32_t*)(prim.VertexBuffer.data() + off), prim.Compression.Scale, prim.Compression.Offset, x, y, z); }
                    else { float* p = (float*)(prim.VertexBuffer.data() + off); x = p[0]; y = p[1]; z = p[2]; }

                    if (off + normOff + 4 <= prim.VertexBuffer.size()) {
                        if (isNormComp) UnpackNormal(*(uint32_t*)(prim.VertexBuffer.data() + off + normOff), nx, ny, nz);
                        else { float* n = (float*)(prim.VertexBuffer.data() + off + normOff); nx = n[0]; ny = n[1]; nz = n[2]; }
                    }
                    if (off + uOff + 4 <= prim.VertexBuffer.size()) {
                        if (isNormComp) { int16_t* t = (int16_t*)(prim.VertexBuffer.data() + off + uOff); u = DecompressUV(t[0]); v_c = DecompressUV(t[1]); }
                        else { float* t = (float*)(prim.VertexBuffer.data() + off + uOff); u = t[0]; v_c = t[1]; }
                    }
                }

                if (x < min[0]) min[0] = x; if (y < min[1]) min[1] = y; if (z < min[2]) min[2] = z;
                if (x > max[0]) max[0] = x; if (y > max[1]) max[1] = y; if (z > max[2]) max[2] = z;
                posData.push_back(x); posData.push_back(y); posData.push_back(z);
                normData.push_back(nx); normData.push_back(ny); normData.push_back(nz);
                uvData.push_back(u); uvData.push_back(v_c);

                if (hasBones) {
                    if (off + iOff + 4 <= prim.VertexBuffer.size()) {
                        uint8_t* wgt = (uint8_t*)(prim.VertexBuffer.data() + off + wOff);
                        uint8_t* ind = (uint8_t*)(prim.VertexBuffer.data() + off + iOff);
                        float w[4] = { wgt[0] / 255.0f, wgt[1] / 255.0f, wgt[2] / 255.0f, wgt[3] / 255.0f };
                        float sum = w[0] + w[1] + w[2] + w[3];
                        if (sum > 0.001f) { w[0] /= sum; w[1] /= sum; w[2] /= sum; w[3] /= sum; }
                        else { w[0] = 1.0f; w[1] = 0.0f; w[2] = 0.0f; w[3] = 0.0f; }

                        for (int k = 0; k < 4; k++) {
                            uint16_t lID = 0, pID = ind[k] / 3;
                            if (blk < prim.AnimatedBlocks.size() && pID < prim.AnimatedBlocks[blk].Groups.size()) lID = prim.AnimatedBlocks[blk].Groups[pID];
                            if (lID >= mesh.BoneCount) lID = 0;
                            jointData.push_back(lID); weightData.push_back(w[k]);
                        }
                    }
                    else { for (int k = 0; k < 4; k++) { jointData.push_back(0); weightData.push_back(k == 0 ? 1.0f : 0.0f); } }
                }
            }

            bufferViews.push_back({ bin.Write(posData.data(), posData.size() * 4), (int)posData.size() * 4, 34962 });
            accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)prim.VertexCount, 5126, "VEC3", min, max, true));
            primitivesJson << "\"POSITION\":" << accessors.size() - 1;

            bufferViews.push_back({ bin.Write(normData.data(), normData.size() * 4), (int)normData.size() * 4, 34962 });
            accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)prim.VertexCount, 5126, "VEC3", 0, 0, false));
            primitivesJson << ",\"NORMAL\":" << accessors.size() - 1;

            bufferViews.push_back({ bin.Write(uvData.data(), uvData.size() * 4), (int)uvData.size() * 4, 34962 });
            accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)prim.VertexCount, 5126, "VEC2", 0, 0, false));
            primitivesJson << ",\"TEXCOORD_0\":" << accessors.size() - 1;

            if (hasBones) {
                bufferViews.push_back({ bin.Write(jointData.data(), jointData.size() * 2), (int)jointData.size() * 2, 34962 });
                accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)prim.VertexCount, 5123, "VEC4", 0, 0, false));
                primitivesJson << ",\"JOINTS_0\":" << accessors.size() - 1;
                bufferViews.push_back({ bin.Write(weightData.data(), weightData.size() * 4), (int)weightData.size() * 4, 34962 });
                accessors.push_back(Accessor((int)bufferViews.size() - 1, (int)prim.VertexCount, 5126, "VEC4", 0, 0, false));
                primitivesJson << ",\"WEIGHTS_0\":" << accessors.size() - 1;
            }
            primitivesJson << "},\"indices\":" << idxAcc << ",\"material\":" << prim.MaterialIndex;
            if (!prim.ClothPrimitives.empty()) {
                primitivesJson << ",\"extras\":{\"cloth\":[";
                for (size_t k = 0; k < prim.ClothPrimitives.size(); k++) primitivesJson << (k > 0 ? "," : "") << "{\"matIdx\":" << prim.ClothPrimitives[k].MaterialIndex << ",\"data\":\"" << ToHex(prim.ClothPrimitives[k].ParticleProgramData) << "\"}";
                primitivesJson << "]}";
            }
            primitivesJson << "}";
        }

        int ibmAcc = -1;
        if (mesh.BoneCount > 0) {
            std::vector<float> ibm;
            for (int i = 0; i < mesh.BoneCount; i++) { Mat4 t; Transpose(IBMs[i], t); for (int k = 0; k < 16; k++) ibm.push_back(t.m[k]); }
            bufferViews.push_back({ bin.Write(ibm.data(), ibm.size() * 4), (int)ibm.size() * 4, 0 });
            accessors.push_back(Accessor((int)bufferViews.size() - 1, mesh.BoneCount, 5126, "MAT4", 0, 0, false));
            ibmAcc = (int)accessors.size() - 1;
        }

        // --- ANIMATION SAMPLING (ENHANCED FOR PARTIAL & DELTA) ---
        std::stringstream animationJson;
        bool hasAnimation = false;

        if (anim && anim->Data.IsParsed) {
            float duration = anim->Data.Duration;
            if (duration < 0.01f) {
                float maxTrackDuration = 0.0f;
                for (const auto& t : anim->Data.Tracks) {
                    if (t.SamplesPerSecond > 0 && t.FrameCount > 0) {
                        float d = (float)t.FrameCount / t.SamplesPerSecond;
                        if (d > maxTrackDuration) maxTrackDuration = d;
                    }
                }
                if (maxTrackDuration > 0.0f) duration = maxTrackDuration;
            }

            int frames = (int)(duration * 30.0f);
            if (frames < 2) frames = 2;

            std::vector<float> timeData;
            for (int i = 0; i < frames; i++) timeData.push_back(i / 30.0f);

            bufferViews.push_back({ bin.Write(timeData.data(), timeData.size() * 4), (int)timeData.size() * 4, 0 });
            float timeMin[3] = { timeData.front(), 0, 0 }; float timeMax[3] = { timeData.back(), 0, 0 };
            accessors.push_back(Accessor((int)bufferViews.size() - 1, frames, 5126, "SCALAR", timeMin, timeMax, true));
            int timeAccIdx = (int)accessors.size() - 1;

            animationJson << "{\"name\":" << Esc(anim->Data.ObjectName.empty() ? "ExportedAnim" : anim->Data.ObjectName) << ",\"channels\":[";
            std::stringstream samplersJson;
            int samplerCount = 0;
            bool firstChannel = true;

            // PRE-CALCULATE MATHEMATICALLY PERFECT BIND POSE (Same as Renderer)
            std::vector<DirectX::XMMATRIX> dxBindLocal(mesh.BoneCount);
            if (animType == 7) {
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
                    else {
                        dxIBM[i] = DirectX::XMMatrixIdentity();
                        dxBindGlobal[i] = DirectX::XMMatrixIdentity();
                    }
                }
                for (int i = 0; i < mesh.BoneCount; i++) {
                    int p = mesh.Bones[i].ParentIndex;
                    if (p == -1 || p >= mesh.BoneCount) dxBindLocal[i] = dxBindGlobal[i];
                    else dxBindLocal[i] = DirectX::XMMatrixMultiply(dxBindGlobal[i], dxIBM[p]);
                }
            }

            for (int b = 0; b < mesh.BoneCount; b++) {

                const AnimTrack* track = nullptr;
                std::string boneName = (b < mesh.BoneNames.size() ? mesh.BoneNames[b] : "");
                std::transform(boneName.begin(), boneName.end(), boneName.begin(), ::tolower);

                for (const auto& t : anim->Data.Tracks) {
                    std::string tName = t.BoneName;
                    std::transform(tName.begin(), tName.end(), tName.begin(), ::tolower);
                    bool match = false;
                    if (boneName.length() > 0 && tName.length() >= boneName.length()) {
                        if (tName.compare(0, boneName.length(), boneName) == 0) {
                            if (tName.length() == boneName.length()) match = true;
                            else {
                                char next = tName[boneName.length()];
                                if ((next < 'a' || next > 'z') && (next < '0' || next > '9')) match = true;
                            }
                        }
                    }
                    if (match) { track = &t; break; }
                }

                if (!track) continue; // Skip exporting this bone's track if no track data exists

                std::vector<Vec3> transData;
                std::vector<Vec4> rotData;

                for (int f = 0; f < frames; f++) {
                    ::Vec3 p_anim; ::Vec4 r_anim;
                    float time = f / 30.0f;
                    int trackFrame = (int)(time * track->SamplesPerSecond) % (track->FrameCount > 0 ? track->FrameCount : 1);
                    track->EvaluateFrame(trackFrame, p_anim, r_anim);

                    if (animType == 7) {
                        // EXACT RENDERER MATH: Bake the Delta into the Local Bind Pose
                        DirectX::XMVECTOR vRot = DirectX::XMQuaternionConjugate(DirectX::XMVectorSet(r_anim.x, r_anim.y, r_anim.z, r_anim.w));
                        DirectX::XMVECTOR vPos = DirectX::XMVectorSet(p_anim.x, p_anim.y, p_anim.z, 1.0f);

                        DirectX::XMMATRIX trackMat = DirectX::XMMatrixMultiply(
                            DirectX::XMMatrixRotationQuaternion(vRot),
                            DirectX::XMMatrixTranslationFromVector(vPos)
                        );

                        DirectX::XMMATRIX finalLocal = DirectX::XMMatrixMultiply(trackMat, dxBindLocal[b]);

                        // Decompose into absolute glTF-ready transforms
                        DirectX::XMVECTOR s, r, t;
                        DirectX::XMMatrixDecompose(&s, &r, &t, finalLocal);

                        transData.push_back({ DirectX::XMVectorGetX(t), DirectX::XMVectorGetY(t), DirectX::XMVectorGetZ(t) });
                        rotData.push_back({ DirectX::XMVectorGetX(r), DirectX::XMVectorGetY(r), DirectX::XMVectorGetZ(r), DirectX::XMVectorGetW(r) });
                    }
                    else {
                        // Type 6 or 9: Output standard Fable tracks (Conjugated for glTF)
                        transData.push_back({ p_anim.x, p_anim.y, p_anim.z });
                        rotData.push_back({ -r_anim.x, -r_anim.y, -r_anim.z, r_anim.w });
                    }
                }

                bufferViews.push_back({ bin.Write(transData.data(), transData.size() * 12), (int)transData.size() * 12, 0 });
                accessors.push_back(Accessor((int)bufferViews.size() - 1, frames, 5126, "VEC3", 0, 0, false));
                int transAccIdx = (int)accessors.size() - 1;

                bufferViews.push_back({ bin.Write(rotData.data(), rotData.size() * 16), (int)rotData.size() * 16, 0 });
                accessors.push_back(Accessor((int)bufferViews.size() - 1, frames, 5126, "VEC4", 0, 0, false));
                int rotAccIdx = (int)accessors.size() - 1;

                if (samplerCount > 0) samplersJson << ",";
                samplersJson << "{\"input\":" << timeAccIdx << ",\"interpolation\":\"LINEAR\",\"output\":" << transAccIdx << "},";
                samplersJson << "{\"input\":" << timeAccIdx << ",\"interpolation\":\"LINEAR\",\"output\":" << rotAccIdx << "}";

                if (!firstChannel) animationJson << ",";
                animationJson << "{\"sampler\":" << (samplerCount * 2) << ",\"target\":{\"node\":" << b << ",\"path\":\"translation\"}},";
                animationJson << "{\"sampler\":" << (samplerCount * 2 + 1) << ",\"target\":{\"node\":" << b << ",\"path\":\"rotation\"}}";

                samplerCount++;
                firstChannel = false;
            }

            animationJson << "],\"samplers\":[" << samplersJson.str() << "]}";
            hasAnimation = (samplerCount > 0);
        }

        // --- NODE EXPORT (Standard) ---
        std::vector<std::string> nodeStrs;

        for (int i = 0; i < mesh.BoneCount; i++) {
            Mat4 local; int p = mesh.Bones[i].ParentIndex;
            if (p == -1) local = WorldTransforms[i]; else local = Multiply(IBMs[p], WorldTransforms[i]);
            Mat4 t; Transpose(local, t);

            std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":" << Esc(i < mesh.BoneNames.size() ? mesh.BoneNames[i] : "B" + std::to_string(i)) << ",\"matrix\":[";
            for (int k = 0; k < 16; k++) ss << t.m[k] << (k < 15 ? "," : "");
            ss << "]"; if (!nodeChildren[i].empty()) { ss << ",\"children\":["; for (size_t k = 0; k < nodeChildren[i].size(); k++) ss << nodeChildren[i][k] << (k < nodeChildren[i].size() - 1 ? "," : ""); ss << "]"; }
            ss << "}"; nodeStrs.push_back(ss.str());
        }

        if (!isRepeated || mesh.Generators.empty()) {
            std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":" << Esc(mesh.MeshName) << ",\"mesh\":0" << (mesh.BoneCount > 0 ? ",\"skin\":0" : "") << "}";
            nodeStrs.push_back(ss.str());
        }
        else {
            std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":\"" << mesh.MeshName << "_Base\"}";
            nodeStrs.push_back(ss.str());
        }

        for (size_t i = 0; i < mesh.Helpers.size(); i++) {
            const auto& h = mesh.Helpers[i]; std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":" << Esc(i < mesh.HelperNameStrings.size() ? mesh.HelperNameStrings[i] : "HPNT_" + std::to_string(h.NameCRC)) << ",\"translation\":[" << h.Pos[0] << "," << h.Pos[1] << "," << h.Pos[2] << "],\"extras\":{\"type\":\"Helper\",\"crc\":" << h.NameCRC << "}}";
            nodeStrs.push_back(ss.str());
        }

        for (size_t i = 0; i < mesh.Dummies.size(); i++) {
            const auto& d = mesh.Dummies[i]; Mat4 dMat; memcpy(dMat.m, d.Transform, 48); dMat.m[3] = 0; dMat.m[7] = 0; dMat.m[11] = 0; dMat.m[15] = 1; Mat4 t; Transpose(dMat, t);
            std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":" << Esc(i < mesh.DummyNameStrings.size() ? mesh.DummyNameStrings[i] : "HDMY_" + std::to_string(d.NameCRC)) << ",\"matrix\":[";
            for (int k = 0; k < 16; k++) ss << t.m[k] << (k < 15 ? "," : "");
            ss << "],\"extras\":{\"type\":\"Dummy\",\"crc\":" << d.NameCRC << "}}";
            nodeStrs.push_back(ss.str());
        }

        for (size_t i = 0; i < mesh.Generators.size(); i++) {
            Mat4 dMat; memcpy(dMat.m, mesh.Generators[i].Transform, 48); dMat.m[3] = 0; dMat.m[7] = 0; dMat.m[11] = 0; dMat.m[15] = 1; Mat4 t; Transpose(dMat, t);
            std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":\"GEN_" << mesh.Generators[i].ObjectName << "\",\"matrix\":[";
            for (int k = 0; k < 16; k++) ss << t.m[k] << (k < 15 ? "," : "");
            ss << "]";
            if (isRepeated) ss << ",\"mesh\":0";
            ss << ",\"extras\":{\"type\":\"Generator\",\"bankId\":" << mesh.Generators[i].BankIndex << "}}";
            nodeStrs.push_back(ss.str());
        }

        for (size_t i = 0; i < mesh.Volumes.size(); i++) {
            const auto& v = mesh.Volumes[i]; std::stringstream ss; ss.imbue(std::locale("C"));
            ss << "{\"name\":\"VOL_" << v.Name << "\",\"extras\":{\"type\":\"Volume\",\"ID\":" << v.ID << ",\"planes\":[";
            for (size_t p = 0; p < v.Planes.size(); p++) {
                const auto& plane = v.Planes[p];
                if (p > 0) ss << ",";
                ss << "{\"n\":[" << plane.Normal[0] << "," << plane.Normal[1] << "," << plane.Normal[2] << "],\"d\":" << plane.D << "}";
            }
            ss << "]}}"; nodeStrs.push_back(ss.str());
        }

        { std::stringstream ss; ss.imbue(std::locale("C")); ss << "{\"name\":\"Scene_Root\",\"matrix\":[1,0,0,0,0,0,-1,0,0,1,0,0,0,0,0,1],\"children\":["; if (!nodeChildren[rootWrapperIdx].empty()) for (size_t k = 0; k < nodeChildren[rootWrapperIdx].size(); k++) ss << nodeChildren[rootWrapperIdx][k] << (k < nodeChildren[rootWrapperIdx].size() - 1 ? "," : ""); ss << "]}"; nodeStrs.push_back(ss.str()); }

        json << "\"nodes\":["; for (size_t i = 0; i < nodeStrs.size(); i++) json << (i > 0 ? "," : "") << nodeStrs[i]; json << "],";
        json << "\"scene\":0,\"scenes\":[{\"nodes\":[" << rootWrapperIdx << "]}],";

        if (hasAnimation) {
            json << "\"animations\":[" << animationJson.str() << "],";
        }

        json << "\"meshes\":[{\"name\":" << Esc(mesh.MeshName) << ",\"primitives\":[" << primitivesJson.str() << "]}],";
        if (mesh.BoneCount > 0) {
            int rootBoneIdx = -1;
            for (int i = 0; i < mesh.BoneCount; i++) {
                if (mesh.Bones[i].ParentIndex == -1) { rootBoneIdx = i; break; }
            }

            json << "\"skins\":[{\"inverseBindMatrices\":" << ibmAcc;
            if (rootBoneIdx != -1) json << ",\"skeleton\":" << rootBoneIdx;
            json << ",\"joints\":[";
            for (int i = 0; i < mesh.BoneCount; i++) json << i << (i < mesh.BoneCount - 1 ? "," : "");
            json << "]}],";
        }

        json << "\"materials\":[";
        for (size_t i = 0; i < mesh.Materials.size(); i++) {
            const auto& m = mesh.Materials[i];
            json << (i > 0 ? "," : "") << "{ \"name\": " << Esc(m.Name) << ", \"extras\": {";
            json << "\"ID\":" << m.ID << ",\"DecalID\":" << m.DecalID << ",\"DiffuseMapID\":" << m.DiffuseMapID << ",\"BumpMapID\":" << m.BumpMapID << ",\"ReflectionMapID\":" << m.ReflectionMapID << ",\"IlluminationMapID\":" << m.IlluminationMapID << ",\"MapFlags\":" << m.MapFlags << ",\"SelfIllumination\":" << m.SelfIllumination << ",\"IsTwoSided\":" << (m.IsTwoSided ? "true" : "false") << ",\"IsTransparent\":" << (m.IsTransparent ? "true" : "false") << ",\"BooleanAlpha\":" << (m.BooleanAlpha ? "true" : "false") << ",\"DegenerateTriangles\":" << (m.DegenerateTriangles ? "true" : "false") << ",\"UseFilenames\":" << (m.UseFilenames ? "true" : "false") << "}}";
        }
        json << "],";

        json << "\"accessors\":[";
        for (size_t i = 0; i < accessors.size(); i++) {
            json << (i > 0 ? "," : "") << "{\"bufferView\":" << accessors[i].view << ",\"componentType\":" << accessors[i].compType << ",\"count\":" << accessors[i].count << ",\"type\":\"" << accessors[i].type << "\"";
            if (accessors[i].hasBounds) json << ",\"min\":[" << accessors[i].min[0] << "," << accessors[i].min[1] << "," << accessors[i].min[2] << "],\"max\":[" << accessors[i].max[0] << "," << accessors[i].max[1] << "," << accessors[i].max[2] << "]";
            json << "}";
        }
        json << "],\"bufferViews\":[";
        for (size_t i = 0; i < bufferViews.size(); i++) {
            json << (i > 0 ? "," : "") << "{\"buffer\":0,\"byteOffset\":" << bufferViews[i].offset << ",\"byteLength\":" << bufferViews[i].length << (bufferViews[i].target ? ",\"target\":" : "") << (bufferViews[i].target ? std::to_string(bufferViews[i].target) : "") << "}";
        }
        json << "],\"buffers\":[{\"uri\":\"" << binUri << "\",\"byteLength\":" << bin.data.size() << "}]}";
        json.close();

        std::ofstream bOut(binFilename, std::ios::binary); bOut.write((char*)bin.data.data(), bin.data.size()); bOut.close();
        return true;
    }

    static bool ExportBBM(const CBBMParser& bbm, const std::string& filename) {
        if (!bbm.IsParsed) return false;

        C3DMeshContent tempMesh;
        tempMesh.MeshName = "Physics_Mesh_Export";
        tempMesh.DebugStatus = "Converted from BBM";

        if (!bbm.ParsedMaterials.empty()) {
            for (size_t i = 0; i < bbm.ParsedMaterials.size(); i++) {
                const auto& src = bbm.ParsedMaterials[i];
                C3DMaterial mat = {};
                mat.ID = (int32_t)i; mat.Name = src.Name; mat.IsTwoSided = src.TwoSided; mat.IsTransparent = src.Transparent;
                mat.BooleanAlpha = src.BooleanAlpha; mat.DegenerateTriangles = src.DegenerateTriangles; mat.SelfIllumination = (int32_t)src.SelfIllumination;
                mat.DiffuseMapID = src.DiffuseBank; mat.BumpMapID = src.BumpBank; mat.ReflectionMapID = src.ReflectBank; mat.IlluminationMapID = src.IllumBank;
                tempMesh.Materials.push_back(mat);
            }
            tempMesh.MaterialCount = (int32_t)tempMesh.Materials.size();
        }
        else {
            C3DMaterial mat = {}; mat.ID = 0; mat.Name = "DefaultPhysicsMat"; tempMesh.Materials.push_back(mat); tempMesh.MaterialCount = 1;
        }

        if (!bbm.Bones.empty()) {
            tempMesh.BoneCount = (int32_t)bbm.Bones.size(); tempMesh.Bones.resize(tempMesh.BoneCount); tempMesh.BoneIndices.resize(tempMesh.BoneCount); tempMesh.BoneTransformsRaw.resize(tempMesh.BoneCount * 64);
            for (size_t i = 0; i < bbm.Bones.size(); i++) {
                const auto& src = bbm.Bones[i]; C3DBone& dst = tempMesh.Bones[i];
                dst.NameCRC = 0; dst.ParentIndex = src.ParentIndex; dst.OriginalNoChildren = 0;
                memcpy(dst.LocalizationMatrix, src.LocalTransform, 48);
                tempMesh.BoneNames.push_back(src.Name); tempMesh.BoneIndices[i] = (uint16_t)src.Index;
                float* dstMat = (float*)(tempMesh.BoneTransformsRaw.data() + (i * 64));
                dstMat[0] = src.LocalTransform[0]; dstMat[1] = src.LocalTransform[1]; dstMat[2] = src.LocalTransform[2]; dstMat[3] = 0.0f;
                dstMat[4] = src.LocalTransform[3]; dstMat[5] = src.LocalTransform[4]; dstMat[6] = src.LocalTransform[5]; dstMat[7] = 0.0f;
                dstMat[8] = src.LocalTransform[6]; dstMat[9] = src.LocalTransform[7]; dstMat[10] = src.LocalTransform[8]; dstMat[11] = 0.0f;
                dstMat[12] = src.LocalTransform[9]; dstMat[13] = src.LocalTransform[10]; dstMat[14] = src.LocalTransform[11]; dstMat[15] = 1.0f;
            }
        }

        if (!bbm.ParsedVertices.empty()) {
            C3DPrimitive prim = {};
            if (bbm.PhysicsMaterialIndex >= 0 && bbm.PhysicsMaterialIndex < tempMesh.Materials.size()) prim.MaterialIndex = bbm.PhysicsMaterialIndex; else prim.MaterialIndex = 0;
            prim.VertexCount = (uint32_t)bbm.ParsedVertices.size(); prim.TriangleCount = (uint32_t)bbm.ParsedIndices.size() / 3; prim.IndexCount = (uint32_t)bbm.ParsedIndices.size();
            prim.VertexStride = 32; prim.IsCompressed = false; prim.VertexBuffer.resize(prim.VertexCount * prim.VertexStride);
            uint8_t* dst = prim.VertexBuffer.data();
            for (const auto& v : bbm.ParsedVertices) {
                memcpy(dst, &v.Position, 12); memcpy(dst + 12, &v.Normal, 12); memcpy(dst + 24, &v.UV, 8); dst += 32;
            }
            prim.IndexBuffer = bbm.ParsedIndices; tempMesh.Primitives.push_back(prim); tempMesh.PrimitiveCount++;
        }

        for (const auto& h : bbm.Helpers) {
            CHelperPoint hp = {}; hp.NameCRC = 0; hp.BoneIndex = h.BoneIndex; hp.Pos[0] = h.Position.x; hp.Pos[1] = h.Position.y; hp.Pos[2] = h.Position.z;
            tempMesh.Helpers.push_back(hp); std::string exportName = h.Name; if (h.SubMeshIndex != -1) exportName += "_Sub" + std::to_string(h.SubMeshIndex); tempMesh.HelperNameStrings.push_back(exportName);
        }
        tempMesh.HelperPointCount = (uint16_t)tempMesh.Helpers.size();

        for (const auto& d : bbm.Dummies) {
            CDummyObject dum = {}; dum.NameCRC = 0; dum.BoneIndex = d.BoneIndex; memcpy(dum.Transform, d.Transform, 48); tempMesh.Dummies.push_back(dum);
            std::string exportName = d.Name; if (d.UseLocalOrigin) exportName += "_LOC"; tempMesh.DummyNameStrings.push_back(exportName);
        }
        tempMesh.DummyObjectCount = (uint16_t)tempMesh.Dummies.size();

        for (const auto& v : bbm.Volumes) {
            CMeshVolume vol = {}; vol.ID = v.ID; vol.Name = v.Name;
            for (const auto& p : v.Planes) { CPlane plane; plane.Normal[0] = p.Normal[0]; plane.Normal[1] = p.Normal[1]; plane.Normal[2] = p.Normal[2]; plane.D = p.D; vol.Planes.push_back(plane); }
            tempMesh.Volumes.push_back(vol);
        }
        tempMesh.MeshVolumeCount = (uint16_t)tempMesh.Volumes.size();

        tempMesh.AutoCalculateBounds();

        return Export(tempMesh, filename, nullptr, 6);
    }
};