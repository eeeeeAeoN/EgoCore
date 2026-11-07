// Generic lowering: assign parsed text fields to a C++ data-model class.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fable/defs/visit.hpp>
#include <fable/defs/transfer.hpp>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/definitions.hpp"
#include "fable/defs/compiler/diagnostic.hpp"
#include "fable/defs/compiler/names.hpp"
#include "fable/defs/compiler/symbols.hpp"
#include "fable/defs/compiler/type_traits.hpp"
#include "fable/defs/compiler/wire.hpp"

namespace fable::defs::compiler {

struct LowerError {
    Span span;
    std::string message;
};

struct LowerStats {
    std::size_t applied = 0;
    std::size_t absent = 0;
    std::size_t unsupported = 0;
    std::vector<LowerError> errors;
};

/// The scalar value an expression evaluated to.
struct Value {
    enum class Kind { Integer, Float, Bool, String };

    Kind kind = Kind::Integer;
    std::int64_t integer = 0;
    double floating = 0.0;
    bool boolean = false;
    std::string string;
};

/// Evaluate an expression against the header symbol table. `NULL` is 0.
std::optional<LowerError> evaluate_expr(const Expr &expr, const SymbolTable &symbols,
                                        const DefinitionIndex *definitions, Value &out);

namespace detail {

template <class T>
inline constexpr bool is_leaf_class_v =
    std::is_same_v<T, std::string> || std::is_same_v<T, std::u16string> ||
    std::is_same_v<T, CDefString> || std::is_same_v<T, CDefIndex>;

template <class T>
bool assign_value(T &field, const Value &value, LowerStats &stats,
                  NamesBuilder *names = nullptr,
                  const DefinitionIndex *definitions = nullptr) {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, CDefString>) {
        if (value.kind == Value::Kind::String) {
            field.TablePos = value.string.empty()
                                 ? -1
                                 : (names != nullptr
                                        ? static_cast<std::int32_t>(names->intern(value.string))
                                        : -1);
            return true;
        }
        if (value.kind == Value::Kind::Integer) {
            field.TablePos = static_cast<std::int32_t>(value.integer);
            return true;
        }
        if (value.kind == Value::Kind::Bool) {
            field.TablePos = value.boolean ? 1 : 0;
            return true;
        }
        return false;
    } else if constexpr (std::is_same_v<U, CDefIndex>) {
        if (value.kind == Value::Kind::String && definitions != nullptr) {
            if (auto index = definitions->index_of(value.string)) {
                field.Val = *index;
                return true;
            }
            return false;
        }
        if (value.kind == Value::Kind::Integer) {
            field.Val = static_cast<std::int32_t>(value.integer);
            return true;
        }
        if (value.kind == Value::Kind::Bool) {
            field.Val = value.boolean ? 1 : 0;
            return true;
        }
        return false;
    } else if constexpr (std::is_same_v<U, std::string>) {
        if (value.kind != Value::Kind::String) {
            return false;
        }
        field = value.string;
        return true;
    } else if constexpr (std::is_same_v<U, std::u16string>) {
        if (value.kind != Value::Kind::String) {
            return false;
        }
        // The corpus is predominantly ASCII. Full UTF-8 conversion belongs in
        // the text layer; this keeps the data path symmetric for now.
        field.clear();
        field.reserve(value.string.size());
        for (unsigned char c : value.string) {
            field.push_back(static_cast<char16_t>(c));
        }
        return true;
    } else if constexpr (std::is_same_v<U, bool>) {
        if (value.kind == Value::Kind::Bool) {
            field = value.boolean;
            return true;
        }
        if (value.kind == Value::Kind::Integer) {
            field = value.integer != 0;
            return true;
        }
        return false;
    } else if constexpr (std::is_enum_v<U>) {
        if (value.kind == Value::Kind::Integer) {
            field = static_cast<U>(value.integer);
            return true;
        }
        if (value.kind == Value::Kind::Bool) {
            field = static_cast<U>(value.boolean ? 1 : 0);
            return true;
        }
        return false;
    } else if constexpr (std::is_integral_v<U>) {
        if (value.kind == Value::Kind::Integer) {
            field = static_cast<U>(value.integer);
            return true;
        }
        if (value.kind == Value::Kind::Bool) {
            field = static_cast<U>(value.boolean ? 1 : 0);
            return true;
        }
        return false;
    } else if constexpr (std::is_floating_point_v<U>) {
        if (value.kind == Value::Kind::Float) {
            field = static_cast<U>(value.floating);
            return true;
        }
        if (value.kind == Value::Kind::Integer) {
            field = static_cast<U>(value.integer);
            return true;
        }
        return false;
    } else {
        (void)field;
        (void)value;
        (void)stats;
        return false;
    }
}

template <class E>
bool assign_vector_element(E &element, const Value &value, LowerStats &stats,
                           NamesBuilder *names = nullptr,
                           const DefinitionIndex *definitions = nullptr) {
    using U = std::remove_cv_t<std::remove_reference_t<E>>;
    if constexpr (std::is_same_v<U, bool>) {
        if (value.kind == Value::Kind::Bool) {
            element = value.boolean;
            return true;
        }
        if (value.kind == Value::Kind::Integer) {
            element = value.integer != 0;
            return true;
        }
        return false;
    } else {
        return assign_value(element, value, stats, names, definitions);
    }
}

struct Context {
    const SymbolTable &symbols;
    const DefinitionIndex *definitions;
    LowerStats &stats;
    std::vector<std::string> *applied;
    NamesBuilder *names;
};

template <class T>
void lower_object(T &object, const std::vector<const Statement *> &statements,
                  std::size_t depth, Context &context);

// ── path helpers ───────────────────────────────────────────────────────────

inline bool field_at(const Statement &statement, std::size_t depth,
                     const std::string &name) {
    if (statement.kind != Statement::Kind::Field) {
        return false;
    }
    const auto &segments = statement.field.path.segments;
    return segments.size() > depth && segments[depth].kind == PathSegment::Kind::Field &&
           segments[depth].field == name;
}

inline bool method_at(const Statement &statement, std::size_t depth,
                      const std::string &name) {
    if (statement.kind != Statement::Kind::MethodCall) {
        return false;
    }
    const auto &segments = statement.method_call.object.segments;
    return segments.size() == depth + 1 && segments[depth].kind == PathSegment::Kind::Field &&
           segments[depth].field == name;
}

inline bool method_path_at(const Statement &statement, std::size_t depth,
                           const std::string &name) {
    if (statement.kind != Statement::Kind::MethodCall) {
        return false;
    }
    const auto &segments = statement.method_call.object.segments;
    return segments.size() > depth && segments[depth].kind == PathSegment::Kind::Field &&
           segments[depth].field == name;
}

inline std::vector<const Statement *> children(const std::vector<const Statement *> &statements,
                                               std::size_t depth, const std::string &name) {
    std::vector<const Statement *> out;
    for (const Statement *statement : statements) {
        if (field_at(*statement, depth, name) || method_path_at(*statement, depth, name)) {
            out.push_back(statement);
        }
    }
    return out;
}

inline const Expr *leaf_value(const std::vector<const Statement *> &statements,
                              std::size_t depth) {
    const Expr *out = nullptr;
    for (const Statement *statement : statements) {
        if (statement->kind == Statement::Kind::Field &&
            statement->field.path.segments.size() == depth + 1) {
            out = &statement->field.expr; // last wins
        }
    }
    return out;
}

inline bool has_deeper(const std::vector<const Statement *> &statements,
                       std::size_t depth) {
    for (const Statement *statement : statements) {
        if (statement->kind == Statement::Kind::Field &&
            statement->field.path.segments.size() > depth + 1) {
            return true;
        }
    }
    return false;
}

/// A statement whose path ends exactly at `size` (a direct element/whole value).
inline const Expr *direct_value(const std::vector<const Statement *> &statements,
                                std::size_t size) {
    const Expr *out = nullptr;
    for (const Statement *statement : statements) {
        if (statement->kind == Statement::Kind::Field &&
            statement->field.path.segments.size() == size) {
            out = &statement->field.expr;
        }
    }
    return out;
}

inline bool has_method_work(const std::vector<const Statement *> &statements,
                            std::size_t depth) {
    for (const Statement *statement : statements) {
        if (statement->kind == Statement::Kind::MethodCall &&
            statement->method_call.object.segments.size() == depth + 1) {
            return true;
        }
    }
    return false;
}

inline bool has_index_work(const std::vector<const Statement *> &statements,
                           std::size_t depth) {
    for (const Statement *statement : statements) {
        const std::vector<PathSegment> *path = nullptr;
        if (statement->kind == Statement::Kind::Field) {
            path = &statement->field.path.segments;
        } else if (statement->kind == Statement::Kind::MethodCall) {
            path = &statement->method_call.object.segments;
        } else {
            continue;
        }
        const auto &segments = *path;
        if (segments.size() > depth + 1 &&
            segments[depth + 1].kind == PathSegment::Kind::Index) {
            return true;
        }
    }
    return false;
}

inline void record(Context &context, const std::string &name) {
    ++context.stats.applied;
    if (context.applied) {
        context.applied->push_back(name);
    }
}

inline bool evaluate_leaf(const Expr *expr, Context &context, Value &value) {
    if (expr == nullptr) {
        return false;
    }
    if (auto error = evaluate_expr(*expr, context.symbols, context.definitions, value)) {
        context.stats.errors.push_back(std::move(*error));
        return false;
    }
    return true;
}

template <class M>
void assign_add_to_map(M &field, const Expr &add, Context &context) {
    using Key = typename IsMap<M>::key_type;
    using Mapped = typename IsMap<M>::mapped_type;
    if (add.kind != Expr::Kind::Constructor || add.arguments.size() < 2) {
        ++context.stats.unsupported;
        return;
    }
    Key key{};
    Value key_value;
    if (!evaluate_leaf(&add.arguments[0], context, key_value) ||
        !assign_value(key, key_value, context.stats, context.names, context.definitions)) {
        ++context.stats.unsupported;
        return;
    }
    Mapped mapped{};
    Value mapped_value;
    if (evaluate_leaf(&add.arguments[1], context, mapped_value) &&
        assign_value(mapped, mapped_value, context.stats, context.names, context.definitions)) {
        field[key] = mapped;
        ++context.stats.applied;
    } else {
        ++context.stats.unsupported;
    }
}

template <class T>
void assign_constructor(T &field, const Expr &expr, Context &context);

/// Positional assignment for a compound constructor such as
/// `CBrainUpdateZone(...)` or `CRGBColour(...)`. The model's visit order is
/// the constructor argument order for the generic case; the bespoke arms in
/// the Rust compiler handle the few permutations.
class CtorVisitor {
public:
    CtorVisitor(const Expr &expr, Context &context)
        : expr_(expr), context_(context) {}

    template <class U>
    void operator()(const char *, U &field) {
        if (index_ >= expr_.arguments.size()) {
            return;
        }
        const Expr &arg = expr_.arguments[index_++];
        if (arg.kind == Expr::Kind::Constructor && is_model_class<U>::value) {
            assign_constructor(field, arg, context_);
            ++assigned_;
            return;
        }
        Value value;
        if (evaluate_leaf(&arg, context_, value) &&
            assign_value(field, value, context_.stats, context_.names, context_.definitions)) {
            ++assigned_;
        } else {
            ++context_.stats.unsupported;
        }
    }

    template <class U>
    void operator()(const char *, ::fable::defs::detail::BitfieldRef<U> ref) {
        if (index_ >= expr_.arguments.size()) {
            return;
        }
        const Expr &arg = expr_.arguments[index_++];
        Value value;
        if (evaluate_leaf(&arg, context_, value) &&
            (value.kind == Value::Kind::Bool || value.kind == Value::Kind::Integer)) {
            ref.set(value.kind == Value::Kind::Bool ? value.boolean : value.integer != 0);
            ++assigned_;
        } else {
            ++context_.stats.unsupported;
        }
    }

    std::size_t assigned() const { return assigned_; }

private:
    const Expr &expr_;
    Context &context_;
    std::size_t index_ = 0;
    std::size_t assigned_ = 0;
};

template <class T>
void assign_constructor(T &field, const Expr &expr, Context &context) {
    if constexpr (std::is_same_v<T, CActionInputControl>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.size() < 3) {
            ++context.stats.unsupported;
            return;
        }
        CActionInputControl value{};
        Value scalar;
        if (evaluate_leaf(&expr.arguments[0], context, scalar)) {
            assign_value(value.GameAction, scalar, context.stats, context.names, context.definitions);
        }
        if (evaluate_leaf(&expr.arguments[1], context, scalar)) {
            assign_value(value.ControllerType, scalar, context.stats, context.names, context.definitions);
        }
        if (evaluate_leaf(&expr.arguments[2], context, scalar)) {
            switch (value.ControllerType) {
            case CONTROLLER_KEYBOARD:
                assign_value(value.KeyboardKey, scalar, context.stats, context.names, context.definitions);
                break;
            case CONTROLLER_XBOX_PAD:
                assign_value(value.XboxButton, scalar, context.stats, context.names, context.definitions);
                break;
            case CONTROLLER_MOUSE:
                assign_value(value.MouseButton, scalar, context.stats, context.names, context.definitions);
                break;
            default:
                break;
            }
        }
        if (expr.arguments.size() >= 4 &&
            expr.arguments[3].kind == Expr::Kind::Constructor) {
            assign_constructor(value.ControlDirection, expr.arguments[3], context);
        }
        field = std::move(value);
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::COpinionTransientOffset>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.empty()) {
            ++context.stats.unsupported;
            return;
        }
        std::array<Value, 5> values{};
        for (std::size_t i = 0; i < expr.arguments.size() && i < values.size(); ++i) {
            evaluate_leaf(&expr.arguments[i], context, values[i]);
        }
        const auto as_float = [](const Value &v) {
            return v.kind == Value::Kind::Float ? static_cast<float>(v.floating)
                                                : static_cast<float>(v.integer);
        };
        const std::int32_t opinion = static_cast<std::int32_t>(values[0].integer);
        float peak = 0.0f;
        float run_in_secs = 0.0f;
        float run_out_secs = 0.0f;
        float persist = 0.0f;
        if (expr.arguments.size() >= 5) {
            peak = as_float(values[1]);
            run_in_secs = as_float(values[2]);
            run_out_secs = as_float(values[3]);
            persist = as_float(values[4]);
        } else {
            persist = as_float(values[1]);
        }
        const float fps = 15.0f;
        const std::int32_t run_in_frames =
            static_cast<std::int32_t>(std::lround(run_in_secs * fps));
        const std::int32_t run_out_frames =
            static_cast<std::int32_t>(std::lround(run_out_secs * fps));
        const double peak64 = peak;
        const double persist64 = persist;
        float offset_in = peak;
        std::int32_t frames_in = 1;
        if (run_in_frames != 0) {
            offset_in = static_cast<float>(peak64 / run_in_frames);
            frames_in = run_in_frames;
        }
        float offset_out = persist - peak;
        std::int32_t frames_out = 1;
        if (run_out_frames != 0) {
            offset_out = static_cast<float>((persist64 - peak64) / run_out_frames);
            frames_out = run_out_frames;
        }
        ::fable::defs::COpinionTransientOffset value;
        value.Opinion = static_cast<EOpinion>(opinion);
        value.OffsetPerFrameRunIn = offset_in;
        value.FramesToRunIn = frames_in;
        value.FramesOfCappedPeak = 0;
        value.OffsetPerFrameRunOut = offset_out;
        value.FramesToRunOut = frames_out;
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CExplosionRing>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.size() < 2) {
            ++context.stats.unsupported;
            return;
        }
        ::fable::defs::CExplosionRing value{};
        Value radius;
        Value count;
        evaluate_leaf(&expr.arguments[0], context, radius);
        evaluate_leaf(&expr.arguments[1], context, count);
        value.RingRadius = radius.kind == Value::Kind::Float
                               ? static_cast<float>(radius.floating)
                               : static_cast<float>(radius.integer);
        value.NumExplosions = static_cast<std::int32_t>(count.integer);
        if (expr.arguments.size() >= 3) {
            Value angle;
            if (evaluate_leaf(&expr.arguments[2], context, angle)) {
                value.AngleOffset = angle.kind == Value::Kind::Float
                                        ? static_cast<float>(angle.floating)
                                        : static_cast<float>(angle.integer);
            }
        }
        if (expr.arguments.size() >= 4) {
            Value seconds;
            if (evaluate_leaf(&expr.arguments[3], context, seconds)) {
                value.SecondsBeforeSpawnNewRing =
                    seconds.kind == Value::Kind::Float
                        ? static_cast<float>(seconds.floating)
                        : static_cast<float>(seconds.integer);
            }
        }
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::NParticleAttachment::CParticleAttachmentInfo>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.size() < 2) {
            ++context.stats.unsupported;
            return;
        }
        ::fable::defs::NParticleAttachment::CParticleAttachmentInfo value{};
        if (expr.arguments[0].kind == Expr::Kind::String &&
            !expr.arguments[0].text.empty() && context.names != nullptr) {
            value.AttachmentObjectName.TablePos = static_cast<std::int32_t>(
                context.names->intern(expr.arguments[0].text));
        }
        Value particle;
        if (evaluate_leaf(&expr.arguments[1], context, particle)) {
            value.ParticleIndex = static_cast<std::int32_t>(particle.integer);
        }
        if (expr.arguments.size() >= 3) {
            Value distance;
            if (evaluate_leaf(&expr.arguments[2], context, distance)) {
                value.GenerationCutoffDistance = distance.kind == Value::Kind::Float
                                                     ? static_cast<float>(distance.floating)
                                                     : static_cast<float>(distance.integer);
            }
        }
        if (expr.arguments.size() >= 4) {
            Value dummy;
            if (evaluate_leaf(&expr.arguments[3], context, dummy)) {
                value.DummyObject = dummy.kind == Value::Kind::Bool
                                        ? dummy.boolean
                                        : dummy.integer != 0;
            }
        }
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CObjectAugmentationParticleSet>) {
        if (expr.kind != Expr::Kind::Constructor) {
            ++context.stats.unsupported;
            return;
        }
        const auto arg_i32 = [&](std::size_t index) -> std::int32_t {
            Value value;
            if (index < expr.arguments.size()) {
                evaluate_leaf(&expr.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        const auto arg_string_or_i32 = [&](std::size_t index) -> std::int32_t {
            if (index >= expr.arguments.size()) {
                return 0;
            }
            const Expr &arg = expr.arguments[index];
            if (arg.kind == Expr::Kind::String) {
                if (arg.text.empty() || context.names == nullptr) {
                    return -1;
                }
                return static_cast<std::int32_t>(context.names->intern(arg.text));
            }
            return arg_i32(index);
        };
        ::fable::defs::CObjectAugmentationParticleSet value{};
        Value period;
        if (!expr.arguments.empty()) {
            evaluate_leaf(&expr.arguments[0], context, period);
            value.PeriodSecs = period.kind == Value::Kind::Float
                                   ? static_cast<float>(period.floating)
                                   : static_cast<float>(period.integer);
        }
        value.ParticleEffect = arg_string_or_i32(2);
        value.ParticleEffectToBlendTo = arg_string_or_i32(3);
        value.AttackParticleEffect = arg_string_or_i32(4);
        value.AttackParticleEffectToBlendTo = arg_string_or_i32(5);
        const std::int32_t pairs = std::max<std::int32_t>(arg_i32(1), 0);
        for (std::int32_t i = 0; i < pairs; ++i) {
            const std::size_t start_index = 6 + static_cast<std::size_t>(i) * 2;
            const std::size_t end_index = start_index + 1;
            ::fable::defs::CDefString start;
            start.TablePos = arg_string_or_i32(start_index);
            value.StartPoint.push_back(start);
            ::fable::defs::CDefString end;
            end.TablePos = arg_string_or_i32(end_index);
            value.EndPoint.push_back(end);
        }
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CBlendedParticleEffectSet>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.size() != 4) {
            ++context.stats.unsupported;
            return;
        }
        CBlendedParticleEffectSet value{};
        Value particle1;
        Value particle1_value;
        Value particle2;
        Value particle2_value;
        evaluate_leaf(&expr.arguments[0], context, particle1);
        evaluate_leaf(&expr.arguments[1], context, particle1_value);
        evaluate_leaf(&expr.arguments[2], context, particle2);
        evaluate_leaf(&expr.arguments[3], context, particle2_value);
        value.Particle1Index = static_cast<std::int32_t>(particle1.integer);
        value.Particle1Value = particle1_value.kind == Value::Kind::Float
                                   ? static_cast<float>(particle1_value.floating)
                                   : static_cast<float>(particle1_value.integer);
        value.Particle2Index = static_cast<std::int32_t>(particle2.integer);
        value.Particle2Value = particle2_value.kind == Value::Kind::Float
                                   ? static_cast<float>(particle2_value.floating)
                                   : static_cast<float>(particle2_value.integer);
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CDegradableInfo>) {
        if (expr.kind != Expr::Kind::Constructor) {
            ++context.stats.unsupported;
            return;
        }
        ::fable::defs::CDegradableInfo value{};
        std::array<Value, 4> args{};
        for (std::size_t i = 0; i < expr.arguments.size() && i < args.size(); ++i) {
            evaluate_leaf(&expr.arguments[i], context, args[i]);
        }
        if (args[0].kind == Value::Kind::Float) {
            value.HealthPercentage = static_cast<float>(args[0].floating);
        } else {
            value.HealthPercentage = static_cast<float>(args[0].integer);
        }
        value.BankIndex = static_cast<std::int32_t>(args[1].integer);
        value.SmashParticleEmitter = static_cast<std::int32_t>(args[2].integer);
        value.BlocksNavigation = args[3].kind == Value::Kind::Bool
                                     ? args[3].boolean
                                     : args[3].integer != 0;
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CPhysicalPrimitiveInit>) {
        if (expr.kind != Expr::Kind::Constructor) {
            ++context.stats.unsupported;
            return;
        }
        ::fable::defs::CPhysicalPrimitiveInit value{};
        const bool sphere = expr.text.find("Sphere") != std::string::npos;
        const bool cylinder = expr.text.find("Cylinder") != std::string::npos;
        const bool mesh = expr.text.find("Mesh") != std::string::npos;
        if (sphere) {
            value.Type = 1;
            value.BaseName.TablePos = -1;
            value.Radius = -1.0f;
            if (expr.arguments.size() >= 2) {
                if (expr.arguments[0].kind == Expr::Kind::String &&
                    !expr.arguments[0].text.empty() && context.names != nullptr) {
                    value.BaseName.TablePos = static_cast<std::int32_t>(
                        context.names->intern(expr.arguments[0].text));
                }
                Value radius;
                if (evaluate_leaf(&expr.arguments[1], context, radius)) {
                    value.Radius = radius.kind == Value::Kind::Float
                                       ? static_cast<float>(radius.floating)
                                       : static_cast<float>(radius.integer);
                }
            }
        } else if (cylinder) {
            value.Type = 2;
            value.Radius = -1.0f;
            value.Height = -1.0f;
            if (expr.arguments.size() >= 3) {
                if (expr.arguments[0].kind == Expr::Kind::String &&
                    !expr.arguments[0].text.empty() && context.names != nullptr) {
                    value.BaseName.TablePos = static_cast<std::int32_t>(
                        context.names->intern(expr.arguments[0].text));
                }
                Value radius;
                if (evaluate_leaf(&expr.arguments[1], context, radius)) {
                    value.Radius = radius.kind == Value::Kind::Float
                                       ? static_cast<float>(radius.floating)
                                       : static_cast<float>(radius.integer);
                }
                Value height;
                if (evaluate_leaf(&expr.arguments[2], context, height)) {
                    value.Height = height.kind == Value::Kind::Float
                                       ? static_cast<float>(height.floating)
                                       : static_cast<float>(height.integer);
                }
            }
        } else if (mesh) {
            value.Type = 3;
        } else {
            value.Type = 0;
        }
        field = value;
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, ::fable::defs::CRGBColour>) {
        if (expr.kind != Expr::Kind::Constructor ||
            (expr.arguments.size() != 3 && expr.arguments.size() != 4)) {
            ++context.stats.unsupported;
            return;
        }
        std::uint32_t channels[4] = {0, 0, 0, 255};
        for (std::size_t i = 0; i < expr.arguments.size(); ++i) {
            Value value;
            if (evaluate_leaf(&expr.arguments[i], context, value)) {
                if (value.kind == Value::Kind::Integer) {
                    channels[i] = static_cast<std::uint32_t>(value.integer) & 0xFFu;
                } else if (value.kind == Value::Kind::Bool) {
                    channels[i] = value.boolean ? 1u : 0u;
                }
            }
        }
        field.IntValue = (channels[3] << 24) | (channels[0] << 16) |
                         (channels[1] << 8) | channels[2];
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, CComboMultiplierData>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.size() < 2) {
            ++context.stats.unsupported;
            return;
        }
        CComboMultiplierData value{};
        Value scalar;
        if (evaluate_leaf(&expr.arguments[0], context, scalar)) {
            assign_value(value.ExperienceMultiplierNumSecondsForAttenuation, scalar,
                         context.stats, context.names, context.definitions);
        }
        if (evaluate_leaf(&expr.arguments[1], context, scalar)) {
            assign_value(value.ExperienceMultiplierParamDecayDeltaPerSecond, scalar,
                         context.stats, context.names, context.definitions);
        }
        for (std::size_t i = 2; i < expr.arguments.size(); ++i) {
            assign_add_to_map(value.ComboMultiplier, expr.arguments[i], context);
        }
        field = std::move(value);
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, CHeroStatIncreaseData>) {
        if (expr.kind != Expr::Kind::Constructor) {
            ++context.stats.unsupported;
            return;
        }
        CHeroStatIncreaseData value{};
        for (const Expr &arg : expr.arguments) {
            assign_add_to_map(value.StatIncreases, arg, context);
        }
        field = std::move(value);
        ++context.stats.applied;
    } else if constexpr (std::is_same_v<T, CAttackHistoryCombo>) {
        if (expr.kind != Expr::Kind::Constructor || expr.arguments.empty()) {
            ++context.stats.unsupported;
            return;
        }
        CAttackHistoryCombo value{};
        Value scalar;
        if (evaluate_leaf(&expr.arguments[0], context, scalar)) {
            assign_value(value.Multiplier, scalar, context.stats, context.names, context.definitions);
        }
        for (std::size_t i = 1; i < expr.arguments.size(); ++i) {
            const Expr &add = expr.arguments[i];
            if (add.kind != Expr::Kind::Constructor ||
                add.text != "Add" || add.arguments.empty()) {
                ++context.stats.unsupported;
                continue;
            }
            std::pair<EAttackHistoryType, EDamageAttribute> pair{};
            Value first;
            if (evaluate_leaf(&add.arguments[0], context, first)) {
                assign_value(pair.first, first, context.stats, context.names, context.definitions);
            }
            if (add.arguments.size() >= 2) {
                Value second;
                if (evaluate_leaf(&add.arguments[1], context, second)) {
                    assign_value(pair.second, second, context.stats, context.names, context.definitions);
                }
            } else {
                pair.second = static_cast<EDamageAttribute>(-1);
            }
            value.AttackCombo.push_back(pair);
            ++context.stats.applied;
        }
        field = std::move(value);
        ++context.stats.applied;
    } else if constexpr (is_model_class<T>::value) {
        if (expr.kind != Expr::Kind::Constructor) {
            ++context.stats.unsupported;
            return;
        }
        CtorVisitor visitor(expr, context);
        ::fable::defs::visit_model_fields(field, visitor);
        if (visitor.assigned() != 0) {
            ++context.stats.applied;
        }
    } else {
        (void)field;
        (void)expr;
        ++context.stats.unsupported;
    }
}

/// Schema-level defaults for a newly created container element. UI states
/// default `StateChangeFlag` to 7 even when the text never names the state.
template <class T>
void apply_element_schema_defaults(T &value) {
    ::fable::defs::apply_model_defaults(value);
    if constexpr (std::is_same_v<T, ::fable::defs::NUISystem::CUIStateDef>) {
        if (value.StateChangeFlag == 0) {
            value.StateChangeFlag = 7;
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CThingShotDef>) {
        value.ExplosionDef.Val = -1;
        value.GroundExplosionDef.Val = -1;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CEnvironmentThemeDef>) {
        value.WaterRefractionBlendEnd = 15.0f;
        value.WaterFlowSpeedFactor = 0.25f;
        value.WaterOscilationSpeed = 0.05f;
        value.WaterSwellFactor = 0.0625f;
        value.WaterShoreSwellFactor = 0.041666668f;
        value.WaterReflectionOffset = 0.01f;
    }
}

// ── containers ────────────────────────────────────────────────────────────

inline std::optional<std::int64_t> evaluate_index(const Expr &expr, Context &context) {
    Value value;
    if (!evaluate_leaf(&expr, context, value)) {
        return std::nullopt;
    }
    if (value.kind == Value::Kind::Integer) {
        return value.integer;
    }
    if (value.kind == Value::Kind::Bool) {
        return value.boolean ? 1 : 0;
    }
    return std::nullopt;
}

inline bool container_method(const Statement &statement, std::size_t depth,
                             const std::string &name) {
    if (statement.kind != Statement::Kind::MethodCall) {
        return false;
    }
    const auto &segments = statement.method_call.object.segments;
    return segments.size() == depth + 1 && segments[depth].kind == PathSegment::Kind::Field &&
           segments[depth].field == name;
}

template <class V>
void apply_vector(V &field, const std::vector<const Statement *> &statements,
                  std::size_t depth, Context &context) {
    using Element = typename IsVector<V>::value_type;

    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::MethodCall ||
            statement->method_call.object.segments.size() != depth + 1) {
            continue;
        }
        const std::string &method = statement->method_call.method;
        if (method == "clear" || method == "Clear") {
            field.clear();
        } else if (method == "Add" || method == "add") {
            if (statement->method_call.arguments.empty()) {
                ++context.stats.unsupported;
                continue;
            }
            const Expr &first = statement->method_call.arguments[0];
            if constexpr (is_model_class<Element>::value) {
                if (first.kind == Expr::Kind::Constructor) {
                    Element element{};
                    ::fable::defs::apply_model_defaults(element);
                    assign_constructor(element, first, context);
                    field.push_back(std::move(element));
                    ++context.stats.applied;
                    continue;
                }
            }
            Value value;
            if (!evaluate_leaf(&first, context, value)) {
                continue;
            }
            if constexpr (is_model_class<Element>::value) {
                Element element{};
                ::fable::defs::apply_model_defaults(element);
                Expr synthetic;
                synthetic.kind = Expr::Kind::Constructor;
                synthetic.arguments = statement->method_call.arguments;
                assign_constructor(element, synthetic, context);
                field.push_back(std::move(element));
                ++context.stats.applied;
            } else {
                Element element{};
                if (assign_value(element, value, context.stats, context.names,
                                 context.definitions)) {
                    field.push_back(std::move(element));
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            }
        }
    }

    std::size_t clear_position = 0;
    for (std::size_t i = 0; i < statements.size(); ++i) {
        const Statement *statement = statements[i];
        if (statement->kind == Statement::Kind::MethodCall &&
            statement->method_call.object.segments.size() == depth + 1 &&
            (statement->method_call.method == "clear" ||
             statement->method_call.method == "Clear")) {
            clear_position = i;
        }
    }
    std::map<std::int64_t, std::vector<const Statement *>> groups;
    for (std::size_t statement_index = 0; statement_index < statements.size();
         ++statement_index) {
        if (statement_index < clear_position) {
            continue;
        }
        const Statement *statement = statements[statement_index];
        const std::vector<PathSegment> *path = nullptr;
        if (statement->kind == Statement::Kind::Field) {
            path = &statement->field.path.segments;
        } else if (statement->kind == Statement::Kind::MethodCall) {
            path = &statement->method_call.object.segments;
        } else {
            continue;
        }
        const auto &segments = *path;
        if (segments.size() <= depth + 1 ||
            segments[depth + 1].kind != PathSegment::Kind::Index) {
            continue;
        }
        auto index = evaluate_index(segments[depth + 1].index, context);
        if (!index || *index < 0) {
            ++context.stats.unsupported;
            continue;
        }
        groups[*index].push_back(statement);
    }

    for (auto &[raw_index, group] : groups) {
        std::size_t index = static_cast<std::size_t>(raw_index);
        if (field.size() <= index) {
            field.resize(index + 1);
        }
        if (const Expr *direct = direct_value(group, depth + 2)) {
            if constexpr (is_model_class<Element>::value) {
                if (direct->kind == Expr::Kind::Constructor) {
                    assign_constructor(field[index], *direct, context);
                } else {
                    Value value;
                    if (evaluate_leaf(direct, context, value)) {
                        ++context.stats.unsupported;
                    }
                }
            } else if constexpr (std::is_same_v<Element, bool>) {
                // `std::vector<bool>` returns a proxy, so assigning through a
                // by-value element parameter would silently discard the write.
                Value value;
                if (evaluate_leaf(direct, context, value) &&
                    (value.kind == Value::Kind::Bool ||
                     value.kind == Value::Kind::Integer)) {
                    field[index] = value.kind == Value::Kind::Bool
                                       ? value.boolean
                                       : value.integer != 0;
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            } else {
                Value value;
                if (evaluate_leaf(direct, context, value) &&
                    assign_vector_element(field[index], value, context.stats, context.names, context.definitions)) {
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            }
        } else if constexpr (std::is_same_v<Element, ::fable::defs::CRGBColour>) {
            apply_rgb_colour_children(field[index], group, depth + 2, context);
        } else if constexpr (is_model_class<Element>::value) {
            apply_element_schema_defaults(field[index]);
            lower_object(field[index], group, depth + 2, context);
        } else {
            ++context.stats.unsupported;
        }
    }
}

template <class L>
void apply_list(L &field, const std::vector<const Statement *> &statements,
                std::size_t depth, Context &context) {
    using Element = typename IsList<L>::value_type;
    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::MethodCall ||
            statement->method_call.object.segments.size() != depth + 1) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.method == "clear" || call.method == "Clear") {
            field.clear();
            continue;
        }
        if ((call.method != "Add" && call.method != "add") || call.arguments.empty()) {
            continue;
        }
        Element element{};
        if constexpr (is_model_class<Element>::value) {
            ::fable::defs::apply_model_defaults(element);
            Expr synthetic;
            synthetic.kind = Expr::Kind::Constructor;
            synthetic.arguments = call.arguments;
            assign_constructor(element, synthetic, context);
        } else {
            Value value;
            if (!evaluate_leaf(&call.arguments[0], context, value) ||
                !assign_value(element, value, context.stats, context.names,
                              context.definitions)) {
                ++context.stats.unsupported;
                continue;
            }
        }
        field.push_back(std::move(element));
        ++context.stats.applied;
    }
}

template <class M>
void apply_map(M &field, const std::vector<const Statement *> &statements,
               std::size_t depth, Context &context, bool first_wins = false) {
    using Key = typename IsMap<M>::key_type;
    using Mapped = typename IsMap<M>::mapped_type;

    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::MethodCall ||
            statement->method_call.object.segments.size() != depth + 1) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.method == "clear" || call.method == "Clear") {
            field.clear();
            continue;
        }
        if ((call.method == "Add" || call.method == "add") &&
            call.arguments.size() >= 2) {
            Key key{};
            Value key_value;
            bool key_ok = false;
            if constexpr (std::is_same_v<Key, std::string> ||
                          std::is_same_v<Key, CDefString>) {
                if (call.arguments[0].kind == Expr::Kind::String ||
                    call.arguments[0].kind == Expr::Kind::Symbol) {
                    if constexpr (std::is_same_v<Key, CDefString>) {
                        key.TablePos = context.names != nullptr
                                           ? static_cast<std::int32_t>(
                                                 context.names->intern(call.arguments[0].text))
                                           : -1;
                    } else {
                        key = call.arguments[0].text;
                    }
                    key_ok = true;
                }
            } else if constexpr (std::is_integral_v<Key> || std::is_enum_v<Key>) {
                if (call.arguments[0].kind == Expr::Kind::String) {
                    key = static_cast<Key>(crc32(call.arguments[0].text));
                    key_ok = true;
                } else if (evaluate_leaf(&call.arguments[0], context, key_value) &&
                           assign_value(key, key_value, context.stats, context.names, context.definitions)) {
                    key_ok = true;
                }
            }
            if (!key_ok) {
                ++context.stats.unsupported;
                continue;
            }
            if (first_wins && field.find(key) != field.end()) {
                continue;
            }
            if constexpr (is_model_class<Mapped>::value) {
                if (call.arguments[1].kind == Expr::Kind::Constructor) {
                    apply_element_schema_defaults(field[key]);
                    assign_constructor(field[key], call.arguments[1], context);
                } else {
                    ++context.stats.unsupported;
                }
            } else {
                Mapped mapped{};
                Value mapped_value;
                if (evaluate_leaf(&call.arguments[1], context, mapped_value) &&
                    assign_value(mapped, mapped_value, context.stats, context.names, context.definitions)) {
                    field[key] = mapped;
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            }
        }
    }

    std::map<std::int64_t, std::vector<const Statement *>> groups;
    std::map<std::string, std::vector<const Statement *>> string_groups;
    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::Field) {
            continue;
        }
        const auto &segments = statement->field.path.segments;
        if (segments.size() <= depth + 1 ||
            segments[depth + 1].kind != PathSegment::Kind::Index) {
            continue;
        }
        const Expr &key_expr = segments[depth + 1].index;
        if constexpr (std::is_same_v<Key, std::string> ||
                      std::is_same_v<Key, CDefString>) {
            if (key_expr.kind == Expr::Kind::String || key_expr.kind == Expr::Kind::Symbol) {
                string_groups[key_expr.text].push_back(statement);
            } else {
                ++context.stats.unsupported;
            }
        } else {
            auto index = evaluate_index(key_expr, context);
            if (!index) {
                ++context.stats.unsupported;
                continue;
            }
            groups[*index].push_back(statement);
        }
    }

    if constexpr (std::is_same_v<Key, std::string> ||
                  std::is_same_v<Key, CDefString>) {
        for (auto &[key_text, group] : string_groups) {
            Key key{};
            if constexpr (std::is_same_v<Key, CDefString>) {
                key.TablePos = context.names != nullptr
                                   ? static_cast<std::int32_t>(context.names->intern(key_text))
                                   : -1;
            } else {
                key = key_text;
            }
            if (const Expr *direct = direct_value(group, depth + 2)) {
                if constexpr (is_model_class<Mapped>::value) {
                    if (direct->kind == Expr::Kind::Constructor) {
                        apply_element_schema_defaults(field[key]);
                        assign_constructor(field[key], *direct, context);
                    } else {
                        ++context.stats.unsupported;
                    }
                } else {
                    Value value;
                    if (evaluate_leaf(direct, context, value) &&
                        assign_value(field[key], value, context.stats, context.names, context.definitions)) {
                        ++context.stats.applied;
                    } else {
                        ++context.stats.unsupported;
                    }
                }
            } else if constexpr (is_model_class<Mapped>::value) {
                apply_element_schema_defaults(field[key]);
                lower_object(field[key], group, depth + 2, context);
            } else {
                ++context.stats.unsupported;
            }
        }
    } else if constexpr (std::is_integral_v<Key> || std::is_enum_v<Key>) {
        for (auto &[raw_key, group] : groups) {
            Key key = static_cast<Key>(raw_key);
            if (const Expr *direct = direct_value(group, depth + 2)) {
                if constexpr (is_model_class<Mapped>::value) {
                    if (direct->kind == Expr::Kind::Constructor) {
                        ::fable::defs::apply_model_defaults(field[key]);
                        assign_constructor(field[key], *direct, context);
                    } else {
                        ++context.stats.unsupported;
                    }
                } else {
                    Value value;
                    if (evaluate_leaf(direct, context, value) &&
                        assign_value(field[key], value, context.stats, context.names, context.definitions)) {
                        ++context.stats.applied;
                    } else {
                        ++context.stats.unsupported;
                    }
                }
            } else if constexpr (is_model_class<Mapped>::value) {
                apply_element_schema_defaults(field[key]);
                lower_object(field[key], group, depth + 2, context);
            } else {
                ++context.stats.unsupported;
            }
        }
    } else {
        if (!groups.empty()) {
            context.stats.unsupported += groups.size();
        }
    }
}

template <class M>
void apply_multimap(M &field, const std::vector<const Statement *> &statements,
                    std::size_t depth, Context &context) {
    using Key = typename IsMultiMap<M>::key_type;
    using Mapped = typename IsMultiMap<M>::mapped_type;
    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::Field) {
            continue;
        }
        const auto &segments = statement->field.path.segments;
        if (segments.size() <= depth + 1 ||
            segments[depth + 1].kind != PathSegment::Kind::Index) {
            continue;
        }
        const Expr &key_expr = segments[depth + 1].index;
        Key key{};
        bool key_ok = false;
        if constexpr (std::is_same_v<Key, std::string>) {
            if (key_expr.kind == Expr::Kind::String || key_expr.kind == Expr::Kind::Symbol) {
                key = key_expr.text;
                key_ok = true;
            }
        } else {
            Value key_value;
            if (evaluate_leaf(&key_expr, context, key_value) &&
                assign_value(key, key_value, context.stats, context.names,
                             context.definitions)) {
                key_ok = true;
            }
        }
        if (!key_ok) {
            ++context.stats.unsupported;
            continue;
        }
        const Expr *direct = direct_value(segments.size() > depth + 1
                                              ? std::vector<const Statement *>{statement}
                                              : std::vector<const Statement *>{},
                                          depth + 2);
        if (direct != nullptr) {
            if constexpr (is_model_class<Mapped>::value) {
                if (direct->kind == Expr::Kind::Constructor) {
                    Mapped mapped{};
                    apply_element_schema_defaults(mapped);
                    assign_constructor(mapped, *direct, context);
                    field.emplace(key, mapped);
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            } else {
                Mapped mapped{};
                Value mapped_value;
                if (evaluate_leaf(direct, context, mapped_value) &&
                    assign_value(mapped, mapped_value, context.stats, context.names,
                                 context.definitions)) {
                    field.emplace(key, mapped);
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            }
        }
    }
}

template <class S>
void apply_set(S &field, const std::vector<const Statement *> &statements,
               std::size_t depth, Context &context) {
    using Element = typename IsSet<S>::value_type;
    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::MethodCall ||
            statement->method_call.object.segments.size() != depth + 1) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.method == "clear" || call.method == "Clear") {
            field.clear();
            continue;
        }
        if ((call.method != "Add" && call.method != "add") || call.arguments.empty()) {
            continue;
        }
        Element element{};
        Value value;
        if (evaluate_leaf(&call.arguments[0], context, value) &&
            assign_value(element, value, context.stats, context.names,
                         context.definitions)) {
            field.insert(std::move(element));
            ++context.stats.applied;
        } else {
            ++context.stats.unsupported;
        }
    }
}

template <class A>
void apply_array(A &field, const std::vector<const Statement *> &statements,
                 std::size_t depth, Context &context) {
    using Element = std::remove_extent_t<A>;
    std::map<std::int64_t, std::vector<const Statement *>> groups;
    for (const Statement *statement : statements) {
        if (statement->kind != Statement::Kind::Field) {
            continue;
        }
        const auto &segments = statement->field.path.segments;
        if (segments.size() <= depth + 1 ||
            segments[depth + 1].kind != PathSegment::Kind::Index) {
            continue;
        }
        auto index = evaluate_index(segments[depth + 1].index, context);
        if (!index || *index < 0 ||
            static_cast<std::size_t>(*index) >= std::extent_v<A>) {
            ++context.stats.unsupported;
            continue;
        }
        groups[*index].push_back(statement);
    }
    for (auto &[raw_index, group] : groups) {
        std::size_t index = static_cast<std::size_t>(raw_index);
        if (const Expr *direct = direct_value(group, depth + 2)) {
            if constexpr (is_model_class<Element>::value) {
                if (direct->kind == Expr::Kind::Constructor) {
                    ::fable::defs::apply_model_defaults(field[index]);
                    assign_constructor(field[index], *direct, context);
                } else {
                    ++context.stats.unsupported;
                }
            } else {
                Value value;
                if (evaluate_leaf(direct, context, value) &&
                    assign_vector_element(field[index], value, context.stats, context.names, context.definitions)) {
                    ++context.stats.applied;
                } else {
                    ++context.stats.unsupported;
                }
            }
        } else if constexpr (is_model_class<Element>::value) {
            apply_element_schema_defaults(field[index]);
            lower_object(field[index], group, depth + 2, context);
        } else {
            ++context.stats.unsupported;
        }
    }
}

inline void apply_rgb_colour_children(::fable::defs::CRGBColour &field,
                                        const std::vector<const Statement *> &child,
                                        std::size_t channel_depth, Context &context) {
    std::uint32_t packed = field.IntValue;
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::Field) {
            continue;
        }
        const auto &segments = statement->field.path.segments;
        if (segments.size() != channel_depth + 1 ||
            segments[channel_depth].kind != PathSegment::Kind::Field) {
            continue;
        }
        const std::string &name = segments[channel_depth].field;
        Value value;
        if (!evaluate_leaf(&statement->field.expr, context, value)) {
            continue;
        }
        const std::uint32_t byte =
            value.kind == Value::Kind::Float
                ? static_cast<std::uint32_t>(value.floating) & 0xFFu
                : static_cast<std::uint32_t>(value.integer) & 0xFFu;
        std::uint32_t shift = 0;
        if (name == "R") {
            shift = 16;
        } else if (name == "G") {
            shift = 8;
        } else if (name == "B") {
            shift = 0;
        } else if (name == "A") {
            shift = 24;
        } else {
            continue;
        }
        packed = (packed & ~(0xFFu << shift)) | (byte << shift);
    }
    field.IntValue = packed;
}

inline void apply_personality_traits(::fable::defs::COpinionPersonalityTraits &field,
                                     const std::vector<const Statement *> &child,
                                     std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "PersonalityTraits" ||
            call.method != "Set" || call.arguments.empty()) {
            continue;
        }
        Value trait_value;
        if (!evaluate_leaf(&call.arguments[0], context, trait_value)) {
            continue;
        }
        std::int64_t trait = trait_value.integer;
        if (trait < 0 || trait >= 5) {
            continue;
        }
        for (std::size_t i = 1; i < call.arguments.size() && i <= 9; ++i) {
            Value value;
            if (!evaluate_leaf(&call.arguments[i], context, value)) {
                continue;
            }
            const float f = value.kind == Value::Kind::Float
                                ? static_cast<float>(value.floating)
                                : static_cast<float>(value.integer);
            std::uint32_t bits = 0;
            std::memcpy(&bits, &f, sizeof(bits));
            const std::size_t offset =
                static_cast<std::size_t>(trait) * 36 + (i - 1) * 4;
            for (std::size_t b = 0; b < 4; ++b) {
                field.Data[offset + b] =
                    static_cast<std::uint8_t>(bits >> (b * 8));
            }
        }
        ++context.stats.applied;
    }
}

inline void apply_wound_morphs(::fable::defs::CWoundMorphs &field,
                                const std::vector<const Statement *> &child,
                                std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "WoundMorphs") {
            continue;
        }
        const auto eval = [&](std::size_t index, bool floating) -> Value {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            (void)floating;
            return value;
        };
        const auto as_int = [](const Value &v) -> std::int32_t {
            return v.kind == Value::Kind::Float ? static_cast<std::int32_t>(v.floating)
                                                : static_cast<std::int32_t>(v.integer);
        };
        const auto as_float = [](const Value &v) -> float {
            return v.kind == Value::Kind::Float ? static_cast<float>(v.floating)
                                                : static_cast<float>(v.integer);
        };
        ::fable::defs::CWoundMorphs::CEntry entry;
        if (call.method == "AddWound" && call.arguments.size() >= 8) {
            entry.Location = static_cast<EBodyLocation>(as_int(eval(0, false)));
            entry.ReplacingTextureIndex = as_int(eval(1, false));
            entry.ReplacingBumpMapIndex = as_int(eval(2, false));
            entry.WoundTextureIndex = as_int(eval(3, false));
            entry.WoundBumpMapIndex = as_int(eval(4, false));
            entry.ScarTextureIndex = as_int(eval(5, false));
            entry.ScarBumpMapIndex = as_int(eval(6, false));
            entry.FadeToScarDuration = 0;
            entry.TotalDuration = as_int(eval(7, false));
            entry.PermanentScarStrength = 0.0f;
        } else if (call.method == "AddWoundAndScar" && call.arguments.size() >= 10) {
            entry.Location = static_cast<EBodyLocation>(as_int(eval(0, false)));
            entry.ReplacingTextureIndex = as_int(eval(1, false));
            entry.ReplacingBumpMapIndex = as_int(eval(2, false));
            entry.WoundTextureIndex = as_int(eval(3, false));
            entry.WoundBumpMapIndex = as_int(eval(4, false));
            entry.ScarTextureIndex = as_int(eval(5, false));
            entry.ScarBumpMapIndex = as_int(eval(6, false));
            entry.FadeToScarDuration = as_int(eval(7, false));
            entry.TotalDuration = as_int(eval(8, false));
            entry.PermanentScarStrength = as_float(eval(9, true));
        } else {
            continue;
        }
        field.Morphs.emplace_back(entry.Location, entry);
        ++context.stats.applied;
    }
}

inline void apply_expression_set(::fable::defs::CExpressionSet &field,
                                 const std::vector<const Statement *> &child,
                                 std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "Expressions" ||
            call.method != "Add" || call.arguments.size() < 2) {
            continue;
        }
        Value type_value;
        Value anim_value;
        if (!evaluate_leaf(&call.arguments[0], context, type_value) ||
            !evaluate_leaf(&call.arguments[1], context, anim_value)) {
            continue;
        }
        ::fable::defs::CExpressionSet::CEntry entry;
        entry.Type = static_cast<EExpressionType>(type_value.integer);
        entry.AnimIndex = static_cast<std::int32_t>(anim_value.integer);
        field.Expressions.emplace_back(entry.Type, entry);
        ++context.stats.applied;
    }
    std::sort(field.Expressions.begin(), field.Expressions.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
}

inline void apply_texture_morphs(::fable::defs::CTextureMorphs &field,
                                 const std::vector<const Statement *> &child,
                                 std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "TextureMorphs" ||
            (call.method != "Add" && call.method != "add") ||
            call.arguments.size() < 5) {
            continue;
        }
        const auto eval_i32 = [&](std::size_t index) -> std::int32_t {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        ::fable::defs::CTextureMorphEntry entry;
        entry.MorphType = static_cast<EHeroMorphType>(eval_i32(1));
        entry.TextureLayer = eval_i32(0);
        entry.ReplacingTextureIndex = eval_i32(2);
        entry.BankIndex = eval_i32(3);
        entry.Blend = static_cast<ECompositeBlendType>(eval_i32(4));
        field.Morphs.emplace_back(entry.MorphType, entry);
        ++context.stats.applied;
    }
}

inline void apply_replaceable_meshes(::fable::defs::NGraphicAppearance::CReplaceableMeshes &field,
                                      const std::vector<const Statement *> &child,
                                      std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "Meshes" ||
            (call.method != "Add" && call.method != "add") ||
            call.arguments.size() < 2) {
            continue;
        }
        Value type;
        Value bank;
        evaluate_leaf(&call.arguments[0], context, type);
        evaluate_leaf(&call.arguments[1], context, bank);
        ::fable::defs::NGraphicAppearance::CReplaceableMeshEntry graphic;
        graphic.GraphicType = static_cast<std::uint8_t>(type.integer);
        graphic.BankIndex = static_cast<std::int32_t>(bank.integer);
        graphic.AnimStep = 1.0f;
        graphic.RenderSizeX = 1.0f;
        field.Vector.push_back(graphic);
        ++context.stats.applied;
    }
}

inline void apply_appearance_modifier_graphics(
    ::fable::defs::CAppearanceModifierGraphics &field,
    const std::vector<const Statement *> &child, std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "Graphics" ||
            (call.method != "Add" && call.method != "add") ||
            call.arguments.size() < 6) {
            continue;
        }
        const auto eval_i32 = [&](std::size_t index) -> std::int32_t {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        const auto eval_f32 = [&](std::size_t index) -> float {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<float>(value.floating)
                       : static_cast<float>(value.integer);
        };
        ::fable::defs::CAppearanceModifierGraphics::CEntry entry;
        entry.Type = static_cast<EEngineGraphicType>(eval_i32(0));
        entry.MorphType = static_cast<EHeroMorphType>(eval_i32(3));
        entry.BankIndex = eval_i32(1);
        entry.RenderSizeX = eval_f32(2);
        entry.StartMorphStrength = eval_f32(4);
        entry.EndMorphStrength = eval_f32(5);
        field.Graphics.push_back(entry);
        ++context.stats.applied;
    }
}

inline void apply_random_appearance_morph(
    ::fable::defs::CRandomAppearanceMorph &field,
    const std::vector<const Statement *> &child, std::size_t depth, Context &context) {
    struct TexCall {
        std::int32_t mesh = 0;
        std::int32_t n = 0;
        std::int32_t original = 0;
        std::vector<std::int32_t> replacements;
    };
    std::vector<TexCall> calls;
    std::vector<TexCall> all_calls;
    std::int32_t max_n = 0;
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "RandomAppearanceMorph") {
            continue;
        }
        const auto eval_i32 = [&](std::size_t index) -> std::int32_t {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        const auto eval_bool = [&](std::size_t index) -> bool {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            if (value.kind == Value::Kind::Bool) {
                return value.boolean;
            }
            if (value.kind == Value::Kind::Integer) {
                return value.integer != 0;
            }
            return false;
        };
        if (call.method == "AddBodyPart" && call.arguments.size() >= 3) {
            const std::int32_t body_part = eval_i32(0);
            const std::int32_t priority = eval_i32(1);
            for (std::size_t i = 2; i < call.arguments.size(); ++i) {
                const std::int32_t mesh = eval_i32(i);
                if (body_part == 0) {
                    ::fable::defs::CRandomAppearanceMorph::CBodyPart0Mesh entry;
                    entry.MeshId = mesh;
                    entry.TextureId = priority;
                    field.BodyParts0.Meshes.push_back(entry);
                } else if (body_part == 1) {
                    ::fable::defs::CRandomAppearanceMorph::CBodyPart1Mesh entry;
                    entry.MeshId = mesh;
                    entry.TextureId = priority;
                    field.BodyParts1.Meshes.push_back(entry);
                } else if (body_part == 2) {
                    ::fable::defs::CRandomAppearanceMorph::CBodyPart2Mesh entry;
                    entry.MeshId = mesh;
                    entry.TextureId = priority;
                    field.BodyParts2.Meshes.push_back(entry);
                }
            }
            ++context.stats.applied;
        } else if (call.method == "AddTextureToMesh" && call.arguments.size() >= 4) {
            TexCall item;
            item.mesh = eval_i32(0);
            item.n = eval_i32(1);
            max_n = std::max(max_n, item.n);
            item.original = eval_i32(2);
            for (std::size_t i = 3; i < call.arguments.size(); ++i) {
                item.replacements.push_back(eval_i32(i));
            }
            calls.push_back(std::move(item));
            ++context.stats.applied;
        } else if (call.method == "AddSkeletalMorph") {
            // (key, "bone_or_morph_file.bncfg", allow_variation)
            const std::int32_t key = eval_i32(0);
            std::string filename;
            if (call.arguments.size() > 1 &&
                (call.arguments[1].kind == Expr::Kind::String ||
                 call.arguments[1].kind == Expr::Kind::Symbol)) {
                filename = call.arguments[1].text;
            }
            ::fable::defs::CRandomAppearanceMorph::CSkeletalMorphEntry entry;
            entry.Key = key;
            entry.MorphId = key;
            entry.BoneOrMorphIndex = context.names != nullptr
                                         ? static_cast<std::int32_t>(
                                               context.names->intern(filename))
                                         : -1;
            entry.AllowVariation = eval_bool(2);
            field.SkeletalMorphs.push_back(entry);
            ++context.stats.applied;
        } else if (call.method == "AddTextureToAll") {
            // (original, replace0[, replace1…]) — same grouping as a mesh,
            // but stored in GeneralTextureMorphs.
            TexCall item;
            item.mesh = -1;
            item.n = 0;
            item.original = eval_i32(0);
            for (std::size_t i = 1; i < call.arguments.size(); ++i) {
                item.replacements.push_back(eval_i32(i));
            }
            all_calls.push_back(std::move(item));
            ++context.stats.applied;
        }
    }
    const auto build = [&](const std::vector<TexCall> &source, std::int32_t mesh) {
        std::vector<std::pair<std::uint32_t, ::fable::defs::CRandomAppearanceMorph::CTextureMorphEntry>> out;
        struct Item {
            std::int32_t original;
            std::int32_t replacement;
            std::int32_t n;
        };
        std::vector<Item> items;
        for (const TexCall &call : source) {
            if (call.mesh != mesh) {
                continue;
            }
            for (std::int32_t replacement : call.replacements) {
                items.push_back(Item{call.original, replacement, call.n});
            }
        }
        if (items.empty()) {
            return out;
        }
        const std::size_t total = items.size();
        // Groups of `original` in appearance order: preserve the authored
        // statement order rather than sorting by texture id.
        std::vector<std::int32_t> origins;
        for (const Item &item : items) {
            if (std::find(origins.begin(), origins.end(), item.original) == origins.end()) {
                origins.push_back(item.original);
            }
        }
        std::size_t previous_size = 0;
        bool previous_any = false;
        for (std::int32_t origin : origins) {
            std::vector<const Item *> group;
            for (const Item &item : items) {
                if (item.original == origin) {
                    group.push_back(&item);
                }
            }
            bool any_n = false;
            for (const Item *item : group) {
                any_n = any_n || item->n >= 1;
            }
            for (std::size_t pos = 0; pos < group.size(); ++pos) {
                std::uint32_t key = 0;
                if (out.empty()) {
                    key = static_cast<std::uint32_t>(total * 12);
                } else if (pos == 0) {
                    key = previous_any ? static_cast<std::uint32_t>(previous_size) : 0;
                } else {
                    key = static_cast<std::uint32_t>(std::max<std::int32_t>(group[pos]->n - 1, 0));
                }
                ::fable::defs::CRandomAppearanceMorph::CTextureMorphEntry morph;
                std::memcpy(morph.Morph, &group[pos]->original, 4);
                std::memcpy(morph.Morph + 4, &group[pos]->replacement, 4);
                out.emplace_back(key, morph);
            }
            previous_size = group.size();
            previous_any = any_n;
        }
        return out;
    };
    const auto max_group = [&](std::int32_t mesh) -> std::uint32_t {
        std::int32_t n = 0;
        for (const TexCall &call : calls) {
            if (call.mesh == mesh) {
                n = std::max(n, call.n);
            }
        }
        return static_cast<std::uint32_t>(std::max<std::int32_t>(n, 0));
    };
    for (auto &mesh : field.BodyParts0.Meshes) {
        auto entries = build(calls, mesh.MeshId);
        mesh.TextureMorphs.Entries = std::move(entries);
        mesh.TextureMorphs.Trailing = max_group(mesh.MeshId);
    }
    for (auto &mesh : field.BodyParts1.Meshes) {
        auto entries = build(calls, mesh.MeshId);
        mesh.TextureMorphs.Entries = std::move(entries);
        mesh.TextureMorphs.Trailing = max_group(mesh.MeshId);
    }
    for (auto &mesh : field.BodyParts2.Meshes) {
        auto entries = build(calls, mesh.MeshId);
        mesh.TextureMorphs.Entries = std::move(entries);
        mesh.TextureMorphs.Trailing = max_group(mesh.MeshId);
    }
    if (!all_calls.empty()) {
        field.GeneralTextureMorphs.Entries = build(all_calls, -1);
        field.GeneralTextureMorphs.Trailing = 0;
        field.GeneralTextureMorphs.Dirty = 0;
    }
    field.FinalTrailing = static_cast<std::uint32_t>(std::max<std::int32_t>(max_n, 0));
}

inline void apply_skeletal_morphs(::fable::defs::CSkeletalMorphs &field,
                                  const std::vector<const Statement *> &child,
                                  std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "SkeletalMorphs" ||
            (call.method != "Add" && call.method != "add") ||
            call.arguments.size() < 2) {
            continue;
        }
        const auto eval_i32 = [&](std::size_t index) -> std::int32_t {
            Value value;
            if (index < call.arguments.size()) {
                evaluate_leaf(&call.arguments[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        const std::uint32_t key = static_cast<std::uint32_t>(eval_i32(0));
        ::fable::defs::CSkeletalMorphs::CEntry entry;
        if (call.arguments.size() > 1 && call.arguments[1].kind == Expr::Kind::String) {
            entry.MorphName.TablePos = context.names != nullptr
                                           ? static_cast<std::int32_t>(
                                                 context.names->intern(call.arguments[1].text))
                                           : -1;
        }
        entry.Priority = static_cast<std::int16_t>(eval_i32(2));
        entry.SupressesLowerMorphs = call.arguments.size() > 3 && eval_i32(3) != 0;
        field.Morphs.emplace_back(key, entry);
        ++context.stats.applied;
    }
}

inline void apply_particle_morph_names(::fable::defs::CParticleMorphs &,
                                       const std::vector<const Statement *> &child,
                                       std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            (call.object.segments[0].field != "ParticleMorphs" &&
             call.object.segments[0].field != "IdleParticleMorphs")) {
            continue;
        }
        // The location-name argument is a names.bin string even though the
        // particle entry itself is not modeled yet.
        const std::size_t index = call.method == "AddBlend" ? 6 : 5;
        if (context.names != nullptr && call.arguments.size() > index &&
            call.arguments[index].kind == Expr::Kind::String) {
            context.names->intern(call.arguments[index].text);
        }
    }
}

inline void apply_reaction_matches(::fable::defs::CReactionMatchList &field,
                                    const std::vector<const Statement *> &child,
                                    std::size_t depth, Context &context) {
    for (const Statement *statement : child) {
        if (statement->kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement->method_call;
        if (call.object.segments.size() != depth + 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "Matches" ||
            (call.method != "Add" && call.method != "add") ||
            call.arguments.size() < 2 ||
            call.arguments[1].kind != Expr::Kind::Constructor) {
            continue;
        }
        const Expr &ctor = call.arguments[1];
        const auto eval_i32 = [&](const std::vector<Expr> &args,
                                  std::size_t index) -> std::int32_t {
            Value value;
            if (index < args.size()) {
                evaluate_leaf(&args[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<std::int32_t>(value.floating)
                       : static_cast<std::int32_t>(value.integer);
        };
        const auto eval_f32 = [&](const std::vector<Expr> &args,
                                  std::size_t index) -> float {
            Value value;
            if (index < args.size()) {
                evaluate_leaf(&args[index], context, value);
            }
            return value.kind == Value::Kind::Float
                       ? static_cast<float>(value.floating)
                       : static_cast<float>(value.integer);
        };
        Value reaction;
        evaluate_leaf(&call.arguments[0], context, reaction);
        ::fable::defs::CReactionMatchEntry entry;
        entry.ReactionType = static_cast<std::int32_t>(reaction.integer);
        const auto &a = ctor.arguments;
        if (ctor.text == "Secondary") {
            entry.Tag = 0;
            entry.Axis = eval_i32(a, 0);
            const double lower = eval_f32(a, 1);
            const double upper = eval_f32(a, 2);
            entry.LowerBoundOnAxis = static_cast<float>(lower);
            entry.InvIntervalOnAxis = static_cast<float>(1.0 / (upper - lower));
            entry.MShiftZero = eval_f32(a, 3);
            entry.MShiftWeight = eval_f32(a, 4);
            entry.RShiftZero = eval_f32(a, 5);
            entry.RShiftWeight = eval_f32(a, 6);
        } else if (ctor.text == "SecondaryCentred") {
            entry.Tag = 1;
            entry.Axis = eval_i32(a, 0);
            entry.CentreOnAxis = eval_f32(a, 1);
            entry.RadiusOnAxis = eval_f32(a, 2);
            entry.MShiftZero = eval_f32(a, 3);
            entry.MShiftWeight = eval_f32(a, 4);
            entry.RShiftZero = eval_f32(a, 5);
            entry.RShiftWeight = eval_f32(a, 6);
        } else if (ctor.text == "Pyramid") {
            entry.Tag = 2;
            entry.XAxisOpinionType = eval_i32(a, 0);
            const double x_min = eval_f32(a, 1);
            const double x_centre = eval_f32(a, 2);
            const double x_max = eval_f32(a, 3);
            entry.YAxisOpinionType = eval_i32(a, 4);
            const double y_min = eval_f32(a, 5);
            const double y_centre = eval_f32(a, 6);
            const double y_max = eval_f32(a, 7);
            entry.XCentre = static_cast<float>(x_centre);
            entry.NegInvXRadius = static_cast<float>(1.0 / (x_centre - x_min));
            entry.PosInvXRadius = static_cast<float>(1.0 / (x_max - x_centre));
            entry.YCentre = static_cast<float>(y_centre);
            entry.NegInvYRadius = static_cast<float>(1.0 / (y_centre - y_min));
            entry.PosInvYRadius = static_cast<float>(1.0 / (y_max - y_centre));
        } else if (ctor.text == "MRBlob") {
            entry.Tag = 3;
            const double renown_min = eval_f32(a, 0);
            const double renown_max = eval_f32(a, 1);
            const double morality_min = eval_f32(a, 2);
            const double morality_max = eval_f32(a, 3);
            const double x_centre = morality_min + (morality_max - morality_min) * 0.5;
            const double y_centre = renown_min + (renown_max - renown_min) * 0.5;
            entry.XAxisOpinionType = 0;
            entry.YAxisOpinionType = 1;
            entry.XCentre = static_cast<float>(x_centre);
            entry.NegInvXRadius = static_cast<float>(1.0 / (x_centre - morality_min));
            entry.PosInvXRadius = static_cast<float>(1.0 / (morality_max - x_centre));
            entry.YCentre = static_cast<float>(y_centre);
            entry.NegInvYRadius = static_cast<float>(1.0 / (y_centre - renown_min));
            entry.PosInvYRadius = static_cast<float>(1.0 / (renown_max - y_centre));
            entry.ScarinessShift = eval_f32(a, 4);
            entry.AgreeablenessShift = eval_f32(a, 5);
            entry.AttractivenessShift = eval_f32(a, 6);
        } else {
            continue;
        }
        field.Elements.push_back(entry);
        ++context.stats.applied;
    }
}

// ── object visitor ────────────────────────────────────────────────────────

class Visitor {
public:
    Visitor(const std::vector<const Statement *> &statements, std::size_t depth,
            Context &context)
        : statements_(statements), depth_(depth), context_(context) {}

    template <class U>
    void operator()(const char *name, U &field) {
        std::vector<const Statement *> child = children(statements_, depth_, name);
        bool handled = false;
        if (const Expr *leaf = leaf_value(child, depth_)) {
            if constexpr (is_model_class<U>::value) {
                if (leaf->kind == Expr::Kind::Constructor) {
                    assign_constructor(field, *leaf, context_);
                    record(context_, name);
                    handled = true;
                }
            }
            if (!handled) {
                Value value;
                if (evaluate_leaf(leaf, context_, value)) {
                    if (assign_value(field, value, context_.stats, context_.names, context_.definitions)) {
                        record(context_, name);
                    } else {
                        ++context_.stats.unsupported;
                    }
                }
                handled = true;
            }
        }

        if constexpr (IsVector<U>::value) {
            if (has_method_work(child, depth_) || has_index_work(child, depth_)) {
                apply_vector(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (IsList<U>::value) {
            if (has_method_work(child, depth_)) {
                apply_list(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (IsSet<U>::value) {
            if (has_method_work(child, depth_)) {
                apply_set(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (IsMap<U>::value) {
            if (has_method_work(child, depth_) || has_index_work(child, depth_)) {
                apply_map(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (IsMultiMap<U>::value) {
            if (has_method_work(child, depth_) || has_index_work(child, depth_)) {
                apply_multimap(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_array_v<U>) {
            if (has_index_work(child, depth_)) {
                apply_array(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CRandomAppearanceMorph>) {
            if (has_method_work(child, depth_)) {
                apply_random_appearance_morph(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CReactionMatchList>) {
            if (has_method_work(child, depth_)) {
                apply_reaction_matches(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CAppearanceModifierGraphics>) {
            if (has_method_work(child, depth_)) {
                apply_appearance_modifier_graphics(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::NGraphicAppearance::CReplaceableMeshes>) {
            if (has_method_work(child, depth_)) {
                apply_replaceable_meshes(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CSkeletalMorphs>) {
            if (has_method_work(child, depth_)) {
                apply_skeletal_morphs(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CParticleMorphs>) {
            if (has_method_work(child, depth_)) {
                apply_particle_morph_names(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CTextureMorphs>) {
            if (has_method_work(child, depth_)) {
                apply_texture_morphs(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CWoundMorphs>) {
            if (has_method_work(child, depth_)) {
                apply_wound_morphs(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CExpressionSet>) {
            if (has_method_work(child, depth_)) {
                apply_expression_set(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CRGBColour>) {
            if (has_deeper(child, depth_)) {
                apply_rgb_colour_children(field, child, depth_ + 1, context_);
                record(context_, name);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::COpinionPersonalityTraits>) {
            if (has_method_work(child, depth_)) {
                apply_personality_traits(field, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, ::fable::defs::CFireEffectCreationSet>) {
            if (has_method_work(child, depth_)) {
                apply_map(field.ContainmentMap, child, depth_, context_);
                handled = true;
            }
        } else if constexpr (std::is_same_v<U, CSpecialEffectsStringMap>) {
            if (has_method_work(child, depth_)) {
                apply_map(field.ContainmentMap, child, depth_, context_, true);
                handled = true;
            }
        } else if constexpr (is_model_class<U>::value) {
            ::fable::defs::apply_model_defaults(field);
            if (has_deeper(child, depth_)) {
                lower_object(field, child, depth_ + 1, context_);
                handled = true;
            }
        }

        if (child.empty() && !handled) {
            if (depth_ == 0) {
                ++context_.stats.absent;
            }
        } else if (!handled) {
            ++context_.stats.unsupported;
        }
    }

    template <class U>
    void operator()(const char *name, ::fable::defs::detail::BitfieldRef<U> ref) {
        std::vector<const Statement *> child = children(statements_, depth_, name);
        const Expr *leaf = leaf_value(child, depth_);
        if (leaf == nullptr) {
            if (child.empty()) {
                if (depth_ == 0) {
                    ++context_.stats.absent;
                }
            } else {
                ++context_.stats.unsupported;
            }
            return;
        }
        Value value;
        if (!evaluate_leaf(leaf, context_, value)) {
            return;
        }
        bool result = false;
        if (value.kind == Value::Kind::Bool) {
            result = value.boolean;
        } else if (value.kind == Value::Kind::Integer) {
            result = value.integer != 0;
        } else {
            ++context_.stats.unsupported;
            return;
        }
        ref.set(result);
        record(context_, name);
    }

private:
    const std::vector<const Statement *> &statements_;
    std::size_t depth_;
    Context &context_;
};

template <class T>
void lower_object(T &object, const std::vector<const Statement *> &statements,
                  std::size_t depth, Context &context) {
    Visitor visitor(statements, depth, context);
    ::fable::defs::visit_model_fields(object, visitor);
}

} // namespace detail

/// Lower a definition body into `out`. Only fields present in the body are
/// touched; everything else keeps its default-constructed value.
/// Apply the transfer defaults plus schema-level seeds that cannot be
/// expressed as a plain Transfer default. The UI font is the canonical case:
/// `Transfer` sees `this->Font`, but the real default is the interned
/// `ENG_ARIAL_16` string.
template <class T>
void apply_schema_defaults(T &value, NamesBuilder *names) {
    ::fable::defs::apply_model_defaults(value);
    if constexpr (std::is_same_v<T, ::fable::defs::NUISystem::CUIDef>) {
        if (names != nullptr) {
            value.Font.TablePos =
                static_cast<std::int32_t>(names->intern("ENG_ARIAL_16"));
        }
        for (auto &state : value.States) {
            if (state.StateChangeFlag == 0) {
                state.StateChangeFlag = 7;
            }
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CThingObjectDef>) {
        value.Health = 1.0f;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CWeaponDef>) {
        value.AnimationGroupName = "SWORD";
        value.ProjectileWeaponHelperNameTip = "weapon_pos_a";
        value.ProjectileWeaponHelperNameBase = "weapon_pos_b";
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CThingShotDef>) {
        value.ExplosionDef.Val = -1;
        value.GroundExplosionDef.Val = -1;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CEnvironmentThemeDef>) {
        value.WaterRefractionBlendEnd = 15.0f;
        value.WaterFlowSpeedFactor = 0.25f;
        value.WaterOscilationSpeed = 0.05f;
        value.WaterSwellFactor = 0.0625f;
        value.WaterShoreSwellFactor = 0.041666668f;
        value.WaterReflectionOffset = 0.01f;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CVillagePeopleDef>) {
        // The wire has two consecutive `MaleTeacher` controls (the second
        // backed by the PDB's FemaleTeacher field).  The text's single
        // `MaleTeacher` applies to the first; the duplicate must stay unset.
        if (value.MaleTeacher != 0) {
            value.FemaleTeacher = 0;
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CScriptDef>) {
        // Retail's script def deliberately serializes this field twice but
        // leaves both copies at zero; the text value is not applied.
        value.TemplePrayerFactorHighest = 0;
        value.TemplePrayerFactorHighest2 = 0;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CDegradableDef>) {
        std::sort(value.Degradations.begin(), value.Degradations.end(),
                  [](const auto &a, const auto &b) {
                      return a.HealthPercentage > b.HealthPercentage;
                  });
        for (auto &degradation : value.Degradations) {
            degradation.Type = static_cast<std::uint8_t>(value.GraphicType);
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::COpinionSourceDef>) {
        if (value.OpinionProduced.OpinionValues.size() < 5) {
            value.OpinionProduced.OpinionValues.resize(5, 0.0f);
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::COpinionPersonalityDef>) {
        std::fill(std::begin(value.PersonalityTraits.Data),
                  std::end(value.PersonalityTraits.Data), std::uint8_t{0});
        const std::uint32_t one = 0x3f800000u;
        for (std::size_t trait = 0; trait < 5; ++trait) {
            for (std::size_t field = 0; field < 8; ++field) {
                const std::size_t offset = trait * 36 + 4 + field * 4;
                for (std::size_t i = 0; i < 4; ++i) {
                    value.PersonalityTraits.Data[offset + i] =
                        static_cast<std::uint8_t>(one >> (i * 8));
                }
            }
        }
    }
}

/// `Components.Add("X"[, override])` / `Components.Remove("X")` replay for
/// the Thing family. The set is rebuilt from the merged body, matching the
/// compiler's copy-parent-then-apply semantics.
inline std::uint32_t thing_component_driver_type(std::string_view name) {
    struct Driver {
        std::string_view name;
        std::uint32_t value;
    };
    static constexpr Driver drivers[] = {
        {"CTCDInternalMarker", 12},      {"CTCDParticleEmitter", 19},
        {"CTCDCameraPoint", 20},         {"CTCDNavigationSeed", 21},
        {"CTCDHeroSuit", 22},            {"CTCDNone", 23},
        {"CTCDRegionEntrance", 24},      {"CTCDRegionExit", 25},
        {"CTCDExperienceOrb", 26},       {"CTCDExplosion", 27},
        {"CTCDRumble", 28},              {"CTCDPhysicalObstruction", 29},
        {"CTCDExplosiveTrail", 30},
    };
    for (const Driver &driver : drivers) {
        if (driver.name == name) {
            return driver.value;
        }
    }
    return 0;
}

template <class T>
void build_thing_components(T &out, const Definition &definition, NamesBuilder *names) {
    if constexpr (std::is_base_of_v<::fable::defs::CThingBaseDef, T>) {
        if (names == nullptr) {
            return;
        }
        struct Component {
            std::string name;
            bool override_flag = false;
        };
        std::vector<Component> components;
        for (const Statement &statement : definition.body) {
            if (statement.kind != Statement::Kind::MethodCall) {
                continue;
            }
            const MethodCall &call = statement.method_call;
            if (call.object.segments.size() != 1 ||
                call.object.segments[0].kind != PathSegment::Kind::Field ||
                call.object.segments[0].field != "Components" ||
                call.arguments.empty() ||
                call.arguments[0].kind != Expr::Kind::String) {
                continue;
            }
            const std::string &name = call.arguments[0].text;
            if (call.method == "Add" || call.method == "add") {
                bool over = false;
                if (call.arguments.size() >= 2) {
                    const Expr &arg = call.arguments[1];
                    over = (arg.kind == Expr::Kind::Bool && arg.boolean) ||
                           arg.kind == Expr::Kind::Symbol;
                }
                components.push_back(Component{name, over});
            } else if (call.method == "Remove" || call.method == "remove") {
                components.erase(
                    std::remove_if(components.begin(), components.end(),
                                   [&](const Component &c) { return c.name == name; }),
                    components.end());
            }
        }
        out.Components.Entries.clear();
        out.Components.Entries.reserve(components.size());
        for (const Component &component : components) {
            ::fable::defs::CThingComponentSet::CEntry entry;
            entry.Name.TablePos =
                static_cast<std::int32_t>(names->intern(component.name));
            entry.DriverType = thing_component_driver_type(component.name);
            entry.Override = component.override_flag;
            out.Components.Entries.push_back(entry);
        }
        out.Components.TrailingU32 = 28011726;
    }
}

/// `SoundMap.AddSingle/AddPair/AddCriteria` replay for entity sounds and
/// regions. Keys are CRC32 names; criteria strings are interned. Runs before
/// generic lowering to preserve names.bin interning order.
template <class T>
void build_sound_map(T &out, const Definition &definition, const SymbolTable &symbols,
                     const DefinitionIndex *definitions, NamesBuilder *names) {
    if constexpr (std::is_same_v<T, ::fable::defs::CEntitySoundDef> ||
                  std::is_same_v<T, ::fable::defs::CRegionDef>) {
        if (names == nullptr) {
            return;
        }
        auto &f0 = out.SoundMap.SoundMap;
        auto &f1 = out.SoundMap.CriteriaMap;
        for (const Statement &statement : definition.body) {
            if (statement.kind != Statement::Kind::MethodCall) {
                continue;
            }
            const MethodCall &call = statement.method_call;
            if (call.object.segments.size() != 1 ||
                call.object.segments[0].kind != PathSegment::Kind::Field ||
                call.object.segments[0].field != "SoundMap" ||
                call.arguments.empty() ||
                call.arguments[0].kind != Expr::Kind::String) {
                continue;
            }
            const std::uint32_t key = crc32(call.arguments[0].text);
            auto eval_i32 = [&](std::size_t index) -> std::int32_t {
                if (index >= call.arguments.size()) {
                    return 0;
                }
                Value value;
                if (!evaluate_expr(call.arguments[index], symbols, definitions, value)) {
                    if (value.kind == Value::Kind::Integer) {
                        return static_cast<std::int32_t>(value.integer);
                    }
                    if (value.kind == Value::Kind::Bool) {
                        return value.boolean ? 1 : 0;
                    }
                }
                return 0;
            };
            if (call.method == "AddSingle") {
                const std::int32_t sound = eval_i32(1);
                ::fable::defs::CSoundPair pair;
                pair.FirstSound = sound;
                pair.LastSound = sound;
                f0.emplace_back(key, pair);
            } else if (call.method == "AddPair") {
                ::fable::defs::CSoundPair pair;
                pair.FirstSound = eval_i32(1);
                pair.LastSound = eval_i32(2);
                f0.emplace_back(key, pair);
            } else if (call.method == "AddCriteria" && call.arguments.size() >= 2 &&
                       call.arguments[1].kind == Expr::Kind::String) {
                ::fable::defs::CDefString criteria;
                criteria.TablePos = static_cast<std::int32_t>(
                    names->intern(call.arguments[1].text));
                f1.emplace_back(key, criteria);
            }
        }
        std::sort(f0.begin(), f0.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });
        std::sort(f1.begin(), f1.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });
    }
}

/// Container normalizations performed by the retail Transfer bodies after
/// reading: `CThingCreatureDef::NavigatorTypes` is sorted and deduplicated.
template <class T>
void post_process_schema(T &value) {
    if constexpr (std::is_same_v<T, ::fable::defs::CThingCreatureDef>) {
        std::sort(value.NavigatorTypes.begin(), value.NavigatorTypes.end());
        value.NavigatorTypes.erase(
            std::unique(value.NavigatorTypes.begin(), value.NavigatorTypes.end()),
            value.NavigatorTypes.end());
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CVillagePeopleDef>) {
        // The wire has two consecutive `MaleTeacher` controls (the second
        // backed by the PDB's FemaleTeacher field).  The text's single
        // `MaleTeacher` applies to the first; the duplicate must stay unset.
        if (value.MaleTeacher != 0) {
            value.FemaleTeacher = 0;
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CScriptDef>) {
        // Retail's script def deliberately serializes this field twice but
        // leaves both copies at zero; the text value is not applied.
        value.TemplePrayerFactorHighest = 0;
        value.TemplePrayerFactorHighest2 = 0;
    }
    if constexpr (std::is_same_v<T, ::fable::defs::CDegradableDef>) {
        std::sort(value.Degradations.begin(), value.Degradations.end(),
                  [](const auto &a, const auto &b) {
                      return a.HealthPercentage > b.HealthPercentage;
                  });
        for (auto &degradation : value.Degradations) {
            degradation.Type = static_cast<std::uint8_t>(value.GraphicType);
        }
    }
    if constexpr (std::is_same_v<T, ::fable::defs::COpinionSourceDef>) {
        for (std::size_t i = 0; i < 79; ++i) {
            value.ReactionFlags[i] = i < 18 ? false : value.ReactionFlagDefault;
        }
        for (const auto &[reaction, enabled] : value.ReactionFlag) {
            const auto index = static_cast<std::size_t>(reaction);
            if (index < 79) {
                value.ReactionFlags[index] = enabled;
            }
        }
        value.OpinionProduced.OpinionValues.assign(5, 0.0f);
        for (const auto &[opinion, amount] : value.ProducedOpinion) {
            const auto index = static_cast<std::size_t>(opinion);
            if (index < 5) {
                value.OpinionProduced.OpinionValues[index] = amount;
            }
        }
    }
}

/// Replay `CAnimationSet` method calls (`Animation.Add/AddCombat/Remove/
/// StartGroup/EndGroup/SetDefault*`) into the wire entry list. Runs before
/// generic lowering so animation-key strings are interned in the same order as
/// the reference compiler.
inline void build_animation_set(::fable::defs::CAnimationSet &set,
                                const Definition &definition,
                                const SymbolTable &symbols,
                                const DefinitionIndex *definitions,
                                NamesBuilder *names) {
    if (names == nullptr) {
        return;
    }
    const auto eval_i32 = [&](const Expr &expr) -> std::int32_t {
        Value value;
        if (!evaluate_expr(expr, symbols, definitions, value)) {
            if (value.kind == Value::Kind::Integer) {
                return static_cast<std::int32_t>(value.integer);
            }
            if (value.kind == Value::Kind::Bool) {
                return value.boolean ? 1 : 0;
            }
            if (value.kind == Value::Kind::Float) {
                return static_cast<std::int32_t>(value.floating);
            }
        }
        return 0;
    };
    const auto eval_f32 = [&](const Expr &expr) -> float {
        Value value;
        if (!evaluate_expr(expr, symbols, definitions, value)) {
            if (value.kind == Value::Kind::Float) {
                return static_cast<float>(value.floating);
            }
            if (value.kind == Value::Kind::Integer) {
                return static_cast<float>(value.integer);
            }
        }
        return 0.0f;
    };
    const auto eval_string = [&](const Expr &expr, bool &ok) -> std::string {
        if (expr.kind == Expr::Kind::String) {
            ok = true;
            return expr.text;
        }
        ok = false;
        return {};
    };
    std::int32_t current_group = -1;
    for (const Statement &statement : definition.body) {
        if (statement.kind != Statement::Kind::MethodCall) {
            continue;
        }
        const MethodCall &call = statement.method_call;
        if (call.object.segments.size() != 1 ||
            call.object.segments[0].kind != PathSegment::Kind::Field ||
            call.object.segments[0].field != "Animation") {
            continue;
        }
        const auto &args = call.arguments;
        if (call.method == "StartGroup") {
            if (!args.empty()) {
                bool ok = false;
                const std::string group = eval_string(args[0], ok);
                current_group = ok ? static_cast<std::int32_t>(names->intern(group)) : -1;
            }
            continue;
        }
        if (call.method == "EndGroup") {
            current_group = -1;
            continue;
        }
        if (call.method == "SetDefaultDelay" && !args.empty()) {
            set.DefaultDelay = eval_i32(args[0]);
            continue;
        }
        if (call.method == "SetDefaultGroupName" && !args.empty()) {
            bool ok = false;
            const std::string group = eval_string(args[0], ok);
            set.DefaultGroup.TablePos =
                ok ? static_cast<std::int32_t>(names->intern(group)) : -1;
            continue;
        }
        if (call.method == "SetDefaultFlags" && !args.empty()) {
            set.DefaultFlags = eval_i32(args[0]);
            continue;
        }
        if (call.method == "SetDefaultTransitionTime" && !args.empty()) {
            set.DefaultTransitionTime = eval_i32(args[0]);
            continue;
        }
        if (call.method == "Remove" && !args.empty()) {
            bool ok = false;
            const std::string key = eval_string(args[0], ok);
            if (ok) {
                const std::uint32_t crc = crc32(key);
                auto it = std::find_if(set.Anims.begin(), set.Anims.end(),
                                       [&](const auto &entry) { return entry.first == crc; });
                if (it != set.Anims.end()) {
                    set.Anims.erase(it);
                }
            }
            continue;
        }
        if ((call.method != "Add" && call.method != "AddCombat") || args.empty()) {
            continue;
        }
        bool key_ok = false;
        const std::string key = eval_string(args[0], key_ok);
        if (!key_ok) {
            continue;
        }
        ::fable::defs::CAnimationEntry entry;
        entry.BankIndex = args.size() > 1 ? eval_i32(args[1]) : 0;
        entry.AnimName.TablePos = static_cast<std::int32_t>(names->intern(key));
        entry.GroupName.TablePos = current_group;
        const bool combat = call.method == "AddCombat";
        std::int32_t transition = combat ? 1 : -1;
        std::int32_t delay = combat ? 0 : -1;
        for (std::size_t i = 2; i < args.size(); ++i) {
            if (args[i].kind != Expr::Kind::Constructor) {
                continue;
            }
            const Expr &ctor = args[i];
            auto component = [&](std::uint32_t tag) {
                ::fable::defs::CAnimationComponent c;
                c.Tag = tag;
                return c;
            };
            if (ctor.text == "TransTime" && !ctor.arguments.empty()) {
                transition = eval_i32(ctor.arguments[0]);
            } else if (ctor.text == "Delay" && !ctor.arguments.empty()) {
                delay = eval_i32(ctor.arguments[0]);
            } else if (ctor.text == "Flags" && !ctor.arguments.empty()) {
                auto c = component(0);
                c.Flags = eval_i32(ctor.arguments[0]);
                entry.Components.push_back(c);
            } else if (ctor.text == "Handedness") {
                auto c = component(3);
                c.StartHandedness = ctor.arguments.empty() ? 0 : eval_i32(ctor.arguments[0]);
                c.EndHandedness = ctor.arguments.size() < 2 ? 0 : eval_i32(ctor.arguments[1]);
                entry.Components.push_back(c);
            } else if (ctor.text == "Combo") {
                auto c = component(4);
                if (ctor.arguments.size() >= 2) {
                    c.ComboStage = eval_i32(ctor.arguments[0]);
                    c.ComboId = eval_i32(ctor.arguments[1]);
                } else {
                    c.ComboStage = -777;
                    c.ComboId = ctor.arguments.empty() ? 0 : eval_i32(ctor.arguments[0]);
                }
                entry.Components.push_back(c);
            } else if (ctor.text == "Recoil" && !ctor.arguments.empty()) {
                auto c = component(5);
                c.RecoilAnimIndex = eval_i32(ctor.arguments[0]);
                entry.Components.push_back(c);
            } else if (ctor.text == "CombatMisc") {
                auto c = component(6);
                c.MeleeFlourish = !ctor.arguments.empty() && eval_i32(ctor.arguments[0]) != 0;
                c.MeleeKnockdown = ctor.arguments.size() >= 2 && eval_i32(ctor.arguments[1]) != 0;
                entry.Components.push_back(c);
            } else if (ctor.text == "TargetOffset") {
                auto c = component(7);
                c.TargetOffset = true;
                c.TargetOffsetVectorX = ctor.arguments.empty() ? 0.0f : eval_f32(ctor.arguments[0]);
                c.TargetOffsetVectorY = ctor.arguments.size() < 2 ? 0.0f : eval_f32(ctor.arguments[1]);
                entry.Components.push_back(c);
            } else if (ctor.text == "NextAnimFilter" && !ctor.arguments.empty()) {
                auto c = component(11);
                bool ok = false;
                const std::string filter = eval_string(ctor.arguments[0], ok);
                c.NextFilter.TablePos = ok ? static_cast<std::int32_t>(names->intern(filter)) : -1;
                entry.Components.push_back(c);
            } else if (ctor.text == "SpeedMultiplier" && !ctor.arguments.empty()) {
                auto c = component(9);
                c.AnimationSpeedMultiplier = eval_f32(ctor.arguments[0]);
                entry.Components.push_back(c);
            } else if (ctor.text == "StrikeResponseAnim" && !ctor.arguments.empty()) {
                auto c = component(12);
                bool ok = false;
                c.ResponseAnimName = eval_string(ctor.arguments[0], ok);
                entry.Components.push_back(c);
            }
        }
        if (transition >= 0) {
            auto c = ::fable::defs::CAnimationComponent{};
            c.Tag = 1;
            c.TransitionInTime = transition;
            entry.Components.push_back(c);
        }
        if (delay >= 0) {
            auto c = ::fable::defs::CAnimationComponent{};
            c.Tag = 2;
            c.Delay = delay;
            entry.Components.push_back(c);
        }
        std::stable_sort(entry.Components.begin(), entry.Components.end(),
                         [](const auto &a, const auto &b) { return a.Tag < b.Tag; });
        set.Anims.emplace_back(crc32(key), std::move(entry));
    }
    std::sort(set.Anims.begin(), set.Anims.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
}

template <class T>
void lower_definition(const Definition &def, T &out, const SymbolTable &symbols,
                      LowerStats &stats, std::vector<std::string> *applied = nullptr,
                      const DefinitionIndex *definitions = nullptr,
                      NamesBuilder *names = nullptr) {
    std::vector<const Statement *> statements;
    statements.reserve(def.body.size());
    for (const auto &statement : def.body) {
        statements.push_back(&statement);
    }
    build_sound_map(out, def, symbols, definitions, names);
    apply_schema_defaults(out, names);
    if constexpr (std::is_same_v<T, ::fable::defs::CAppearanceDef> ||
                  std::is_same_v<T, ::fable::defs::CAnimatingObjectDef>) {
        build_animation_set(out.Animation, def, symbols, definitions, names);
    }
    detail::Context context{symbols, definitions, stats, applied, names};
    detail::lower_object(out, statements, 0, context);
    build_thing_components(out, def, names);
    post_process_schema(out);
}

} // namespace fable::defs::compiler
