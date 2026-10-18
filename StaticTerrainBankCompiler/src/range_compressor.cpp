#include "egostb/range_compressor.hpp"
#include <cstring>
#include <algorithm>

namespace egostb {

static const uint32_t BlockSizeMasks[5] = {0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF};

std::vector<uint8_t> RangeCompressor::compress(const void* input, int elementCount, int elementSize, bool enableCompression) {
    if (elementCount <= 0 || elementSize <= 0) {
        return {0};
    }

    const uint8_t* inBytes = static_cast<const uint8_t*>(input);
    size_t rawSize = static_cast<size_t>(elementCount) * elementSize;

    if (!enableCompression) {
        std::vector<uint8_t> out;
        out.reserve(rawSize + 1);
        out.push_back(0); // 0x00 = Uncompressed raw stream
        out.insert(out.end(), inBytes, inBytes + rawSize);
        return out;
    }

    // Buffer to hold compressed stream starting with 0x01
    std::vector<uint8_t> out;
    out.reserve(rawSize + 32);
    out.push_back(1); // 0x01 = Range-compressed format

    // Determine sub-blocks: break elementSize into chunks of 4, 2, or 1 byte
    int offset = 0;
    while (offset < elementSize) {
        int blockSize = 4;
        if (elementSize - offset == 3) {
            blockSize = 2; // will do 2 then 1
        } else if (elementSize - offset < 4) {
            blockSize = elementSize - offset;
        }

        // Find min and max for this block
        uint32_t minVal = 0xFFFFFFFF;
        uint32_t maxVal = 0;

        for (int i = 0; i < elementCount; ++i) {
            const uint8_t* elemPtr = inBytes + i * elementSize + offset;
            uint32_t val = 0;
            if (blockSize == 4) {
                std::memcpy(&val, elemPtr, 4);
            } else if (blockSize == 2) {
                uint16_t v16 = 0;
                std::memcpy(&v16, elemPtr, 2);
                val = v16;
            } else {
                val = *elemPtr;
            }

            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }

        uint8_t flags = 0;
        if (blockSize == 4) flags |= 0x40;
        else if (blockSize == 2) flags |= 0x20;

        if (minVal == maxVal) {
            // Constant value: BitCount = 0, RangeBias = minVal
            flags |= 0x01; // has RangeBias
            out.push_back(flags);
            out.push_back(0); // bit_count = 0
            // Write RangeBias
            if (blockSize == 4) {
                uint8_t b[4]; std::memcpy(b, &minVal, 4);
                out.insert(out.end(), b, b + 4);
            } else if (blockSize == 2) {
                uint8_t b[2]; std::memcpy(b, &minVal, 2);
                out.insert(out.end(), b, b + 2);
            } else {
                out.push_back(static_cast<uint8_t>(minVal));
            }
        } else {
            uint32_t range = maxVal - minVal;
            int bitCount = 0;
            while ((1ULL << bitCount) <= range && bitCount < 32) {
                bitCount++;
            }

            flags |= 0x01; // has RangeBias
            out.push_back(flags);
            out.push_back(static_cast<uint8_t>(bitCount));

            // Write RangeBias
            if (blockSize == 4) {
                uint8_t b[4]; std::memcpy(b, &minVal, 4);
                out.insert(out.end(), b, b + 4);
            } else if (blockSize == 2) {
                uint8_t b[2]; std::memcpy(b, &minVal, 2);
                out.insert(out.end(), b, b + 2);
            } else {
                out.push_back(static_cast<uint8_t>(minVal));
            }

            // Bit-pack the values
            if (bitCount == 32) {
                for (int i = 0; i < elementCount; ++i) {
                    const uint8_t* elemPtr = inBytes + i * elementSize + offset;
                    uint32_t val = 0;
                    std::memcpy(&val, elemPtr, 4);
                    val -= minVal;
                    uint8_t b[4]; std::memcpy(b, &val, 4);
                    out.insert(out.end(), b, b + 4);
                }
            } else {
                uint32_t accum = 0;
                int bitsInAccum = 0;

                for (int i = 0; i < elementCount; ++i) {
                    const uint8_t* elemPtr = inBytes + i * elementSize + offset;
                    uint32_t val = 0;
                    if (blockSize == 4) std::memcpy(&val, elemPtr, 4);
                    else if (blockSize == 2) { uint16_t v = 0; std::memcpy(&v, elemPtr, 2); val = v; }
                    else val = *elemPtr;

                    val = (val - minVal) & BlockSizeMasks[blockSize];

                    accum |= (val << bitsInAccum);
                    bitsInAccum += bitCount;

                    if (bitsInAccum >= 32) {
                        uint8_t b[4]; std::memcpy(b, &accum, 4);
                        out.insert(out.end(), b, b + 4);
                        bitsInAccum -= 32;
                        accum = (val >> (bitCount - bitsInAccum));
                    }
                }

                if (bitsInAccum > 0) {
                    uint8_t b[4]; std::memcpy(b, &accum, 4);
                    out.insert(out.end(), b, b + 4);
                }
            }
        }

        offset += blockSize;
    }

    // End marker: 0x80
    out.push_back(0x80);

    // If compressed size is not smaller than raw, fallback to uncompressed format (0x00 + raw bytes)
    if (out.size() >= rawSize + 1) {
        out.clear();
        out.reserve(rawSize + 1);
        out.push_back(0); // 0x00 = Uncompressed raw stream
        out.insert(out.end(), inBytes, inBytes + rawSize);
    }

    return out;
}

static int calcShuffleOperations(uint32_t shuffle_mask, uint32_t* mask_ops, uint32_t* shift_ops) {
    int op_count = 0;
    int v5 = 0;
    int v6 = 0;
    while (v5 < 32) {
        if (((1u << v5) & shuffle_mask) != 0) {
            uint32_t v7 = 0;
            uint32_t v8 = 1u << v5;
            int v9 = v5;
            do {
                if ((v8 & shuffle_mask) == 0) break;
                v7 |= v8;
                v9++;
                v8 <<= 1;
            } while (v9 < 32);
            mask_ops[op_count] = v7;
            shift_ops[op_count] = v5 - v6;
            op_count++;
            v6 += (v9 - v5);
            v5 = v9;
        }
        v5++;
    }
    return op_count;
}

bool RangeCompressor::decompress(const uint8_t* input, size_t inputSize, void* output, int elementCount, int elementSize) {
    if (!input || !output || elementCount <= 0 || elementSize <= 0) return false;

    if (*input == 0) {
        // Raw uncompressed stream
        size_t rawSize = static_cast<size_t>(elementCount) * elementSize;
        if (inputSize < rawSize + 1) return false;
        std::memcpy(output, input + 1, rawSize);
        return true;
    }

    // Compressed stream
    int8_t flags = static_cast<int8_t>(input[1]);
    const uint8_t* ptr = input + 2;
    uint8_t* outBuf = static_cast<uint8_t*>(output);

    while (flags >= 0) {
        int blockSize = 1;
        if (flags & 0x40) blockSize = 4;
        else if (flags & 0x20) blockSize = 2;

        int bitCount = *ptr++;
        uint32_t rangeBias = 0;
        uint8_t rangeShift = 0;
        uint32_t orMask = 0;
        int shuffleOpCount = 0;
        uint32_t shuffleOpMasks[16] = {};
        uint32_t shuffleOpShifts[16] = {};

        if (flags & 0x11) {
            if (blockSize == 4) std::memcpy(&rangeBias, ptr, 4);
            else if (blockSize == 2) { uint16_t v = 0; std::memcpy(&v, ptr, 2); rangeBias = v; }
            else rangeBias = *ptr;
            ptr += blockSize;
        }

        if (flags & 2) {
            rangeShift = *ptr++;
        }

        if (flags & 8) {
            if (blockSize == 4) std::memcpy(&orMask, ptr, 4);
            else if (blockSize == 2) { uint16_t v = 0; std::memcpy(&v, ptr, 2); orMask = v; }
            else orMask = *ptr;
            ptr += blockSize;
        }

        if (flags & 4) {
            uint32_t shuffleMask = 0;
            if (blockSize == 4) std::memcpy(&shuffleMask, ptr, 4);
            else if (blockSize == 2) { uint16_t v = 0; std::memcpy(&v, ptr, 2); shuffleMask = v; }
            else shuffleMask = *ptr;
            shuffleOpCount = calcShuffleOperations(shuffleMask, shuffleOpMasks, shuffleOpShifts);
            ptr += blockSize;
        }

        int bitOffset = (bitCount == 0) ? 0 : 32;
        uint32_t dataMask = (bitCount == 32) ? 0xFFFFFFFF : ((1u << bitCount) - 1u);
        uint32_t dataIn = 0;

        uint8_t* elemDst = outBuf;
        for (int i = 0; i < elementCount; ++i) {
            uint32_t val = 0;
            if (bitCount == 32) {
                std::memcpy(&val, ptr, 4);
                ptr += 4;
            } else {
                if (bitOffset == 32) {
                    std::memcpy(&dataIn, ptr, 4);
                    ptr += 4;
                    bitOffset = 0;
                }
                uint32_t v20 = dataIn >> bitOffset;
                bitOffset += bitCount;
                val = dataMask & v20;
                if (bitOffset > 32) {
                    std::memcpy(&dataIn, ptr, 4);
                    ptr += 4;
                    val |= dataMask & (dataIn << (bitCount - bitOffset + 32));
                    bitOffset -= 32;
                }
            }

            if (flags & 1) {
                val = BlockSizeMasks[blockSize] & (rangeBias + val);
            }
            uint32_t v22 = val << rangeShift;
            if (flags & 4) {
                uint32_t v23 = 0;
                for (int k = 0; k < shuffleOpCount; ++k) {
                    v23 |= shuffleOpMasks[k] & (v22 << shuffleOpShifts[k]);
                }
                v22 = v23;
            }
            uint32_t v26 = orMask | v22;
            if (flags & 0x10) {
                v26 = BlockSizeMasks[blockSize] & (rangeBias + v26);
            }

            if (blockSize == 4) std::memcpy(elemDst, &v26, 4);
            else if (blockSize == 2) { uint16_t v = static_cast<uint16_t>(v26); std::memcpy(elemDst, &v, 2); }
            else *elemDst = static_cast<uint8_t>(v26);

            elemDst += elementSize;
        }

        outBuf += blockSize;
        flags = static_cast<int8_t>(*ptr++);
    }

    return true;
}

} // namespace egostb
