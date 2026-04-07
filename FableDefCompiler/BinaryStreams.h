#pragma once
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <cstdint>

struct C2DVector { float x, y; };
struct C3DVector { float x, y, z; };

class CDefStringTable {
private:
    uint32_t m_RandomID;
    uint32_t m_StringCount;
    std::vector<uint8_t> m_StringStream;
    std::map<std::string, uint32_t> m_StringMap;

public:
    CDefStringTable(uint32_t randomId = 0xDEADBEEF) : m_RandomID(randomId), m_StringCount(0) {}

    static uint32_t GetCRC(const std::string& str) {
        uint32_t crc = 0;
        for (char c : str) {
            crc ^= static_cast<uint8_t>(c);
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (-(int32_t)(crc & 1) & 0xEDB88320);
            }
        }
        return crc;
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
        if (length > 0) {
            os.write(reinterpret_cast<const char*>(m_StringStream.data()), length);
        }
        os.close();
    }
};

class CDataOutputStream {
public:
    virtual ~CDataOutputStream() = default;
    virtual void Write(const void* data, size_t size) = 0;

    void WriteSLONG(int32_t val) { Write(&val, sizeof(val)); }
    void WriteULONG(uint32_t val) { Write(&val, sizeof(val)); }
    void WriteFloat(float val) { Write(&val, sizeof(val)); }
    void WriteBool(bool val) { Write(&val, sizeof(val)); }
    void WriteSWORD(int16_t val) { Write(&val, sizeof(val)); }
    void WriteEBOOL(int32_t val) { Write(&val, sizeof(val)); }

    void Write2DVector(const C2DVector* vec) { Write(vec, sizeof(C2DVector)); }
    void Write3DVector(const C3DVector* vec) { Write(vec, sizeof(C3DVector)); }

    void WriteString(const std::string& str) {
        uint32_t crc = CDefStringTable::GetCRC(str);
        WriteULONG(crc);
    }

    void WriteNullTerminatedString(const std::string& str) {
        Write(str.c_str(), str.length() + 1);
    }

    void WriteWideString(const std::wstring& wstr) {
        WriteSLONG((int32_t)wstr.length());
        if (!wstr.empty()) {
            Write(wstr.c_str(), wstr.length() * sizeof(wchar_t));
        }
        uint16_t nullTerm = 0;
        Write(&nullTerm, 2);
    }

    void WriteVectorUint32(const std::vector<uint32_t>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (uint32_t v : vec) WriteULONG(v);
    }

    void WriteVectorFloat(const std::vector<float>& vec) {
        WriteSLONG((int32_t)vec.size());
        for (float v : vec) WriteFloat(v);
    }

    void WriteMapUint32ToString(const std::map<uint32_t, std::string>& m) {
        WriteSLONG((int32_t)m.size());
        for (const auto& pair : m) {
            WriteULONG(pair.first);
            WriteNullTerminatedString(pair.second);
        }
    }

    void WriteMapUint32ToUint32(const std::map<uint32_t, uint32_t>& m) {
        WriteSLONG((int32_t)m.size());
        for (const auto& pair : m) {
            WriteULONG(pair.first);
            WriteULONG(pair.second);
        }
    }

    void WriteCharString(const std::string& str) {
        WriteSLONG((int32_t)str.length());
        if (!str.empty()) Write(str.c_str(), str.length());
    }

    void WriteMapInt32ToString(const std::map<int32_t, std::string>& m) {
        WriteSLONG((int32_t)m.size());
        for (const auto& pair : m) {
            WriteSLONG(pair.first);
            WriteNullTerminatedString(pair.second);
        }
    }

};

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