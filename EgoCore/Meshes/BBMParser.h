#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>
#include "Utils.h"

class CBBMParser {
public:

    struct ChunkHeader { char id[4]; uint32_t size; };
    struct Vec3 { float x, y, z; };
    struct Vec2 { float u, v; };

    struct C3DVertex2 {
        Vec3 Position;
        Vec3 Normal;
        Vec2 UV;
    };

    struct HelperPoint {
        std::string Name;
        Vec3 Position;
        int32_t BoneIndex;
        int32_t SubMeshIndex;
    };

    struct DummyObject {
        std::string Name;
        float Transform[12];
        int32_t BoneIndex;
        int32_t SubMeshIndex;
        Vec3 Position;
        Vec3 Direction;
        bool UseLocalOrigin;
    };

    struct BBMPlane { float Normal[3]; float D; };

    struct Volume {
        std::string Name;
        uint32_t ID;
        uint32_t PlaneCount;
        std::vector<BBMPlane> Planes;
    };

    struct Bone {
        int32_t Index;
        int32_t ParentIndex;
        int32_t FirstChildIndex;
        int32_t NextSiblingIndex;
        float LocalTransform[12];
        std::string Name;
    };

    struct SmoothingGroups {
        std::vector<uint32_t> FaceGroupMasks;
    };

    struct BBMMap {
        int32_t Type;
        std::string Description;
        std::string Filename;
    };

    struct BBMMaterial {
        std::string Name;
        int32_t Index = -1;
        bool IsEngineMaterial = false;
        bool TwoSided = false; bool Transparent = false; bool BooleanAlpha = false; bool DegenerateTriangles = false;
        uint32_t Ambient = 0xFFFFFFFF; uint32_t Diffuse = 0xFFFFFFFF; uint32_t Specular = 0xFFFFFFFF;
        uint32_t SelfIllumination = 0;
        float Shiny = 0.0f; float ShinyStrength = 0.0f; float Transparency = 0.0f;
        int DiffuseBank = 0; int BumpBank = 0; int ReflectBank = 0; int IllumBank = 0;
        std::vector<BBMMap> Maps;
        std::string TextureDiffuse; std::string TextureBump; std::string TextureReflect; std::string TextureIllum;
    };

    struct Cloth {
        std::string Name;
        int32_t MaterialIndex;
        std::string ParticleCode;
        std::vector<uint8_t> CompiledCode;
    };

    struct VertexWeight {
        uint16_t VertexIndex;
        float BlendValue;
    };
    struct VertexGroup {
        int32_t BoneIndex;
        std::vector<VertexWeight> Weights;
    };

    std::string DebugInfo;
    bool IsParsed = false;
    uint32_t FileVersion = 0;
    std::string FileComment;
    int32_t PhysicsMaterialIndex = 0;

    std::vector<C3DVertex2> ParsedVertices;
    std::vector<uint32_t> ParsedUniqueVertices;
    std::vector<uint16_t> ParsedIndices;
    std::vector<HelperPoint> Helpers;
    std::vector<DummyObject> Dummies;
    std::vector<Volume> Volumes;
    std::vector<Bone> Bones;
    std::vector<BBMMaterial> ParsedMaterials;
    std::vector<Cloth> Cloths;
    std::vector<VertexGroup> VertexGroups;
    SmoothingGroups SmoothGroups;
    float LastTransform[12] = { 1,0,0, 0,1,0, 0,0,1, 0,0,0 };
    bool HasTransform = false;

    bool Parse(const std::vector<uint8_t>& compressedData) {
        IsParsed = false; DebugInfo = ""; PhysicsMaterialIndex = 0;
        ParsedVertices.clear(); ParsedIndices.clear(); Helpers.clear(); Dummies.clear();
        Volumes.clear(); Bones.clear(); ParsedMaterials.clear(); SmoothGroups.FaceGroupMasks.clear();
        Cloths.clear(); VertexGroups.clear();
        HasTransform = false; memset(LastTransform, 0, 48);
        LastTransform[0] = 1; LastTransform[4] = 1; LastTransform[8] = 1;

        if (compressedData.size() < 8) return false;

        uint32_t uncompressedSize = *(uint32_t*)compressedData.data();
        if (uncompressedSize == 0 || uncompressedSize > 50000000) uncompressedSize = 10 * 1024 * 1024;

        DebugInfo += "=== BBM PARSER v10.0 (Stable Baseline + Groups) ===\n";

        std::vector<uint8_t> data = DecompressRawLZO(compressedData, 4, uncompressedSize);
        if (data.empty()) { DebugInfo += "[X] Decompression failed\n"; return false; }

        size_t c = 0;
        size_t end = data.size();

        uint32_t magic = 0; Read(data, c, magic);
        std::string fileType = ReadStringFixed(data, c, 4);
        Read(data, c, FileVersion);
        FileComment = ReadString(data, c, end);
        Align(c);

        DebugInfo += "Version: " + std::to_string(FileVersion) + "\n";

        ProcessChunks(data.data(), c, end, 0);

        IsParsed = true;
        DebugInfo += "\n[+] Parse Complete. " + std::to_string(ParsedVertices.size()) + " Verts, " + std::to_string(ParsedMaterials.size()) + " Mats.\n";
        return true;
    }

private:
    void ProcessChunks(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        std::string indent(depth * 2, ' ');

        while (cursor + 8 <= end) {
            ChunkHeader header;
            memcpy(&header, base + cursor, 8);

            std::string id = "????";
            bool isPrintable = true;
            for (int i = 0; i < 4; i++) if (!isprint((unsigned char)header.id[i])) isPrintable = false;
            if (isPrintable) id = std::string(header.id, 4);

            uint32_t chunkSize = header.size;
            size_t nextChunkOffset = cursor + 8 + chunkSize;

            if (nextChunkOffset > end) {
                uint32_t maskedSize = chunkSize & 0xFFFF;
                size_t maskedOffset = cursor + 8 + maskedSize;
                bool validRecovery = false;
                if (maskedOffset + 4 <= end) {
                    validRecovery = true;
                    for (int k = 0; k < 4; k++) {
                        uint8_t c = base[maskedOffset + k];
                        if (!isalnum(c) && c != '_') validRecovery = false;
                    }
                }
                if (validRecovery) {
                    chunkSize = maskedSize;
                    nextChunkOffset = maskedOffset;
                }
                else {
                    DebugInfo += indent + "[!] Overrun " + id + " (Raw: " + ToHex32(chunkSize) + ")\n";
                    break;
                }
            }

            cursor += 8;

            if (id == "3DRT" || id == "HLPR" || id == "GRUP") {
                ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
            }
            else if (id == "SUBM") { ParseSUBM(base, cursor, nextChunkOffset, depth); }
            else if (id == "PRIM") { ParsePRIM(base, cursor, nextChunkOffset, depth); }
            else if (id == "BONE") { ParseBONE(base, cursor, nextChunkOffset, depth); }
            else if (id == "MTRL") { ParseMTRL(base, cursor, nextChunkOffset, depth); }
            else if (id == "EMTL") { ParseEMTL(base, cursor, nextChunkOffset, depth); }
            else if (id == "MTLS") { ParseMTLS(base, cursor, nextChunkOffset, depth); }
            else if (id == "CLTH") { ParseCLTH(base, cursor, nextChunkOffset, depth); }
            else if (id == "VGRP") { ParseVGRP(base, cursor, nextChunkOffset, depth); }
            else if (id == "VERT") { ParseVERT(base, cursor, nextChunkOffset, depth); }
            else if (id == "TRIS") { ParseTRIS(base, cursor, nextChunkOffset, depth); }
            else if (id == "UNIV") { ParseUNIV(base, cursor, nextChunkOffset, depth); }
            else if (id == "SMTH") { ParseSMTH(base, cursor, nextChunkOffset, depth); }
            else if (id == "HPNT") { ParseHPNT(base, cursor, nextChunkOffset, depth); }
            else if (id == "HDMY") { ParseHDMY(base, cursor, nextChunkOffset, depth); }
            else if (id == "TRFM") { ParseTRFM(base, cursor, nextChunkOffset, depth); }
            else if (id == "HCVL") { ParseHCVL(base, cursor, nextChunkOffset, depth); }
            else if (id == "MMAP") { ParseMMAP(base, cursor, nextChunkOffset, depth); }
            else if (id == "MTLE") { ParseMTLE(base, cursor, nextChunkOffset, depth); }

            cursor = nextChunkOffset;
        }
    }

    void ParseSUBM(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        std::string indent(depth * 2, ' ');
        std::string name = ReadString(base, cursor, end);
        DebugInfo += indent + "  Name: \"" + name + "\"\n";

        for (size_t i = cursor; i < cursor + 256 && i + 4 < end; i++) {
            if (IsChunkHeader(base + i, "PRIM") || IsChunkHeader(base + i, "TRFM") ||
                IsChunkHeader(base + i, "BONE") || IsChunkHeader(base + i, "CLTH") ||
                IsChunkHeader(base + i, "VGRP")) {
                cursor = i;
                ProcessChunks(base, cursor, end, depth + 1);
                return;
            }
        }
    }

    void ParseVGRP(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        if (nextChunkOffset - cursor < 8) {
            if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
            return;
        }

        VertexGroup grp;
        memcpy(&grp.BoneIndex, base + cursor, 4); cursor += 4;

        uint32_t count = 0;
        memcpy(&count, base + cursor, 4); cursor += 4;

        size_t dataSize = count * 6;
        if (cursor + dataSize <= nextChunkOffset) {
            grp.Weights.reserve(count);
            for (uint32_t i = 0; i < count; i++) {
                VertexWeight vw;
                memcpy(&vw.VertexIndex, base + cursor, 2); cursor += 2;
                memcpy(&vw.BlendValue, base + cursor, 4); cursor += 4;
                grp.Weights.push_back(vw);
            }
        }
        VertexGroups.push_back(grp);

        if (cursor < nextChunkOffset) {
            ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
        }
    }

    void ParseCLTH(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        uint32_t version = 0;
        if (cursor + 4 <= nextChunkOffset) { memcpy(&version, base + cursor, 4); cursor += 4; }

        Cloth cloth;
        cloth.Name = ReadString(base, cursor, nextChunkOffset);
        if (cursor + 4 <= nextChunkOffset) { memcpy(&cloth.MaterialIndex, base + cursor, 4); cursor += 4; }
        cloth.ParticleCode = ReadString(base, cursor, nextChunkOffset);

        if (cursor + 1 <= nextChunkOffset) {
            bool hasCompiled = (base[cursor] != 0); cursor += 1;
            if (hasCompiled) {
                uint32_t codeSize = 0;
                if (cursor + 4 <= nextChunkOffset) { memcpy(&codeSize, base + cursor, 4); cursor += 4; }
                if (codeSize > 0 && cursor + codeSize <= nextChunkOffset) {
                    cloth.CompiledCode.resize(codeSize);
                    memcpy(cloth.CompiledCode.data(), base + cursor, codeSize);
                    cursor += codeSize;
                }
            }
        }
        Cloths.push_back(cloth);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParsePRIM(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        std::string indent(depth * 2, ' ');
        if (cursor + 4 <= end) {
            int32_t headerVal = 0; memcpy(&headerVal, base + cursor, 4);
            if (headerVal >= -1 && headerVal < 1000) PhysicsMaterialIndex = headerVal;
        }
        for (size_t i = 0; i < 256 && cursor + i + 4 < end; i++) {
            if (IsChunkHeader(base + cursor + i, "TRIS") || IsChunkHeader(base + cursor + i, "VERT") ||
                IsChunkHeader(base + cursor + i, "UNIV") || IsChunkHeader(base + cursor + i, "SMTH")) {
                cursor += i;
                ProcessChunks(base, cursor, end, depth + 1);
                return;
            }
        }
    }

    void ParseBONE(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        if ((nextChunkOffset - cursor) < 64) { cursor = nextChunkOffset; return; }
        Bone bone;
        memcpy(&bone.Index, base + cursor, 4); cursor += 4;
        memcpy(&bone.ParentIndex, base + cursor, 4); cursor += 4;
        memcpy(&bone.FirstChildIndex, base + cursor, 4); cursor += 4;
        memcpy(&bone.NextSiblingIndex, base + cursor, 4); cursor += 4;
        memcpy(bone.LocalTransform, base + cursor, 48); cursor += 48;
        if (cursor < nextChunkOffset) bone.Name = ReadString(base, cursor, nextChunkOffset);
        else bone.Name = "<Unnamed>";
        Bones.push_back(bone);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParseTRFM(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if (cursor + 48 <= end) { memcpy(LastTransform, base + cursor, 48); HasTransform = true; cursor += 48; }
        if (cursor < end) ProcessChunks(base, cursor, end, depth + 1);
    }

    void ParseVERT(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        size_t chunkSize = end - cursor;
        if (chunkSize <= 4) return;
        uint32_t vertCount = 0; memcpy(&vertCount, base + cursor, 4);
        size_t stride = (vertCount > 0) ? (chunkSize - 4) / vertCount : 0;
        if (stride >= 12 && vertCount > 0) {
            size_t prevSize = ParsedVertices.size();
            ParsedVertices.reserve(prevSize + vertCount);
            float* m = LastTransform;
            for (uint32_t i = 0; i < vertCount; i++) {
                size_t vOffset = cursor + 4 + (i * stride);
                C3DVertex2 vert = {};
                if (vOffset + 12 <= end) memcpy(&vert.Position, base + vOffset, 12);
                if (HasTransform) {
                    float x = vert.Position.x, y = vert.Position.y, z = vert.Position.z;
                    vert.Position.x = x * m[0] + y * m[3] + z * m[6] + m[9];
                    vert.Position.y = x * m[1] + y * m[4] + z * m[7] + m[10];
                    vert.Position.z = x * m[2] + y * m[5] + z * m[8] + m[11];
                }
                if (stride >= 24 && vOffset + 24 <= end) memcpy(&vert.Normal, base + vOffset + 12, 12);
                if (stride >= 32 && vOffset + 32 <= end) memcpy(&vert.UV, base + vOffset + 24, 8);
                ParsedVertices.push_back(vert);
            }
        }
    }

    void ParseTRIS(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if ((end - cursor) <= 4) return;
        uint32_t triCount = 0; memcpy(&triCount, base + cursor, 4);
        size_t idxCount = (end - cursor - 4) / 2;
        if (idxCount > 0) {
            size_t old = ParsedIndices.size();
            ParsedIndices.resize(old + idxCount);
            memcpy(ParsedIndices.data() + old, base + cursor + 4, idxCount * 2);
        }
    }

    void ParseMTLS(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if (cursor + 4 <= end && (IsChunkHeader(base + cursor, "MTRL") || IsChunkHeader(base + cursor, "EMTL"))) {
            ProcessChunks(base, cursor, end, depth + 1);
        }
        else {
            while (cursor < end) {
                std::string s = ReadString(base, cursor, end);
                if (s.length() > 2) {
                    bool valid = true; for (char c : s) if (!isprint((unsigned char)c) && c != ' ') valid = false;
                    if (valid) { BBMMaterial m; m.Name = s; m.Index = (int)ParsedMaterials.size(); ParsedMaterials.push_back(m); }
                }
            }
        }
    }

    void ParseHPNT(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        if ((nextChunkOffset - cursor) < 20) return;
        HelperPoint h;
        memcpy(&h.Position, base + cursor, 12); cursor += 12;
        memcpy(&h.SubMeshIndex, base + cursor, 4); cursor += 4;
        memcpy(&h.BoneIndex, base + cursor, 4); cursor += 4;
        h.Name = ReadString(base, cursor, nextChunkOffset);
        Helpers.push_back(h);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParseHDMY(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        DummyObject d; d.UseLocalOrigin = false;
        uint32_t ver = 0; if (cursor + 4 <= nextChunkOffset) { memcpy(&ver, base + cursor, 4); cursor += 4; }
        if (cursor + 12 <= nextChunkOffset) { memcpy(&d.Direction, base + cursor, 12); cursor += 12; }
        if (ver >= 2 && cursor + 48 <= nextChunkOffset) { memcpy(d.Transform, base + cursor, 48); cursor += 48; }
        else { memset(d.Transform, 0, 48); d.Transform[0] = 1; d.Transform[4] = 1; d.Transform[8] = 1; }
        if (ver >= 3 && cursor + 1 <= nextChunkOffset) { d.UseLocalOrigin = (base[cursor] != 0); cursor += 1; }
        if (cursor + 20 <= nextChunkOffset) {
            memcpy(&d.Position, base + cursor, 12); cursor += 12;
            memcpy(&d.SubMeshIndex, base + cursor, 4); cursor += 4;
            memcpy(&d.BoneIndex, base + cursor, 4); cursor += 4;
            d.Name = ReadString(base, cursor, nextChunkOffset);
        }
        Dummies.push_back(d);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParseMTRL(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        BBMMaterial mat = {};
        mat.Name = ReadString(base, cursor, nextChunkOffset);
        if (cursor + 4 <= nextChunkOffset) { memcpy(&mat.Index, base + cursor, 4); cursor += 4; }
        if (cursor + 1 <= nextChunkOffset) { mat.TwoSided = (base[cursor] != 0); cursor += 1; }
        if (cursor + 24 <= nextChunkOffset) {
            memcpy(&mat.Ambient, base + cursor, 4); cursor += 4; memcpy(&mat.Diffuse, base + cursor, 4); cursor += 4; memcpy(&mat.Specular, base + cursor, 4); cursor += 4;
            memcpy(&mat.Shiny, base + cursor, 4); cursor += 4; memcpy(&mat.ShinyStrength, base + cursor, 4); cursor += 4; memcpy(&mat.Transparency, base + cursor, 4); cursor += 4;
        }
        ParsedMaterials.push_back(mat);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParseEMTL(const uint8_t* base, size_t& cursor, size_t nextChunkOffset, int depth) {
        BBMMaterial mat = {}; mat.IsEngineMaterial = true;
        if (cursor + 8 <= nextChunkOffset) cursor += 8;
        mat.Name = ReadString(base, cursor, nextChunkOffset);
        if (cursor + 20 <= nextChunkOffset) {
            cursor += 4;
            memcpy(&mat.DiffuseBank, base + cursor, 4); cursor += 4; memcpy(&mat.BumpBank, base + cursor, 4); cursor += 4; memcpy(&mat.ReflectBank, base + cursor, 4); cursor += 4; memcpy(&mat.IllumBank, base + cursor, 4); cursor += 4;
        }
        if (cursor + 1 <= nextChunkOffset && base[cursor] != 0) {
            cursor++;
            mat.TextureDiffuse = ReadString(base, cursor, nextChunkOffset); ReadString(base, cursor, nextChunkOffset);
            mat.TextureBump = ReadString(base, cursor, nextChunkOffset); ReadString(base, cursor, nextChunkOffset);
        }
        ParsedMaterials.push_back(mat);
        if (cursor < nextChunkOffset) ProcessChunks(base, cursor, nextChunkOffset, depth + 1);
    }

    void ParseHCVL(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if ((end - cursor) < 8) return; Volume v;
        memcpy(&v.ID, base + cursor, 4); cursor += 4;
        v.Name = ReadString(base, cursor, end); Align(cursor);
        if (cursor + 4 <= end) {
            memcpy(&v.PlaneCount, base + cursor, 4); cursor += 4;
            for (uint32_t i = 0; i < v.PlaneCount && cursor + 16 <= end; i++) {
                BBMPlane p; memcpy(&p, base + cursor, 16); v.Planes.push_back(p); cursor += 16;
            }
        }
        Volumes.push_back(v);
    }

    void ParseUNIV(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if ((end - cursor) < 4) return; uint32_t c = 0; memcpy(&c, base + cursor, 4); cursor += 4;
        if (cursor + c * 4 > end) c = (end - cursor) / 4;
        for (uint32_t i = 0; i < c; i++) { uint32_t v; memcpy(&v, base + cursor, 4); ParsedUniqueVertices.push_back(v); cursor += 4; }
    }
    void ParseSMTH(const uint8_t* base, size_t& cursor, size_t end, int depth) {
        if ((end - cursor) < 4) return; uint32_t c = 0; memcpy(&c, base + cursor, 4); cursor += 4;
        if (cursor + c * 4 <= end) { for (uint32_t i = 0; i < c; i++) { uint32_t v; memcpy(&v, base + cursor, 4); SmoothGroups.FaceGroupMasks.push_back(v); cursor += 4; } }
    }
    void ParseMMAP(const uint8_t* base, size_t& cursor, size_t end, int depth) { cursor = end; }
    void ParseMTLE(const uint8_t* base, size_t& cursor, size_t end, int depth) { cursor = end; }

    std::string ToHex32(uint32_t val) { std::stringstream ss; ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << val; return ss.str(); }
    template <typename T> bool Read(const std::vector<uint8_t>& b, size_t& c, T& val) { if (c + sizeof(T) > b.size()) return false; memcpy(&val, b.data() + c, sizeof(T)); c += sizeof(T); return true; }
    std::string ReadString(const std::vector<uint8_t>& b, size_t& c, size_t limit) { return ReadString(b.data(), c, limit); }
    std::string ReadString(const uint8_t* b, size_t& c, size_t limit) { std::string s; while (c < limit && b[c] != 0) { s += (char)b[c]; c++; } if (c < limit) c++; return s; }
    std::string ReadStringFixed(const std::vector<uint8_t>& b, size_t& c, size_t len) { if (c + len > b.size()) return ""; std::string s((char*)b.data() + c, len); c += len; return s; }
    void Align(size_t& cursor) { cursor = (cursor + 3) & ~3; }
    bool IsChunkHeader(const uint8_t* ptr, const char* sig) { return ptr[0] == sig[0] && ptr[1] == sig[1] && ptr[2] == sig[2] && ptr[3] == sig[3]; }
};