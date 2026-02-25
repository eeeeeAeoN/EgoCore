#pragma once
#include "BankBackend.h"
#include <fstream>
#include <vector>
#include <string>

class BigBankCompiler {
public:
    static bool Compile(LoadedBank* bank) {
        if (!bank || !bank->Stream) return false;

        std::string tempPath = bank->FullPath + ".tmp";
        std::ofstream out(tempPath, std::ios::binary);
        if (!out.is_open()) return false;

        std::vector<char> zeroBuffer(2048, 0);
        std::vector<char> dataBuffer;
        dataBuffer.reserve(1024 * 1024);

        struct BIGHeader { char m[4] = { 'B','I','G','B' }; uint32_t v = 0; uint32_t fOff = 0; uint32_t fSize = 0; } header;
        header.v = bank->FileVersion;
        out.write((char*)&header, sizeof(header));

        uint32_t globalAlign = 2048;
        uint32_t pos = (uint32_t)out.tellp();
        if (pos < globalAlign) {
            out.write(zeroBuffer.data(), globalAlign - pos);
        }

        uint32_t currentDataOffset = (uint32_t)out.tellp();

        struct CompiledEntry {
            BankEntry Entry;
            std::vector<uint8_t> RawInfo;
        };

        struct CompiledSubBank {
            InternalBankInfo Info;
            std::vector<CompiledEntry> Entries;
        };

        std::vector<CompiledSubBank> newSubBanks;

        // Process Data
        for (int i = 0; i < (int)bank->SubBanks.size(); ++i) {
            CompiledSubBank csb;
            csb.Info = bank->SubBanks[i];

            uint32_t sbAlign = csb.Info.Align;
            if (sbAlign == 0) sbAlign = 1;

            if (i == bank->ActiveSubBankIndex) {
                // ACTIVE
                for (int k = 0; k < (int)bank->Entries.size(); ++k) {
                    CompiledEntry ce;
                    ce.Entry = bank->Entries[k];
                    if (bank->SubheaderCache.count(k)) ce.RawInfo = bank->SubheaderCache[k];

                    char* srcPtr = nullptr;
                    size_t srcSize = 0;

                    if (bank->ModifiedEntryData.count(k)) {
                        srcPtr = (char*)bank->ModifiedEntryData[k].data();
                        srcSize = bank->ModifiedEntryData[k].size();
                    }
                    else if (ce.Entry.Size > 0) {
                        if (dataBuffer.size() < ce.Entry.Size) dataBuffer.resize(ce.Entry.Size);
                        bank->Stream->clear();
                        bank->Stream->seekg(ce.Entry.Offset, std::ios::beg);
                        bank->Stream->read(dataBuffer.data(), ce.Entry.Size);
                        srcPtr = dataBuffer.data();
                        srcSize = ce.Entry.Size;
                    }

                    if (sbAlign > 1) {
                        uint32_t p = (uint32_t)out.tellp();
                        uint32_t r = p % sbAlign;
                        if (r != 0) {
                            out.write(zeroBuffer.data(), sbAlign - r);
                            currentDataOffset += (sbAlign - r);
                        }
                    }

                    ce.Entry.Offset = currentDataOffset;
                    ce.Entry.Size = (uint32_t)srcSize;
                    if (srcSize > 0 && srcPtr) out.write(srcPtr, srcSize);
                    currentDataOffset += ce.Entry.Size;

                    csb.Entries.push_back(ce);
                }
            }
            else {
                // INACTIVE
                bank->Stream->clear();
                bank->Stream->seekg(csb.Info.Offset, std::ios::beg);

                uint32_t statsCount = 0; bank->Stream->read((char*)&statsCount, 4);
                // Capture Header for Inactive Subbanks too
                csb.Info.HeaderData.clear();
                if (statsCount < 1000) {
                    csb.Info.HeaderData.push_back(statsCount);
                    for (uint32_t k = 0; k < statsCount * 2; ++k) {
                        uint32_t val; bank->Stream->read((char*)&val, 4);
                        csb.Info.HeaderData.push_back(val);
                    }
                }
                else {
                    bank->Stream->seekg(-4, std::ios::cur);
                    csb.Info.HeaderData.push_back(0);
                }

                // Read Entries
                for (uint32_t e = 0; e < csb.Info.EntryCount; ++e) {
                    CompiledEntry ce;
                    uint32_t magic;
                    bank->Stream->read((char*)&magic, 4);
                    bank->Stream->read((char*)&ce.Entry.ID, 4);
                    bank->Stream->read((char*)&ce.Entry.Type, 4);
                    bank->Stream->read((char*)&ce.Entry.Size, 4);
                    bank->Stream->read((char*)&ce.Entry.Offset, 4);
                    bank->Stream->read((char*)&ce.Entry.CRC, 4);

                    ce.Entry.Name = ReadBankString(*bank->Stream);
                    bank->Stream->read((char*)&ce.Entry.Timestamp, 4);

                    uint32_t depCount = 0; bank->Stream->read((char*)&depCount, 4);
                    for (uint32_t d = 0; d < depCount; d++) ce.Entry.Dependencies.push_back(ReadBankString(*bank->Stream));

                    bank->Stream->read((char*)&ce.Entry.InfoSize, 4);
                    if (ce.Entry.InfoSize > 0) {
                        ce.RawInfo.resize(ce.Entry.InfoSize);
                        bank->Stream->read((char*)ce.RawInfo.data(), ce.Entry.InfoSize);
                    }

                    if (magic == 42) csb.Entries.push_back(ce);
                }

                // Copy Blobs
                for (auto& ce : csb.Entries) {
                    if (sbAlign > 1) {
                        uint32_t p = (uint32_t)out.tellp();
                        uint32_t r = p % sbAlign;
                        if (r != 0) {
                            out.write(zeroBuffer.data(), sbAlign - r);
                            currentDataOffset += (sbAlign - r);
                        }
                    }

                    uint32_t newOffset = currentDataOffset;
                    if (ce.Entry.Size > 0) {
                        bank->Stream->seekg(ce.Entry.Offset, std::ios::beg);
                        if (dataBuffer.size() < ce.Entry.Size) dataBuffer.resize(ce.Entry.Size);
                        bank->Stream->read(dataBuffer.data(), ce.Entry.Size);
                        out.write(dataBuffer.data(), ce.Entry.Size);
                    }
                    ce.Entry.Offset = newOffset;
                    currentDataOffset += ce.Entry.Size;
                }
            }
            csb.Info.EntryCount = (uint32_t)csb.Entries.size();
            newSubBanks.push_back(csb);
        }

        // Align before ToC
        uint32_t tocPos = (uint32_t)out.tellp();
        if (tocPos % 2048 != 0) {
            out.write(zeroBuffer.data(), 2048 - (tocPos % 2048));
        }

        // 4. Write Entry Tables
        for (auto& sb : newSubBanks) {
            sb.Info.Offset = (uint32_t)out.tellp();

            // [FIX] Write Preserved Header or Regenerate
            uint32_t preservedSum = 0;
            if (sb.Info.HeaderData.size() >= 1) {
                uint32_t cnt = sb.Info.HeaderData[0];
                if (sb.Info.HeaderData.size() == 1 + cnt * 2) {
                    for (uint32_t k = 0; k < cnt; ++k) preservedSum += sb.Info.HeaderData[2 + k * 2];
                }
            }

            if (preservedSum == sb.Entries.size() && !sb.Info.HeaderData.empty()) {
                out.write((char*)sb.Info.HeaderData.data(), sb.Info.HeaderData.size() * 4);
            }
            else {
                // Mismatch/Missing -> Synthesize simple header
                uint32_t h[3] = { 1, 0, (uint32_t)sb.Entries.size() };
                out.write((char*)h, 12);
            }

            // Write Entries
            for (const auto& ce : sb.Entries) {
                uint32_t magic = 42;
                out.write((char*)&magic, 4);
                out.write((char*)&ce.Entry.ID, 4);
                out.write((char*)&ce.Entry.Type, 4);
                out.write((char*)&ce.Entry.Size, 4);
                out.write((char*)&ce.Entry.Offset, 4);
                out.write((char*)&ce.Entry.CRC, 4);
                WriteBankString(out, ce.Entry.Name);
                out.write((char*)&ce.Entry.Timestamp, 4);

                uint32_t dCount = (uint32_t)ce.Entry.Dependencies.size();
                out.write((char*)&dCount, 4);
                for (const auto& s : ce.Entry.Dependencies) WriteBankString(out, s);

                out.write((char*)&ce.Entry.InfoSize, 4);
                if (ce.Entry.InfoSize > 0 && !ce.RawInfo.empty()) {
                    out.write((char*)ce.RawInfo.data(), ce.RawInfo.size());
                }
            }
            uint32_t endPos = (uint32_t)out.tellp();
            sb.Info.Size = endPos - sb.Info.Offset;
        }

        uint32_t footerStart = (uint32_t)out.tellp();

        uint32_t bankCount = (uint32_t)newSubBanks.size();
        out.write((char*)&bankCount, 4);

        for (const auto& sb : newSubBanks) {
            WriteNullTermString(out, sb.Info.Name);
            out.write((char*)&sb.Info.Version, 4);
            out.write((char*)&sb.Info.EntryCount, 4);
            out.write((char*)&sb.Info.Offset, 4);
            out.write((char*)&sb.Info.Size, 4);
            out.write((char*)&sb.Info.Align, 4);
        }

        uint32_t footerEnd = (uint32_t)out.tellp();
        uint32_t footerSize = footerEnd - footerStart;

        out.seekp(0, std::ios::beg);
        header.fOff = footerStart;
        header.fSize = footerSize;
        out.write((char*)&header, sizeof(header));

        out.close();
        bank->Stream->close();
        try {
            fs::copy_file(tempPath, bank->FullPath, fs::copy_options::overwrite_existing);
            fs::remove(tempPath);
            return true;
        }
        catch (...) { return false; }
    }

private:
    static void WriteBankString(std::ofstream& out, const std::string& s) {
        uint32_t len = (uint32_t)s.length();
        out.write((char*)&len, 4);
        if (len > 0) out.write(s.data(), len);
    }
    static void WriteNullTermString(std::ofstream& out, const std::string& s) {
        out.write(s.c_str(), s.length() + 1);
    }
    static std::string ReadBankString(std::fstream& file) {
        uint32_t len = 0; file.read((char*)&len, 4);
        if (len > 0) {
            std::string s(len, '\0'); file.read(&s[0], len);
            s.erase(std::find(s.begin(), s.end(), '\0'), s.end());
            return s;
        }
        return "";
    }
};