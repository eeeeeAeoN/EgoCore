#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <filesystem>
#include "AudioHelpers.h"

#pragma pack(push, 1)

struct LHFileSegmentHeader {
    char     Name[32];
    uint32_t PayloadSize;
};

struct LHAudioLookupContent {
    uint32_t Priority;
    uint32_t NumEntries;
};

struct LookupEntry {
    uint32_t SoundID;
    uint32_t Length;
    uint32_t Offset;
};

#pragma pack(pop)

class AudioPlayer {
    ma_device device;
    bool isInit = false;
    std::vector<int16_t> activeBuffer;
    std::atomic<size_t> playCursor = 0;
    int currentSampleRate = 22050;
    int currentChannels = 1;

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        AudioPlayer* player = (AudioPlayer*)pDevice->pUserData;
        if (!player) return;
        int16_t* out = (int16_t*)pOutput;
        size_t cursor = player->playCursor.load();

        size_t totalSamples = player->activeBuffer.size();
        size_t framesAvailable = (totalSamples - cursor) / player->currentChannels;
        size_t framesToCopy = (std::min)((size_t)frameCount, framesAvailable);

        if (framesToCopy > 0) {
            size_t samplesToCopy = framesToCopy * player->currentChannels;

            for (size_t i = 0; i < samplesToCopy; i++) {
                int32_t sample = player->activeBuffer[cursor + i];
                sample = sample / 2;
                out[i] = (int16_t)sample;
            }

            player->playCursor.store(cursor + samplesToCopy);
        }

        if (framesToCopy < frameCount) {
            size_t samplesRemaining = (frameCount - framesToCopy) * player->currentChannels;
            memset(out + (framesToCopy * player->currentChannels), 0, samplesRemaining * sizeof(int16_t));
        }
    }

public:
    AudioPlayer() { memset(&device, 0, sizeof(device)); }
    ~AudioPlayer() { Reset(); }

    void Reset() {
        if (isInit) { ma_device_uninit(&device); isInit = false; }
        activeBuffer.clear(); playCursor = 0;
    }

    void PlayPCM(const std::vector<int16_t>& pcm, int sampleRate, int channels = 1) {
        Reset();
        activeBuffer = pcm;
        currentSampleRate = sampleRate;
        currentChannels = channels;

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_s16;
        config.playback.channels = channels;
        config.sampleRate = sampleRate;
        config.dataCallback = data_callback;
        config.pUserData = this;

        if (ma_device_init(NULL, &config, &device) == MA_SUCCESS) {
            isInit = true;
            ma_device_start(&device);
        }
    }

    void PlayWav(const std::vector<uint8_t>& fileData) {
        if (fileData.size() < 12) return;
        const uint8_t* d = fileData.data();

        if (memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) return;

        uint16_t formatTag = 1;
        int channels = 1;
        int rate = 22050;
        int blockAlign = 0;
        std::vector<int16_t> pcm;

        size_t cursor = 12;
        size_t size = fileData.size();

        while (cursor + 8 < size) {
            uint32_t chunkID = *(uint32_t*)(d + cursor);
            uint32_t chunkSz = *(uint32_t*)(d + cursor + 4);
            cursor += 8;

            if (memcmp(&chunkID, "fmt ", 4) == 0) {
                if (chunkSz >= 16) {
                    formatTag = *(uint16_t*)(d + cursor);
                    channels = *(uint16_t*)(d + cursor + 2);
                    rate = *(uint32_t*)(d + cursor + 4);
                    blockAlign = *(uint16_t*)(d + cursor + 12);
                }
            }
            else if (memcmp(&chunkID, "data", 4) == 0) {
                if (cursor + chunkSz <= size) {
                    if (formatTag == 0x0069) { // Xbox ADPCM
                        std::vector<uint8_t> adpcmData(d + cursor, d + cursor + chunkSz);
                        pcm = XboxAdpcmDecoder::Decode(adpcmData, channels, blockAlign);
                    }
                    else { // Assume PCM
                        size_t sampleCount = chunkSz / 2;
                        pcm.resize(sampleCount);
                        memcpy(pcm.data(), d + cursor, chunkSz);
                    }
                    PlayPCM(pcm, rate, channels);
                }
                return;
            }
            cursor += chunkSz;
        }
    }

    void Play() { if (isInit) ma_device_start(&device); }
    void Pause() { if (isInit) ma_device_stop(&device); }
    void Stop() {
        if (isInit) {
            ma_device_stop(&device);
            playCursor = 0;
        }
    }

    bool IsPlaying() { return isInit && ma_device_is_started(&device); }

    float GetProgress() {
        return activeBuffer.empty() ? 0.0f : (float)playCursor / activeBuffer.size();
    }

    float GetTotalDuration() {
        return activeBuffer.empty() ? 0.0f : (float)(activeBuffer.size() / currentChannels) / currentSampleRate;
    }

    float GetCurrentTime() {
        return activeBuffer.empty() ? 0.0f : (float)(playCursor / currentChannels) / currentSampleRate;
    }

    void Seek(float p) {
        if (!activeBuffer.empty()) {
            size_t target = (size_t)(p * activeBuffer.size());
            target = target - (target % currentChannels);
            playCursor = target;
        }
    }
};

class AudioBankParser {
public:
    std::string FileName;
    std::fstream Stream;
    uint32_t AudioBlobStart = 0;

    LHFileSegmentHeader CompDataHeader;
    LHFileSegmentHeader TableSegmentHeader;
    LHAudioLookupContent TableContent;
    std::vector<uint8_t> FooterData;

    std::vector<LookupEntry> Entries;
    bool IsLoaded = false;
    std::map<uint32_t, std::vector<uint8_t>> ModifiedCache;
    uint32_t InitialMaxID = 0;
    bool IsDirty = false;

    AudioPlayer Player;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        char magic[8];
        Stream.read(magic, 8);
        if (memcmp(magic, "LiOnHeAd", 8) != 0) return false;

        Stream.read((char*)&CompDataHeader, sizeof(LHFileSegmentHeader));
        AudioBlobStart = (uint32_t)Stream.tellg();

        Stream.seekg(0, std::ios::end);
        size_t fileSize = (size_t)Stream.tellg();
        if (AudioBlobStart + CompDataHeader.PayloadSize > fileSize) return false;

        uint32_t tableOffset = AudioBlobStart + CompDataHeader.PayloadSize;
        Stream.seekg(tableOffset, std::ios::beg);

        Stream.read((char*)&TableSegmentHeader, sizeof(LHFileSegmentHeader));
        Stream.read((char*)&TableContent, sizeof(LHAudioLookupContent));

        if (TableContent.NumEntries > 100000) return false;

        Entries.clear();
        InitialMaxID = 0;

        for (uint32_t i = 0; i < TableContent.NumEntries; i++) {
            LookupEntry entry;
            Stream.read((char*)&entry, sizeof(LookupEntry));
            Entries.push_back(entry);
            if (entry.SoundID > InitialMaxID) InitialMaxID = entry.SoundID;
        }

        std::streampos currentPos = Stream.tellg();
        Stream.seekg(0, std::ios::end);
        size_t footerSize = (size_t)(Stream.tellg() - currentPos);

        if (footerSize > 0) {
            Stream.seekg(currentPos, std::ios::beg);
            FooterData.resize(footerSize);
            Stream.read((char*)FooterData.data(), footerSize);
        }
        else {
            FooterData.clear();
        }

        IsLoaded = true;
        ModifiedCache.clear();
        return true;
    }

    void DeleteEntry(int index) {
        if (index < 0 || index >= (int)Entries.size()) return;
        uint32_t id = Entries[index].SoundID;
        Entries.erase(Entries.begin() + index);
        if (ModifiedCache.count(id)) ModifiedCache.erase(id);
        IsDirty = true;
    }

    bool CloneEntry(int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= (int)Entries.size()) return false;
        uint32_t maxID = 0;
        for (const auto& e : Entries) if (e.SoundID > maxID) maxID = e.SoundID;
        uint32_t newID = (maxID > 0) ? maxID + 1 : 20000;

        std::vector<uint8_t> blob = GetRawBlob(sourceIndex);
        if (blob.size() < 4) return false;
        memcpy(blob.data(), &newID, 4);

        LookupEntry newEntry;
        newEntry.SoundID = newID;
        newEntry.Length = (uint32_t)blob.size();
        newEntry.Offset = 0;
        Entries.push_back(newEntry);
        ModifiedCache[newID] = blob;
        return true;
    }

    bool ImportWav(int index, const std::string& wavPath) {
        if (index < 0 || index >= (int)Entries.size()) return false;
        std::vector<uint8_t> finalBlob = CreateAudioBlob(Entries[index].SoundID, wavPath);
        if (finalBlob.empty()) return false;
        Entries[index].Length = (uint32_t)finalBlob.size();
        ModifiedCache[Entries[index].SoundID] = finalBlob;
        return true;
    }

    bool AddEntry(uint32_t newID, const std::string& wavPath) {
        std::vector<uint8_t> finalBlob = CreateAudioBlob(newID, wavPath);
        if (finalBlob.empty()) return false;
        LookupEntry newEntry;
        newEntry.SoundID = newID;
        newEntry.Length = (uint32_t)finalBlob.size();
        newEntry.Offset = 0;
        Entries.push_back(newEntry);
        ModifiedCache[newID] = finalBlob;
        return true;
    }

    bool SaveBank(const std::string& path) {
        std::string target = (path == FileName) ? path + ".tmp" : path;
        std::ofstream out(target, std::ios::binary);
        if (!out.is_open()) return false;

        out.write("LiOnHeAd", 8);
        out.write((char*)&CompDataHeader, sizeof(CompDataHeader));

        std::sort(Entries.begin(), Entries.end(), [](const LookupEntry& a, const LookupEntry& b) { return a.SoundID < b.SoundID; });

        std::vector<LookupEntry> finalEntries;
        uint32_t currentOffset = 0;

        for (int i = 0; i < (int)Entries.size(); i++) {
            uint32_t requiredAlign = 4;
            if (Entries[i].SoundID > InitialMaxID) requiredAlign = 2048;

            uint32_t currentAbsPos = (uint32_t)out.tellp();
            uint32_t pad = 0;
            if (currentAbsPos % requiredAlign != 0) {
                pad = requiredAlign - (currentAbsPos % requiredAlign);
            }
            if (pad > 0) {
                std::vector<char> zeros(pad, 0);
                out.write(zeros.data(), pad);
                currentOffset += pad;
            }

            std::vector<uint8_t> data = GetRawBlob(i);
            if (data.size() >= 4) memcpy(data.data(), &Entries[i].SoundID, 4);

            out.write((char*)data.data(), data.size());
            uint32_t sizeWritten = (uint32_t)data.size();

            LookupEntry e = Entries[i];
            e.Offset = currentOffset;
            e.Length = sizeWritten;
            finalEntries.push_back(e);
            currentOffset += sizeWritten;
        }

        CompDataHeader.PayloadSize = currentOffset;
        out.seekp(8, std::ios::beg);
        out.write((char*)&CompDataHeader, sizeof(CompDataHeader));
        out.seekp(0, std::ios::end);

        uint32_t pos = (uint32_t)out.tellp();
        uint32_t tablePad = (pos % 4 != 0) ? (4 - (pos % 4)) : 0;
        if (tablePad > 0) { const char z[4] = { 0 }; out.write(z, tablePad); }

        TableContent.NumEntries = (uint32_t)finalEntries.size();
        TableSegmentHeader.PayloadSize = sizeof(LHAudioLookupContent) + (TableContent.NumEntries * sizeof(LookupEntry));

        out.write((char*)&TableSegmentHeader, sizeof(TableSegmentHeader));
        out.write((char*)&TableContent, sizeof(TableContent));
        for (const auto& e : finalEntries) out.write((char*)&e, sizeof(LookupEntry));

        if (!FooterData.empty()) out.write((char*)FooterData.data(), FooterData.size());

        out.close();
        if (path == FileName) {
            Stream.close();
            std::filesystem::remove(path);
            std::filesystem::rename(target, path);
            Parse(path);
        }
        return true;
    }

    std::vector<int16_t> GetDecodedAudio(int index) {
        if (index < 0 || index >= (int)Entries.size()) return {};
        std::vector<uint8_t> buf = GetRawBlob(index);

        int ds = -1;
        for (size_t i = 0; i < buf.size() - 8; i++) {
            if (memcmp(&buf[i], "data", 4) == 0) { ds = (int)i + 8; break; }
        }
        if (ds == -1) return {};

        return XboxAdpcmDecoder::Decode(std::vector<uint8_t>(buf.begin() + ds, buf.end()), 1, 36);
    }

private:
    std::vector<uint8_t> GetRawBlob(int index) {
        if (index < 0 || index >= (int)Entries.size()) return {};
        uint32_t id = Entries[index].SoundID;
        if (ModifiedCache.count(id)) return ModifiedCache[id];
        const auto& e = Entries[index];
        if (e.Length == 0) return {};
        std::vector<uint8_t> buf(e.Length);
        Stream.clear();
        Stream.seekg(AudioBlobStart + e.Offset, std::ios::beg);
        Stream.read((char*)buf.data(), e.Length);
        return buf;
    }

    std::vector<uint8_t> CreateAudioBlob(uint32_t id, const std::string& wavPath) {
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return {};
        f.seekg(0, std::ios::end);
        std::vector<uint8_t> raw((size_t)f.tellg());
        f.seekg(0, std::ios::beg);
        f.read((char*)raw.data(), raw.size());

        if (raw.size() < 12 || memcmp(&raw[0], "RIFF", 4) != 0) return {};

        uint16_t chans = 0; uint32_t rate = 22050; uint16_t bits = 0;
        const uint8_t* pcm = nullptr; size_t pcmSz = 0;

        size_t c = 12;
        while (c < raw.size() - 8) {
            uint32_t chunkID = *(uint32_t*)&raw[c];
            uint32_t sz = *(uint32_t*)&raw[c + 4];
            if (memcmp(&chunkID, "fmt ", 4) == 0) {
                chans = *(uint16_t*)&raw[c + 10];
                rate = *(uint32_t*)&raw[c + 12];
                bits = *(uint16_t*)&raw[c + 22];
            }
            else if (memcmp(&chunkID, "data", 4) == 0) {
                pcm = &raw[c + 8]; pcmSz = sz;
            }
            c += 8 + sz;
        }
        if (!pcm || bits != 16) return {};

        std::vector<int16_t> pcm16;
        size_t count = pcmSz / 2;
        const int16_t* s = (const int16_t*)pcm;
        pcm16.assign(s, s + count);

        std::vector<uint8_t> adpcm = XboxAdpcmEncoder::Encode(pcm16, chans);

        uint32_t riffPayloadSize = 36 + 4 + (uint32_t)adpcm.size();
        std::vector<uint8_t> riff;
        riff.reserve(riffPayloadSize + 8);

        auto u32 = [&](uint32_t v) { riff.insert(riff.end(), (uint8_t*)&v, (uint8_t*)&v + 4); };
        auto u16 = [&](uint16_t v) { riff.insert(riff.end(), (uint8_t*)&v, (uint8_t*)&v + 2); };
        auto str = [&](const char* s) { riff.insert(riff.end(), s, s + 4); };

        uint16_t blockAlign = (chans == 2) ? 72 : 36;

        str("RIFF"); u32(riffPayloadSize); str("WAVE");
        str("fmt "); u32(20); u16(0x0069); u16(chans); u32(rate); u32((rate * blockAlign) / 64); u16(blockAlign); u16(4); u16(2); u16(64);
        str("data"); u32((uint32_t)adpcm.size());
        riff.insert(riff.end(), adpcm.begin(), adpcm.end());

        std::vector<uint8_t> header;
        header.reserve(36);
        auto h_u32 = [&](uint32_t v) { header.insert(header.end(), (uint8_t*)&v, (uint8_t*)&v + 4); };
        auto h_u16 = [&](uint16_t v) { header.insert(header.end(), (uint8_t*)&v, (uint8_t*)&v + 2); };

        h_u32(id); h_u16(0x0001); h_u16((uint16_t)rate);
        if (adpcm.size() < 0x2000) h_u16((uint16_t)adpcm.size()); else h_u16(0x9C40);
        h_u16(0x0001); h_u32(0x01010000); h_u16(0x5806); h_u32(0x00000064); h_u32(0x00003FC0); h_u16(0x4190);
        h_u16((uint16_t)TableContent.Priority); h_u16(0x0000); h_u32(0xFFFFFFFF);

        std::vector<uint8_t> finalBlob;
        finalBlob.reserve(header.size() + riff.size());
        finalBlob.insert(finalBlob.end(), header.begin(), header.end());
        finalBlob.insert(finalBlob.end(), riff.begin(), riff.end());
        return finalBlob;
    }
};

inline AudioPlayer player;