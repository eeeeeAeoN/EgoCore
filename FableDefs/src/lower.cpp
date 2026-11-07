#include "fable/defs/compiler/lower.hpp"

#include <cstdint>
#include <string>

namespace fable::defs::compiler {

namespace {

std::optional<LowerError> evaluate_number(const Expr &expr, Value &out) {
    std::string text = expr.text;
    bool floating = text.find('.') != std::string::npos ||
                    (!text.empty() && text.back() == 'f');
    try {
        if (floating) {
            if (!text.empty() && text.back() == 'f') {
                text.pop_back();
            }
            out.kind = Value::Kind::Float;
            out.floating = std::stod(text);
        } else {
            out.kind = Value::Kind::Integer;
            out.integer = std::stoll(text);
        }
        return std::nullopt;
    } catch (...) {
        return LowerError{expr.span, "invalid number `" + expr.text + "`"};
    }
}

} // namespace

std::optional<LowerError> evaluate_expr(const Expr &expr, const SymbolTable &symbols,
                                        const DefinitionIndex *definitions, Value &out) {
    switch (expr.kind) {
    case Expr::Kind::Number:
        return evaluate_number(expr, out);
    case Expr::Kind::Bool:
        out.kind = Value::Kind::Bool;
        out.boolean = expr.boolean;
        return std::nullopt;
    case Expr::Kind::String:
        out.kind = Value::Kind::String;
        out.string = expr.text;
        return std::nullopt;
    case Expr::Kind::Symbol: {
        if (expr.text == "NULL") {
            out.kind = Value::Kind::Integer;
            out.integer = 0;
            return std::nullopt;
        }
        if (expr.text == "true" || expr.text == "TRUE" || expr.text == "BTRUE") {
            out.kind = Value::Kind::Bool;
            out.boolean = true;
            return std::nullopt;
        }
        if (expr.text == "false" || expr.text == "FALSE" || expr.text == "BFALSE") {
            out.kind = Value::Kind::Bool;
            out.boolean = false;
            return std::nullopt;
        }
        if (auto value = symbols.lookup(expr.text)) {
            out.kind = Value::Kind::Integer;
            out.integer = *value;
            return std::nullopt;
        }
        if (definitions != nullptr) {
            if (auto index = definitions->index_of(expr.text)) {
                out.kind = Value::Kind::Integer;
                out.integer = *index;
                return std::nullopt;
            }
        }
        return LowerError{expr.span, "unknown constant or definition `" + expr.text + "`"};
    }
    case Expr::Kind::Constructor:
        // A one-argument constructor is the common scalar wrapper form
        // (`EParticle(...)`, `CDefString(...)`). More complex constructors are
        // handled by the bespoke lowering layer later.
        if (expr.arguments.size() == 1) {
            return evaluate_expr(expr.arguments[0], symbols, definitions, out);
        }
        return LowerError{expr.span,
                          "constructor `" + expr.text + "` is not supported by generic lowering"};
    case Expr::Kind::BitOr: {
        out.kind = Value::Kind::Integer;
        std::int64_t value = 0;
        for (const auto &term : expr.terms) {
            Value child;
            if (auto error = evaluate_expr(term, symbols, definitions, child)) {
                return error;
            }
            if (child.kind == Value::Kind::Bool) {
                value |= child.boolean ? 1 : 0;
            } else if (child.kind == Value::Kind::Integer) {
                value |= child.integer;
            } else {
                return LowerError{term.span, "bit-or term is not an integer"};
            }
        }
        out.integer = value;
        return std::nullopt;
    }
    case Expr::Kind::Add: {
        bool floating = false;
        double f = 0.0;
        std::int64_t i = 0;
        for (const auto &term : expr.terms) {
            Value child;
            if (auto error = evaluate_expr(term, symbols, definitions, child)) {
                return error;
            }
            if (child.kind == Value::Kind::Float) {
                if (!floating) {
                    f = static_cast<double>(i);
                    floating = true;
                }
                f += child.floating;
            } else if (child.kind == Value::Kind::Integer) {
                if (floating) {
                    f += static_cast<double>(child.integer);
                } else {
                    i += child.integer;
                }
            } else if (child.kind == Value::Kind::Bool) {
                if (floating) {
                    f += child.boolean ? 1.0 : 0.0;
                } else {
                    i += child.boolean ? 1 : 0;
                }
            } else {
                return LowerError{term.span, "addition term is not numeric"};
            }
        }
        if (floating) {
            out.kind = Value::Kind::Float;
            out.floating = f;
        } else {
            out.kind = Value::Kind::Integer;
            out.integer = i;
        }
        return std::nullopt;
    }
    }
    return LowerError{expr.span, "unsupported expression"};
}

} // namespace fable::defs::compiler
