#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <cstring> 
#include <filesystem> 

#include "miniaudio.h"

// ... (Structs 1-4 remain unchanged: LHAudioBankCompData, AdpcmEncoder, etc.) ...
// ... Copy previous content for sections 1, 2, 3, 4 ...

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
// 2. ADPCM SHARED TABLES
// =========================================================
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

// =========================================================
// 3. XBOX ADPCM ENCODER
// =========================================================
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

// =========================================================
// 4. XBOX ADPCM DECODER
// =========================================================
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

// =========================================================
// 5. AUDIO PLAYER
// =========================================================
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

// =========================================================
// 6. BANK PARSER
// =========================================================
class AudioBankParser {
public:
    std::string FileName;
    std::fstream Stream;
    uint32_t AudioBlobStart = 44;

    LHAudioBankCompData MainHeader;
    LHAudioBankLookupTable TableHeader;
    std::vector<uint8_t> FooterData;

    std::vector<AudioLookupEntry> Entries;
    bool IsLoaded = false;
    AudioPlayer Player;

    std::map<int, std::vector<uint8_t>> ModifiedCache;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        Stream.read((char*)&MainHeader, sizeof(LHAudioBankCompData));
        AudioBlobStart = sizeof(LHAudioBankCompData);

        uint32_t tableOffset = AudioBlobStart + MainHeader.TotalAudioSize;
        Stream.seekg(tableOffset, std::ios::beg);

        Stream.read((char*)&TableHeader, sizeof(LHAudioBankLookupTable));

        Entries.clear();
        for (uint32_t i = 0; i < TableHeader.NumEntries; i++) {
            AudioLookupEntry entry;
            Stream.read((char*)&entry, sizeof(AudioLookupEntry));
            Entries.push_back(entry);
        }

        std::streampos currentPos = Stream.tellg();
        Stream.seekg(0, std::ios::end);
        std::streampos endPos = Stream.tellg();
        size_t footerSize = (size_t)(endPos - currentPos);

        if (footerSize > 0) {
            Stream.seekg(currentPos, std::ios::beg);
            FooterData.resize(footerSize);
            Stream.read((char*)FooterData.data(), footerSize);
        }
        else FooterData.clear();

        IsLoaded = true;
        ModifiedCache.clear();
        return true;
    }

    void DeleteEntry(int index) {
        if (index < 0 || index >= (int)Entries.size()) return;
        Entries.erase(Entries.begin() + index);
        std::map<int, std::vector<uint8_t>> newCache;
        for (auto& [k, v] : ModifiedCache) {
            if (k < index) newCache[k] = v;
            else if (k > index) newCache[k - 1] = v;
        }
        ModifiedCache = newCache;
    }

    // --- NEW: Helper to get duration of external WAV ---
    static float GetWavDuration(const std::string& wavPath) {
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return 0.0f;

        f.seekg(0, std::ios::end);
        size_t fileSize = f.tellg();
        f.seekg(0, std::ios::beg);

        std::vector<uint8_t> header(44); // standard header
        if (fileSize < 44) return 0.0f;
        f.read((char*)header.data(), 44);
        f.close();

        if (memcmp(&header[0], "RIFF", 4) != 0) return 0.0f;

        // Offset 24: Sample Rate (4 bytes)
        // Offset 28: Byte Rate (4 bytes)
        // Offset 40: Data Chunk Size (4 bytes)

        // NOTE: This assumes standard WAV header. More robust parsing in ImportWav used, 
        // but for quick duration check this is usually enough.
        uint32_t sampleRate = *(uint32_t*)&header[24];
        uint32_t byteRate = *(uint32_t*)&header[28];

        // If byteRate is 0, calc from sampleRate * channels * bits/8
        if (sampleRate == 0) return 0.0f;

        // Try scanning chunks like ImportWav for accurate duration
        // Re-open and scan properly
        f.open(wavPath, std::ios::binary);
        f.seekg(12, std::ios::beg);

        size_t cursor = 12;
        uint32_t pcmBytes = 0;
        uint32_t rate = 22050;
        uint16_t bits = 16;
        uint16_t channels = 1;

        while (cursor < fileSize - 8) {
            uint32_t chunkId = 0, chunkSize = 0;
            f.read((char*)&chunkId, 4);
            f.read((char*)&chunkSize, 4);

            if (memcmp(&chunkId, "fmt ", 4) == 0) {
                f.seekg(2, std::ios::cur);
                f.read((char*)&channels, 2);
                f.read((char*)&rate, 4);
                f.seekg(6, std::ios::cur);
                f.read((char*)&bits, 2);
                f.seekg(chunkSize - 16, std::ios::cur); // skip rest of fmt
            }
            else if (memcmp(&chunkId, "data", 4) == 0) {
                pcmBytes = chunkSize;
                break; // found data
            }
            else {
                f.seekg(chunkSize, std::ios::cur);
            }
            cursor = (size_t)f.tellg();
        }

        if (pcmBytes == 0 || rate == 0 || channels == 0 || bits == 0) return 0.0f;

        float bytesPerSample = (bits / 8.0f) * channels;
        float totalSamples = pcmBytes / bytesPerSample;
        return totalSamples / rate;
    }

    bool AddEntry(uint32_t id, const std::string& wavPath) {
        AudioLookupEntry newE;
        newE.SoundID = id;
        newE.Length = 0;
        newE.Offset = 0;

        Entries.push_back(newE);
        int idx = (int)Entries.size() - 1;

        return ImportWav(idx, wavPath);
    }

    std::vector<int16_t> GetDecodedAudio(int index) {
        if (index < 0 || index >= Entries.size()) return {};
        std::vector<uint8_t> rawBuffer;

        if (ModifiedCache.count(index)) {
            rawBuffer = ModifiedCache[index];
        }
        else {
            const auto& e = Entries[index];
            rawBuffer.resize(e.Length);
            Stream.clear();
            Stream.seekg(AudioBlobStart + e.Offset, std::ios::beg);
            Stream.read((char*)rawBuffer.data(), e.Length);
        }

        int dataStart = -1;
        for (size_t i = 0; i < rawBuffer.size() - 8; i++) {
            if (memcmp(&rawBuffer[i], "data", 4) == 0) { dataStart = i + 8; break; }
        }
        if (dataStart == -1) return {};

        std::vector<uint8_t> adpcmData(rawBuffer.begin() + dataStart, rawBuffer.end());
        return XboxAdpcmDecoder::Decode(adpcmData);
    }

    bool ImportWav(int index, const std::string& wavPath) {
        if (index < 0 || index >= Entries.size()) return false;

        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return false;

        f.seekg(0, std::ios::end);
        size_t fileSize = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<uint8_t> fileData(fileSize);
        f.read((char*)fileData.data(), fileSize);
        f.close();

        if (fileSize < 12 || memcmp(&fileData[0], "RIFF", 4) != 0 || memcmp(&fileData[8], "WAVE", 4) != 0) return false;

        uint16_t numChannels = 0;
        uint32_t sampleRate = 22050;
        uint16_t bitsPerSample = 0;
        const uint8_t* pcmStart = nullptr;
        size_t pcmBytes = 0;

        size_t cursor = 12;
        while (cursor < fileSize - 8) {
            uint32_t chunkId = 0, chunkSize = 0;
            memcpy(&chunkId, &fileData[cursor], 4);
            memcpy(&chunkSize, &fileData[cursor + 4], 4);
            size_t chunkDataPos = cursor + 8;

            if (memcmp(&chunkId, "fmt ", 4) == 0) {
                memcpy(&numChannels, &fileData[chunkDataPos + 2], 2);
                memcpy(&sampleRate, &fileData[chunkDataPos + 4], 4);
                memcpy(&bitsPerSample, &fileData[chunkDataPos + 14], 2);
            }
            else if (memcmp(&chunkId, "data", 4) == 0) {
                pcmStart = &fileData[chunkDataPos];
                pcmBytes = chunkSize;
            }
            cursor += 8 + chunkSize;
        }

        if (!pcmStart || pcmBytes == 0 || numChannels == 0) return false;

        std::vector<int16_t> finalPcm;
        if (bitsPerSample == 16) {
            size_t numSamples = pcmBytes / 2;
            const int16_t* src = (const int16_t*)pcmStart;
            if (numChannels == 1) finalPcm.assign(src, src + numSamples);
            else if (numChannels == 2) {
                size_t numFrames = numSamples / 2;
                finalPcm.reserve(numFrames);
                for (size_t i = 0; i < numFrames; i++) finalPcm.push_back(src[i * 2]);
            }
        }
        else return false;

        std::vector<uint8_t> encoded = XboxAdpcmEncoder::Encode(finalPcm);

        uint32_t riffSize = 4 + 28 + 8 + (uint32_t)encoded.size();

        std::vector<uint8_t> riffFile;
        riffFile.reserve(riffSize + 8);

        auto pushU32 = [&](uint32_t v) { riffFile.insert(riffFile.end(), { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) }); };
        auto pushU16 = [&](uint16_t v) { riffFile.insert(riffFile.end(), { (uint8_t)v, (uint8_t)(v >> 8) }); };
        auto pushStr = [&](const char* s) { riffFile.insert(riffFile.end(), s, s + 4); };

        pushStr("RIFF"); pushU32(riffSize); pushStr("WAVE");
        pushStr("fmt "); pushU32(20); pushU16(0x0069); pushU16(1);
        pushU32(sampleRate);
        pushU32((sampleRate * 36) / 64);
        pushU16(36); pushU16(4); pushU16(2); pushU16(64);

        pushStr("data"); pushU32((uint32_t)encoded.size());
        riffFile.insert(riffFile.end(), encoded.begin(), encoded.end());

        std::vector<uint8_t> finalBlob;
        const auto& originalEntry = Entries[index];
        Stream.clear();
        Stream.seekg(AudioBlobStart + originalEntry.Offset, std::ios::beg);

        std::vector<uint8_t> headerProbe(64);
        if (originalEntry.Length > 64) Stream.read((char*)headerProbe.data(), 64);

        int riffStart = -1;
        for (int i = 0; i < 60; i++) if (memcmp(&headerProbe[i], "RIFF", 4) == 0) { riffStart = i; break; }

        if (riffStart > 0) {
            finalBlob.resize(riffStart);
            memcpy(finalBlob.data(), headerProbe.data(), riffStart);

            if (riffStart >= 12) {
                uint16_t* pRate = (uint16_t*)&finalBlob[6];
                *pRate = (uint16_t)sampleRate;
                uint32_t* pSize = (uint32_t*)&finalBlob[8];
                *pSize = (uint32_t)riffFile.size();
            }
        }
        else {
            finalBlob.resize(30, 0);
            memcpy(finalBlob.data(), &originalEntry.SoundID, 4);
        }

        finalBlob.insert(finalBlob.end(), riffFile.begin(), riffFile.end());

        Entries[index].Length = (uint32_t)finalBlob.size();

        ModifiedCache[index] = finalBlob;
        return true;
    }

    bool SaveBank(const std::string& path) {
        bool isOverwriting = (path == FileName);
        std::string targetPath = isOverwriting ? path + ".tmp" : path;

        std::ofstream out(targetPath, std::ios::binary);
        if (!out.is_open()) return false;

        out.write((char*)&MainHeader, sizeof(MainHeader));

        std::vector<AudioLookupEntry> newEntries;
        uint32_t currentOffset = 0;

        for (int i = 0; i < (int)Entries.size(); i++) {
            std::vector<uint8_t> buffer;

            if (ModifiedCache.count(i)) {
                buffer = ModifiedCache[i];
            }
            else {
                const auto& oldE = Entries[i];
                buffer.resize(oldE.Length);
                Stream.clear();
                Stream.seekg(AudioBlobStart + oldE.Offset, std::ios::beg);
                Stream.read((char*)buffer.data(), oldE.Length);
            }

            out.write((char*)buffer.data(), buffer.size());

            AudioLookupEntry newE = Entries[i];
            newE.Offset = currentOffset;
            newE.Length = (uint32_t)buffer.size();
            newEntries.push_back(newE);

            currentOffset += (uint32_t)buffer.size();
        }

        TableHeader.NumEntries = (uint32_t)newEntries.size();
        out.write((char*)&TableHeader, sizeof(TableHeader));

        for (const auto& e : newEntries) out.write((char*)&e, sizeof(AudioLookupEntry));

        if (!FooterData.empty()) out.write((char*)FooterData.data(), FooterData.size());

        out.seekp(0, std::ios::beg);
        MainHeader.TotalAudioSize = currentOffset;
        out.write((char*)&MainHeader, sizeof(MainHeader));

        out.close();

        if (isOverwriting) {
            Stream.close();
            std::filesystem::remove(path);
            std::filesystem::rename(targetPath, path);
            Parse(path);
        }

        return true;
    }
};