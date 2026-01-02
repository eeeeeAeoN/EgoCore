#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include "miniaudio.h"

// --- ENCODING TABLES ---
static const int16_t StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29800, 32767
};

static const int IndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// --- WAV WRITER (ADDED FIX) ---
static void WriteWavFile(const std::string& path, const std::vector<int16_t>& pcm, int sampleRate, int channels) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return;

    int byteRate = sampleRate * channels * 2;
    int blockAlign = channels * 2;
    int dataSize = (int)pcm.size() * 2;
    int chunkSize = 36 + dataSize;

    f.write("RIFF", 4);
    f.write((char*)&chunkSize, 4);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1; // PCM
    short numChannels = (short)channels;
    int sRate = sampleRate;
    short bAlign = (short)blockAlign;
    short bitsPerSample = 16;

    f.write((char*)&subchunk1Size, 4);
    f.write((char*)&audioFormat, 2);
    f.write((char*)&numChannels, 2);
    f.write((char*)&sRate, 4);
    f.write((char*)&byteRate, 4);
    f.write((char*)&bAlign, 2);
    f.write((char*)&bitsPerSample, 2);

    f.write("data", 4);
    f.write((char*)&dataSize, 4);
    f.write((char*)pcm.data(), dataSize);
    f.close();
}

// --- ADPCM CLASSES ---
class XboxAdpcmEncoder {
    struct State {
        int16_t sample = 0;
        uint8_t index = 0;
    };

    static uint8_t EncodeNibble(State& state, int16_t target) {
        int32_t delta = target;
        delta -= state.sample;

        uint8_t nibble = 0;
        if (delta < 0) { delta = -delta; nibble |= 8; }

        uint16_t step = StepTable[state.index];
        int32_t diff = step >> 3;

        for (uint8_t i = 0; i < 3; i++) {
            if (delta > step) {
                delta -= step; diff += step; nibble |= (4 >> i);
            }
            step >>= 1;
        }

        if (nibble & 8) diff = -diff;

        state.sample = (int16_t)(std::clamp)((int32_t)state.sample + diff, -32768, 32767);
        state.index = (uint8_t)(std::clamp)((int)state.index + IndexTable[nibble], 0, 88);

        return nibble;
    }

public:
    static std::vector<uint8_t> Encode(const std::vector<int16_t>& pcm) {
        std::vector<uint8_t> out;
        if (pcm.empty()) return out;

        const int SamplesPerBlock = 65;
        size_t totalSamples = pcm.size();
        size_t numBlocks = (totalSamples + SamplesPerBlock - 1) / SamplesPerBlock;

        out.reserve(numBlocks * 36);

        State state;
        size_t cursor = 0;

        for (size_t b = 0; b < numBlocks; b++) {
            int16_t headerSample = (cursor < totalSamples) ? pcm[cursor] : 0;
            state.sample = headerSample;
            state.index = (b == 0) ? 0 : state.index;

            out.push_back(state.sample & 0xFF);
            out.push_back((state.sample >> 8) & 0xFF);
            out.push_back(state.index);
            out.push_back(0);

            cursor++;

            for (int i = 0; i < 32; i++) {
                int16_t s1 = (cursor < totalSamples) ? pcm[cursor++] : 0;
                int16_t s2 = (cursor < totalSamples) ? pcm[cursor++] : 0;
                uint8_t n1 = EncodeNibble(state, s1);
                uint8_t n2 = EncodeNibble(state, s2);
                out.push_back(n1 | (n2 << 4));
            }
        }
        return out;
    }
};

class XboxAdpcmDecoder {
public:
    static std::vector<int16_t> Decode(const std::vector<uint8_t>& adpcmData) {
        std::vector<int16_t> pcm;
        if (adpcmData.empty()) return pcm;

        int blockSize = 36;
        int numBlocks = (int)adpcmData.size() / blockSize;
        pcm.reserve(numBlocks * 65);

        for (int b = 0; b < numBlocks; b++) {
            const uint8_t* block = &adpcmData[b * blockSize];

            int16_t predictor = (int16_t)(block[0] | (block[1] << 8));
            uint16_t stepIndex = (uint16_t)(block[2]);
            stepIndex = (std::clamp)((int)stepIndex, 0, 88);

            int32_t currentVal = predictor;
            pcm.push_back((int16_t)currentVal);

            for (int i = 4; i < 36; i++) {
                uint8_t byte = block[i];
                for (int nibble = 0; nibble < 2; nibble++) {
                    int step = StepTable[stepIndex];
                    int delta = (nibble == 0) ? (byte & 0x0F) : ((byte >> 4) & 0x0F);

                    int diff = step >> 3;
                    if (delta & 4) diff += step;
                    if (delta & 2) diff += step >> 1;
                    if (delta & 1) diff += step >> 2;
                    if (delta & 8) currentVal -= diff; else currentVal += diff;

                    currentVal = (std::clamp)(currentVal, -32768, 32767);
                    pcm.push_back((int16_t)currentVal);

                    stepIndex = (std::clamp)((int)stepIndex + IndexTable[delta], 0, 88);
                }
            }
        }
        return pcm;
    }
};

// --- AUDIO PLAYER ---
class AudioPlayer {
    ma_device device;
    bool isInit = false;
    std::vector<int16_t> activeBuffer;
    std::atomic<size_t> playCursor = 0;
    int currentSampleRate = 22050;

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        AudioPlayer* player = (AudioPlayer*)pDevice->pUserData;
        if (!player) return;
        int16_t* out = (int16_t*)pOutput;
        size_t cursor = player->playCursor.load();
        size_t total = player->activeBuffer.size();
        size_t available = (total > cursor) ? (total - cursor) : 0;
        size_t toCopy = (std::min)((size_t)frameCount, available);

        if (toCopy > 0) {
            memcpy(out, &player->activeBuffer[cursor], toCopy * sizeof(int16_t));
            player->playCursor.store(cursor + toCopy);
        }
        if (toCopy < frameCount) {
            memset(out + toCopy, 0, (frameCount - toCopy) * sizeof(int16_t));
        }
    }

public:
    AudioPlayer() { memset(&device, 0, sizeof(device)); }
    ~AudioPlayer() { Reset(); }

    void Reset() {
        if (isInit) { ma_device_uninit(&device); isInit = false; }
        activeBuffer.clear(); playCursor = 0;
    }

    void PlayPCM(const std::vector<int16_t>& pcm, int sampleRate) {
        Reset();
        activeBuffer = pcm;
        currentSampleRate = sampleRate;
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_s16;
        config.playback.channels = 1;
        config.sampleRate = sampleRate;
        config.dataCallback = data_callback;
        config.pUserData = this;
        if (ma_device_init(NULL, &config, &device) == MA_SUCCESS) {
            isInit = true;
            ma_device_start(&device);
        }
    }

    void Play() { if (isInit) ma_device_start(&device); }
    void Pause() { if (isInit) ma_device_stop(&device); }
    bool IsPlaying() { return isInit && ma_device_is_started(&device); }
    float GetProgress() { return activeBuffer.empty() ? 0.0f : (float)playCursor / activeBuffer.size(); }
    float GetTotalDuration() { return activeBuffer.empty() ? 0.0f : (float)activeBuffer.size() / currentSampleRate; }
    float GetCurrentTime() { return activeBuffer.empty() ? 0.0f : (float)playCursor / currentSampleRate; }
    void Seek(float p) { if (!activeBuffer.empty()) playCursor = (size_t)(p * activeBuffer.size()); }
};