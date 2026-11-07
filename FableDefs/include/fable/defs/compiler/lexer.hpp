// Tokenizer for the def and header grammars.
#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "fable/defs/compiler/diagnostic.hpp"

namespace fable::defs::compiler {

enum class TokenKind {
    Ident,
    Number,
    Str,
    Definition,
    DefinitionTemplate,
    EndDefinition,
    Define,
    Ifdef,
    Ifndef,
    Else,
    Endif,
    Pragma,
    Namespace,
    Enum,
    Dot,
    LBracket,
    RBracket,
    LParen,
    RParen,
    LBrace,
    RBrace,
    Lt,
    Gt,
    Backslash,
    Pipe,
    Plus,
    Shl,
    Eq,
    Comma,
    Semi,
    Eof,
};

const char *describe(TokenKind kind);

struct Token {
    TokenKind kind = TokenKind::Eof;
    Span span;
    std::string_view text;
};

struct LexError {
    Span span;
    std::string message;
};

using LexResult = std::variant<std::vector<Token>, LexError>;

LexResult lex(std::string_view input, FileId file);

} // namespace fable::defs::compiler
