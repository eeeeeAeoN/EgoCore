#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring> 

struct CTextTag {
    int32_t Position;
    std::string Name;
};

struct CTextGroupItem {
    uint32_t ID;
    std::string CachedName;
    std::string CachedContent;
};

struct CTextEntry {
    std::wstring Content;
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

    // Added for Type 2 support
    std::vector<std::string> NarratorStrings;
    std::vector<uint8_t> RawData;

    bool IsParsed = false;
    bool IsGroup = false;
    bool IsNarratorList = false;

    std::string DebugLog;

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

    void Parse(const std::vector<uint8_t>& data, int entryType) {
        IsParsed = false;
        IsGroup = false;
        IsNarratorList = false;
        TextData = CTextEntry();
        GroupData = CTextGroup();
        NarratorStrings.clear();
        RawData = data;
        DebugLog = "";

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

                // Signature check: 0E 00 00 00 [NarratorList]
                const uint8_t sig[] = { 0x0E, 0x00, 0x00, 0x00, '[', 'N', 'a', 'r', 'r', 'a', 't', 'o', 'r', 'L', 'i', 's', 't', ']' };
                size_t sigLen = 18;

                size_t foundOffset = -1;
                for (size_t i = 0; i < size - sigLen; i++) {
                    if (memcmp(ptr + i, sig, sigLen) == 0) {
                        foundOffset = i;
                        break;
                    }
                }

                if (foundOffset != -1) {
                    cursor = foundOffset + sigLen;

                    // Skip padding zeros
                    while (cursor < size && ptr[cursor] == 0) cursor++;

                    // Skip Unknown Int (often 14 15 00 00)
                    if (cursor + 4 <= size) cursor += 4;

                    // Read Count
                    uint32_t count = 0;
                    if (cursor + 4 <= size) {
                        count = *(uint32_t*)(ptr + cursor);
                        cursor += 4;
                    }

                    // Read Strings
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
            else { // TYPE_TEXT (Default)
                IsGroup = false;

                // 1. Strings
                TextData.Content = ReadWString(ptr, cursor, size);
                TextData.SpeechBank = ReadPresizedString(ptr, cursor, size);
                TextData.Speaker = ReadPresizedString(ptr, cursor, size);
                TextData.Identifier = ReadPresizedString(ptr, cursor, size);

                // 2. Tags (Modifiers)
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
};