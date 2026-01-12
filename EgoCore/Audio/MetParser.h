#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

#pragma pack(push, 1)

// --- RAW STRUCTURES (On Disk) ---

struct MetFileHeader {
    uint32_t Version;
    uint32_t ResourceCount;
};

struct MetResourceHeader {
    uint32_t ResourceID;
    uint32_t PathLength;
};

struct MetResourceData {
    uint32_t LugSize;
    uint32_t LugOffset;
    uint16_t Channels;
    uint16_t FormatTag;
    uint32_t SampleRate;
    uint32_t Term1;
    uint32_t Term2;
};

// 32-Byte Common Body
struct MetLogicBodyCommon {
    uint16_t Gain;          // Offset 0
    uint16_t Pitch;         // Offset 2
    uint16_t PitchVar;      // Offset 4
    uint16_t Flags;         // Offset 6
    float    MinDist;       // Offset 8
    float    MaxDist;       // Offset 12
    uint32_t SoundID;       // Offset 16
    uint32_t ResourceID;    // Offset 20
    uint32_t ResourceID_2;  // Offset 24
    uint32_t DependencyLen; // Offset 28
};

struct MetCurveEntry {
    uint32_t ID_A; uint32_t Z1; uint32_t Z2;
    uint16_t VMax; uint16_t VDef; uint16_t VMin;
    uint32_t Flags; float FA; float FB; uint16_t UnkS;
    uint32_t S1; uint32_t S2; uint32_t P1; uint32_t P2;
};

#pragma pack(pop)

// --- PARSED DATA (In Memory) ---

struct MetResource {
    uint32_t ID;
    std::string SourcePath;
    MetResourceData Data;
};

struct MetLogic {
    uint32_t LoopCount = 0; // In Format C, this holds Priority
    MetLogicBodyCommon Body;
    std::string DependencyName;
    std::vector<uint8_t> TailData;
};

struct MetScript {
    std::string Name;
    std::vector<uint32_t> SoundIDs;
};

class MetParser {
public:
    std::string FileName;
    bool IsLoaded = false;

    std::string CategoryName;
    uint32_t GlobalPriority = 0;
    uint32_t HeaderTerminator = 0xFFFFFFFF;

    // 0 = Standard (12 byte tail)
    // 1 = Extended (16 byte tail) 
    // 2 = Generic  (12 byte header, conditional tail)
    int FormatType = 0;

    std::vector<MetResource> Resources;
    std::vector<MetLogic> LogicEntries;
    std::vector<MetCurveEntry> Curves;
    std::vector<MetScript> Scripts;

    bool Parse(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return false;
        FileName = path;

        Resources.clear();
        LogicEntries.clear();
        Curves.clear();
        Scripts.clear();

        // 1. FILE HEADER
        MetFileHeader fh;
        f.read((char*)&fh, sizeof(fh));
        if (fh.ResourceCount > 50000) return false;

        // 2. RESOURCES
        for (uint32_t i = 0; i < fh.ResourceCount; i++) {
            MetResource r;
            MetResourceHeader rh;
            f.read((char*)&rh, sizeof(rh));
            r.ID = rh.ResourceID;

            if (rh.PathLength > 0 && rh.PathLength < 2048) {
                std::vector<char> buf(rh.PathLength);
                f.read(buf.data(), rh.PathLength);
                r.SourcePath.assign(buf.data(), rh.PathLength);
            }
            f.read((char*)&r.Data, sizeof(MetResourceData));
            Resources.push_back(r);
        }

        // 3. LOGIC HEADER
        if (f.peek() == EOF) { IsLoaded = true; return true; }

        uint32_t logicCount = 0;
        f.read((char*)&logicCount, 4);
        if (logicCount > 200000) return false;

        uint32_t unk1 = 0;
        f.read((char*)&unk1, 4);

        if (unk1 == 1) {
            // STANDARD / EXTENDED FORMAT
            uint32_t unk2; f.read((char*)&unk2, 4);

            uint32_t nameLen; f.read((char*)&nameLen, 4);
            if (nameLen > 0 && nameLen < 1024) {
                std::vector<char> buf(nameLen);
                f.read(buf.data(), nameLen);
                CategoryName.assign(buf.data(), nameLen);
            }

            uint32_t unk3; f.read((char*)&unk3, 4);
            f.read((char*)&GlobalPriority, 4);
            f.read((char*)&HeaderTerminator, 4);

            // DETECTION LOGIC
            if (HeaderTerminator == 0) FormatType = 2; // Generic (Rare case)
            else if (unk3 == 1) FormatType = 1;        // Extended (16 Byte Tail)
            else FormatType = 0;                       // Standard (12 Byte Tail)
        }
        else {
            // GENERIC FORMAT (Format C)
            // Rewind the 4 bytes we peeked (they belong to Entry 1 Header)
            FormatType = 2;
            f.seekg(-4, std::ios::cur);
        }

        // 4. LOGIC ENTRIES
        for (uint32_t i = 0; i < logicCount; i++) {
            MetLogic l;

            // ENTRY HEADER
            if (FormatType == 2) {
                // 12 Bytes: [Priority] [0] [0]
                uint32_t v1, v2, v3;
                f.read((char*)&v1, 4);
                f.read((char*)&v2, 4);
                f.read((char*)&v3, 4);
                l.LoopCount = v1;
            }
            else {
                // 4 Bytes: [LoopCount]
                f.read((char*)&l.LoopCount, 4);
            }

            // BODY
            f.read((char*)&l.Body, sizeof(MetLogicBodyCommon));

            // STRING
            if (l.Body.DependencyLen > 0 && l.Body.DependencyLen < 2048) {
                std::vector<char> buf(l.Body.DependencyLen);
                f.read(buf.data(), l.Body.DependencyLen);
                l.DependencyName.assign(buf.data(), l.Body.DependencyLen);
            }
            else if (l.Body.DependencyLen >= 2048) {
                l.Body.DependencyLen = 0; // Safety skip
            }

            // TAIL
            if (FormatType == 2) {
                // Generic: 4 bytes ONLY if string exists
                if (l.Body.DependencyLen > 0) {
                    l.TailData.resize(4);
                    f.read((char*)l.TailData.data(), 4);
                }
            }
            else if (FormatType == 1) {
                // Extended: 16 bytes tail (Matches Dumps #1, #3)
                l.TailData.resize(16);
                f.read((char*)l.TailData.data(), 16);
            }
            else {
                // Standard: 12 bytes tail
                l.TailData.resize(12);
                f.read((char*)l.TailData.data(), 12);
            }

            LogicEntries.push_back(l);
        }

        // 5. SCRIPTS / CURVES
        if (f.peek() == EOF) { IsLoaded = true; return true; }

        uint32_t nextCount = 0;
        f.read((char*)&nextCount, 4);

        std::streampos pos = f.tellg();
        uint32_t peekVal = 0;
        f.read((char*)&peekVal, 4);
        f.seekg(pos);

        bool isScriptTable = (peekVal > 0 && peekVal < 500);

        if (!isScriptTable) {
            // Parse Curves
            uint32_t curveCount = nextCount;
            if (curveCount < 20000) {
                for (uint32_t k = 0; k < curveCount; k++) {
                    MetCurveEntry c;
                    f.read((char*)&c, sizeof(MetCurveEntry));
                    Curves.push_back(c);
                }
            }
            if (f.peek() != EOF) f.read((char*)&nextCount, 4);
            else nextCount = 0;
        }

        // Parse Scripts
        uint32_t scriptCount = nextCount;
        if (scriptCount > 0 && scriptCount < 50000) {
            for (uint32_t k = 0; k < scriptCount; k++) {
                MetScript s;
                uint32_t nLen = 0;
                f.read((char*)&nLen, 4);
                if (nLen > 0 && nLen < 512) {
                    std::vector<char> buf(nLen);
                    f.read(buf.data(), nLen);
                    s.Name.assign(buf.data(), nLen);
                }
                uint32_t idCount = 0;
                f.read((char*)&idCount, 4);

                if (idCount > 0 && idCount < 2000) {
                    s.SoundIDs.resize(idCount);
                    f.read((char*)s.SoundIDs.data(), idCount * 4);
                }
                Scripts.push_back(s);
            }
        }

        IsLoaded = true;
        return true;
    }

    const MetLogic* FindLogicForResource(uint32_t resID) {
        for (const auto& l : LogicEntries) {
            if (l.Body.ResourceID == resID) return &l;
        }
        return nullptr;
    }
};