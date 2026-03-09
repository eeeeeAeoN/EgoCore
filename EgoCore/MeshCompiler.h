#pragma once
#include "MeshParser.h"
#include "minilzo.h"
#include <vector>
#include <string>
#include <cstring>

class MeshCompiler {
public:
    // Compiles a single LOD (the currently active C3DMeshContent)
    static std::vector<uint8_t> CompileSingleLOD(const C3DMeshContent& m) {
        std::vector<uint8_t> data;
        auto Write = [&](const void* val, size_t len) {
            size_t s = data.size(); data.resize(s + len); memcpy(data.data() + s, val, len);
            };
        auto WriteString = [&](const std::string& str) {
            Write(str.c_str(), str.length() + 1);
            };

        // 1. Write Uncompressed Structural Headers
        WriteString(m.MeshName);
        Write(&m.AnimatedFlag, 1);
        Write(m.BoundingSphereCenter, 12);
        Write(&m.BoundingSphereRadius, 4);
        Write(m.BoundingBoxMin, 12);
        Write(m.BoundingBoxMax, 12);
        Write(&m.HelperPointCount, 2);
        Write(&m.DummyObjectCount, 2);

        uint16_t calcPackedNamesSize = (uint16_t)m.PackedNamesRaw.size();
        Write(&calcPackedNamesSize, 2);

        Write(&m.MeshVolumeCount, 2);
        Write(&m.MeshGeneratorCount, 2);

        for (const auto& h : m.Helpers) { Write(&h.NameCRC, 4); Write(h.Pos, 12); Write(&h.BoneIndex, 4); }
        for (const auto& d : m.Dummies) { Write(&d.NameCRC, 4); Write(d.Transform, 48); Write(&d.BoneIndex, 4); }

        if (!m.PackedNamesRaw.empty()) Write(m.PackedNamesRaw.data(), m.PackedNamesRaw.size());

        for (const auto& vol : m.Volumes) {
            Write(&vol.ID, 4); WriteString(vol.Name);
            uint32_t pCount = (uint32_t)vol.Planes.size(); Write(&pCount, 4);
            if (pCount > 0) Write(vol.Planes.data(), pCount * 16);
        }

        for (const auto& gen : m.Generators) {
            Write(gen.Transform, 48); Write(&gen.BoneIndex, 4); WriteString(gen.ObjectName); Write(&gen.BankIndex, 4); Write(&gen.UseLocalOrigin, 1);
        }

        Write(&m.MaterialCount, 4); Write(&m.PrimitiveCount, 4); Write(&m.BoneCount, 4);

        std::vector<uint8_t> bNamesBlock;
        for (const auto& s : m.BoneNames) bNamesBlock.insert(bNamesBlock.end(), s.c_str(), s.c_str() + s.length() + 1);
        uint32_t calcBoneNameSize = (uint32_t)bNamesBlock.size();
        Write(&calcBoneNameSize, 4);

        Write(&m.ClothFlag, 1); Write(&m.TotalStaticBlocks, 2); Write(&m.TotalAnimatedBlocks, 2);

        if (m.BoneCount > 0) {
            Write(m.BoneIndices.data(), 2 * m.BoneCount);
            if (!bNamesBlock.empty()) Write(bNamesBlock.data(), bNamesBlock.size());
            if (!m.Bones.empty()) Write(m.Bones.data(), 60 * m.BoneCount);
            if (!m.BoneKeyframesRaw.empty()) Write(m.BoneKeyframesRaw.data(), 48 * m.BoneCount);
            if (!m.BoneTransformsRaw.empty()) Write(m.BoneTransformsRaw.data(), 64 * m.BoneCount);
        }

        Write(m.RootMatrix, 48);

        for (const auto& mat : m.Materials) {
            Write(&mat.ID, 4); WriteString(mat.Name); Write(&mat.DecalID, 4); Write(&mat.DiffuseMapID, 4); Write(&mat.BumpMapID, 4);
            Write(&mat.ReflectionMapID, 4); Write(&mat.IlluminationMapID, 4); Write(&mat.MapFlags, 4); Write(&mat.SelfIllumination, 4);
            Write(&mat.IsTwoSided, 1); Write(&mat.IsTransparent, 1); Write(&mat.BooleanAlpha, 1); Write(&mat.DegenerateTriangles, 1);
            Write(&mat.UseFilenames, 1);
            if (mat.UseFilenames) {
                for (int j = 0; j < 4; j++) {
                    if (j < mat.TextureFileNames.size()) WriteString(mat.TextureFileNames[j]);
                    else WriteString("");
                }
            }
        }

        for (const auto& prim : m.Primitives) {
            Write(&prim.MaterialIndex, 4); Write(&prim.RepeatingMeshReps, 4); Write(prim.SphereCenter, 12); Write(&prim.SphereRadius, 4);
            Write(&prim.AvgTextureStretch, 4); Write(&prim.StaticBlockCount, 4); Write(&prim.AnimatedBlockCount, 4); Write(&prim.VertexCount, 4);
            Write(&prim.TriangleCount, 4); Write(&prim.IndexCount, 4); Write(&prim.InitFlags, 4); Write(&prim.StaticBlockCount_2, 4); Write(&prim.AnimatedBlockCount_2, 4);

            for (const auto& sb : prim.StaticBlocks) { Write(&sb.PrimitiveCount, 4); Write(&sb.StartIndex, 4); Write(&sb.IsStrip, 1); Write(&sb.ChangeFlags, 1); Write(&sb.DegenerateTriangles, 1); Write(&sb.MaterialIndex, 4); }
            for (const auto& ab : prim.AnimatedBlocks) {
                Write(&ab.PrimitiveCount, 4); Write(&ab.StartIndex, 4); Write(&ab.IsStrip, 1); Write(&ab.ChangeFlags, 1); Write(&ab.DegenerateTriangles, 1);
                Write(&ab.VertexCount, 4); Write(&ab.BonesPerVertex, 2); Write(&ab.PalettedFlag, 1);
                uint8_t gCount = (uint8_t)ab.Groups.size(); Write(&gCount, 1);
                if (gCount > 0) Write(ab.Groups.data(), gCount);
            }

            Write(&prim.Compression, 32);

            uint32_t stride = prim.VertexStride; Write(&stride, 4);
            uint32_t type = prim.BufferType; Write(&type, 4);

            // 2. COMPRESS BUFFERS (LZO)
            WriteLZO(data, prim.VertexBuffer);

            std::vector<uint8_t> idxBytes(prim.IndexBuffer.size() * 2);
            if (!prim.IndexBuffer.empty()) memcpy(idxBytes.data(), prim.IndexBuffer.data(), idxBytes.size());
            WriteLZO(data, idxBytes);

            uint32_t cpCount = (uint32_t)prim.ClothPrimitives.size(); Write(&cpCount, 4);
            for (const auto& cp : prim.ClothPrimitives) {
                Write(&cp.PrimitiveIndex, 4); Write(&cp.MaterialIndex, 4);
                WriteLZO(data, cp.ParticleProgramData);
            }
        }
        return data;
    }

private:
    static void WriteLZO(std::vector<uint8_t>& out, const std::vector<uint8_t>& uncomp) {
        if (uncomp.empty()) {
            uint16_t zero = 0;
            out.insert(out.end(), (uint8_t*)&zero, ((uint8_t*)&zero) + 2);
            return;
        }
        if (lzo_init() != LZO_E_OK) return;

        std::vector<uint8_t> compBuf(uncomp.size() + (uncomp.size() / 16) + 64 + 3);
        lzo_uint compLen = 0;
        std::vector<uint8_t> wrkmem(LZO1X_1_MEM_COMPRESS);

        if (lzo1x_1_compress(uncomp.data(), uncomp.size(), compBuf.data(), &compLen, wrkmem.data()) == LZO_E_OK) {
            uint32_t len32 = (uint32_t)compLen;
            out.insert(out.end(), (uint8_t*)&len32, ((uint8_t*)&len32) + 4);
            out.insert(out.end(), compBuf.begin(), compBuf.begin() + compLen);
        }
    }
};