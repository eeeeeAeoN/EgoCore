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

// ---------------------------------------------------------------- encoder
//
// Two stages. A hash-chain match finder records, for every position, the
// longest match overall and the longest within each cheaper distance class
// (M2 <= 2048, M3 <= 16384), the nearest 3-byte match in the M1L band
// (2049..3072) and whether a 2-byte match exists within 1024 (M1). Then a
// backward dynamic programme over the exact grammar: literal runs cost their
// bytes plus the run token (free for 0..3 literals riding in the previous
// match token, +1 for 4..18, +1 and length extension beyond), and the token
// that may follow a run depends on the run's length class:
//   after 0 literals    : M2 / M3 / M4
//   after 1..3 literals : also M1  (2 bytes at distance 1..1024, 2 bytes)
//   after a run >= 4    : also M1L (3 bytes at distance 2049..3072, 2 bytes)
// which is exactly why lzo1x_999 output is smaller than a plain parse. The
// forward pass emits tokens in that grammar. The point is size: retail STB
// frames were written with lzo1x_999 and an edited frame must fit its slot.

namespace {

struct Cand { uint32_t dist = 0, len = 0; };
struct Cands {
    Cand best, m2, m3;      // longest overall / within 2048 / within 16384
    uint32_t m1lDist = 0;   // 3-byte match at 2049..3072 (0 = none)
    uint32_t m1Dist = 0;    // 2-byte match at 1..1024 (0 = none)
};

constexpr size_t kMaxDist = 0xBFFF;
constexpr size_t kMinLen = 3;
constexpr size_t kNiceLen = 128;
constexpr uint32_t kChainDepth = 32;
constexpr uint32_t kInf = 0x3FFFFFFFu;

inline uint32_t extBytes(size_t rem) { return uint32_t((rem - 1) / 255 + 1); }

// bytes for an M2/M3/M4 token of `len` at `dist` (no literal-run bytes)
inline uint32_t tokenCost(size_t len, size_t dist) {
    if (dist <= 2048 && len <= 8) return 2;
    if (dist <= 16384) { const size_t t = len - 2; return 3 + (t > 31 ? extBytes(t - 31) : 0); }
    const size_t t = len - 2;
    return 3 + (t > 7 ? extBytes(t - 7) : 0);
}

struct Choice { uint32_t len = 0, dist = 0; uint8_t kind = 0; };   // kind: 0 M2/3/4, 1 M1, 2 M1L

struct TableEntry {
    uint32_t epoch = 0;
    uint32_t pos = 0;
};

inline uint32_t countTrailingZeroBits64(uint64_t val) {
#if defined(_MSC_VER)
    unsigned long idx = 0;
    _BitScanForward64(&idx, val);
    return static_cast<uint32_t>(idx);
#else
    return static_cast<uint32_t>(__builtin_ctzll(val));
#endif
}

struct Scratch {
    uint32_t currentEpoch = 1;
    std::vector<Cands> cands;
    std::vector<TableEntry> pairLast;
    std::vector<TableEntry> head;
    std::vector<uint32_t> next;
    std::vector<uint32_t> A;
    std::vector<uint32_t> L;
    std::vector<uint32_t> B[3];
    std::vector<Choice> Bc[3];
    std::vector<uint32_t> Ak;
};

static thread_local Scratch t_scratch;

void findMatches(const uint8_t* in, size_t n, Scratch& s) {
    s.cands.assign(n, Cands{});
    if (n < 2) return;

    constexpr size_t kHashBits = 16;
    constexpr size_t kHashSize = size_t(1) << kHashBits;

    if (s.pairLast.size() != 65536) {
        s.pairLast.assign(65536, TableEntry{0, 0});
    }
    if (s.head.size() != kHashSize) {
        s.head.assign(kHashSize, TableEntry{0, 0});
    }

    s.currentEpoch++;
    if (s.currentEpoch == 0) {
        s.currentEpoch = 1;
        std::fill(s.pairLast.begin(), s.pairLast.end(), TableEntry{0, 0});
        std::fill(s.head.begin(), s.head.end(), TableEntry{0, 0});
    }

    const uint32_t epoch = s.currentEpoch;

    // 2-byte pairs: last position of each pair, for M1
    for (size_t i = 0; i + 2 <= n; ++i) {
        const uint32_t key = uint32_t(in[i]) | (uint32_t(in[i + 1]) << 8);
        const auto& entry = s.pairLast[key];
        if (entry.epoch == epoch) {
            const uint32_t p = entry.pos;
            if (i - p <= 1024) s.cands[i].m1Dist = uint32_t(i - p);
        }
        s.pairLast[key] = {epoch, uint32_t(i)};
    }
    if (n < kMinLen) return;

    s.next.assign(n, 0xFFFFFFFFu);
    auto hash = [&](size_t i) {
        const uint32_t v = uint32_t(in[i]) | (uint32_t(in[i + 1]) << 8) | (uint32_t(in[i + 2]) << 16);
        return size_t((v * 0x9E3779B1u) >> (32 - kHashBits));
    };
    for (size_t i = 0; i + kMinLen <= n; ++i) {
        const size_t h = hash(i);
        uint32_t p = 0xFFFFFFFFu;
        if (s.head[h].epoch == epoch) {
            p = s.head[h].pos;
        }
        Cands& c = s.cands[i];
        uint32_t depth = 0;
        const size_t maxLen = n - i;
        while (p != 0xFFFFFFFFu && depth++ < kChainDepth) {
            const size_t dist = i - p;
            if (dist > kMaxDist) break;
            const bool canImprove = c.best.len < maxLen && in[p + c.best.len] == in[i + c.best.len];
            const bool wantClass = (dist <= 16384 && c.m3.len < maxLen) || (dist >= 2049 && dist <= 3072 && !c.m1lDist);
            if (canImprove || wantClass) {
                size_t len = 0;
                while (len + 8 <= maxLen) {
                    uint64_t a, b;
                    std::memcpy(&a, in + p + len, 8);
                    std::memcpy(&b, in + i + len, 8);
                    uint64_t diff = a ^ b;
                    if (diff != 0) {
                        len += countTrailingZeroBits64(diff) >> 3;
                        goto match_len_done;
                    }
                    len += 8;
                }
                while (len < maxLen && in[p + len] == in[i + len]) ++len;
            match_len_done:;
                if (len >= kMinLen) {
                    if (len > c.best.len) c.best = {uint32_t(dist), uint32_t(len)};
                    if (dist <= 2048 && len > c.m2.len) c.m2 = {uint32_t(dist), uint32_t(len)};
                    if (dist <= 16384 && len > c.m3.len) c.m3 = {uint32_t(dist), uint32_t(len)};
                    if (dist >= 2049 && dist <= 3072 && !c.m1lDist) c.m1lDist = uint32_t(dist);
                    if (len >= kNiceLen) break;
                }
            }
            p = s.next[p];
        }
        s.next[i] = (s.head[h].epoch == epoch) ? s.head[h].pos : 0xFFFFFFFFu;
        s.head[h] = {epoch, uint32_t(i)};
    }
}


} // namespace

std::vector<uint8_t> compress(const uint8_t* in, size_t n) {
    std::vector<uint8_t> out;
    out.reserve(n / 2 + 16);
    if (n == 0) { out = {0x11, 0x00, 0x00}; return out; }

    Scratch& s = t_scratch;
    findMatches(in, n, s);

    s.A.assign(n + 1, kInf);
    s.L.assign(n + 1, kInf);
    for (int c = 0; c < 3; ++c) {
        s.B[c].assign(n + 1, kInf);
        s.Bc[c].assign(n + 1, Choice{});
    }
    s.Ak.assign(n + 1, 0);
    constexpr uint32_t kLong = 0xFFFFFFFFu;
    s.A[n] = 3; s.L[n] = 3;
    for (int c = 0; c < 3; ++c) s.B[c][n] = 3;

    auto runCost = [](size_t k) -> uint32_t {   // bytes for a literal run of k (token + literals)
        if (k == 0) return 0;
        if (k <= 3) return uint32_t(k);
        if (k <= 18) return uint32_t(k + 1);
        return uint32_t(k + 1 + extBytes(k - 18));
    };

    for (size_t i = n; i-- > 0;) {
        // ---- B: a match starts at i
        const Cands& c = s.cands[i];
        Choice bestCh[3]; uint32_t bestV[3] = {kInf, kInf, kInf};
        auto consider = [&](const Cand& cd) {
            if (cd.len < kMinLen) return;
            auto tryLen = [&](size_t Lm) {
                if (Lm < kMinLen || Lm > cd.len) return;
                const uint32_t v = tokenCost(Lm, cd.dist) + s.A[i + Lm];
                for (int k = 0; k < 3; ++k) if (v < bestV[k]) { bestV[k] = v; bestCh[k] = {uint32_t(Lm), cd.dist, 0}; }
            };
            tryLen(cd.len);
            static const size_t thresholds[] = {3, 4, 5, 8, 9, 33};
            for (size_t t : thresholds) if (t < cd.len) tryLen(t);
        };
        consider(c.best); consider(c.m2); consider(c.m3);
        if (c.m1Dist && i + 2 <= n) {            // class 1 only
            const uint32_t v = 2 + s.A[i + 2];
            if (v < bestV[1]) { bestV[1] = v; bestCh[1] = {2, c.m1Dist, 1}; }
        }
        if (c.m1lDist && i + 3 <= n) {           // class 2 only
            const uint32_t v = 2 + s.A[i + 3];
            if (v < bestV[2]) { bestV[2] = v; bestCh[2] = {3, c.m1lDist, 2}; }
        }
        for (int k = 0; k < 3; ++k) { s.B[k][i] = bestV[k]; s.Bc[k][i] = bestCh[k]; }

        // ---- L: inside a long literal run
        s.L[i] = std::min(1 + s.L[i + 1], s.B[2][i]);

        // ---- A: choose how many literals precede the next match (or the end)
        uint32_t best = kInf, bk = 0;
        const size_t rem = n - i;
        // finish with literals only
        if (rem <= 18) { const uint32_t v = runCost(rem) + 3; if (v < best) { best = v; bk = uint32_t(rem); } }
        if (s.B[0][i] < best) { best = s.B[0][i]; bk = 0; }
        for (size_t k = 1; k <= 18 && k < rem; ++k) {
            // The very first run of 1..3 literals ("17+k" byte) is followed by a
            // plain match token in the C decoder but by an after-literal-run token
            // in LZO's i386 assembly decoder (lzo1x_decompress_asm_fast, which
            // Fable's landscape loader uses): the two disagree on tokens < 16
            // there, so only M2/M3/M4 (class 0) are allowed after it.
            const int cls = (i == 0 && k <= 3) ? 0 : (k <= 3 ? 1 : 2);
            const uint32_t v = runCost(k) + s.B[cls][i + k];
            if (v < best) { best = v; bk = uint32_t(k); }
        }
        if (rem >= 19) {
            // long run: 19 literals (+ token + first extension byte) then keep going or match
            const uint32_t v = 19 + 2 + s.L[i + 19];
            if (v < best) { best = v; bk = kLong; }
        }
        s.A[i] = best; s.Ak[i] = bk;
    }

    // ---- forward emission
    auto extend = [&](size_t remv) { while (remv > 255) { out.push_back(0); remv -= 255; } out.push_back(uint8_t(remv)); };
    size_t litBitsPos = SIZE_MAX;
    bool first = true;
    auto emitRun = [&](size_t start, size_t k) {   // k literals in[start..start+k)
        if (first) {
            first = false;
            if (k <= 3) out.push_back(uint8_t(17 + k));
            else if (k <= 18) out.push_back(uint8_t(k - 3));
            else { out.push_back(0); extend(k - 18); }
        } else if (k == 0) {
        } else if (k <= 3) {
            out[litBitsPos] |= uint8_t(k);
        } else if (k <= 18) {
            out.push_back(uint8_t(k - 3));
        } else {
            out.push_back(0); extend(k - 18);
        }
        out.insert(out.end(), in + start, in + start + k);
    };
    auto emitMatch = [&](const Choice& ch) {
        const size_t len = ch.len, dist = ch.dist;
        if (ch.kind == 1) {                                   // M1: 2 bytes, dist 1..1024
            const size_t d = dist - 1;
            litBitsPos = out.size();
            out.push_back(uint8_t((d & 3) << 2));
            out.push_back(uint8_t(d >> 2));
        } else if (ch.kind == 2) {                            // M1L: 3 bytes, dist 2049..3072
            const size_t d = dist - 0x801;
            litBitsPos = out.size();
            out.push_back(uint8_t((d & 3) << 2));
            out.push_back(uint8_t(d >> 2));
        } else if (dist <= 2048 && len <= 8) {                // M2
            litBitsPos = out.size();
            out.push_back(uint8_t(((len - 1) << 5) | (((dist - 1) & 7) << 2)));
            out.push_back(uint8_t((dist - 1) >> 3));
        } else if (dist <= 16384) {                           // M3
            const size_t t = len - 2;
            if (t <= 31) out.push_back(uint8_t(0x20 | t));
            else { out.push_back(0x20); extend(t - 31); }
            const size_t d = dist - 1;
            litBitsPos = out.size();
            out.push_back(uint8_t((d & 0x3F) << 2));
            out.push_back(uint8_t(d >> 6));
        } else {                                              // M4
            const size_t d = dist - 0x4000;
            const uint8_t t0 = uint8_t(0x10 | ((d >> 11) & 8));
            const size_t t = len - 2;
            if (t <= 7) out.push_back(uint8_t(t0 | t));
            else { out.push_back(t0); extend(t - 7); }
            const size_t d14 = d & 0x3FFF;
            litBitsPos = out.size();
            out.push_back(uint8_t((d14 & 0x3F) << 2));
            out.push_back(uint8_t(d14 >> 6));
        }
    };

    size_t i = 0;
    while (i < n) {
        uint32_t k = s.Ak[i];
        size_t runLen;
        int cls;
        if (k == kLong) {
            // extend the run while L says "one more literal" is the choice
            size_t j = i + 19;
            while (j < n && 1 + s.L[j + 1] <= s.B[2][j]) ++j;
            runLen = j - i; cls = 2;
        } else {
            runLen = k; cls = k == 0 ? 0 : (k <= 3 ? ((i == 0) ? 0 : 1) : 2);
        }
        emitRun(i, runLen);
        i += runLen;
        if (i >= n) break;
        const Choice ch = s.Bc[cls][i];
        if (ch.len == 0) {   // no match possible here (B == inf): cannot happen when A chose it; guard
            emitRun(i, n - i); i = n; break;
        }
        emitMatch(ch);
        i += ch.len;
    }
    out.push_back(0x11); out.push_back(0x00); out.push_back(0x00);   // end marker
    return out;
}

} // namespace albion::lzo1x
