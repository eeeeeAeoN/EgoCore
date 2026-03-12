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

inline uint32_t CalculateFableCRC(const std::string& str) {
    uint32_t crc = 0; // Fable uses 0 as the initial value
    for (char c : str) {
        crc ^= (uint8_t)c; // Strictly case-sensitive, no tolower()
        for (int i = 0; i < 8; i++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return crc; // No final bitwise inversion!
}

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
    std::vector<std::string> TextureFileNames; // NEW: Non-lossy preserve
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
    uint32_t BufferType; // NEW: Non-lossy preserve
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
    int32_t MeshType = -1;

    CMeshEntryMetadata EntryMeta;
    bool AnimatedFlag = false;
    float BoundingSphereCenter[3] = { 0 }; float BoundingSphereRadius = 0.0f;
    float BoundingBoxMin[3] = { 0 }; float BoundingBoxMax[3] = { 0 };
    uint16_t HelperPointCount = 0; uint16_t DummyObjectCount = 0; uint16_t PackedNamesSize = 0;
    uint16_t MeshVolumeCount = 0; uint16_t MeshGeneratorCount = 0;

    std::vector<uint8_t> PackedNamesRaw;
    std::map<uint32_t, std::string> CRCNameMap; // Resolves CRCs to actual string names

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

    void UnpackNames() {
        CRCNameMap.clear();
        if (PackedNamesRaw.size() < 2) return;
        uint16_t dummyOffset = *(uint16_t*)PackedNamesRaw.data();

        // 1. Read Helper Strings
        size_t cursor = 2;
        while (cursor < dummyOffset && cursor < PackedNamesRaw.size()) {
            std::string s = "";
            while (cursor < dummyOffset && PackedNamesRaw[cursor] != 0) {
                s += (char)PackedNamesRaw[cursor];
                cursor++;
            }
            if (cursor < dummyOffset) cursor++; // Skip null terminator

            if (!s.empty()) CRCNameMap[CalculateFableCRC(s)] = s;
        }

        // 2. Read Dummy Strings
        cursor = dummyOffset;
        while (cursor < PackedNamesRaw.size()) {
            std::string s = "";
            while (cursor < PackedNamesRaw.size() && PackedNamesRaw[cursor] != 0) {
                s += (char)PackedNamesRaw[cursor];
                cursor++;
            }
            if (cursor < PackedNamesRaw.size()) cursor++; // Skip null terminator

            if (!s.empty()) CRCNameMap[CalculateFableCRC(s)] = s;
        }
    }

    void PackNames(const std::vector<std::string>& helperNames, const std::vector<std::string>& dummyNames) {
        // Sets automatically sort the strings alphabetically, exactly as Fable expects
        std::set<std::string> hSet(helperNames.begin(), helperNames.end());
        std::set<std::string> dSet(dummyNames.begin(), dummyNames.end());

        PackedNamesRaw.clear();
        PackedNamesRaw.push_back(0); PackedNamesRaw.push_back(0);

        for (const auto& s : hSet) {
            if (s.empty()) continue;
            PackedNamesRaw.insert(PackedNamesRaw.end(), s.begin(), s.end());
            PackedNamesRaw.push_back(0);
        }

        // Fable Engine perfectly aligns the offset by requiring an empty string terminator here
        PackedNamesRaw.push_back(0);

        uint16_t dummyOffset = (uint16_t)PackedNamesRaw.size();
        PackedNamesRaw[0] = dummyOffset & 0xFF;
        PackedNamesRaw[1] = (dummyOffset >> 8) & 0xFF;

        for (const auto& s : dSet) {
            if (s.empty()) continue;
            PackedNamesRaw.insert(PackedNamesRaw.end(), s.begin(), s.end());
            PackedNamesRaw.push_back(0);
        }

        PackedNamesRaw.push_back(0);
        PackedNamesSize = (uint16_t)PackedNamesRaw.size();
    }

    std::string GetNameFromCRC(uint32_t crc) const {
        auto it = CRCNameMap.find(crc);
        if (it != CRCNameMap.end()) return it->second;
        return "";
    }

    std::vector<uint8_t> SerializeEntryMetadata() {
        std::vector<uint8_t> data;
        auto Write = [&](const void* val, size_t len) {
            size_t s = data.size(); data.resize(s + len); memcpy(data.data() + s, val, len);
            };

        Write(&EntryMeta.PhysicsIndex, 4);
        Write(EntryMeta.BoundingSphereCenter, 12);
        Write(&EntryMeta.BoundingSphereRadius, 4);
        Write(EntryMeta.BoundingBoxMin, 12);
        Write(EntryMeta.BoundingBoxMax, 12);

        Write(&EntryMeta.LODCount, 4);
        for (uint32_t s : EntryMeta.LODSizes) Write(&s, 4);
        Write(&EntryMeta.SafeBoundingRadius, 4);
        for (float e : EntryMeta.LODErrors) Write(&e, 4);

        uint32_t texCount = (uint32_t)EntryMeta.TextureIDs.size();
        Write(&texCount, 4);
        for (int32_t id : EntryMeta.TextureIDs) Write(&id, 4);

        return data;
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

    void UpdateMetadata(size_t compiledLZOSize) {
        EntryMeta.TextureIDs.clear();
        for (const auto& mat : Materials) {
            if (mat.DiffuseMapID > 0 && std::find(EntryMeta.TextureIDs.begin(), EntryMeta.TextureIDs.end(), mat.DiffuseMapID) == EntryMeta.TextureIDs.end()) EntryMeta.TextureIDs.push_back(mat.DiffuseMapID);
            if (mat.BumpMapID > 0 && std::find(EntryMeta.TextureIDs.begin(), EntryMeta.TextureIDs.end(), mat.BumpMapID) == EntryMeta.TextureIDs.end()) EntryMeta.TextureIDs.push_back(mat.BumpMapID);
            if (mat.ReflectionMapID > 0 && std::find(EntryMeta.TextureIDs.begin(), EntryMeta.TextureIDs.end(), mat.ReflectionMapID) == EntryMeta.TextureIDs.end()) EntryMeta.TextureIDs.push_back(mat.ReflectionMapID);
            if (mat.IlluminationMapID > 0 && std::find(EntryMeta.TextureIDs.begin(), EntryMeta.TextureIDs.end(), mat.IlluminationMapID) == EntryMeta.TextureIDs.end()) EntryMeta.TextureIDs.push_back(mat.IlluminationMapID);
            if (mat.DecalID > 0 && std::find(EntryMeta.TextureIDs.begin(), EntryMeta.TextureIDs.end(), mat.DecalID) == EntryMeta.TextureIDs.end()) EntryMeta.TextureIDs.push_back(mat.DecalID);
        }
        EntryMeta.LODCount = 1;
        EntryMeta.LODSizes.clear();
        EntryMeta.LODSizes.push_back((uint32_t)compiledLZOSize);
        EntryMeta.HasData = true;

        memcpy(EntryMeta.BoundingSphereCenter, BoundingSphereCenter, 12);
        EntryMeta.BoundingSphereRadius = BoundingSphereRadius;
        memcpy(EntryMeta.BoundingBoxMin, BoundingBoxMin, 12);
        memcpy(EntryMeta.BoundingBoxMax, BoundingBoxMax, 12);
    }

    int CalculateVertexStride(int initFlags, bool isAnimated) {
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

    bool Parse(const std::vector<uint8_t>& data, int entryType = -1) {
        MeshType = entryType;
        IsParsed = false;
        Helpers.clear(); Dummies.clear(); Volumes.clear(); Generators.clear();
        Materials.clear(); Primitives.clear();
        BoneIndices.clear(); BoneNames.clear(); Bones.clear();
        PackedNamesRaw.clear();
        CRCNameMap.clear();
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

        // 1. Compressed Helpers
        if (HelperPointCount > 0) {
            helpersRaw = DecompressLZO(b, cursor, sz, 20 * HelperPointCount);
            for (int i = 0; i < HelperPointCount && i * 20 + 20 <= helpersRaw.size(); ++i) {
                CHelperPoint h;
                memcpy(&h.NameCRC, helpersRaw.data() + i * 20, 4);
                memcpy(h.Pos, helpersRaw.data() + i * 20 + 4, 12);
                memcpy(&h.BoneIndex, helpersRaw.data() + i * 20 + 16, 4);
                Helpers.push_back(h);
            }
        }

        // 2. Compressed Dummies
        if (DummyObjectCount > 0) {
            dummiesRaw = DecompressLZO(b, cursor, sz, 56 * DummyObjectCount);
            for (int i = 0; i < DummyObjectCount && i * 56 + 56 <= dummiesRaw.size(); ++i) {
                CDummyObject d;
                memcpy(&d.NameCRC, dummiesRaw.data() + i * 56, 4);
                memcpy(d.Transform, dummiesRaw.data() + i * 56 + 4, 48);
                memcpy(&d.BoneIndex, dummiesRaw.data() + i * 56 + 52, 4);
                Dummies.push_back(d);
            }
        }

        // 3. Compressed Packed Names
        if (PackedNamesSize > 0) {
            PackedNamesRaw = DecompressLZO(b, cursor, sz, PackedNamesSize);
            UnpackNames();
        }

        // 4. Uncompressed Volumes (Contains compressed planes internally)
        for (int i = 0; i < MeshVolumeCount; i++) {
            CMeshVolume vol;
            if (!Read(b, cursor, sz, vol.ID)) break;
            vol.Name = ReadString(b, cursor, sz);

            uint32_t planeCount = 0;
            if (!Read(b, cursor, sz, planeCount)) break;

            if (planeCount > 0) {
                auto planeData = DecompressLZO(b, cursor, sz, 16 * planeCount);
                if (planeData.size() == 16 * planeCount) {
                    for (uint32_t p = 0; p < planeCount; p++) {
                        CPlane plane;
                        memcpy(&plane, planeData.data() + (p * 16), 16);
                        vol.Planes.push_back(plane);
                    }
                }
            }
            Volumes.push_back(vol);
        }

        // 5. Uncompressed Generators
        for (int i = 0; i < MeshGeneratorCount; i++) {
            CMeshGenerator gen;
            Read(b, cursor, sz, gen.Transform);
            Read(b, cursor, sz, gen.BoneIndex);
            gen.ObjectName = ReadString(b, cursor, sz);
            Read(b, cursor, sz, gen.BankIndex);
            Read(b, cursor, sz, gen.UseLocalOrigin);
            Generators.push_back(gen);
        }

        if (cursor >= sz) { DebugStatus = "File ended after Stats"; return false; }
        Read(b, cursor, sz, MaterialCount); Read(b, cursor, sz, PrimitiveCount); Read(b, cursor, sz, BoneCount); Read(b, cursor, sz, BoneNameSize); Read(b, cursor, sz, ClothFlag); Read(b, cursor, sz, TotalStaticBlocks); Read(b, cursor, sz, TotalAnimatedBlocks);

        if (BoneCount > 0) {
            if (BoneCount > 1000) { DebugStatus = "Invalid Bone Count"; return false; }
            BoneIndices.resize(BoneCount);
            if (cursor + (2 * BoneCount) <= sz) { memcpy(BoneIndices.data(), b + cursor, 2 * BoneCount); cursor += 2 * BoneCount; }
            else { DebugStatus = "Truncated Bone Indices"; return false; }

            BoneNamesRaw = DecompressLZO(b, cursor, sz, BoneNameSize);
            size_t bnCursor = 0;
            for (int i = 0; i < BoneCount && bnCursor < BoneNamesRaw.size(); i++) {
                BoneNames.push_back(ReadString(BoneNamesRaw.data(), bnCursor, BoneNamesRaw.size()));
            }

            auto boneDataRaw = DecompressLZO(b, cursor, sz, 60 * BoneCount);
            if (boneDataRaw.size() == 60 * BoneCount) { for (int i = 0; i < BoneCount; i++) { C3DBone bone; memcpy(&bone, boneDataRaw.data() + (i * 60), 60); Bones.push_back(bone); } }

            BoneKeyframesRaw = DecompressLZO(b, cursor, sz, 48 * BoneCount);
            BoneTransformsRaw = DecompressLZO(b, cursor, sz, 64 * BoneCount);
        }
        Read(b, cursor, sz, RootMatrix);

        for (int i = 0; i < MaterialCount; i++) {
            if (i > 1000) break; C3DMaterial mat; Read(b, cursor, sz, mat.ID); mat.Name = ReadString(b, cursor, sz); Read(b, cursor, sz, mat.DecalID); Read(b, cursor, sz, mat.DiffuseMapID); Read(b, cursor, sz, mat.BumpMapID); Read(b, cursor, sz, mat.ReflectionMapID); Read(b, cursor, sz, mat.IlluminationMapID); Read(b, cursor, sz, mat.MapFlags); Read(b, cursor, sz, mat.SelfIllumination); Read(b, cursor, sz, mat.IsTwoSided); Read(b, cursor, sz, mat.IsTransparent); Read(b, cursor, sz, mat.BooleanAlpha); Read(b, cursor, sz, mat.DegenerateTriangles); Read(b, cursor, sz, mat.UseFilenames);
            if (mat.UseFilenames) {
                for (int j = 0; j < 4; j++) {
                    mat.TextureFileNames.push_back(ReadString(b, cursor, sz));
                }
            }
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
            prim.BufferType = type;

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
            int reps = (p.RepeatingMeshReps > 1) ? p.RepeatingMeshReps : 1;
            int totalVertsToRead = p.VertexCount * reps;

            // Fable exceptions: Type 2 and Type 4 meshes store raw floats regardless of the InitFlags
            bool isPosComp = p.IsCompressed;
            if (MeshType == 4 || (p.VertexStride == 36 && p.AnimatedBlockCount == 0)) isPosComp = false;
            if (reps > 1) isPosComp = false;

            for (int v = 0; v < totalVertsToRead; v++) {
                size_t offset = v * p.VertexStride;
                if (offset + 12 > p.VertexBuffer.size()) break; // Safely ensure 12 bytes exist

                float x, y, z;
                if (isPosComp) {
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

        memcpy(BoundingBoxMin, min, 12); memcpy(BoundingBoxMax, max, 12);
        BoundingSphereCenter[0] = (min[0] + max[0]) * 0.5f;
        BoundingSphereCenter[1] = (min[1] + max[1]) * 0.5f;
        BoundingSphereCenter[2] = (min[2] + max[2]) * 0.5f;

        float maxDistSq = 0;
        for (const auto& p : Primitives) {
            int reps = (p.RepeatingMeshReps > 1) ? p.RepeatingMeshReps : 1;
            int totalVertsToRead = p.VertexCount * reps;

            bool isPosComp = p.IsCompressed;
            if (MeshType == 4 || (p.VertexStride == 36 && p.AnimatedBlockCount == 0)) isPosComp = false;
            if (reps > 1) isPosComp = false;

            for (int v = 0; v < totalVertsToRead; v++) {
                size_t offset = v * p.VertexStride;
                if (offset + 12 > p.VertexBuffer.size()) break;

                float x, y, z;
                if (isPosComp) {
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

    std::vector<uint8_t> SerializeUncompressed() const {
        std::vector<uint8_t> data;
        auto Write = [&](const void* val, size_t len) {
            size_t s = data.size(); data.resize(s + len); memcpy(data.data() + s, val, len);
            };
        auto WriteString = [&](const std::string& str) {
            Write(str.c_str(), str.length() + 1);
            };

        WriteString(MeshName);
        Write(&AnimatedFlag, 1);
        Write(BoundingSphereCenter, 12);
        Write(&BoundingSphereRadius, 4);
        Write(BoundingBoxMin, 12);
        Write(BoundingBoxMax, 12);
        Write(&HelperPointCount, 2);
        Write(&DummyObjectCount, 2);

        uint16_t calcPackedNamesSize = (uint16_t)PackedNamesRaw.size();
        Write(&calcPackedNamesSize, 2);

        Write(&MeshVolumeCount, 2);
        Write(&MeshGeneratorCount, 2);

        for (const auto& h : Helpers) { Write(&h.NameCRC, 4); Write(h.Pos, 12); Write(&h.BoneIndex, 4); }
        for (const auto& d : Dummies) { Write(&d.NameCRC, 4); Write(d.Transform, 48); Write(&d.BoneIndex, 4); }

        if (!PackedNamesRaw.empty()) Write(PackedNamesRaw.data(), PackedNamesRaw.size());

        for (const auto& vol : Volumes) {
            uint32_t version = 1; // ASM hardcodes this to 1u
            Write(&version, 4);
            WriteString(vol.Name);
            uint32_t pCount = (uint32_t)vol.Planes.size();
            Write(&pCount, 4);
            if (pCount > 0) Write(vol.Planes.data(), pCount * 16);
        }

        for (const auto& gen : Generators) {
            Write(gen.Transform, 48); Write(&gen.BoneIndex, 4); WriteString(gen.ObjectName); Write(&gen.BankIndex, 4); Write(&gen.UseLocalOrigin, 1);
        }

        Write(&MaterialCount, 4); Write(&PrimitiveCount, 4); Write(&BoneCount, 4);

        std::vector<uint8_t> bNamesBlock;
        for (const auto& s : BoneNames) bNamesBlock.insert(bNamesBlock.end(), s.c_str(), s.c_str() + s.length() + 1);
        uint32_t calcBoneNameSize = (uint32_t)bNamesBlock.size();
        Write(&calcBoneNameSize, 4);

        Write(&ClothFlag, 1); Write(&TotalStaticBlocks, 2); Write(&TotalAnimatedBlocks, 2);

        if (BoneCount > 0) {
            Write(BoneIndices.data(), 2 * BoneCount);
            if (!bNamesBlock.empty()) Write(bNamesBlock.data(), bNamesBlock.size());
            if (!Bones.empty()) Write(Bones.data(), 60 * BoneCount);
            if (!BoneKeyframesRaw.empty()) Write(BoneKeyframesRaw.data(), 48 * BoneCount);
            if (!BoneTransformsRaw.empty()) Write(BoneTransformsRaw.data(), 64 * BoneCount);
        }

        Write(RootMatrix, 48);

        for (const auto& mat : Materials) {
            Write(&mat.ID, 4); WriteString(mat.Name); Write(&mat.DecalID, 4); Write(&mat.DiffuseMapID, 4); Write(&mat.BumpMapID, 4);
            Write(&mat.ReflectionMapID, 4); Write(&mat.IlluminationMapID, 4); Write(&mat.MapFlags, 4); Write(&mat.SelfIllumination, 4);
            Write(&mat.IsTwoSided, 1); Write(&mat.IsTransparent, 1); Write(&mat.BooleanAlpha, 1); Write(&mat.DegenerateTriangles, 1);
            Write(&mat.UseFilenames, 1);
            if (mat.UseFilenames) {
                for (int j = 0; j < 4; j++) {
                    if (j < mat.TextureFileNames.size()) WriteString(mat.TextureFileNames[j]);
                    else WriteString(""); // Failsafe
                }
            }
        }

        for (const auto& prim : Primitives) {
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

            if (!prim.VertexBuffer.empty()) { Write(prim.VertexBuffer.data(), prim.VertexBuffer.size()); }
            else { uint16_t h = 0; Write(&h, 2); }

            if (!prim.IndexBuffer.empty()) { Write(prim.IndexBuffer.data(), prim.IndexBuffer.size() * 2); }
            else { uint16_t h = 0; Write(&h, 2); }

            uint32_t cpCount = (uint32_t)prim.ClothPrimitives.size(); Write(&cpCount, 4);
            for (const auto& cp : prim.ClothPrimitives) {
                Write(&cp.PrimitiveIndex, 4); Write(&cp.MaterialIndex, 4);
                uint32_t progLen = (uint32_t)cp.ParticleProgramData.size(); Write(&progLen, 4);
                if (progLen > 0) Write(cp.ParticleProgramData.data(), progLen);
            }
        }
        return data;
    }
};