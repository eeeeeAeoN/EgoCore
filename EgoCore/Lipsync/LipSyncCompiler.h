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

        struct TempHeaderBIG { char m[4]; uint32_t v; uint32_t footOff; uint32_t footSz; };
        TempHeaderBIG header = { {'B','I','G','B'}, 100, 0, 0 };
        out.write((char*)&header, sizeof(header));

        std::vector<std::vector<EntryWriteData>> finalSubBankEntries;

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
            InternalBankInfo& info = state.SubBanks[i];
            std::map<uint32_t, EntryWriteData> mergedEntries;

            state.Stream->clear();
            state.Stream->seekg(info.Offset, std::ios::beg);

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

            if (state.PendingAdds.count(i)) {
                for (const auto& [id, addData] : state.PendingAdds[i]) {
                    EntryWriteData newItem;
                    newItem.ID = id;
                    newItem.Type = addData.Type;
                    newItem.Name = addData.NamePrefix + "_" + std::to_string(id);
                    newItem.CRC = 0;
                    newItem.Raw = addData.Raw;
                    newItem.Info = addData.Info;

                    if (mergedEntries.count(id)) newItem.Deps = mergedEntries[id].Deps;
                    else if (!addData.Dependencies.empty()) newItem.Deps = addData.Dependencies;
                    else newItem.Deps.push_back("SPEAKER_FEMALE1");

                    mergedEntries[id] = newItem;
                }
            }

            if (state.PendingDeletes.count(i)) {
                for (uint32_t delID : state.PendingDeletes[i]) mergedEntries.erase(delID);
            }

            std::vector<EntryWriteData> sortedList;
            for (auto& pair : mergedEntries) sortedList.push_back(pair.second);

            for (auto& entry : sortedList) {
                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }

            finalSubBankEntries.push_back(sortedList);
        }

        out.seekp(sizeof(TempHeaderBIG), std::ios::beg);

        std::vector<std::vector<uint32_t>> savedOffsets;

        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            std::vector<uint32_t> bankOffsets;
            for (auto& entry : finalSubBankEntries[i]) {

                uint32_t pos = (uint32_t)out.tellp();
                uint32_t pad = (pos % 4 != 0) ? (4 - (pos % 4)) : 0;
                if (pad > 0) { std::vector<char> z(pad, 0); out.write(z.data(), pad); }

                uint32_t finalOffset = (uint32_t)out.tellp();
                bankOffsets.push_back(finalOffset);
                out.write((char*)entry.Raw.data(), entry.Raw.size());
            }
            savedOffsets.push_back(bankOffsets);
        }

        std::vector<uint32_t> tableOffsets;
        std::vector<uint32_t> tableSizes;

        for (int i = 0; i < (int)finalSubBankEntries.size(); i++) {
            uint32_t pos = (uint32_t)out.tellp();
            while (pos % 4 != 0) { out.put(0); pos++; }

            uint32_t tStart = (uint32_t)out.tellp();

            uint32_t u1 = 1;
            uint32_t u2 = 1;
            uint32_t cnt = (uint32_t)finalSubBankEntries[i].size();
            out.write((char*)&u1, 4);
            out.write((char*)&u2, 4);
            out.write((char*)&cnt, 4);

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

                uint32_t flags = (e.Raw.size() > 0) ? 2 : 1;
                out.write((char*)&flags, 4);
                out.write((char*)&unk, 4);

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

        uint32_t realFootOff = (uint32_t)out.tellp();
        header.footOff = realFootOff;
        uint32_t bankCount = (uint32_t)state.SubBanks.size();
        out.write((char*)&bankCount, 4);

        for (int i = 0; i < (int)state.SubBanks.size(); i++) {
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