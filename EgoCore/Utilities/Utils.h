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
    // Texture Types (labels for textures.big entries)
    TYPE_GRAPHIC_SINGLE = 0x0,
    TYPE_GRAPHIC_SEQUENCE = 0x1,
    TYPE_BUMPMAP = 0x2,
    TYPE_BUMPMAP_SEQUENCE = 0x3,
    TYPE_GRAPHIC_VOLUME_TEXTURE = 0x4,
    TYPE_GRAPHIC_FLAT_SEQUENCE = 0x5,

    // Mesh / Anim Types (labels for graphics.big entries)
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

inline int LZO1X_Decompress(const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
    const uint8_t* ip = in;
    const uint8_t* ip_end = in + in_len;
    uint8_t* op = out;
    uint8_t* op_start = out;

    *out_len = 0;

    unsigned int t;
    unsigned int m_len;
    const uint8_t* m_pos;

    if (*ip > 17) {
        t = *ip++ - 17;
        if (t < 4) {
            goto match_next;
        }
        do {
            *op++ = *ip++;
        } while (--t > 0);
        goto first_literal_run;
    }

LABEL_5:
    t = *ip++;

    if (t < 16) {
        if (t == 0) {
            while (*ip == 0) {
                t += 255;
                ip++;
            }
            t += *ip++ + 15;
        }

        *(uint32_t*)op = *(uint32_t*)ip;
        op += 4;
        ip += 4;

        m_len = t;
        if (m_len > 0) {
            m_len--;
            if (m_len < 4) {
                while (m_len > 0) {
                    *op++ = *ip++;
                    m_len--;
                }
            }
            else {
                do {
                    *(uint32_t*)op = *(uint32_t*)ip;
                    m_len -= 4;
                    op += 4;
                    ip += 4;
                } while (m_len >= 4);
                while (m_len > 0) {
                    *op++ = *ip++;
                    m_len--;
                }
            }
        }

    first_literal_run:
        t = *ip++;
        if (t < 16) {
            m_pos = op - (t >> 2) - 2049 - (4 * (*ip++));
            *op++ = m_pos[0];
            *op++ = m_pos[1];
            *op++ = m_pos[2];
            goto match_done;
        }
    }

    while (true) {
        if (t >= 64) {
            m_pos = op - 1 - ((t >> 2) & 7) - (8 * (*ip++));
            m_len = (t >> 5) - 1;
            goto copy_match;
        }

        if (t >= 32) {
            m_len = t & 31;
            if (m_len == 0) {
                while (*ip == 0) {
                    m_len += 255;
                    ip++;
                }
                m_len += *ip++ + 31;
            }
            m_pos = op - 1 - (*(uint16_t*)ip >> 2);
            ip += 2;
            goto do_match;
        }

        if (t >= 16) {
            m_pos = op - 0x800 * (t & 8);
            m_len = t & 7;
            if (m_len == 0) {
                while (*ip == 0) {
                    m_len += 255;
                    ip++;
                }
                m_len += *ip++ + 7;
            }
            uint16_t offset = *(uint16_t*)ip;
            ip += 2;
            m_pos -= (offset >> 2);

            if (m_pos == op) {
                *out_len = op - op_start;
                if (ip == ip_end) return 0;
                return (ip < ip_end) ? -8 : -4;
            }
            m_pos -= 0x4000;
            goto do_match;
        }

        m_pos = op - 1 - (t >> 2) - (4 * (*ip++));
        *op++ = m_pos[0];
        *op++ = m_pos[1];
        goto match_done;

    do_match:
        if (m_len >= 6 && (op - m_pos) >= 4) {
            *(uint32_t*)op = *(uint32_t*)m_pos;
            op += 4;
            m_pos += 4;
            unsigned int remaining = m_len - 2;
            do {
                *(uint32_t*)op = *(uint32_t*)m_pos;
                remaining -= 4;
                op += 4;
                m_pos += 4;
            } while (remaining >= 4);
            while (remaining > 0) {
                *op++ = *m_pos++;
                remaining--;
            }
            goto match_done;
        }

    copy_match:
        *op++ = *m_pos++;
        *op++ = *m_pos++;
        do {
            *op++ = *m_pos++;
        } while (--m_len > 0);

    match_done:
        t = *(ip - 2) & 3;
        if (t == 0) goto LABEL_5;

    match_next:
        do {
            *op++ = *ip++;
        } while (--t > 0);

        t = *ip++;
    }

    *out_len = op - op_start;
    return 0;
}

inline std::vector<uint8_t> DecompressLZO(const uint8_t* data, size_t& cursor, size_t fileSize, size_t expectedSize) {
    //
    std::vector<uint8_t> result(expectedSize, 0);
    size_t totalDecompressed = 0;

    // Fable's assembly expects 3 bytes to be copied manually at the end
    size_t lzoTarget = (expectedSize > 3) ? (expectedSize - 3) : 0;

    while (totalDecompressed < lzoTarget && cursor + 2 <= fileSize) {
        uint32_t compSize = *(uint16_t*)(data + cursor);
        cursor += 2;

        if (compSize == 0xFFFF) {
            if (cursor + 4 > fileSize) break;
            compSize = *(uint32_t*)(data + cursor);
            cursor += 4;
        }

        if (compSize == 0) {
            size_t remaining = lzoTarget - totalDecompressed;
            // Shielded std::min to fix C2589
            size_t toCopy = (std::min)(remaining, fileSize - cursor);
            memcpy(result.data() + totalDecompressed, data + cursor, toCopy);
            cursor += toCopy;
            totalDecompressed += toCopy;
        }
        else {
            if (cursor + compSize > fileSize) break;

            size_t outLen = 0;
            //
            int ret = LZO1X_Decompress(
                data + cursor,
                compSize,
                result.data() + totalDecompressed,
                &outLen
            );

            cursor += compSize;
            totalDecompressed += outLen;
            if (ret != 0 && outLen == 0) break;
        }
    }

    // Mandatory 3-byte trailer from Fable's assembly
    if (cursor + 3 <= fileSize && expectedSize >= 3) {
        memcpy(result.data() + (expectedSize - 3), data + cursor, 3);
        cursor += 3;
    }

    return result;
}

inline std::vector<uint8_t> DecompressRawLZO(const std::vector<uint8_t>& data, size_t offset, size_t expectedSize) {
    std::vector<uint8_t> result;

    if (offset >= data.size()) return result;

    size_t outSize = expectedSize > 0 ? expectedSize : (data.size() * 10);
    result.resize(outSize);

    size_t processedSize = 0;

    int ret = LZO1X_Decompress(
        data.data() + offset,
        data.size() - offset,
        result.data(),
        &processedSize
    );

    if (processedSize > 0) {
        result.resize(processedSize);
    }
    else {
        result.clear();
    }

    return result;
}