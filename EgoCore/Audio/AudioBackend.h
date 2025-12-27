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

// 1. DATA BLOCK HEADER (44 Bytes)
struct LHAudioBankCompData {
    char     Signature[32];
    uint32_t Unk1;          // DO NOT TOUCH
    uint32_t Unk2;          // DO NOT TOUCH
    uint32_t TotalDataSize;
};

// 2. LOOKUP TABLE HEADER
struct LHAudioBankLookupTable {
    char     Signature[32];
    uint32_t TableContentSize; // Verified: (NumEntries * 12) + 8
    uint32_t UnkFlags;
    uint32_t NumEntries;
};

// 3. LOOKUP ENTRY
struct LookupEntry {
    uint32_t SoundID;
    uint32_t Length;
    uint32_t Offset;
};

// 4. HEADER (32 Bytes)
struct WAVSUBHEADER {
    uint32_t SoundID;        // 00-03
    uint16_t NumChannels;    // 04-05
    uint16_t SampleRate;     // 06-07
    uint32_t Buffer;         // 08-11
    uint32_t unk0;           // 12-15
    uint8_t  unk1;           // 16
    uint8_t  BaseVolume;     // 17
    uint16_t Probability;    // 18-19
    float    unk2;           // 20-23
    float    unk3;           // 24-27
    uint32_t unk4;           // 28-31
};
#pragma pack(pop)

class AudioBankParser {
public:
    std::string FileName;
    std::fstream Stream;
    uint32_t AudioBlobStart = 44;

    LHAudioBankCompData MainHeader;
    LHAudioBankLookupTable TableHeader;
    std::vector<uint8_t> FooterData;

    std::vector<LookupEntry> Entries;
    bool IsLoaded = false;
    AudioPlayer Player;

    std::map<uint32_t, std::vector<uint8_t>> ModifiedCache;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        Stream.read((char*)&MainHeader, sizeof(LHAudioBankCompData));
        AudioBlobStart = 44;

        uint32_t tableOffset = AudioBlobStart + MainHeader.TotalDataSize;
        Stream.seekg(tableOffset, std::ios::beg);
        Stream.read((char*)&TableHeader, sizeof(LHAudioBankLookupTable));

        Entries.clear();
        for (uint32_t i = 0; i < TableHeader.NumEntries; i++) {
            LookupEntry entry;
            Stream.read((char*)&entry, sizeof(LookupEntry));
            Entries.push_back(entry);
        }

        std::streampos currentPos = Stream.tellg();
        Stream.seekg(0, std::ios::end);
        size_t footerSize = (size_t)(Stream.tellg() - currentPos);
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
        uint32_t id = Entries[index].SoundID;
        Entries.erase(Entries.begin() + index);
        if (ModifiedCache.count(id)) ModifiedCache.erase(id);
    }

    bool CloneEntry(int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= Entries.size()) return false;
        uint32_t maxID = 0;
        for (const auto& e : Entries) if (e.SoundID > maxID) maxID = e.SoundID;
        uint32_t newID = (maxID > 0) ? maxID + 1 : 20000;

        std::vector<uint8_t> data;
        uint32_t sourceID = Entries[sourceIndex].SoundID;

        if (ModifiedCache.count(sourceID)) {
            data = ModifiedCache[sourceID];
        }
        else {
            const auto& sourceEntry = Entries[sourceIndex];
            data.resize(sourceEntry.Length);
            Stream.clear();
            Stream.seekg(AudioBlobStart + sourceEntry.Offset, std::ios::beg);
            Stream.read((char*)data.data(), sourceEntry.Length);
        }

        // Patch ID
        if (data.size() >= 4) memcpy(data.data(), &newID, 4);

        LookupEntry newEntry;
        newEntry.SoundID = newID;
        newEntry.Length = (uint32_t)data.size();
        newEntry.Offset = 0;
        Entries.push_back(newEntry);
        ModifiedCache[newID] = data;

        return true;
    }

    bool AddEntry(uint32_t id, const std::string& wavPath) {
        for (const auto& e : Entries) if (e.SoundID == id) return false;
        LookupEntry newE;
        newE.SoundID = id;
        newE.Length = 0;
        newE.Offset = 0;
        Entries.push_back(newE);
        return ImportWav((int)Entries.size() - 1, wavPath);
    }

    std::vector<int16_t> GetDecodedAudio(int index) {
        if (index < 0 || index >= Entries.size()) return {};

        uint32_t id = Entries[index].SoundID;
        std::vector<uint8_t> buf;

        if (ModifiedCache.count(id)) buf = ModifiedCache[id];
        else {
            const auto& e = Entries[index];
            if (e.Length == 0) return {};
            buf.resize(e.Length);
            Stream.clear();
            Stream.seekg(AudioBlobStart + e.Offset, std::ios::beg);
            Stream.read((char*)buf.data(), e.Length);
        }

        int ds = -1;
        for (size_t i = 0; i < buf.size() - 8; i++) {
            if (memcmp(&buf[i], "data", 4) == 0) { ds = i + 8; break; }
        }
        if (ds == -1) return {};
        return XboxAdpcmDecoder::Decode(std::vector<uint8_t>(buf.begin() + ds, buf.end()));
    }

    bool ImportWav(int index, const std::string& wavPath) {
        if (index < 0 || index >= Entries.size()) return false;
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return false;

        f.seekg(0, std::ios::end);
        std::vector<uint8_t> raw(f.tellg());
        f.seekg(0, std::ios::beg);
        f.read((char*)raw.data(), raw.size());

        if (raw.size() < 12 || memcmp(&raw[0], "RIFF", 4) != 0) return false;

        uint16_t chans = 0; uint32_t rate = 22050; uint16_t bits = 0;
        const uint8_t* pcm = nullptr; size_t pcmSz = 0;

        size_t c = 12;
        while (c < raw.size() - 8) {
            uint32_t id = *(uint32_t*)&raw[c];
            uint32_t sz = *(uint32_t*)&raw[c + 4];
            if (memcmp(&id, "fmt ", 4) == 0) {
                chans = *(uint16_t*)&raw[c + 10];
                rate = *(uint32_t*)&raw[c + 12];
                bits = *(uint16_t*)&raw[c + 22];
            }
            else if (memcmp(&id, "data", 4) == 0) {
                pcm = &raw[c + 8]; pcmSz = sz;
            }
            c += 8 + sz;
        }
        if (!pcm) return false;

        std::vector<int16_t> pcm16;
        if (bits == 16) {
            size_t count = pcmSz / 2;
            const int16_t* s = (const int16_t*)pcm;
            if (chans == 1) pcm16.assign(s, s + count);
            else for (size_t i = 0; i < count / 2; i++) pcm16.push_back(s[i * 2]);
        }
        else return false;

        std::vector<uint8_t> adpcm = XboxAdpcmEncoder::Encode(pcm16);

        // --- 1. CONSTRUCT RIFF ---
        uint32_t riffPayloadSize = 36 + 4 + (uint32_t)adpcm.size();
        std::vector<uint8_t> riff;
        riff.reserve(riffPayloadSize + 8);

        auto u32 = [&](uint32_t v) { riff.insert(riff.end(), (uint8_t*)&v, (uint8_t*)&v + 4); };
        auto u16 = [&](uint16_t v) { riff.insert(riff.end(), (uint8_t*)&v, (uint8_t*)&v + 2); };
        auto str = [&](const char* s) { riff.insert(riff.end(), s, s + 4); };

        str("RIFF"); u32(riffPayloadSize); str("WAVE");
        str("fmt "); u32(20); u16(0x0069); u16(1); u32(rate); u32((rate * 36) / 64); u16(36); u16(4); u16(2); u16(64);
        str("data"); u32((uint32_t)adpcm.size());
        riff.insert(riff.end(), adpcm.begin(), adpcm.end());

        // --- 2. CONSTRUCT HEADER ---
        WAVSUBHEADER h = {};
        h.SoundID = Entries[index].SoundID;
        h.NumChannels = 1;
        h.SampleRate = (uint16_t)rate;
        h.Buffer = 0x00019C40;
        h.unk0 = 0x01010000;
        h.unk1 = 0x06;
        h.BaseVolume = 0x7F;
        h.Probability = 0x0064;
        h.unk2 = 1.5f;
        h.unk3 = 18.0f;
        h.unk4 = 500;

        std::vector<uint8_t> blob;
        blob.resize(sizeof(WAVSUBHEADER) + 4);
        memcpy(blob.data(), &h, sizeof(WAVSUBHEADER));
        uint32_t term = 0xFFFFFFFF;
        memcpy(blob.data() + sizeof(WAVSUBHEADER), &term, 4);
        blob.insert(blob.end(), riff.begin(), riff.end());

        Entries[index].Length = (uint32_t)blob.size();
        ModifiedCache[Entries[index].SoundID] = blob;
        return true;
    }

    bool SaveBank(const std::string& path) {
        std::string target = (path == FileName) ? path + ".tmp" : path;
        std::ofstream out(target, std::ios::binary);
        if (!out.is_open()) return false;

        out.write((char*)&MainHeader, sizeof(MainHeader));

        std::sort(Entries.begin(), Entries.end(),
            [](const LookupEntry& a, const LookupEntry& b) {
                return a.SoundID < b.SoundID;
            });

        std::vector<LookupEntry> finalEntries;
        uint32_t currentOffset = 0;

        // STRICT X4 ALIGNMENT
        const uint32_t ALIGNMENT = 4;

        for (int i = 0; i < (int)Entries.size(); i++) {
            std::vector<uint8_t> data;
            uint32_t id = Entries[i].SoundID;

            if (ModifiedCache.count(id)) {
                data = ModifiedCache[id];
            }
            else {
                data.resize(Entries[i].Length);
                Stream.clear();
                Stream.seekg(AudioBlobStart + Entries[i].Offset, std::ios::beg);
                Stream.read((char*)data.data(), Entries[i].Length);
            }

            // 1. Force ID Match
            if (ModifiedCache.count(id) && data.size() >= 4) {
                memcpy(data.data(), &id, 4);
            }

            // 2. Write Data
            out.write((char*)data.data(), data.size());

            // 3. Calculate x4 Padding for NEXT entry
            uint32_t sizeWritten = (uint32_t)data.size();
            uint32_t absoluteEnd = AudioBlobStart + currentOffset + sizeWritten;

            uint32_t pad = 0;
            if (absoluteEnd % ALIGNMENT != 0) {
                pad = ALIGNMENT - (absoluteEnd % ALIGNMENT);
            }

            if (pad > 0) {
                const std::vector<char> zeros(pad, 0);
                out.write(zeros.data(), pad);
            }

            // 4. Record Entry
            // CRITICAL: Length includes the padding we just wrote.
            // This ensures (Offset + Length) lands on the 4-byte boundary for the next entry.
            LookupEntry e = Entries[i];
            e.SoundID = id;
            e.Offset = currentOffset;
            e.Length = sizeWritten + pad;
            finalEntries.push_back(e);

            currentOffset += e.Length;
        }

        // Table Alignment
        uint32_t pos = (uint32_t)out.tellp();
        uint32_t pad = (pos % 4 != 0) ? (4 - (pos % 4)) : 0;
        if (pad > 0) { const char z[4] = { 0 }; out.write(z, pad); }

        // Write Table
        TableHeader.NumEntries = (uint32_t)finalEntries.size();
        TableHeader.TableContentSize = (TableHeader.NumEntries * sizeof(LookupEntry)) + 8;

        out.write((char*)&TableHeader, sizeof(TableHeader));
        for (const auto& e : finalEntries) out.write((char*)&e, sizeof(LookupEntry));

        if (!FooterData.empty()) out.write((char*)FooterData.data(), FooterData.size());

        out.seekp(0);
        MainHeader.TotalDataSize = pos + pad - 44;
        out.write((char*)&MainHeader, sizeof(MainHeader));
        out.close();

        if (path == FileName) {
            Stream.close();
            std::filesystem::remove(path);
            std::filesystem::rename(target, path);
            Parse(path);
        }
        return true;
    }

    static float GetWavDuration(const std::string& wavPath) {
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return 0.0f;
        f.seekg(0, std::ios::end);
        size_t s = f.tellg();
        f.seekg(0, std::ios::beg);
        if (s < 44) return 0.0f;
        std::vector<uint8_t> h(44);
        f.read((char*)h.data(), 44);
        if (memcmp(&h[0], "RIFF", 4) != 0) return 0.0f;
        uint32_t r = *(uint32_t*)&h[24];
        return r == 0 ? 0.0f : (float)(s - 44) / (r * 2);
    }
};