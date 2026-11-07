// zlib/DEFLATE decompression used by the def binary reader.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fable::defs::compiler {

/// Inflate a zlib stream (`78 01`-style header included). Returns false and
/// fills `error` on malformed input.
bool zlib_inflate(const std::uint8_t *data, std::size_t size,
                  std::vector<std::uint8_t> &out, std::string &error);

} // namespace fable::defs::compiler
