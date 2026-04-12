#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

struct CLipSyncPhonemeRef {
    uint8_t ID;
    std::string Symbol;
};

struct CLipSyncFrameKey {
    uint8_t ID;
    uint8_t WeightByte;
    float WeightFloat;
};

struct CLipSyncFrame {
    std::vector<CLipSyncFrameKey> Keys;
};

struct CLipSyncData {
    float Duration = 0.0f;

    float FPS = 0.0f;
    uint32_t FrameCount = 0;

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

    static GeneratedBlob GenerateEmpty(float duration = 1.0f) {
        GeneratedBlob result;

        result.Info.resize(4);
        memcpy(result.Info.data(), &duration, 4);

        float fpsVal = 22050.0f / 512.0f;
        uint32_t frameCount = (uint32_t)std::ceil(duration * fpsVal);

        std::vector<std::string> defaults = { "AH", "EE", "MM", "OH", "SZ" };
        uint32_t dictCount = (uint32_t)defaults.size();

        result.Raw.insert(result.Raw.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);
        for (uint8_t i = 0; i < dictCount; i++) {
            result.Raw.push_back(i);
            for (char c : defaults[i]) result.Raw.push_back((uint8_t)c);
            result.Raw.push_back(0);
        }

        uint32_t fpsInt = (uint32_t)fpsVal;
        result.Raw.insert(result.Raw.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);
        result.Raw.insert(result.Raw.end(), (uint8_t*)&frameCount, (uint8_t*)&frameCount + 4);

        for (uint32_t i = 0; i < frameCount; i++) {
            result.Raw.push_back(0);
        }

        return result;
    }

    std::vector<uint8_t> Recompile() {
        std::vector<uint8_t> out;

        std::sort(Data.Dictionary.begin(), Data.Dictionary.end(),
            [](const CLipSyncPhonemeRef& a, const CLipSyncPhonemeRef& b) {
                return a.ID < b.ID;
            });

        uint32_t dictCount = (uint32_t)Data.Dictionary.size();
        out.insert(out.end(), (uint8_t*)&dictCount, (uint8_t*)&dictCount + 4);

        for (const auto& item : Data.Dictionary) {
            out.push_back(item.ID);
            for (char c : item.Symbol) out.push_back((uint8_t)c);
            out.push_back(0);
        }

        uint32_t fpsInt = (uint32_t)Data.FPS;
        out.insert(out.end(), (uint8_t*)&fpsInt, (uint8_t*)&fpsInt + 4);
        out.insert(out.end(), (uint8_t*)&Data.FrameCount, (uint8_t*)&Data.FrameCount + 4);

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
        Data = CLipSyncData();
        Data.IsParsed = false;
        IsParsed = false;

        if (rawData.size() < 4) return;
        const uint8_t* ptr = rawData.data();
        size_t size = rawData.size();
        size_t cursor = 0;

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

        if (Data.FPS > 0.0f) {
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
                key.WeightFloat = (float)key.WeightByte / 255.0f;
                frame.Keys.push_back(key);
            }
            Data.Frames.push_back(frame);
        }

        Data.IsParsed = true;
        IsParsed = true;
    }
};