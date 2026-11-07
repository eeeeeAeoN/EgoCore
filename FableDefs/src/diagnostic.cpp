#include "fable/defs/compiler/diagnostic.hpp"

#include <cstddef>
#include <string>

namespace fable::defs::compiler {

std::string render_error(const ParseError &error, std::string_view input,
                         std::string_view path) {
    std::size_t limit = error.span.start;
    if (limit > input.size()) {
        limit = input.size();
    }
    std::size_t line = 1;
    std::size_t column = 1;
    for (std::size_t i = 0; i < limit; ++i) {
        if (input[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    std::string out;
    out.reserve(path.size() + error.message.size() + 32);
    out.append(path);
    out.push_back(':');
    out.append(std::to_string(line));
    out.push_back(':');
    out.append(std::to_string(column));
    out.append(": ");
    out.append(error.message);
    return out;
}

} // namespace fable::defs::compiler
