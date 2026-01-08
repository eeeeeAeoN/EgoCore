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

// Generic Segment Header (36 bytes) used by the game's loop
struct LHFileSegmentHeader {
    char     Name[32];      // Null-terminated, padded with 0
    uint32_t PayloadSize;
};

// The content of the "LHAudioBankLookupTable" segment
struct LHAudioLookupContent {
    uint32_t Priority;      // Typically 500 (Dialogue) or 1000 (Script)
    uint32_t NumEntries;
};

// The Entry structure inside the Lookup Table
struct LookupEntry {
    uint32_t SoundID;
    uint32_t Length;        // Size of the Audio Blob (Header + RIFF)
    uint32_t Offset;        // Relative to the start of the Audio Segment payload
};

#pragma pack(pop)

class AudioBankParser {
public:
    std::string FileName;
    std::fstream Stream;
    uint32_t AudioBlobStart = 0;

    // Headers
    LHFileSegmentHeader CompDataHeader;
    LHFileSegmentHeader TableSegmentHeader;
    LHAudioLookupContent TableContent;

    // The "BankInfo" segment (Footer)
    std::vector<uint8_t> FooterData;

    std::vector<LookupEntry> Entries;
    bool IsLoaded = false;

    // Cache for Modified/New Entries (Contains full 36-byte header + RIFF)
    std::map<uint32_t, std::vector<uint8_t>> ModifiedCache;
    uint32_t InitialMaxID = 0;

    // The Player Instance
    AudioPlayer Player;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        // Check Magic Signature
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

        // Remove from cache if it was a new add
        if (ModifiedCache.count(id)) ModifiedCache.erase(id);

        // CRITICAL: We need a way to tell the saver that this bank IS dirty, 
        // even if ModifiedCache is empty (because we just removed something).
        IsDirty = true;
    }

    bool IsDirty = false;

    // --- UPDATED: COPY & PATCH WITH CORRECT ID LOGIC ---
    bool CloneEntry(int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= (int)Entries.size()) return false;

        // 1. Generate New ID (Matches Add Logic: Max + 1)
        uint32_t maxID = 0;
        for (const auto& e : Entries) if (e.SoundID > maxID) maxID = e.SoundID;
        uint32_t newID = (maxID > 0) ? maxID + 1 : 20000;

        // 2. Get Source Blob
        std::vector<uint8_t> blob = GetRawBlob(sourceIndex);
        if (blob.size() < 4) return false;

        // 3. Patch ID (First 4 bytes)
        memcpy(blob.data(), &newID, 4);

        // 4. Add to List
        LookupEntry newEntry;
        newEntry.SoundID = newID;
        newEntry.Length = (uint32_t)blob.size();
        newEntry.Offset = 0; // Calculated on Save
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

        uint32_t byteRate = *(uint32_t*)&h[28];
        if (byteRate == 0) return 0.0f;

        return (float)(s - 44) / byteRate;
    }

    bool SaveBank(const std::string& path) {
        std::string target = (path == FileName) ? path + ".tmp" : path;
        std::ofstream out(target, std::ios::binary);
        if (!out.is_open()) return false;

        out.write("LiOnHeAd", 8);
        out.write((char*)&CompDataHeader, sizeof(CompDataHeader));

        std::sort(Entries.begin(), Entries.end(),
            [](const LookupEntry& a, const LookupEntry& b) { return a.SoundID < b.SoundID; });

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
            if (data.size() >= 4) {
                memcpy(data.data(), &Entries[i].SoundID, 4);
            }

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
        for (const auto& e : finalEntries) {
            out.write((char*)&e, sizeof(LookupEntry));
        }

        if (!FooterData.empty()) {
            out.write((char*)FooterData.data(), FooterData.size());
        }

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

        return XboxAdpcmDecoder::Decode(std::vector<uint8_t>(buf.begin() + ds, buf.end()));
    }

private:
    std::vector<uint8_t> GetRawBlob(int index) {
        if (index < 0 || index >= (int)Entries.size()) return {};
        uint32_t id = Entries[index].SoundID;

        if (ModifiedCache.count(id)) {
            return ModifiedCache[id];
        }

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
        if (!pcm) return {};

        std::vector<int16_t> pcm16;
        if (bits == 16) {
            size_t count = pcmSz / 2;
            const int16_t* s = (const int16_t*)pcm;
            if (chans == 1) pcm16.assign(s, s + count);
            else for (size_t i = 0; i < count / 2; i++) pcm16.push_back(s[i * 2]);
        }
        else return {};

        std::vector<uint8_t> adpcm = XboxAdpcmEncoder::Encode(pcm16);

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

        std::vector<uint8_t> header;
        header.reserve(36);

        auto h_u32 = [&](uint32_t v) { header.insert(header.end(), (uint8_t*)&v, (uint8_t*)&v + 4); };
        auto h_u16 = [&](uint16_t v) { header.insert(header.end(), (uint8_t*)&v, (uint8_t*)&v + 2); };

        h_u32(id);
        h_u16(0x0001);
        h_u16((uint16_t)rate);

        if (adpcm.size() < 0x2000) h_u16((uint16_t)adpcm.size());
        else h_u16(0x9C40);

        h_u16(0x0001); h_u32(0x01010000); h_u16(0x5806); h_u32(0x00000064); h_u32(0x00003FC0); h_u16(0x4190);
        h_u16((uint16_t)TableContent.Priority); h_u16(0x0000); h_u32(0xFFFFFFFF);

        std::vector<uint8_t> finalBlob;
        finalBlob.reserve(header.size() + riff.size());
        finalBlob.insert(finalBlob.end(), header.begin(), header.end());
        finalBlob.insert(finalBlob.end(), riff.begin(), riff.end());

        return finalBlob;
    }
};