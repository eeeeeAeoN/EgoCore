// Parser for the unified def/header grammar.
#pragma once

#include <string_view>
#include <variant>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/diagnostic.hpp"

namespace fable::defs::compiler {

using ParseResult = std::variant<SourceAst, ParseError>;

ParseResult parse_source(std::string_view input, FileId file);

/// Parse one expression from text (used by tests and small tools).
std::variant<Expr, ParseError> parse_expr_text(std::string_view input);

} // namespace fable::defs::compiler
