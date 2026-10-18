#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>

namespace egostb {

class RangeCompressor {
public:
    // Compress an array of elements (element_size in bytes, e.g. 16 for vertices, 2 for indices).
    // Returns the compressed byte stream (starts with 0x01 if compressed, or 0x00 if uncompressed raw).
    static std::vector<uint8_t> compress(const void* input, int elementCount, int elementSize, bool enableCompression = true);

    // Decompress an array of elements.
    // output must have size >= elementCount * elementSize.
    static bool decompress(const uint8_t* input, size_t inputSize, void* output, int elementCount, int elementSize);
};

} // namespace egostb
