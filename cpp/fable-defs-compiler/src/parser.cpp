#include "fable/defs/compiler/parser.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fable/defs/compiler/lexer.hpp"

namespace fable::defs::compiler {

const char *expr_kind_name(Expr::Kind kind) {
    switch (kind) {
    case Expr::Kind::Number: return "number";
    case Expr::Kind::Bool: return "boolean";
    case Expr::Kind::String: return "string";
    case Expr::Kind::Symbol: return "symbol";
    case Expr::Kind::Constructor: return "constructor";
    case Expr::Kind::BitOr: return "bit-or";
    case Expr::Kind::Add: return "add";
    }
    return "expression";
}

std::vector<const Spanned<Definition> *> SourceAst::definitions() const {
    std::vector<const Spanned<Definition> *> out;
    for (const auto &item : items) {
        if (item.kind == Item::Kind::Definition) {
            out.push_back(&item.definition);
        }
    }
    return out;
}

bool starts_declaration(Item::Kind kind) {
    return kind == Item::Kind::Enum || kind == Item::Kind::Define ||
           kind == Item::Kind::Namespace || kind == Item::Kind::Conditional ||
           kind == Item::Kind::PragmaOnce;
}

namespace {

bool starts_declaration(TokenKind kind) {
    return kind == TokenKind::Enum || kind == TokenKind::Define ||
           kind == TokenKind::Namespace || kind == TokenKind::Ifdef ||
           kind == TokenKind::Ifndef || kind == TokenKind::Pragma;
}

bool is_body_terminator(TokenKind kind) {
    return kind == TokenKind::Definition || kind == TokenKind::DefinitionTemplate ||
           kind == TokenKind::Define || kind == TokenKind::Ifdef ||
           kind == TokenKind::Ifndef || kind == TokenKind::Else ||
           kind == TokenKind::Endif || kind == TokenKind::Pragma ||
           kind == TokenKind::Namespace || kind == TokenKind::Enum ||
           kind == TokenKind::Eof;
}

class Cursor {
public:
    Cursor(std::vector<Token> tokens, FileId file)
        : tokens_(std::move(tokens)), file_(file) {}

    Token peek() const { return tokens_[pos_]; }

    Token peek_at(std::size_t n) const {
        std::size_t index = pos_ + n;
        return tokens_[index < tokens_.size() ? index : tokens_.size() - 1];
    }

    Token bump() {
        Token tok = tokens_[pos_];
        if (tok.kind != TokenKind::Eof) {
            ++pos_;
        }
        return tok;
    }

    bool at(TokenKind kind) const { return peek().kind == kind; }

    bool at_ident(std::string_view name) const {
        const Token tok = peek();
        return tok.kind == TokenKind::Ident && tok.text == name;
    }

    std::uint32_t prev_end() const {
        return pos_ == 0 ? 0 : tokens_[pos_ - 1].span.end;
    }

    Span prev_span() const {
        return pos_ == 0 ? Span{} : tokens_[pos_ - 1].span;
    }

    FileId file() const { return file_; }

    ParseError err(std::string message) const {
        return ParseError{peek().span, std::move(message)};
    }

    ParseError err_at(Span span, std::string message) const {
        return ParseError{span, std::move(message)};
    }

    ParseError unexpected(std::string_view expected) const {
        return err("expected " + std::string(expected) + ", found " +
                   describe(peek().kind));
    }

    std::optional<Token> expect(TokenKind kind) {
        if (at(kind)) {
            return bump();
        }
        return std::nullopt;
    }

    std::optional<Spanned<std::string>> expect_ident_spanned() {
        Token tok = peek();
        if (tok.kind == TokenKind::Ident) {
            bump();
            return Spanned<std::string>{std::string(tok.text), tok.span};
        }
        return std::nullopt;
    }

private:
    std::vector<Token> tokens_;
    FileId file_;
    std::size_t pos_ = 0;
};

ParseResult parse_file(Cursor &cursor);

std::optional<ParseError> parse_definition(Cursor &cursor, Spanned<Definition> &out);
std::optional<ParseError> parse_statement(Cursor &cursor, Statement &out);
std::optional<ParseError> parse_tagged_block(Cursor &cursor, TaggedBlock &out);
std::optional<ParseError> parse_property_path(Cursor &cursor, PropertyPath &out);
std::optional<ParseError> parse_expr(Cursor &cursor, Expr &out);
std::optional<ParseError> parse_add_expr(Cursor &cursor, Expr &out);
std::optional<ParseError> parse_leaf_expr(Cursor &cursor, Expr &out);
std::optional<ParseError> parse_call_with_name(Cursor &cursor, std::string_view name,
                                               std::vector<Expr> &arguments);
std::optional<ParseError> parse_declaration(Cursor &cursor, Item &out);
std::optional<ParseError> parse_enum_expr(Cursor &cursor, EnumExpr &out);
std::optional<ParseError> parse_enum_shift(Cursor &cursor, EnumExpr &out);
std::optional<ParseError> parse_enum_leaf(Cursor &cursor, EnumExpr &out);

std::optional<ParseError> parse_definition(Cursor &cursor, Spanned<Definition> &out) {
    Token header = cursor.peek();
    std::uint32_t def_start = header.span.start;
    bool is_template = false;
    if (header.kind == TokenKind::DefinitionTemplate) {
        cursor.bump();
        is_template = true;
    } else if (header.kind == TokenKind::Definition) {
        cursor.bump();
    } else {
        return cursor.unexpected("#definition or #definition_template");
    }

    auto def_type = cursor.expect_ident_spanned();
    if (!def_type) {
        return cursor.unexpected("definition type");
    }
    auto name = cursor.expect_ident_spanned();
    if (!name) {
        return cursor.unexpected("definition name");
    }

    std::optional<std::string> specializes;
    Span specializes_span;
    if (cursor.at_ident("specialises")) {
        Token keyword = cursor.bump();
        Token parent_tok = cursor.peek();
        auto parent = cursor.expect_ident_spanned();
        if (!parent) {
            return cursor.unexpected("specialised parent");
        }
        specializes = parent->value;
        specializes_span = Span::new_span(
            cursor.file(), keyword.span.start,
            static_cast<std::uint32_t>(parent_tok.span.start + parent->value.size()));
    }

    std::vector<Statement> body;
    std::uint32_t def_end = 0;
    for (;;) {
        TokenKind kind = cursor.peek().kind;
        if (kind == TokenKind::EndDefinition) {
            def_end = cursor.bump().span.end;
            if (cursor.at(TokenKind::Semi)) {
                def_end = cursor.bump().span.end;
            }
            break;
        }
        if (is_body_terminator(kind)) {
            return cursor.err("missing #end_definition");
        }
        Statement stmt;
        if (auto error = parse_statement(cursor, stmt)) {
            return error;
        }
        body.push_back(std::move(stmt));
    }

    out.value.is_template = is_template;
    out.value.def_type = std::move(def_type->value);
    out.value.name = std::move(name->value);
    out.value.specializes = std::move(specializes);
    out.value.specializes_span = specializes_span;
    out.value.body = std::move(body);
    out.span = Span::new_span(cursor.file(), def_start, def_end);
    out.value.span = out.span;
    return std::nullopt;
}

std::optional<ParseError> parse_statement(Cursor &cursor, Statement &out) {
    std::uint32_t start = cursor.peek().span.start;

    if (cursor.at(TokenKind::Lt) && cursor.peek_at(1).kind != TokenKind::Backslash) {
        TaggedBlock block;
        if (auto error = parse_tagged_block(cursor, block)) {
            return error;
        }
        out.kind = Statement::Kind::TaggedBlock;
        out.tagged_block = std::make_shared<TaggedBlock>(std::move(block));
        out.span = Span::new_span(cursor.file(), start, cursor.prev_end());
        return std::nullopt;
    }

    PropertyPath path;
    if (auto error = parse_property_path(cursor, path)) {
        return error;
    }

    if (cursor.at(TokenKind::LParen)) {
        if (path.segments.empty() ||
            path.segments.back().kind != PathSegment::Kind::Field) {
            return cursor.err("method call has no method name: the path ends in an index");
        }
        std::string method = std::move(path.segments.back().field);
        path.segments.pop_back();
        std::vector<Expr> arguments;
        if (auto error = parse_call_with_name(cursor, method, arguments)) {
            return error;
        }
        if (cursor.at(TokenKind::Semi)) {
            cursor.bump();
        }
        out.kind = Statement::Kind::MethodCall;
        out.method_call.object = std::move(path);
        out.method_call.method = std::move(method);
        out.method_call.arguments = std::move(arguments);
        out.span = Span::new_span(cursor.file(), start, cursor.prev_end());
        return std::nullopt;
    }

    Expr expr;
    if (auto error = parse_expr(cursor, expr)) {
        return error;
    }
    if (cursor.at(TokenKind::Semi)) {
        cursor.bump();
    }
    out.kind = Statement::Kind::Field;
    out.field.path = std::move(path);
    out.field.expr = std::move(expr);
    out.span = Span::new_span(cursor.file(), start, cursor.prev_end());
    return std::nullopt;
}

std::optional<ParseError> parse_tagged_block(Cursor &cursor, TaggedBlock &out) {
    if (!cursor.expect(TokenKind::Lt)) {
        return cursor.unexpected("<");
    }
    auto tag = cursor.expect_ident_spanned();
    if (!tag) {
        return cursor.unexpected("tag name");
    }
    if (!cursor.expect(TokenKind::Gt)) {
        return cursor.unexpected(">");
    }
    std::vector<Statement> body;
    for (;;) {
        TokenKind kind = cursor.peek().kind;
        if (kind == TokenKind::Lt && cursor.peek_at(1).kind == TokenKind::Backslash) {
            std::uint32_t close_start = cursor.peek().span.start;
            cursor.bump(); // <
            cursor.bump(); // backslash
            auto close_tag = cursor.expect_ident_spanned();
            if (!close_tag) {
                return cursor.unexpected("closing tag name");
            }
            if (!cursor.expect(TokenKind::Gt)) {
                return cursor.unexpected(">");
            }
            if (close_tag->value != tag->value) {
                return cursor.err_at(
                    Span::new_span(cursor.file(), close_start, cursor.prev_end()),
                    "mismatched tag: opened <" + tag->value + ">, closed <\\" +
                        close_tag->value + ">");
            }
            break;
        }
        if (is_body_terminator(kind) || kind == TokenKind::EndDefinition) {
            return cursor.unexpected("<\\" + tag->value + ">");
        }
        Statement stmt;
        if (auto error = parse_statement(cursor, stmt)) {
            return error;
        }
        body.push_back(std::move(stmt));
    }
    out.tag = std::move(tag->value);
    out.body = std::move(body);
    return std::nullopt;
}

std::optional<ParseError> parse_property_path(Cursor &cursor, PropertyPath &out) {
    auto first = cursor.expect_ident_spanned();
    if (!first) {
        return cursor.unexpected("field name");
    }
    PathSegment segment;
    segment.kind = PathSegment::Kind::Field;
    segment.field = std::move(first->value);
    segment.span = first->span;
    out.segments.push_back(std::move(segment));

    for (;;) {
        if (cursor.at(TokenKind::Dot)) {
            cursor.bump();
            auto name = cursor.expect_ident_spanned();
            if (!name) {
                return cursor.unexpected("field name");
            }
            PathSegment next;
            next.kind = PathSegment::Kind::Field;
            next.field = std::move(name->value);
            next.span = name->span;
            out.segments.push_back(std::move(next));
        } else if (cursor.at(TokenKind::LBracket)) {
            Token open = cursor.bump();
            Expr index;
            if (auto error = parse_expr(cursor, index)) {
                return error;
            }
            if (!cursor.expect(TokenKind::RBracket)) {
                return cursor.unexpected("]");
            }
            PathSegment next;
            next.kind = PathSegment::Kind::Index;
            next.index = std::move(index);
            next.span = Span::new_span(cursor.file(), open.span.start, cursor.prev_end());
            out.segments.push_back(std::move(next));
        } else {
            break;
        }
    }
    return std::nullopt;
}

std::optional<ParseError> parse_expr(Cursor &cursor, Expr &out) {
    std::uint32_t start = cursor.peek().span.start;
    Expr first;
    if (auto error = parse_add_expr(cursor, first)) {
        return error;
    }
    std::vector<Expr> terms;
    terms.push_back(std::move(first));
    while (cursor.at(TokenKind::Pipe)) {
        cursor.bump();
        Expr next;
        if (auto error = parse_add_expr(cursor, next)) {
            return error;
        }
        terms.push_back(std::move(next));
    }
    if (terms.size() == 1) {
        out = std::move(terms.front());
        return std::nullopt;
    }
    out.kind = Expr::Kind::BitOr;
    out.terms = std::move(terms);
    out.span = Span::new_span(cursor.file(), start, cursor.prev_end());
    return std::nullopt;
}

std::optional<ParseError> parse_add_expr(Cursor &cursor, Expr &out) {
    std::uint32_t start = cursor.peek().span.start;
    Expr first;
    if (auto error = parse_leaf_expr(cursor, first)) {
        return error;
    }
    std::vector<Expr> terms;
    terms.push_back(std::move(first));
    while (cursor.at(TokenKind::Plus)) {
        cursor.bump();
        Expr next;
        if (auto error = parse_leaf_expr(cursor, next)) {
            return error;
        }
        terms.push_back(std::move(next));
    }
    if (terms.size() == 1) {
        out = std::move(terms.front());
        return std::nullopt;
    }
    out.kind = Expr::Kind::Add;
    out.terms = std::move(terms);
    out.span = Span::new_span(cursor.file(), start, cursor.prev_end());
    return std::nullopt;
}

std::optional<ParseError> parse_leaf_expr(Cursor &cursor, Expr &out) {
    Token tok = cursor.peek();
    switch (tok.kind) {
    case TokenKind::Str: {
        cursor.bump();
        std::string_view text = tok.text;
        out.kind = Expr::Kind::String;
        out.text = std::string(text.substr(1, text.size() >= 2 ? text.size() - 2 : 0));
        out.span = tok.span;
        return std::nullopt;
    }
    case TokenKind::Number:
        cursor.bump();
        out.kind = Expr::Kind::Number;
        out.text = std::string(tok.text);
        out.span = tok.span;
        return std::nullopt;
    case TokenKind::Ident: {
        cursor.bump();
        if (tok.text == "TRUE" || tok.text == "BTRUE") {
            out.kind = Expr::Kind::Bool;
            out.boolean = true;
            out.span = tok.span;
            return std::nullopt;
        }
        if (tok.text == "FALSE" || tok.text == "BFALSE") {
            out.kind = Expr::Kind::Bool;
            out.boolean = false;
            out.span = tok.span;
            return std::nullopt;
        }
        if (cursor.at(TokenKind::LParen)) {
            std::vector<Expr> arguments;
            if (auto error = parse_call_with_name(cursor, tok.text, arguments)) {
                return error;
            }
            out.kind = Expr::Kind::Constructor;
            out.text = std::string(tok.text);
            out.arguments = std::move(arguments);
            out.span = Span::new_span(cursor.file(), tok.span.start, cursor.prev_end());
            return std::nullopt;
        }
        out.kind = Expr::Kind::Symbol;
        out.text = std::string(tok.text);
        out.span = tok.span;
        return std::nullopt;
    }
    default:
        return cursor.unexpected("expression");
    }
}

std::optional<ParseError> parse_call_with_name(Cursor &cursor, std::string_view name,
                                               std::vector<Expr> &arguments) {
    (void)name;
    if (!cursor.expect(TokenKind::LParen)) {
        return cursor.unexpected("(");
    }
    if (!cursor.at(TokenKind::RParen)) {
        for (;;) {
            Expr arg;
            if (auto error = parse_expr(cursor, arg)) {
                return error;
            }
            arguments.push_back(std::move(arg));
            if (cursor.at(TokenKind::Comma)) {
                cursor.bump();
            } else {
                break;
            }
        }
    }
    if (!cursor.expect(TokenKind::RParen)) {
        return cursor.unexpected(")");
    }
    return std::nullopt;
}
std::optional<ParseError> parse_enum_body(Cursor &cursor, EnumDecl &out) {
    if (cursor.at(TokenKind::Ident)) {
        out.name = cursor.expect_ident_spanned();
    }
    if (!cursor.expect(TokenKind::LBrace)) {
        return cursor.unexpected("{");
    }
    for (;;) {
        if (cursor.at(TokenKind::Eof)) {
            return cursor.err("unterminated enum");
        }
        if (cursor.at(TokenKind::RBrace)) {
            break;
        }
        EnumVariant variant;
        auto name = cursor.expect_ident_spanned();
        if (!name) {
            return cursor.unexpected("identifier");
        }
        variant.name = std::move(*name);
        if (cursor.at(TokenKind::Eq)) {
            cursor.bump();
            EnumExpr value;
            if (auto error = parse_enum_expr(cursor, value)) {
                return error;
            }
            variant.value = std::move(value);
        }
        bool has_value = variant.value.has_value();
        out.variants.push_back(std::move(variant));
        if (cursor.at(TokenKind::Comma)) {
            cursor.bump();
        } else if (cursor.at(TokenKind::RBrace)) {
            break;
        } else {
            return cursor.unexpected(has_value ? "`,` or `}`" : "`=`, `,` or `}`");
        }
    }
    if (!cursor.expect(TokenKind::RBrace)) {
        return cursor.unexpected("}");
    }
    if (cursor.at(TokenKind::Semi)) {
        cursor.bump();
    }
    return std::nullopt;
}

std::optional<ParseError> parse_enum_expr(Cursor &cursor, EnumExpr &out) {
    EnumExpr first;
    if (auto error = parse_enum_shift(cursor, first)) {
        return error;
    }
    std::vector<EnumExpr> terms;
    terms.push_back(std::move(first));
    while (cursor.at(TokenKind::Pipe)) {
        cursor.bump();
        EnumExpr next;
        if (auto error = parse_enum_shift(cursor, next)) {
            return error;
        }
        terms.push_back(std::move(next));
    }
    if (terms.size() == 1) {
        out = std::move(terms.front());
    } else {
        out.kind = EnumExpr::Kind::BitOr;
        out.terms = std::move(terms);
    }
    return std::nullopt;
}

std::optional<ParseError> parse_enum_shift(Cursor &cursor, EnumExpr &out) {
    EnumExpr first;
    if (auto error = parse_enum_leaf(cursor, first)) {
        return error;
    }
    std::vector<EnumExpr> terms;
    terms.push_back(std::move(first));
    while (cursor.at(TokenKind::Shl)) {
        cursor.bump();
        EnumExpr next;
        if (auto error = parse_enum_leaf(cursor, next)) {
            return error;
        }
        terms.push_back(std::move(next));
    }
    if (terms.size() == 1) {
        out = std::move(terms.front());
    } else {
        out.kind = EnumExpr::Kind::Shift;
        out.terms = std::move(terms);
    }
    return std::nullopt;
}

std::optional<ParseError> parse_enum_leaf(Cursor &cursor, EnumExpr &out) {
    Token tok = cursor.peek();
    if (tok.kind == TokenKind::Number) {
        cursor.bump();
        std::int64_t value = 0;
        try {
            value = std::stoll(std::string(tok.text));
        } catch (...) {
            return cursor.err_at(tok.span, "invalid number");
        }
        out.kind = EnumExpr::Kind::Int;
        out.integer = value;
        out.span = tok.span;
        return std::nullopt;
    }
    if (tok.kind == TokenKind::Ident) {
        cursor.bump();
        out.kind = EnumExpr::Kind::Ident;
        out.ident = std::string(tok.text);
        out.span = tok.span;
        return std::nullopt;
    }
    return cursor.unexpected("number or identifier");
}

std::optional<ParseError> parse_define_body(Cursor &cursor, Define &out) {
    auto name = cursor.expect_ident_spanned();
    if (!name) {
        return cursor.unexpected("identifier");
    }
    out.name = std::move(*name);
    if (!cursor.at(TokenKind::Number)) {
        out.value = std::nullopt;
        return std::nullopt;
    }
    Token tok = cursor.bump();
    try {
        out.value = std::stoll(std::string(tok.text));
    } catch (...) {
        return cursor.err_at(tok.span, "invalid number");
    }
    return std::nullopt;
}

std::optional<ParseError> parse_namespace_body(Cursor &cursor, Namespace &out) {
    auto name = cursor.expect_ident_spanned();
    if (!name) {
        return cursor.unexpected("identifier");
    }
    out.name = std::move(name->value);
    if (!cursor.expect(TokenKind::LBrace)) {
        return cursor.unexpected("{");
    }
    for (;;) {
        if (cursor.at(TokenKind::Eof)) {
            return cursor.err("unterminated namespace");
        }
        if (cursor.at(TokenKind::RBrace)) {
            break;
        }
        Item item;
        if (auto error = parse_declaration(cursor, item)) {
            return error;
        }
        out.items.push_back(std::move(item));
    }
    if (!cursor.expect(TokenKind::RBrace)) {
        return cursor.unexpected("}");
    }
    if (cursor.at(TokenKind::Semi)) {
        cursor.bump();
    }
    return std::nullopt;
}

std::optional<ParseError> parse_if_def_body(Cursor &cursor, bool inverted, IfDef &out) {
    auto condition = cursor.expect_ident_spanned();
    if (!condition) {
        return cursor.unexpected("identifier");
    }
    out.condition = std::move(condition->value);
    out.inverted = inverted;

    for (;;) {
        if (cursor.at(TokenKind::Eof)) {
            return cursor.err("unterminated #ifdef");
        }
        if (cursor.at(TokenKind::Else) || cursor.at(TokenKind::Endif)) {
            break;
        }
        Item item;
        if (auto error = parse_declaration(cursor, item)) {
            return error;
        }
        out.if_branch.push_back(std::move(item));
    }
    if (cursor.at(TokenKind::Else)) {
        cursor.bump();
        std::vector<Item> else_branch;
        for (;;) {
            if (cursor.at(TokenKind::Eof)) {
                return cursor.err("unterminated #ifdef");
            }
            if (cursor.at(TokenKind::Endif)) {
                break;
            }
            Item item;
            if (auto error = parse_declaration(cursor, item)) {
                return error;
            }
            else_branch.push_back(std::move(item));
        }
        out.else_branch = std::move(else_branch);
    }
    if (!cursor.expect(TokenKind::Endif)) {
        return cursor.unexpected("#endif");
    }
    if (cursor.at(TokenKind::Ident)) {
        cursor.bump();
    }
    return std::nullopt;
}

std::optional<ParseError> parse_declaration(Cursor &cursor, Item &out) {
    Token tok = cursor.peek();
    switch (tok.kind) {
    case TokenKind::Enum: {
        cursor.bump();
        out.kind = Item::Kind::Enum;
        return parse_enum_body(cursor, out.enum_decl);
    }
    case TokenKind::Define: {
        cursor.bump();
        out.kind = Item::Kind::Define;
        return parse_define_body(cursor, out.define);
    }
    case TokenKind::Namespace: {
        cursor.bump();
        out.kind = Item::Kind::Namespace;
        auto ns = std::make_shared<Namespace>();
        if (auto error = parse_namespace_body(cursor, *ns)) {
            return error;
        }
        out.namespace_ = std::move(ns);
        return std::nullopt;
    }
    case TokenKind::Ifdef:
    case TokenKind::Ifndef: {
        bool inverted = tok.kind == TokenKind::Ifndef;
        cursor.bump();
        out.kind = Item::Kind::Conditional;
        auto conditional = std::make_shared<IfDef>();
        if (auto error = parse_if_def_body(cursor, inverted, *conditional)) {
            return error;
        }
        out.conditional = std::move(conditional);
        return std::nullopt;
    }
    case TokenKind::Pragma: {
        out.kind = Item::Kind::PragmaOnce;
        out.span = cursor.bump().span;
        if (cursor.at_ident("once")) {
            cursor.bump();
        }
        return std::nullopt;
    }
    default:
        return cursor.err("unknown item");
    }
}

ParseResult parse_file(Cursor &cursor) {
    SourceAst ast;
    for (;;) {
        TokenKind kind = cursor.peek().kind;
        if (kind == TokenKind::Eof) {
            break;
        }
        if (kind == TokenKind::Definition || kind == TokenKind::DefinitionTemplate) {
            Item item;
            item.kind = Item::Kind::Definition;
            if (auto error = parse_definition(cursor, item.definition)) {
                return *error;
            }
            ast.items.push_back(std::move(item));
        } else if (starts_declaration(kind)) {
            Item item;
            if (auto error = parse_declaration(cursor, item)) {
                return *error;
            }
            ast.items.push_back(std::move(item));
        } else {
            std::uint32_t start = cursor.peek().span.start;
            std::uint32_t end = cursor.bump().span.end;
            for (;;) {
                TokenKind next = cursor.peek().kind;
                if (next == TokenKind::Eof || next == TokenKind::Definition ||
                    next == TokenKind::DefinitionTemplate || starts_declaration(next)) {
                    break;
                }
                end = cursor.bump().span.end;
            }
            ast.ignored.push_back(Span::new_span(cursor.file(), start, end));
        }
    }
    return ast;
}

} // namespace

ParseResult parse_source(std::string_view input, FileId file) {
    LexResult lexed = lex(input, file);
    if (auto *error = std::get_if<LexError>(&lexed)) {
        return ParseError{error->span, error->message};
    }
    Cursor cursor(std::get<std::vector<Token>>(std::move(lexed)), file);
    return parse_file(cursor);
}

std::variant<Expr, ParseError> parse_expr_text(std::string_view input) {
    LexResult lexed = lex(input, FileId::anonymous());
    if (auto *error = std::get_if<LexError>(&lexed)) {
        return ParseError{error->span, error->message};
    }
    Cursor cursor(std::get<std::vector<Token>>(std::move(lexed)), FileId::anonymous());
    Expr expr;
    if (auto error = parse_expr(cursor, expr)) {
        return *error;
    }
    return expr;
}

} // namespace fable::defs::compiler
