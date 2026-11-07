// Declaration evaluation: enum variants and #defines -> name -> i64.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/diagnostic.hpp"

namespace fable::defs::compiler {

struct SymbolError {
    Span span;
    std::string message;
};

struct Redefinition {
    std::string name;
    std::int64_t previous = 0;
    std::int64_t value = 0;
    Span span;
};

class SymbolTable {
public:
    std::optional<std::int64_t> lookup(const std::string &name) const;
    std::size_t size() const { return symbols_.size(); }
    bool empty() const { return symbols_.empty(); }
    const std::map<std::string, std::int64_t> &symbols() const { return symbols_; }

    /// Insert, returning the value it replaced, if any.
    std::optional<std::int64_t> insert(const std::string &name, std::int64_t value);

    /// Walk a file's declarations, skipping definitions. Last definition wins.
    std::optional<SymbolError> evaluate_items(const std::vector<Item> &items,
                                              std::vector<Redefinition> &redefined);

private:
    std::optional<SymbolError> evaluate_item(const Item &item,
                                             std::vector<Redefinition> &redefined);
    std::optional<SymbolError> evaluate_enum(const EnumDecl &decl,
                                             std::vector<Redefinition> &redefined);
    std::optional<SymbolError> evaluate_enum_expr(const EnumExpr &expr,
                                                  std::int64_t &out);
    void define(const Spanned<std::string> &name, std::int64_t value,
                std::vector<Redefinition> &redefined);

    std::map<std::string, std::int64_t> symbols_;
};


/// Engine enums the def-script parser registers in C++ but that appear in no
/// text header. Missing names are injected after headers are read, so a corpus
/// definition always wins.
inline void inject_engine_enums(SymbolTable &symbols) {
    auto fallback = [&](const char *name, std::int64_t value) {
        if (!symbols.lookup(name).has_value()) {
            symbols.insert(name, value);
        }
    };
    fallback("WATER_BUMP_PC", 0);
    fallback("BLEND_NULL", 0);
    fallback("BLEND_ADDITIVE", 1);
    fallback("BLEND_ALPHA", 2);
    fallback("BLEND_SOLID", 3);
    fallback("BLEND_MULTIPLY", 4);
}

} // namespace fable::defs::compiler
