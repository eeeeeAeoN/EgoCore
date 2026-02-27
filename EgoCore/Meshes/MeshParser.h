#pragma once
#include "imgui.h"
#include "Utils.h"
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include "BBMParser.h"

inline void UnpackPOSPACKED3(uint32_t packed, const float* scale, const float* offset, float& x, float& y, float& z) {
    int32_t ix = packed & 0x7FF; if (ix & 0x400) ix |= 0xFFFFF800;
    int32_t iy = (packed >> 11) & 0x7FF; if (iy & 0x400) iy |= 0xFFFFF800;
    int32_t iz = (packed >> 22); if (iz & 0x200) iz |= 0xFFFFFC00;
    x = (float)ix * 0.0009775171f * scale[0] + offset[0];
    y = (float)iy * 0.0009775171f * scale[1] + offset[1];
    z = (float)iz * 0.0019569471f * scale[2] + offset[2];
}

inline void UnpackNORMPACKED3(uint32_t packed, float& x, float& y, float& z) {
    int32_t ix = packed & 0x7FF; if (ix & 0x400) ix |= 0xFFFFF800;
    int32_t iy = (packed >> 11) & 0x7FF; if (iy & 0x400) iy |= 0xFFFFF800;
    int32_t iz = (packed >> 22); if (iz & 0x200) iz |= 0xFFFFFC00;
    x = (float)ix * 0.0009775171f; y = (float)iy * 0.0009775171f; z = (float)iz * 0.0019569471f;
}

inline float DecompressUV(int16_t v) { return ((float)v / 2048.0f) - 8.0f; }

struct CVertexCompressionParams { float Scale[4]; float Offset[4]; };
struct CHelperPoint { uint32_t NameCRC; float Pos[3]; int32_t BoneIndex; };
struct CDummyObject { uint32_t NameCRC; float Transform[12]; int32_t BoneIndex; };
struct C3DBone { uint32_t NameCRC; int32_t ParentIndex; int32_t OriginalNoChildren; float LocalizationMatrix[12]; };
struct CPlane { float Normal[3]; float D; };
struct CMeshVolume { uint32_t ID; std::string Name; std::vector<CPlane> Planes; };
struct CMeshGenerator { float Transform[12]; int32_t BoneIndex; std::string ObjectName; uint32_t BankIndex; bool UseLocalOrigin; };

struct C3DMaterial {
    int32_t ID; std::string Name; int32_t DecalID; int32_t DiffuseMapID; int32_t BumpMapID;
    int32_t ReflectionMapID; int32_t IlluminationMapID; int32_t MapFlags; int32_t SelfIllumination;
    bool IsTwoSided; bool IsTransparent; bool BooleanAlpha; bool DegenerateTriangles; bool UseFilenames;
};

struct CStaticBlock { uint32_t PrimitiveCount; uint32_t StartIndex; bool IsStrip; uint8_t ChangeFlags; bool DegenerateTriangles; int32_t MaterialIndex; };
struct CAnimatedBlock {
    uint32_t PrimitiveCount; uint32_t StartIndex; bool IsStrip; uint8_t ChangeFlags; bool DegenerateTriangles;
    uint32_t VertexCount; uint16_t BonesPerVertex; bool PalettedFlag; std::vector<uint8_t> Groups;
};
struct CClothPrimitive { uint32_t PrimitiveIndex; uint32_t MaterialIndex; std::vector<uint8_t> ParticleProgramData; };

struct C3DPrimitive {
    int32_t MaterialIndex; int32_t RepeatingMeshReps; float SphereCenter[3]; float SphereRadius; float AvgTextureStretch;
    uint32_t StaticBlockCount; uint32_t AnimatedBlockCount;
    uint32_t VertexCount; uint32_t TriangleCount; uint32_t IndexCount; uint32_t InitFlags;
    uint32_t StaticBlockCount_2; uint32_t AnimatedBlockCount_2;
    bool IsCompressed; bool HasExtraData; int32_t VertexStride;
    CVertexCompressionParams Compression;
    std::vector<CStaticBlock> StaticBlocks;
    std::vector<CAnimatedBlock> AnimatedBlocks;
    std::vector<uint8_t> VertexBuffer;
    std::vector<uint16_t> IndexBuffer;
    std::vector<CClothPrimitive> ClothPrimitives;
};

struct CMeshEntryMetadata {
    bool HasData = false; uint32_t RawSize = 0; int32_t PhysicsIndex = -1;
    float BoundingSphereCenter[3] = { 0 }; float BoundingSphereRadius = 0.0f;
    float BoundingBoxMin[3] = { 0 }; float BoundingBoxMax[3] = { 0 };
    uint32_t LODCount = 0; std::vector<uint32_t> LODSizes; float SafeBoundingRadius = 0.0f;
    std::vector<float> LODErrors; std::vector<int32_t> TextureIDs;
};

struct C3DMeshContent {
    std::string MeshName;
    CMeshEntryMetadata EntryMeta;
    bool AnimatedFlag = false;
    float BoundingSphereCenter[3] = { 0 }; float BoundingSphereRadius = 0.0f;
    float BoundingBoxMin[3] = { 0 }; float BoundingBoxMax[3] = { 0 };
    uint16_t HelperPointCount = 0; uint16_t DummyObjectCount = 0; uint16_t PackedNamesSize = 0;
    uint16_t MeshVolumeCount = 0; uint16_t MeshGeneratorCount = 0;
    std::vector<std::string> HelperNameStrings; std::vector<std::string> DummyNameStrings;
    std::vector<CHelperPoint> Helpers; std::vector<CDummyObject> Dummies;
    std::vector<CMeshVolume> Volumes; std::vector<CMeshGenerator> Generators;
    int32_t MaterialCount = 0; int32_t PrimitiveCount = 0; int32_t BoneCount = 0;
    int32_t BoneNameSize = 0; bool ClothFlag = false;
    uint16_t TotalStaticBlocks = 0; uint16_t TotalAnimatedBlocks = 0;
    std::vector<uint16_t> BoneIndices; std::vector<uint8_t> BoneNamesRaw; std::vector<std::string> BoneNames;
    std::vector<C3DBone> Bones;
    std::vector<uint8_t> BoneKeyframesRaw; std::vector<uint8_t> BoneTransformsRaw;
    float RootMatrix[12];
    std::vector<C3DMaterial> Materials; std::vector<C3DPrimitive> Primitives;
    bool IsParsed = false; std::string DebugStatus = "Not Parsed";

    std::string ReadString(const uint8_t* buffer, size_t& cursor, size_t fileSize) {
        std::string s = "";
        while (cursor < fileSize && buffer[cursor] != 0) { s += (char)buffer[cursor]; cursor++; }
        if (cursor < fileSize) cursor++;
        return s;
    }

    template <typename T>
    bool Read(const uint8_t* buffer, size_t& cursor, size_t fileSize, T& dest) {
        if (cursor + sizeof(T) > fileSize) return false;
        memcpy(&dest, buffer + cursor, sizeof(T));
        cursor += sizeof(T);
        return true;
    }

    void ParseEntryMetadata(const std::vector<uint8_t>& data) {
        EntryMeta = CMeshEntryMetadata();
        EntryMeta.RawSize = (uint32_t)data.size();
        EntryMeta.HasData = true;
        if (data.size() < 44) return;
        size_t cursor = 0;
        Read(data.data(), cursor, data.size(), EntryMeta.PhysicsIndex);
        Read(data.data(), cursor, data.size(), EntryMeta.BoundingSphereCenter);
        Read(data.data(), cursor, data.size(), EntryMeta.BoundingSphereRadius);
        Read(data.data(), cursor, data.size(), EntryMeta.BoundingBoxMin);
        Read(data.data(), cursor, data.size(), EntryMeta.BoundingBoxMax);
        if (!Read(data.data(), cursor, data.size(), EntryMeta.LODCount)) return;
        for (uint32_t i = 0; i < EntryMeta.LODCount; i++) {
            uint32_t size = 0; if (Read(data.data(), cursor, data.size(), size)) EntryMeta.LODSizes.push_back(size);
        }
        Read(data.data(), cursor, data.size(), EntryMeta.SafeBoundingRadius);
        if (EntryMeta.LODCount > 1) {
            for (uint32_t i = 0; i < EntryMeta.LODCount - 1; i++) { float err = 0.0f; if (Read(data.data(), cursor, data.size(), err)) EntryMeta.LODErrors.push_back(err); }
        }
        uint32_t texCount = 0;
        if (Read(data.data(), cursor, data.size(), texCount)) {
            if (cursor + (texCount * 4) <= data.size()) {
                for (uint32_t i = 0; i < texCount; i++) { int32_t id = 0; Read(data.data(), cursor, data.size(), id); EntryMeta.TextureIDs.push_back(id); }
            }
        }
    }

    int CalculateVertexStride(int initFlags, bool isAnimated) {
        // 0x10 flag forces Position to be uncompressed
        bool isPosComp = (initFlags & 4) != 0 && (initFlags & 0x10) == 0;
        bool isNormComp = (initFlags & 4) != 0;
        bool hasBump = (initFlags & 2) != 0;

        int stride = 0;
        stride += isPosComp ? 4 : 12; // Pos
        if (isAnimated) stride += 8; // Weights (4) + Indices (4)
        stride += isNormComp ? 4 : 12; // Norm
        stride += isNormComp ? 4 : 8; // UV
        if (hasBump) stride += isNormComp ? 8 : 16; // Bump
        return stride;
    }

    bool Parse(const std::vector<uint8_t>& data) {
        IsParsed = false;
        Helpers.clear(); Dummies.clear(); Volumes.clear(); Generators.clear();
        Materials.clear(); Primitives.clear(); HelperNameStrings.clear(); DummyNameStrings.clear();
        BoneIndices.clear(); BoneNames.clear(); Bones.clear();
        DebugStatus = "Reset";

        if (data.size() < 100) { DebugStatus = "File too small"; return false; }
        const uint8_t* b = data.data(); size_t sz = data.size(); size_t cursor = 0;

        MeshName = ReadString(b, cursor, sz);
        Read(b, cursor, sz, AnimatedFlag);
        Read(b, cursor, sz, BoundingSphereCenter);
        Read(b, cursor, sz, BoundingSphereRadius);
        Read(b, cursor, sz, BoundingBoxMin);
        Read(b, cursor, sz, BoundingBoxMax);
        Read(b, cursor, sz, HelperPointCount);
        Read(b, cursor, sz, DummyObjectCount);
        Read(b, cursor, sz, PackedNamesSize);
        Read(b, cursor, sz, MeshVolumeCount);
        Read(b, cursor, sz, MeshGeneratorCount);

        std::vector<uint8_t> helpersRaw, dummiesRaw;
        if (HelperPointCount > 0) helpersRaw = DecompressLZO(b, cursor, sz, 20 * HelperPointCount);
        if (DummyObjectCount > 0) dummiesRaw = DecompressLZO(b, cursor, sz, 56 * DummyObjectCount);

        if (PackedNamesSize > 0) {
            std::vector<uint8_t> namesRaw = DecompressLZO(b, cursor, sz, PackedNamesSize);
            if (namesRaw.size() >= 2) {
                uint16_t dummyOff = *(uint16_t*)namesRaw.data();
                size_t nCursor = 2;
                for (int i = 0; i < HelperPointCount; i++) {
                    if (nCursor >= dummyOff) break;
                    std::string s = ReadString(namesRaw.data(), nCursor, dummyOff);
                    HelperNameStrings.push_back(s);
                }
                nCursor = dummyOff;
                for (int i = 0; i < DummyObjectCount; i++) {
                    if (nCursor >= namesRaw.size()) break;
                    std::string s = ReadString(namesRaw.data(), nCursor, namesRaw.size());
                    DummyNameStrings.push_back(s);
                }
            }
        }

        for (int i = 0; i < HelperPointCount && i * 20 < helpersRaw.size(); ++i) {
            CHelperPoint h; memcpy(&h.NameCRC, helpersRaw.data() + i * 20, 4); memcpy(h.Pos, helpersRaw.data() + i * 20 + 4, 12); memcpy(&h.BoneIndex, helpersRaw.data() + i * 20 + 16, 4); Helpers.push_back(h);
        }
        for (int i = 0; i < DummyObjectCount && i * 56 < dummiesRaw.size(); ++i) {
            CDummyObject d; memcpy(&d.NameCRC, dummiesRaw.data() + i * 56, 4); memcpy(d.Transform, dummiesRaw.data() + i * 56 + 4, 48); memcpy(&d.BoneIndex, dummiesRaw.data() + i * 56 + 52, 4); Dummies.push_back(d);
        }
        for (int i = 0; i < MeshVolumeCount; i++) {
            CMeshVolume vol; if (!Read(b, cursor, sz, vol.ID)) break; vol.Name = ReadString(b, cursor, sz);
            uint32_t planeCount = 0; if (!Read(b, cursor, sz, planeCount)) break;
            if (planeCount > 0) {
                auto planeData = DecompressLZO(b, cursor, sz, 16 * planeCount);
                if (planeData.size() == 16 * planeCount) { for (uint32_t p = 0; p < planeCount; p++) { CPlane plane; memcpy(&plane, planeData.data() + (p * 16), 16); vol.Planes.push_back(plane); } }
            }
            Volumes.push_back(vol);
        }
        for (int i = 0; i < MeshGeneratorCount; i++) {
            CMeshGenerator gen; Read(b, cursor, sz, gen.Transform); Read(b, cursor, sz, gen.BoneIndex); gen.ObjectName = ReadString(b, cursor, sz); Read(b, cursor, sz, gen.BankIndex); Read(b, cursor, sz, gen.UseLocalOrigin); Generators.push_back(gen);
        }

        if (cursor >= sz) { DebugStatus = "File ended after Stats"; return false; }
        Read(b, cursor, sz, MaterialCount); Read(b, cursor, sz, PrimitiveCount); Read(b, cursor, sz, BoneCount); Read(b, cursor, sz, BoneNameSize); Read(b, cursor, sz, ClothFlag); Read(b, cursor, sz, TotalStaticBlocks); Read(b, cursor, sz, TotalAnimatedBlocks);

        if (BoneCount > 0) {
            if (BoneCount > 1000) { DebugStatus = "Invalid Bone Count"; return false; }
            BoneIndices.resize(BoneCount);
            if (cursor + (2 * BoneCount) <= sz) { memcpy(BoneIndices.data(), b + cursor, 2 * BoneCount); cursor += 2 * BoneCount; }
            else { DebugStatus = "Truncated Bone Indices"; return false; }

            BoneNamesRaw = DecompressLZO(b, cursor, sz, BoneNameSize);
            size_t bnCursor = 0; while (bnCursor < BoneNamesRaw.size()) { std::string s = ReadString(BoneNamesRaw.data(), bnCursor, BoneNamesRaw.size()); if (!s.empty()) BoneNames.push_back(s); }

            auto boneDataRaw = DecompressLZO(b, cursor, sz, 60 * BoneCount);
            if (boneDataRaw.size() == 60 * BoneCount) { for (int i = 0; i < BoneCount; i++) { C3DBone bone; memcpy(&bone, boneDataRaw.data() + (i * 60), 60); Bones.push_back(bone); } }

            BoneKeyframesRaw = DecompressLZO(b, cursor, sz, 48 * BoneCount);
            BoneTransformsRaw = DecompressLZO(b, cursor, sz, 64 * BoneCount);
        }
        Read(b, cursor, sz, RootMatrix);

        for (int i = 0; i < MaterialCount; i++) {
            if (i > 1000) break; C3DMaterial mat; Read(b, cursor, sz, mat.ID); mat.Name = ReadString(b, cursor, sz); Read(b, cursor, sz, mat.DecalID); Read(b, cursor, sz, mat.DiffuseMapID); Read(b, cursor, sz, mat.BumpMapID); Read(b, cursor, sz, mat.ReflectionMapID); Read(b, cursor, sz, mat.IlluminationMapID); Read(b, cursor, sz, mat.MapFlags); Read(b, cursor, sz, mat.SelfIllumination); Read(b, cursor, sz, mat.IsTwoSided); Read(b, cursor, sz, mat.IsTransparent); Read(b, cursor, sz, mat.BooleanAlpha); Read(b, cursor, sz, mat.DegenerateTriangles); Read(b, cursor, sz, mat.UseFilenames);
            if (mat.UseFilenames) { for (int j = 0; j < 4; j++) ReadString(b, cursor, sz); }
            Materials.push_back(mat);
        }

        for (int i = 0; i < PrimitiveCount; i++) {
            if (i > 1000) break; C3DPrimitive prim;
            if (!Read(b, cursor, sz, prim.MaterialIndex)) break;
            Read(b, cursor, sz, prim.RepeatingMeshReps); Read(b, cursor, sz, prim.SphereCenter); Read(b, cursor, sz, prim.SphereRadius); Read(b, cursor, sz, prim.AvgTextureStretch);
            Read(b, cursor, sz, prim.StaticBlockCount); Read(b, cursor, sz, prim.AnimatedBlockCount);
            Read(b, cursor, sz, prim.VertexCount); Read(b, cursor, sz, prim.TriangleCount); Read(b, cursor, sz, prim.IndexCount); Read(b, cursor, sz, prim.InitFlags);
            Read(b, cursor, sz, prim.StaticBlockCount_2); Read(b, cursor, sz, prim.AnimatedBlockCount_2);

            prim.IsCompressed = (prim.InitFlags & 4) != 0;
            prim.HasExtraData = (prim.InitFlags & 2) != 0;
            bool isPrimAnimated = (prim.AnimatedBlockCount > 0);
            prim.VertexStride = CalculateVertexStride(prim.InitFlags, isPrimAnimated);

            for (uint32_t j = 0; j < prim.StaticBlockCount; j++) {
                CStaticBlock sb; Read(b, cursor, sz, sb.PrimitiveCount); Read(b, cursor, sz, sb.StartIndex); Read(b, cursor, sz, sb.IsStrip); Read(b, cursor, sz, sb.ChangeFlags); Read(b, cursor, sz, sb.DegenerateTriangles); Read(b, cursor, sz, sb.MaterialIndex); prim.StaticBlocks.push_back(sb);
            }
            for (uint32_t j = 0; j < prim.AnimatedBlockCount; j++) {
                CAnimatedBlock ab; Read(b, cursor, sz, ab.PrimitiveCount); Read(b, cursor, sz, ab.StartIndex); Read(b, cursor, sz, ab.IsStrip); Read(b, cursor, sz, ab.ChangeFlags); Read(b, cursor, sz, ab.DegenerateTriangles); Read(b, cursor, sz, ab.VertexCount); Read(b, cursor, sz, ab.BonesPerVertex); Read(b, cursor, sz, ab.PalettedFlag);
                uint8_t groupCount = 0; Read(b, cursor, sz, groupCount);
                if (groupCount > 0) { ab.Groups.resize(groupCount); if (cursor + groupCount <= sz) { memcpy(ab.Groups.data(), b + cursor, groupCount); cursor += groupCount; } }
                prim.AnimatedBlocks.push_back(ab);
            }

            if (cursor + 32 > sz) { DebugStatus = "Truncated before Vertex Comp"; break; }
            memcpy(&prim.Compression, b + cursor, 32); cursor += 32;

            uint32_t stride = 0, type = 0; Read(b, cursor, sz, stride); Read(b, cursor, sz, type);
            if (stride > 0) prim.VertexStride = stride;

            int32_t reps = (prim.RepeatingMeshReps <= 1) ? 1 : prim.RepeatingMeshReps;
            size_t vBufferSize = (size_t)prim.VertexStride * prim.VertexCount * reps;
            if (vBufferSize > 0) { prim.VertexBuffer = DecompressLZO(b, cursor, sz, vBufferSize); }
            else { if (cursor + 2 <= sz) { uint16_t h = *(uint16_t*)(b + cursor); if (h == 0) cursor += 2; } }

            size_t iBufferSize = 2 * reps * prim.IndexCount;
            if (iBufferSize > 0) { auto idxBytes = DecompressLZO(b, cursor, sz, iBufferSize); if (!idxBytes.empty()) { prim.IndexBuffer.resize(idxBytes.size() / 2); memcpy(prim.IndexBuffer.data(), idxBytes.data(), idxBytes.size()); } }
            else { if (cursor + 2 <= sz) { uint16_t h = *(uint16_t*)(b + cursor); if (h == 0) cursor += 2; } }

            uint32_t clothPrimCount = 0;
            if (Read(b, cursor, sz, clothPrimCount)) {
                for (uint32_t c = 0; c < clothPrimCount; c++) {
                    CClothPrimitive cp; Read(b, cursor, sz, cp.PrimitiveIndex); Read(b, cursor, sz, cp.MaterialIndex);
                    uint32_t progLen = 0; Read(b, cursor, sz, progLen);
                    if (progLen > 0) { cp.ParticleProgramData = DecompressLZO(b, cursor, sz, progLen); }
                    prim.ClothPrimitives.push_back(cp);
                }
            }
            Primitives.push_back(prim);
        }
        IsParsed = true; DebugStatus = "Successfully parsed mesh"; return true;
    }

    void AutoCalculateBounds() {
        if (!IsParsed || Primitives.empty()) return;

        float min[3] = { 1e9, 1e9, 1e9 };
        float max[3] = { -1e9, -1e9, -1e9 };

        for (const auto& p : Primitives) {
            for (int v = 0; v < p.VertexCount; v++) {
                size_t offset = v * p.VertexStride;
                if (offset + 4 > p.VertexBuffer.size()) break;

                float x, y, z;
                if (p.IsCompressed) {
                    uint32_t packed = *(uint32_t*)(p.VertexBuffer.data() + offset);
                    UnpackPOSPACKED3(packed, p.Compression.Scale, p.Compression.Offset, x, y, z);
                }
                else {
                    const float* raw = (const float*)(p.VertexBuffer.data() + offset);
                    x = raw[0]; y = raw[1]; z = raw[2];
                }

                if (x < min[0]) min[0] = x; if (y < min[1]) min[1] = y; if (z < min[2]) min[2] = z;
                if (x > max[0]) max[0] = x; if (y > max[1]) max[1] = y; if (z > max[2]) max[2] = z;
            }
        }

        memcpy(BoundingBoxMin, min, 12);
        memcpy(BoundingBoxMax, max, 12);

        BoundingSphereCenter[0] = (min[0] + max[0]) * 0.5f;
        BoundingSphereCenter[1] = (min[1] + max[1]) * 0.5f;
        BoundingSphereCenter[2] = (min[2] + max[2]) * 0.5f;

        float maxDistSq = 0;
        for (const auto& p : Primitives) {
            for (int v = 0; v < p.VertexCount; v++) {
                size_t offset = v * p.VertexStride;
                float x, y, z;
                if (p.IsCompressed) {
                    UnpackPOSPACKED3(*(uint32_t*)(p.VertexBuffer.data() + offset), p.Compression.Scale, p.Compression.Offset, x, y, z);
                }
                else {
                    const float* raw = (const float*)(p.VertexBuffer.data() + offset);
                    x = raw[0]; y = raw[1]; z = raw[2];
                }
                float dx = x - BoundingSphereCenter[0];
                float dy = y - BoundingSphereCenter[1];
                float dz = z - BoundingSphereCenter[2];
                float dSq = dx * dx + dy * dy + dz * dz;
                if (dSq > maxDistSq) maxDistSq = dSq;
            }
        }
        BoundingSphereRadius = sqrtf(maxDistSq);
    }
};
