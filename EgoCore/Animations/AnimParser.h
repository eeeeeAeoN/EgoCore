#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include "Utils.h"

struct C3DAnimationInfo {
    float Duration = 0.0f;
    float NonLoopingDuration = 0.0f;
    Vec3 MovementVector = { 0,0,0 };
    float Rotation = 0.0f;

    bool Deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 24) return false;

        const uint8_t* ptr = data.data();
        size_t offset = 0;

        memcpy(&Duration, ptr + offset, 4); offset += 4;
        memcpy(&NonLoopingDuration, ptr + offset, 4); offset += 4;
        memcpy(&MovementVector, ptr + offset, 12); offset += 12;
        memcpy(&Rotation, ptr + offset, 4); offset += 4;

        return true;
    }

    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data(24);
        uint8_t* ptr = data.data();
        size_t offset = 0;

        memcpy(ptr + offset, &Duration, 4); offset += 4;
        memcpy(ptr + offset, &NonLoopingDuration, 4); offset += 4;
        memcpy(ptr + offset, &MovementVector, 12); offset += 12;
        memcpy(ptr + offset, &Rotation, 4); offset += 4;

        return data;
    }
};