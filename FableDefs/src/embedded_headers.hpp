#pragma once

#include <cstddef>
#include <string_view>

namespace fable::defs::compiler {

struct EmbeddedHeader {
    std::string_view name;
    std::string_view content;
};

const EmbeddedHeader *embedded_non_retail_headers(std::size_t &count);

} // namespace fable::defs::compiler
