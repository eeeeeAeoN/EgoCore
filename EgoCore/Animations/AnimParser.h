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
    float SamplesPerSecond = 30.0f;
    uint32_t FrameCount = 0;
    float PositionFactor = 1.0f;
    float ScalingFactor = 1.0f;

    std::vector<Vec4> RotationTrack;
    std::vector<uint8_t> PalettedRotations;

    std::vector<Vec3> PositionTrack;
    std::vector<uint8_t> PalettedPositions;

    // --- HELPER FOR EXPORT/PLAYBACK ---
    // Evaluates the animation at a specific frame index, handling palettes and defaults
    void EvaluateFrame(int frame, Vec3& outPos, Vec4& outRot) const {
        // 1. Evaluate Rotation
        if (RotationTrack.empty()) {
            outRot = { 0.0f, 0.0f, 0.0f, 1.0f }; // Identity Quaternion
        }
        else {
            int rotIdx = frame; // Default to current frame
            if (!PalettedRotations.empty()) {
                rotIdx = (frame < PalettedRotations.size()) ? PalettedRotations[frame] : PalettedRotations.back();
            }
            if (rotIdx >= RotationTrack.size()) rotIdx = RotationTrack.size() - 1; // Clamp to avoid crashes
            outRot = RotationTrack[rotIdx];
        }

        // 2. Evaluate Position
        if (PositionTrack.empty()) {
            outPos = { 0.0f, 0.0f, 0.0f };
        }
        else {
            int posIdx = frame; // Default to current frame
            if (!PalettedPositions.empty()) {
                posIdx = (frame < PalettedPositions.size()) ? PalettedPositions[frame] : PalettedPositions.back();
            }
            if (posIdx >= PositionTrack.size()) posIdx = PositionTrack.size() - 1; // Clamp to avoid crashes
            outPos = PositionTrack[posIdx];
        }
    }
};

struct C3DAnimationInfo {
    // ToC Metadata
    float Duration = 0.0f;
    float NonLoopingDuration = 0.0f;
    Vec3 MovementVector = { 0,0,0 };
    float Rotation = 0.0f;

    // Payload Metadata
    std::string ObjectName = "";
    bool IsCyclic = false;

    std::string DebugInfo = "";
    bool IsParsed = false;
    std::vector<AnimTrack> Tracks;

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
        Data.DebugInfo = "=== ANIM PARSER ===\n";
        Data.IsParsed = false;
        Data.Tracks.clear();
        Data.ObjectName = "";

        if (compressedData.size() < 8) return false;

        uint32_t magic = *(uint32_t*)compressedData.data();
        std::vector<uint8_t> rawData;

        if (magic != 0x3E3E3E3E) {
            uint32_t uncompressedSize = magic;
            if (uncompressedSize == 0 || uncompressedSize > 50000000) uncompressedSize = 10 * 1024 * 1024;
            rawData = DecompressRawLZO(compressedData, 4, uncompressedSize);
            if (rawData.empty()) {
                Data.DebugInfo += "[X] LZO Decompression failed\n";
                return false;
            }
        }
        else {
            rawData = compressedData;
        }

        Data.DebugInfo += "Decompressed Size: " + std::to_string(rawData.size()) + " bytes\n";

        size_t cursor = 0;
        while (cursor + 8 < rawData.size()) {
            std::string sig((char*)rawData.data() + cursor, 4);
            uint32_t chunkSize = *(uint32_t*)(rawData.data() + cursor + 4);
            size_t payloadStart = cursor + 8;

            if (sig == "ANRT" && payloadStart + 5 <= rawData.size()) {
                Data.IsCyclic = (rawData[payloadStart] != 0);
            }
            else if (sig == "AOBJ" && payloadStart < rawData.size()) {
                size_t c = payloadStart;
                while (c < rawData.size() && rawData[c] != '\0') {
                    Data.ObjectName += (char)rawData[c];
                    c++;
                }
                Data.DebugInfo += "Skeleton Target: " + Data.ObjectName + "\n";
            }
            else if (sig == "XSEQ" || sig == "SEQ0") {
                if (payloadStart + chunkSize <= rawData.size()) {
                    ParseXSEQ(rawData.data(), payloadStart, payloadStart + chunkSize);
                }
            }

            // Advance by byte if not a sequence chunk, otherwise skip the whole chunk to find next
            if (sig == "XSEQ" || sig == "SEQ0") cursor += 8 + chunkSize;
            else cursor++;
        }

        Data.DebugInfo += "\n[+] Successfully parsed " + std::to_string(Data.Tracks.size()) + " tracks.\n";
        Data.IsParsed = true;
        return true;
    }

private:
    void ParseXSEQ(const uint8_t* base, size_t start, size_t end) {
        size_t c = start;
        AnimTrack track;

        if (c + 8 > end) return;
        c += 4; // Skip Magic

        track.ParentIndex = *(int32_t*)(base + c); c += 4;

        while (c < end && base[c] != '\0') {
            track.BoneName += (char)base[c];
            c++;
        }
        if (c < end) c++;

        if (c + 18 > end) return;

        c += 1; // Flags 1
        track.SamplesPerSecond = *(float*)(base + c); c += 4;
        track.FrameCount = *(uint32_t*)(base + c); c += 4;
        c += 4; // Flags 2,3,4,5
        track.PositionFactor = *(float*)(base + c); c += 4;
        track.ScalingFactor = *(float*)(base + c); c += 4;

        if (c + 2 > end) return;
        uint16_t rotCount = *(uint16_t*)(base + c); c += 2;
        if (c + (rotCount * 16) > end) return;
        for (int i = 0; i < rotCount; i++) {
            Vec4 q; memcpy(&q, base + c, 16);
            track.RotationTrack.push_back(q);
            c += 16;
        }

        if (c + 2 > end) return;
        uint16_t palRotCount = *(uint16_t*)(base + c); c += 2;
        if (c + palRotCount > end) return;
        for (int i = 0; i < palRotCount; i++) track.PalettedRotations.push_back(base[c++]);

        if (c + 2 > end) return;
        uint16_t posCount = *(uint16_t*)(base + c); c += 2;
        if (c + (posCount * 6) > end) return;
        for (int i = 0; i < posCount; i++) {
            int16_t ix, iy, iz;
            memcpy(&ix, base + c, 2); c += 2;
            memcpy(&iy, base + c, 2); c += 2;
            memcpy(&iz, base + c, 2); c += 2;

            Vec3 pos = {
                (float)ix * track.PositionFactor,
                (float)iy * track.PositionFactor,
                (float)iz * track.PositionFactor
            };
            track.PositionTrack.push_back(pos);
        }

        if (c + 2 > end) return;
        uint16_t palPosCount = *(uint16_t*)(base + c); c += 2;
        if (c + palPosCount > end) return;
        for (int i = 0; i < palPosCount; i++) track.PalettedPositions.push_back(base[c++]);

        Data.Tracks.push_back(track);

        Data.DebugInfo += "- Track: " + track.BoneName + "\n";
        Data.DebugInfo += "  Frames: " + std::to_string(track.FrameCount) + " @ " + std::to_string(track.SamplesPerSecond) + "fps\n";
        Data.DebugInfo += "  Rotations: " + std::to_string(rotCount) + " unique, " + std::to_string(palRotCount) + " paletted\n";
        Data.DebugInfo += "  Positions: " + std::to_string(posCount) + " unique, " + std::to_string(palPosCount) + " paletted\n\n";
    }
};