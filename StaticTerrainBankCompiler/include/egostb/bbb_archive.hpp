#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace egostb {

struct BBBEntry {
    uint32_t id = 0;
    uint32_t type = 0;
    uint32_t offset = 0;
    uint32_t crc = 0;
    std::string name;
    
    uint32_t devId = 0;
    uint32_t devType = 1;
    std::string devName;
    
    std::vector<uint8_t> payload;
    std::vector<uint8_t> extra;
};

class BBBArchive {
public:
    uint32_t version1 = 115; // 0x73
    uint32_t version2 = 174; // 0xAE
    uint32_t version3 = 1;
    uint32_t alignment = 2048; // 0x800

    std::vector<BBBEntry> entries;

    static BBBArchive read(const std::filesystem::path& path);
    void write(const std::filesystem::path& path) const;
    
    std::vector<uint8_t> serialize() const;
};

} // namespace egostb
