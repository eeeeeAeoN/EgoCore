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
#include <stdlib.h> 
#include "AudioHelpers.h"

#define LUG_SIG_STR "LiOnHeAd"
#define SEG_INFO_NAME "LHFileSegmentBankInfo"
#define SEG_WAVE_NAME "LHAudioWaveData"
#define SEG_TABLE_NAME "LHAudioBankSampleTable"
#define SEG_CRITERIA_NAME "LHAudioBankCriteiaInfo"

#define MASK_PITCH            0x0001
#define MASK_SEND_PITCH       0x0004
#define MASK_SEND_GAIN        0x0008
#define MASK_FLAGS            0x0010 
#define MASK_VOLUME           0x0020
#define MASK_LOOP             0x0040
#define MASK_MINDIST          0x0080
#define MASK_MAXDIST          0x0100
#define MASK_NON_INTERRUPT    0x0400 

struct LugScript {
    std::string Name;
    std::vector<uint32_t> SoundIDs;
};

#pragma pack(push, 1)

struct LugDriverRaw {
    uint32_t Priority;
    uint32_t ControlMask;
    uint32_t LoopCount;
    uint32_t Pad_0C;
    uint32_t PitchSend;
    uint32_t GainSend;
    uint32_t Flags;
    uint32_t Volume;
    uint32_t Pitch;
    uint32_t PitchVar;
    uint32_t MinDist;
    uint32_t MaxDist;
    uint32_t Unk_30;
    uint32_t FlagCheck1;
    uint32_t FlagCheck2;
    uint32_t Probability;
};

struct LugEntryRaw {
    char     SourcePath[260];
    uint32_t ResID_A;
    uint32_t ResID_B;
    uint32_t Length;
    uint32_t Offset;
    uint32_t Unk_Res[4];
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
    uint16_t PadEx;
    uint32_t LoopStartByte;
    uint32_t LoopEndByte;
    char     GroupName[256];
    LugDriverRaw Driver;
    uint8_t  EndPad[12];
};

static_assert(sizeof(LugDriverRaw) == 64, "LugDriverRaw size mismatch!");
static_assert(sizeof(LugEntryRaw) == 652, "LugEntryRaw size mismatch!");
#pragma pack(pop)

class LugParser {
public:
    std::string FileName;
    std::fstream Stream;
    bool IsLoaded = false;
    bool IsDirty = false;
    std::string BankTitle = "Bank title/description";
    uint32_t WaveDataStart = 0;
    uint16_t OriginalHeaderHigh = 0x0001;

    struct ParsedLugEntry {
        int OriginalIndex = -1;
        uint32_t SoundID = 0;
        std::string Name, FullPath, GroupName;
        uint32_t Offset = 0, Length = 0;
        uint16_t FormatTag = 0, Channels = 0;
        uint32_t SampleRate = 0, LoopStart = 0, LoopEnd = 0;
        uint32_t Priority = 1, LoopCount = 0;
        uint16_t PitchSend = 0, GainSend = 0;
        float Volume = 1.0f, Pitch = 1.0f, PitchVar = 0.0f;

        float Probability = 100.0f;
        uint32_t RawProbability = 1;

        float MinDist = 0.0f, MaxDist = 0.0f;
        bool Flag_Reverb = false, Flag_Occlusion = false, Flag_Interrupt = false;
        bool Flag_UseMinDist = false, Flag_UseMaxDist = false;

        bool ExplicitVolume = false;
        bool ExplicitPitch = false;

        LugEntryRaw RawMeta = { 0 };
        std::vector<uint8_t> CachedData;
    };

    std::vector<ParsedLugEntry> Entries;
    std::map<int, LugEntryRaw> GhostSlots;
    std::vector<LugScript> Scripts;

    bool AddEntryFromWav(const std::string& wavPath) {
        uint32_t maxID = 0;
        for (const auto& en : Entries) if (en.SoundID > maxID) maxID = en.SoundID;
        uint32_t newID = (maxID > 0) ? maxID + 1 : 20000;

        ParsedLugEntry e;
        e.OriginalIndex = -1;
        e.SoundID = newID;
        e.GroupName = "Default";
        e.Priority = 1; e.Volume = 1.0f; e.Pitch = 1.0f; e.Probability = 100.0f;
        e.LoopCount = 0;

        Entries.push_back(e);
        int idx = (int)Entries.size() - 1;

        if (ImportWav(idx, wavPath)) {
            std::filesystem::path p(wavPath);
            Entries[idx].Name = p.filename().string();
            Entries[idx].FullPath = p.filename().string();
            IsDirty = true;
            return true;
        }
        else {
            Entries.pop_back();
            return false;
        }
    }

    bool ImportWav(int index, const std::string& wavPath) {
        if (index < 0 || index >= Entries.size()) return false;
        std::ifstream f(wavPath, std::ios::binary);
        if (!f.is_open()) return false;
        f.seekg(0, std::ios::end);
        std::vector<uint8_t> raw((size_t)f.tellg());
        f.seekg(0, std::ios::beg);
        f.read((char*)raw.data(), raw.size());

        if (raw.size() < 12 || memcmp(&raw[0], "RIFF", 4) != 0) return false;

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
            else if (memcmp(&chunkID, "data", 4) == 0) { pcm = &raw[c + 8]; pcmSz = sz; }
            c += 8 + sz;
        }

        if (!pcm) return false;

        std::vector<uint8_t> riffData;

        if (bits == 16) {
            std::vector<int16_t> pcm16; size_t count = pcmSz / 2;
            const int16_t* s = (const int16_t*)pcm;
            pcm16.assign(s, s + count);

            std::vector<uint8_t> adpcm = XboxAdpcmEncoder::Encode(pcm16, chans);

            uint32_t riffPayloadSize = 36 + 4 + (uint32_t)adpcm.size();
            riffData.reserve(riffPayloadSize + 8);
            auto u32 = [&](uint32_t v) { riffData.insert(riffData.end(), (uint8_t*)&v, (uint8_t*)&v + 4); };
            auto u16 = [&](uint16_t v) { riffData.insert(riffData.end(), (uint8_t*)&v, (uint8_t*)&v + 2); };
            auto str = [&](const char* s) { riffData.insert(riffData.end(), s, s + 4); };

            uint16_t blockAlign = (chans == 2) ? 72 : 36;
            str("RIFF"); u32(riffPayloadSize); str("WAVE");
            str("fmt "); u32(20); u16(0x0069); u16(chans); u32(rate); u32((rate * blockAlign) / 64); u16(blockAlign); u16(4); u16(2); u16(64);
            str("data"); u32((uint32_t)adpcm.size());
            riffData.insert(riffData.end(), adpcm.begin(), adpcm.end());

            Entries[index].FormatTag = 0x0069;
        }
        else {
            riffData = raw;
            Entries[index].FormatTag = 1;
        }

        auto& e = Entries[index];
        e.CachedData = riffData;
        e.Channels = chans;
        e.SampleRate = rate;
        e.Length = (uint32_t)riffData.size();
        e.LoopStart = 0;
        e.LoopEnd = (uint32_t)riffData.size();

        IsDirty = true;
        return true;
    }

    void DeleteEntry(int index) {
        if (index >= 0 && index < (int)Entries.size()) {
            if (Entries[index].OriginalIndex != -1) {
                LugEntryRaw ghost = { 0 };
                GhostSlots[Entries[index].OriginalIndex] = ghost;
            }
            Entries.erase(Entries.begin() + index);
            IsDirty = true;
        }
    }

    void CloneEntry(int index) {
        if (index < 0 || index >= (int)Entries.size()) return;
        ParsedLugEntry e = Entries[index];
        e.OriginalIndex = -1;
        uint32_t maxID = 0;
        for (const auto& en : Entries) if (en.SoundID > maxID) maxID = en.SoundID;
        e.SoundID = maxID + 1;
        Entries.push_back(e);
        IsDirty = true;
    }

    bool Parse(const std::string& path) {
        FileName = path;
        if (Stream.is_open()) Stream.close();
        Stream.open(path, std::ios::binary | std::ios::in);
        if (!Stream.is_open()) return false;

        char magic[8]; Stream.read(magic, 8);
        if (memcmp(magic, LUG_SIG_STR, 8) != 0) return false;

        Entries.clear(); GhostSlots.clear(); Scripts.clear();
        WaveDataStart = 0; IsDirty = false;

        Stream.seekg(0, std::ios::end);
        uint32_t fileSize = (uint32_t)Stream.tellg();
        Stream.seekg(8, std::ios::beg);

        while (Stream.tellg() < fileSize) {
            char segName[32] = { 0 }; uint32_t segSize = 0;
            Stream.read(segName, 32);
            if (Stream.gcount() < 32) break;
            Stream.read((char*)&segSize, 4);
            uint32_t payloadStart = (uint32_t)Stream.tellg();

            if (strncmp(segName, SEG_INFO_NAME, 32) == 0) {
                if (segSize >= 260) {
                    char title[261] = { 0 }; Stream.read(title, 260); BankTitle = title;
                }
            }
            else if (strncmp(segName, SEG_WAVE_NAME, 32) == 0) WaveDataStart = payloadStart;
            else if (strncmp(segName, SEG_TABLE_NAME, 32) == 0) {
                uint32_t countCombined; Stream.read((char*)&countCombined, 4);
                uint16_t totalSlots = (uint16_t)(countCombined & 0xFFFF);
                OriginalHeaderHigh = (uint16_t)(countCombined >> 16);

                for (uint32_t i = 0; i < totalSlots; i++) {
                    LugEntryRaw raw; Stream.read((char*)&raw, sizeof(LugEntryRaw));
                    if (raw.ResID_B == 0) GhostSlots[i] = raw;
                    else {
                        ParsedLugEntry e; e.OriginalIndex = i; e.RawMeta = raw;
                        e.SoundID = raw.ResID_B; e.FullPath = raw.SourcePath; e.GroupName = raw.GroupName;
                        e.Offset = raw.Offset; e.Length = raw.Length;
                        e.FormatTag = raw.wFormatTag; e.Channels = raw.nChannels; e.SampleRate = raw.nSamplesPerSec;
                        e.LoopStart = raw.LoopStartByte; e.LoopEnd = raw.LoopEndByte;

                        uint32_t m = raw.Driver.ControlMask;
                        e.Priority = raw.Driver.Priority;
                        e.LoopCount = (m & MASK_LOOP) ? raw.Driver.LoopCount : 0;
                        e.PitchSend = (m & MASK_SEND_PITCH) ? (uint16_t)raw.Driver.PitchSend : 0;
                        e.GainSend = (m & MASK_SEND_GAIN) ? (uint16_t)raw.Driver.GainSend : 0;
                        e.ExplicitVolume = (m & MASK_VOLUME) != 0; e.Volume = e.ExplicitVolume ? (float)raw.Driver.Volume / 127.0f : 1.0f;
                        e.ExplicitPitch = (m & MASK_PITCH) != 0; e.Pitch = e.ExplicitPitch ? (float)raw.Driver.Pitch / 100.0f : 1.0f;
                        e.PitchVar = (float)raw.Driver.PitchVar / 100.0f;
                        e.RawProbability = raw.Driver.Probability;
                        if (raw.Driver.Probability > 0 && raw.Driver.Probability != 0xFFFFFFFF) e.Probability = 100.0f / (float)raw.Driver.Probability; else e.Probability = 100.0f;
                        if (m & MASK_MINDIST) memcpy(&e.MinDist, &raw.Driver.MinDist, 4);
                        if (m & MASK_MAXDIST) memcpy(&e.MaxDist, &raw.Driver.MaxDist, 4);
                        if (m & MASK_FLAGS) { e.Flag_Reverb = (raw.Driver.Flags & 0x01) != 0; e.Flag_Occlusion = (raw.Driver.Flags & 0x02) != 0; }
                        e.Flag_Interrupt = ((m & MASK_NON_INTERRUPT) == 0);
                        e.Flag_UseMinDist = (m & MASK_MINDIST) != 0; e.Flag_UseMaxDist = (m & MASK_MAXDIST) != 0;

                        size_t lastSlash = e.FullPath.find_last_of("\\/");
                        e.Name = (lastSlash != std::string::npos) ? e.FullPath.substr(lastSlash + 1) : e.FullPath;
                        Entries.push_back(e);
                    }
                }
            }
            else if (strncmp(segName, SEG_CRITERIA_NAME, 32) == 0) {
                uint32_t critCount; Stream.read((char*)&critCount, 4);
                for (uint32_t i = 0; i < critCount; i++) {
                    LugScript s; uint32_t l; Stream.read((char*)&l, 4);
                    if (l > 0) { std::vector<char> b(l + 1, 0); Stream.read(b.data(), l); s.Name = b.data(); }
                    uint32_t vc; Stream.read((char*)&vc, 4);
                    if (vc > 0) { s.SoundIDs.resize(vc); Stream.read((char*)s.SoundIDs.data(), vc * 4); }
                    Scripts.push_back(s);
                }
            }
            Stream.seekg(payloadStart + segSize, std::ios::beg);
        }
        IsLoaded = true; return true;
    }

private:
    void WriteSeg(std::ofstream& out, const char* name, uint32_t size) {
        char buf[32] = { 0 }; memcpy(buf, name, (std::min)((size_t)32, strlen(name)));
        out.write(buf, 32); out.write((char*)&size, 4);
    }

    std::vector<uint8_t> GetRawBlobFromDisk(uint32_t offset, uint32_t length) {
        if (length == 0 || offset == 0xFFFFFFFF) return {};
        // Use a temporary stream to avoid conflicting with the main Stream state during Save
        std::ifstream fs(FileName, std::ios::binary);
        if (!fs.is_open()) return {};

        uint32_t a = WaveDataStart + offset;
        std::vector<uint8_t> b(length + 1024);
        fs.seekg(a, std::ios::beg);
        fs.read((char*)b.data(), (std::min)((size_t)b.size(), (size_t)1024 + length));

        for (int i = 0; i < 1024 && i < (int)b.size() - 8; i++) {
            if (memcmp(&b[i], "RIFF", 4) == 0) {
                uint32_t rS = 0; memcpy(&rS, &b[i + 4], 4);
                std::vector<uint8_t> res(rS + 8);
                fs.seekg(a + i, std::ios::beg);
                fs.read((char*)res.data(), rS + 8);
                return res;
            }
        }
        std::vector<uint8_t> fb(length);
        fs.seekg(a, std::ios::beg);
        fs.read((char*)fb.data(), fb.size());
        return fb;
    }

public:
    bool Save(const std::string& path) {
        if (Entries.empty() && GhostSlots.empty()) return false;
        struct Blob { std::vector<uint8_t> d; uint32_t origOff; uint32_t newOff; };
        std::vector<Blob> blobs;
        std::vector<int> blobIdx(Entries.size(), -1);
        std::map<int, int> ghostBlobIdx;

        auto FindOrAddBlob = [&](const std::vector<uint8_t>& data, uint32_t origOffset, bool isCached) -> int {
            if (data.empty()) return -1;
            if (!isCached && origOffset != 0xFFFFFFFF) { for (int b = 0; b < (int)blobs.size(); b++) if (blobs[b].origOff == origOffset) return b; }
            else { for (int b = 0; b < (int)blobs.size(); b++) if (blobs[b].d == data) return b; }
            blobs.push_back({ data, origOffset, 0 }); return (int)blobs.size() - 1;
            };

        for (size_t i = 0; i < Entries.size(); i++) {
            std::vector<uint8_t> d;
            if (!Entries[i].CachedData.empty()) d = Entries[i].CachedData;
            else d = GetRawBlobFromDisk(Entries[i].Offset, Entries[i].Length);
            blobIdx[i] = FindOrAddBlob(d, Entries[i].Offset, !Entries[i].CachedData.empty());
        }
        for (auto& g : GhostSlots) {
            if (g.second.Length > 0 && g.second.Offset != 0xFFFFFFFF) {
                ghostBlobIdx[g.first] = FindOrAddBlob(GetRawBlobFromDisk(g.second.Offset, g.second.Length), g.second.Offset, false);
            }
        }

        std::string tmp = path + ".tmp";
        std::ofstream out(tmp, std::ios::binary);
        if (!out.is_open()) return false;

        out.write(LUG_SIG_STR, 8);
        WriteSeg(out, SEG_INFO_NAME, 520);
        char b260[260] = { 0 }; strncpy_s(b260, 260, BankTitle.c_str(), _TRUNCATE);
        out.write(b260, 260); memset(b260, 0, 260); out.write(b260, 260);

        uint32_t cOff = 0;
        for (auto& b : blobs) { if (cOff % 2 != 0) cOff++; b.newOff = cOff; cOff += (uint32_t)b.d.size(); }
        if (cOff % 2 != 0) cOff++;

        WriteSeg(out, SEG_WAVE_NAME, cOff);
        long wStart = (long)out.tellp();
        for (const auto& b : blobs) {
            while (((long)out.tellp() - wStart) % 2 != 0) out.put(0);
            if (!b.d.empty()) out.write((char*)b.d.data(), b.d.size());
        }
        while (((long)out.tellp() - wStart) % 2 != 0) out.put(0);

        std::map<int, LugEntryRaw*> writeMap;

        for (auto& g : GhostSlots) {
            if (ghostBlobIdx.count(g.first) && ghostBlobIdx[g.first] != -1) g.second.Offset = blobs[ghostBlobIdx[g.first]].newOff;
            writeMap[g.first] = &g.second;
        }

        int maxI = -1;
        if (!writeMap.empty()) maxI = writeMap.rbegin()->first;
        for (const auto& e : Entries) {
            if (e.OriginalIndex > maxI) maxI = e.OriginalIndex;
        }

        std::vector<LugEntryRaw> active(Entries.size());

        for (size_t i = 0; i < Entries.size(); i++) {
            const auto& e = Entries[i]; LugEntryRaw& r = active[i]; r = e.RawMeta;
            strncpy_s(r.SourcePath, 260, e.FullPath.c_str(), _TRUNCATE); strncpy_s(r.GroupName, 256, e.GroupName.c_str(), _TRUNCATE);

            if (blobIdx[i] != -1) { r.Offset = blobs[blobIdx[i]].newOff; r.Length = (uint32_t)blobs[blobIdx[i]].d.size(); }
            else { r.Offset = 0; r.Length = 0; }

            if (e.OriginalIndex == -1) r.ResID_A = e.SoundID; else r.ResID_A = e.RawMeta.ResID_A;
            r.ResID_B = e.SoundID; r.wFormatTag = e.FormatTag; r.nChannels = e.Channels; r.nSamplesPerSec = e.SampleRate;

            if (e.FormatTag == 1) { r.nBlockAlign = e.Channels * 2; r.wBitsPerSample = 16; r.nAvgBytesPerSec = e.SampleRate * r.nBlockAlign; }
            else { r.nBlockAlign = (e.Channels == 2 ? 72 : 36); r.wBitsPerSample = 4; r.nAvgBytesPerSec = (e.SampleRate * r.nBlockAlign) / 64; }

            r.cbSize = 0; r.PadEx = 0; r.LoopStartByte = e.LoopStart; r.LoopEndByte = e.LoopEnd;

            uint32_t m = 0;
            if (e.OriginalIndex != -1) m = e.RawMeta.Driver.ControlMask & ~(MASK_LOOP | MASK_VOLUME | MASK_PITCH | MASK_MINDIST | MASK_MAXDIST | MASK_FLAGS | MASK_NON_INTERRUPT | MASK_SEND_GAIN | MASK_SEND_PITCH);
            else m = MASK_NON_INTERRUPT;

            r.Driver.Priority = e.Priority;
            if (e.LoopCount != 0) { r.Driver.LoopCount = e.LoopCount; m |= MASK_LOOP; }
            else r.Driver.LoopCount = 0;
            if (e.PitchSend > 0) { r.Driver.PitchSend = e.PitchSend; m |= MASK_SEND_PITCH; }
            else r.Driver.PitchSend = 0;
            if (e.GainSend > 0) { r.Driver.GainSend = e.GainSend; m |= MASK_SEND_GAIN; }
            else r.Driver.GainSend = 0;
            if (e.ExplicitVolume || e.Volume != 1.0f) { r.Driver.Volume = (uint32_t)(e.Volume * 127.0f + 0.5f); m |= MASK_VOLUME; }
            else r.Driver.Volume = 0;
            if (e.ExplicitPitch || e.Pitch != 1.0f) { r.Driver.Pitch = (uint32_t)(e.Pitch * 100.0f + 0.5f); m |= MASK_PITCH; }
            else r.Driver.Pitch = 0;
            r.Driver.PitchVar = (uint32_t)(e.PitchVar * 100.0f + 0.5f);
            if (e.Probability != 100.0f) r.Driver.Probability = (uint32_t)(100.0f / e.Probability); else r.Driver.Probability = e.RawProbability;
            if (e.Flag_UseMinDist) { memcpy(&r.Driver.MinDist, &e.MinDist, 4); m |= MASK_MINDIST; }
            else r.Driver.MinDist = 0;
            if (e.Flag_UseMaxDist) { memcpy(&r.Driver.MaxDist, &e.MaxDist, 4); m |= MASK_MAXDIST; }
            else r.Driver.MaxDist = 0;

            uint32_t flagBits = 0;
            if (e.OriginalIndex != -1) flagBits = e.RawMeta.Driver.Flags & ~0x03;
            if (e.Flag_Reverb) flagBits |= 0x01; if (e.Flag_Occlusion) flagBits |= 0x02;
            if (flagBits != 0 || (e.OriginalIndex != -1 && (e.RawMeta.Driver.ControlMask & MASK_FLAGS))) { r.Driver.Flags = flagBits; m |= MASK_FLAGS; }
            else r.Driver.Flags = 0;
            if (!e.Flag_Interrupt) m |= MASK_NON_INTERRUPT;
            r.Driver.ControlMask = m;

            int tIdx = e.OriginalIndex;
            if (tIdx == -1) tIdx = ++maxI;

            writeMap[tIdx] = &r;
        }

        uint32_t totalT = (uint32_t)writeMap.size();
        WriteSeg(out, SEG_TABLE_NAME, 4 + (totalT * 652));
        uint32_t cF = ((uint32_t)OriginalHeaderHigh << 16) | totalT; out.write((char*)&cF, 4);
        for (auto const& [idx, ptr] : writeMap) out.write((char*)ptr, 652);

        uint32_t dS = 0; for (const auto& s : Scripts) dS += 8 + (uint32_t)s.Name.length() + (uint32_t)s.SoundIDs.size() * 4;
        WriteSeg(out, SEG_CRITERIA_NAME, 4 + dS);
        uint32_t sC = (uint32_t)Scripts.size(); out.write((char*)&sC, 4);
        for (const auto& s : Scripts) {
            uint32_t l = (uint32_t)s.Name.length(); out.write((char*)&l, 4);
            out.write(s.Name.c_str(), l); uint32_t vC = (uint32_t)s.SoundIDs.size();
            out.write((char*)&vC, 4); out.write((char*)s.SoundIDs.data(), vC * 4);
        }

        out.close(); Stream.close();
        IsDirty = false;
        std::filesystem::remove(path); std::filesystem::rename(tmp, path);
        return Parse(path);
    }

    std::vector<uint8_t> GetAudioBlob(int idx) {
        if (idx < 0 || idx >= Entries.size()) return {};
        if (!Entries[idx].CachedData.empty()) return Entries[idx].CachedData;
        return GetRawBlobFromDisk(Entries[idx].Offset, Entries[idx].Length);
    }
};