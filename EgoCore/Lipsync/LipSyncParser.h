#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cmath>

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
    std::vector<CLipSyncPhonemeRef> Dictionary;
    std::vector<CLipSyncFrame> Frames;

    bool IsParsed = false;
};

class CLipSyncParser {
public:
    CLipSyncData Data;
    bool IsParsed = false;

    // --- NEW: Generate a blank entry from scratch ---
    struct GeneratedBlob {
        std::vector<uint8_t> Raw;
        std::vector<uint8_t> Info;
    };

    static GeneratedBlob GenerateEmpty(float duration = 1.0f) {
        GeneratedBlob result;

        // 1. Info Data (Just duration float)
        result.Info.resize(4);
        memcpy(result.Info.data(), &duration, 4);

        // 2. Raw Data Construction
        // Constants matching vanilla engine
        float fpsVal = 22050.0f / 512.0f; // ~43.066f
        uint32_t frameCount = (uint32_t)std::ceil(duration * fpsVal);

        // A. Dictionary Header (Count = 0)
        uint32_t dictCount = 0;
        result.Raw.insert(result.Raw.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);

        // B. Timing Header
        uint32_t fpsInt = (uint32_t)fpsVal;
        result.Raw.insert(result.Raw.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);
        result.Raw.insert(result.Raw.end(), (uint8_t*)&frameCount, (uint8_t*)&frameCount + 4);

        // C. Frames
        for (uint32_t i = 0; i < frameCount; i++) {
            result.Raw.push_back(0); // KeyCount = 0
        }

        return result;
    }

    // --- NEW: Serialize Data back to bytes ---
    std::vector<uint8_t> Recompile() {
        std::vector<uint8_t> out;

        // 1. Dictionary
        uint32_t dictCount = (uint32_t)Data.Dictionary.size();
        out.insert(out.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);

        for (const auto& item : Data.Dictionary) {
            out.push_back(item.ID);
            // Null-terminated string
            for (char c : item.Symbol) out.push_back((uint8_t)c);
            out.push_back(0);
        }

        // 2. Timing
        uint32_t fpsInt = (uint32_t)Data.FPS;
        if (fpsInt == 0) fpsInt = 43; // Safety default
        out.insert(out.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);

        uint32_t frameCount = (uint32_t)Data.Frames.size();
        out.insert(out.end(), (uint8_t*)&frameCount, (uint8_t*)&frameCount + 4);

        // 3. Frames
        for (const auto& frame : Data.Frames) {
            uint8_t keyCount = (uint8_t)frame.Keys.size();
            out.push_back(keyCount);
            for (const auto& key : frame.Keys) {
                out.push_back(key.ID);
                out.push_back(key.WeightByte);
            }
        }

        return out;
    }

    void Parse(const std::vector<uint8_t>& rawData, const std::vector<uint8_t>& infoData) {
        Data = CLipSyncData();
        Data.IsParsed = false;
        IsParsed = false;

        if (infoData.size() >= 4) {
            Data.Duration = *(float*)infoData.data();
        }

        if (rawData.empty()) return;

        size_t cursor = 0;
        size_t size = rawData.size();
        const uint8_t* ptr = rawData.data();

        if (cursor + 4 > size) return;
        uint32_t dictCount = *(uint32_t*)(ptr + cursor);
        cursor += 4;

        for (uint32_t i = 0; i < dictCount; i++) {
            if (cursor + 1 > size) break;
            CLipSyncPhonemeRef item;
            item.ID = *(uint8_t*)(ptr + cursor); cursor++;
            while (cursor < size) {
                char c = (char)ptr[cursor++];
                if (c == 0) break;
                item.Symbol += c;
            }
            Data.Dictionary.push_back(item);
        }

        if (cursor + 8 > size) return;
        uint32_t fpsRaw = *(uint32_t*)(ptr + cursor); cursor += 4;
        Data.FPS = (float)fpsRaw;
        Data.FrameCount = *(uint32_t*)(ptr + cursor); cursor += 4;

        if (Data.Duration == 0.0f && Data.FPS > 0.0f) {
            Data.Duration = (float)Data.FrameCount / Data.FPS;
        }

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