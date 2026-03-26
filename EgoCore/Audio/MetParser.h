#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <filesystem>
#include "LugParser.h"

class MetParser {
private:
    static void WriteString(std::ofstream& out, const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        out.write((char*)&len, 4);
        if (len > 0) out.write(s.data(), len);
    }

public:
    static bool GenerateMetFile(const std::string& lugPath, LugParser& lugParser) {
        std::filesystem::path p(lugPath);
        p.replace_extension(".met");
        std::string metPath = p.string();

        std::ofstream out(metPath, std::ios::binary);
        if (!out.is_open()) return false;

        std::sort(lugParser.Scripts.begin(), lugParser.Scripts.end(),
            [](const LugScript& a, const LugScript& b) { return a.Name < b.Name; });

        uint32_t version = 1;
        out.write((char*)&version, 4);

        std::map<uint32_t, LugEntryRaw> uniqueResources;
        for (const auto& e : lugParser.Entries) {
            if (uniqueResources.find(e.RawMeta.ResID_B) == uniqueResources.end()) {
                uniqueResources[e.RawMeta.ResID_B] = e.RawMeta;
            }
        }

        for (const auto& g : lugParser.GhostSlots) {
            if (g.second.ResID_B != 0 && g.second.Length > 0) {
                if (uniqueResources.find(g.second.ResID_B) == uniqueResources.end()) {
                    uniqueResources[g.second.ResID_B] = g.second;
                }
            }
        }

        uint32_t resCount = (uint32_t)uniqueResources.size();
        out.write((char*)&resCount, 4);

        for (const auto& pair : uniqueResources) {
            uint32_t resID = pair.first;
            const LugEntryRaw& raw = pair.second;

            out.write((char*)&resID, 4);
            WriteString(out, std::string(raw.SourcePath));
            out.write((char*)&raw.Length, 4);

            uint32_t absOffset = lugParser.WaveDataStart + raw.Offset;
            out.write((char*)&absOffset, 4);

            out.write((char*)&raw.nChannels, 2);
            out.write((char*)&raw.wFormatTag, 2);
            out.write((char*)&raw.nSamplesPerSec, 4);
            out.write((char*)&raw.LoopStartByte, 4);
            out.write((char*)&raw.LoopEndByte, 4);
        }

        std::map<uint32_t, const LugParser::ParsedLugEntry*> sortedDrivers;
        for (const auto& e : lugParser.Entries) {
            sortedDrivers[e.RawMeta.ResID_A] = &e;
        }

        uint32_t driverCount = (uint32_t)sortedDrivers.size();
        out.write((char*)&driverCount, 4);

        for (const auto& pair : sortedDrivers) {
            uint32_t driverID = pair.first;
            const auto& e = *pair.second;

            out.write((char*)&driverID, 4);
            out.write((char*)&e.RawMeta.ResID_B, 4);
            WriteString(out, e.GroupName);

            uint16_t s6 = 1;
            uint16_t s7 = 0;
            if (e.OriginalIndex != -1) {
                s6 = (uint16_t)(e.RawMeta.Unk_Res[1] >> 16);
                s7 = (uint16_t)(e.RawMeta.Unk_Res[1] & 0xFFFF);
            }
            out.write((char*)&s6, 2);
            out.write((char*)&s7, 2);

            uint32_t outPrio = (e.Priority == 0) ? 1 : e.Priority;
            out.write((char*)&outPrio, 4);

            out.write((char*)&e.LoopCount, 4);
            out.write((char*)&e.PitchSend, 2);
            out.write((char*)&e.GainSend, 2);

            uint16_t vol = (uint16_t)(e.Volume * 127.0f + 0.5f);
            uint16_t pit = (uint16_t)(e.Pitch * 100.0f + 0.5f);
            uint16_t pvar = (uint16_t)(e.PitchVar * 100.0f + 0.5f);

            out.write((char*)&vol, 2);
            out.write((char*)&pit, 2);
            out.write((char*)&pvar, 2);

            uint16_t flags = 0;

            if (e.OriginalIndex != -1) {
                if (e.RawMeta.Driver.ControlMask & 0x10) {
                    if (e.RawMeta.Driver.Flags & 1) flags |= 0x01;
                    if (e.RawMeta.Driver.Flags & 2) flags |= 0x02;
                }

                if (e.RawMeta.Driver.FlagCheck2 != 0) {
                    flags |= 0x04;
                }

                if ((e.RawMeta.Driver.ControlMask & 0x400) == 0 || e.RawMeta.Driver.FlagCheck1 != 2) {
                    flags |= 0x08;
                }

                if (e.RawMeta.Driver.ControlMask & 0x80) {
                    flags |= 0x10;
                }

                if (e.RawMeta.Driver.ControlMask & 0x100) {
                    flags |= 0x20;
                }
            }
            else {
                if (e.Flag_Reverb) flags |= 0x01;
                if (e.Flag_Occlusion) flags |= 0x02;
                flags |= 0x04;
                if (e.Flag_Interrupt) flags |= 0x08;
                if (e.Flag_UseMinDist) flags |= 0x10;
                if (e.Flag_UseMaxDist) flags |= 0x20;
            }

            out.write((char*)&flags, 2);

            out.write((char*)&e.MinDist, 4);
            out.write((char*)&e.MaxDist, 4);

            uint32_t prob = (e.Probability != 100.0f) ? (uint32_t)(100.0f / e.Probability) : e.RawProbability;
            if (prob == 0xFFFFFFFF || prob == 0) {
                prob = 1;
            }
            out.write((char*)&prob, 4);
        }

        uint32_t trigCount = (uint32_t)lugParser.Scripts.size();
        out.write((char*)&trigCount, 4);

        for (const auto& s : lugParser.Scripts) {
            WriteString(out, s.Name);
            uint32_t memCount = (uint32_t)s.SoundIDs.size();
            out.write((char*)&memCount, 4);
            for (uint32_t id : s.SoundIDs) {
                out.write((char*)&id, 4);
            }
        }

        out.close();
        return true;
    }
};