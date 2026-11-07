#include "fable/defs/compiler/names.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace fable::defs::compiler {

namespace {

std::uint32_t crc32_of(std::string_view text) {
    std::uint32_t crc = 0;
    for (unsigned char byte : text) {
        std::uint32_t c = (crc ^ byte) & 0xFF;
        for (int i = 0; i < 8; ++i) {
            c = (c & 1) != 0 ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc = c ^ (crc >> 8);
    }
    return crc;
}

void append_u32(std::vector<std::uint8_t> &out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 24));
}

} // namespace

std::uint32_t NamesBuilder::intern(std::string_view name) {
    auto it = offsets_.find(name);
    if (it != offsets_.end()) {
        return it->second;
    }
    // Each entry is 4-byte CRC + string + NUL. The recorded offset is the
    // string start, which is why the first name lives at offset 4 rather than
    // 0 (the first four content bytes are its CRC).
    std::uint32_t offset = stream_length_ + 4;
    std::string owned(name);
    offsets_.emplace(owned, offset);
    stream_length_ += 4 + static_cast<std::uint32_t>(owned.size()) + 1;
    names_.push_back(std::move(owned));
    return offset;
}

std::optional<std::uint32_t> NamesBuilder::find(std::string_view name) const {
    auto it = offsets_.find(name);
    if (it == offsets_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::uint8_t> NamesBuilder::to_bytes(
    const std::array<std::uint8_t, 20> &header) const {
    std::vector<std::uint8_t> out(header.begin(), header.end());
    for (const std::string &name : names_) {
        append_u32(out, crc32_of(name));
        out.insert(out.end(), name.begin(), name.end());
        out.push_back(0);
    }

    const std::uint32_t count = static_cast<std::uint32_t>(names_.size());
    // `CDefStringTable` writes `StringCount` at +8 and `StreamLength` at +12;
    // the latter is `file_size - 16` (header plus four leading bytes).
    const std::uint32_t stream_len = 4 + stream_length_;
    for (int i = 0; i < 4; ++i) {
        out[8 + i] = static_cast<std::uint8_t>(count >> (i * 8));
        out[12 + i] = static_cast<std::uint8_t>(stream_len >> (i * 8));
    }
    return out;
}

} // namespace fable::defs::compiler
