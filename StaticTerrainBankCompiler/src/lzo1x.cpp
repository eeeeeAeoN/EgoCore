#include "egostb/lzo1x.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

// LZO1X stream grammar (as documented by the format's public specifications):
//   first byte > 17            : (t-17) literals follow; then a match token
//   token < 16 after literals  : M1 match, 3 bytes, distance 2049..3072
//   token 0..15 otherwise      : literal run of t+3 (t==0: extended length)
//   token 16..31               : M4 match, distance 16385..49151 (or end marker)
//   token 32..63               : M3 match, distance 1..16384
//   token 64..255              : M2 match, length 3..8, distance 1..2048
//   low 2 bits of the last distance byte: 0..3 literals that follow a match
// Lengths are extended with 255-per-zero-byte runs.

namespace albion::lzo1x {

const char* statusName(Status s) {
    switch (s) {
        case Status::Ok: return "ok";
        case Status::InputOverrun: return "input overrun";
        case Status::OutputOverrun: return "output overrun";
        case Status::LookbehindOverrun: return "lookbehind overrun";
        case Status::Corrupt: return "corrupt stream";
        case Status::InputNotConsumed: return "input not consumed";
    }
    return "?";
}

Status decompress(const uint8_t* in, size_t inLen, uint8_t* out, size_t* outLen) {
    const uint8_t* ip = in;
    const uint8_t* const inEnd = in + inLen;
    uint8_t* op = out;
    uint8_t* const outEnd = out + *outLen;
    auto finish = [&](Status s) { *outLen = size_t(op - out); return s; };

    #define NEED_IN(n)  do { if (size_t(inEnd - ip) < size_t(n)) return finish(Status::InputOverrun); } while (0)
    #define NEED_OUT(n) do { if (size_t(outEnd - op) < size_t(n)) return finish(Status::OutputOverrun); } while (0)

    auto copyLiterals = [&](size_t n) -> Status {
        if (size_t(inEnd - ip) < n) return Status::InputOverrun;
        if (size_t(outEnd - op) < n) return Status::OutputOverrun;
        for (size_t i = 0; i < n; ++i) *op++ = *ip++;
        return Status::Ok;
    };
    // Reads the zero-extended length continuation: each 0x00 adds 255, then the final byte.
    auto extend = [&](size_t& t) -> Status {
        for (;;) {
            if (ip >= inEnd) return Status::InputOverrun;
            const uint8_t b = *ip++;
            if (b == 0) { t += 255; if (t > (size_t(1) << 30)) return Status::Corrupt; continue; }
            t += b;
            return Status::Ok;
        }
    };
    auto copyMatch = [&](size_t dist, size_t n) -> Status {
        if (dist == 0 || dist > size_t(op - out)) return Status::LookbehindOverrun;
        if (size_t(outEnd - op) < n) return Status::OutputOverrun;
        const uint8_t* m = op - dist;
        for (size_t i = 0; i < n; ++i) *op++ = *m++;   // byte-wise: overlapping copies are the point
        return Status::Ok;
    };

    NEED_IN(1);
    size_t t = 0;
    bool afterLiterals = false;   // next small token is an M1-after-literals match
    Status st;

    if (*ip > 17) {
        t = size_t(*ip++) - 17;
        if (t < 4) {
            // 1..3 literals then straight into a match token
            if ((st = copyLiterals(t)) != Status::Ok) return finish(st);
            NEED_IN(1);
            t = *ip++;
            goto match;
        }
        if ((st = copyLiterals(t)) != Status::Ok) return finish(st);
        afterLiterals = true;
        NEED_IN(1);
        t = *ip++;
        if (t >= 16) goto match;
        goto m1_after_literals;
    }

    for (;;) {
        NEED_IN(1);
        t = *ip++;
        if (t < 16) {
            if (afterLiterals) {
            m1_after_literals:
                // M1 following a literal run: distance = 2049 + (t >> 2) + (b << 2), length 3
                NEED_IN(1);
                const size_t dist = 0x801 + (t >> 2) + (size_t(*ip++) << 2);
                if ((st = copyMatch(dist, 3)) != Status::Ok) return finish(st);
                afterLiterals = false;
                goto match_done;
            }
            // literal run of t + 3 (t == 0: extended)
            if (t == 0) {
                t = 15;
                if ((st = extend(t)) != Status::Ok) return finish(st);
            }
            if ((st = copyLiterals(t + 3)) != Status::Ok) return finish(st);
            afterLiterals = true;
            continue;
        }
    match:
        afterLiterals = false;
        for (;;) {
            size_t dist, len;
            if (t >= 64) {                       // M2
                NEED_IN(1);
                len = (t >> 5) - 1 + 2;          // 3..8 (t>>5 in 2..7) -> copy (t>>5)-1 +2 bytes
                dist = 1 + ((t >> 2) & 7) + (size_t(*ip++) << 3);
            } else if (t >= 32) {                // M3
                len = t & 31;
                if (len == 0) { len = 31; if ((st = extend(len)) != Status::Ok) return finish(st); }
                len += 2;
                NEED_IN(2);
                dist = 1 + (size_t(ip[0]) >> 2) + (size_t(ip[1]) << 6);
                ip += 2;
            } else if (t >= 16) {                // M4 (or end marker)
                const size_t high = (t & 8) << 11;
                len = t & 7;
                if (len == 0) { len = 7; if ((st = extend(len)) != Status::Ok) return finish(st); }
                len += 2;
                NEED_IN(2);
                dist = (size_t(ip[0]) >> 2) + (size_t(ip[1]) << 6) + high;
                ip += 2;
                if (dist == 0) {                 // end marker: 0x11 0x00 0x00
                    *outLen = size_t(op - out);
                    return ip == inEnd ? Status::Ok : Status::InputNotConsumed;
                }
                dist += 0x4000;
            } else {                             // M1 (not after literals): 2 bytes, distance 1..1024
                NEED_IN(1);
                dist = 1 + (t >> 2) + (size_t(*ip++) << 2);
                len = 2;
            }
            if ((st = copyMatch(dist, len)) != Status::Ok) return finish(st);
        match_done:
            // trailing literal count lives in the low 2 bits of the byte before the last distance byte
            t = size_t(ip[-2]) & 3;
            if (t == 0) break;                   // back to the literal-run / token loop
            if ((st = copyLiterals(t)) != Status::Ok) return finish(st);
            NEED_IN(1);
            t = *ip++;
            // a token < 16 here is an M1 match (distance 1..1024), handled by the loop
        }
    }
    #undef NEED_IN
    #undef NEED_OUT
}

// ---------------------------------------------------------------- fast greedy encoder
//
// Single-pass 14-bit hash-table LZO1X encoder optimized for maximum compilation
// speed while emitting standard M2/M3/M4 streams 100% compatible with both
// C decompress() and retail x86 lzo1x_decompress_asm_fast (no M1 tokens < 16
// are emitted after literal runs).

namespace {

constexpr size_t kMaxDist = 0xBFFF; // 49151
constexpr size_t kHashBits = 14;
constexpr size_t kHashSize = size_t(1) << kHashBits;

struct TableEntry {
    uint32_t epoch = 0;
    uint32_t pos = 0;
};

struct FastScratch {
    uint32_t epoch = 1;
    std::vector<TableEntry> head;
};

static thread_local FastScratch t_fast;

inline uint32_t countTrailingZeroBits64(uint64_t val) {
#if defined(_MSC_VER)
    unsigned long idx = 0;
    _BitScanForward64(&idx, val);
    return static_cast<uint32_t>(idx);
#else
    return static_cast<uint32_t>(__builtin_ctzll(val));
#endif
}

inline uint32_t hash4(const uint8_t* p) {
    uint32_t v = 0;
    std::memcpy(&v, p, 4);
    return (v * 0x9E3779B1u) >> (32 - kHashBits);
}

} // namespace

std::vector<uint8_t> compress(const uint8_t* in, size_t n) {
    std::vector<uint8_t> out;
    out.reserve(n / 2 + 32);
    if (n == 0) {
        out = {0x11, 0x00, 0x00};
        return out;
    }

    FastScratch& s = t_fast;
    if (s.head.size() != kHashSize) {
        s.head.assign(kHashSize, TableEntry{0, 0});
    }
    if (++s.epoch == 0) {
        s.epoch = 1;
        std::fill(s.head.begin(), s.head.end(), TableEntry{0, 0});
    }
    const uint32_t epoch = s.epoch;
    TableEntry* const head = s.head.data();

    auto extend = [&](size_t remv) {
        while (remv > 255) {
            out.push_back(0);
            remv -= 255;
        }
        out.push_back(uint8_t(remv));
    };

    size_t litBitsPos = SIZE_MAX;
    bool first = true;
    auto emitRun = [&](size_t start, size_t k) {
        if (first) {
            first = false;
            if (k <= 3) {
                out.push_back(uint8_t(17 + k));
            } else if (k <= 18) {
                out.push_back(uint8_t(k - 3));
            } else {
                out.push_back(0);
                extend(k - 18);
            }
            out.insert(out.end(), in + start, in + start + k);
        } else if (k > 0) {
            if (k <= 3) {
                out[litBitsPos] |= uint8_t(k);
            } else if (k <= 18) {
                out.push_back(uint8_t(k - 3));
            } else {
                out.push_back(0);
                extend(k - 18);
            }
            out.insert(out.end(), in + start, in + start + k);
        }
    };

    size_t anchor = 0;
    size_t ip = 0;
    while (ip + 4 <= n) {
        const uint32_t h = hash4(in + ip);
        const TableEntry prev = head[h];
        head[h] = {epoch, static_cast<uint32_t>(ip)};

        if (prev.epoch == epoch && ip > prev.pos) {
            const size_t dist = ip - prev.pos;
            if (dist <= kMaxDist) {
                uint32_t vCur = 0, vPrev = 0;
                std::memcpy(&vCur, in + ip, 4);
                std::memcpy(&vPrev, in + prev.pos, 4);
                if (vCur == vPrev) {
                    const size_t maxLen = n - ip;
                    size_t len = 4;
                    while (len + 8 <= maxLen) {
                        uint64_t a = 0, b = 0;
                        std::memcpy(&a, in + prev.pos + len, 8);
                        std::memcpy(&b, in + ip + len, 8);
                        uint64_t diff = a ^ b;
                        if (diff != 0) {
                            len += countTrailingZeroBits64(diff) >> 3;
                            goto match_done_len;
                        }
                        len += 8;
                    }
                    while (len < maxLen && in[prev.pos + len] == in[ip + len]) {
                        ++len;
                    }
                match_done_len:;

                    emitRun(anchor, ip - anchor);

                    if (dist <= 2048 && len <= 8) {
                        litBitsPos = out.size();
                        out.push_back(uint8_t(((len - 1) << 5) | (((dist - 1) & 7) << 2)));
                        out.push_back(uint8_t((dist - 1) >> 3));
                    } else if (dist <= 16384) {
                        const size_t t = len - 2;
                        if (t <= 31) {
                            out.push_back(uint8_t(0x20 | t));
                        } else {
                            out.push_back(0x20);
                            extend(t - 31);
                        }
                        const size_t d = dist - 1;
                        litBitsPos = out.size();
                        out.push_back(uint8_t((d & 0x3F) << 2));
                        out.push_back(uint8_t(d >> 6));
                    } else {
                        const size_t d = dist - 0x4000;
                        const uint8_t t0 = uint8_t(0x10 | ((d >> 11) & 8));
                        const size_t t = len - 2;
                        if (t <= 7) {
                            out.push_back(uint8_t(t0 | t));
                        } else {
                            out.push_back(t0);
                            extend(t - 7);
                        }
                        const size_t d14 = d & 0x3FFF;
                        litBitsPos = out.size();
                        out.push_back(uint8_t((d14 & 0x3F) << 2));
                        out.push_back(uint8_t(d14 >> 6));
                    }

                    ip += len;
                    anchor = ip;
                    if (ip + 4 <= n) {
                        // Seed hash at last byte of match for better chain continuity
                        head[hash4(in + ip - 1)] = {epoch, static_cast<uint32_t>(ip - 1)};
                    }
                    continue;
                }
            }
        }
        ++ip;
    }

    if (anchor < n) {
        emitRun(anchor, n - anchor);
    }
    out.push_back(0x11);
    out.push_back(0x00);
    out.push_back(0x00);
    return out;
}

} // namespace albion::lzo1x
