// The shared names table (`names.bin`) builder.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fable::defs::compiler {

/// Interns UTF-8 names in first-seen order. The returned value is the byte
/// offset of the string in the `names.bin` content region (after the 20-byte
/// header), where each entry is `u32 crc32 + NUL-terminated UTF-8`.
class NamesBuilder {
public:
    std::uint32_t intern(std::string_view name);
    std::optional<std::uint32_t> find(std::string_view name) const;

    std::size_t size() const { return names_.size(); }
    /// Content bytes after the 20-byte header.
    std::uint32_t stream_length() const { return stream_length_; }
    const std::vector<std::string> &names() const { return names_; }

    /// Serialize with the supplied 20-byte base header. The string count and
    /// stream length words at offsets 8 and 12 are patched in, matching
    /// `CDefStringTable`.
    std::vector<std::uint8_t> to_bytes(const std::array<std::uint8_t, 20> &header) const;

private:
    std::map<std::string, std::uint32_t, std::less<>> offsets_;
    std::vector<std::string> names_;
    std::uint32_t stream_length_ = 0;
};

} // namespace fable::defs::compiler
