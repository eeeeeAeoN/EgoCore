#pragma once
#include "BankBackend.h"
#include "LipSyncProperties.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <map>

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

    // --- HELPER: GET ENTRY DATA ---
    struct EntryWriteData {
        std::vector<uint8_t> Raw;
        std::vector<uint8_t> Info;
        bool IsModified;
    };

    // Helper to fetch data from the state or file
    inline EntryWriteData GetEntryDataFromState(LipSyncState& state, int sbIdx, uint32_t id, uint32_t offset, uint32_t size, uint32_t infoOffset, uint32_t infoSize) {
        EntryWriteData result;
        result.IsModified = false;

        // 1. Check Pending Adds (Modified/New Data)
        if (state.PendingAdds.count(sbIdx) && state.PendingAdds[sbIdx].count(id)) {
            const auto& added = state.PendingAdds[sbIdx][id];
            result.Raw = added.Raw;
            result.Info = added.Info;
            result.IsModified = true;
            return result;
        }

        // 2. Fallback: Read from Original File
        if (result.Raw.empty() && size > 0) {
            result.Raw.resize(size);
            state.Stream->clear();
            state.Stream->seekg(offset, std::ios::beg);
            state.Stream->read((char*)result.Raw.data(), size);
        }

        if (result.Info.empty() && infoSize > 0) {
            result.Info.resize(infoSize);
            state.Stream->clear();
            state.Stream->seekg(infoOffset, std::ios::beg);
            state.Stream->read((char*)result.Info.data(), infoSize);
        }

        return result;
    }

    // --- MAIN COMPILER FUNCTION FOR STATE ---
    inline bool CompileLipSyncFromState(LipSyncState& state) {
        std::cout << "[COMPILER] Starting Recompile of: " << state.FilePath << std::endl;

        if (!state.Stream || !state.Stream->is_open()) {
            std::cout << "[COMPILER] ERROR: Input stream not open." << std::endl;
            return false;
        }

        std::string tempPath = state.FilePath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) {
            std::cout << "[COMPILER] ERROR: Could not open temp file: " << tempPath << std::endl;
            return false;
        }

        // 1. HEADER
        // FIX: Version is now 100 (0x64), not 1
        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, 100, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<std::vector<BankEntry>> finalSubBankEntries;
        state.Stream->clear();

        // 2. WRITE DATA BLOBS
        std::cout << "[COMPILER] Processing " << state.SubBanks.size() << " subbanks..." << std::endl;

        for (int sbIdx = 0; sbIdx < (int)state.SubBanks.size(); sbIdx++) {
            const auto& info = state.SubBanks[sbIdx];
            std::cout << "[COMPILER]   SubBank " << sbIdx << ": " << info.Name << " (" << info.EntryCount << " entries)" << std::endl;

            std::vector<BankEntry> currentEntries;
            uint32_t align = info.Align;
            if (align < 1) align = 2048;

            // Read original entries list
            state.Stream->seekg(info.Offset, std::ios::beg);
            uint32_t val = 0; state.Stream->read((char*)&val, 4);
            if (val == 42) state.Stream->seekg(-4, std::ios::cur);
            else if (val < 1000) state.Stream->seekg(val * 8, std::ios::cur);
            else state.Stream->seekg(-4, std::ios::cur);

            for (uint32_t k = 0; k < info.EntryCount; k++) {
                BankEntry e; uint32_t magic;
                state.Stream->read((char*)&magic, 4); state.Stream->read((char*)&e.ID, 4);
                state.Stream->read((char*)&e.Type, 4); state.Stream->read((char*)&e.Size, 4);
                state.Stream->read((char*)&e.Offset, 4); state.Stream->read((char*)&e.CRC, 4);

                if (magic == 42) {
                    e.Name = ReadBankString(*state.Stream);
                    state.Stream->seekg(4, std::ios::cur);
                    uint32_t depC = 0; state.Stream->read((char*)&depC, 4);
                    for (uint32_t d = 0; d < depC; d++) e.Dependencies.push_back(ReadBankString(*state.Stream));
                }
                state.Stream->read((char*)&e.InfoSize, 4);
                e.SubheaderFileOffset = (uint32_t)state.Stream->tellg();
                if (e.InfoSize > 0) state.Stream->seekg(e.InfoSize, std::ios::cur);
                currentEntries.push_back(e);
            }

            // Write Data & Apply Pending Changes
            std::vector<BankEntry> writtenEntries;

            // Process Existing
            for (const auto& e : currentEntries) {
                if (state.PendingDeletes[sbIdx].count(e.ID)) {
                    std::cout << "[COMPILER]     Skipping Deleted ID: " << e.ID << std::endl;
                    continue;
                }

                // Alignment
                uint32_t currentPos = (uint32_t)out.tellp();
                if (align > 1 && (currentPos % align) != 0) {
                    uint32_t padBytes = align - (currentPos % align);
                    std::vector<char> zeros(padBytes, 0);
                    out.write(zeros.data(), padBytes);
                }

                BankEntry finalE = e;
                finalE.Offset = (uint32_t)out.tellp();

                EntryWriteData data = GetEntryDataFromState(state, sbIdx, e.ID, e.Offset, e.Size, e.SubheaderFileOffset, e.InfoSize);

                if (!data.Raw.empty()) out.write((char*)data.Raw.data(), data.Raw.size());

                finalE.Size = (uint32_t)data.Raw.size();
                finalE.InfoSize = (uint32_t)data.Info.size();
                writtenEntries.push_back(finalE);
            }

            // Process New Adds
            if (state.PendingAdds.count(sbIdx)) {
                for (const auto& [id, data] : state.PendingAdds[sbIdx]) {
                    // Check if we already wrote it (overwrite case)
                    bool alreadyWritten = false;
                    for (const auto& w : writtenEntries) if (w.ID == id) alreadyWritten = true;

                    if (!alreadyWritten) {
                        std::cout << "[COMPILER]     Writing NEW ID: " << id << std::endl;

                        BankEntry newE;
                        newE.ID = id;
                        newE.Type = data.Type;

                        // FIX: Standard naming [BankName]_[ID]
                        newE.Name = data.NamePrefix + "_" + std::to_string(id);

                        // FIX: Hardcoded Speaker
                        newE.Dependencies.push_back("SPEAKER_FEMALE1");

                        // Alignment
                        uint32_t currentPos = (uint32_t)out.tellp();
                        if (align > 1 && (currentPos % align) != 0) {
                            uint32_t padBytes = align - (currentPos % align);
                            std::vector<char> zeros(padBytes, 0);
                            out.write(zeros.data(), padBytes);
                        }

                        newE.Offset = (uint32_t)out.tellp();
                        out.write((char*)data.Raw.data(), data.Raw.size());
                        newE.Size = (uint32_t)data.Raw.size();
                        newE.InfoSize = (uint32_t)data.Info.size();
                        writtenEntries.push_back(newE);
                    }
                }
            }
            finalSubBankEntries.push_back(writtenEntries);
        }

        // 3. WRITE ENTRY TABLES
        std::cout << "[COMPILER] Writing Entry Tables..." << std::endl;
        std::vector<uint32_t> entryTableOffsets;
        std::vector<uint32_t> entryTableSizes;

        for (int sbIdx = 0; sbIdx < (int)finalSubBankEntries.size(); sbIdx++) {
            // Align Table
            uint32_t align = state.SubBanks[sbIdx].Align;
            if (align < 1) align = 2048;

            uint32_t currentPos = (uint32_t)out.tellp();
            if (align > 1 && (currentPos % align) != 0) {
                uint32_t padBytes = align - (currentPos % align);
                std::vector<char> zeros(padBytes, 0);
                out.write(zeros.data(), padBytes);
            }

            uint32_t startPos = (uint32_t)out.tellp();
            entryTableOffsets.push_back(startPos);

            const auto& entries = finalSubBankEntries[sbIdx];

            // Map
            std::map<uint32_t, uint32_t> typeCounts;
            for (const auto& e : entries) typeCounts[e.Type]++;
            uint32_t mapSize = (uint32_t)typeCounts.size();
            out.write((char*)&mapSize, 4);
            for (const auto& [type, count] : typeCounts) {
                out.write((char*)&type, 4);
                out.write((char*)&count, 4);
            }

            // Entries
            for (const auto& e : entries) {
                uint32_t magic = 42;
                out.write((char*)&magic, 4); out.write((char*)&e.ID, 4);
                out.write((char*)&e.Type, 4); out.write((char*)&e.Size, 4);
                out.write((char*)&e.Offset, 4); out.write((char*)&e.CRC, 4);
                WriteBankString(out, e.Name);
                uint32_t pad = 0; out.write((char*)&pad, 4);
                uint32_t depCount = (uint32_t)e.Dependencies.size();
                out.write((char*)&depCount, 4);
                for (const auto& s : e.Dependencies) WriteBankString(out, s);
                out.write((char*)&e.InfoSize, 4);

                if (e.InfoSize > 0) {
                    bool infoWritten = false;
                    if (state.PendingAdds.count(sbIdx) && state.PendingAdds[sbIdx].count(e.ID)) {
                        const auto& info = state.PendingAdds[sbIdx][e.ID].Info;
                        out.write((char*)info.data(), info.size());
                        infoWritten = true;
                    }
                    if (!infoWritten) {
                        std::vector<uint8_t> infoBuf(e.InfoSize);
                        state.Stream->clear();
                        state.Stream->seekg(e.SubheaderFileOffset, std::ios::beg);
                        state.Stream->read((char*)infoBuf.data(), e.InfoSize);
                        out.write((char*)infoBuf.data(), e.InfoSize);
                    }
                }
            }

            uint32_t endPos = (uint32_t)out.tellp();
            uint32_t tableSize = endPos - startPos;
            entryTableSizes.push_back(tableSize);
            std::cout << "[COMPILER]   Table " << sbIdx << " written. Size: " << tableSize << " Offset: " << startPos << std::endl;
        }

        // 4. FOOTER
        std::cout << "[COMPILER] Writing Footer..." << std::endl;
        header.footOff = (uint32_t)out.tellp();
        uint32_t bankCount = (uint32_t)state.SubBanks.size();
        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo info = state.SubBanks[i];
            info.EntryCount = (uint32_t)finalSubBankEntries[i].size();
            info.Offset = entryTableOffsets[i];
            info.Size = entryTableSizes[i];

            WriteNullTermString(out, info.Name);
            out.write((char*)&info.Version, 4); out.write((char*)&info.EntryCount, 4);
            out.write((char*)&info.Offset, 4); out.write((char*)&info.Size, 4); out.write((char*)&info.Align, 4);
        }

        // 5. FINALIZE
        std::streampos endPos = out.tellp();
        header.footSz = (uint32_t)(endPos - (std::streampos)header.footOff);
        out.seekp(0);
        out.write((char*)&header, sizeof(header));
        out.close();
        state.Stream->close();

        std::cout << "[COMPILER] Replacing file..." << std::endl;
        try {
            std::filesystem::remove(state.FilePath);
            std::filesystem::rename(tempPath, state.FilePath);
            std::cout << "[COMPILER] Success!" << std::endl;
            state.Stream = std::make_unique<std::fstream>(state.FilePath, std::ios::binary | std::ios::in);
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cout << "[COMPILER] FileSystem Error: " << e.what() << std::endl;
            return false;
        }
    }
}