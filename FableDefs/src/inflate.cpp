#include "fable/defs/compiler/inflate.hpp"

#include <array>
#include <cstring>
#include <limits>

namespace fable::defs::compiler {
namespace {

class BitReader {
public:
    BitReader(const std::uint8_t *data, std::size_t size)
        : data_(data), size_(size) {}

    bool bit(std::uint32_t &value) {
        if (bit_ >= size_ * 8) {
            return false;
        }
        value = (data_[bit_ / 8] >> (bit_ % 8)) & 1u;
        ++bit_;
        return true;
    }

    bool bits(unsigned count, std::uint32_t &value) {
        value = 0;
        for (unsigned i = 0; i < count; ++i) {
            std::uint32_t one = 0;
            if (!bit(one)) {
                return false;
            }
            value |= one << i;
        }
        return true;
    }

    bool align() {
        bit_ = (bit_ + 7) & ~static_cast<std::size_t>(7);
        return bit_ <= size_ * 8;
    }

    bool read_bytes(std::uint8_t *out, std::size_t count) {
        if (!align() || bit_ / 8 + count > size_) {
            return false;
        }
        std::memcpy(out, data_ + bit_ / 8, count);
        bit_ += count * 8;
        return true;
    }

private:
    const std::uint8_t *data_;
    std::size_t size_;
    std::size_t bit_ = 0;
};

struct Huffman {
    std::array<std::uint16_t, 16> count{};
    std::vector<std::uint16_t> symbol;
};

bool build_huffman(Huffman &table, const std::uint8_t *lengths, std::size_t n) {
    table.count.fill(0);
    for (std::size_t i = 0; i < n; ++i) {
        ++table.count[lengths[i]];
    }
    if (table.count[0] == n) {
        table.symbol.clear();
        return true;
    }

    int left = 1;
    for (int len = 1; len <= 15; ++len) {
        left <<= 1;
        left -= table.count[len];
        if (left < 0) {
            return false;
        }
    }

    std::array<std::uint16_t, 16> offsets{};
    for (int len = 1; len < 15; ++len) {
        offsets[len + 1] = static_cast<std::uint16_t>(offsets[len] + table.count[len]);
    }
    table.symbol.assign(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
        if (lengths[i] != 0) {
            table.symbol[offsets[lengths[i]]++] = static_cast<std::uint16_t>(i);
        }
    }
    return true;
}

int decode_symbol(BitReader &reader, const Huffman &table) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len <= 15; ++len) {
        std::uint32_t one = 0;
        if (!reader.bit(one)) {
            return -1;
        }
        code |= static_cast<int>(one);
        const int count = table.count[len];
        if (code - first < count) {
            return table.symbol[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

constexpr std::array<int, 29> LENGTH_BASE = {
    3,   4,   5,   6,   7,   8,   9,   10,  11,  13,  15,  17,  19,  23,  27,
    31,  35,  43,  51,  59,  67,  83,  99,  115, 131, 163, 195, 227, 258};
constexpr std::array<int, 29> LENGTH_EXTRA = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr std::array<int, 30> DIST_BASE = {
    1,    2,    3,    4,    5,    7,    9,    13,   17,   25,   33,   49,   65,
    97,   129,  193,  257,  385,  513,  769,  1025, 1537, 2049, 3073, 4097,
    6145, 8193, 12289, 16385, 24577};
constexpr std::array<int, 30> DIST_EXTRA = {
    0, 0, 0, 0, 1, 1, 2, 2, 3,  3,  4,  4,  5,  5,  6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool inflate_codes(BitReader &reader, std::vector<std::uint8_t> &out,
                   const Huffman &literals, const Huffman &distances) {
    for (;;) {
        const int symbol = decode_symbol(reader, literals);
        if (symbol < 0) {
            return false;
        }
        if (symbol < 256) {
            out.push_back(static_cast<std::uint8_t>(symbol));
            continue;
        }
        if (symbol == 256) {
            return true;
        }
        const int length_index = symbol - 257;
        if (length_index < 0 || length_index >= static_cast<int>(LENGTH_BASE.size())) {
            return false;
        }
        std::uint32_t extra = 0;
        if (!reader.bits(LENGTH_EXTRA[length_index], extra)) {
            return false;
        }
        const std::size_t length =
            static_cast<std::size_t>(LENGTH_BASE[length_index]) + extra;

        const int distance_symbol = decode_symbol(reader, distances);
        if (distance_symbol < 0 || distance_symbol >= static_cast<int>(DIST_BASE.size())) {
            return false;
        }
        if (!reader.bits(DIST_EXTRA[distance_symbol], extra)) {
            return false;
        }
        const std::size_t distance =
            static_cast<std::size_t>(DIST_BASE[distance_symbol]) + extra;
        if (distance == 0 || distance > out.size()) {
            return false;
        }
        for (std::size_t i = 0; i < length; ++i) {
            out.push_back(out[out.size() - distance]);
        }
    }
}

bool dynamic_tables(BitReader &reader, Huffman &literals, Huffman &distances) {
    std::uint32_t hlit = 0;
    std::uint32_t hdist = 0;
    std::uint32_t hclen = 0;
    if (!reader.bits(5, hlit) || !reader.bits(5, hdist) || !reader.bits(4, hclen)) {
        return false;
    }
    hlit += 257;
    hdist += 1;
    hclen += 4;

    static constexpr std::array<int, 19> ORDER = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::array<std::uint8_t, 19> code_lengths{};
    for (std::uint32_t i = 0; i < hclen; ++i) {
        std::uint32_t value = 0;
        if (!reader.bits(3, value)) {
            return false;
        }
        code_lengths[ORDER[i]] = static_cast<std::uint8_t>(value);
    }

    Huffman code_table;
    if (!build_huffman(code_table, code_lengths.data(), code_lengths.size())) {
        return false;
    }

    std::vector<std::uint8_t> lengths(hlit + hdist, 0);
    std::size_t index = 0;
    while (index < lengths.size()) {
        const int symbol = decode_symbol(reader, code_table);
        if (symbol < 0) {
            return false;
        }
        if (symbol <= 15) {
            lengths[index++] = static_cast<std::uint8_t>(symbol);
            continue;
        }
        std::uint32_t repeat = 0;
        if (symbol == 16) {
            if (index == 0 || !reader.bits(2, repeat)) {
                return false;
            }
            repeat += 3;
            const std::uint8_t previous = lengths[index - 1];
            while (repeat-- != 0 && index < lengths.size()) {
                lengths[index++] = previous;
            }
        } else if (symbol == 17) {
            if (!reader.bits(3, repeat)) {
                return false;
            }
            repeat += 3;
            while (repeat-- != 0 && index < lengths.size()) {
                lengths[index++] = 0;
            }
        } else if (symbol == 18) {
            if (!reader.bits(7, repeat)) {
                return false;
            }
            repeat += 11;
            while (repeat-- != 0 && index < lengths.size()) {
                lengths[index++] = 0;
            }
        } else {
            return false;
        }
    }

    if (!build_huffman(literals, lengths.data(), hlit) ||
        !build_huffman(distances, lengths.data() + hlit, hdist)) {
        return false;
    }
    return true;
}

std::uint32_t adler32(const std::vector<std::uint8_t> &data) {
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (std::uint8_t byte : data) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

} // namespace

bool zlib_inflate(const std::uint8_t *data, std::size_t size,
                  std::vector<std::uint8_t> &out, std::string &error) {
    out.clear();
    if (size < 6) {
        error = "zlib stream is too short";
        return false;
    }
    const std::uint8_t cmf = data[0];
    const std::uint8_t flg = data[1];
    if ((cmf & 0x0f) != 8) {
        error = "zlib stream does not use DEFLATE";
        return false;
    }
    if (((static_cast<unsigned>(cmf) << 8) | flg) % 31 != 0) {
        error = "zlib header check failed";
        return false;
    }
    if ((flg & 0x20) != 0) {
        error = "zlib preset dictionaries are not supported";
        return false;
    }

    BitReader reader(data + 2, size - 2);
    bool final = false;
    while (!final) {
        std::uint32_t final_bit = 0;
        std::uint32_t type = 0;
        if (!reader.bit(final_bit) || !reader.bits(2, type)) {
            error = "truncated DEFLATE block header";
            return false;
        }
        final = final_bit != 0;

        if (type == 0) {
            std::uint8_t header[4];
            if (!reader.read_bytes(header, sizeof(header))) {
                error = "truncated stored DEFLATE block";
                return false;
            }
            const std::uint16_t length =
                static_cast<std::uint16_t>(header[0] | (header[1] << 8));
            const std::uint16_t inverse =
                static_cast<std::uint16_t>(header[2] | (header[3] << 8));
            if (static_cast<std::uint16_t>(~length) != inverse) {
                error = "stored DEFLATE block length check failed";
                return false;
            }
            const std::size_t old_size = out.size();
            out.resize(old_size + length);
            if (!reader.read_bytes(out.data() + old_size, length)) {
                error = "truncated stored DEFLATE block payload";
                return false;
            }
        } else if (type == 1) {
            std::array<std::uint8_t, 288> literal_lengths{};
            for (int i = 0; i < 144; ++i) literal_lengths[i] = 8;
            for (int i = 144; i < 256; ++i) literal_lengths[i] = 9;
            for (int i = 256; i < 280; ++i) literal_lengths[i] = 7;
            for (int i = 280; i < 288; ++i) literal_lengths[i] = 8;
            std::array<std::uint8_t, 32> distance_lengths{};
            distance_lengths.fill(5);

            Huffman literals;
            Huffman distances;
            if (!build_huffman(literals, literal_lengths.data(), literal_lengths.size()) ||
                !build_huffman(distances, distance_lengths.data(), distance_lengths.size())) {
                error = "invalid fixed Huffman table";
                return false;
            }
            if (!inflate_codes(reader, out, literals, distances)) {
                error = "invalid DEFLATE data";
                return false;
            }
        } else if (type == 2) {
            Huffman literals;
            Huffman distances;
            if (!dynamic_tables(reader, literals, distances)) {
                error = "invalid dynamic Huffman table";
                return false;
            }
            if (!inflate_codes(reader, out, literals, distances)) {
                error = "invalid DEFLATE data";
                return false;
            }
        } else {
            error = "reserved DEFLATE block type";
            return false;
        }
    }

    if (!reader.align()) {
        error = "truncated DEFLATE trailer";
        return false;
    }
    std::uint8_t trailer[4];
    if (reader.read_bytes(trailer, sizeof(trailer))) {
        const std::uint32_t expected =
            (static_cast<std::uint32_t>(trailer[0]) << 24) |
            (static_cast<std::uint32_t>(trailer[1]) << 16) |
            (static_cast<std::uint32_t>(trailer[2]) << 8) |
            trailer[3];
        if (expected != adler32(out)) {
            error = "zlib Adler-32 check failed";
            return false;
        }
    }
    return true;
}

} // namespace fable::defs::compiler
