#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <windows.h>
#include <iostream>
#include <minilzo.h>

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

inline int LZO1X_Decompress_Safe(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_maxlen, size_t* out_len) {
    const uint8_t* ip = in;
    const uint8_t* ip_end = in + in_len;
    uint8_t* op = out;
    uint8_t* op_end = out + out_maxlen;

    *out_len = 0;
    if (ip >= ip_end) return -1;

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

inline int LZO1X_Decompress(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
    return LZO1X_Decompress_Safe(in, in_len, out, 16 * 1024 * 1024, out_len);
}

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

inline std::vector<uint8_t> CompressFableBlock(const uint8_t* src, uint32_t src_len) {
    // Fable's framing requires leaving the last 3 bytes uncompressed.
    if (src_len <= 3) return {};

    std::vector<lzo_align_t> wrkmem(LZO1X_1_MEM_COMPRESS / sizeof(lzo_align_t) + 1);

    uint32_t target_len = src_len - 3;
    std::vector<uint8_t> comp_buf(target_len + (target_len / 16) + 64 + 3);

    lzo_uint out_len = 0;

    int r = lzo1x_1_compress(src, target_len, comp_buf.data(), &out_len, wrkmem.data());

    if (r != LZO_E_OK) {
        return {};
    }

    std::vector<uint8_t> result;

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

    result.insert(result.end(), comp_buf.data(), comp_buf.data() + out_len);
    result.insert(result.end(), src + target_len, src + src_len);

    return result;
}

inline std::vector<uint8_t> CompressLZORaw(const uint8_t* src, uint32_t src_len) {
    if (src_len == 0) return {};
    std::vector<lzo_align_t> wrkmem(LZO1X_1_MEM_COMPRESS / sizeof(lzo_align_t) + 1);

    std::vector<uint8_t> comp_buf(src_len + (src_len / 16) + 64 + 3);
    lzo_uint out_len = 0;

    int r = lzo1x_1_compress(src, src_len, comp_buf.data(), &out_len, wrkmem.data());

    if (r == LZO_E_OK) {
        comp_buf[out_len++] = 17;
        comp_buf[out_len++] = 0;
        comp_buf[out_len++] = 0;

        comp_buf.resize(out_len);
        return comp_buf;
    }
    return {};
}