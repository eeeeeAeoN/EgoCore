#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <cstring>
#include <filesystem>
#include "AudioHelpers.h"

static const char* LUG_SIG = "LiOnHeAd";
static const char* SEG_INFO_NAME = "LHFileSegmentBankInfo";
static const char* SEG_WAVE_NAME = "LHAudioWaveData";
static const char* SEG_TABLE_NAME = "LHAudioBankSampleTable";
static const char* SEG_CRITERIA_NAME = "LHAudioBankCriteiaInfo";

#pragma pack(push, 1)

// TOTAL SIZE: 652 Bytes (0x28C)
struct LugEntryRaw {
    char     SourcePath[260];
    uint32_t ID;
    uint32_t ID_Repeat;
    uint32_t Length;
    uint32_t Offset;

    // Properties Block (16 bytes)
    uint32_t Unk1;
    uint32_t SoundType;
    uint32_t Unk3;
    uint32_t Unk4;

    // WAVEFORMATEX (20 bytes)
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
    uint16_t PadEx;

    // Terminators
    uint32_t Term1;
    uint32_t Term2;

    // Group / Context String (256 bytes)
    char     GroupName[256];

    // Logic Footer (76 bytes)
    uint32_t Priority;
    uint32_t Volume;
    uint32_t PitchVar;
    uint8_t  F_Pad1[16];
    uint32_t Probability;
    uint8_t  F_Pad2[4];
    uint32_t InstanceLimit;
    uint8_t  F_Pad3[8];
    float    MinDist;
    float    MaxDist;
    uint32_t FlagsA;
    uint32_t FlagsB;
    uint32_t FooterTerm;
    uint8_t  F_PadEnd[8];
};

static_assert(sizeof(LugEntryRaw) == 652, "LugEntryRaw size mismatch!");

#pragma pack(pop)

struct LugScript {
    std::string Name;
    std::vector<uint32_t> SoundIDs;
};

class LugParser {
public:
    std::string FileName;
    std::fstream Stream;
    bool IsLoaded = false;
    bool IsDirty = false;

    std::string BankTitle = "Default Bank";
    uint32_t WaveDataStart = 0;

    struct ParsedLugEntry {
        uint32_t SoundID;
        std::string Name;
        std::string FullPath;
        std::string GroupName;
        uint32_t Offset;
        uint32_t Length;

        uint16_t FormatTag;
        uint16_t Channels;
        uint32_t SampleRate;

        uint32_t SoundType;
        uint32_t Priority;
        uint32_t Volume;
        uint32_t PitchVar;
        uint32_t Probability;
        uint32_t InstanceLimit;
        float MinDist;
        float MaxDist;
        uint32_t FlagsA;
        uint32_t FlagsB;

        LugEntryRaw RawMeta;

        std::vector<uint8_t> CachedData;
    };

    std::vector<ParsedLugEntry> Entries;
    std::vector<LugScript> Scripts;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        char magic[8];
        Stream.read(magic, 8);
        if (memcmp(magic, LUG_SIG, 8) != 0) return false;

        Entries.clear();
        Scripts.clear();
        WaveDataStart = 0;

        Stream.seekg(0, std::ios::end);
        uint32_t fileSize = (uint32_t)Stream.tellg();
        Stream.seekg(8, std::ios::beg);

        while (Stream.tellg() < fileSize) {
            char segName[32] = { 0 };
            uint32_t segSize = 0;

            Stream.read(segName, 32);
            if (Stream.gcount() < 32) break;
            Stream.read((char*)&segSize, 4);
            uint32_t payloadStart = (uint32_t)Stream.tellg();

            if (strcmp(segName, SEG_INFO_NAME) == 0) {
                Stream.seekg(4, std::ios::cur);
                char title[260] = { 0 };
                Stream.read(title, 260);
                BankTitle = title;
            }
            else if (strcmp(segName, SEG_WAVE_NAME) == 0) {
                // Wave payload starts at payloadStart (0x258)
                WaveDataStart = payloadStart;
            }
            else if (strcmp(segName, SEG_TABLE_NAME) == 0) {
                // --- FIX: NO PADDING SKIP ---
                // The 32 bytes read into segName INCLUDED the padding.
                // The stream is now pointing directly at the 'Count'.
                // 'segSize' from header is the size of the payload.

                uint32_t countCombined;
                Stream.read((char*)&countCombined, 4);
                uint16_t count = (uint16_t)(countCombined & 0xFFFF);

                for (uint32_t i = 0; i < count; i++) {
                    LugEntryRaw raw;
                    Stream.read((char*)&raw, sizeof(LugEntryRaw));

                    ParsedLugEntry e;
                    e.SoundID = raw.ID;
                    e.FullPath = raw.SourcePath;
                    e.GroupName = raw.GroupName;
                    e.Offset = raw.Offset;
                    e.Length = raw.Length;

                    e.FormatTag = raw.wFormatTag;
                    e.Channels = raw.nChannels;
                    e.SampleRate = raw.nSamplesPerSec;

                    e.SoundType = raw.SoundType;
                    e.Priority = raw.Priority;
                    e.Volume = raw.Volume;
                    e.PitchVar = raw.PitchVar;
                    e.Probability = raw.Probability;
                    e.InstanceLimit = raw.InstanceLimit;
                    e.MinDist = raw.MinDist;
                    e.MaxDist = raw.MaxDist;
                    e.FlagsA = raw.FlagsA;
                    e.FlagsB = raw.FlagsB;

                    e.RawMeta = raw;

                    std::string s = e.FullPath;
                    size_t lastSlash = s.find_last_of("\\/");
                    if (lastSlash != std::string::npos) e.Name = s.substr(lastSlash + 1);
                    else e.Name = s;

                    Entries.push_back(e);
                }
            }
            else if (strcmp(segName, SEG_CRITERIA_NAME) == 0) {
                // Same logic here: No padding skip unless observed otherwise
                uint32_t rs; // Actually, Criteria might have the extra size field
                // Based on previous code: 10 bytes pad, 4 real size, 4 count.
                // Let's stick to the previous logic for Criteria unless proven wrong.
                // Actually, if Table doesn't have it, Criteria likely doesn't either.
                // Let's assume standard layout: Just Count.

                // User Dump for Criteria:
                // "4C 48 ... (Name) 00 ... (Pad inside name) 30 42 01 00 (Size) D2 05 00 00 (Count)"
                // This matches Table: Name(32) + Size(4) + Payload(Count + Data).

                uint32_t critCount;
                Stream.read((char*)&critCount, 4);

                for (uint32_t i = 0; i < critCount; i++) {
                    LugScript s;
                    uint32_t l; Stream.read((char*)&l, 4);
                    if (l > 0) {
                        std::vector<char> b(l + 1, 0);
                        Stream.read(b.data(), l);
                        s.Name = b.data();
                    }
                    uint32_t vc; Stream.read((char*)&vc, 4);
                    if (vc > 0) {
                        s.SoundIDs.resize(vc);
                        Stream.read((char*)s.SoundIDs.data(), vc * 4);
                    }
                    Scripts.push_back(s);
                }
            }
            if (segSize == 0 && Stream.tellg() >= fileSize) break;
            Stream.seekg(payloadStart + segSize, std::ios::beg);
        }
        IsLoaded = true;
        return true;
    }

    std::vector<uint8_t> GetAudioBlob(int index) {
        if (index < 0 || index >= Entries.size()) return {};
        const auto& e = Entries[index];
        if (!e.CachedData.empty()) return e.CachedData;

        // Scan logic from previous step is good
        uint32_t absoluteStart = WaveDataStart + e.Offset;
        uint32_t scanSize = (std::min)((uint32_t)256, e.Length + 128);
        std::vector<uint8_t> scanBuf(scanSize);

        Stream.clear();
        Stream.seekg(absoluteStart, std::ios::beg);
        Stream.read((char*)scanBuf.data(), scanSize);

        int riffOffset = -1;
        for (int i = 0; i < (int)scanBuf.size() - 4; i++) {
            if (memcmp(&scanBuf[i], "RIFF", 4) == 0) {
                riffOffset = i;
                break;
            }
        }
        if (riffOffset == -1) riffOffset = 0;

        uint32_t realStart = absoluteStart + riffOffset;
        std::vector<uint8_t> blob(e.Length);
        Stream.clear();
        Stream.seekg(realStart, std::ios::beg);
        Stream.read((char*)blob.data(), e.Length);
        return blob;
    }

    // --- WRITE LOGIC ---
    void AddEntry(uint32_t id, const std::string& wavPath) {
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return;
        f.seekg(0, std::ios::end);
        std::vector<uint8_t> data((size_t)f.tellg());
        f.seekg(0, std::ios::beg);
        f.read((char*)data.data(), data.size());

        ParsedLugEntry e;
        memset(&e.RawMeta, 0, sizeof(LugEntryRaw));

        e.SoundID = id;
        e.FullPath = wavPath;
        e.Name = std::filesystem::path(wavPath).filename().string();
        e.CachedData = data;
        e.Length = (uint32_t)data.size();

        if (data.size() >= 36 && memcmp(data.data(), "RIFF", 4) == 0) {
            e.FormatTag = *(uint16_t*)(data.data() + 20);
            e.Channels = *(uint16_t*)(data.data() + 22);
            e.SampleRate = *(uint32_t*)(data.data() + 24);
        }
        else {
            e.FormatTag = 1; e.Channels = 1; e.SampleRate = 22050;
        }

        e.SoundType = 0; e.Priority = 10000; e.Volume = 1500;
        e.MinDist = 3.0f; e.MaxDist = 10.0f; e.FlagsA = 2; e.FlagsB = 1;
        Entries.push_back(e);
        IsDirty = true;
    }

    void CloneEntry(int index) {
        if (index < 0 || index >= Entries.size()) return;
        ParsedLugEntry copy = Entries[index];
        uint32_t maxID = 0;
        for (const auto& e : Entries) if (e.SoundID > maxID) maxID = e.SoundID;
        copy.SoundID = maxID + 1;
        copy.Name += "_Copy";
        if (copy.CachedData.empty()) copy.CachedData = GetAudioBlob(index);
        Entries.push_back(copy);
        IsDirty = true;
    }

    void DeleteEntry(int index) {
        if (index < 0 || index >= Entries.size()) return;
        uint32_t idToRemove = Entries[index].SoundID;
        for (auto& s : Scripts) {
            auto it = std::remove(s.SoundIDs.begin(), s.SoundIDs.end(), idToRemove);
            if (it != s.SoundIDs.end()) s.SoundIDs.erase(it, s.SoundIDs.end());
        }
        Entries.erase(Entries.begin() + index);
        IsDirty = true;
    }

    bool Save(const std::string& path) {
        std::string tempPath = path + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        out.write(LUG_SIG, 8);

        // 1. INFO
        uint32_t infoPayloadSize = 0x234 - 44;
        {
            char name[32] = { 0 }; strcpy_s(name, SEG_INFO_NAME);
            out.write(name, 32); out.write((char*)&infoPayloadSize, 4);
            uint32_t ver = 0x00000208; out.write((char*)&ver, 4);
            out.write(BankTitle.c_str(), BankTitle.length() + 1);
            uint32_t written = 4 + (uint32_t)BankTitle.length() + 1;
            if (written < infoPayloadSize) {
                std::vector<char> pad(infoPayloadSize - written, 0);
                out.write(pad.data(), pad.size());
            }
        }

        // 2. WAVE DATA
        std::vector<uint32_t> newOffsets;
        uint32_t currentOffset = 0;

        for (const auto& e : Entries) {
            if (currentOffset % 4 != 0) currentOffset += (4 - (currentOffset % 4));
            newOffsets.push_back(currentOffset);
            if (!e.CachedData.empty()) currentOffset += (uint32_t)e.CachedData.size();
            else currentOffset += e.Length;
        }
        if (currentOffset % 4 != 0) currentOffset += (4 - (currentOffset % 4));
        uint32_t totalWavePayload = 8 + currentOffset;

        {
            char name[32] = { 0 }; strcpy_s(name, SEG_WAVE_NAME);
            out.write(name, 32); out.write((char*)&totalWavePayload, 4);
            uint32_t pad = 0; out.write((char*)&pad, 4);
            out.write((char*)&currentOffset, 4);

            for (size_t i = 0; i < Entries.size(); i++) {
                long currentPos = (long)out.tellp();
                long relPos = currentPos - (0x234 + 36 + 8);
                while (relPos % 4 != 0) { out.put(0); currentPos++; relPos++; }
                std::vector<uint8_t> data = GetAudioBlob((int)i);
                if (!data.empty()) out.write((char*)data.data(), data.size());
            }
            long currentPos = (long)out.tellp();
            long relPos = currentPos - (0x234 + 36 + 8);
            while (relPos % 4 != 0) { out.put(0); relPos++; }
        }

        // 3. TABLE
        // No Padding Skip in Read means No Padding Write here!
        {
            uint32_t count = (uint32_t)Entries.size();
            uint32_t entriesSize = count * sizeof(LugEntryRaw);
            uint32_t totalPayload = 4 + entriesSize; // Just Count + Entries
            char name[32] = { 0 }; strcpy_s(name, SEG_TABLE_NAME);
            out.write(name, 32); out.write((char*)&totalPayload, 4);

            uint32_t countField = (count << 16) | count; out.write((char*)&countField, 4);

            for (size_t i = 0; i < Entries.size(); i++) {
                const auto& e = Entries[i];
                LugEntryRaw raw = e.RawMeta;
                strncpy_s(raw.SourcePath, e.FullPath.c_str(), 259);
                strncpy_s(raw.GroupName, e.GroupName.c_str(), 259);
                raw.ID = e.SoundID; raw.ID_Repeat = e.SoundID;
                raw.Length = (!e.CachedData.empty()) ? (uint32_t)e.CachedData.size() : e.Length;
                raw.Offset = newOffsets[i];
                raw.SoundType = e.SoundType;
                raw.wFormatTag = e.FormatTag; raw.nChannels = e.Channels; raw.nSamplesPerSec = e.SampleRate;
                raw.nAvgBytesPerSec = e.SampleRate * e.Channels * 2;
                raw.nBlockAlign = e.Channels * 2; raw.wBitsPerSample = 16;
                raw.Priority = e.Priority; raw.Volume = e.Volume; raw.PitchVar = e.PitchVar;
                raw.Probability = e.Probability; raw.InstanceLimit = e.InstanceLimit;
                raw.MinDist = e.MinDist; raw.MaxDist = e.MaxDist;
                raw.FlagsA = e.FlagsA; raw.FlagsB = e.FlagsB;
                raw.Term1 = 0xFFFFFFFF; raw.Term2 = 0xFFFFFFFF; raw.FooterTerm = 0xFFFFFFFF;
                out.write((char*)&raw, sizeof(raw));
            }
        }

        // 4. CRITERIA
        {
            uint32_t critDataSize = 0;
            for (const auto& s : Scripts) {
                critDataSize += 4 + (uint32_t)s.Name.length();
                critDataSize += 4 + (uint32_t)(s.SoundIDs.size() * 4);
            }
            uint32_t count = (uint32_t)Scripts.size();
            uint32_t realSize = critDataSize; // Just Data
            uint32_t totalPayload = 4 + realSize; // Count + Data

            char name[32] = { 0 }; strcpy_s(name, SEG_CRITERIA_NAME);
            out.write(name, 32); out.write((char*)&totalPayload, 4);
            out.write((char*)&count, 4);
            for (const auto& s : Scripts) {
                uint32_t l = (uint32_t)s.Name.length(); out.write((char*)&l, 4);
                if (l > 0) out.write(s.Name.c_str(), l);
                uint32_t vc = (uint32_t)s.SoundIDs.size(); out.write((char*)&vc, 4);
                if (vc > 0) out.write((char*)s.SoundIDs.data(), vc * 4);
            }
        }

        out.close();
        Stream.close();
        if (path == FileName) {
            std::filesystem::remove(path);
            std::filesystem::rename(tempPath, path);
            Parse(path);
        }
        return true;
    }
};