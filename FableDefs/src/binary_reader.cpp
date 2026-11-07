#include "fable/defs/compiler/binary_reader.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <string_view>

#include "fable/defs/compiler/inflate.hpp"
#include "fable/defs/compiler/manifest.hpp"

namespace fable::defs::compiler {
namespace {

bool take_u8(const std::vector<std::uint8_t> &data, std::size_t &pos,
             std::uint8_t &value) {
    if (pos >= data.size()) {
        return false;
    }
    value = data[pos++];
    return true;
}

bool take_u16(const std::vector<std::uint8_t> &data, std::size_t &pos,
              std::uint16_t &value) {
    if (pos + 2 > data.size()) {
        return false;
    }
    value = static_cast<std::uint16_t>(data[pos] | (data[pos + 1] << 8));
    pos += 2;
    return true;
}

bool take_u32(const std::vector<std::uint8_t> &data, std::size_t &pos,
              std::uint32_t &value) {
    if (pos + 4 > data.size()) {
        return false;
    }
    value = static_cast<std::uint32_t>(data[pos]) |
            (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
            (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
            (static_cast<std::uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return true;
}


bool parse_names(const std::vector<std::uint8_t> &data, Names &out,
                 std::string &error) {
    if (data.size() < 20) {
        error = "names.bin is shorter than its 20-byte header";
        return false;
    }
    std::size_t pos = 20;
    while (pos < data.size()) {
        const std::size_t entry_offset = pos;
        std::uint32_t crc = 0;
        if (!take_u32(data, pos, crc)) {
            error = "truncated names.bin CRC";
            return false;
        }
        const std::size_t text_start = pos;
        while (pos < data.size() && data[pos] != 0) {
            ++pos;
        }
        if (pos >= data.size()) {
            error = "unterminated name string in names.bin";
            return false;
        }
        const std::uint32_t offset =
            static_cast<std::uint32_t>(entry_offset + 4 - 20);
        NamesEntry entry;
        entry.offset = offset;
        entry.crc = crc;
        entry.string.assign(
            reinterpret_cast<const char *>(data.data() + text_start), pos - text_start);
        ++pos; // NUL
        out.insert(std::move(entry));
    }
    return true;
}

} // namespace

bool read_file(const std::string &path, std::vector<std::uint8_t> &out,
               std::string &error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (!file.eof() && file.fail()) {
        error = "cannot read " + path;
        return false;
    }
    return true;
}

bool Names::load(const std::string &path, Names &out, std::string &error) {
    std::vector<std::uint8_t> data;
    if (!read_file(path, data, error)) {
        return false;
    }
    out.entries_.clear();
    return parse_names(data, out, error);
}

std::optional<std::string> Names::find(std::uint32_t offset) const {
    const auto it = entries_.find(offset);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second.string;
}

bool DefBinary::load(const std::string &path, const Names &names, DefBinary &out,
                     std::string &error) {
    std::vector<std::uint8_t> data;
    if (!read_file(path, data, error)) {
        return false;
    }

    std::size_t pos = 0;
    std::uint8_t use_names_bin = 0;
    std::uint32_t file_indicator = 0;
    std::uint32_t platform_indicator = 0;
    std::uint32_t entry_count = 0;
    if (!take_u8(data, pos, use_names_bin) ||
        !take_u32(data, pos, file_indicator) ||
        !take_u32(data, pos, platform_indicator) ||
        !take_u32(data, pos, entry_count)) {
        error = "truncated def binary header";
        return false;
    }
    (void)use_names_bin;
    (void)file_indicator;
    (void)platform_indicator;

    struct NameRef {
        std::uint32_t def_name_offset = 0;
        std::uint32_t file_name_offset = 0;
        std::uint32_t counter = 0;
    };
    std::vector<NameRef> name_refs(entry_count);
    for (NameRef &ref : name_refs) {
        if (!take_u32(data, pos, ref.def_name_offset) ||
            !take_u32(data, pos, ref.file_name_offset) ||
            !take_u32(data, pos, ref.counter)) {
            error = "truncated NameRef table";
            return false;
        }
    }

    std::uint32_t chunk_count = 0;
    std::uint32_t chunk_reserved = 0;
    if (!take_u32(data, pos, chunk_count) || !take_u32(data, pos, chunk_reserved)) {
        error = "truncated chunk index header";
        return false;
    }
    (void)chunk_reserved;
    if (chunk_count == 0) {
        error = "chunk index contains no sentinel";
        return false;
    }

    struct ChunkIndexEntry {
        std::uint32_t compressed_offset = 0;
        std::uint32_t cumulative_entry_count = 0;
    };
    std::vector<ChunkIndexEntry> index(chunk_count - 1);
    for (ChunkIndexEntry &entry : index) {
        if (!take_u32(data, pos, entry.compressed_offset) ||
            !take_u32(data, pos, entry.cumulative_entry_count)) {
            error = "truncated chunk index entry";
            return false;
        }
    }
    ChunkIndexEntry sentinel;
    if (!take_u32(data, pos, sentinel.compressed_offset) ||
        !take_u32(data, pos, sentinel.cumulative_entry_count)) {
        error = "truncated chunk index sentinel";
        return false;
    }
    const std::uint32_t compressed_total = sentinel.compressed_offset;

    const std::size_t chunk_base = pos;
    std::vector<BinaryEntry> entries;
    entries.reserve(entry_count);
    std::uint32_t chunk_entry_base = 0;
    for (std::size_t chunk_index = 0; chunk_index < index.size(); ++chunk_index) {
        const std::uint32_t start = index[chunk_index].compressed_offset;
        const std::uint32_t end = chunk_index + 1 < index.size()
                                      ? index[chunk_index + 1].compressed_offset
                                      : compressed_total;
        if (start > end || chunk_base + end > data.size()) {
            error = "chunk offset is outside the def binary";
            return false;
        }
        std::vector<std::uint8_t> compressed(
            data.begin() + static_cast<std::ptrdiff_t>(chunk_base + start),
            data.begin() + static_cast<std::ptrdiff_t>(chunk_base + end));
        std::vector<std::uint8_t> decompressed;
        std::string inflate_error;
        if (!zlib_inflate(compressed.data(), compressed.size(), decompressed,
                          inflate_error)) {
            error = "chunk " + std::to_string(chunk_index) + ": " + inflate_error;
            return false;
        }

        const std::uint32_t chunk_entry_count =
            index[chunk_index].cumulative_entry_count - chunk_entry_base;
        std::vector<std::uint16_t> offsets(chunk_entry_count);
        std::size_t offset_pos = 0;
        for (std::uint16_t &offset : offsets) {
            if (!take_u16(decompressed, offset_pos, offset)) {
                error = "truncated entry offset table";
                return false;
            }
        }
        const std::size_t payload_base = chunk_entry_count * 2;

        for (std::uint32_t local = 0; local < chunk_entry_count; ++local) {
            const std::size_t entry_start = offsets[local];
            const std::size_t entry_end =
                local + 1 < chunk_entry_count ? offsets[local + 1] : decompressed.size();
            if (entry_start < payload_base || entry_start > entry_end ||
                entry_end > decompressed.size()) {
                error = "invalid entry offset";
                return false;
            }

            const std::uint32_t global_index = chunk_entry_base + local;
            if (global_index >= name_refs.size()) {
                error = "entry index exceeds NameRef table";
                return false;
            }
            const NameRef &ref = name_refs[global_index];
            const auto def_name = names.find(ref.def_name_offset);
            if (!def_name) {
                error = "entry " + std::to_string(global_index) +
                        " has no def name in names.bin";
                return false;
            }

            BinaryEntry entry;
            entry.global_index = global_index;
            entry.class_index = ref.counter;
            entry.def_name_offset = ref.def_name_offset;
            entry.file_name_offset = ref.file_name_offset;
            entry.def_name = *def_name;
            if (ref.file_name_offset != 0xFFFFFFFFu) {
                if (auto file_name = names.find(ref.file_name_offset)) {
                    entry.file_name = *file_name;
                }
            }

            std::size_t entry_pos = entry_start;
            std::uint8_t is_real = 0;
            std::uint8_t is_template = 0;
            std::uint8_t default_values = 0;
            if (!take_u8(decompressed, entry_pos, is_real) ||
                !take_u8(decompressed, entry_pos, is_template) ||
                !take_u8(decompressed, entry_pos, default_values)) {
                error = "truncated entry preamble";
                return false;
            }
            entry.is_real = is_real != 0;
            entry.is_template = is_template != 0;
            entry.are_default_values_applied = default_values != 0;

            if (has_subdef_table(entry.def_name)) {
                std::uint16_t count = 0;
                if (!take_u16(decompressed, entry_pos, count)) {
                    error = "truncated sub-def table";
                    return false;
                }
                std::vector<SubDefRecord> records;
                records.reserve(count);
                for (std::uint16_t i = 0; i < count; ++i) {
                    SubDefRecord record;
                    if (!take_u32(decompressed, entry_pos, record.name_crc) ||
                        !take_u32(decompressed, entry_pos, record.def_index) ||
                        !take_u32(decompressed, entry_pos, record.owner_index)) {
                        error = "truncated sub-def record";
                        return false;
                    }
                    records.push_back(record);
                }
                entry.sub_defs = std::move(records);
            }

            entry.raw_bytes.assign(
                decompressed.begin() + static_cast<std::ptrdiff_t>(entry_start),
                decompressed.begin() + static_cast<std::ptrdiff_t>(entry_end));
            entry.body.assign(
                decompressed.begin() + static_cast<std::ptrdiff_t>(entry_pos),
                decompressed.begin() + static_cast<std::ptrdiff_t>(entry_end));
            entries.push_back(std::move(entry));
        }
        chunk_entry_base = index[chunk_index].cumulative_entry_count;
    }

    out.entry_count = entry_count;
    out.entries = std::move(entries);
    return true;
}

} // namespace fable::defs::compiler
