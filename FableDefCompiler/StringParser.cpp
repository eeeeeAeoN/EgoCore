#include "StringParser.h"
#include <cstring>

CStringParser::CStringParser()
    : m_BufferStart(nullptr), m_Cursor(nullptr), m_BufferEnd(nullptr),
    m_CurrentLine(1), m_InQuotes(false) {
}

void CStringParser::Init(const std::string& buffer, const std::string& filename) {
    m_BufferStart = buffer.data();
    m_Cursor = m_BufferStart;
    m_BufferEnd = m_BufferStart + buffer.length();

    m_CurrentLine = 1;
    m_InQuotes = false;
    m_Filename = filename;
}

bool CStringParser::IsEOF() const {
    return m_Cursor >= m_BufferEnd || *m_Cursor == '\0';
}

int CStringParser::GetCurrentLine() const { return m_CurrentLine; }
bool CStringParser::IsInQuotes() const { return m_InQuotes; }

void CStringParser::MoveStringPos(unsigned int n) {
    for (unsigned int i = 0; i < n && !IsEOF(); ++i) {
        if (*m_Cursor == '"' || *m_Cursor == '\'') m_InQuotes = !m_InQuotes;
        if (*m_Cursor == '\n') m_CurrentLine++;
        m_Cursor++;
    }
}

// ---------------------------------------------------------
// Skipping Functions
// ---------------------------------------------------------

void CStringParser::SkipWhitespace() {
    while (!IsEOF()) {
        // Space, Tab, \r, \n
        if (std::isspace(static_cast<unsigned char>(*m_Cursor))) {
            MoveStringPos(1);
            continue;
        }
        // // Comments
        if (*m_Cursor == '/' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '/') {
            SkipPastNewline();
            continue;
        }
        // /* Comments */
        if (*m_Cursor == '/' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '*') {
            MoveStringPos(2);
            while (!IsEOF()) {
                if (*m_Cursor == '*' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '/') {
                    MoveStringPos(2);
                    break;
                }
                MoveStringPos(1);
            }
            continue;
        }
        break;
    }
}

unsigned int CStringParser::SkipUntilWhitespace() {
    const char* start = m_Cursor;
    while (!IsEOF() && !std::isspace(static_cast<unsigned char>(*m_Cursor))) {
        MoveStringPos(1);
    }
    return static_cast<unsigned int>(m_Cursor - start);
}

int CStringParser::SkipPastNewline() {
    const char* start = m_Cursor;
    while (!IsEOF()) {
        char c = *m_Cursor;
        if (c == '\n' || c == '\r') {
            while (!IsEOF() && (*m_Cursor == '\n' || *m_Cursor == '\r')) MoveStringPos(1);
            break;
        }
        MoveStringPos(1);
    }
    return static_cast<int>(m_Cursor - start);
}

// ---------------------------------------------------------
// Strict Tokenizer (Matches Fable Engine)
// ---------------------------------------------------------

bool CStringParser::ReadNextItem(CParsedItem& item) {
    SkipWhitespace();

    if (IsEOF()) {
        item.Type = EParsedItemType::EndOfFile;
        return false;
    }

    char c = *m_Cursor;
    char nextC = (m_Cursor + 1 < m_BufferEnd) ? *(m_Cursor + 1) : '\0';

    // 1. Number?
    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && std::isdigit(static_cast<unsigned char>(nextC)))) {
        return ReadNextNumber(item);
    }

    // 2. Identifier? (Strictly alnum, _, or #)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '#') {
        return ReadNextString(item);
    }

    // 3. Symbol (Everything else, exactly 1 char)
    return ReadNextSymbol(item);
}

bool CStringParser::ReadNextString(CParsedItem& item) {
    if (IsEOF()) return false;
    const char* start = m_Cursor;

    // FABLE ENGINE RULE: Loop STOPS if it hits a bracket, dot, \r, \n, space, etc.
    while (!IsEOF() && (std::isalnum(static_cast<unsigned char>(*m_Cursor)) || *m_Cursor == '_' || *m_Cursor == '#')) {
        MoveStringPos(1);
    }

    if (m_Cursor == start) return false;
    item.Type = EParsedItemType::Identifier;
    item.StringValue = std::string(start, m_Cursor - start);
    return true;
}

bool CStringParser::ReadNextSymbol(CParsedItem& item) {
    if (IsEOF()) return false;

    item.Type = EParsedItemType::Symbol;
    item.StringValue = std::string(1, *m_Cursor); // Grab exactly 1 character
    MoveStringPos(1);
    return true;
}

bool CStringParser::ReadNextNumber(CParsedItem& item) {
    if (IsEOF()) return false;
    const char* start = m_Cursor;
    bool hasDecimal = false;
    bool isHex = false;

    if (*m_Cursor == '-') MoveStringPos(1);

    if (*m_Cursor == '0' && m_Cursor + 1 < m_BufferEnd && std::tolower(*(m_Cursor + 1)) == 'x') {
        isHex = true;
        MoveStringPos(2);
    }

    while (!IsEOF()) {
        char c = *m_Cursor;
        if (isHex ? std::isxdigit(static_cast<unsigned char>(c)) : std::isdigit(static_cast<unsigned char>(c))) {
            MoveStringPos(1);
        }
        else if (c == '.' && !hasDecimal && !isHex) {
            hasDecimal = true;
            MoveStringPos(1);
        }
        else break;
    }

    std::string numStr(start, m_Cursor - start);
    item.StringValue = numStr;

    try {
        if (hasDecimal) {
            item.Type = EParsedItemType::Float;
            item.FloatValue = std::stof(numStr);
        }
        else {
            item.Type = EParsedItemType::Integer;
            item.IntValue = std::stoi(numStr, nullptr, isHex ? 16 : 0);
        }
    }
    catch (...) {
        item.Type = EParsedItemType::Unknown;
        return false;
    }
    return true;
}

bool CStringParser::ReadLineAsString(std::string& outStr) {
    if (IsEOF()) return false;
    const char* start = m_Cursor;
    while (!IsEOF() && *m_Cursor != '\n' && *m_Cursor != '\r') MoveStringPos(1);
    outStr = std::string(start, m_Cursor - start);
    SkipPastNewline();
    return true;
}

bool CStringParser::ReadNextItemAsQuotedString(std::string& outStr) {
    SkipWhitespace();
    if (IsEOF()) return false;

    char quoteChar = *m_Cursor;
    if (quoteChar != '"' && quoteChar != '\'') return false;

    MoveStringPos(1);
    outStr.clear();

    while (!IsEOF()) {
        char c = *m_Cursor;
        if (c == '\\') {
            MoveStringPos(1);
            if (!IsEOF()) {
                outStr += *m_Cursor;
                MoveStringPos(1);
            }
        }
        else if (c == quoteChar) {
            MoveStringPos(1);
            return true;
        }
        else {
            outStr += c;
            MoveStringPos(1);
        }
    }
    return false;
}

void CStringParser::SetStringPos(unsigned int n) {
    if (n > (m_BufferEnd - m_BufferStart)) return;
    m_Cursor = m_BufferStart + n;
    m_InQuotes = false;
    for (const char* ptr = m_BufferStart; ptr < m_Cursor; ++ptr) {
        if (*ptr == '"' || *ptr == '\'') m_InQuotes = !m_InQuotes;
    }
}

// ---------------------------------------------------------
// Advanced Seekers and Extractors
// ---------------------------------------------------------

bool CStringParser::SkipUntilString(const std::string& str) {
    if (str.empty()) return false;
    while (!IsEOF()) {
        if (std::strncmp(m_Cursor, str.c_str(), str.length()) == 0) return true;
        MoveStringPos(1);
    }
    return false;
}

bool CStringParser::SkipUntilWholeString(const std::string& str) {
    if (str.empty()) return false;
    while (SkipUntilString(str)) {
        bool leftOk = (m_Cursor == m_BufferStart) || (!std::isalnum(*(m_Cursor - 1)) && *(m_Cursor - 1) != '_');
        const char* right = m_Cursor + str.length();
        bool rightOk = (right >= m_BufferEnd) || (!std::isalnum(*right) && *right != '_');
        if (leftOk && rightOk) return true;
        MoveStringPos(1);
    }
    return false;
}

bool CStringParser::SkipPastString(const std::string& str) {
    if (SkipUntilString(str)) { MoveStringPos(str.length()); return true; }
    return false;
}

bool CStringParser::SkipPastWholeString(const std::string& str) {
    if (SkipUntilWholeString(str)) { MoveStringPos(str.length()); return true; }
    return false;
}

bool CStringParser::ReadAsStringUntilWhitespace(std::string& outStr) {
    if (IsEOF()) return false;
    const char* start = m_Cursor;
    SkipUntilWhitespace();
    outStr = std::string(start, m_Cursor - start);
    return true;
}

bool CStringParser::ReadAsStringUntilString(const std::string& target, std::string& outStr) {
    const char* start = m_Cursor;
    if (SkipUntilString(target)) {
        outStr = std::string(start, m_Cursor - start);
        return true;
    }
    return false;
}

bool CStringParser::ReadAsStringUntilWholeString(const std::string& target, std::string& outStr) {
    const char* start = m_Cursor;
    if (SkipUntilWholeString(target)) {
        outStr = std::string(start, m_Cursor - start);
        return true;
    }
    return false;
}

bool CStringParser::ReadAsStringUntilPastString(const std::string& target, std::string& outStr) {
    const char* start = m_Cursor;
    if (SkipUntilString(target)) {
        MoveStringPos(target.length());
        outStr = std::string(start, m_Cursor - start);
        return true;
    }
    return false;
}

// ---------------------------------------------------------
// Strict Typed Extractors
// ---------------------------------------------------------

bool CStringParser::ReadAsString(std::string& outStr) {
    CParsedItem item;
    if (ReadNextItem(item) && item.Type == EParsedItemType::Identifier) {
        outStr = item.StringValue;
        return true;
    }
    Error("Error parsing string");
    return false;
}

bool CStringParser::ReadAsIdentifierOrNumber(std::string& outStr) {
    CParsedItem item;
    if (ReadNextItem(item)) {
        if (item.Type == EParsedItemType::Identifier || item.Type == EParsedItemType::Integer || item.Type == EParsedItemType::Float) {
            outStr = item.StringValue;
            return true;
        }
    }
    Error("Expected an identifier or a number.");
    return false;
}

bool CStringParser::ReadAsSymbol(std::string& outSymbol) {
    CParsedItem item;
    if (ReadNextItem(item) && item.Type == EParsedItemType::Symbol) {
        outSymbol = item.StringValue;
        return true;
    }
    Error("Error parsing symbol");
    return false;
}

int CStringParser::ReadAsInteger() {
    CParsedItem item;
    if (ReadNextItem(item) && item.Type == EParsedItemType::Integer) return item.IntValue;
    Error("Error parsing integer");
    return 0;
}

float CStringParser::ReadAsFloat() {
    CParsedItem item;
    if (ReadNextItem(item)) {
        if (item.Type == EParsedItemType::Float) return item.FloatValue;
        if (item.Type == EParsedItemType::Integer) return static_cast<float>(item.IntValue);
    }
    Error("Error parsing float");
    return 0.0f;
}

// ---------------------------------------------------------
// State Saving & Error
// ---------------------------------------------------------

CStringParser::CursorState CStringParser::SaveState() const { return { m_Cursor, m_CurrentLine, m_InQuotes }; }
void CStringParser::RestoreState(const CursorState& state) { m_Cursor = state.Ptr; m_CurrentLine = state.Line; m_InQuotes = state.InQuotes; }

bool CStringParser::PeekNextItem(CParsedItem& item) {
    CursorState oldState = SaveState();
    bool result = ReadNextItem(item);
    RestoreState(oldState);
    return result;
}

EParsedItemType CStringParser::PeekNextItemType() {
    CParsedItem item;
    return PeekNextItem(item) ? item.Type : EParsedItemType::Unknown;
}

bool CStringParser::NextItemExists() {
    CursorState oldState = SaveState();
    SkipWhitespace();
    bool exists = !IsEOF();
    RestoreState(oldState);
    return exists;
}

void CStringParser::Error(const std::string& msg) const {
    std::string fullError = m_Filename + "(" + std::to_string(m_CurrentLine) + ") : error : " + msg;
    printf("%s\n", fullError.c_str());
}