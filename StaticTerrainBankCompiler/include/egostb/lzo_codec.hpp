#pragma once

#include <cstdint>
#include <vector>

namespace egostb {

class LzoCodec {
public:
    static bool init();
    static std::vector<uint8_t> decompress(const uint8_t* src, size_t srcLen, size_t expectedDstLen);
    static std::vector<uint8_t> compress(const uint8_t* src, size_t srcLen);
};

} // namespace egostb
