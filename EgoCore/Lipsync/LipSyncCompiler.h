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

    inline EntryWriteData GetEntryData(LoadedBank* bank, int sbIdx, const BankEntry& e, bool isActiveSubBank, int entryIndex) {
        EntryWriteData result;
        result.IsModified = false;

        // 1. Check LipSync Specific Pending Adds
        // FIX: Remove 'isActiveSubBank' check here. PendingAdds handles all subbanks.
        if (g_LipSyncState.PendingAdds.count(sbIdx)) {
            if (g_LipSyncState.PendingAdds[sbIdx].count(e.ID)) {
                const auto& added = g_LipSyncState.PendingAdds[sbIdx][e.ID];
                result.Raw = added.Raw;
                result.Info = added.Info;
                result.IsModified = true;
                return result;
            }
        }

        // 2. Check Generic Modified Data (Only for active subbank in UI)
        if (isActiveSubBank && entryIndex != -1 && bank->ModifiedEntryData.count(entryIndex)) {
            result.Raw = bank->ModifiedEntryData[entryIndex];
            result.IsModified = true;
        }

        // 3. Fallback: Read from Original File
        if (result.Raw.empty() && e.Size > 0) {
            result.Raw.resize(e.Size);
            bank->Stream->clear();
            bank->Stream->seekg(e.Offset, std::ios::beg);
            bank->Stream->read((char*)result.Raw.data(), e.Size);
        }

        if (result.Info.empty() && e.InfoSize > 0) {
            result.Info.resize(e.InfoSize);
            bank->Stream->clear();
            bank->Stream->seekg(e.SubheaderFileOffset, std::ios::beg);
            bank->Stream->read((char*)result.Info.data(), e.InfoSize);
        }

        return result;
    }

    inline bool CompileLipSyncBank(LoadedBank* bank) {
        if (!bank || !bank->Stream->is_open()) return false;

        std::string tempPath = bank->FullPath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        // 1. HEADER
        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, bank->FileVersion, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<std::vector<BankEntry>> finalSubBankEntries;
        bank->Stream->clear();

        // 2. WRITE DATA BLOBS
        for (int sbIdx = 0; sbIdx < (int)bank->SubBanks.size(); sbIdx++) {
            std::vector<BankEntry> currentEntries;
            uint32_t align = bank->SubBanks[sbIdx].Align;
            if (align < 1) align = 2048;

            bool isActiveSubBank = (sbIdx == bank->ActiveSubBankIndex);

            if (isActiveSubBank) {
                currentEntries = bank->Entries;
            }
            else {
                const auto& info = bank->SubBanks[sbIdx];
                bank->Stream->seekg(info.Offset, std::ios::beg);

                uint32_t val = 0; bank->Stream->read((char*)&val, 4);
                if (val == 42) bank->Stream->seekg(-4, std::ios::cur);
                else if (val < 1000) bank->Stream->seekg(val * 8, std::ios::cur);
                else bank->Stream->seekg(-4, std::ios::cur);

                for (uint32_t k = 0; k < info.EntryCount; k++) {
                    BankEntry e; uint32_t magic;
                    bank->Stream->read((char*)&magic, 4); bank->Stream->read((char*)&e.ID, 4);
                    bank->Stream->read((char*)&e.Type, 4); bank->Stream->read((char*)&e.Size, 4);
                    bank->Stream->read((char*)&e.Offset, 4); bank->Stream->read((char*)&e.CRC, 4);

                    if (magic == 42) {
                        e.Name = ReadBankString(*bank->Stream);
                        bank->Stream->seekg(4, std::ios::cur);
                        uint32_t depC = 0; bank->Stream->read((char*)&depC, 4);
                        for (uint32_t d = 0; d < depC; d++) e.Dependencies.push_back(ReadBankString(*bank->Stream));
                    }

                    bank->Stream->read((char*)&e.InfoSize, 4);
                    e.SubheaderFileOffset = (uint32_t)bank->Stream->tellg();
                    if (e.InfoSize > 0) bank->Stream->seekg(e.InfoSize, std::ios::cur);

                    currentEntries.push_back(e);
                }
            }

            std::vector<BankEntry> writtenEntries;
            for (int i = 0; i < (int)currentEntries.size(); ++i) {
                auto& e = currentEntries[i];

                if (g_LipSyncState.PendingDeletes[sbIdx].count(e.ID)) continue;

                uint32_t currentPos = (uint32_t)out.tellp();
                if (align > 1 && (currentPos % align) != 0) {
                    uint32_t padBytes = align - (currentPos % align);
                    std::vector<char> zeros(padBytes, 0);
                    out.write(zeros.data(), padBytes);
                }

                BankEntry finalE = e;
                finalE.Offset = (uint32_t)out.tellp();

                int lookupIndex = isActiveSubBank ? i : -1;
                EntryWriteData data = GetEntryData(bank, sbIdx, e, isActiveSubBank, lookupIndex);

                if (!data.Raw.empty()) {
                    out.write((char*)data.Raw.data(), data.Raw.size());
                }
                finalE.Size = (uint32_t)data.Raw.size();
                finalE.InfoSize = (uint32_t)data.Info.size();

                writtenEntries.push_back(finalE);
            }

            // Handle NEW Additions
            if (g_LipSyncState.PendingAdds.count(sbIdx)) {
                for (const auto& [id, data] : g_LipSyncState.PendingAdds[sbIdx]) {
                    bool exists = false;
                    for (const auto& existing : writtenEntries) if (existing.ID == id) exists = true;

                    if (!exists) {
                        BankEntry newE;
                        newE.ID = id;
                        newE.Type = 1;
                        newE.Name = "";

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
        std::vector<uint32_t> entryTableOffsets;
        for (int sbIdx = 0; sbIdx < (int)finalSubBankEntries.size(); sbIdx++) {
            entryTableOffsets.push_back((uint32_t)out.tellp());
            const auto& entries = finalSubBankEntries[sbIdx];
            bool isActiveSubBank = (sbIdx == bank->ActiveSubBankIndex);

            std::map<uint32_t, uint32_t> typeCounts;
            for (const auto& e : entries) typeCounts[e.Type]++;

            uint32_t mapSize = (uint32_t)typeCounts.size();
            out.write((char*)&mapSize, 4);
            for (const auto& [type, count] : typeCounts) {
                out.write((char*)&type, 4);
                out.write((char*)&count, 4);
            }

            for (int i = 0; i < (int)entries.size(); ++i) {
                const auto& e = entries[i];
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
                    // Refetch Info (Safe logic)
                    // If entry is from PendingAdds, fetch from there.
                    // If entry is original/modified, check memory or disk.
                    // Using ID lookup is safest here.

                    bool infoWritten = false;
                    if (g_LipSyncState.PendingAdds.count(sbIdx) && g_LipSyncState.PendingAdds[sbIdx].count(e.ID)) {
                        const auto& info = g_LipSyncState.PendingAdds[sbIdx][e.ID].Info;
                        if (info.size() == e.InfoSize) {
                            out.write((char*)info.data(), info.size());
                            infoWritten = true;
                        }
                    }

                    if (!infoWritten) {
                        std::vector<uint8_t> infoBuf(e.InfoSize);
                        bank->Stream->clear();
                        bank->Stream->seekg(e.SubheaderFileOffset, std::ios::beg);
                        bank->Stream->read((char*)infoBuf.data(), e.InfoSize);
                        out.write((char*)infoBuf.data(), e.InfoSize);
                    }
                }
            }
        }

        // 4. FOOTER
        header.footOff = (uint32_t)out.tellp();
        uint32_t bankCount = (uint32_t)bank->SubBanks.size();
        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)bank->SubBanks.size(); i++) {
            InternalBankInfo info = bank->SubBanks[i];
            info.EntryCount = (uint32_t)finalSubBankEntries[i].size();
            info.Offset = entryTableOffsets[i];

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

        // Safety: ensure main stream is closed before replacing
        bank->Stream->close();

        try {
            std::filesystem::remove(bank->FullPath);
            std::filesystem::rename(tempPath, bank->FullPath);
            return true;
        }
        catch (...) {
            // If rename fails, try to cleanup temp
            std::filesystem::remove(tempPath);
            return false;
        }
    }
}