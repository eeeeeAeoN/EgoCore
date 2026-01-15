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

    // Terminators (8 bytes)
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

static_assert(sizeof(LugEntryRaw) == 652, "LugEntryRaw size mismatch! Must be 652 bytes.");

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
        int OriginalIndex = -1;
        bool IsDeleted = false;

        uint32_t SoundID = 0;
        std::string Name;
        std::string FullPath;
        std::string GroupName;
        uint32_t Offset = 0;
        uint32_t Length = 0;

        uint16_t FormatTag = 0;
        uint16_t Channels = 0;
        uint32_t SampleRate = 0;

        uint32_t SoundType = 0;
        uint32_t Priority = 0;
        uint32_t Volume = 0;
        uint32_t PitchVar = 0;
        uint32_t Probability = 0;
        uint32_t InstanceLimit = 0;
        float MinDist = 0;
        float MaxDist = 0;
        uint32_t FlagsA = 0;
        uint32_t FlagsB = 0;

        LugEntryRaw RawMeta = { 0 };

        std::vector<uint8_t> CachedData;
    };

    std::vector<ParsedLugEntry> Entries;
    std::map<int, LugEntryRaw> GhostSlots;
    std::vector<LugScript> Scripts;

    // --- MANIPULATION METHODS ---

    void DeleteEntry(int index) {
        if (index >= 0 && index < (int)Entries.size()) {
            ParsedLugEntry& e = Entries[index];
            LugEntryRaw ghost = { 0 };
            if (e.OriginalIndex != -1) GhostSlots[e.OriginalIndex] = ghost;
            Entries.erase(Entries.begin() + index);
            IsDirty = true;
        }
    }

    void CloneEntry(int index) {
        if (index < 0 || index >= (int)Entries.size()) return;
        ParsedLugEntry e = Entries[index];
        e.OriginalIndex = -1;
        Entries.push_back(e);
        IsDirty = true;
    }

    // --- PARSING ---

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        char magic[8];
        Stream.read(magic, 8);
        if (memcmp(magic, LUG_SIG, 8) != 0) return false;

        Entries.clear();
        GhostSlots.clear();
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
                if (segSize > 0) {
                    std::vector<char> titleBuf(segSize + 1, 0);
                    Stream.read(titleBuf.data(), segSize);
                    BankTitle = titleBuf.data();
                }
            }
            else if (strcmp(segName, SEG_WAVE_NAME) == 0) {
                WaveDataStart = payloadStart;
            }
            else if (strcmp(segName, SEG_TABLE_NAME) == 0) {
                uint32_t countCombined;
                Stream.read((char*)&countCombined, 4);
                uint16_t totalSlots = (uint16_t)(countCombined & 0xFFFF);

                for (uint32_t i = 0; i < totalSlots; i++) {
                    LugEntryRaw raw;
                    Stream.read((char*)&raw, sizeof(LugEntryRaw));

                    if (raw.ID == 0 && raw.Length == 0) {
                        GhostSlots[i] = raw;
                    }
                    else {
                        ParsedLugEntry e;
                        e.OriginalIndex = i;
                        e.RawMeta = raw;

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

                        std::string s = e.FullPath;
                        size_t lastSlash = s.find_last_of("\\/");
                        if (lastSlash != std::string::npos) e.Name = s.substr(lastSlash + 1);
                        else e.Name = s;

                        Entries.push_back(e);
                    }
                }
            }
            else if (strcmp(segName, SEG_CRITERIA_NAME) == 0) {
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

        uint32_t absoluteStart = WaveDataStart + e.Offset;

        // Use a generous scan window (4096) to find RIFF headers even with large padding/misalignment
        uint32_t scanCap = 4096;
        uint32_t scanSize = (std::min)(scanCap, e.Length + 2048);
        if (scanSize < 512) scanSize = 512;

        std::vector<uint8_t> scanBuf(scanSize);

        Stream.clear();
        Stream.seekg(absoluteStart, std::ios::beg);
        Stream.read((char*)scanBuf.data(), scanSize);

        int riffOffset = -1;
        int maxLoop = (int)scanBuf.size() - 4;
        for (int i = 0; i < maxLoop; i++) {
            if (memcmp(&scanBuf[i], "RIFF", 4) == 0) {
                riffOffset = i;
                break;
            }
        }

        // Fallback: No RIFF found, just read strict e.Length
        if (riffOffset == -1) {
            std::vector<uint8_t> blob(e.Length);
            Stream.clear();
            Stream.seekg(absoluteStart, std::ios::beg);
            Stream.read((char*)blob.data(), e.Length);
            return blob;
        }

        uint32_t realStart = absoluteStart + riffOffset;
        uint32_t riffChunkSize = 0;

        if (riffOffset + 8 <= (int)scanBuf.size()) {
            memcpy(&riffChunkSize, &scanBuf[riffOffset + 4], 4);
        }
        else {
            Stream.seekg(realStart + 4, std::ios::beg);
            Stream.read((char*)&riffChunkSize, 4);
        }

        uint32_t exactSize = riffChunkSize + 8;

        // --- FIX: TRUST RIFF HEADER OVER TABLE LENGTH ---
        // If the table metadata says Length is 0 (or very small), but we found a valid 
        // RIFF header that is larger, we trust the RIFF header.
        // This handles cases where Fable allocated large chunks but recorded 0 length in the table.
        if (e.Length < exactSize) {
            // We trust the RIFF size, but we cap it to a sanity limit (e.g. 10MB) just in case
            if (exactSize > 10 * 1024 * 1024) exactSize = e.Length;
        }
        else if (exactSize > e.Length + 4096) {
            // If RIFF says it's WAY bigger than Table + Padding, it might be reading garbage.
            // But if e.Length was 0, the first check would have handled it.
            exactSize = e.Length;
        }

        std::vector<uint8_t> blob(exactSize);
        Stream.clear();
        Stream.seekg(realStart, std::ios::beg);
        Stream.read((char*)blob.data(), exactSize);

        return blob;
    }

    // --- SAVING ---

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
            out.write(BankTitle.c_str(), BankTitle.length() + 1);
            uint32_t written = (uint32_t)BankTitle.length() + 1;
            if (written < infoPayloadSize) {
                std::vector<char> pad(infoPayloadSize - written, 0);
                out.write(pad.data(), pad.size());
            }
        }

        // 2. DEDUPLICATION & WAVE WRITING
        struct UniqueBlob {
            std::vector<uint8_t> data;
            uint32_t offset;
        };
        std::vector<UniqueBlob> uniqueBlobs;
        std::vector<int> entryToBlobIndex(Entries.size(), -1);

        for (size_t i = 0; i < Entries.size(); i++) {
            std::vector<uint8_t> data = GetAudioBlob((int)i);
            int foundIdx = -1;
            // Only deduplicate if the blob isn't empty. 
            // If it IS empty (size 0), we treat it as unique to avoid weird mapping issues?
            // Actually, deduplicating empty blobs is fine.
            for (int b = 0; b < (int)uniqueBlobs.size(); b++) {
                if (uniqueBlobs[b].data == data) {
                    foundIdx = b;
                    break;
                }
            }
            if (foundIdx != -1) {
                entryToBlobIndex[i] = foundIdx;
            }
            else {
                uniqueBlobs.push_back({ data, 0 });
                entryToBlobIndex[i] = (int)uniqueBlobs.size() - 1;
            }
        }

        uint32_t currentOffset = 0;
        for (auto& blob : uniqueBlobs) {
            if (currentOffset % 2 != 0) currentOffset += (2 - (currentOffset % 2));
            blob.offset = currentOffset;
            currentOffset += (uint32_t)blob.data.size();
        }
        if (currentOffset % 2 != 0) currentOffset += (2 - (currentOffset % 2));
        uint32_t totalWavePayload = currentOffset;

        {
            char name[32] = { 0 }; strcpy_s(name, SEG_WAVE_NAME);
            out.write(name, 32); out.write((char*)&totalWavePayload, 4);
            long payloadStartPos = 0x234 + 36;

            for (const auto& blob : uniqueBlobs) {
                long currentPos = (long)out.tellp();
                long relPos = currentPos - payloadStartPos;
                while (relPos % 2 != 0) { out.put(0); currentPos++; relPos++; }
                if (!blob.data.empty()) out.write((char*)blob.data.data(), blob.data.size());
            }
            long currentPos = (long)out.tellp();
            long relPos = currentPos - payloadStartPos;
            while (relPos % 2 != 0) { out.put(0); relPos++; }
        }

        // 3. TABLE SEGMENT
        {
            std::map<int, LugEntryRaw*> WriteMap;

            for (auto& g : GhostSlots) {
                WriteMap[g.first] = &g.second;
            }

            std::vector<LugEntryRaw> activeRaw(Entries.size());
            int maxIndex = -1;
            if (!WriteMap.empty()) maxIndex = WriteMap.rbegin()->first;

            for (size_t i = 0; i < Entries.size(); i++) {
                LugEntryRaw& raw = activeRaw[i];
                const auto& e = Entries[i];

                raw = e.RawMeta;

                strncpy_s(raw.SourcePath, e.FullPath.c_str(), 260);
                strncpy_s(raw.GroupName, e.GroupName.c_str(), 256);
                raw.ID = e.SoundID;
                raw.ID_Repeat = e.SoundID;
                if (entryToBlobIndex[i] != -1) {
                    const auto& blob = uniqueBlobs[entryToBlobIndex[i]];
                    raw.Offset = blob.offset;
                    raw.Length = (uint32_t)blob.data.size();
                }
                raw.SoundType = e.SoundType;
                raw.Priority = e.Priority;
                raw.Volume = e.Volume;
                raw.PitchVar = e.PitchVar;
                raw.Probability = e.Probability;
                raw.InstanceLimit = e.InstanceLimit;
                raw.MinDist = e.MinDist;
                raw.MaxDist = e.MaxDist;
                raw.FlagsA = e.FlagsA;
                raw.FlagsB = e.FlagsB;

                if (e.FormatTag == 1) {
                    raw.wFormatTag = 1; raw.nChannels = e.Channels; raw.nSamplesPerSec = e.SampleRate;
                    raw.nAvgBytesPerSec = e.SampleRate * e.Channels * 2; raw.nBlockAlign = e.Channels * 2;
                    raw.wBitsPerSample = 16;
                }

                if (raw.Term1 == 0 && raw.Term2 == 0) {
                    raw.Term1 = 0xFFFFFFFF; raw.Term2 = 0xFFFFFFFF;
                    if (raw.FooterTerm == 0) raw.FooterTerm = 1;
                    uint32_t* pEndPad = (uint32_t*)raw.F_PadEnd;
                    pEndPad[0] = 0xFFFFFFFF; pEndPad[1] = 0x00000000;
                }

                int targetIdx = e.OriginalIndex;
                if (targetIdx == -1 || WriteMap.count(targetIdx)) {
                    targetIdx = ++maxIndex;
                }
                else {
                    if (targetIdx > maxIndex) maxIndex = targetIdx;
                }
                WriteMap[targetIdx] = &raw;
            }

            uint32_t finalTotal = (uint32_t)WriteMap.size();
            uint32_t finalActive = (uint32_t)Entries.size();
            uint32_t entriesSize = finalTotal * sizeof(LugEntryRaw);
            uint32_t totalPayload = 4 + entriesSize;

            char name[32] = { 0 }; strcpy_s(name, SEG_TABLE_NAME);
            out.write(name, 32); out.write((char*)&totalPayload, 4);

            uint32_t countField = (finalActive << 16) | finalTotal;
            out.write((char*)&countField, 4);

            for (auto const& [idx, ptr] : WriteMap) {
                out.write((char*)ptr, sizeof(LugEntryRaw));
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
            uint32_t totalPayload = 4 + critDataSize;

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