#include "fable/defs/compiler/symbols.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace fable::defs::compiler {

std::optional<std::int64_t> SymbolTable::lookup(const std::string &name) const {
    auto it = symbols_.find(name);
    if (it == symbols_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::int64_t> SymbolTable::insert(const std::string &name,
                                                std::int64_t value) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        std::int64_t previous = it->second;
        it->second = value;
        return previous;
    }
    symbols_.emplace(name, value);
    return std::nullopt;
}

std::optional<SymbolError> SymbolTable::evaluate_items(
    const std::vector<Item> &items, std::vector<Redefinition> &redefined) {
    for (const auto &item : items) {
        if (auto error = evaluate_item(item, redefined)) {
            return error;
        }
    }
    return std::nullopt;
}

std::optional<SymbolError> SymbolTable::evaluate_item(
    const Item &item, std::vector<Redefinition> &redefined) {
    switch (item.kind) {
    case Item::Kind::Enum:
        return evaluate_enum(item.enum_decl, redefined);
    case Item::Kind::Define:
        if (item.define.value) {
            define(item.define.name, *item.define.value, redefined);
        }
        return std::nullopt;
    case Item::Kind::Namespace:
        if (item.namespace_) {
            for (const auto &child : item.namespace_->items) {
                if (auto error = evaluate_item(child, redefined)) {
                    return error;
                }
            }
        }
        return std::nullopt;
    case Item::Kind::Conditional:
        if (item.conditional) {
            bool defined = item.conditional->condition == "_WINDOWS";
            bool taken = defined != item.conditional->inverted;
            const std::vector<Item> *branch = nullptr;
            if (taken) {
                branch = &item.conditional->if_branch;
            } else if (item.conditional->else_branch) {
                branch = &*item.conditional->else_branch;
            }
            if (branch) {
                for (const auto &child : *branch) {
                    if (auto error = evaluate_item(child, redefined)) {
                        return error;
                    }
                }
            }
        }
        return std::nullopt;
    case Item::Kind::Definition:
    case Item::Kind::PragmaOnce:
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<SymbolError> SymbolTable::evaluate_enum(
    const EnumDecl &decl, std::vector<Redefinition> &redefined) {
    std::optional<std::int64_t> last;
    for (const auto &variant : decl.variants) {
        std::int64_t value = 0;
        if (variant.value) {
            if (auto error = evaluate_enum_expr(*variant.value, value)) {
                return error;
            }
        } else {
            value = last ? *last + 1 : 0;
        }
        define(variant.name, value, redefined);
        last = value;
    }
    return std::nullopt;
}

std::optional<SymbolError> SymbolTable::evaluate_enum_expr(const EnumExpr &expr,
                                                           std::int64_t &out) {
    switch (expr.kind) {
    case EnumExpr::Kind::Int:
        out = expr.integer;
        return std::nullopt;
    case EnumExpr::Kind::Ident:
        if (auto value = lookup(expr.ident)) {
            out = *value;
            return std::nullopt;
        }
        return SymbolError{expr.span, "unknown constant `" + expr.ident + "`"};
    case EnumExpr::Kind::Shift: {
        if (expr.terms.empty()) {
            out = 0;
            return std::nullopt;
        }
        std::int64_t value = 0;
        if (auto error = evaluate_enum_expr(expr.terms.front(), value)) {
            return error;
        }
        for (std::size_t i = 1; i < expr.terms.size(); ++i) {
            std::int64_t amount = 0;
            if (auto error = evaluate_enum_expr(expr.terms[i], amount)) {
                return error;
            }
            if (amount < 0 || amount >= 64) {
                return SymbolError{
                    expr.terms[i].span,
                    "shift amount " + std::to_string(amount) +
                        " is out of range (expected 0..64)"};
            }
            value <<= amount;
        }
        out = value;
        return std::nullopt;
    }
    case EnumExpr::Kind::BitOr: {
        std::int64_t value = 0;
        for (const auto &term : expr.terms) {
            std::int64_t next = 0;
            if (auto error = evaluate_enum_expr(term, next)) {
                return error;
            }
            value |= next;
        }
        out = value;
        return std::nullopt;
    }
    }
    return std::nullopt;
}

void SymbolTable::define(const Spanned<std::string> &name, std::int64_t value,
                         std::vector<Redefinition> &redefined) {
    auto it = symbols_.find(name.value);
    if (it != symbols_.end()) {
        redefined.push_back(Redefinition{name.value, it->second, value, name.span});
        it->second = value;
    } else {
        symbols_.emplace(name.value, value);
    }
}

} // namespace fable::defs::compiler
