// --- START OF FILE MeshCompiler.h ---

#pragma once
#include "MeshParser.h"
#include "Utils.h" // Ensures CompressFableBlock is available
#include <vector>
#include <string>
#include <cstring>

class MeshCompiler {
public:
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

        uint16_t namesSize = (uint16_t)m.PackedNamesRaw.size();
        Write(&namesSize, 2);
        Write(&m.MeshVolumeCount, 2);
        Write(&m.MeshGeneratorCount, 2);

        // Compress Helper Arrays
        if (m.HelperPointCount > 0) {
            std::vector<uint8_t> hRaw(m.Helpers.size() * 20);
            for (size_t i = 0; i < m.Helpers.size(); i++) {
                memcpy(hRaw.data() + i * 20, &m.Helpers[i].NameCRC, 4);
                memcpy(hRaw.data() + i * 20 + 4, m.Helpers[i].Pos, 12);
                memcpy(hRaw.data() + i * 20 + 16, &m.Helpers[i].BoneIndex, 4);
            }
            WriteLZOBlock(data, hRaw);
        }

        // Compress Dummy Arrays
        if (m.DummyObjectCount > 0) {
            std::vector<uint8_t> dRaw(m.Dummies.size() * 56);
            for (size_t i = 0; i < m.Dummies.size(); i++) {
                memcpy(dRaw.data() + i * 56, &m.Dummies[i].NameCRC, 4);
                memcpy(dRaw.data() + i * 56 + 4, m.Dummies[i].Transform, 48);
                memcpy(dRaw.data() + i * 56 + 52, &m.Dummies[i].BoneIndex, 4);
            }
            WriteLZOBlock(data, dRaw);
        }

        // Compress Packed Strings
        if (namesSize > 0) {
            WriteLZOBlock(data, m.PackedNamesRaw);
        }

        // Compress Volumes
        for (const auto& vol : m.Volumes) {
            Write(&vol.ID, 4); WriteString(vol.Name);
            uint32_t pCount = (uint32_t)vol.Planes.size(); Write(&pCount, 4);
            if (pCount > 0) {
                std::vector<uint8_t> pRaw(pCount * 16);
                memcpy(pRaw.data(), vol.Planes.data(), pCount * 16);
                WriteLZOBlock(data, pRaw);
            }
        }

        // Uncompressed Generators
        for (const auto& gen : m.Generators) {
            Write(gen.Transform, 48); Write(&gen.BoneIndex, 4); WriteString(gen.ObjectName); Write(&gen.BankIndex, 4); Write(&gen.UseLocalOrigin, 1);
        }

        Write(&m.MaterialCount, 4); Write(&m.PrimitiveCount, 4); Write(&m.BoneCount, 4);

        // Compress Bone Names
        std::vector<uint8_t> bNamesBlock;
        for (const auto& s : m.BoneNames) bNamesBlock.insert(bNamesBlock.end(), s.c_str(), s.c_str() + s.length() + 1);
        uint32_t calcBoneNameSize = (uint32_t)bNamesBlock.size();
        Write(&calcBoneNameSize, 4);

        Write(&m.ClothFlag, 1); Write(&m.TotalStaticBlocks, 2); Write(&m.TotalAnimatedBlocks, 2);

        // Compress Bone Data
        if (m.BoneCount > 0) {
            Write(m.BoneIndices.data(), 2 * m.BoneCount);
            WriteLZOBlock(data, bNamesBlock);

            std::vector<uint8_t> bRaw(m.Bones.size() * 60);
            memcpy(bRaw.data(), m.Bones.data(), m.Bones.size() * 60);
            WriteLZOBlock(data, bRaw);

            WriteLZOBlock(data, m.BoneKeyframesRaw);
            WriteLZOBlock(data, m.BoneTransformsRaw);
        }

        Write(m.RootMatrix, 48);

        // Uncompressed Materials
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

            // 2. COMPRESS PRIMITIVE BUFFERS USING FABLE'S CUSTOM WRAPPER
            WriteLZOBlock(data, prim.VertexBuffer);

            std::vector<uint8_t> idxBytes(prim.IndexBuffer.size() * 2);
            if (!prim.IndexBuffer.empty()) memcpy(idxBytes.data(), prim.IndexBuffer.data(), idxBytes.size());
            WriteLZOBlock(data, idxBytes);

            uint32_t cpCount = (uint32_t)prim.ClothPrimitives.size(); Write(&cpCount, 4);
            for (const auto& cp : prim.ClothPrimitives) {
                Write(&cp.PrimitiveIndex, 4); Write(&cp.MaterialIndex, 4);
                WriteLZOBlock(data, cp.ParticleProgramData);
            }
        }
        return data;
    }


    static std::vector<uint8_t> CompilePhysics(const CBBMParser& bbm) {
        struct ChunkWriter {
            std::vector<uint8_t> buffer;
            std::vector<size_t> stack;

            void PushChunk(const std::string& id) {
                Write(id.c_str(), 4);
                uint32_t placeholder = 0;
                stack.push_back(buffer.size());
                Write(&placeholder, 4);
            }

            void PopChunk() {
                if (stack.empty()) return;
                size_t sizeOffset = stack.back();
                stack.pop_back();
                uint32_t chunkSize = (uint32_t)(buffer.size() - sizeOffset - 4);
                memcpy(buffer.data() + sizeOffset, &chunkSize, 4);
            }

            void Write(const void* data, size_t size) {
                if (size == 0) return;
                size_t old = buffer.size();
                buffer.resize(old + size);
                memcpy(buffer.data() + old, data, size);
            }

            void WriteString(const std::string& str) {
                Write(str.c_str(), str.length() + 1);
            }

            void Align() {
                while (buffer.size() % 4 != 0) {
                    uint8_t zero = 0;
                    Write(&zero, 1);
                }
            }
        };

        ChunkWriter w;

        w.PushChunk("3DMF");
        uint32_t version = 2; w.Write(&version, 4);
        w.WriteString(bbm.FileComment.empty() ? "Generated by EgoCore" : bbm.FileComment);
        w.Align();

        w.PushChunk("3DRT");

        // --- MATERIALS ---
        w.PushChunk("MTLS");
        if (bbm.ParsedMaterials.empty()) {
            w.PushChunk("MTRL");
            w.WriteString("Default_Physics_Material");
            uint32_t idx = 0; w.Write(&idx, 4);
            uint8_t twoSided = 1; w.Write(&twoSided, 1);
            uint32_t colors[6] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0 };
            w.Write(colors, 24);
            w.PopChunk();
        }
        else {
            for (const auto& m : bbm.ParsedMaterials) {
                w.PushChunk(m.IsEngineMaterial ? "EMTL" : "MTRL");
                if (m.IsEngineMaterial) {
                    uint32_t pad[2] = { 0, 0 }; w.Write(pad, 8);
                    w.WriteString(m.Name);
                    uint32_t banks[5] = { 0, (uint32_t)m.DiffuseBank, (uint32_t)m.BumpBank, (uint32_t)m.ReflectBank, (uint32_t)m.IllumBank };
                    w.Write(banks, 20);
                    uint8_t hasTex = (!m.TextureDiffuse.empty() || !m.TextureBump.empty()) ? 1 : 0;
                    w.Write(&hasTex, 1);
                    if (hasTex) {
                        w.WriteString(m.TextureDiffuse); w.WriteString("");
                        w.WriteString(m.TextureBump); w.WriteString("");
                    }
                }
                else {
                    w.WriteString(m.Name);
                    w.Write(&m.Index, 4);
                    uint8_t twoSided = m.TwoSided ? 1 : 0; w.Write(&twoSided, 1);
                    uint32_t block[6] = { m.Ambient, m.Diffuse, m.Specular, *(uint32_t*)&m.Shiny, *(uint32_t*)&m.ShinyStrength, *(uint32_t*)&m.Transparency };
                    w.Write(block, 24);
                }
                w.PopChunk();
            }
        }
        w.PopChunk(); // MTLS

        // --- SUBMESH & PRIMITIVES ---
        w.PushChunk("SUBM");
        w.WriteString("Physics_Hull");

        w.PushChunk("PRIM");
        int32_t physMatIdx = bbm.PhysicsMaterialIndex >= 0 ? bbm.PhysicsMaterialIndex : 0;
        w.Write(&physMatIdx, 4);

        if (!bbm.ParsedIndices.empty()) {
            w.PushChunk("TRIS");
            uint32_t triCount = (uint32_t)bbm.ParsedIndices.size() / 3;
            w.Write(&triCount, 4);
            w.Write(bbm.ParsedIndices.data(), bbm.ParsedIndices.size() * 2);
            w.PopChunk();
        }

        if (!bbm.ParsedVertices.empty()) {
            w.PushChunk("VERT");
            uint32_t vCount = (uint32_t)bbm.ParsedVertices.size();
            w.Write(&vCount, 4);
            for (const auto& v : bbm.ParsedVertices) {
                w.Write(&v.Position, 12);
                w.Write(&v.Normal, 12);
                w.Write(&v.UV, 8);
            }
            w.PopChunk();
        }
        w.PopChunk(); // PRIM
        w.PopChunk(); // SUBM

        // --- HELPERS / DUMMIES / VOLUMES ---
        if (!bbm.Helpers.empty() || !bbm.Dummies.empty() || !bbm.Volumes.empty()) {
            w.PushChunk("HLPR");
            for (const auto& h : bbm.Helpers) {
                w.PushChunk("HPNT");
                w.Write(&h.Position, 12);
                w.Write(&h.SubMeshIndex, 4);
                w.Write(&h.BoneIndex, 4);
                w.WriteString(h.Name);
                w.PopChunk();
            }
            for (const auto& d : bbm.Dummies) {
                w.PushChunk("HDMY");
                uint32_t ver = 3; w.Write(&ver, 4);
                w.Write(&d.Direction, 12);
                w.Write(d.Transform, 48);
                uint8_t local = d.UseLocalOrigin ? 1 : 0; w.Write(&local, 1);
                w.Write(&d.Position, 12);
                w.Write(&d.SubMeshIndex, 4);
                w.Write(&d.BoneIndex, 4);
                w.WriteString(d.Name);
                w.PopChunk();
            }
            for (const auto& vol : bbm.Volumes) {
                w.PushChunk("HCVL");
                w.Write(&vol.ID, 4);
                w.WriteString(vol.Name);
                w.Align();
                uint32_t pCount = (uint32_t)vol.Planes.size();
                w.Write(&pCount, 4);
                if (pCount > 0) w.Write(vol.Planes.data(), pCount * 16);
                w.PopChunk();
            }
            w.PopChunk(); // HLPR
        }

        w.PopChunk(); // 3DRT
        w.PopChunk(); // 3DMF

        // --- FINAL LZO COMPRESSION STAGE ---
        std::vector<uint8_t> uncompressed = w.buffer;
        uint32_t uncompSize = (uint32_t)uncompressed.size();

        std::vector<uint8_t> outBytes;
        outBytes.resize(4);
        memcpy(outBytes.data(), &uncompSize, 4); // Fable Monolithic Uncompressed Size Header

        std::vector<uint8_t> lzoData = CompressLZORaw(uncompressed.data(), uncompSize);
        if (lzoData.empty()) {
            outBytes.insert(outBytes.end(), uncompressed.begin(), uncompressed.end());
        }
        else {
            outBytes.insert(outBytes.end(), lzoData.begin(), lzoData.end());
        }

        return outBytes;
    }

private:
    static void WriteLZOBlock(std::vector<uint8_t>& out, const std::vector<uint8_t>& uncomp) {
        // If the uncompressed block is empty, Fable writes a flat 0x0000 
        if (uncomp.empty()) {
            uint16_t zero = 0;
            out.insert(out.end(), (uint8_t*)&zero, ((uint8_t*)&zero) + 2);
            return;
        }

        // Delegate to the custom Fable Wrapper in Utils.h
        std::vector<uint8_t> compressed = CompressFableBlock(uncomp.data(), (uint32_t)uncomp.size());

        // Fable's CompressFableBlock returns empty if the source is <= 3 bytes
        // In that case, we write size '0' followed by the raw bytes.
        if (compressed.empty()) {
            uint16_t zero = 0;
            out.insert(out.end(), (uint8_t*)&zero, ((uint8_t*)&zero) + 2);
            out.insert(out.end(), uncomp.begin(), uncomp.end());
        }
        else {
            out.insert(out.end(), compressed.begin(), compressed.end());
        }
    }
};