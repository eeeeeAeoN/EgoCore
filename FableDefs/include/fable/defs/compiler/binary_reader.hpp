// Reader for the four compiled def binaries and the shared names table.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fable::defs::compiler {

struct NamesEntry {
    std::uint32_t offset = 0;
    std::uint32_t crc = 0;
    std::string string;
};

class Names {
public:
    static bool load(const std::string &path, Names &out, std::string &error);

    std::optional<std::string> find(std::uint32_t offset) const;
    const std::map<std::uint32_t, NamesEntry> &entries() const { return entries_; }
    void insert(NamesEntry entry) { entries_[entry.offset] = std::move(entry); }

private:
    std::map<std::uint32_t, NamesEntry> entries_;
};

struct SubDefRecord {
    std::uint32_t name_crc = 0;
    std::uint32_t def_index = 0;
    std::uint32_t owner_index = 0;
};

struct BinaryEntry {
    std::uint32_t global_index = 0;
    std::uint32_t class_index = 0;
    std::uint32_t def_name_offset = 0;
    std::uint32_t file_name_offset = 0;
    std::string def_name;
    std::string file_name;
    bool is_real = false;
    bool is_template = false;
    bool are_default_values_applied = false;
    std::optional<std::vector<SubDefRecord>> sub_defs;
    std::vector<std::uint8_t> body;
    std::vector<std::uint8_t> raw_bytes;
};

struct DefBinary {
    static bool load(const std::string &path, const Names &names, DefBinary &out,
                     std::string &error);

    std::uint32_t entry_count = 0;
    std::vector<BinaryEntry> entries;
};

bool read_file(const std::string &path, std::vector<std::uint8_t> &out,
               std::string &error);

} // namespace fable::defs::compiler
