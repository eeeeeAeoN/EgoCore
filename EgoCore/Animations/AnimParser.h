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
    uint8_t PreFPSFlag = 0;
    uint8_t PostFrameFlags[4] = { 0,0,0,0 };
    float SamplesPerSecond = 30.0f;
    uint32_t FrameCount = 0;
    float PositionFactor = 1.0f;
    float ScalingFactor = 1.0f;

    std::vector<Vec4> RotationTrack;
    std::vector<uint8_t> PalettedRotations;
    std::vector<Vec3> PositionTrack;
    std::vector<uint8_t> PalettedPositions;

    void EvaluateFrame(int frame, Vec3& outPos, Vec4& outRot) const {
        if (RotationTrack.empty()) outRot = { 0.0f, 0.0f, 0.0f, 1.0f };
        else {
            int rotIdx = frame;
            if (!PalettedRotations.empty()) rotIdx = (frame < PalettedRotations.size()) ? PalettedRotations[frame] : PalettedRotations.back();
            if (rotIdx >= RotationTrack.size()) rotIdx = (int)RotationTrack.size() - 1;
            outRot = RotationTrack[rotIdx];
        }

        if (PositionTrack.empty()) outPos = { 0.0f, 0.0f, 0.0f };
        else {
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

    // Preserved exact header structures
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

        if (compressedData.size() < 8) return false;

        std::vector<uint8_t> rawData;
        uint32_t magic = *(uint32_t*)compressedData.data();

        // 1. Decompress if necessary
        if (magic != 0x3E3E3E3E) { // 0x3E3E3E3E is ">>>>"
            uint32_t uncompressedSize = magic;
            if (uncompressedSize == 0 || uncompressedSize > 50000000) uncompressedSize = 10 * 1024 * 1024;
            rawData = DecompressRawLZO(compressedData, 4, uncompressedSize);
            if (rawData.empty()) return false;
        }
        else {
            rawData = compressedData;
        }

        size_t cursor = 0;

        // 2. Parse Fable 3DAF Header
        if (cursor + 8 <= rawData.size()) {
            uint32_t headerMagic = *(uint32_t*)(rawData.data() + cursor);
            if (headerMagic == 0x3E3E3E3E) {
                cursor += 4; // Skip ">>>>"
                std::string sig((char*)rawData.data() + cursor, 4);
                if (sig == "3DAF") {
                    cursor += 4; // Skip "3DAF"
                    cursor += 4; // Skip Version

                    while (cursor < rawData.size() && rawData[cursor] != '\0') cursor++;
                    if (cursor < rawData.size()) cursor++;

                    cursor = (cursor + 3) & ~3; // 3DAF Header is aligned
                }
                else cursor -= 4; // Fallback
            }
        }

        // 3. Kick off structural parsing
        ParseChunkBlock(rawData.data(), cursor, rawData.size(), false);

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

            if (nextChunkStart > endBoundary) break; // Corrupted, abort safely

            if (sig == "ANRT") {
                Data.IsCyclic = (base[payloadStart] != 0);
                memcpy(&Data.Duration, base + payloadStart + 1, 4);

                size_t innerCursor = payloadStart + 5;
                size_t headerStart = innerCursor;
                // Safely jump over unknown ANRT fields until we hit the first subchunk
                while (innerCursor + 4 <= nextChunkStart) {
                    std::string subSig((char*)base + innerCursor, 4);
                    if (subSig == "HLPR" || subSig == "AOBJ") break;
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
                if (innerCursor < nextChunkStart) innerCursor++; // skip \0

                size_t headerStart = innerCursor;
                // Safely jump over ParentIndex, FirstChild, NextSibling, SubMesh, etc.
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
                if (payloadStart + 12 <= nextChunkStart) {
                    memcpy(&Data.MovementVector, base + payloadStart, 12);
                }
            }
            else if (sig == "XALO") {
                if (chunkSize > 0 && chunkSize < 1024 * 1024) {
                    Data.XaloData.assign(base + payloadStart, base + nextChunkStart);
                }
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

            // Tightly packed: Move to exact chunk boundary without guessing padding
            cursor = nextChunkStart;
        }
    }

    void ParseXSEQ(const uint8_t* base, size_t start, size_t end, std::vector<AnimTrack>& targetVector) {
        size_t c = start;
        AnimTrack track;
        if (c + 8 > end) return;

        track.BoneIndex = *(uint32_t*)(base + c); c += 4;
        track.ParentIndex = *(int32_t*)(base + c); c += 4;

        while (c < end && base[c] != '\0') { track.BoneName += (char)base[c]; c++; }
        if (c < end) c++;
        if (c + 18 > end) return;

        track.PreFPSFlag = *(uint8_t*)(base + c); c += 1;
        track.SamplesPerSecond = *(float*)(base + c); c += 4;
        track.FrameCount = *(uint32_t*)(base + c); c += 4;
        memcpy(track.PostFrameFlags, base + c, 4); c += 4;
        track.PositionFactor = *(float*)(base + c); c += 4;
        track.ScalingFactor = *(float*)(base + c); c += 4;

        if (c + 2 <= end) {
            uint16_t rotCount = *(uint16_t*)(base + c); c += 2;
            if (c + (rotCount * 16) <= end) {
                for (int i = 0; i < rotCount; i++) {
                    Vec4 q; memcpy(&q, base + c, 16);
                    track.RotationTrack.push_back(q);
                    c += 16;
                }
            }
        }
        if (c + 2 <= end) {
            uint16_t palRotCount = *(uint16_t*)(base + c); c += 2;
            if (c + palRotCount <= end) {
                for (int i = 0; i < palRotCount; i++) track.PalettedRotations.push_back(base[c++]);
            }
        }
        if (c + 2 <= end) {
            uint16_t posCount = *(uint16_t*)(base + c); c += 2;
            if (c + (posCount * 6) <= end) {
                for (int i = 0; i < posCount; i++) {
                    int16_t ix, iy, iz;
                    memcpy(&ix, base + c, 2); c += 2;
                    memcpy(&iy, base + c, 2); c += 2;
                    memcpy(&iz, base + c, 2); c += 2;
                    Vec3 pos = { (float)ix * track.PositionFactor, (float)iy * track.PositionFactor, (float)iz * track.PositionFactor };
                    track.PositionTrack.push_back(pos);
                }
            }
        }
        if (c + 2 <= end) {
            uint16_t palPosCount = *(uint16_t*)(base + c); c += 2;
            if (c + palPosCount <= end) {
                for (int i = 0; i < palPosCount; i++) track.PalettedPositions.push_back(base[c++]);
            }
        }
        targetVector.push_back(track);
    }
};

struct AnimUIContext {
    bool ShowHexWindow = false;
    std::vector<uint8_t> HexBuffer;
};