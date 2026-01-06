#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm> // Added for std::sort

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

    struct GeneratedBlob {
        std::vector<uint8_t> Raw;
        std::vector<uint8_t> Info;
    };

    // --- FIX: Generate Correct 4-Byte Info + Sorted Raw Data ---
    static GeneratedBlob GenerateEmpty(float duration = 1.0f) {
        GeneratedBlob result;

        // 1. Info Data: Strictly 4 bytes for Duration (Matches Vanilla)
        result.Info.resize(4);
        memcpy(result.Info.data(), &duration, 4);

        // 2. Raw Data Construction
        float fpsVal = 22050.0f / 512.0f; // ~43.06 FPS
        uint32_t frameCount = (uint32_t)std::ceil(duration * fpsVal);

        // A. Dictionary (Standard Fable Phonemes - SORTED IDs 0-4)
        std::vector<std::string> defaults = { "AH", "EE", "MM", "OH", "SZ" };
        uint32_t dictCount = (uint32_t)defaults.size();

        result.Raw.insert(result.Raw.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);
        for (uint8_t i = 0; i < dictCount; i++) {
            result.Raw.push_back(i); // ID
            for (char c : defaults[i]) result.Raw.push_back((uint8_t)c);
            result.Raw.push_back(0); // Null Terminator
        }

        // B. Timing Header
        uint32_t fpsInt = (uint32_t)fpsVal;
        result.Raw.insert(result.Raw.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);
        result.Raw.insert(result.Raw.end(), (uint8_t*)&frameCount, (uint8_t*)&frameCount + 4);

        // C. Frames (Empty, but valid)
        for (uint32_t i = 0; i < frameCount; i++) {
            result.Raw.push_back(0); // KeyCount = 0
        }

        return result;
    }

    // --- FIX: Sort Dictionary before writing (REQUIRED FOR ANIMATION) ---
    std::vector<uint8_t> Recompile() {
        std::vector<uint8_t> out;

        // CRITICAL: Engine requires Dictionary sorted by ID.
        std::sort(Data.Dictionary.begin(), Data.Dictionary.end(),
            [](const CLipSyncPhonemeRef& a, const CLipSyncPhonemeRef& b) {
                return a.ID < b.ID;
            });

        // 1. Dictionary
        uint32_t dictCount = (uint32_t)Data.Dictionary.size();
        out.insert(out.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);

        for (const auto& item : Data.Dictionary) {
            out.push_back(item.ID);
            for (char c : item.Symbol) out.push_back((uint8_t)c);
            out.push_back(0);
        }

        // 2. Timing
        uint32_t fpsInt = (uint32_t)Data.FPS;
        out.insert(out.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);
        out.insert(out.end(), (uint8_t*)&Data.FrameCount, (uint8_t*)&Data.FrameCount + 4);

        // 3. Frames
        for (const auto& frame : Data.Frames) {
            if (frame.Keys.size() > 255) {
                out.push_back(255);
            }
            else {
                out.push_back((uint8_t)frame.Keys.size());
            }

            for (const auto& key : frame.Keys) {
                out.push_back(key.ID);
                uint8_t weightByte = (uint8_t)(key.WeightFloat * 255.0f);
                out.push_back(weightByte);
            }
        }

        return out;
    }

    void Parse(const std::vector<uint8_t>& rawData, const std::vector<uint8_t>& infoData) {
        Data = CLipSyncData(); // Reset
        Data.IsParsed = false;

        if (rawData.size() < 4) return;
        const uint8_t* ptr = rawData.data();
        size_t size = rawData.size();
        size_t cursor = 0;

        // 1. Dictionary
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

        // 2. Timing
        if (cursor + 8 > size) return;
        uint32_t fpsRaw = *(uint32_t*)(ptr + cursor); cursor += 4;
        Data.FPS = (float)fpsRaw;
        Data.FrameCount = *(uint32_t*)(ptr + cursor); cursor += 4;

        // 3. Duration Calc (Ignore Info block float if parsing raw)
        if (Data.FPS > 0.0f) {
            Data.Duration = (float)Data.FrameCount / Data.FPS;
        }

        // 4. Frames
        for (uint32_t i = 0; i < Data.FrameCount; i++) {
            if (cursor + 1 > size) break;
            CLipSyncFrame frame;
            uint8_t keyCount = ptr[cursor++];
            for (uint8_t k = 0; k < keyCount; k++) {
                if (cursor + 2 > size) break;
                CLipSyncFrameKey key;
                key.ID = ptr[cursor++];
                key.WeightByte = ptr[cursor++];
                key.WeightFloat = (float)key.WeightByte / 255.0f;
                frame.Keys.push_back(key);
            }
            Data.Frames.push_back(frame);
        }

        Data.IsParsed = true;
    }
};