#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <iomanip>
#include "Utils.h"

struct AnimTrack {
    uint32_t BoneIndex = 0;
    int32_t ParentIndex = -1;
    std::string BoneName = "";

    uint8_t PreFPSFlag = 0; // It IS 1 byte!

    float SamplesPerSecond = 30.0f;
    uint32_t FrameCount = 0;
    uint8_t PostFrameFlags[4] = { 0,0,0,0 };
    float PositionFactor = 1.0f;
    float ScalingFactor = 1.0f;

    std::vector<Vec4> RotationTrack;
    std::vector<uint16_t> PalettedRotations;
    std::vector<Vec3> PositionTrack;
    std::vector<uint16_t> PalettedPositions;

    void EvaluateFrame(int frame, Vec3& outPos, Vec4& outRot) const {
        if (!RotationTrack.empty()) {
            int rotIdx = frame;
            if (!PalettedRotations.empty()) rotIdx = (frame < PalettedRotations.size()) ? PalettedRotations[frame] : PalettedRotations.back();
            if (rotIdx >= RotationTrack.size()) rotIdx = (int)RotationTrack.size() - 1;
            outRot = RotationTrack[rotIdx];
        }

        if (!PositionTrack.empty()) {
            int posIdx = frame;
            if (!PalettedPositions.empty()) posIdx = (frame < PalettedPositions.size()) ? PalettedPositions[frame] : PalettedPositions.back();
            if (posIdx >= PositionTrack.size()) posIdx = (int)PositionTrack.size() - 1;
            outPos = PositionTrack[posIdx];
        }
    }
};

struct TimeEvent {
    std::string Name;
    float Time;
};

struct C3DAnimationInfo {
    float Duration = 0.0f;
    float NonLoopingDuration = 0.0f;
    Vec3 MovementVector = { 0,0,0 };
    float Rotation = 0.0f;

    bool HasHelper = false;
    std::vector<TimeEvent> TimeEvents;
    std::vector<uint8_t> XaloData;

    std::string ObjectName = "";
    bool IsCyclic = false;
    std::vector<uint32_t> BoneMaskBits;

    std::vector<uint8_t> AnrtHeaderData;
    std::vector<uint8_t> AobjHeaderData;

    std::string DebugInfo = "";
    bool IsParsed = false;
    std::vector<AnimTrack> Tracks;
    std::vector<AnimTrack> HelperTracks;

    bool Deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 24) return false;
        memcpy(&Duration, data.data(), 4);
        memcpy(&NonLoopingDuration, data.data() + 4, 4);
        memcpy(&MovementVector, data.data() + 8, 12);
        memcpy(&Rotation, data.data() + 20, 4);
        return true;
    }

    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data(24);
        memcpy(data.data(), &Duration, 4);
        memcpy(data.data() + 4, &NonLoopingDuration, 4);
        memcpy(data.data() + 8, &MovementVector, 12);
        memcpy(data.data() + 20, &Rotation, 4);
        return data;
    }
};

class AnimParser {
public:
    C3DAnimationInfo Data;
    std::vector<uint8_t> UncompressedData;

    bool Parse(const std::vector<uint8_t>& compressedData) {
        Data.IsParsed = false;
        Data.Tracks.clear();
        Data.HelperTracks.clear();
        Data.TimeEvents.clear();
        Data.XaloData.clear();
        Data.ObjectName = "";
        Data.BoneMaskBits.clear();
        Data.AnrtHeaderData.clear();
        Data.AobjHeaderData.clear();
        Data.Duration = 0.0f;
        Data.HasHelper = false;
        UncompressedData.clear();

        if (compressedData.size() < 8) return false;

        uint32_t magic = *(uint32_t*)compressedData.data();

        if (magic != 0x3E3E3E3E) {
            uint32_t uncompressedSize = magic;
            if (uncompressedSize == 0 || uncompressedSize > 50000000) uncompressedSize = 10 * 1024 * 1024;
            UncompressedData = DecompressRawLZO(compressedData, 4, uncompressedSize);
            if (UncompressedData.empty()) return false;
        }
        else {
            UncompressedData = compressedData;
        }

        size_t cursor = 0;

        if (cursor + 8 <= UncompressedData.size()) {
            uint32_t headerMagic = *(uint32_t*)(UncompressedData.data() + cursor);
            if (headerMagic == 0x3E3E3E3E) {
                cursor += 4;
                std::string sig((char*)UncompressedData.data() + cursor, 4);
                if (sig == "3DAF") {
                    cursor += 4;
                    cursor += 4;
                    while (cursor < UncompressedData.size() && UncompressedData[cursor] != '\0') cursor++;
                    if (cursor < UncompressedData.size()) cursor++;
                    cursor = (cursor + 3) & ~3;
                }
                else cursor -= 4;
            }
        }

        ParseChunkBlock(UncompressedData.data(), cursor, UncompressedData.size(), false);
        Data.IsParsed = true;
        return true;
    }

private:
    void ParseChunkBlock(const uint8_t* base, size_t& cursor, size_t endBoundary, bool isHelper) {
        while (cursor + 8 <= endBoundary) {
            std::string sig((char*)base + cursor, 4);
            uint32_t chunkSize = *(uint32_t*)(base + cursor + 4);
            size_t payloadStart = cursor + 8;
            size_t nextChunkStart = payloadStart + chunkSize;

            if (nextChunkStart > endBoundary) break;

            if (sig == "ANRT") {
                Data.IsCyclic = (base[payloadStart] != 0);
                memcpy(&Data.Duration, base + payloadStart + 1, 4);

                size_t innerCursor = payloadStart + 5;
                size_t headerStart = innerCursor;
                while (innerCursor + 4 <= nextChunkStart) {
                    std::string subSig((char*)base + innerCursor, 4);
                    if (subSig == "HLPR" || subSig == "AOBJ" || subSig == "XALO") break;
                    innerCursor++;
                }
                Data.AnrtHeaderData.assign(base + headerStart, base + innerCursor);
                ParseChunkBlock(base, innerCursor, nextChunkStart, isHelper);
            }
            else if (sig == "HLPR") {
                Data.HasHelper = true;
                size_t innerCursor = payloadStart;
                ParseChunkBlock(base, innerCursor, nextChunkStart, true);
            }
            else if (sig == "AOBJ") {
                size_t innerCursor = payloadStart;
                while (innerCursor < nextChunkStart && base[innerCursor] != '\0') {
                    Data.ObjectName += (char)base[innerCursor++];
                }
                if (innerCursor < nextChunkStart) innerCursor++;

                size_t headerStart = innerCursor;
                while (innerCursor + 4 <= nextChunkStart) {
                    std::string subSig((char*)base + innerCursor, 4);
                    if (subSig == "XSEQ" || subSig == "SEQ0" || subSig == "AMSK") break;
                    innerCursor++;
                }
                Data.AobjHeaderData.assign(base + headerStart, base + innerCursor);
                ParseChunkBlock(base, innerCursor, nextChunkStart, false);
            }
            else if (sig == "TMEV") {
                TimeEvent ev;
                size_t c = payloadStart;
                while (c < nextChunkStart && base[c] != '\0') { ev.Name += (char)base[c++]; }
                if (c < nextChunkStart) c++;
                if (c + 4 <= nextChunkStart) memcpy(&ev.Time, base + c, 4);
                Data.TimeEvents.push_back(ev);
            }
            else if (sig == "MVEC") {
                if (payloadStart + 12 <= nextChunkStart) memcpy(&Data.MovementVector, base + payloadStart, 12);
            }
            else if (sig == "XALO") {
                if (chunkSize > 0 && chunkSize < 1024 * 1024) Data.XaloData.assign(base + payloadStart, base + nextChunkStart);
            }
            else if (sig == "AMSK") {
                uint32_t wordCount = (chunkSize + 3) / 4;
                Data.BoneMaskBits.resize(wordCount, 0);
                memcpy(Data.BoneMaskBits.data(), base + payloadStart, chunkSize);
            }
            else if (sig == "XSEQ" || sig == "SEQ0") {
                if (isHelper) ParseXSEQ(base, payloadStart, nextChunkStart, Data.HelperTracks);
                else ParseXSEQ(base, payloadStart, nextChunkStart, Data.Tracks);
            }
            cursor = nextChunkStart;
        }
    }

    void ParseXSEQ(const uint8_t* base, size_t start, size_t end, std::vector<AnimTrack>& outTracks) {
        AnimTrack track;
        size_t c = start;

        if (c + 8 > end) return;

        memcpy(&track.BoneIndex, base + c, 4); c += 4;
        memcpy(&track.ParentIndex, base + c, 4); c += 4;

        while (c < end && base[c] != '\0') { track.BoneName += (char)base[c]; c++; }
        if (c < end) c++;

        if (c + 17 > end) return;

        track.PreFPSFlag = base[c]; c += 1;

        memcpy(&track.SamplesPerSecond, base + c, 4); c += 4;
        memcpy(&track.FrameCount, base + c, 4); c += 4;
        memcpy(track.PostFrameFlags, base + c, 4); c += 4;

        memcpy(&track.PositionFactor, base + c, 4); c += 4;
        memcpy(&track.ScalingFactor, base + c, 4); c += 4;

        // Fable's PositionFactor is exactly the multiplier needed!
        float truePosFactor = track.PositionFactor;

        // 1. UNIQUE ROTATIONS
        uint16_t rotCount = 0;
        if (c + 2 <= end) {
            memcpy(&rotCount, base + c, 2); c += 2;
            for (int i = 0; i < rotCount && c + 16 <= end; i++) {
                Vec4 q; memcpy(&q, base + c, 16); c += 16;
                track.RotationTrack.push_back(q);
            }
        }

        // 2. PALETTED ROTATIONS
        if (c + 2 <= end) {
            uint16_t palRotCount = 0; memcpy(&palRotCount, base + c, 2); c += 2;
            int bpi = (rotCount > 255) ? 2 : 1;
            for (int i = 0; i < palRotCount && c + bpi <= end; i++) {
                uint16_t idx = 0;
                if (bpi == 1) { idx = base[c++]; }
                else { memcpy(&idx, base + c, 2); c += 2; }
                track.PalettedRotations.push_back(idx);
            }
        }

        // 3. UNIQUE POSITIONS
        uint16_t posCount = 0;
        if (c + 2 <= end) {
            memcpy(&posCount, base + c, 2); c += 2;
            for (int i = 0; i < posCount && c + 6 <= end; i++) {
                int16_t ix, iy, iz;
                memcpy(&ix, base + c, 2); c += 2;
                memcpy(&iy, base + c, 2); c += 2;
                memcpy(&iz, base + c, 2); c += 2;

                Vec3 pos;
                pos.x = (float)ix * truePosFactor;
                pos.y = (float)iy * truePosFactor;
                pos.z = (float)iz * truePosFactor;
                track.PositionTrack.push_back(pos);
            }
        }

        // 4. PALETTED POSITIONS
        if (c + 2 <= end) {
            uint16_t palPosCount = 0; memcpy(&palPosCount, base + c, 2); c += 2;
            int bpi = (posCount > 255) ? 2 : 1;
            for (int i = 0; i < palPosCount && c + bpi <= end; i++) {
                uint16_t idx = 0;
                if (bpi == 1) { idx = base[c++]; }
                else { memcpy(&idx, base + c, 2); c += 2; }
                track.PalettedPositions.push_back(idx);
            }
        }

        outTracks.push_back(track);
    }
};

struct AnimUIContext {
    bool ShowHexWindow = false;
    std::vector<uint8_t> HexBuffer;
};