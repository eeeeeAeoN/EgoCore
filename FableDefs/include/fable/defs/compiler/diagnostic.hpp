// Diagnostics and source locations for the C++ def compiler.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace fable::defs::compiler {

struct FileId {
    std::uint32_t value = 0;

    friend bool operator==(FileId a, FileId b) { return a.value == b.value; }
    friend bool operator!=(FileId a, FileId b) { return !(a == b); }

    static constexpr FileId anonymous() { return FileId{0xFFFFFFFFu}; }
};

struct Span {
    FileId file;
    std::uint32_t start = 0;
    std::uint32_t end = 0;

    static constexpr Span new_span(FileId file, std::uint32_t start, std::uint32_t end) {
        return Span{file, start, end};
    }
};

template <class T>
struct Spanned {
    T value;
    Span span;
};

struct ParseError {
    Span span;
    std::string message;
};

/// Render a parse/symbol error as `path:line:col: message`.
std::string render_error(const ParseError &error, std::string_view input,
                         std::string_view path);

} // namespace fable::defs::compiler
