#pragma once
#include "BankBackend.h"
#include "TextParser.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <map>

namespace TextCompiler {

    // --- UTILS ---
    inline void WriteBankString(std::ofstream& out, const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        out.write((char*)&len, 4);
        if (len > 0) out.write(s.data(), len);
    }

    inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
        out.write(s.c_str(), s.length() + 1);
    }

    // --- MAIN COMPILER FUNCTION ---
    inline bool CompileTextBank(LoadedBank* bank) {
        if (!bank || !bank->Stream->is_open()) return false;

        std::string tempPath = bank->FullPath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        // 1. HEADER (Placeholder)
        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, bank->FileVersion, 0, 0 };
        out.write((char*)&header, sizeof(header));

        // We prepare the list of entries to write (Active Subbank only for text.big)
        std::vector<BankEntry> finalEntries;

        // 2. WRITE DATA BLOBS
        // We iterate the in-memory entries.
        // We write their content to the new file.
        // We update their Offsets in a COPY of the entry (finalEntries), preserving the original for now.

        bank->Stream->clear();

        {
            auto& srcEntries = bank->Entries; // The list currently in memory
            uint32_t align = 1;
            if (!bank->SubBanks.empty()) align = bank->SubBanks[bank->ActiveSubBankIndex].Align;
            if (align < 1) align = 1;

            for (int i = 0; i < (int)srcEntries.size(); i++) {
                BankEntry e = srcEntries[i]; // Copy the entry structure

                // Alignment Padding
                uint32_t currentPos = (uint32_t)out.tellp();
                if (align > 1 && (currentPos % align) != 0) {
                    uint32_t padBytes = align - (currentPos % align);
                    std::vector<char> zeros(padBytes, 0);
                    out.write(zeros.data(), padBytes);
                }

                // Record NEW Data Offset
                e.Offset = (uint32_t)out.tellp();

                // Write Data
                if (bank->ModifiedEntryData.count(i)) {
                    // Modified in tool
                    auto& data = bank->ModifiedEntryData[i];
                    out.write((char*)data.data(), data.size());
                    e.Size = (uint32_t)data.size();
                }
                else {
                    // Unmodified: Copy Raw from Original File
                    // Use the original offset from the source entry
                    uint32_t originalOffset = srcEntries[i].Offset;
                    uint32_t originalSize = srcEntries[i].Size;

                    if (originalSize > 0) {
                        std::vector<uint8_t> buffer(originalSize);
                        bank->Stream->seekg(originalOffset, std::ios::beg);
                        bank->Stream->read((char*)buffer.data(), originalSize);
                        out.write((char*)buffer.data(), originalSize);
                    }
                    e.Size = originalSize;
                }

                finalEntries.push_back(e);
            }
        }

        // --- NEW STEP: CALCULATE TYPE COUNTS ---
        // The game requires a map of {Type -> Count} before the entry list.
        std::map<uint32_t, uint32_t> typeCounts;
        for (const auto& e : finalEntries) {
            typeCounts[e.Type]++;
        }

        // 3. WRITE ENTRY TABLES
        // Note: text.big has one subbank, so we write one table offset.
        uint32_t entryTableOffset = (uint32_t)out.tellp();

        // A. Write Stats Map
        uint32_t mapSize = (uint32_t)typeCounts.size();
        out.write((char*)&mapSize, 4); // Number of types

        for (const auto& [type, count] : typeCounts) {
            out.write((char*)&type, 4);
            out.write((char*)&count, 4);
        }

        // B. Write Entries
        for (const auto& e : finalEntries) {
            uint32_t magic = 42;
            out.write((char*)&magic, 4); out.write((char*)&e.ID, 4);
            out.write((char*)&e.Type, 4); out.write((char*)&e.Size, 4);
            out.write((char*)&e.Offset, 4); out.write((char*)&e.CRC, 4);

            WriteBankString(out, e.Name);
            uint32_t pad = 0; out.write((char*)&pad, 4);

            // Dependencies
            uint32_t depCount = (uint32_t)e.Dependencies.size();
            out.write((char*)&depCount, 4);
            for (const auto& s : e.Dependencies) WriteBankString(out, s);

            // Info (Metadata)
            // Copy metadata if it exists (e.g. for Narrator List or others if needed)
            uint32_t isz = e.InfoSize;
            out.write((char*)&isz, 4);
            if (isz > 0) {
                std::vector<uint8_t> infoBuf(isz);
                bank->Stream->clear();
                bank->Stream->seekg(e.SubheaderFileOffset, std::ios::beg);
                bank->Stream->read((char*)infoBuf.data(), isz);
                out.write((char*)infoBuf.data(), isz);
            }
        }

        // 4. FOOTER (Bank List)
        header.footOff = (uint32_t)out.tellp();

        // We assume 1 subbank for text.big recompilation context
        uint32_t bankCount = 1;
        out.write((char*)&bankCount, 4);

        if (!bank->SubBanks.empty()) {
            InternalBankInfo info = bank->SubBanks[bank->ActiveSubBankIndex];

            WriteNullTermString(out, info.Name);

            // Update info with new values
            uint32_t totalEntries = (uint32_t)finalEntries.size();
            out.write((char*)&info.Version, 4);
            out.write((char*)&totalEntries, 4);

            // InfoOffset points to the start of the Table (where Map Size is)
            out.write((char*)&entryTableOffset, 4);

            // InfoSize calculation: (CurrentPos - entryTableOffset)
            uint32_t currentPos = (uint32_t)out.tellp();
            // The footer itself isn't included in InfoSize usually, InfoSize is the size of the Table.
            // But we haven't finished writing the footer line yet.
            // Let's look at open logic: InfoSize is usually ignored or just stored.
            // We'll calculate it as size of Step 3.
            uint32_t tableSize = (uint32_t)header.footOff - entryTableOffset;
            out.write((char*)&tableSize, 4);

            out.write((char*)&info.Align, 4);
        }

        // 5. FINALIZE HEADER
        std::streampos endPos = out.tellp();
        header.footSz = (uint32_t)(endPos - (std::streampos)header.footOff);

        out.seekp(0);
        out.write((char*)&header, sizeof(header));
        out.close();

        bank->Stream->close();
        std::filesystem::remove(bank->FullPath);
        std::filesystem::rename(tempPath, bank->FullPath);

        return true;
    }
}