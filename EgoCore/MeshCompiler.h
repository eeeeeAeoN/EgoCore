// --- START OF FILE MeshCompiler.h ---

#pragma once
#include "MeshParser.h"
#include "Utils.h" // Ensures CompressFableBlock is available
#include <vector>
#include <string>
#include <cstring>

class MeshCompiler {
public:
    static std::vector<uint8_t> CompileSingleLOD(const C3DMeshContent& m, bool disableLZO = false) {
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
            WriteLZOBlock(data, hRaw, disableLZO);
        }

        // Compress Dummy Arrays
        if (m.DummyObjectCount > 0) {
            std::vector<uint8_t> dRaw(m.Dummies.size() * 56);
            for (size_t i = 0; i < m.Dummies.size(); i++) {
                memcpy(dRaw.data() + i * 56, &m.Dummies[i].NameCRC, 4);
                memcpy(dRaw.data() + i * 56 + 4, m.Dummies[i].Transform, 48);
                memcpy(dRaw.data() + i * 56 + 52, &m.Dummies[i].BoneIndex, 4);
            }
            WriteLZOBlock(data, dRaw, disableLZO);
        }

        // Compress Packed Strings
        if (namesSize > 0) {
            WriteLZOBlock(data, m.PackedNamesRaw, disableLZO);
        }

        // Compress Volumes
        for (const auto& vol : m.Volumes) {
            uint32_t version = 1; // ASM hardcodes this to 1u
            Write(&version, 4);
            WriteString(vol.Name);
            uint32_t pCount = (uint32_t)vol.Planes.size();
            Write(&pCount, 4);
            if (pCount > 0) {
                std::vector<uint8_t> pRaw(pCount * 16);
                memcpy(pRaw.data(), vol.Planes.data(), pCount * 16);
                WriteLZOBlock(data, pRaw, disableLZO);
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
            WriteLZOBlock(data, bNamesBlock, disableLZO);

            std::vector<uint8_t> bRaw(m.Bones.size() * 60);
            memcpy(bRaw.data(), m.Bones.data(), m.Bones.size() * 60);
            WriteLZOBlock(data, bRaw, disableLZO);

            WriteLZOBlock(data, m.BoneKeyframesRaw, disableLZO);
            WriteLZOBlock(data, m.BoneTransformsRaw, disableLZO);
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
            WriteLZOBlock(data, prim.VertexBuffer, disableLZO);

            std::vector<uint8_t> idxBytes(prim.IndexBuffer.size() * 2);
            if (!prim.IndexBuffer.empty()) memcpy(idxBytes.data(), prim.IndexBuffer.data(), idxBytes.size());
            WriteLZOBlock(data, idxBytes, disableLZO);

            uint32_t cpCount = (uint32_t)prim.ClothPrimitives.size(); Write(&cpCount, 4);
            for (const auto& cp : prim.ClothPrimitives) {
                Write(&cp.PrimitiveIndex, 4); Write(&cp.MaterialIndex, 4);
                WriteLZOBlock(data, cp.ParticleProgramData, disableLZO);
            }
        }
        return data;
    }

    static std::vector<uint8_t> CompileForExport(const C3DMeshContent& m, bool disableLZO = false) {
        std::vector<uint8_t> data = CompileSingleLOD(m, disableLZO);

        if (m.MeshType == 2 || m.MeshType == 5) {
            // Replicate exactly what the internal Fable compiler does on flush
            C3DMeshContent ghostLOD = m;
            ghostLOD.Materials.clear();
            ghostLOD.MaterialCount = 0;
            ghostLOD.Primitives.clear();
            ghostLOD.PrimitiveCount = 0;
            ghostLOD.TotalStaticBlocks = 0;
            ghostLOD.TotalAnimatedBlocks = 0;

            std::vector<uint8_t> ghostBytes = CompileSingleLOD(ghostLOD, disableLZO);
            data.insert(data.end(), ghostBytes.begin(), ghostBytes.end());
        }
        return data;
    }

    static std::vector<uint8_t> CompileWithGhostLOD(const C3DMeshContent& m) {
        return CompileForExport(m, false);
    }

    static std::vector<uint8_t> CompileAnimatedMesh(const C3DMeshContent& m) {
        return CompileForExport(m, false);
    }

    // --- NEW: GENERATES THE RAW UNCOMPRESSED BINARY FOR DEBUGGING ---
    static std::vector<uint8_t> CompilePhysicsUncompressed(const CBBMParser& bbm) {
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

        // --- 1. FIX FABLE MAGIC HEADER ---
        w.Write(">>>>", 4); // Replaces the leaked 0x28000040 memory pointer
        w.Write("3DMF", 4);
        uint32_t version = 100; // 0x64
        w.Write(&version, 4);
        w.WriteString("Copyright Big Blue Box Studios Ltd.");
        w.Align();

        w.PushChunk("3DRT");

        // --- MATERIALS ---
        w.PushChunk("MTLS");
        if (bbm.ParsedMaterials.empty()) {
            w.PushChunk("MTRL");
            w.WriteString("23 - Default");
            uint32_t idx = 0; w.Write(&idx, 4);
            uint8_t twoSided = 0; w.Write(&twoSided, 1); // Hex dump uses 00 for TwoSided here
            // Exact hex dump material floats/colors mapped
            uint32_t colors[6] = { 0xFF96EFF7, 0xFF96EFF7, 0xFFE5E5E5, 0x3DCCCCCC, 0x00000000, 0x3E99999A };
            w.Write(colors, 24);
            w.PopChunk();
        }
        else {
            for (const auto& m : bbm.ParsedMaterials) {
                w.PushChunk(m.IsEngineMaterial ? "EMTL" : "MTRL");

                // --- 2. FIX EMTL CHUNK STRUCTURE ---
                if (m.IsEngineMaterial) {
                    uint32_t ver = 4; w.Write(&ver, 4);
                    w.Write(&m.Index, 4);
                    w.WriteString(m.Name);

                    uint32_t flags = 0;
                    if (m.DiffuseBank > 0) flags |= 1;
                    if (m.BumpBank > 0) flags |= 2;
                    if (m.ReflectBank > 0) flags |= 4;
                    if (m.IllumBank > 0) flags |= 8;
                    w.Write(&flags, 4);

                    uint32_t banks[4] = { (uint32_t)m.DiffuseBank, (uint32_t)m.BumpBank, (uint32_t)m.ReflectBank, (uint32_t)m.IllumBank };
                    w.Write(banks, 16);

                    uint8_t hasTex = (!m.TextureDiffuse.empty() || !m.TextureBump.empty()) ? 1 : 0;
                    w.Write(&hasTex, 1);
                    if (hasTex) {
                        w.WriteString(m.TextureDiffuse); w.WriteString("");
                        w.WriteString(m.TextureBump); w.WriteString("");
                    }

                    w.Write(&m.SelfIllumination, 4);
                    uint8_t bools[4] = {
                        (uint8_t)(m.TwoSided ? 1 : 0),
                        (uint8_t)(m.Transparent ? 1 : 0),
                        (uint8_t)(m.BooleanAlpha ? 1 : 0),
                        (uint8_t)(m.DegenerateTriangles ? 1 : 0)
                    };
                    w.Write(bools, 4);
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
        w.WriteString("collision");

        // CRITICAL STRUCTURAL FIX: 4 Int32 properties explicitly required by C3DMeshFileSubMeshChunk
        uint32_t subMeshIdx = 0; w.Write(&subMeshIdx, 4);
        int32_t parentIdx = -1;  w.Write(&parentIdx, 4);
        int32_t firstChild = -1; w.Write(&firstChild, 4);
        int32_t nextSibling = -1; w.Write(&nextSibling, 4);

        // Provide standard local transform matrix
        w.PushChunk("TRFM");
        float identity[12] = { 1,0,0, 0,1,0, 0,0,1, 0,0,0 };
        w.Write(identity, 48);
        w.PopChunk(); // TRFM

        w.PushChunk("PRIM");

        // Primitive Material Index
        int32_t physMatIdx = bbm.PhysicsMaterialIndex >= 0 ? bbm.PhysicsMaterialIndex : 0;
        w.Write(&physMatIdx, 4);

        // --- 3. HEX MATCH: TRIS MUST COME BEFORE VERT ---
        if (!bbm.ParsedIndices.empty()) {
            w.PushChunk("TRIS");
            uint32_t triCount = (uint32_t)bbm.ParsedIndices.size() / 3;
            w.Write(&triCount, 4);
            w.Write(bbm.ParsedIndices.data(), bbm.ParsedIndices.size() * 2);
            w.PopChunk(); // TRIS
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
            w.PopChunk(); // VERT
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
                uint32_t version = 1; // ASM hardcodes this to 1u
                w.Write(&version, 4);
                w.WriteString(vol.Name);

                uint32_t pCount = (uint32_t)vol.Planes.size();
                w.Write(&pCount, 4);
                if (pCount > 0) w.Write(vol.Planes.data(), pCount * 16);
                w.PopChunk();
            }
            w.PopChunk(); // HLPR
        }

        w.PopChunk(); // 3DRT

        // CRITICAL STRUCTURAL FIX: 0x00000000 End marker to tell the reader there are no more chunks!
        uint32_t endMarker = 0;
        w.Write(&endMarker, 4);

        return w.buffer;
    }


    static std::vector<uint8_t> CompilePhysics(const CBBMParser& bbm) {
        // Build the raw uncompressed bytes exactly as they will be read
        std::vector<uint8_t> uncompressed = CompilePhysicsUncompressed(bbm);
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
    static void WriteLZOBlock(std::vector<uint8_t>& out, const std::vector<uint8_t>& uncomp, bool disableLZO = false) {
        if (disableLZO || uncomp.empty()) {
            uint16_t zero = 0;
            out.insert(out.end(), (uint8_t*)&zero, ((uint8_t*)&zero) + 2);
            out.insert(out.end(), uncomp.begin(), uncomp.end());
            return;
        }

        std::vector<uint8_t> compressed = CompressFableBlock(uncomp.data(), (uint32_t)uncomp.size());

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