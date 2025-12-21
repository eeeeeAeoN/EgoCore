#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

struct CLipSyncPhonemeRef {
    uint8_t ID;
    std::string Symbol;
};

struct CLipSyncFrameKey {
    uint8_t ID;
    uint8_t WeightByte;
    float WeightFloat; // Cached: Byte / 255.0f
};

struct CLipSyncFrame {
    std::vector<CLipSyncFrameKey> Keys;
};

struct CLipSyncData {
    // Header Info
    float Duration = 0.0f;

    // Internal Timing
    float FPS = 0.0f;
    uint32_t FrameCount = 0;

    // Data
    // We keep the dictionary internally to resolve IDs to names, 
    // even if we don't display the table.
    std::vector<CLipSyncPhonemeRef> Dictionary;
    std::vector<CLipSyncFrame> Frames;

    bool IsParsed = false;
};

class CLipSyncParser {
public:
    CLipSyncData Data;
    bool IsParsed = false;

    void Parse(const std::vector<uint8_t>& rawData, const std::vector<uint8_t>& infoData) {
        Data = CLipSyncData();
        Data.IsParsed = false;
        IsParsed = false;

        // 1. Parse Duration from Metadata (if available)
        if (infoData.size() >= 4) {
            Data.Duration = *(float*)infoData.data();
        }

        if (rawData.empty()) return;

        size_t cursor = 0;
        size_t size = rawData.size();
        const uint8_t* ptr = rawData.data();

        // 2. Read Dictionary Header
        if (cursor + 4 > size) return;
        uint32_t dictCount = *(uint32_t*)(ptr + cursor);
        cursor += 4;

        for (uint32_t i = 0; i < dictCount; i++) {
            if (cursor + 1 > size) break;
            CLipSyncPhonemeRef item;
            item.ID = *(uint8_t*)(ptr + cursor); cursor++;

            // Read Null-Term String
            while (cursor < size) {
                char c = (char)ptr[cursor++];
                if (c == 0) break;
                item.Symbol += c;
            }
            Data.Dictionary.push_back(item);
        }

        // 3. Read Timing Header
        if (cursor + 8 > size) return;

        // Assembly reads 4 bytes into float/int
        uint32_t fpsRaw = *(uint32_t*)(ptr + cursor); cursor += 4;
        Data.FPS = (float)fpsRaw;

        Data.FrameCount = *(uint32_t*)(ptr + cursor); cursor += 4;

        // Fallback duration calc
        if (Data.Duration == 0.0f && Data.FPS > 0.0f) {
            Data.Duration = (float)Data.FrameCount / Data.FPS;
        }

        // 4. Read Frame Data
        // Assembly Logic: Loop FrameCount times -> Read KeyCount -> Loop KeyCount times
        for (uint32_t i = 0; i < Data.FrameCount; i++) {
            if (cursor + 1 > size) break;

            CLipSyncFrame frame;
            uint8_t keyCount = ptr[cursor++];

            for (uint8_t k = 0; k < keyCount; k++) {
                if (cursor + 2 > size) break;
                CLipSyncFrameKey key;
                key.ID = ptr[cursor++];
                key.WeightByte = ptr[cursor++];
                key.WeightFloat = key.WeightByte / 255.0f;
                frame.Keys.push_back(key);
            }
            Data.Frames.push_back(frame);
        }

        Data.IsParsed = true;
        IsParsed = true;
    }
};