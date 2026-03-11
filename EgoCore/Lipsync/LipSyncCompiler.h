#pragma once
#include "BankBackend.h"
#include "LipSyncProperties.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <map>
#include <algorithm>

namespace LipSyncCompiler {

    // --- UTILS ---
    inline void WriteBankString(std::ofstream& out, const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        out.write((char*)&len, 4);
        if (len > 0) out.write(s.data(), len);
    }

    inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
        out.write(s.c_str(), s.length() + 1);
    }

    inline std::string ReadCompString(std::fstream& file) {
        uint32_t len = 0; file.read((char*)&len, 4);
        if (len > 0 && len < 4096) {
            std::string s(len, '\0'); file.read(&s[0], len);
            return s;
        }
        return "";
    }

    struct EntryWriteData {
        uint32_t ID;
        int32_t Type;
        uint32_t CRC;
        std::string Name;
        std::vector<uint8_t> Raw;
        std::vector<uint8_t> Info;
        std::vector<std::string> Deps;
    };

    inline bool CompileLipSyncFromState(LipSyncState& state) {
        if (!state.Stream || !state.Stream->is_open()) return false;

        std::string tempPath = state.FilePath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        // 1. HEADER PLACEHOLDER
        // Version 100 (0x64) matches Vanilla
        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, 100, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<std::vector<EntryWriteData>> finalSubBankEntries;

        // 2. PROCESS EACH SUB-BANK
        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo& info = state.SubBanks[i];
            std::map<uint32_t, EntryWriteData> mergedEntries;

            // A. READ EXISTING ENTRIES
            state.Stream->clear();
            state.Stream->seekg(info.Offset, std::ios::beg);

            // READ THE SUB-HEADER TO SKIP IT [1, 1, Count]
            uint32_t u1, u2, eCount;
            state.Stream->read((char*)&u1, 4);
            state.Stream->read((char*)&u2, 4);
            state.Stream->read((char*)&eCount, 4);

            for (uint32_t k = 0; k < eCount; k++) {
                EntryWriteData e;
                uint32_t magicE;
                state.Stream->read((char*)&magicE, 4);
                state.Stream->read((char*)&e.ID, 4);
                state.Stream->read((char*)&e.Type, 4);

                uint32_t size, offset;
                state.Stream->read((char*)&size, 4);
                state.Stream->read((char*)&offset, 4);
                state.Stream->read((char*)&e.CRC, 4);

                if (magicE != 42 && size == 0) continue;

                e.Name = ReadCompString(*state.Stream);

                uint32_t unk4, flags, unk6;
                state.Stream->read((char*)&unk4, 4);
                state.Stream->read((char*)&flags, 4);
                state.Stream->read((char*)&unk6, 4);

                // Flags=2 means it has dependency. Flags=1/0 usually empty/script.
                if (flags == 2) {
                    std::string dep = ReadCompString(*state.Stream);
                    if (!dep.empty()) e.Deps.push_back(dep);
                }

                uint32_t infoSize = 0; state.Stream->read((char*)&infoSize, 4);
                std::streampos infoPos = state.Stream->tellg();

                e.Raw.resize(size);
                state.Stream->seekg(offset, std::ios::beg);
                state.Stream->read((char*)e.Raw.data(), size);

                state.Stream->seekg(infoPos, std::ios::beg);
                e.Info.resize(infoSize);
                state.Stream->read((char*)e.Info.data(), infoSize);

                mergedEntries[e.ID] = e;
            }

            // B. APPLY PENDING ADDS/MODS
            if (state.PendingAdds.count(i)) {
                for (const auto& [id, addData] : state.PendingAdds[i]) {
                    EntryWriteData newItem;
                    newItem.ID = id;
                    newItem.Type = addData.Type;
                    newItem.Name = addData.NamePrefix + "_" + std::to_string(id);
                    newItem.CRC = 0;
                    newItem.Raw = addData.Raw;
                    newItem.Info = addData.Info;

                    // Ensure dependencies exist for new entries
                    if (mergedEntries.count(id)) newItem.Deps = mergedEntries[id].Deps;
                    else if (!addData.Dependencies.empty()) newItem.Deps = addData.Dependencies;
                    else newItem.Deps.push_back("SPEAKER_FEMALE1");

                    mergedEntries[id] = newItem;
                }
            }

            // C. APPLY PENDING DELETES
            if (state.PendingDeletes.count(i)) {
                for (uint32_t delID : state.PendingDeletes[i]) mergedEntries.erase(delID);
            }

            // D. STORE SORTED LIST
            std::vector<EntryWriteData> sortedList;
            for (auto& pair : mergedEntries) sortedList.push_back(pair.second);

            // WRITE RAW DATA IMMEDIATELY
            for (auto& entry : sortedList) {
                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }

            finalSubBankEntries.push_back(sortedList);
        }

        // RESET WRITE HEAD FOR PASS 2
        out.seekp(sizeof(TempHeaderBIG), std::ios::beg);

        std::vector<std::vector<uint32_t>> savedOffsets;

        // 3. WRITE RAW DATA AND RECORD OFFSETS
        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            std::vector<uint32_t> bankOffsets;
            for (auto& entry : finalSubBankEntries[i]) {

                // Align Data to 4 bytes
                uint32_t pos = (uint32_t)out.tellp();
                uint32_t pad = (pos % 4 != 0) ? (4 - (pos % 4)) : 0;
                if (pad > 0) { std::vector<char> z(pad, 0); out.write(z.data(), pad); }

                uint32_t finalOffset = (uint32_t)out.tellp();
                bankOffsets.push_back(finalOffset);
                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }
            savedOffsets.push_back(bankOffsets);
        }

        // 4. WRITE TABLES
        std::vector<uint32_t> tableOffsets;
        std::vector<uint32_t> tableSizes;

        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            // Align Table Start to 4 bytes (Critical for Fable)
            uint32_t pos = (uint32_t)out.tellp();
            while (pos % 4 != 0) { out.put(0); pos++; }

            uint32_t tStart = (uint32_t)out.tellp();

            // --- CRITICAL: WRITE SUB-HEADER [1, 1, COUNT] ---
            uint32_t u1 = 1;
            uint32_t u2 = 1;
            uint32_t cnt = (uint32_t)finalSubBankEntries[i].size();
            out.write((char*)&u1, 4);
            out.write((char*)&u2, 4);
            out.write((char*)&cnt, 4);
            // -------------------------------------------

            auto& entries = finalSubBankEntries[i];
            auto& offsets = savedOffsets[i];

            for (size_t k = 0; k < entries.size(); k++) {
                EntryWriteData& e = entries[k];
                uint32_t magic = 42;
                out.write((char*)&magic, 4);
                out.write((char*)&e.ID, 4);
                out.write((char*)&e.Type, 4);
                uint32_t sz = (uint32_t)e.Raw.size();
                out.write((char*)&sz, 4);
                out.write((char*)&offsets[k], 4);
                out.write((char*)&e.CRC, 4);

                WriteBankString(out, e.Name);

                uint32_t unk = 0; out.write((char*)&unk, 4);

                // Flags: 2 if valid/has deps, 1/0 if empty
                uint32_t flags = (e.Raw.size() > 0) ? 2 : 1;
                out.write((char*)&flags, 4);
                out.write((char*)&unk, 4);

                // Dependency (Only if flags == 2)
                if (flags == 2) {
                    if (e.Deps.empty()) WriteBankString(out, "SPEAKER_FEMALE1");
                    else WriteBankString(out, e.Deps[0]);
                }

                uint32_t infoSz = (uint32_t)e.Info.size();
                out.write((char*)&infoSz, 4);
                if (infoSz > 0) out.write((char*)e.Info.data(), infoSz);
            }

            uint32_t tEnd = (uint32_t)out.tellp();
            tableOffsets.push_back(tStart);
            tableSizes.push_back(tEnd - tStart);
        }

        // 5. WRITE FOOTER AND UPDATE STATE
        uint32_t realFootOff = (uint32_t)out.tellp();
        header.footOff = realFootOff;
        uint32_t bankCount = (uint32_t)state.SubBanks.size();
        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            // !!! CRITICAL FIX !!!
            // We use a reference (&) here so we update the LIVE state.
            // Previously we were copying it, writing new values to the copy, 
            // and leaving the memory state with old offsets.
            InternalBankInfo& info = state.SubBanks[i];

            info.EntryCount = (uint32_t)finalSubBankEntries[i].size();
            info.Offset = tableOffsets[i];
            info.Size = tableSizes[i];

            WriteNullTermString(out, info.Name);
            out.write((char*)&info.Version, 4);
            out.write((char*)&info.EntryCount, 4);
            out.write((char*)&info.Offset, 4);
            out.write((char*)&info.Size, 4);
            out.write((char*)&info.Align, 4);
        }

        std::streampos finalPos = out.tellp();
        header.footSz = (uint32_t)(finalPos - (std::streampos)realFootOff);
        out.seekp(0);
        out.write((char*)&header, sizeof(header));
        out.close();

        state.Stream->close();
        try {
            std::filesystem::remove(state.FilePath);
            std::filesystem::rename(tempPath, state.FilePath);
            return true;
        }
        catch (...) { return false; }
    }
}