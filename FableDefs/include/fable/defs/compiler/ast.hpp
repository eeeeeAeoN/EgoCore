// The text-def / header AST.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fable/defs/compiler/diagnostic.hpp"

namespace fable::defs::compiler {

struct Expr {
    enum class Kind { Number, Bool, String, Symbol, Constructor, BitOr, Add };

    Kind kind = Kind::Number;
    /// Number/String/Symbol text, or the constructor name.
    std::string text;
    bool boolean = false;
    /// Constructor arguments.
    std::vector<Expr> arguments;
    /// BitOr/Add terms.
    std::vector<Expr> terms;
    Span span;
};

const char *expr_kind_name(Expr::Kind kind);

struct PathSegment {
    enum class Kind { Field, Index };

    Kind kind = Kind::Field;
    std::string field;
    Expr index;
    Span span;
};

struct PropertyPath {
    std::vector<PathSegment> segments;
};

struct Field {
    PropertyPath path;
    Expr expr;
};

struct MethodCall {
    PropertyPath object;
    std::string method;
    std::vector<Expr> arguments;
};

struct Statement;

struct TaggedBlock {
    std::string tag;
    std::vector<Statement> body;
};

struct Statement {
    enum class Kind { Field, MethodCall, TaggedBlock };

    Kind kind = Kind::Field;
    Field field;
    MethodCall method_call;
    std::shared_ptr<TaggedBlock> tagged_block;
    Span span;
};

struct Definition {
    bool is_template = false;
    std::string def_type;
    std::string name;
    std::optional<std::string> specializes;
    Span specializes_span;
    std::vector<Statement> body;
    Span span;
};

struct EnumExpr {
    enum class Kind { Int, Ident, Shift, BitOr };

    Kind kind = Kind::Int;
    std::int64_t integer = 0;
    std::string ident;
    std::vector<EnumExpr> terms;
    Span span;
};

struct EnumVariant {
    Spanned<std::string> name;
    std::optional<EnumExpr> value;
};

struct EnumDecl {
    std::optional<Spanned<std::string>> name;
    std::vector<EnumVariant> variants;
};

struct Define {
    Spanned<std::string> name;
    /// `#define NAME` has no value and binds no symbol.
    std::optional<std::int64_t> value;
};

struct Namespace {
    std::string name;
    std::vector<struct Item> items;
};

struct IfDef {
    std::string condition;
    std::vector<struct Item> if_branch;
    std::optional<std::vector<struct Item>> else_branch;
    bool inverted = false;
};

struct Item {
    enum class Kind { Definition, Enum, Define, Namespace, Conditional, PragmaOnce };

    Kind kind = Kind::Definition;
    Spanned<Definition> definition;
    EnumDecl enum_decl;
    Define define;
    std::shared_ptr<Namespace> namespace_;
    std::shared_ptr<IfDef> conditional;
    Span span;
};

struct SourceAst {
    std::vector<Item> items;
    std::vector<Span> ignored;

    std::vector<const Spanned<Definition> *> definitions() const;
};

/// Whether an item kind declares symbols at file scope.
bool starts_declaration(Item::Kind kind);

} // namespace fable::defs::compiler
