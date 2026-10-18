#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace albion::lzo1x {

enum class Status { Ok, InputOverrun, OutputOverrun, LookbehindOverrun, Corrupt, InputNotConsumed };

Status decompress(const uint8_t* in, size_t inLen, uint8_t* out, size_t* outLen);
const char* statusName(Status s);
std::vector<uint8_t> compress(const uint8_t* in, size_t inLen);

} // namespace albion::lzo1x
