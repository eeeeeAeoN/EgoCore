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
    // We are at the end if the cursor hits the end pointer or a null terminator
    return m_Cursor >= m_BufferEnd || *m_Cursor == '\0';
}

int CStringParser::GetCurrentLine() const {
    return m_CurrentLine;
}

bool CStringParser::IsInQuotes() const {
    return m_InQuotes;
}


void CStringParser::MoveStringPos(unsigned int n) {
    // Move forward 'n' times safely
    for (unsigned int i = 0; i < n && !IsEOF(); ++i) {

        // Toggle the InQuotes flag if we hit a quotation mark
        if (*m_Cursor == '"' || *m_Cursor == '\'') {
            m_InQuotes = !m_InQuotes;
        }

        // Keep track of line numbers automatically
        if (*m_Cursor == '\n') {
            m_CurrentLine++;
        }

        m_Cursor++; // Actually move the pointer
    }
}

// ---------------------------------------------------------
// Skipping Functions
// ---------------------------------------------------------

void CStringParser::SkipWhitespace() {
    while (!IsEOF()) {
        // 1. Skip standard whitespace
        if (std::isspace(static_cast<unsigned char>(*m_Cursor))) {
            MoveStringPos(1);
            continue;
        }

        // 2. Skip single-line comments (//)
        if (*m_Cursor == '/' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '/') {
            SkipPastNewline();
            continue;
        }

        // 3. Skip multi-line comments (/* ... */)
        if (*m_Cursor == '/' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '*') {
            MoveStringPos(2); // Step over the opening /*

            while (!IsEOF()) {
                if (*m_Cursor == '*' && m_Cursor + 1 < m_BufferEnd && *(m_Cursor + 1) == '/') {
                    MoveStringPos(2); // Step over the closing */
                    break;
                }
                MoveStringPos(1);
            }
            continue; // Loop back around to check if there is trailing whitespace
        }

        // If we get here, it's not whitespace and not a comment. We are looking at real code.
        break;
    }
}

unsigned int CStringParser::SkipUntilWhitespace() {
    const char* start = m_Cursor;

    // Keep moving forward until we hit a space or the end of the file
    while (!IsEOF() && !std::isspace(static_cast<unsigned char>(*m_Cursor))) {
        MoveStringPos(1);
    }

    // Return how many characters we skipped over
    return static_cast<unsigned int>(m_Cursor - start);
}

int CStringParser::SkipPastNewline() {
    const char* start = m_Cursor;

    while (!IsEOF()) {
        char c = *m_Cursor;

        // If we hit a newline (Linux \n or Windows \r)
        if (c == '\n' || c == '\r') {
            // Consume all consecutive newlines to completely clear the line break
            while (!IsEOF() && (*m_Cursor == '\n' || *m_Cursor == '\r')) {
                MoveStringPos(1);
            }
            break; // Stop after clearing the newlines
        }

        MoveStringPos(1);
    }

    return static_cast<int>(m_Cursor - start);
}

bool CStringParser::ReadNextItem(CParsedItem& item) {
    SkipWhitespace();

    if (IsEOF()) {
        item.Type = EParsedItemType::EndOfFile;
        return false;
    }

    char c = *m_Cursor;
    char nextC = (m_Cursor + 1 < m_BufferEnd) ? *(m_Cursor + 1) : '\0';

    // 1. Is it a Number? (Starts with a digit, or a '-' followed by a digit)
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '-' && std::isdigit(static_cast<unsigned char>(nextC)))) {
        return ReadNextNumber(item);
    }

    // 2. Is it an Identifier/String? (Starts with a letter or an underscore)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return ReadNextString(item);
    }

    // 3. If it's neither, it must be a Symbol (like '{', ';', '=')
    return ReadNextSymbol(item);
}

bool CStringParser::ReadNextString(CParsedItem& item) {
    if (IsEOF()) return false;

    const char* start = m_Cursor;

    // Keep moving forward as long as it's a letter, number, or underscore
    while (!IsEOF() && (std::isalnum(static_cast<unsigned char>(*m_Cursor)) || *m_Cursor == '_')) {
        MoveStringPos(1);
    }

    if (m_Cursor == start) return false; // Nothing was read

    item.Type = EParsedItemType::Identifier;
    item.StringValue = std::string(start, m_Cursor - start);
    return true;
}

bool CStringParser::ReadNextNumber(CParsedItem& item) {
    if (IsEOF()) return false;

    const char* start = m_Cursor;
    bool hasDecimal = false;
    bool isHex = false;

    // Skip past the negative sign if it exists
    if (*m_Cursor == '-') {
        MoveStringPos(1);
    }

    // Check for Hexadecimal prefix (0x or 0X)
    if (*m_Cursor == '0' && m_Cursor + 1 < m_BufferEnd &&
        (*(m_Cursor + 1) == 'x' || *(m_Cursor + 1) == 'X')) {
        isHex = true;
        MoveStringPos(2); // Step over '0x'
    }

    // Keep reading digits based on base type
    while (!IsEOF()) {
        char c = *m_Cursor;

        if (isHex) {
            // If it's hex, accept 0-9, a-f, A-F
            if (std::isxdigit(static_cast<unsigned char>(c))) {
                MoveStringPos(1);
            }
            else {
                break; // Stop at the first non-hex character
            }
        }
        else {
            // Standard Base-10 parsing
            if (std::isdigit(static_cast<unsigned char>(c))) {
                MoveStringPos(1);
            }
            else if (c == '.' && !hasDecimal) {
                hasDecimal = true;
                MoveStringPos(1);
            }
            else {
                break; // Not a number character, stop reading
            }
        }
    }

    std::string numStr(start, m_Cursor - start);

    if (hasDecimal && !isHex) { // Hex values won't have decimals
        item.Type = EParsedItemType::Float;
        item.FloatValue = std::stof(numStr);
        item.StringValue = numStr;
    }
    else {
        item.Type = EParsedItemType::Integer;
        // std::stoi with base 0 automatically parses "0x..." strings properly!
        item.IntValue = std::stoi(numStr, nullptr, 0);
        item.StringValue = numStr;
    }

    return true;
}

bool CStringParser::ReadLineAsString(std::string& outStr) {
    if (IsEOF()) return false;

    const char* start = m_Cursor;

    // Move forward until we hit EOF or a newline
    while (!IsEOF() && *m_Cursor != '\n' && *m_Cursor != '\r') {
        MoveStringPos(1);
    }

    outStr = std::string(start, m_Cursor - start);
    SkipPastNewline(); // Step over the actual line breaks

    return true;
}

bool CStringParser::ReadNextSymbol(CParsedItem& item) {
    if (IsEOF()) return false;

    item.Type = EParsedItemType::Symbol;
    item.StringValue = std::string(1, *m_Cursor); // Grab exactly 1 character

    MoveStringPos(1);
    return true;
}

bool CStringParser::ReadNextItemAsQuotedString(std::string& outStr) {
    SkipWhitespace();
    if (IsEOF()) return false;

    char quoteChar = *m_Cursor;

    // Check if we are actually starting at a quote
    if (quoteChar != '"' && quoteChar != '\'') return false;

    MoveStringPos(1); // Step over the opening quote
    outStr.clear();

    while (!IsEOF()) {
        char c = *m_Cursor;

        if (c == '\\') {
            // It's an escape character, skip the backslash and read the next literal char
            MoveStringPos(1);
            if (!IsEOF()) {
                outStr += *m_Cursor;
                MoveStringPos(1);
            }
        }
        else if (c == quoteChar) {
            // We found the closing quote! Step over it and finish.
            MoveStringPos(1);
            return true;
        }
        else {
            // Standard character, just add it to the string
            outStr += c;
            MoveStringPos(1);
        }
    }

    return false; // Error: Reached the end of the file without finding a closing quote
}

void CStringParser::SetStringPos(unsigned int n) {
    if (n > (m_BufferEnd - m_BufferStart)) return; // Safety check

    m_Cursor = m_BufferStart + n;

    // The engine recalculates the InQuotes state from the beginning up to 'n'
    m_InQuotes = false;
    for (const char* ptr = m_BufferStart; ptr < m_Cursor; ++ptr) {
        if (*ptr == '"' || *ptr == '\'') {
            m_InQuotes = !m_InQuotes;
        }
    }
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

// ---------------------------------------------------------
// Advanced Skipping
// ---------------------------------------------------------

bool CStringParser::SkipUntilString(const std::string& str) {
    if (str.empty()) return false;

    while (!IsEOF()) {
        // If the remaining buffer matches the target string
        if (std::strncmp(m_Cursor, str.c_str(), str.length()) == 0) {
            return true; // Success: Cursor is now exactly AT the start of the string
        }
        MoveStringPos(1);
    }
    return false;
}

bool CStringParser::SkipUntilWholeString(const std::string& str) {
    if (str.empty()) return false;

    while (SkipUntilString(str)) {
        // We found the string, but is it a WHOLE word? (e.g., don't match "TRUE" inside "TRUE_VALUE")

        // Check character before the match
        bool leftOk = (m_Cursor == m_BufferStart) ||
            (!std::isalnum(*(m_Cursor - 1)) && *(m_Cursor - 1) != '_');

        // Check character after the match
        const char* right = m_Cursor + str.length();
        bool rightOk = (right >= m_BufferEnd) ||
            (!std::isalnum(*right) && *right != '_');

        if (leftOk && rightOk) {
            return true; // It is a standalone word!
        }

        // It was a partial match. Step forward one char and keep looking.
        MoveStringPos(1);
    }
    return false;
}

bool CStringParser::SkipPastString(const std::string& str) {
    if (SkipUntilString(str)) {
        MoveStringPos(str.length()); // Found it, now jump over it
        return true;
    }
    return false;
}

bool CStringParser::SkipPastWholeString(const std::string& str) {
    if (SkipUntilWholeString(str)) {
        MoveStringPos(str.length()); // Found the whole word, now jump over it
        return true;
    }
    return false;
}
// ---------------------------------------------------------
// Advanced Seeking & Extracting
// ---------------------------------------------------------

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
        MoveStringPos(target.length()); // Move past the target
        outStr = std::string(start, m_Cursor - start); // Extract everything including the target
        return true;
    }
    return false;
}

// ---------------------------------------------------------
// Strict Typed Readers
// ---------------------------------------------------------

bool CStringParser::ReadAsString(std::string& outStr) {
    CParsedItem item;

    // Attempt to read the next item, and ensure it's specifically an Identifier/String
    if (ReadNextItem(item) && item.Type == EParsedItemType::Identifier) {
        outStr = item.StringValue;
        return true;
    }

    Error("Error parsing string");
    return false;
}

int CStringParser::ReadAsInteger() {
    CParsedItem item;

    if (ReadNextItem(item) && item.Type == EParsedItemType::Integer) {
        return item.IntValue;
    }

    Error("Error parsing integer");
    return 0;
}

float CStringParser::ReadAsFloat() {
    CParsedItem item;

    if (ReadNextItem(item)) {
        // If it's explicitly a float (has a decimal), return it
        if (item.Type == EParsedItemType::Float) {
            return item.FloatValue;
        }
        // If it happens to be an integer (e.g., they typed '5' instead of '5.0'), gracefully cast it
        if (item.Type == EParsedItemType::Integer) {
            return static_cast<float>(item.IntValue);
        }
    }

    Error("Error parsing float");
    return 0.0f;
}

// ---------------------------------------------------------
// State Saving & Peeking
// ---------------------------------------------------------

CStringParser::CursorState CStringParser::SaveState() const {
    return { m_Cursor, m_CurrentLine, m_InQuotes };
}

void CStringParser::RestoreState(const CursorState& state) {
    m_Cursor = state.Ptr;
    m_CurrentLine = state.Line;
    m_InQuotes = state.InQuotes;
}

bool CStringParser::PeekNextItem(CParsedItem& item) {
    CursorState oldState = SaveState();
    bool result = ReadNextItem(item);
    RestoreState(oldState);
    return result;
}

EParsedItemType CStringParser::PeekNextItemType() {
    CParsedItem item;
    if (PeekNextItem(item)) {
        return item.Type;
    }
    return EParsedItemType::Unknown;
}

bool CStringParser::NextItemExists() {
    CursorState oldState = SaveState();
    SkipWhitespace();
    bool exists = !IsEOF();
    RestoreState(oldState);
    return exists;
}

// ---------------------------------------------------------
// Error Reporting
// ---------------------------------------------------------

void CStringParser::Error(const std::string& msg) const {
    // Formats exactly like Lionhead's: "filename.def(42) : error : message"
    std::string fullError = m_Filename + "(" + std::to_string(m_CurrentLine) + ") : error : " + msg;

    // TODO: Route this to your FableCompiler::LogToFile! 
    // For now, we print to standard out:
    printf("%s\n", fullError.c_str());
}