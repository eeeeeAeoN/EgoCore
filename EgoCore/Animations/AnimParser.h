#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <iomanip>
#include "Utils.h"

struct AnimTrack {
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
        Data.Duration = 0.0f;
        Data.HasHelper = false;

        if (compressedData.size() < 8) return false;
        uint32_t magic = *(uint32_t*)compressedData.data();
        std::vector<uint8_t> rawData;

        if (magic != 0x3E3E3E3E) {
            uint32_t uncompressedSize = magic;
            if (uncompressedSize == 0 || uncompressedSize > 50000000) uncompressedSize = 10 * 1024 * 1024;
            rawData = DecompressRawLZO(compressedData, 4, uncompressedSize);
            if (rawData.empty()) return false;
        }
        else {
            rawData = std::vector<uint8_t>(compressedData.begin() + 4, compressedData.end());
        }

        size_t cursor = 0;
        bool inHelper = false;
        size_t helperEnd = 0;

        while (cursor + 8 <= rawData.size()) {
            // Keep track of when we leave the Helper block
            if (inHelper && cursor >= helperEnd) inHelper = false;

            std::string sig((char*)rawData.data() + cursor, 4);
            uint32_t chunkSize = *(uint32_t*)(rawData.data() + cursor + 4);
            size_t payloadStart = cursor + 8;

            // --- 1. CONTAINERS (Step inside, let scanner handle the data gaps) ---
            if (sig == "ANRT" && payloadStart + 5 <= rawData.size()) {
                Data.IsCyclic = (rawData[payloadStart] != 0);
                memcpy(&Data.Duration, rawData.data() + payloadStart + 1, 4);
                cursor += 8;
            }
            else if (sig == "HLPR") {
                inHelper = true;
                helperEnd = payloadStart + chunkSize;
                Data.HasHelper = true;
                cursor += 8;
            }
            else if (sig == "AOBJ") {
                if (Data.ObjectName.empty() && payloadStart < rawData.size()) {
                    size_t c = payloadStart;
                    while (c < rawData.size() && rawData[c] != '\0') { Data.ObjectName += (char)rawData[c]; c++; }
                }
                cursor += 8;
            }
            // --- 2. TERMINAL CHUNKS (Read, then strictly skip the whole payload) ---
            else if (sig == "TMEV" && payloadStart + chunkSize <= rawData.size()) {
                TimeEvent ev;
                size_t c = payloadStart;
                while (c < payloadStart + chunkSize && rawData[c] != '\0') { ev.Name += (char)rawData[c]; c++; }
                if (c < payloadStart + chunkSize) c++; // skip null terminator
                if (c + 4 <= payloadStart + chunkSize) memcpy(&ev.Time, rawData.data() + c, 4);
                Data.TimeEvents.push_back(ev);
                cursor += 8 + chunkSize;
            }
            else if (sig == "MVEC" && payloadStart + 12 <= rawData.size()) {
                memcpy(&Data.MovementVector, rawData.data() + payloadStart, 12);
                cursor += 8 + chunkSize;
            }
            else if (sig == "XALO" && payloadStart + chunkSize <= rawData.size() && chunkSize < 1024) {
                Data.XaloData.assign(rawData.data() + payloadStart, rawData.data() + payloadStart + chunkSize);
                cursor += 8 + chunkSize;
            }
            else if (sig == "AMSK" && payloadStart + chunkSize <= rawData.size() && chunkSize < 1024) {
                uint32_t wordCount = (chunkSize + 3) / 4;
                Data.BoneMaskBits.resize(wordCount, 0);
                memcpy(Data.BoneMaskBits.data(), rawData.data() + payloadStart, chunkSize);
                cursor += 8 + chunkSize;
            }
            else if ((sig == "XSEQ" || sig == "SEQ0") && payloadStart + chunkSize <= rawData.size()) {
                if (inHelper) ParseXSEQ(rawData.data(), payloadStart, payloadStart + chunkSize, Data.HelperTracks);
                else ParseXSEQ(rawData.data(), payloadStart, payloadStart + chunkSize, Data.Tracks);
                cursor += 8 + chunkSize;
            }
            // --- 3. THE SELF-HEALING SCANNER ---
            else {
                // If we are sitting on raw string data or padding, creep forward 1 byte
                // until we align with the next real "3DAF", "XSEQ", etc.
                cursor++;
            }
        }

        Data.IsParsed = true;
        return true;
    }

private:
    void ParseXSEQ(const uint8_t* base, size_t start, size_t end, std::vector<AnimTrack>& targetVector) {
        size_t c = start;
        AnimTrack track;
        if (c + 8 > end) return;
        c += 4; // Skip inner magic
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