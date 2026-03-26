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

    inline void WriteBankString(std::ofstream& out, const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        out.write((char*)&len, 4);
        if (len > 0) out.write(s.data(), len);
    }

    inline void WriteNullTermString(std::ofstream& out, const std::string& s) {
        out.write(s.c_str(), s.length() + 1);
    }

    inline bool CompileTextBank(LoadedBank* bank) {
        if (!bank || !bank->Stream->is_open()) return false;

        std::string tempPath = bank->FullPath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, bank->FileVersion, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<BankEntry> finalEntries;

        bank->Stream->clear();

        {
            auto& srcEntries = bank->Entries;
            uint32_t align = 1;
            if (!bank->SubBanks.empty()) align = bank->SubBanks[bank->ActiveSubBankIndex].Align;
            if (align < 1) align = 1;

            for (int i = 0; i < (int)srcEntries.size(); i++) {
                BankEntry e = srcEntries[i];

                uint32_t currentPos = (uint32_t)out.tellp();
                if (align > 1 && (currentPos % align) != 0) {
                    uint32_t padBytes = align - (currentPos % align);
                    std::vector<char> zeros(padBytes, 0);
                    out.write(zeros.data(), padBytes);
                }

                e.Offset = (uint32_t)out.tellp();

                if (bank->ModifiedEntryData.count(i)) {
                    auto& data = bank->ModifiedEntryData[i];
                    out.write((char*)data.data(), data.size());
                    e.Size = (uint32_t)data.size();
                }
                else {
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

        std::map<uint32_t, uint32_t> typeCounts;
        for (const auto& e : finalEntries) {
            typeCounts[e.Type]++;
        }

        uint32_t entryTableOffset = (uint32_t)out.tellp();

        uint32_t mapSize = (uint32_t)typeCounts.size();
        out.write((char*)&mapSize, 4);

        for (const auto& [type, count] : typeCounts) {
            out.write((char*)&type, 4);
            out.write((char*)&count, 4);
        }

        for (const auto& e : finalEntries) {
            uint32_t magic = 42;
            out.write((char*)&magic, 4); out.write((char*)&e.ID, 4);
            out.write((char*)&e.Type, 4); out.write((char*)&e.Size, 4);
            out.write((char*)&e.Offset, 4); out.write((char*)&e.CRC, 4);

            WriteBankString(out, e.Name);
            uint32_t pad = 0; out.write((char*)&pad, 4);

            uint32_t depCount = (uint32_t)e.Dependencies.size();
            out.write((char*)&depCount, 4);
            for (const auto& s : e.Dependencies) WriteBankString(out, s);

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

        header.footOff = (uint32_t)out.tellp();

        uint32_t bankCount = 1;
        out.write((char*)&bankCount, 4);

        if (!bank->SubBanks.empty()) {
            InternalBankInfo info = bank->SubBanks[bank->ActiveSubBankIndex];

            WriteNullTermString(out, info.Name);

            uint32_t totalEntries = (uint32_t)finalEntries.size();
            out.write((char*)&info.Version, 4);
            out.write((char*)&totalEntries, 4);
            out.write((char*)&entryTableOffset, 4);
            uint32_t currentPos = (uint32_t)out.tellp();
            uint32_t tableSize = (uint32_t)header.footOff - entryTableOffset;
            out.write((char*)&tableSize, 4);

            out.write((char*)&info.Align, 4);
        }

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