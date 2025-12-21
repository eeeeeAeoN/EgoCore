#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <iomanip>

struct BinaryEntry {
    uint32_t CRC;
    int32_t ID;
};

struct BinaryData {
    std::string FileName;
    std::vector<BinaryEntry> Entries;
    bool IsParsed = false;
};

class BinaryParser {
public:
    BinaryData Data;
    std::string StatusMessage;

    void Parse(const std::string& path) {
        Data = BinaryData(); // Reset
        Data.FileName = path.substr(path.find_last_of("/\\") + 1);
        StatusMessage = "Loading...";

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            StatusMessage = "Failed to open file.";
            return;
        }

        // Read Count
        int32_t count = 0;
        file.read((char*)&count, 4);

        // Basic sanity check
        if (count < 0 || count > 2000000) {
            StatusMessage = "Invalid header or empty file (Count: " + std::to_string(count) + ")";
            return;
        }

        // Read Entries
        Data.Entries.reserve(count);
        for (int i = 0; i < count; i++) {
            BinaryEntry e;
            file.read((char*)&e.CRC, 4);
            file.read((char*)&e.ID, 4);

            if (file.gcount() != 4) break; // EOF check
            Data.Entries.push_back(e);
        }

        Data.IsParsed = true;
        StatusMessage = "Loaded " + std::to_string(Data.Entries.size()) + " entries from " + Data.FileName;
    }
};