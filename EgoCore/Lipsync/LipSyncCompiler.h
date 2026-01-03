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
        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, 2, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<uint32_t> entryTableOffsets;
        std::vector<uint32_t> entryTableSizes;
        std::vector<std::vector<EntryWriteData>> finalSubBankEntries;

        // 2. PROCESS EACH SUB-BANK
        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo& info = state.SubBanks[i];

            // Map to store Final Entries for this sub-bank (ID -> Data)
            std::map<uint32_t, EntryWriteData> mergedEntries;

            // A. READ EXISTING ENTRIES FROM DISK
            state.Stream->clear();
            state.Stream->seekg(info.Offset, std::ios::beg);

            // Skip stats header
            uint32_t checkVal = 0; state.Stream->read((char*)&checkVal, 4);
            if (checkVal == 42) state.Stream->seekg(-4, std::ios::cur);
            else if (checkVal < 1000) state.Stream->seekg(checkVal * 8, std::ios::cur);
            else state.Stream->seekg(-4, std::ios::cur);

            for (uint32_t k = 0; k < info.EntryCount; k++) {
                EntryWriteData e;
                uint32_t magicE;
                state.Stream->read((char*)&magicE, 4);
                state.Stream->read((char*)&e.ID, 4);
                state.Stream->read((char*)&e.Type, 4);

                uint32_t size, offset;
                state.Stream->read((char*)&size, 4);
                state.Stream->read((char*)&offset, 4);
                state.Stream->read((char*)&e.CRC, 4);

                if (magicE != 42) continue; // Skip garbage

                // Read Metadata
                e.Name = ReadBankString(*state.Stream);

                // Read Dependencies
                state.Stream->seekg(4, std::ios::cur); // unknown
                uint32_t depCount = 0; state.Stream->read((char*)&depCount, 4);
                for (uint32_t d = 0; d < depCount; d++) e.Deps.push_back(ReadBankString(*state.Stream));

                // Read Info
                uint32_t infoSize = 0; state.Stream->read((char*)&infoSize, 4);
                std::streampos infoPos = state.Stream->tellg();

                // --- LOAD RAW DATA ---
                // Store file coordinates to read later (optimization) OR read now.
                // For safety, let's read now (memory intensive but safer).
                e.Raw.resize(size);
                std::streampos retPos = state.Stream->tellg();

                state.Stream->seekg(offset, std::ios::beg);
                state.Stream->read((char*)e.Raw.data(), size);

                state.Stream->seekg(infoPos, std::ios::beg);
                e.Info.resize(infoSize);
                state.Stream->read((char*)e.Info.data(), infoSize);

                mergedEntries[e.ID] = e;
            }

            // B. APPLY PENDING ADDS/MODIFICATIONS
            // This overwrites existing IDs and adds new ones
            if (state.PendingAdds.count(i)) {
                for (const auto& [id, addData] : state.PendingAdds[i]) {
                    EntryWriteData newItem;
                    newItem.ID = id;
                    newItem.Type = addData.Type;
                    // Name construction: PREFIX_ID
                    newItem.Name = addData.NamePrefix + "_" + std::to_string(id);
                    // CRC needs to be calculated? The game might ignore it or we can calc it.
                    // For now use 0 or calc simple hash.
                    newItem.CRC = 0;
                    newItem.Raw = addData.Raw;
                    newItem.Info = addData.Info;

                    // Dependencies? Usually none for raw lipsync unless linked.
                    // We preserve deps if it was an edit, otherwise empty.
                    if (mergedEntries.count(id)) {
                        newItem.Deps = mergedEntries[id].Deps;
                    }

                    mergedEntries[id] = newItem; // Insert or Overwrite
                }
            }

            // C. APPLY PENDING DELETES
            if (state.PendingDeletes.count(i)) {
                for (uint32_t delID : state.PendingDeletes[i]) {
                    mergedEntries.erase(delID);
                }
            }

            // D. WRITE DATA BLOBS
            std::vector<EntryWriteData> sortedList;
            for (auto& pair : mergedEntries) sortedList.push_back(pair.second);

            // Write Raw Blobs immediately to linear space
            for (auto& entry : sortedList) {
                // Align 2048
                uint32_t pos = (uint32_t)out.tellp();
                uint32_t pad = (pos % 2048 != 0) ? (2048 - (pos % 2048)) : 0;
                if (pad > 0) { std::vector<char> z(pad, 0); out.write(z.data(), pad); }

                uint32_t writeOffset = (uint32_t)out.tellp();
                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }

            finalSubBankEntries.push_back(sortedList);
        }

        // Reset OUT to after header
        out.seekp(sizeof(TempHeaderBIG), std::ios::beg);

        std::vector<std::vector<uint32_t>> savedOffsets; // [BankIdx][EntryIdx]

        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            std::vector<uint32_t> bankOffsets;
            for (auto& entry : finalSubBankEntries[i]) {
                uint32_t pos = (uint32_t)out.tellp();
                uint32_t pad = (pos % 2048 != 0) ? (2048 - (pos % 2048)) : 0;
                if (pad > 0) { std::vector<char> z(pad, 0); out.write(z.data(), pad); }

                uint32_t finalOffset = (uint32_t)out.tellp();
                bankOffsets.push_back(finalOffset);

                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }
            savedOffsets.push_back(bankOffsets);
        }

        // 4. WRITE FOOTER
        header.footOff = (uint32_t)out.tellp();
        uint32_t bankCount = (uint32_t)state.SubBanks.size();
        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo info = state.SubBanks[i];
        }

        std::vector<uint32_t> tableOffsets;
        std::vector<uint32_t> tableSizes;

        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            uint32_t tStart = (uint32_t)out.tellp();

            // Write Stats (dummy)
            uint32_t stats = 0; out.write((char*)&stats, 4);

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
                out.write((char*)&offsets[k], 4); // The calculated data offset
                out.write((char*)&e.CRC, 4);

                WriteBankString(out, e.Name);

                uint32_t unk = 0; out.write((char*)&unk, 4);

                uint32_t depCount = (uint32_t)e.Deps.size();
                out.write((char*)&depCount, 4);
                for (const auto& d : e.Deps) WriteBankString(out, d);

                uint32_t infoSz = (uint32_t)e.Info.size();
                out.write((char*)&infoSz, 4);
                out.write((char*)e.Info.data(), infoSz);
            }

            uint32_t tEnd = (uint32_t)out.tellp();
            tableOffsets.push_back(tStart);
            tableSizes.push_back(tEnd - tStart);
        }

        // NOW write the Bank List (The Footer proper)
        uint32_t realFootOff = (uint32_t)out.tellp();
        header.footOff = realFootOff;

        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo info = state.SubBanks[i];
            info.EntryCount = (uint32_t)finalSubBankEntries[i].size();
            info.Offset = tableOffsets[i];
            info.Size = tableSizes[i]; // Size of the table, not the data

            WriteNullTermString(out, info.Name);
            out.write((char*)&info.Version, 4);
            out.write((char*)&info.EntryCount, 4);
            out.write((char*)&info.Offset, 4);
            out.write((char*)&info.Size, 4);
            out.write((char*)&info.Align, 4);
        }

        std::streampos finalPos = out.tellp();
        header.footSz = (uint32_t)(finalPos - (std::streampos)realFootOff);

        // Update Header
        out.seekp(0);
        out.write((char*)&header, sizeof(header));
        out.close();

        // CLOSE INPUT
        state.Stream->close();

        // RENAME
        try {
            std::filesystem::remove(state.FilePath);
            std::filesystem::rename(tempPath, state.FilePath);
            return true;
        }
        catch (...) {
            return false;
        }
    }
}