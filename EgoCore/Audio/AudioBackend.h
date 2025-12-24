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

// Bank Header
struct LHAudioBankCompData {
    char     Signature[32];
    uint32_t Unk1;
    uint32_t Unk2;
    uint32_t TotalAudioSize; // Offset to the Lookup Table
};

// Table Header (Corrected Names)
struct LHAudioBankLookupTable {
    char     Signature[32];
    uint32_t TableContentSize; // WAS BankID: Size of Entries[] + 8 bytes
    uint32_t UnkFlags;         // WAS BankFlags: Usually 500 or 0
    uint32_t NumEntries;
};

// Entry Meta (12 Bytes - Dense Packed)
struct AudioLookupEntry {
    uint32_t SoundID;
    uint32_t Length;
    uint32_t Offset; // Relative to AudioBlobStart (44)
};

// WAV Parameter Header (32 Bytes - Corrected)
struct FableWavHeader {
    uint16_t SharedTypeID;   // 00
    uint16_t SampleRate;     // 02
    uint32_t DataSize;       // 04: Size of RIFF/DATA following this struct
    uint16_t UnkPadding1;    // 08
    uint16_t UnkFlags;       // 10
    uint8_t  GroupID;        // 12
    uint8_t  BaseVolume;     // 13
    uint16_t Probability;    // 14
    uint16_t UnkPadding2;    // 16
    uint32_t MinDist;        // 18: Encoded Float
    uint16_t Pitch;          // 22: Encoded Float (High Word)
    uint32_t MaxDist;        // 24
    uint32_t Terminator;     // 28: FF FF FF FF
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

    std::vector<AudioLookupEntry> Entries;
    bool IsLoaded = false;
    AudioPlayer Player;

    std::map<int, std::vector<uint8_t>> ModifiedCache;

    bool Parse(const std::string& path) {
        FileName = path;
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        // Read Bank Header
        Stream.read((char*)&MainHeader, sizeof(LHAudioBankCompData));
        AudioBlobStart = sizeof(LHAudioBankCompData);

        // Jump to Lookup Table
        uint32_t tableOffset = AudioBlobStart + MainHeader.TotalAudioSize;
        Stream.seekg(tableOffset, std::ios::beg);

        // Read Table Header
        Stream.read((char*)&TableHeader, sizeof(LHAudioBankLookupTable));

        // Read Entries
        Entries.clear();
        for (uint32_t i = 0; i < TableHeader.NumEntries; i++) {
            AudioLookupEntry entry;
            Stream.read((char*)&entry, sizeof(AudioLookupEntry));
            Entries.push_back(entry);
        }

        // Read Footer (Everything after the entries)
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

        // Rebuild cache indices
        std::map<int, std::vector<uint8_t>> newCache;
        for (auto& [k, v] : ModifiedCache) {
            if (k < index) newCache[k] = v;
            else if (k > index) newCache[k - 1] = v;
        }
        ModifiedCache = newCache;
    }

    bool AddEntry(uint32_t id, const std::string& wavPath) {
        for (const auto& e : Entries) {
            if (e.SoundID == id) return false;
        }

        AudioLookupEntry newE;
        newE.SoundID = id;
        newE.Length = 0;
        newE.Offset = 0;
        Entries.push_back(newE);

        return ImportWav((int)Entries.size() - 1, wavPath);
    }

    // --- DECODER & DURATION HELPERS ---
    static float GetWavDuration(const std::string& wavPath) {
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return 0.0f;
        f.seekg(0, std::ios::end);
        size_t fileSize = f.tellg();
        f.seekg(0, std::ios::beg);
        if (fileSize < 44) return 0.0f;
        std::vector<uint8_t> header(44);
        f.read((char*)header.data(), 44);
        f.close();
        if (memcmp(&header[0], "RIFF", 4) != 0) return 0.0f;
        uint32_t rate = *(uint32_t*)&header[24];
        if (rate == 0) return 0.0f;

        // Approx duration from size for simplicity, or keep full parse if preferred
        return (float)(fileSize - 44) / (rate * 2);
    }

    std::vector<int16_t> GetDecodedAudio(int index) {
        if (index < 0 || index >= Entries.size()) return {};
        std::vector<uint8_t> rawBuffer;

        if (ModifiedCache.count(index)) {
            rawBuffer = ModifiedCache[index];
        }
        else {
            const auto& e = Entries[index];
            if (e.Length == 0) return {};
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

    // --- IMPORT WAV ---
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

        if (fileSize < 12 || memcmp(&fileData[0], "RIFF", 4) != 0) return false;

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

        if (!pcmStart || pcmBytes == 0) return false;

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

        // Build RIFF
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

        // Header Logic
        std::vector<uint8_t> headerBlob;
        const auto& originalEntry = Entries[index];
        bool neighborFound = false;

        // Try to Clone Neighbor
        if (originalEntry.Length > 0 && originalEntry.Offset > 0) {
            Stream.clear();
            Stream.seekg(AudioBlobStart + originalEntry.Offset, std::ios::beg);
            std::vector<uint8_t> probe(128);
            Stream.read((char*)probe.data(), 128);
            int riffIdx = -1;
            for (int r = 0; r < 100; r++) { if (memcmp(&probe[r], "RIFF", 4) == 0) { riffIdx = r; break; } }

            if (riffIdx > 0) {
                headerBlob.resize(riffIdx);
                memcpy(headerBlob.data(), probe.data(), riffIdx);
                neighborFound = true;
            }
        }

        if (!neighborFound) {
            for (const auto& existing : Entries) {
                if (existing.Length > 0 && existing.Offset > 0 && existing.SoundID != originalEntry.SoundID) {
                    Stream.clear();
                    Stream.seekg(AudioBlobStart + existing.Offset, std::ios::beg);
                    std::vector<uint8_t> probe(128);
                    Stream.read((char*)probe.data(), 128);
                    int riffIdx = -1;
                    for (int r = 0; r < 100; r++) { if (memcmp(&probe[r], "RIFF", 4) == 0) { riffIdx = r; break; } }
                    if (riffIdx > 0) {
                        headerBlob.resize(riffIdx);
                        memcpy(headerBlob.data(), probe.data(), riffIdx);
                        neighborFound = true;
                        break;
                    }
                }
            }
        }

        if (neighborFound) {
            // Patch Sample Rate (Offset 6 relative to start)
            if (headerBlob.size() >= 12) {
                uint16_t* pRate = (uint16_t*)&headerBlob[6];
                *pRate = (uint16_t)sampleRate;
                uint32_t* pSize = (uint32_t*)&headerBlob[8];
                *pSize = (uint32_t)riffFile.size();
            }
        }
        else {
            // Create New 36-byte Header (4 ID + 32 Struct)
            headerBlob.resize(4 + sizeof(FableWavHeader));
            uint32_t zero = 0;
            memcpy(&headerBlob[0], &zero, 4);

            FableWavHeader h = {};
            h.SharedTypeID = 1;
            h.SampleRate = (uint16_t)sampleRate;
            h.DataSize = (uint32_t)riffFile.size();
            h.UnkFlags = 0x0101;
            h.GroupID = 6;
            h.BaseVolume = 0x7F;
            h.Probability = 0x64;
            h.MinDist = 0x00003FC0;
            h.Pitch = 0x4190;
            h.MaxDist = 500;
            h.Terminator = 0xFFFFFFFF;

            memcpy(&headerBlob[4], &h, sizeof(FableWavHeader));
        }

        // Update ID
        uint32_t currentSoundID = Entries[index].SoundID;
        if (headerBlob.size() >= 4) {
            memcpy(headerBlob.data(), &currentSoundID, 4);
        }

        std::vector<uint8_t> finalBlob = headerBlob;
        finalBlob.insert(finalBlob.end(), riffFile.begin(), riffFile.end());

        Entries[index].Length = (uint32_t)finalBlob.size();
        ModifiedCache[index] = finalBlob;
        return true;
    }

    // --- SAVE BANK: FIXED TABLE HEADER CALCULATION ---
    bool SaveBank(const std::string& path) {
        bool isOverwriting = (path == FileName);
        std::string targetPath = isOverwriting ? path + ".tmp" : path;

        std::ofstream out(targetPath, std::ios::binary);
        if (!out.is_open()) return false;

        out.write((char*)&MainHeader, sizeof(MainHeader));

        struct EntryWriteData { AudioLookupEntry Meta; std::vector<uint8_t> Data; };
        std::vector<EntryWriteData> writeList;

        for (int i = 0; i < (int)Entries.size(); i++) {
            EntryWriteData item;
            item.Meta = Entries[i];
            if (ModifiedCache.count(i)) item.Data = ModifiedCache[i];
            else {
                const auto& oldE = Entries[i];
                if (oldE.Length > 0) {
                    item.Data.resize(oldE.Length);
                    Stream.clear();
                    Stream.seekg(AudioBlobStart + oldE.Offset, std::ios::beg);
                    Stream.read((char*)item.Data.data(), oldE.Length);
                }
            }
            writeList.push_back(item);
        }

        // DO NOT SORT - Preserve implicit order or grouping

        std::vector<AudioLookupEntry> newEntries;
        uint32_t currentOffset = 0;

        // WRITE AUDIO BLOBS (DENSE PACKING)
        for (auto& item : writeList) {
            if (!item.Data.empty()) {
                if (item.Data.size() >= 4) {
                    memcpy(item.Data.data(), &item.Meta.SoundID, 4);
                }
                out.write((char*)item.Data.data(), item.Data.size());
            }

            AudioLookupEntry newE;
            newE.SoundID = item.Meta.SoundID;
            newE.Offset = currentOffset;
            newE.Length = (uint32_t)item.Data.size();
            newEntries.push_back(newE);

            currentOffset += (uint32_t)item.Data.size();
        }

        // ALIGN TABLE START
        uint32_t globalPos = (uint32_t)out.tellp();
        if (globalPos % 4 != 0) {
            const char z[4] = { 0 };
            out.write(z, 4 - (globalPos % 4));
        }

        uint32_t entriesPayloadSize = (uint32_t)newEntries.size() * sizeof(AudioLookupEntry);
        TableHeader.TableContentSize = entriesPayloadSize + 8;

        TableHeader.NumEntries = (uint32_t)newEntries.size();
        out.write((char*)&TableHeader, sizeof(TableHeader));
        for (const auto& e : newEntries) out.write((char*)&e, sizeof(AudioLookupEntry));

        // WRITE FOOTER
        if (!FooterData.empty()) out.write((char*)FooterData.data(), FooterData.size());

        // UPDATE HEADER
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