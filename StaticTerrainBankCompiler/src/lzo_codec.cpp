#include "egostb/lzo_codec.hpp"
#include "minilzo.h"
#include <stdexcept>
#include <iostream>
#include <string>

namespace egostb {

static bool s_lzoInitialized = false;

bool LzoCodec::init() {
    if (s_lzoInitialized) return true;
    if (lzo_init() != LZO_E_OK) {
        return false;
    }
    s_lzoInitialized = true;
    return true;
}

std::vector<uint8_t> LzoCodec::decompress(const uint8_t* src, size_t srcLen, size_t expectedDstLen) {
    if (!init()) {
        throw std::runtime_error("LZO failed to initialize");
    }

    std::vector<uint8_t> dst(expectedDstLen);
    lzo_uint dstLen = expectedDstLen;
    int r = lzo1x_decompress(src, static_cast<lzo_uint>(srcLen), dst.data(), &dstLen, nullptr);
    if (r != LZO_E_OK) {
        throw std::runtime_error("LZO decompression error code: " + std::to_string(r));
    }
    if (dstLen != expectedDstLen) {
        dst.resize(dstLen);
    }
    return dst;
}

std::vector<uint8_t> LzoCodec::compress(const uint8_t* src, size_t srcLen) {
    if (!init()) {
        throw std::runtime_error("LZO failed to initialize");
    }

    // Allocate worst-case buffer: srcLen + srcLen / 16 + 64 + 3
    lzo_uint maxDstLen = srcLen + srcLen / 16 + 64 + 3;
    std::vector<uint8_t> dst(maxDstLen);
    std::vector<uint8_t> wrkmem(LZO1X_1_MEM_COMPRESS);

    lzo_uint outLen = 0;
    int r = lzo1x_1_compress(src, static_cast<lzo_uint>(srcLen), dst.data(), &outLen, wrkmem.data());
    if (r != LZO_E_OK) {
        throw std::runtime_error("LZO compression error code: " + std::to_string(r));
    }
    dst.resize(outLen);
    return dst;
}

} // namespace egostb
