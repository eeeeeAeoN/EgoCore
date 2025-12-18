#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct CTextTag {
    int32_t Position;
    std::string Name;
};

// [NEW] Holds cached metadata for group items
struct CTextGroupItem {
    uint32_t ID;
    std::string CachedName;    // Populated after parse
    std::string CachedContent; // Populated after parse
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
    bool IsParsed = false;
    bool IsGroup = false;
    std::string DebugLog;

    // Helper: Read Null-Terminated Wide String
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

    // Helper: Read Presized String
    std::string ReadPresizedString(const uint8_t* data, size_t& offset, size_t max) {
        if (offset + 4 > max) return "";
        uint32_t len = *(uint32_t*)(data + offset);
        offset += 4;
        if (len == 0 || offset + len > max) return "";
        std::string res((const char*)(data + offset), len);
        offset += len;
        return res;
    }

    // Helper: Read Null-Terminated String
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
        TextData = CTextEntry();
        GroupData = CTextGroup();
        DebugLog = "";

        if (data.empty()) return;

        size_t cursor = 0;
        size_t size = data.size();
        const uint8_t* ptr = data.data();

        try {
            // --- TYPE 1: GROUP ENTRY ---
            if (entryType == 1) {
                IsGroup = true;
                if (cursor + 4 <= size) {
                    uint32_t count = *(uint32_t*)(ptr + cursor);
                    cursor += 4;

                    for (uint32_t i = 0; i < count; i++) {
                        if (cursor + 4 > size) break;
                        uint32_t id = *(uint32_t*)(ptr + cursor);
                        cursor += 4;

                        // [FIX] Store in new Item struct
                        CTextGroupItem item;
                        item.ID = id;
                        // Names/Content will be filled by ResolveGroupMetadata later
                        GroupData.Items.push_back(item);
                    }
                }
                IsParsed = true;
            }
            // --- TYPE 0: TEXT ENTRY ---
            else {
                IsGroup = false;
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
};