#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include "miniaudio.h"

// =========================================================
// 1. FABLE FILE STRUCTURES
// =========================================================
#pragma pack(push, 1)

struct LHAudioBankCompData {
    char     Signature[32];
    uint32_t Unk1;
    uint32_t Unk2;
    uint32_t TotalAudioSize;
};

struct LHAudioBankLookupTable {
    char     Signature[32];
    uint32_t BankID;
    uint32_t BankFlags;
    uint32_t NumEntries;
};

struct AudioLookupEntry {
    uint32_t SoundID;
    uint32_t Length;
    uint32_t Offset;
};

#pragma pack(pop)

// =========================================================
// 2. XBOX ADPCM DECODER
// =========================================================
class XboxAdpcmDecoder {
    static const int16_t StepTable[89];
public:
    static std::vector<int16_t> Decode(const std::vector<uint8_t>& adpcmData, int channels) {
        std::vector<int16_t> pcm;
        if (adpcmData.empty()) return pcm;

        int blockSize = 36 * channels;
        int numBlocks = (int)adpcmData.size() / blockSize;

        pcm.reserve(numBlocks * 64 * channels);

        for (int b = 0; b < numBlocks; b++) {
            for (int ch = 0; ch < channels; ch++) {
                const uint8_t* block = &adpcmData[b * blockSize + ch * 36];

                int16_t predictor = (int16_t)(block[0] | (block[1] << 8));
                uint16_t stepIndex = (uint16_t)(block[2] | (block[3] << 8));

                // Fix min/max macro collision
                stepIndex = (std::clamp)((int)stepIndex, 0, 88);

                int32_t currentVal = predictor;

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

                        static const int IndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
                        stepIndex = (std::clamp)(stepIndex + IndexTable[delta], 0, 88);
                    }
                }
            }
        }
        return pcm;
    }
};

const int16_t XboxAdpcmDecoder::StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29800, 32767
};

// =========================================================
// 3. AUDIO PLAYER (Miniaudio Wrapper)
// =========================================================
class AudioPlayer {
    ma_device device;
    bool isInit = false;
    std::vector<int16_t> activeBuffer;

    // We use atomic so the UI thread can read position while Audio thread writes it
    std::atomic<size_t> playCursor = 0;

    int currentSampleRate = 22050;

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        AudioPlayer* player = (AudioPlayer*)pDevice->pUserData;
        if (!player) return;

        int16_t* out = (int16_t*)pOutput;
        size_t cursor = player->playCursor.load();
        size_t total = player->activeBuffer.size();

        size_t samplesNeeded = frameCount;
        size_t samplesAvailable = (total > cursor) ? (total - cursor) : 0;
        size_t toCopy = (std::min)(samplesNeeded, samplesAvailable);

        if (toCopy > 0) {
            memcpy(out, &player->activeBuffer[cursor], toCopy * sizeof(int16_t));
            player->playCursor.store(cursor + toCopy);
        }

        // Fill remainder with silence if we hit end
        if (toCopy < samplesNeeded) {
            memset(out + toCopy, 0, (samplesNeeded - toCopy) * sizeof(int16_t));
            // Optional: Auto-stop or loop could go here
        }
    }

public:
    AudioPlayer() { memset(&device, 0, sizeof(device)); }
    ~AudioPlayer() { if (isInit) ma_device_uninit(&device); }

    void LoadPCM(const std::vector<int16_t>& pcm, int sampleRate) {
        // Stop old playback
        if (isInit) {
            ma_device_uninit(&device);
            isInit = false;
        }

        activeBuffer = pcm;
        playCursor = 0;
        currentSampleRate = sampleRate;

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_s16;
        config.playback.channels = 1;
        config.sampleRate = sampleRate;
        config.dataCallback = data_callback;
        config.pUserData = this;

        if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return;
        isInit = true;
    }

    void Play() {
        if (isInit && !ma_device_is_started(&device)) {
            ma_device_start(&device);
        }
    }

    void Pause() {
        if (isInit && ma_device_is_started(&device)) {
            ma_device_stop(&device);
        }
    }

    void Toggle() {
        if (isInit) {
            if (ma_device_is_started(&device)) Pause();
            else Play();
        }
    }

    bool IsPlaying() {
        return isInit && ma_device_is_started(&device);
    }

    // Returns progress 0.0 -> 1.0
    float GetProgress() {
        if (activeBuffer.empty()) return 0.0f;
        return (float)playCursor.load() / (float)activeBuffer.size();
    }

    // Returns current time in seconds
    float GetCurrentTime() {
        return (float)playCursor.load() / (float)currentSampleRate;
    }

    // Returns total duration in seconds
    float GetTotalDuration() {
        if (activeBuffer.empty()) return 0.0f;
        return (float)activeBuffer.size() / (float)currentSampleRate;
    }

    // Seek to 0.0 -> 1.0
    void Seek(float progress) {
        if (activeBuffer.empty()) return;
        size_t newPos = (size_t)(progress * activeBuffer.size());
        playCursor.store(newPos);
    }

    void Reset() {
        if (isInit) {
            ma_device_uninit(&device);
            isInit = false;
        }
        activeBuffer.clear();
        playCursor = 0;
    }
};

// =========================================================
// 4. BANK PARSER
// =========================================================
class AudioBankParser {
public:
    std::string FileName;
    std::fstream Stream;
    uint32_t AudioBlobStart = 44;
    uint32_t AudioBlobSize = 0;
    std::vector<AudioLookupEntry> Entries;
    bool IsLoaded = false;

    // The player instance for this bank
    AudioPlayer Player;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        LHAudioBankCompData mainHeader;
        Stream.read((char*)&mainHeader, sizeof(LHAudioBankCompData));
        AudioBlobSize = mainHeader.TotalAudioSize;

        uint32_t tableOffset = 44 + AudioBlobSize;
        Stream.seekg(tableOffset, std::ios::beg);

        LHAudioBankLookupTable tableHeader;
        Stream.read((char*)&tableHeader, sizeof(LHAudioBankLookupTable));

        Entries.clear();
        for (uint32_t i = 0; i < tableHeader.NumEntries; i++) {
            AudioLookupEntry entry;
            Stream.read((char*)&entry, sizeof(AudioLookupEntry));
            Entries.push_back(entry);
        }

        IsLoaded = true;
        return true;
    }

    std::vector<int16_t> GetDecodedAudio(int index) {
        if (index < 0 || index >= Entries.size()) return {};

        const auto& e = Entries[index];
        std::vector<uint8_t> rawBuffer(e.Length);

        Stream.clear();
        Stream.seekg(AudioBlobStart + e.Offset, std::ios::beg);
        Stream.read((char*)rawBuffer.data(), e.Length);

        int riffStart = -1;
        for (size_t i = 0; i < rawBuffer.size() - 4; i++) {
            if (rawBuffer[i] == 'R' && rawBuffer[i + 1] == 'I' && rawBuffer[i + 2] == 'F' && rawBuffer[i + 3] == 'F') {
                riffStart = i; break;
            }
        }

        if (riffStart == -1) return {};

        int dataStart = -1;
        for (size_t i = riffStart; i < rawBuffer.size() - 4; i++) {
            if (rawBuffer[i] == 'd' && rawBuffer[i + 1] == 'a' && rawBuffer[i + 2] == 't' && rawBuffer[i + 3] == 'a') {
                dataStart = i + 8;
                break;
            }
        }

        if (dataStart == -1) return {};

        std::vector<uint8_t> adpcmData(rawBuffer.begin() + dataStart, rawBuffer.end());
        return XboxAdpcmDecoder::Decode(adpcmData, 1);
    }
};