#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring> 
#include <codecvt>

struct CTextTag {
    int32_t Position = 0;
    std::string Name;
};

struct CTextGroupItem {
    uint32_t ID;
    std::string CachedName;
    std::string CachedContent;
};

struct CTextEntry {
    std::wstring Content; // Content is FIRST
    std::string SpeechBank;
    std::string Speaker;
    std::string Identifier;
    std::vector<CTextTag> Tags;
};

struct CTextGroup {
    std::vector<CTextGroupItem> Items;
};

class CTextParser {
public:
    CTextEntry TextData;
    CTextGroup GroupData;

    // Narrator List Data
    uint32_t NarratorStartID = 0; // The "7B 01 00 00" from the dump
    std::vector<std::string> NarratorStrings;

    std::vector<uint8_t> RawData;

    bool IsParsed = false;
    bool IsGroup = false;
    bool IsNarratorList = false;

    std::string DebugLog;

    // --- READ HELPERS ---
    std::wstring ReadWString(const uint8_t* data, size_t& offset, size_t max) {
        std::wstring res;
        while (offset + 2 <= max) {
            uint16_t c = *(uint16_t*)(data + offset);
            offset += 2;
            if (c == 0) break;
            res += (wchar_t)c;
        }
        return res;
    }

    std::string ReadPresizedString(const uint8_t* data, size_t& offset, size_t max) {
        if (offset + 4 > max) return "";
        uint32_t len = *(uint32_t*)(data + offset);
        offset += 4;
        if (len == 0 || offset + len > max) return "";
        std::string res((const char*)(data + offset), len);
        offset += len;
        return res;
    }

    std::string ReadNullTermString(const uint8_t* data, size_t& offset, size_t max) {
        std::string res;
        while (offset < max) {
            char c = (char)data[offset++];
            if (c == 0) break;
            res += c;
        }
        return res;
    }

    // --- WRITE HELPERS ---
    void WriteWString(std::vector<uint8_t>& buf, const std::wstring& str) {
        for (wchar_t c : str) {
            uint16_t val = (uint16_t)c;
            buf.push_back(val & 0xFF);
            buf.push_back((val >> 8) & 0xFF);
        }
        buf.push_back(0); buf.push_back(0); // Wide Null Terminator
    }

    void WritePresizedString(std::vector<uint8_t>& buf, const std::string& str) {
        uint32_t len = (uint32_t)str.length();
        buf.insert(buf.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
        if (len > 0) {
            buf.insert(buf.end(), str.begin(), str.end());
        }
    }

    void WriteNullTermString(std::vector<uint8_t>& buf, const std::string& str) {
        buf.insert(buf.end(), str.begin(), str.end());
        buf.push_back(0);
    }

    // --- MAIN FUNCTIONS ---

    void Parse(const std::vector<uint8_t>& data, int entryType) {
        IsParsed = false;
        IsGroup = false;
        IsNarratorList = false;
        TextData = CTextEntry();
        GroupData = CTextGroup();
        NarratorStrings.clear();
        RawData = data;
        DebugLog = "";
        NarratorStartID = 0;

        if (data.empty()) return;

        size_t cursor = 0;
        size_t size = data.size();
        const uint8_t* ptr = data.data();

        try {
            if (entryType == 1) { // TYPE_GROUP
                IsGroup = true;
                if (cursor + 4 <= size) {
                    uint32_t count = *(uint32_t*)(ptr + cursor);
                    cursor += 4;

                    for (uint32_t i = 0; i < count; i++) {
                        if (cursor + 4 > size) break;
                        uint32_t id = *(uint32_t*)(ptr + cursor);
                        cursor += 4;

                        CTextGroupItem item;
                        item.ID = id;
                        GroupData.Items.push_back(item);
                    }
                }
                IsParsed = true;
            }
            else if (entryType == 2) { // TYPE_NARRATOR_LIST
                IsNarratorList = true;
                const uint8_t sig[] = { '[', 'N', 'a', 'r', 'r', 'a', 't', 'o', 'r', 'L', 'i', 's', 't', ']' };
                size_t sigLen = 14;

                size_t foundOffset = -1;
                for (size_t i = 0; i < size - sigLen; i++) {
                    if (memcmp(ptr + i, sig, sigLen) == 0) {
                        foundOffset = i;
                        break;
                    }
                }

                if (foundOffset != -1) {
                    cursor = foundOffset + sigLen;

                    // SKIP PADDING (00 00 00 00) - Confirmed by hex dump
                    if (cursor + 4 <= size) cursor += 4;

                    uint32_t count = 0;
                    if (cursor + 4 <= size) {
                        count = *(uint32_t*)(ptr + cursor);
                        cursor += 4;
                    }

                    // READ START ID (e.g. 7B 01 00 00) - Confirmed by hex dump
                    if (cursor + 4 <= size) {
                        NarratorStartID = *(uint32_t*)(ptr + cursor);
                        cursor += 4;
                    }

                    if (count > 0 && count < 20000) {
                        for (uint32_t i = 0; i < count; i++) {
                            if (cursor >= size) break;
                            std::string s = ReadNullTermString(ptr, cursor, size);
                            NarratorStrings.push_back(s);
                        }
                    }
                }
                IsParsed = true;
            }
            else { // TYPE_TEXT (Default - Type 0)
                IsGroup = false;

                // ORIGINAL ORDER (Correct)
                TextData.Content = ReadWString(ptr, cursor, size);
                TextData.SpeechBank = ReadPresizedString(ptr, cursor, size);
                TextData.Speaker = ReadPresizedString(ptr, cursor, size);
                TextData.Identifier = ReadPresizedString(ptr, cursor, size);

                if (cursor + 4 <= size) {
                    uint32_t tagCount = *(uint32_t*)(ptr + cursor);
                    cursor += 4;
                    for (uint32_t i = 0; i < tagCount; i++) {
                        if (cursor + 4 > size) break;
                        CTextTag tag;
                        tag.Position = *(int32_t*)(ptr + cursor);
                        cursor += 4;
                        tag.Name = ReadNullTermString(ptr, cursor, size);
                        TextData.Tags.push_back(tag);
                    }
                }
                IsParsed = true;
            }
        }
        catch (...) {
            DebugLog = "Exception during parsing.";
            IsParsed = false;
        }
    }

    // --- RECOMPILE ---
    std::vector<uint8_t> Recompile() {
        std::vector<uint8_t> buf;

        if (IsGroup) {
            uint32_t count = (uint32_t)GroupData.Items.size();
            buf.insert(buf.end(), (uint8_t*)&count, (uint8_t*)&count + 4);
            for (const auto& item : GroupData.Items) {
                buf.insert(buf.end(), (uint8_t*)&item.ID, (uint8_t*)&item.ID + 4);
            }
        }
        else if (IsNarratorList) {
            // Rebuild Signature
            std::string sig = "[NarratorList]";
            buf.insert(buf.end(), sig.begin(), sig.end());

            // 4 bytes padding
            uint32_t pad = 0;
            buf.insert(buf.end(), (uint8_t*)&pad, (uint8_t*)&pad + 4);

            // Count
            uint32_t count = (uint32_t)NarratorStrings.size();
            buf.insert(buf.end(), (uint8_t*)&count, (uint8_t*)&count + 4);

            // Start ID (Preserve the one we read)
            buf.insert(buf.end(), (uint8_t*)&NarratorStartID, (uint8_t*)&NarratorStartID + 4);

            // Strings
            for (const auto& str : NarratorStrings) {
                WriteNullTermString(buf, str);
            }
        }
        else {
            // Type 0 - ORIGINAL ORDER (Correct)
            WriteWString(buf, TextData.Content);
            WritePresizedString(buf, TextData.SpeechBank);
            WritePresizedString(buf, TextData.Speaker);
            WritePresizedString(buf, TextData.Identifier);

            uint32_t tagCount = (uint32_t)TextData.Tags.size();
            buf.insert(buf.end(), (uint8_t*)&tagCount, (uint8_t*)&tagCount + 4);

            for (const auto& tag : TextData.Tags) {
                buf.insert(buf.end(), (uint8_t*)&tag.Position, (uint8_t*)&tag.Position + 4);
                WriteNullTermString(buf, tag.Name);
            }
        }
        return buf;
    }
};