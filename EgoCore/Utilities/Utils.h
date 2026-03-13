#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <windows.h>
#include <iostream>

enum EDataType : int32_t {
    TYPE_GRAPHIC_SINGLE = 0x0,
    TYPE_GRAPHIC_SEQUENCE = 0x1,
    TYPE_BUMPMAP = 0x2,
    TYPE_BUMPMAP_SEQUENCE = 0x3,
    TYPE_GRAPHIC_VOLUME_TEXTURE = 0x4,
    TYPE_GRAPHIC_FLAT_SEQUENCE = 0x5,

    TYPE_STATIC_MESH = 0x1,
    TYPE_STATIC_REPEATED_MESH = 0x2,
    TYPE_STATIC_PHYSICS_MESH = 0x3,
    TYPE_STATIC_PARTICLE_MESH = 0x4,
    TYPE_ANIMATED_MESH = 0x5,
    TYPE_ANIMATION = 0x6,
    TYPE_DELTA_ANIMATION = 0x7,
    TYPE_LIPSYNC_ANIMATION = 0x8,
    TYPE_PARTIAL_ANIMATION = 0x9,
    TYPE_RELATIVE_ANIMATION = 0xA
};

struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

inline std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        if (isspace(hex[i])) { i--; continue; }
        std::string s = hex.substr(i, 2);
        try { bytes.push_back((uint8_t)strtol(s.c_str(), NULL, 16)); }
        catch (...) {}
    }
    return bytes;
}

inline std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t b : bytes) ss << std::setw(2) << (int)b << " ";
    return ss.str();
}

static bool g_ConsoleInitialized = false;

inline void InitDebugConsole() {
    if (!g_ConsoleInitialized) {
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        std::cout << "=== Fable Bank Parser Debug Console ===" << std::endl;
        g_ConsoleInitialized = true;
    }
}

inline void LogHex(const char* label, const uint8_t* data, size_t offset, size_t count) {
    if (!data) return;
    std::cout << label << " @ 0x" << std::hex << std::setw(4) << std::setfill('0') << offset << ": ";
    for (size_t i = 0; i < count && i < 32; i++) {
        std::cout << std::setw(2) << std::setfill('0') << (int)data[offset + i] << " ";
    }
    std::cout << std::dec << std::endl;
}

namespace FableLZO {

#pragma pack(push, 1)
    struct lzo1x_999_t {
        int32_t init;
        uint32_t look, m_len, m_off, last_m_len, last_m_off;
        const uint8_t* bp, * ip, * in, * in_end;
        uint8_t* out;
        void(__cdecl* cb)(uint32_t, uint32_t);
        uint32_t textsize, codesize, printcount;
        uint32_t lit_bytes, match_bytes, rep_bytes, lazy;
        uint32_t r1_lit, r1_m_len;
        uint32_t m1a_m, m1b_m, m2_m, m3_m, m4_m;
        uint32_t lit1_r, lit2_r, lit3_r;
    };

    struct lzo1x_999_swd_t {
        uint32_t n, f, threshold, max_chain, nice_length;
        int32_t use_best_off;
        uint32_t lazy_insert, m_len, m_off, look;
        int32_t b_char;
        uint32_t best_off[34];
        lzo1x_999_t* c;
        uint32_t m_pos;
        uint32_t best_pos[34];
        const uint8_t* dict, * dict_end;
        uint32_t dict_len, ip, bp, rp, b_size;
        uint8_t* b_wrap;
        uint32_t node_count, first_rp;
        uint8_t b[53247];
        uint8_t padding;
        uint16_t head3[16384];
        uint16_t succ3[51199];
        uint16_t best3[51199];
        uint16_t llen3[16384];
        uint16_t head2[65536];
    };
#pragma pack(pop)

    inline uint8_t* STORE_RUN(lzo1x_999_t* c, uint8_t* op, const uint8_t* ii, uint32_t t) {
        c->lit_bytes += t;
        if (op == c->out && t <= 238) { *op++ = (uint8_t)(t + 17); }
        else if (t > 3) {
            if (t > 18) {
                *op++ = 0; uint32_t tt = t - 18;
                if (tt > 255) { uint32_t count = (t - 274) / 255 + 1; memset(op, 0, count); op += count; tt = t - 18; }
                *op++ = (uint8_t)tt; c->lit3_r++;
            }
            else { *op++ = (uint8_t)(t - 3); c->lit2_r++; }
        }
        else { *(op - 2) |= (uint8_t)t; c->lit1_r++; }
        memcpy(op, ii, t); return op + t;
    }

    inline uint8_t* code_match(uint8_t* op, uint32_t m_len, lzo1x_999_t* c, uint32_t m_off) {
        c->match_bytes += m_len;
        if (m_len == 2) { *op++ = (uint8_t)(4 * ((m_off - 1) & 3)); *op++ = (uint8_t)((m_off - 1) >> 2); c->m1a_m++; }
        else if (m_len > 8 || m_off > 2048) {
            if (m_len == 3 && m_off <= 3072 && c->r1_lit >= 4) { *op++ = (uint8_t)(4 * ((m_off - 1) & 3)); *op++ = (uint8_t)((m_off - 2049) >> 2); c->m1b_m++; }
            else if (m_off > 16384) {
                uint32_t bit = ((m_off - 16384) >> 11) & 8;
                if (m_len > 9) { uint8_t len = (uint8_t)(m_len - 9); *op++ = (uint8_t)(bit | 0x10); if (len > 255) { uint32_t count = (m_len - 265) / 255 + 1; memset(op, 0, count); op += count; } *op++ = len; }
                else { *op++ = (uint8_t)(bit | (m_len - 2) | 0x10); }
                *op++ = (uint8_t)(4 * m_off); *op++ = (uint8_t)((m_off - 16384) >> 6); c->m4_m++;
            }
            else {
                uint32_t off = m_off - 1; uint32_t len = m_len;
                if (len > 33) { len -= 33; *op++ = 32; if (len > 255) { uint32_t count = (len - 256) / 255 + 1; memset(op, 0, count); op += count; } *op++ = (uint8_t)len; }
                else { *op++ = (uint8_t)((len - 2) | 0x20); }
                *op++ = (uint8_t)(4 * off); *op++ = (uint8_t)(off >> 6); c->m3_m++;
            }
        }
        else { *op++ = (uint8_t)(4 * (((m_off - 1) & 7) | (8 * (m_len - 1)))); *op++ = (uint8_t)((m_off - 1) >> 3); c->m2_m++; }
        c->last_m_off = m_off; c->last_m_len = m_len; return op;
    }

    inline void swd_getbyte(lzo1x_999_swd_t* s) {
        lzo1x_999_t* c = s->c;
        if (c->ip >= c->in_end) { if (s->look) s->look--; }
        else { uint8_t val = *c->ip++; s->b[s->ip] = val; if (s->ip < s->f) s->b_wrap[s->ip] = val; }
        if (++s->ip == s->b_size) s->ip = 0;
        if (++s->bp == s->b_size) s->bp = 0;
        if (++s->rp == s->b_size) s->rp = 0;
    }

    inline void swd_remove_node(lzo1x_999_swd_t* s, uint32_t node) {
        if (s->node_count) { s->node_count--; }
        else {
            uint32_t h = ((40799 * (s->b[node + 2] ^ (32 * (s->b[node + 1] ^ (32 * (uint32_t)s->b[node]))))) >> 5) & 0x3FFF;
            s->llen3[h]--; uint16_t head = *(uint16_t*)&s->b[node]; if (s->head2[head] == node) s->head2[head] = 0xFFFF;
        }
    }

    inline int swd_search2(lzo1x_999_swd_t* s) {
        uint32_t v1 = s->head2[*(uint16_t*)&s->b[s->bp]];
        if (v1 == 0xFFFF) return 0;
        if (!s->best_pos[2]) s->best_pos[2] = v1 + 1;
        if (s->m_len < 2) { s->m_len = 2; s->m_pos = v1; }
        return 1;
    }

    inline void swd_search(lzo1x_999_swd_t* s, uint32_t node, uint32_t cnt) {
        uint32_t bp = s->bp; uint32_t m_len = s->m_len; uint8_t* b_ptr = &s->b[bp]; uint8_t* end_ptr = &s->b[bp + s->look]; uint8_t scan_end = b_ptr[m_len - 1];
        while (cnt--) {
            uint8_t* m_ptr = &s->b[node];
            if (m_ptr[m_len - 1] == scan_end && m_ptr[m_len] == b_ptr[m_len] && m_ptr[0] == b_ptr[0] && s->b[node + 1] == s->b[bp + 1]) {
                uint8_t* v8 = &b_ptr[2]; uint8_t* v9 = &m_ptr[2];
                while (v8 < end_ptr && *v8 == *v9) { v8++; v9++; }
                uint32_t v10 = (uint32_t)(v8 - b_ptr);
                if (v10 < 34 && !s->best_pos[v10]) s->best_pos[v10] = node + 1;
                if (v10 > m_len) {
                    m_len = v10; s->m_len = v10; s->m_pos = node;
                    if (v10 == s->look || v10 >= s->nice_length || v10 > s->best3[node]) return;
                    scan_end = b_ptr[m_len - 1];
                }
            }
            node = s->succ3[node];
        }
    }

    inline void better_match(const lzo1x_999_swd_t* swd, uint32_t* m_len, uint32_t* m_off) {
        uint32_t len = *m_len; uint32_t off = *m_off;
        if (len > 3 && off > 0x800) {
            if (len >= 4 && len <= 9 && swd->best_off[len - 1] != 0 && swd->best_off[len - 1] <= 0x800) { *m_len = len - 1; *m_off = swd->best_off[len - 1]; }
            else if (off > 0x4000 && len >= 10) {
                if (len <= 10 && swd->best_off[len - 2] != 0 && swd->best_off[len - 2] <= 0x800) { *m_len = len - 2; *m_off = swd->best_off[len - 2]; }
                else if (len <= 34) { uint32_t bo = swd->best_off[len - 1]; if (bo != 0 && bo <= 0x4000) { *m_len = len - 1; *m_off = bo; } }
            }
        }
    }

    inline void swd_findbest(lzo1x_999_swd_t* s) {
        uint32_t bp = s->bp;
        uint32_t hash = ((40799 * (s->b[bp + 2] ^ (32 * (s->b[bp + 1] ^ (32 * (uint32_t)s->b[bp]))))) >> 5) & 0x3FFF;
        s->succ3[bp] = s->head3[hash];
        uint32_t v3 = s->succ3[bp]; uint32_t v4 = s->llen3[hash]++;
        if (v4 > s->max_chain && s->max_chain) v4 = s->max_chain;
        s->head3[hash] = (uint16_t)bp; s->b_char = s->b[bp];
        if (s->m_len < s->look) {
            uint32_t old_m_len = s->m_len; if (swd_search2(s) && s->look >= 3) swd_search(s, v3, v4);
            if (s->m_len > old_m_len) { uint32_t m_pos = s->m_pos; s->m_off = (s->bp <= m_pos) ? (s->bp + s->b_size - m_pos) : (s->bp - m_pos); }
            s->best3[bp] = (uint16_t)s->m_len;
            if (s->use_best_off) { for (int i = 0; i < 32; ++i) { uint32_t pos = s->best_pos[i + 2]; if (pos) s->best_off[i] = (s->bp <= pos - 1) ? (s->bp + s->b_size - pos + 1) : (s->bp - pos + 1); else s->best_off[i] = 0; } }
        }
        else { if (!s->look) s->b_char = -1; s->m_off = 0; s->best3[bp] = (uint16_t)s->f + 1; }
        swd_remove_node(s, s->rp); s->head2[*(uint16_t*)&s->b[s->bp]] = (uint16_t)s->bp;

        s->c->m_len = s->m_len; s->c->m_off = s->m_off;
        if (s->b_char >= 0) s->c->look = s->look + 1; else { s->c->look = 0; s->c->m_len = 0; }
        s->c->bp = s->c->ip - s->c->look;
    }

    inline int swd_init(lzo1x_999_swd_t* s, const uint8_t* dict, uint32_t dict_len) {
        s->f = 2048; s->max_chain = 2048; s->nice_length = 2048; s->b_wrap = &s->b[51199]; s->n = 49151; s->threshold = 1; s->use_best_off = 0; s->lazy_insert = 0; s->b_size = 51199; s->node_count = 49151;
        memset(s->llen3, 0, sizeof(s->llen3)); memset(s->head2, 0xFF, sizeof(s->head2)); s->ip = 0;
        if (dict && dict_len) { if (dict_len > s->n) { dict = &dict[dict_len - s->n]; dict_len = s->n; } s->dict_end = &dict[dict_len]; s->dict = dict; s->dict_len = dict_len; memcpy(s->b, dict, dict_len); s->ip = dict_len; }
        s->bp = s->ip; s->first_rp = s->ip;
        uint32_t v5 = (uint32_t)(s->c->in_end - s->c->ip); s->look = v5;
        if (v5) { if (v5 > s->f) s->look = s->f; memcpy(&s->b[s->ip], s->c->ip, s->look); s->c->ip += s->look; s->ip += s->look; }
        if (s->ip == s->b_size) s->ip = 0;
        s->rp = (s->first_rp < s->node_count) ? (s->b_size + s->first_rp - s->node_count) : (s->first_rp - s->node_count);
        return 0;
    }

    inline int lzo1x_999_compress_internal(const uint8_t* in, uint32_t in_len, uint8_t* out, uint32_t* out_len, lzo1x_999_swd_t* wrkmem) {
        lzo1x_999_t cc; memset(&cc, 0, sizeof(cc)); cc.in = in; cc.ip = in; cc.in_end = in + in_len; cc.out = out;
        wrkmem->c = &cc; swd_init(wrkmem, nullptr, 0); uint8_t* op = out; uint32_t lit = 0; const uint8_t* ii = in;
        swd_findbest(wrkmem);
        while (cc.look > 0) {
            uint32_t m_len = cc.m_len; uint32_t m_off = cc.m_off;
            if (!lit) ii = cc.bp;
            if (m_len >= 2) {
                better_match(wrkmem, &m_len, &m_off);
                if (lit) { op = STORE_RUN(&cc, op, ii, lit); lit = 0; }
                op = code_match(op, m_len, &cc, m_off);
                for (uint32_t i = 0; i < m_len; ++i) swd_getbyte(wrkmem);
                swd_findbest(wrkmem);
            }
            else { lit++; swd_getbyte(wrkmem); swd_findbest(wrkmem); }
        }
        if (lit) op = STORE_RUN(&cc, op, ii, lit);
        *op++ = 17; *op++ = 0; *op++ = 0;
        *out_len = (uint32_t)(op - out); return 0;
    }
}

// [FIXED] Fully hardened decompressor - C++ goto scope errors fixed
inline int LZO1X_Decompress_Safe(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_maxlen, size_t* out_len) {
    const uint8_t* ip = in;
    const uint8_t* ip_end = in + in_len;
    uint8_t* op = out;
    uint8_t* op_end = out + out_maxlen;

    *out_len = 0;
    if (ip >= ip_end) return -1;

    // Declare ALL variables at the top to prevent C2362 goto skip errors
    unsigned int t;
    unsigned int m_len;
    const uint8_t* m_pos;
    size_t offset;
    size_t base_off;
    size_t total_off;
    uint16_t dist;

    if (*ip > 17) {
        t = *ip++ - 17;
        if (t < 4) { goto match_next; }
        if ((size_t)t > (size_t)(op_end - op) || (size_t)t > (size_t)(ip_end - ip)) return -1;
        do { *op++ = *ip++; } while (--t > 0);
        goto first_literal_run;
    }

LABEL_5:
    if (ip >= ip_end) return -1;
    t = *ip++;
    if (t < 16) {
        if (t == 0) {
            while (*ip == 0) { t += 255; ip++; if (ip >= ip_end) return -1; }
            t += *ip++ + 15;
        }

        if ((size_t)4 > (size_t)(op_end - op) || (size_t)4 > (size_t)(ip_end - ip)) return -1;
        memcpy(op, ip, 4);
        op += 4; ip += 4;

        m_len = t;
        if (m_len > 0) {
            m_len--;
            if ((size_t)m_len > (size_t)(op_end - op) || (size_t)m_len > (size_t)(ip_end - ip)) return -1;

            if (m_len < 4) {
                while (m_len > 0) { *op++ = *ip++; m_len--; }
            }
            else {
                do {
                    memcpy(op, ip, 4);
                    m_len -= 4; op += 4; ip += 4;
                } while (m_len >= 4);
                while (m_len > 0) { *op++ = *ip++; m_len--; }
            }
        }

    first_literal_run:
        if (ip >= ip_end) return -1;
        t = *ip++;
        if (t < 16) {
            if (ip + 1 > ip_end) return -1;

            offset = 2049 + (t >> 2) + (4 * (*ip++));
            if (offset > (size_t)(op - out)) return -1;

            m_pos = op - offset;

            if ((size_t)3 > (size_t)(op_end - op)) return -1;
            *op++ = m_pos[0]; *op++ = m_pos[1]; *op++ = m_pos[2];
            goto match_done;
        }
    }

    while (true) {
        if (ip >= ip_end) return -1;

        if (t >= 64) {
            if (ip + 1 > ip_end) return -1;

            offset = 1 + ((t >> 2) & 7) + (8 * (*ip++));
            if (offset > (size_t)(op - out)) return -1;
            m_pos = op - offset;

            m_len = (t >> 5) - 1;
            goto copy_match;
        }
        if (t >= 32) {
            m_len = t & 31;
            if (m_len == 0) {
                while (*ip == 0) { m_len += 255; ip++; if (ip >= ip_end) return -1; }
                m_len += *ip++ + 31;
            }
            if (ip + 2 > ip_end) return -1;

            memcpy(&dist, ip, 2);
            ip += 2;

            offset = 1 + (dist >> 2);
            if (offset > (size_t)(op - out)) return -1;
            m_pos = op - offset;
            goto do_match;
        }
        if (t >= 16) {
            base_off = 0x800 * (t & 8);
            m_len = t & 7;
            if (m_len == 0) {
                while (*ip == 0) { m_len += 255; ip++; if (ip >= ip_end) return -1; }
                m_len += *ip++ + 7;
            }
            if (ip + 2 > ip_end) return -1;

            memcpy(&dist, ip, 2);
            ip += 2;

            total_off = base_off + (dist >> 2);
            if (total_off == 0) { *out_len = (size_t)(op - out); return 0; }

            total_off += 0x4000;
            if (total_off > (size_t)(op - out)) return -1;
            m_pos = op - total_off;
            goto do_match;
        }

        if (ip + 1 > ip_end) return -1;

        offset = 1 + (t >> 2) + (4 * (*ip++));
        if (offset > (size_t)(op - out)) return -1;
        m_pos = op - offset;

        if ((size_t)2 > (size_t)(op_end - op)) return -1;
        *op++ = m_pos[0]; *op++ = m_pos[1];

    match_done:
        if (ip >= ip_end) return -1;
        t = *(ip - 2) & 3;
        if (t == 0) goto LABEL_5;

    match_next:
        if ((size_t)t > (size_t)(op_end - op) || (size_t)t > (size_t)(ip_end - ip)) return -1;
        do { *op++ = *ip++; } while (--t > 0);
        if (ip >= ip_end) return -1;
        t = *ip++;
    }

do_match:
    if ((size_t)m_len + 2 > (size_t)(op_end - op)) return -1;

    if (m_len >= 6 && (size_t)(op - m_pos) >= 4) {
        memcpy(op, m_pos, 4); op += 4; m_pos += 4;
        unsigned int remaining = m_len - 2;
        while (remaining >= 4) { memcpy(op, m_pos, 4); remaining -= 4; op += 4; m_pos += 4; }
        while (remaining > 0) { *op++ = *m_pos++; remaining--; }
        goto match_done;
    }

copy_match:
    if ((size_t)m_len + 2 > (size_t)(op_end - op)) return -1;
    *op++ = *m_pos++; *op++ = *m_pos++;
    do { *op++ = *m_pos++; } while (--m_len > 0);
    goto match_done;
}

// Default wrapper
inline int LZO1X_Decompress(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
    return LZO1X_Decompress_Safe(in, in_len, out, 16 * 1024 * 1024, out_len);
}

// Forward declarations
inline std::vector<uint8_t> DecompressLZO(const uint8_t* data, size_t& cursor, size_t fileSize, size_t expectedSize) {
    std::vector<uint8_t> result(expectedSize, 0); size_t totalDecompressed = 0;
    size_t lzoTarget = (expectedSize > 3) ? (expectedSize - 3) : 0;
    while (totalDecompressed < lzoTarget && cursor + 2 <= fileSize) {
        uint16_t compSize16; memcpy(&compSize16, data + cursor, 2); cursor += 2;
        uint32_t compSize = compSize16;
        if (compSize == 0xFFFF) {
            if (cursor + 4 > fileSize) break;
            memcpy(&compSize, data + cursor, 4); cursor += 4;
        }
        if (compSize == 0) {
            size_t remaining = lzoTarget - totalDecompressed;
            size_t toCopy = (std::min)(remaining, fileSize - cursor);
            memcpy(result.data() + totalDecompressed, data + cursor, toCopy);
            cursor += toCopy; totalDecompressed += toCopy;
        }
        else {
            if (cursor + compSize > fileSize) break;
            size_t outLen = 0;
            int ret = LZO1X_Decompress_Safe(data + cursor, compSize, result.data() + totalDecompressed, expectedSize - totalDecompressed, &outLen);
            cursor += compSize; totalDecompressed += outLen;
            if (ret != 0 && outLen == 0) break;
        }
    }
    if (cursor + 3 <= fileSize && expectedSize >= 3) { memcpy(result.data() + (expectedSize - 3), data + cursor, 3); cursor += 3; }
    return result;
}

inline std::vector<uint8_t> DecompressRawLZO(const std::vector<uint8_t>& data, size_t offset, size_t expectedSize) {
    std::vector<uint8_t> result; if (offset >= data.size()) return result;
    size_t outSize = expectedSize > 0 ? expectedSize : (data.size() * 10); result.resize(outSize);
    size_t processedSize = 0;
    int ret = LZO1X_Decompress_Safe(data.data() + offset, data.size() - offset, result.data(), outSize, &processedSize);
    if (processedSize > 0) { result.resize(processedSize); }
    else { result.clear(); }
    return result;
}

#include "minilzo.h"

inline std::vector<uint8_t> CompressFableBlock(const uint8_t* src, uint32_t src_len) {
    // Fable's framing requires leaving the last 3 bytes uncompressed. 
    // If the block is too small, just return empty to trigger the uncompressed fallback.
    if (src_len <= 3) return {};

    // Standard miniLZO working memory requirement
    std::vector<lzo_align_t> wrkmem(LZO1X_1_MEM_COMPRESS / sizeof(lzo_align_t) + 1);

    // MiniLZO recommended output buffer size: in_len + (in_len / 16) + 64 + 3
    uint32_t target_len = src_len - 3;
    std::vector<uint8_t> comp_buf(target_len + (target_len / 16) + 64 + 3);

    lzo_uint out_len = 0;

    // Compress everything EXCEPT the last 3 bytes
    int r = lzo1x_1_compress(src, target_len, comp_buf.data(), &out_len, wrkmem.data());

    if (r != LZO_E_OK) {
        return {}; // If compression fails, MeshCompiler will safely write it uncompressed
    }

    std::vector<uint8_t> result;

    // Fable Fallback: If compressed data doesn't actually save space, store it uncompressed
    if (out_len >= target_len) {
        uint16_t zero = 0;
        result.insert(result.end(), (uint8_t*)&zero, (uint8_t*)&zero + 2);
        result.insert(result.end(), src, src + src_len);
    }
    else {
        // 1. Write Fable Size Header
        if (out_len > 0x7FFF) {
            uint16_t magic = 0xFFFF;
            result.insert(result.end(), (uint8_t*)&magic, (uint8_t*)&magic + 2);
            uint32_t longLen = (uint32_t)out_len;
            result.insert(result.end(), (uint8_t*)&longLen, (uint8_t*)&longLen + 4);
        }
        else {
            uint16_t shortLen = (uint16_t)out_len;
            result.insert(result.end(), (uint8_t*)&shortLen, (uint8_t*)&shortLen + 2);
        }

        // 2. Write the LZO compressed payload
        result.insert(result.end(), comp_buf.data(), comp_buf.data() + out_len);

        // 3. Append the uncompressed last 3 bytes
        result.insert(result.end(), src + target_len, src + src_len);
    }

    return result;
}

inline std::vector<uint8_t> CompressLZORaw(const uint8_t* src, uint32_t src_len) {
    if (src_len == 0) return {};
    std::vector<lzo_align_t> wrkmem(LZO1X_1_MEM_COMPRESS / sizeof(lzo_align_t) + 1);

    // MiniLZO safety buffer calculation + 3 bytes for EOF marker
    std::vector<uint8_t> comp_buf(src_len + (src_len / 16) + 64 + 3);
    lzo_uint out_len = 0;

    int r = lzo1x_1_compress(src, src_len, comp_buf.data(), &out_len, wrkmem.data());

    if (r == LZO_E_OK) {
        // FABLE CRITICAL FIX: Append the LZO1X EOF marker (17 00 00)
        comp_buf[out_len++] = 17;
        comp_buf[out_len++] = 0;
        comp_buf[out_len++] = 0;

        comp_buf.resize(out_len);
        return comp_buf;
    }
    return {};
}