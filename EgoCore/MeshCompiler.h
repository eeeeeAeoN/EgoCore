#pragma once
#include "MeshParser.h"
#include "minilzo.h"
#include <vector>
#include <string>
#include <cstring>

class MeshCompiler {
public:
    static std::vector<uint8_t> Compile(const C3DMeshContent& mesh) {
        std::vector<uint8_t> data;

        auto Write = [&](const void* val, size_t len) {
            size_t s = data.size();
            data.resize(s + len);
            memcpy(data.data() + s, val, len);
            };

        auto WriteString = [&](const std::string& str) {
            Write(str.c_str(), str.length() + 1);
            };

        // Fable's block LZO format: [uint32_t CompressedSize] [LZO Payload]
        auto WriteCompressed = [&](const void* val, size_t len) {
            if (len == 0) return;
            std::vector<uint8_t> compBuf(len + (len / 16) + 128);
            lzo_uint compLen = 0;
            std::vector<uint8_t> wrkmem(LZO1X_1_MEM_COMPRESS);

            if (lzo1x_1_compress((const unsigned char*)val, len, compBuf.data(), &compLen, wrkmem.data()) == LZO_E_OK) {
                uint32_t cLen = (uint32_t)compLen;
                Write(&cLen, 4);
                Write(compBuf.data(), compLen);
            }
            };

        // 1. Mesh Name
        WriteString(mesh.MeshName);

        // 2. Base Properties
        uint8_t animFlag = mesh.TotalAnimatedBlocks > 0 ? 1 : 0;
        Write(&animFlag, 1);
        Write(mesh.BoundingSphereCenter, 12);
        Write(&mesh.BoundingSphereRadius, 4);
        Write(mesh.BoundingBoxMin, 12);
        Write(mesh.BoundingBoxMax, 12);

        // 3. Counts
        Write(&mesh.HelperPointCount, 2);
        Write(&mesh.DummyObjectCount, 2);
        Write(&mesh.PackedNamesSize, 2);
        Write(&mesh.MeshVolumeCount, 2);
        Write(&mesh.MeshGeneratorCount, 2);

        uint32_t matCount = (uint32_t)mesh.Materials.size(); Write(&matCount, 4);
        uint32_t primCount = (uint32_t)mesh.Primitives.size(); Write(&primCount, 4);
        Write(&mesh.BoneCount, 4);

        uint32_t calcBoneSize = 0; Write(&calcBoneSize, 4);

        uint8_t clothFlag = mesh.ClothFlag ? 1 : 0; Write(&clothFlag, 1);
        Write(&mesh.TotalStaticBlocks, 2);
        Write(&mesh.TotalAnimatedBlocks, 2);

        // 4. Root Matrix
        Write(mesh.RootMatrix, 48);

        // 5. Materials
        for (const auto& mat : mesh.Materials) {
            Write(&mat.ID, 4);
            WriteString(mat.Name);
            uint8_t ts = mat.IsTwoSided ? 1 : 0; Write(&ts, 1);
            uint8_t uf = mat.UseFilenames ? 1 : 0; Write(&uf, 1);
            uint8_t tr = mat.IsTransparent ? 1 : 0; Write(&tr, 1);
            Write(&mat.MapFlags, 4);

            if (mat.MapFlags & 1) Write(&mat.DiffuseMapID, 4);
            if (mat.MapFlags & 2) Write(&mat.BumpMapID, 4);
            if (mat.MapFlags & 4) Write(&mat.ReflectionMapID, 4);
            if (mat.MapFlags & 8) Write(&mat.IlluminationMapID, 4);

            uint32_t padding = 0;
            Write(&padding, 4);
        }

        // 6. Primitives
        for (const auto& prim : mesh.Primitives) {
            Write(&prim.SphereCenter, 12); Write(&prim.SphereRadius, 4);
            Write(&prim.AvgTextureStretch, 4); Write(&prim.VertexCount, 4); Write(&prim.TriangleCount, 4);
            Write(&prim.StaticBlockCount, 2); Write(&prim.StaticBlockCount_2, 2);

            for (const auto& sb : prim.StaticBlocks) {
                Write(&sb.PrimitiveCount, 4); Write(&sb.StartIndex, 4);
                uint8_t strip = sb.IsStrip ? 1 : 0; Write(&strip, 1);
                Write(&sb.ChangeFlags, 1);
                uint8_t degen = sb.DegenerateTriangles ? 1 : 0; Write(&degen, 1);
                Write(&sb.MaterialIndex, 4);
            }

            for (const auto& ab : prim.AnimatedBlocks) {
                Write(&ab.PrimitiveCount, 4); Write(&ab.StartIndex, 4);
                uint8_t strip = ab.IsStrip ? 1 : 0; Write(&strip, 1);
                Write(&ab.ChangeFlags, 1);
                uint8_t degen = ab.DegenerateTriangles ? 1 : 0; Write(&degen, 1);
                Write(&ab.VertexCount, 4); Write(&ab.BonesPerVertex, 2);
                uint8_t paletted = ab.PalettedFlag ? 1 : 0; Write(&paletted, 1);
                uint8_t gCount = (uint8_t)ab.Groups.size(); Write(&gCount, 1);
                if (gCount > 0) Write(ab.Groups.data(), gCount);
            }

            Write(&prim.Compression, 32);

            uint32_t stride = prim.VertexStride; Write(&stride, 4);
            uint32_t type = prim.BufferType; Write(&type, 4);

            // Vertex Buffer (Type 2 uses standard LZO compression)
            if (!prim.VertexBuffer.empty()) {
                WriteCompressed(prim.VertexBuffer.data(), prim.VertexBuffer.size());
            }
            else {
                uint16_t h = 0; Write(&h, 2);
            }

            // Index Buffer (Type 2 Grass uses UNCOMPRESSED indices!)
            if (!prim.IndexBuffer.empty()) {
                if (mesh.MeshType == 2) {
                    Write(prim.IndexBuffer.data(), prim.IndexBuffer.size() * 2);
                }
                else {
                    WriteCompressed(prim.IndexBuffer.data(), prim.IndexBuffer.size() * 2);
                }
            }
            else {
                uint16_t h = 0; Write(&h, 2);
            }

            uint32_t cpCount = (uint32_t)prim.ClothPrimitives.size(); Write(&cpCount, 4);
            for (const auto& cp : prim.ClothPrimitives) {
                Write(&cp.PrimitiveIndex, 4); Write(&cp.MaterialIndex, 4);
            }
        }

        // --- INJECT BLANK LOD FOR FABLE TYPE 2 ---
        if (mesh.MeshType == 2) {
            WriteString(mesh.MeshName);

            uint8_t animFlag = 0; Write(&animFlag, 1);

            // Fable's blank LOD MUST copy the exact bounding box, or the physics tree crashes
            Write(mesh.BoundingSphereCenter, 12);
            Write(&mesh.BoundingSphereRadius, 4);
            Write(mesh.BoundingBoxMin, 12);
            Write(mesh.BoundingBoxMax, 12);

            uint16_t zero16 = 0;
            Write(&zero16, 2); // HelperPointCount
            Write(&zero16, 2); // DummyObjectCount
            Write(&zero16, 2); // PackedNamesSize
            Write(&zero16, 2); // MeshVolumeCount
            Write(&zero16, 2); // MeshGeneratorCount

            uint32_t zero32 = 0;
            Write(&zero32, 4); // MaterialCount
            Write(&zero32, 4); // PrimitiveCount
            Write(&zero32, 4); // BoneCount
            Write(&zero32, 4); // CalcBoneNameSize

            uint8_t clothFlag = 0; Write(&clothFlag, 1);
            Write(&zero16, 2); // TotalStaticBlocks
            Write(&zero16, 2); // TotalAnimatedBlocks

            // Fable Root Matrix
            float rootMatrix[12] = { 0 };
            rootMatrix[0] = 1.0f; // X.x
            rootMatrix[4] = 1.0f; // Y.y
            rootMatrix[8] = 1.0f; // Z.z
            Write(rootMatrix, 48);
        }

        return data;
    }
};