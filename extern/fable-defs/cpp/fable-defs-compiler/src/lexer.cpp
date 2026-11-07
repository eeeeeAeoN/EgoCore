#include "fable/defs/compiler/lexer.hpp"

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>

namespace fable::defs::compiler {

const char *describe(TokenKind kind) {
    switch (kind) {
    case TokenKind::Ident: return "identifier";
    case TokenKind::Number: return "number";
    case TokenKind::Str: return "string";
    case TokenKind::Definition: return "#definition";
    case TokenKind::DefinitionTemplate: return "#definition_template";
    case TokenKind::EndDefinition: return "#end_definition";
    case TokenKind::Define: return "#define";
    case TokenKind::Ifdef: return "#ifdef";
    case TokenKind::Ifndef: return "#ifndef";
    case TokenKind::Else: return "#else";
    case TokenKind::Endif: return "#endif";
    case TokenKind::Pragma: return "#pragma";
    case TokenKind::Namespace: return "namespace";
    case TokenKind::Enum: return "enum";
    case TokenKind::Dot: return ".";
    case TokenKind::LBracket: return "[";
    case TokenKind::RBracket: return "]";
    case TokenKind::LParen: return "(";
    case TokenKind::RParen: return ")";
    case TokenKind::LBrace: return "{";
    case TokenKind::RBrace: return "}";
    case TokenKind::Lt: return "<";
    case TokenKind::Gt: return ">";
    case TokenKind::Backslash: return "\\";
    case TokenKind::Pipe: return "|";
    case TokenKind::Plus: return "+";
    case TokenKind::Shl: return "<<";
    case TokenKind::Eq: return "=";
    case TokenKind::Comma: return ",";
    case TokenKind::Semi: return ";";
    case TokenKind::Eof: return "end of input";
    }
    return "token";
}

namespace {

using TokenResult = std::variant<Token, LexError>;

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

class Lexer {
public:
    Lexer(std::string_view input, FileId file) : input_(input), file_(file) {}

    LexResult tokenize() {
        std::vector<Token> tokens;
        for (;;) {
            if (auto error = skip_trivia()) {
                return *error;
            }
            if (pos_ >= input_.size()) {
                tokens.push_back(token(TokenKind::Eof, pos_, pos_));
                return tokens;
            }
            auto next = next_token();
            if (auto *error = std::get_if<LexError>(&next)) {
                return *error;
            }
            tokens.push_back(std::get<Token>(next));
        }
    }

private:
    std::string_view rest() const { return input_.substr(pos_); }

    char peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }

    char peek_at(std::size_t offset) const {
        return pos_ + offset < input_.size() ? input_[pos_ + offset] : '\0';
    }

    Token token(TokenKind kind, std::size_t start, std::size_t end) const {
        return Token{kind, Span::new_span(file_, static_cast<std::uint32_t>(start),
                                          static_cast<std::uint32_t>(end)),
                     input_.substr(start, end - start)};
    }

    LexError error(std::size_t start, std::size_t end, std::string message) const {
        return LexError{Span::new_span(file_, static_cast<std::uint32_t>(start),
                                       static_cast<std::uint32_t>(end)),
                        std::move(message)};
    }

    void skip_to_end_of_line() {
        while (pos_ < input_.size()) {
            char c = input_[pos_++];
            if (c == '\n') {
                break;
            }
        }
    }

    bool line_is_decorative() const {
        for (std::size_t i = pos_; i < input_.size(); ++i) {
            char c = input_[i];
            if (c == '\n') {
                return true;
            }
            if (c != '*' && c != '/' && c != ' ' && c != '\t' && c != '\r') {
                return false;
            }
        }
        return true;
    }

    std::optional<LexError> skip_trivia() {
        for (;;) {
            while (pos_ < input_.size()) {
                char c = input_[pos_];
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' ||
                    c == '\f') {
                    ++pos_;
                } else {
                    break;
                }
            }
            if (starts_with(rest(), "//")) {
                skip_to_end_of_line();
                continue;
            }
            if (starts_with(rest(), "/*")) {
                std::size_t close = input_.find("*/", pos_ + 2);
                if (close != std::string_view::npos) {
                    pos_ = close + 2;
                    continue;
                }
                if (line_is_decorative()) {
                    skip_to_end_of_line();
                    continue;
                }
                return error(pos_, pos_ + 2, "unterminated block comment");
            }
            if ((peek() == '*' || peek() == '/') && line_is_decorative()) {
                skip_to_end_of_line();
                continue;
            }
            return std::nullopt;
        }
    }

    TokenResult next_token() {
        char c = peek();
        if (c == '"') {
            return lex_string();
        }
        if (c == '#') {
            return lex_directive();
        }
        if (c >= '0' && c <= '9') {
            return lex_number();
        }
        if (c == '-' && peek_at(1) >= '0' && peek_at(1) <= '9') {
            return lex_number();
        }
        if (c == '_' || std::isalpha(static_cast<unsigned char>(c))) {
            return lex_ident_or_keyword();
        }
        return lex_punct();
    }

    TokenResult lex_number() {
        std::size_t start = pos_;
        if (peek() == '-') {
            ++pos_;
        }
        while (peek() >= '0' && peek() <= '9') {
            ++pos_;
        }
        if (peek() == '.') {
            ++pos_;
            while (peek() >= '0' && peek() <= '9') {
                ++pos_;
            }
        }
        if (peek() == 'f') {
            ++pos_;
        }
        return token(TokenKind::Number, start, pos_);
    }

    TokenResult lex_string() {
        std::size_t start = pos_;
        ++pos_;
        while (pos_ < input_.size()) {
            char c = input_[pos_++];
            if (c == '"') {
                return token(TokenKind::Str, start, pos_);
            }
        }
        return error(start, start + 1, "unterminated string");
    }

    TokenResult lex_ident_or_keyword() {
        std::size_t start = pos_;
        ++pos_;
        while (peek() == '_' || std::isalnum(static_cast<unsigned char>(peek()))) {
            ++pos_;
        }
        std::string_view text = input_.substr(start, pos_ - start);
        TokenKind kind = TokenKind::Ident;
        if (text == "namespace") {
            kind = TokenKind::Namespace;
        } else if (text == "enum") {
            kind = TokenKind::Enum;
        }
        return token(kind, start, pos_);
    }

    TokenResult lex_directive() {
        std::size_t start = pos_;
        std::size_t end = start + 1;
        while (end < input_.size() &&
               (input_[end] == '_' || std::isalnum(static_cast<unsigned char>(input_[end])))) {
            ++end;
        }
        std::string_view text = input_.substr(start, end - start);
        TokenKind kind;
        if (text == "#definition_template") {
            kind = TokenKind::DefinitionTemplate;
        } else if (text == "#definition") {
            kind = TokenKind::Definition;
        } else if (text == "#end_definition") {
            kind = TokenKind::EndDefinition;
        } else if (text == "#define") {
            kind = TokenKind::Define;
        } else if (text == "#ifdef") {
            kind = TokenKind::Ifdef;
        } else if (text == "#ifndef") {
            kind = TokenKind::Ifndef;
        } else if (text == "#else") {
            kind = TokenKind::Else;
        } else if (text == "#endif") {
            kind = TokenKind::Endif;
        } else if (text == "#pragma") {
            kind = TokenKind::Pragma;
        } else {
            return error(start, start + 1, "unexpected character '#'");
        }
        pos_ = end;
        return token(kind, start, end);
    }

    TokenResult lex_punct() {
        std::size_t start = pos_;
        char c = peek();
        if (c == '<') {
            if (peek_at(1) == '<') {
                pos_ += 2;
                return token(TokenKind::Shl, start, pos_);
            }
            ++pos_;
            return token(TokenKind::Lt, start, pos_);
        }
        TokenKind kind;
        switch (c) {
        case '.': kind = TokenKind::Dot; break;
        case '[': kind = TokenKind::LBracket; break;
        case ']': kind = TokenKind::RBracket; break;
        case '(': kind = TokenKind::LParen; break;
        case ')': kind = TokenKind::RParen; break;
        case '{': kind = TokenKind::LBrace; break;
        case '}': kind = TokenKind::RBrace; break;
        case '>': kind = TokenKind::Gt; break;
        case '\\': kind = TokenKind::Backslash; break;
        case '|': kind = TokenKind::Pipe; break;
        case '+': kind = TokenKind::Plus; break;
        case '=': kind = TokenKind::Eq; break;
        case ',': kind = TokenKind::Comma; break;
        case ';': kind = TokenKind::Semi; break;
        default:
            return error(start, start + 1,
                         std::string("unexpected character '") + c + "'");
        }
        ++pos_;
        return token(kind, start, pos_);
    }

    std::string_view input_;
    FileId file_;
    std::size_t pos_ = 0;
};

} // namespace

LexResult lex(std::string_view input, FileId file) {
    return Lexer(input, file).tokenize();
}

} // namespace fable::defs::compiler
