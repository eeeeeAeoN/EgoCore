#pragma once
#include "StringParser.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cstring>

struct C2DVector { float x, y; };
struct C3DVector { float x, y, z; };

// ============================================================
// CDefStringTable
// ============================================================
class CDefStringTable {
private:
    uint32_t m_StringCount;
    std::vector<uint8_t> m_StringStream;
    std::map<std::string, uint32_t> m_StringMap;

public:
    uint32_t m_RandomID;
    CDefStringTable(uint32_t randomId = 0xDEADBEEF) : m_RandomID(randomId), m_StringCount(0) {}

    static uint32_t GetCRC(const std::string& str) {
        uint32_t crc = 0xFFFFFFFF; // Initialize to standard CRC32 start
        for (char c : str) {
            crc ^= static_cast<uint8_t>(std::tolower(c));
            for (int j = 0; j < 8; ++j)
                crc = (crc >> 1) ^ (-(int32_t)(crc & 1) & 0xEDB88320);
        }
        return ~crc; // Return bitwise NOT
    }

    uint32_t AddString(const std::string& text) {
        if (text.empty()) return (uint32_t)-1;
        auto it = m_StringMap.find(text);
        if (it != m_StringMap.end()) return it->second;

        if (m_StringStream.empty()) {
            uint32_t pad = 0;
            const uint8_t* pPad = reinterpret_cast<const uint8_t*>(&pad);
            m_StringStream.insert(m_StringStream.end(), pPad, pPad + 4);
        }

        uint32_t offset = (uint32_t)m_StringStream.size();
        uint32_t crc = GetCRC(text);

        const uint8_t* pCrc = reinterpret_cast<const uint8_t*>(&crc);
        m_StringStream.insert(m_StringStream.end(), pCrc, pCrc + 4);

        const uint8_t* pText = reinterpret_cast<const uint8_t*>(text.c_str());
        m_StringStream.insert(m_StringStream.end(), pText, pText + text.length() + 1);

        m_StringMap[text] = offset;
        m_StringCount++;
        return offset;
    }

    void SaveTable(const std::string& filepath) {
        std::ofstream os(filepath, std::ios::binary);
        if (!os.is_open()) return;
        uint32_t magic = 0x7AB1E;
        uint32_t length = (uint32_t)m_StringStream.size();
        os.write(reinterpret_cast<const char*>(&magic), 4);
        os.write(reinterpret_cast<const char*>(&m_RandomID), 4);
        os.write(reinterpret_cast<const char*>(&m_StringCount), 4);
        os.write(reinterpret_cast<const char*>(&length), 4);
        if (length > 0)
            os.write(reinterpret_cast<const char*>(m_StringStream.data()), length);
        os.close();
    }
};

extern CDefStringTable* GDefStringTable;

// ============================================================
// CDataOutputStream
// ============================================================
class CDataOutputStream {
public:
    virtual ~CDataOutputStream() = default;
    virtual void Write(const void* data, size_t size) = 0;

    void WriteSLONG(int32_t val)  { Write(&val, 4); }
    void WriteULONG(uint32_t val) { Write(&val, 4); }
    void WriteFloat(float val)    { Write(&val, 4); }
    void WriteSWORD(int16_t val)  { Write(&val, 2); }
    void WriteEBOOL(bool val)     { uint8_t b = val ? 1 : 0; Write(&b, 1); }

    void Write2DVector(const C2DVector* vec) { Write(vec, sizeof(C2DVector)); }
    void Write3DVector(const C3DVector* vec) { Write(vec, sizeof(C3DVector)); }

    void WriteString(const std::string& str) {
        uint32_t offset = GDefStringTable ? GDefStringTable->AddString(str) : 0u;
        WriteULONG(offset);
    }

    // CCharString binary: chars + null terminator
    void WriteCharString(const std::string& str) {
        if (!str.empty()) Write(str.c_str(), str.length());
        uint8_t zero = 0;
        Write(&zero, 1);
    }

    // CWideString binary: UTF-16LE chars + WCHAR null terminator
    void WriteNullTerminatedWString(const std::wstring& wstr) {

        for (wchar_t c : wstr) {
            uint16_t ch = (uint16_t)c;
            Write(&ch, 2);
        }
        uint16_t nullterm = 0;
        Write(&nullterm, 2);
    }

    void WriteVectorUint32(const std::vector<uint32_t>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (uint32_t v : vec) WriteULONG(v);
    }

    void WriteVectorInt32(const std::vector<int32_t>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (int32_t v : vec) WriteSLONG(v);
    }

    void WriteVectorFloat(const std::vector<float>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (float v : vec) WriteFloat(v);
    }

    void WriteVectorCharString(const std::vector<std::string>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (const auto& s : vec) WriteCharString(s);
    }

    void WriteVectorWString(const std::vector<std::wstring>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (const auto& s : vec) WriteNullTerminatedWString(s);
    }

    // WriteMap is handled per-type directly in CPersistContext::TransferMap
};

// ============================================================
// CPersistContext — the core serialization context
// Supports MODE_SAVE_BINARY (write binary) and MODE_LOAD_TEXT (scan def text)
// ============================================================
struct CDefString;
struct C2DVector;

class CPersistContext {
public:
    enum EMode {
        MODE_LOAD_TEXT   = 0,
        MODE_SAVE_TEXT   = 1,
        MODE_LOAD_BINARY = 2,
        MODE_SAVE_BINARY = 3,
        MODE_CLEAR       = 4
    };

    EMode Mode;
    CDataOutputStream* PSaveStream;
    bool SafeBinary;

    // Text-mode state
    std::string* PDefText;                         // mutable def text for keyword scanning
    const std::map<std::string, int>* PSymbolMap;  // for identifier → integer resolution
    std::string m_VectorFilter;
    int32_t m_VectorIndex = -1;
    bool UseStdCRC = false;
    bool UseFableCRC = false;
    bool m_ForceNoTags = false;

    // Constructor for binary save mode
    CPersistContext(CDataOutputStream* stream, EMode mode, bool safe = false)
        : PSaveStream(stream), Mode(mode), SafeBinary(safe), PDefText(nullptr), PSymbolMap(nullptr), UseStdCRC(false), UseFableCRC(false), m_ForceNoTags(false) {}

    CPersistContext(std::string* defText, const std::map<std::string, int>* symbolMap)
        : PSaveStream(nullptr), Mode(MODE_LOAD_TEXT), SafeBinary(false),
          PDefText(defText), PSymbolMap(symbolMap), UseStdCRC(false) {}

    // --------------------------------------------------------
    // CRC Tagging Logic for SafeBinary
    // --------------------------------------------------------
    static uint32_t CalculateStdCRC32(const char* str) {
        static uint32_t table[256];
        static bool tableComputed = false;
        if (!tableComputed) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = (uint32_t)i;
                for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
                table[i] = c;
            }
            tableComputed = true;
        }
        uint32_t crc = 0xFFFFFFFF;
        for (const char* p = str; *p; ++p) {
            crc = table[(uint8_t)*p ^ (crc & 0xFF)] ^ (crc >> 8);
        }
        return ~crc;
    }

    static uint32_t CalculateFableCRC32(const char* str) {
        static uint32_t ftable[256];
        static bool ftableComputed = false;
        if (!ftableComputed) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t c = (uint32_t)i;
                for (int j = 0; j < 8; j++) c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
                ftable[i] = c;
            }
            ftableComputed = true;
        }
        uint32_t crc = 0;
        for (const char* p = str; *p; ++p) {
            crc = ftable[(uint8_t)crc ^ (uint8_t)*p] ^ (crc >> 8);
        }
        return crc;
    }

    uint32_t GetCRC(const char* name) {
        return UseStdCRC ? CalculateStdCRC32(name) : CalculateFableCRC32(name);
    }

    void WriteTag(const char* name) {
        if (Mode == MODE_SAVE_BINARY && PSaveStream && !m_ForceNoTags) {
            // Case matters! Fable uses its custom CRC (starts at 0, no invert) for tags.
            uint32_t crc = CalculateFableCRC32(name);
            PSaveStream->WriteSLONG((int32_t)crc);
        }
    }

    void TransferObjectHeader(uint32_t version, bool bForce = false) {
        if (Mode == MODE_SAVE_BINARY && PSaveStream) {
            PSaveStream->WriteEBOOL(true); // Always tag it as a versioned object
            PSaveStream->WriteSLONG((int32_t)version);
            // PSaveStream->WriteEBOOL(bForce); <--- REMOVE THIS. Fable doesn't serialize it.
        }
    }

    // --------------------------------------------------------
    // Text-mode scanner helpers
    // Mirrors the engine's TransferObjectLoadText keyword scan
    // --------------------------------------------------------
    static bool IsWordBoundary(char c) {
        return !(std::isalnum((unsigned char)c) || c == '_');
    }

    // Find 'name' as a whole word in PDefText. Returns position AFTER the name, or npos.
    // Find 'name' as a whole word in PDefText. Returns position AFTER the name, or npos.
    // Enhanced to support white-space insensitive vector prefixes (e.g. "States  [ 0 ]  .  GraphicIndex")
    size_t FindWholeWord(const char* name) const {
        if (!PDefText) return std::string::npos;
        const std::string& text = *PDefText;
        
        // If we have a vector filter active, we must match the prefix first
        if (!m_VectorFilter.empty() && m_VectorIndex != -1) {
            size_t pos = 0;
            while (true) {
                // 1. Find the filter name (e.g., "States")
                size_t found = FindLiteralWholeWord(text, m_VectorFilter.c_str(), pos);
                if (found == std::string::npos) return std::string::npos;
                
                size_t prefixEnd = found;
                SkipWS(prefixEnd);
                
                // 2. Look for open bracket
                if (prefixEnd < text.size() && text[prefixEnd] == '[') {
                    prefixEnd++;
                    int32_t idx = ReadIntExpr(prefixEnd);
                    if (idx == m_VectorIndex) {
                        SkipWS(prefixEnd);
                        if (prefixEnd < text.size() && text[prefixEnd] == ']') {
                            prefixEnd++;
                            SkipWS(prefixEnd);
                            if (prefixEnd < text.size() && text[prefixEnd] == '.') {
                                prefixEnd++;
                                SkipWS(prefixEnd);
                                // 3. Match the actual field name
                                size_t nlen = std::strlen(name);
                                if (text.compare(prefixEnd, nlen, name) == 0) {
                                    bool nextOk = (prefixEnd + nlen >= text.size()) || IsWordBoundary(text[prefixEnd + nlen]);
                                    if (nextOk) return prefixEnd + nlen;
                                }
                            }
                        }
                    }
                }
                pos = found; // Continue searching from this point if match failed
            }
        }

        return FindLiteralWholeWord(text, name, 0);
    }

    size_t FindLiteralWholeWord(const std::string& text, const char* name, size_t startPos) const {
        size_t nlen = std::strlen(name);
        size_t pos = startPos;
        while (pos + nlen <= text.size()) {
            bool match = true;
            for (size_t i = 0; i < nlen; ++i) {
                if (std::tolower((unsigned char)text[pos + i]) != std::tolower((unsigned char)name[i])) {
                    match = false;
                    break;
                }
            }
            if (match) {
                bool prevOk = (pos == 0) || IsWordBoundary(text[pos - 1]);
                bool nextOk = (pos + nlen >= text.size()) || IsWordBoundary(text[pos + nlen]);
                if (prevOk && nextOk) return pos + nlen;
            }
            pos++;
        }
        return std::string::npos;
    }

    // Blank out region [start, end) replacing with spaces (preserving newlines)
    void BlankRegion(size_t start, size_t end) const {
        if (!PDefText) return;
        std::string& text = *PDefText;
        for (size_t i = start; i < end && i < text.size(); ++i) {
            if (text[i] != '\n' && text[i] != '\r') text[i] = ' ';
        }
    }

    // Skip whitespace (including // and /* */ comments) at pos
    void SkipWS(size_t& pos) const {
        const std::string& text = *PDefText;
        while (pos < text.size()) {
            char c = text[pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { pos++; continue; }
            if (c == '/' && pos + 1 < text.size()) {
                if (text[pos + 1] == '/') { // line comment
                    while (pos < text.size() && text[pos] != '\n') pos++;
                    continue;
                }
                if (text[pos + 1] == '*') { // block comment
                    pos += 2;
                    while (pos + 1 < text.size() && !(text[pos] == '*' && text[pos + 1] == '/')) pos++;
                    if (pos + 1 < text.size()) pos += 2;
                    continue;
                }
            }
            break;
        }
    }

    // Read an identifier at pos
    std::string ReadIdent(size_t& pos) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        std::string result;
        while (pos < text.size() && (std::isalnum((unsigned char)text[pos]) || text[pos] == '_'))
            result += text[pos++];
        return result;
    }

    // Read a quoted string at pos (without the quotes)
    std::string ReadQuoted(size_t& pos) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        if (pos >= text.size() || text[pos] != '"') return "";
        pos++; // skip opening "
        std::string result;
        while (pos < text.size() && text[pos] != '"') {
            if (text[pos] == '\\' && pos + 1 < text.size()) {
                pos++;
                switch (text[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default:
                    // If it's an unknown escape (like \V), keep the backslash!
                    result += '\\';
                    result += text[pos];
                    break;
                }
            }
            else {
                result += text[pos];
            }
            pos++;
        }
        if (pos < text.size()) pos++; // skip closing "
        return result;
    }

    // Read either a quoted string or an identifier
    std::string ReadString(size_t& pos) const {
        SkipWS(pos);
        if (pos < PDefText->size() && (*PDefText)[pos] == '"') return ReadQuoted(pos);
        return ReadIdent(pos);
    }

    // Read a single integer term (symbol lookup or literal), no operators
    int32_t ReadIntTerm(size_t& pos) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        if (pos >= text.size()) return 0;
        bool neg = false;
        if (text[pos] == '-') { neg = true; pos++; SkipWS(pos); }
        if (pos >= text.size()) return 0;
        if (std::isalpha((unsigned char)text[pos]) || text[pos] == '_') {
            std::string ident = ReadIdent(pos);
            if (PSymbolMap) {
                auto it = PSymbolMap->find(ident);
                if (it != PSymbolMap->end()) return neg ? -it->second : it->second;
            }
            return 0;
        }
        if (text[pos] == '0' && pos + 1 < text.size() && (text[pos+1] == 'x' || text[pos+1] == 'X')) {
            pos += 2;
            int32_t val = 0;
            while (pos < text.size() && std::isxdigit((unsigned char)text[pos])) {
                char c = text[pos++];
                val = val * 16 + (std::isdigit((unsigned char)c) ? c - '0' : std::tolower((unsigned char)c) - 'a' + 10);
            }
            return neg ? -val : val;
        }
        if (std::isdigit((unsigned char)text[pos])) {
            int32_t val = 0;
            while (pos < text.size() && std::isdigit((unsigned char)text[pos]))
                val = val * 10 + (text[pos++] - '0');
            return neg ? -val : val;
        }
        return 0;
    }

    // Read integer expression with | + - << >> operators
    int32_t ReadIntExpr(size_t& pos) const {
        int32_t result = ReadIntTerm(pos);
        const std::string& text = *PDefText;
        while (true) {
            SkipWS(pos);
            if (pos >= text.size()) break;
            char c = text[pos];
            if (c == '|') { pos++; result = (int32_t)((uint32_t)result | (uint32_t)ReadIntTerm(pos)); }
            else if (c == '+') { pos++; result += ReadIntTerm(pos); }
            else if (c == '-' && pos+1 < text.size() && !std::isdigit((unsigned char)text[pos+1])) break;
            else if (c == '<' && pos + 1 < text.size() && text[pos+1] == '<') { pos += 2; int s = ReadIntTerm(pos); result <<= s; }
            else if (c == '>' && pos + 1 < text.size() && text[pos+1] == '>') { pos += 2; int s = ReadIntTerm(pos); result >>= s; }
            else break;
        }
        return result;
    }

    // Read a float at pos
    float ReadFloat(size_t& pos) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        size_t start = pos;
        if (pos < text.size() && text[pos] == '-') pos++;
        while (pos < text.size() && (std::isdigit((unsigned char)text[pos]) || text[pos] == '.')) pos++;
        if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E')) {
            pos++;
            if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) pos++;
            while (pos < text.size() && std::isdigit((unsigned char)text[pos])) pos++;
        }
        try { return std::stof(text.substr(start, pos - start)); }
        catch (...) { return 0.0f; }
    }

    // Skip past expected character, return true if found
    bool SkipChar(size_t& pos, char ch) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        if (pos < text.size() && text[pos] == ch) { pos++; return true; }
        return false;
    }

    // Skip past a specific string token
    bool SkipToken(size_t& pos, const char* tok) const {
        const std::string& text = *PDefText;
        SkipWS(pos);
        size_t tlen = std::strlen(tok);
        if (pos + tlen <= text.size() && text.compare(pos, tlen, tok) == 0) {
            pos += tlen;
            return true;
        }
        return false;
    }

    // Find ';' starting at pos, return pos AFTER ';', or npos
    size_t FindSemicolon(size_t pos) const {
        if (!PDefText) return std::string::npos;
        const std::string& text = *PDefText;
        while (pos < text.size()) {
            if (text[pos] == ';') return pos + 1;
            if (text[pos] == '/' && pos + 1 < text.size()) {
                if (text[pos+1] == '/') { while (pos < text.size() && text[pos] != '\n') pos++; continue; }
                if (text[pos+1] == '*') { pos += 2; while (pos + 1 < text.size() && !(text[pos] == '*' && text[pos+1] == '/')) pos++; if (pos+1<text.size()) pos+=2; continue; }
            }
            pos++;
        }
        return std::string::npos;
    }

public:
    // --------------------------------------------------------
    // SCALAR Transfer — mirrors CPersistContext::Transfer<T> MODE_SAVE_BINARY / MODE_LOAD_TEXT
    // --------------------------------------------------------

    void Transfer(const char* name, bool& val, const bool& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            // Read boolean: BTRUE/BFALSE or 0/1
            std::string tok = ReadIdent(pos);
            if (!tok.empty()) {
                std::string upper = tok;
                std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                if (upper == "BTRUE" || upper == "TRUE" || upper == "1") val = true;
                else val = false;
            } else {
                val = ReadIntTerm(pos) != 0;
            }
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            // DELETED: && val == defaultVal
            if (m_ForceNoTags) { PSaveStream->WriteEBOOL(val); return; }
            WriteTag(name);
            PSaveStream->WriteEBOOL(val);
        }
    }

    void Transfer(const char* name, int32_t& val, const int32_t& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            val = ReadIntExpr(pos);
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->WriteSLONG(val); return; }
            WriteTag(name);
            PSaveStream->WriteSLONG(val);
        }
    }

    void Transfer(const char* name, uint32_t& val, const uint32_t& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            val = (uint32_t)ReadIntExpr(pos);
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->WriteSLONG((int32_t)val); return; }
            WriteTag(name);
            PSaveStream->WriteSLONG((int32_t)val);
        }
    }

    void Transfer(const char* name, float& val, const float& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            val = ReadFloat(pos);
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->WriteFloat(val); return; }
            WriteTag(name);
            PSaveStream->WriteFloat(val);
        }
    }

    void Transfer(const char* name, std::wstring& val, const std::wstring& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            const std::string& text = *PDefText;
            SkipWS(pos);
            std::string raw;
            if (pos < text.size() && text[pos] == '"') {
                raw = ReadQuoted(pos);
            }
            else {
                raw = ReadIdent(pos);
            }
            // Convert UTF-8/Latin-1 to wstring
            val.clear();
            for (unsigned char c : raw) val += (wchar_t)c;
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->WriteNullTerminatedWString(val); return; }
            WriteTag(name);
            PSaveStream->WriteNullTerminatedWString(val);
        }
    }

    // CCharString (std::string, null-term length-prefixed)
    void Transfer(const char* name, std::string& val, const std::string& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            const std::string& text = *PDefText;
            SkipWS(pos);
            if (pos < text.size() && text[pos] == '"') {
                val = ReadQuoted(pos);
            }
            else {
                val = ReadIdent(pos);
            }
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->WriteCharString(val); return; }
            WriteTag(name);
            PSaveStream->WriteCharString(val);
        }
    }

    void Transfer(const char* name, uint8_t& val, const uint8_t& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            val = (uint8_t)ReadIntExpr(pos);
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            if (m_ForceNoTags) { PSaveStream->Write(&val, 1); return; }
            WriteTag(name);
            PSaveStream->Write(&val, 1);
        }
    }

    void Transfer(const char* name, C2DVector& val, const C2DVector& defaultVal);

    void Transfer(const char* name, CDefString& val, const CDefString& defaultVal);

    // --------------------------------------------------------
    // No-CRC helpers (for sub-component inline binary writes)
    // --------------------------------------------------------
    void TransferNoCRC(std::string& val)  { if (Mode == MODE_SAVE_BINARY) PSaveStream->WriteCharString(val); }
    void TransferNoCRC(uint32_t& val)     { if (Mode == MODE_SAVE_BINARY) PSaveStream->WriteULONG(val); }
    void TransferNoCRC(int32_t& val)      { if (Mode == MODE_SAVE_BINARY) PSaveStream->WriteSLONG(val); }
    void TransferNoCRC(float& val)        { if (Mode == MODE_SAVE_BINARY) PSaveStream->WriteFloat(val); }
    void TransferNoCRC(bool& val)         { if (Mode == MODE_SAVE_BINARY) PSaveStream->WriteEBOOL(val); }

    // --------------------------------------------------------
    // TransferVector — GFSerialiseVectorBinaryOut<T> equivalent
    // Text mode: scans for "name.Add(val);" and "name[i] val;" patterns
    // --------------------------------------------------------

    // Generic primitive vector
    template<typename T>
    void TransferVector(const char* name, std::vector<T>& vec) {
        if (Mode == MODE_LOAD_TEXT) {
            if (!PDefText) return;
            std::string& text = *PDefText;
            size_t nlen = std::strlen(name);
            size_t pos = 0;
            while (true) {
                // Find next whole-word occurrence of name
                size_t found = std::string::npos;
                while (pos + nlen <= text.size()) {
                    size_t p = text.find(name, pos);
                    if (p == std::string::npos) break;
                    bool prevOk = (p == 0) || IsWordBoundary(text[p - 1]);
                    bool nextOk = (p + nlen >= text.size()) || IsWordBoundary(text[p + nlen]);
                    if (prevOk && nextOk) { found = p; break; }
                    pos = p + 1;
                }
                if (found == std::string::npos) break;
                size_t fieldStart = found;
                size_t scanPos = found + nlen;

                SkipWS(scanPos);
                if (scanPos >= text.size()) break;
                char nextCh = text[scanPos];

                if (nextCh == '.') {
                    scanPos++;
                    std::string cmd = ReadIdent(scanPos);
                    if (cmd == "Add") {
                        if (!SkipChar(scanPos, '(')) { pos = scanPos; continue; }
                        T item{};
                        ReadVecItem(scanPos, item);
                        if (!SkipChar(scanPos, ')')) { pos = scanPos; continue; }
                        size_t sc = FindSemicolon(scanPos);
                        BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                        vec.push_back(item);
                        pos = fieldStart; // re-scan (blanked region won't match again)
                    }
                    else if (cmd == "clear") {
                        SkipChar(scanPos, '('); SkipChar(scanPos, ')');
                        size_t sc = FindSemicolon(scanPos);
                        BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                        vec.clear();
                        pos = fieldStart;
                    }
                    else if (cmd == "resize") {
                        SkipChar(scanPos, '(');
                        int32_t newSize = ReadIntExpr(scanPos);
                        SkipChar(scanPos, ')');
                        size_t sc = FindSemicolon(scanPos);
                        BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                        vec.resize((size_t)newSize);
                        pos = fieldStart;
                    }
                    else {
                        pos = scanPos;
                    }
                }
                else if (nextCh == '[') {
                    scanPos++;
                    int32_t idx = ReadIntExpr(scanPos);
                    SkipChar(scanPos, ']');
                    if (idx >= 0 && (size_t)idx >= vec.size()) vec.resize((size_t)idx + 1);
                    if (idx >= 0 && (size_t)idx < vec.size()) {
                        ReadVecItem(scanPos, vec[(size_t)idx]);
                    }
                    size_t sc = FindSemicolon(scanPos);
                    BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                    pos = fieldStart;
                }
                else {
                    pos = found + 1; // Not a vector command, skip
                }
            }
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            //if (!m_ForceNoTags && vec.empty()) return; // SKIP EMPTY VECTORS

            WriteTag(name);
            PSaveStream->WriteSLONG((int32_t)vec.size());
            for (auto& item : vec) WriteVecItem(item);
        }
    }

    template<typename T>
    void TransferVectorOfSubComponents(const char* name, std::vector<T>& vec) {
        if (Mode == MODE_LOAD_TEXT) {
            if (!PDefText) return;
            std::string& text = *PDefText;

            // Step 1: Discover all indices used for this vector prefix (e.g. States[X])
            size_t nlen = std::strlen(name);
            size_t pos = 0;
            int32_t maxIdx = -1;
            std::vector<int32_t> usedIndices;

            while (true) {
                size_t found = FindLiteralWholeWord(text, name, pos);
                if (found == std::string::npos) break;

                size_t scanPos = found;
                SkipWS(scanPos);
                if (scanPos < text.size() && text[scanPos] == '[') {
                    scanPos++;
                    int32_t idx = ReadIntExpr(scanPos);
                    if (idx >= 0) {
                        if (idx > maxIdx) maxIdx = idx;
                        bool exists = false;
                        for (int32_t used : usedIndices) if (used == idx) exists = true;
                        if (!exists) usedIndices.push_back(idx);
                    }
                    SkipWS(scanPos);
                    if (scanPos < text.size() && text[scanPos] == ']') scanPos++;
                    pos = scanPos;
                }
                else {
                    pos = found;
                }
            }

            if (maxIdx == -1) return;
            // Sorting indices to match vanilla serialization order (usually 0, 1, 2...)
            std::sort(usedIndices.begin(), usedIndices.end());

            if ((size_t)(maxIdx + 1) > vec.size()) vec.resize((size_t)maxIdx + 1);

            // Step 2: For each used index, call Transfer with a filter
            for (int32_t idx : usedIndices) {
                m_VectorFilter = name;
                m_VectorIndex = idx;
                vec[idx].Transfer(*this);
            }
            m_VectorFilter = "";
            m_VectorIndex = -1;
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            //if (!m_ForceNoTags && vec.empty()) return; // SKIP EMPTY VECTORS

            WriteTag(name);
            PSaveStream->WriteSLONG((int32_t)vec.size());
            for (auto& item : vec) item.Transfer(*this);
        }
    }

    template<typename K, typename V>
    void TransferMap(const char* name, std::map<K, V>& m, const V& defaultV) {
        if (Mode == MODE_LOAD_TEXT) {
            if (!PDefText) return;
            std::string& text = *PDefText;
            size_t nlen = std::strlen(name);
            size_t pos = 0;
            while (true) {
                size_t found = std::string::npos;
                while (pos + nlen <= text.size()) {
                    size_t p = text.find(name, pos);
                    if (p == std::string::npos) break;
                    bool prevOk = (p == 0) || IsWordBoundary(text[p - 1]);
                    bool nextOk = (p + nlen >= text.size()) || IsWordBoundary(text[p + nlen]);
                    if (prevOk && nextOk) { found = p; break; }
                    pos = p + 1;
                }
                if (found == std::string::npos) break;
                size_t fieldStart = found;
                size_t scanPos = found + nlen;
                SkipWS(scanPos);
                if (scanPos >= text.size() || text[scanPos] != '[') { pos = found + 1; continue; }
                scanPos++;
                K key = ReadMapKey<K>(scanPos);
                SkipChar(scanPos, ']');
                V val = defaultV;
                ReadMapValue(scanPos, val);
                size_t sc = FindSemicolon(scanPos);
                BlankRegion(fieldStart, sc != std::string::npos ? sc : scanPos);
                m[key] = val;
                pos = fieldStart;
            }
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            //if (!m_ForceNoTags && m.empty()) return; // SKIP EMPTY MAPS

            WriteTag(name);
            PSaveStream->WriteSLONG((int32_t)m.size());
            for (auto& pair : m) {
                WriteMapKey(pair.first);
                WriteMapVal(pair.second);
            }
        }
    }

    template<typename T>
    void TransferEnum(const char* name, T& val, const T& defaultVal) {
        if (Mode == MODE_LOAD_TEXT) {
            size_t after = FindWholeWord(name);
            if (after == std::string::npos) return;
            size_t fieldStart = after - std::strlen(name);
            size_t pos = after;
            val = (T)ReadIntExpr(pos);
            size_t sc = FindSemicolon(pos);
            BlankRegion(fieldStart, sc != std::string::npos ? sc : pos);
            return;
        }
        if (Mode == MODE_SAVE_BINARY) {
            // ENUM UNCONDITIONAL WRITE: We intentionally DO NOT check if val == defaultVal.
            // Lionhead's engine always writes Enum tags and values to the binary.
            WriteTag(name);
            PSaveStream->WriteSLONG((int32_t)val);
        }
    }

private:
    // --------------------------------------------------------
    // Item read/write helpers (specialized per type)
    // --------------------------------------------------------

    template<typename T>
    void ReadVecItem(size_t& pos, T& item) {
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_enum_v<T>) {
            item = (T)ReadIntExpr(pos);
        } else if constexpr (std::is_same_v<T, float>) {
            item = ReadFloat(pos);
        } else if constexpr (std::is_same_v<T, bool>) {
            item = ReadIntExpr(pos) != 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            const std::string& text = *PDefText;
            SkipWS(pos);
            if (pos < text.size() && text[pos] == '"') item = ReadQuoted(pos);
            else item = ReadIdent(pos);
        } else if constexpr (std::is_same_v<T, std::wstring>) {
            std::string raw = ReadQuoted(pos);
            item.clear();
            for (unsigned char c : raw) item += (wchar_t)c;
        } else {
            // Complex type — create a sub-context
            // (handled in specializations below)
        }
    }

    template<typename T>
    void WriteVecItem(T& item) {
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_enum_v<T>) {
            PSaveStream->WriteSLONG((int32_t)item);
        } else if constexpr (std::is_same_v<T, float>) {
            PSaveStream->WriteFloat(item);
        } else if constexpr (std::is_same_v<T, bool>) {
            PSaveStream->WriteEBOOL(item);
        } else if constexpr (std::is_same_v<T, std::string>) {
            PSaveStream->WriteCharString(item);
        } else if constexpr (std::is_same_v<T, std::wstring>) {
            PSaveStream->WriteNullTerminatedWString(item);
        } else {
            item.Transfer(*this);
        }
    }

    template<typename K>
    K ReadMapKey(size_t& pos) const {
        if constexpr (std::is_integral_v<K> || std::is_enum_v<K>) {
            return (K)ReadIntExpr(pos);
        } else if constexpr (std::is_same_v<K, std::string>) {
            const std::string& text = *PDefText;
            SkipWS(pos);
            if (pos < text.size() && text[pos] == '"') return ReadQuoted(pos);
            return ReadIdent(pos);
        }
        return K{};
    }

    template<typename K>
    void WriteMapKey(const K& key) const {
        if constexpr (std::is_integral_v<K> || std::is_enum_v<K>) {
            PSaveStream->WriteSLONG((int32_t)key);
        } else if constexpr (std::is_same_v<K, std::string>) {
            PSaveStream->WriteCharString(key);
        }
    }

    template<typename V>
    void ReadMapValue(size_t& pos, V& val) const {
        if constexpr (std::is_same_v<V, int32_t> || std::is_same_v<V, uint32_t> || std::is_enum_v<V>) {
            val = (V)ReadIntExpr(pos);
        } else if constexpr (std::is_same_v<V, float>) {
            val = ReadFloat(pos);
        } else if constexpr (std::is_same_v<V, std::string>) {
            const std::string& text = *PDefText;
            SkipWS(pos);
            if (pos < text.size() && text[pos] == '"') val = ReadQuoted(pos);
            else val = ReadIdent(pos);
        }
    }

    template<typename V>
    void WriteMapVal(const V& val) const {
        if constexpr (std::is_integral_v<V> || std::is_enum_v<V>) {
            PSaveStream->WriteSLONG((int32_t)val);
        } else if constexpr (std::is_same_v<V, float>) {
            PSaveStream->WriteFloat(val);
        } else if constexpr (std::is_same_v<V, std::string>) {
            PSaveStream->WriteCharString(val);
        }
    }

public:
    // --------------------------------------------------------
    // Public accessors used by CActionInputControl::ParseInline
    // These expose the private parsing helpers with public names
    // --------------------------------------------------------
    bool    SkipChar_pub(size_t& pos, char ch)          { return SkipChar(pos, ch); }
    bool    SkipToken_pub(size_t& pos, const char* tok) { return SkipToken(pos, tok); }
    int32_t ReadIntExpr_pub(size_t& pos)               { return ReadIntExpr(pos); }
    float   ReadFloat_pub(size_t& pos)                 { return ReadFloat(pos); }
    void    SkipWS_pub(size_t& pos)                    { SkipWS(pos); }
    std::string& GetDefText()                          { return *PDefText; }
};

// ============================================================
// CMemoryDataOutputStream
// ============================================================
class CMemoryDataOutputStream : public CDataOutputStream {
private:
    std::vector<uint8_t> m_buffer;
public:
    void Write(const void* data, size_t size) override {
        if (size == 0 || data == nullptr) return;
        const uint8_t* byteData = static_cast<const uint8_t*>(data);
        m_buffer.insert(m_buffer.end(), byteData, byteData + size);
    }
    size_t GetLength() const { return m_buffer.size(); }
    const void* PeekData() const { return m_buffer.empty() ? nullptr : m_buffer.data(); }
    void Clear() { m_buffer.clear(); }
    std::vector<uint8_t>& GetBuffer() { return m_buffer; }
};