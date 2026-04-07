#pragma once
#include <string>
#include <cctype>

// ---------------------------------------------------------
// Token Definitions
// ---------------------------------------------------------

enum class EParsedItemType {
    Unknown,
    Identifier,    // e.g., VILLAGE_DARKWOOD_TRADER_CAMP, TRUE
    QuotedString,  // e.g., "dummy_level.wld"
    Integer,       // e.g., 42, -1, 0x1A
    Float,         // e.g., 1.5, -0.5
    Symbol,        // e.g., {, }, =, ;
    EndOfFile
};

struct CParsedItem {
    EParsedItemType Type = EParsedItemType::Unknown;
    std::string StringValue = "";
    int IntValue = 0;
    float FloatValue = 0.0f;
};

// ---------------------------------------------------------
// Lexical Analyzer / Tokenizer
// ---------------------------------------------------------

class CStringParser {
private:
    const char* m_BufferStart;
    const char* m_Cursor;
    const char* m_BufferEnd;

    int m_CurrentLine;
    bool m_InQuotes;
    std::string m_Filename;

public:
    CStringParser();

    // --- Initialization ---
    void Init(const std::string& buffer, const std::string& filename = "");

    // --- State Checks ---
    bool IsEOF() const;
    int GetCurrentLine() const; // Equivalent to Lionhead's CountLinesParsed
    bool IsInQuotes() const;

    // --- State Saving for Peeking ---
    struct CursorState {
        const char* Ptr;
        int Line;
        bool InQuotes;
    };
    CursorState SaveState() const;
    void RestoreState(const CursorState& state);

    // --- Core Movement ---
    void MoveStringPos(unsigned int n);
    void SetStringPos(unsigned int n);

    // --- Core Skipping ---
    void SkipWhitespace(); // Handles spaces, tabs, // and /* */ comments
    unsigned int SkipUntilWhitespace();
    int SkipPastNewline();

    // --- Advanced Skipping ---
    bool SkipUntilString(const std::string& str);
    bool SkipUntilWholeString(const std::string& str);
    bool SkipPastString(const std::string& str);
    bool SkipPastWholeString(const std::string& str);

    // --- Lookahead / Peeking ---
    bool PeekNextItem(CParsedItem& item);
    EParsedItemType PeekNextItemType();
    bool NextItemExists();

    // --- Tokenizer Readers (The Engine) ---
    bool ReadNextItem(CParsedItem& item);
    bool ReadNextString(CParsedItem& item);
    bool ReadNextNumber(CParsedItem& item);
    bool ReadNextSymbol(CParsedItem& item);
    bool ReadNextItemAsQuotedString(std::string& outStr);
    bool ReadLineAsString(std::string& outStr);

    // --- Advanced Seeking & Extracting ---
    bool ReadAsStringUntilWhitespace(std::string& outStr);
    bool ReadAsStringUntilString(const std::string& target, std::string& outStr);
    bool ReadAsStringUntilWholeString(const std::string& target, std::string& outStr);
    bool ReadAsStringUntilPastString(const std::string& target, std::string& outStr);

    // --- Strict Typed Extractors ---
    bool ReadAsString(std::string& outStr);
    bool ReadAsSymbol(std::string& outSymbol);
    int ReadAsInteger();
    float ReadAsFloat();
    bool ReadAsIdentifierOrNumber(std::string& outStr);

    // --- Utilities ---
    void Error(const std::string& msg) const;

    // Static Helper mapping to Lionhead's CharSeperatesString
    static bool CharSeparatesString(char ch) {
        return !std::isalnum(static_cast<unsigned char>(ch)) && ch != '_';
    }
};