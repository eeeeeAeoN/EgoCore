#include "fable/defs/compiler/wire.hpp"

#include <cstdint>
#include <string_view>

namespace fable::defs::compiler {

std::uint32_t crc32(std::string_view text) {
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

} // namespace fable::defs::compiler
