#include "fable/defs/compiler/decompiler.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <variant>

#include "fable/defs/compiler/binary_reader.hpp"
#include "fable/defs/compiler/def_reader.hpp"
#include "fable/defs/compiler/header_resolver.hpp"
#include "fable/defs/compiler/parser.hpp"
#include "fable/defs/compiler/reference_fields.hpp"
#include "fable/defs/compiler/wire.hpp"

namespace fable::defs::compiler {
namespace {

struct Context {
    std::map<std::int32_t, std::string> idx_to_name;
    std::map<std::int32_t, std::string> off_to_str;
    std::map<std::uint32_t, std::string> crc_to_str;
    std::map<std::string, ParsedEntry> nulldefs;
    std::vector<ParsedEntry> entries;
    HeaderResolver headers;
};


void add_crc_string(std::map<std::uint32_t, std::string> &map, const std::string &text) {
    map[crc32(text)] = text;
}

void collect_expr_crc(const Expr &expr, std::map<std::uint32_t, std::string> &map) {
    if (expr.kind == Expr::Kind::String || expr.kind == Expr::Kind::Symbol) {
        add_crc_string(map, expr.text);
    } else if (expr.kind == Expr::Kind::Constructor) {
        add_crc_string(map, expr.text);
        for (const Expr &argument : expr.arguments) {
            collect_expr_crc(argument, map);
        }
    } else if (expr.kind == Expr::Kind::BitOr || expr.kind == Expr::Kind::Add) {
        for (const Expr &term : expr.terms) {
            collect_expr_crc(term, map);
        }
    }
}

void collect_statement_crc(const Statement &statement,
                           std::map<std::uint32_t, std::string> &map) {
    if (statement.kind == Statement::Kind::Field) {
        collect_expr_crc(statement.field.expr, map);
        for (const PathSegment &segment : statement.field.path.segments) {
            if (segment.kind == PathSegment::Kind::Field) {
                add_crc_string(map, segment.field);
            } else {
                collect_expr_crc(segment.index, map);
            }
        }
    } else if (statement.kind == Statement::Kind::MethodCall) {
        add_crc_string(map, statement.method_call.method);
        for (const Expr &argument : statement.method_call.arguments) {
            collect_expr_crc(argument, map);
        }
    } else if (statement.kind == Statement::Kind::TaggedBlock && statement.tagged_block) {
        add_crc_string(map, statement.tagged_block->tag);
        for (const Statement &child : statement.tagged_block->body) {
            collect_statement_crc(child, map);
        }
    }
}

void collect_text_crc(const std::string &defs_directory, Context &context) {
    if (defs_directory.empty()) {
        return;
    }
    namespace fs = std::filesystem;
    std::error_code error;
    for (fs::recursive_directory_iterator it(defs_directory, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }
        const fs::path path = it->path();
        const std::string extension = path.extension().string();
        if (extension != ".def" && extension != ".tpl") {
            continue;
        }
        const std::string filename = path.filename().string();
        if (filename.rfind("z_mod_decompiled", 0) == 0 ||
            filename.find("_deprecated.") != std::string::npos) {
            continue;
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }
        std::string text{std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>()};
        ParseResult parsed = parse_source(text, FileId::anonymous());
        if (std::holds_alternative<ParseError>(parsed)) {
            continue;
        }
        const SourceAst &ast = std::get<SourceAst>(parsed);
        for (const auto &item : ast.items) {
            if (item.kind != Item::Kind::Definition) {
                continue;
            }
            const Definition &definition = item.definition.value;
            add_crc_string(context.crc_to_str, definition.name);
            add_crc_string(context.crc_to_str, definition.def_type);
            for (const Statement &statement : definition.body) {
                collect_statement_crc(statement, context.crc_to_str);
            }
        }
    }
}

bool build_context(const DefBinary &binary, const Names &names,
                   const HeaderResolver &headers, const std::string &defs_directory,
                   Context &context, DecompileStats &stats) {
    context = Context{};
    context.headers = headers;
    for (const auto &[offset, entry] : names.entries()) {
        context.off_to_str[static_cast<std::int32_t>(offset)] = entry.string;
        context.crc_to_str[entry.crc] = entry.string;
    }
    context.entries.resize(binary.entries.size());
    for (std::size_t i = 0; i < binary.entries.size(); ++i) {
        const BinaryEntry &entry = binary.entries[i];
        ParsedEntry parsed;
        std::string error;
        if (!parse_entry(entry, parsed, error)) {
            if (stats.errors.size() < 20) {
                stats.errors.push_back(entry.file_name.empty() ? entry.def_name
                                                               : entry.file_name +
                                                                     ": " + error);
            }
            parsed.def_type = entry.def_name;
            parsed.is_template = entry.is_template;
        }
        context.entries[i] = std::move(parsed);
        if (!entry.file_name.empty() && entry.file_name.rfind("NULLDEF_", 0) != 0) {
            context.idx_to_name[static_cast<std::int32_t>(entry.global_index)] =
                entry.file_name;
        }
        if (!entry.file_name.empty() && entry.file_name.rfind("NULLDEF_", 0) == 0) {
            context.nulldefs[entry.def_name] = context.entries[i];
        }
        context.crc_to_str[crc32(entry.def_name)] = entry.def_name;
        if (!entry.file_name.empty()) {
            context.crc_to_str[crc32(entry.file_name)] = entry.file_name;
        }
    }
    collect_text_crc(defs_directory, context);
    return true;
}

std::string normalized_path(const std::filesystem::path &path) {
    std::string text = path.string();
    std::replace(text.begin(), text.end(), '\\', '/');
    return text;
}

std::map<std::string, std::string> build_parent_map(const std::string &defs_directory) {
    std::map<std::string, std::string> parents;
    if (defs_directory.empty()) {
        return parents;
    }
    namespace fs = std::filesystem;
    std::error_code error;
    for (fs::recursive_directory_iterator it(defs_directory, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }
        const fs::path path = it->path();
        const std::string extension = path.extension().string();
        if (extension != ".def" && extension != ".tpl") {
            continue;
        }
        const std::string filename = path.filename().string();
        if (filename.rfind("z_mod_decompiled", 0) == 0 ||
            filename.find("_deprecated.") != std::string::npos) {
            continue;
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }
        std::string text{std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>()};
        ParseResult parsed = parse_source(text, FileId::anonymous());
        if (std::holds_alternative<ParseError>(parsed)) {
            continue;
        }
        const SourceAst &ast = std::get<SourceAst>(parsed);
        for (const auto &item : ast.items) {
            if (item.kind == Item::Kind::Definition && item.definition.value.specializes) {
                parents[item.definition.value.name] = *item.definition.value.specializes;
            }
        }
    }
    return parents;
}

bool is_scalar_def_index(const std::string &name) {
    static const char *const names[] = {
        "CardVal", "Mult", "Name", "InitialHawkingPhrase", "ReplyToRequestPhrase",
        "CustomerFollowPhrase", "ThankingPhrase", "MinGoldForDonation",
        "MaxGoldForDonation", "NumDaysToKeepSpecialItem", "MinAmountBeforeSpecialItem",
        "BedCost", "PointerParticleEffect", "Courses", "StartingCourse", "Rounds",
        "RenownGainedPerWitnessAhead", "EmoteIconForAvailableWitness",
        "SecondsBetweenEffectUsesAllowed", "Property", "TargetingRange"};
    for (const char *candidate : names) {
        if (candidate == name) {
            return true;
        }
    }
    return false;
}

bool valid_identifier(const std::string &text) {
    if (text.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(text.front());
    if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') ||
          first == '_')) {
        return false;
    }
    for (unsigned char c : text) {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    return true;
}

std::string index_name(std::int64_t index, const Context &ctx) {
    if (index == 0) {
        return "NULL";
    }
    const auto it = ctx.idx_to_name.find(static_cast<std::int32_t>(index));
    if (it != ctx.idx_to_name.end()) {
        return it->second;
    }
    return std::to_string(index);
}

std::string resolve_string(std::int64_t offset, const Context &ctx) {
    if (offset < 0) {
        return {};
    }
    const auto it = ctx.off_to_str.find(static_cast<std::int32_t>(offset));
    return it == ctx.off_to_str.end() ? std::string{} : it->second;
}

std::string quote(const std::string &value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string resolve_integer(std::int64_t value, const std::string &field_name,
                            const std::string &scope, const Context &ctx) {
    std::string resolved = ctx.headers.resolve_field(
        scope, field_name, static_cast<std::int32_t>(value));
    if (resolved == std::to_string(value) && is_reference_field(field_name)) {
        if (value == 0) {
            return "NULL";
        }
        const auto it = ctx.idx_to_name.find(static_cast<std::int32_t>(value));
        if (it != ctx.idx_to_name.end()) {
            return valid_identifier(it->second) ? it->second : quote(it->second);
        }
    }
    return resolved;
}

std::string format_float(std::uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    if (!std::isfinite(value)) {
        return "0.0";
    }
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                      std::chars_format::general);
    std::string text(buffer, result.ptr);
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        text += ".0";
    }
    return text;
}

std::string format_float_plain(std::uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    if (!std::isfinite(value)) {
        return "0";
    }
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                      std::chars_format::general);
    return std::string(buffer, result.ptr);
}

std::string format_expr(const Value &value, const std::string &field_name,
                        const std::string &scope, const Context &ctx) {
    switch (value.kind) {
    case ValueKind::Int:
        return resolve_integer(value.integer, field_name, scope, ctx);
    case ValueKind::Float:
        return format_float(value.f32_bits);
    case ValueKind::Bool:
        return value.boolean ? "TRUE" : "FALSE";
    case ValueKind::String:
        return quote(value.text);
    case ValueKind::WString: {
        std::string text;
        for (char16_t ch : value.wide) {
            text.push_back(ch < 128 ? static_cast<char>(ch) : '?');
        }
        return quote(text);
    }
    case ValueKind::PString: {
        std::string text(value.bytes.begin(), value.bytes.end());
        return quote(text);
    }
    case ValueKind::DefString:
        return quote(resolve_string(value.integer, ctx));
    case ValueKind::DefIndex:
        if (is_scalar_def_index(field_name)) {
            return ctx.headers.resolve_field(scope, field_name,
                                             static_cast<std::int32_t>(value.integer));
        }
        if (value.integer == 0) {
            return "NULL";
        }
        {
            const std::string name = index_name(value.integer, ctx);
            return valid_identifier(name) ? name : quote(name);
        }
    case ValueKind::Enum:
    case ValueKind::Flags:
        return resolve_integer(value.integer, field_name, scope, ctx);
    default:
        return "0";
    }
}

void emit_line(std::string &out, int indent, const std::string &path,
               const std::string &expr) {
    out.append(static_cast<std::size_t>(indent) * 4, ' ');
    out += path;
    out.push_back(' ');
    out += expr;
    out += ";\n";
}

void emit_clear(std::string &out, int indent, const std::string &path) {
    out.append(static_cast<std::size_t>(indent) * 4, ' ');
    out += path;
    out += ".clear();\n";
}

void emit_method(std::string &out, int indent, const std::string &object,
                 const std::string &method, const std::vector<std::string> &arguments) {
    out.append(static_cast<std::size_t>(indent) * 4, ' ');
    out += object;
    out.push_back('.');
    out += method;
    out.push_back('(');
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (i != 0) {
            out += ", ";
        }
        out += arguments[i];
    }
    out += ");\n";
}

const Value *find_value_field(const Value &value, const std::string &name) {
    for (const auto &[field_name, field_value] : value.fields) {
        if (field_name == name) {
            return &field_value;
        }
    }
    return nullptr;
}

std::int32_t value_i32(const Value &value) {
    return static_cast<std::int32_t>(value.integer);
}

std::string value_string(const Value &value, const Context &ctx) {
    if (value.kind == ValueKind::DefString) {
        return resolve_string(value.integer, ctx);
    }
    return value.text;
}

void emit_animation_set(const Value &set, const Value *base, const Context &ctx,
                        std::string &out, int indent) {
    const Value *delay = find_value_field(set, "DefaultDelay");
    const Value *base_delay = base == nullptr ? nullptr : find_value_field(*base, "DefaultDelay");
    if (delay != nullptr && (base_delay == nullptr || delay->integer != base_delay->integer)) {
        emit_line(out, indent, "Animation.SetDefaultDelay", std::to_string(delay->integer));
    }
    const Value *group = find_value_field(set, "DefaultGroup");
    const Value *base_group = base == nullptr ? nullptr : find_value_field(*base, "DefaultGroup");
    if (group != nullptr && group->integer >= 0 &&
        (base_group == nullptr || group->integer != base_group->integer)) {
        emit_method(out, indent, "Animation", "SetDefaultGroupName",
                    {quote(resolve_string(group->integer, ctx))});
    }

    const Value *anims = find_value_field(set, "Anims");
    if (anims == nullptr || anims->kind != ValueKind::List) {
        return;
    }
    std::int32_t current_group = -1;
    for (const Value &item : anims->list) {
        if (item.kind != ValueKind::Struct) {
            continue;
        }
        const Value *key = find_value_field(item, "first");
        const Value *entry = find_value_field(item, "second");
        if (entry == nullptr || entry->kind != ValueKind::Struct) {
            continue;
        }
        const Value *entry_group = find_value_field(*entry, "GroupName");
        const std::int32_t group_pos = entry_group == nullptr ? -1 : value_i32(*entry_group);
        if (group_pos != current_group) {
            if (current_group >= 0) {
                emit_method(out, indent, "Animation", "EndGroup", {});
            }
            current_group = group_pos;
            if (current_group >= 0) {
                emit_method(out, indent, "Animation", "StartGroup",
                            {quote(resolve_string(current_group, ctx))});
            }
        }

        std::string anim_name;
        if (const Value *name = find_value_field(*entry, "AnimName")) {
            anim_name = value_string(*name, ctx);
        }
        if (anim_name.empty() && key != nullptr) {
            const auto it = ctx.crc_to_str.find(static_cast<std::uint32_t>(key->integer));
            if (it != ctx.crc_to_str.end()) {
                anim_name = it->second;
            }
        }
        if (anim_name.empty()) {
            anim_name = "anim_";
            char buffer[9] = {};
            std::snprintf(buffer, sizeof(buffer), "%08x",
                          key == nullptr ? 0u : static_cast<unsigned>(key->integer));
            anim_name += buffer;
        }
        const int32_t bank = find_value_field(*entry, "BankIndex") != nullptr
                                 ? value_i32(*find_value_field(*entry, "BankIndex"))
                                 : 0;
        std::vector<std::string> args = {
            quote(anim_name),
            ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Anim), bank)};

        if (const Value *components = find_value_field(*entry, "Components")) {
            for (const Value &component : components->list) {
                if (component.kind != ValueKind::Variant && component.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *value = nullptr;
                switch (component.tag) {
                case 0:
                    value = find_value_field(component, "Flags");
                    if (value) args.push_back("Flags(" + ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::AnimFlags), value_i32(*value)) + ")");
                    break;
                case 1:
                    value = find_value_field(component, "TransitionInTime");
                    if (value) args.push_back("TransTime(" + std::to_string(value->integer) + ")");
                    break;
                case 2:
                    value = find_value_field(component, "Delay");
                    if (value) args.push_back("Delay(" + std::to_string(value->integer) + ")");
                    break;
                case 3: {
                    const Value *start = find_value_field(component, "StartHandedness");
                    const Value *end = find_value_field(component, "EndHandedness");
                    if (start && end) {
                        args.push_back("Handedness(" +
                            ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Handedness),
                                                       value_i32(*start)) + ", " +
                            ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Handedness),
                                                       value_i32(*end)) + ")");
                    }
                    break;
                }
                case 4: {
                    const Value *stage = find_value_field(component, "ComboStage");
                    const Value *id = find_value_field(component, "ComboId");
                    if (stage && id) {
                        if (value_i32(*stage) == -777) {
                            args.push_back("Combo(" + std::to_string(id->integer) + ")");
                        } else {
                            args.push_back("Combo(" + std::to_string(stage->integer) + ", " +
                                           std::to_string(id->integer) + ")");
                        }
                    }
                    break;
                }
                case 5:
                    value = find_value_field(component, "RecoilAnimIndex");
                    if (value) args.push_back("Recoil(" + ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Anim), value_i32(*value)) + ")");
                    break;
                case 6: {
                    const Value *flourish = find_value_field(component, "MeleeFlourish");
                    const Value *knockdown = find_value_field(component, "MeleeKnockdown");
                    if (flourish && knockdown) {
                        args.push_back(std::string("CombatMisc(") +
                                       (flourish->boolean ? "TRUE" : "FALSE") + ", " +
                                       (knockdown->boolean ? "TRUE" : "FALSE") + ")");
                    }
                    break;
                }
                case 7: {
                    const Value *x = find_value_field(component, "TargetOffsetVectorX");
                    const Value *y = find_value_field(component, "TargetOffsetVectorY");
                    if (x && y) {
                        args.push_back("TargetOffset(" + format_float_plain(x->f32_bits) +
                                       ", " + format_float_plain(y->f32_bits) + ")");
                    }
                    break;
                }
                case 9:
                    value = find_value_field(component, "AnimationSpeedMultiplier");
                    if (value) args.push_back("SpeedMultiplier(" +
                                              format_float_plain(value->f32_bits) + ")");
                    break;
                case 11:
                    value = find_value_field(component, "NextFilter");
                    if (value) args.push_back("NextAnimFilter(" + quote(value_string(*value, ctx)) + ")");
                    break;
                case 12:
                    value = find_value_field(component, "ResponseAnimName");
                    if (value) {
                        const std::string text(value->bytes.begin(), value->bytes.end());
                        args.push_back("StrikeResponseAnim(" + quote(text) + ")");
                    }
                    break;
                default:
                    break;
                }
            }
        }
        emit_method(out, indent, "Animation", "Add", args);
    }
    if (current_group >= 0) {
        emit_method(out, indent, "Animation", "EndGroup", {});
    }
}

bool ignored_field(const std::string &name) {
    return name == "trailing_u32" || name == "trailing" || name == "skip" ||
           name == "Dirty" || name == "dirty";
}

bool sem_equal(const Value &a, const Value &b, const std::string &field_name,
               const Context &a_ctx, const Context &b_ctx) {
    const bool a_numeric = a.kind == ValueKind::Int || a.kind == ValueKind::Enum ||
                           a.kind == ValueKind::Flags;
    const bool b_numeric = b.kind == ValueKind::Int || b.kind == ValueKind::Enum ||
                           b.kind == ValueKind::Flags;
    if (a.kind == ValueKind::DefIndex && b.kind == ValueKind::DefIndex) {
        if (is_scalar_def_index(field_name)) {
            return a.integer == b.integer;
        }
        return index_name(a.integer, a_ctx) == index_name(b.integer, b_ctx);
    }
    if (a.kind == ValueKind::DefString && b.kind == ValueKind::DefString) {
        return resolve_string(a.integer, a_ctx) == resolve_string(b.integer, b_ctx);
    }
    if (a_numeric && b_numeric) {
        return a.integer == b.integer;
    }
    if (a.kind != b.kind) {
        return false;
    }
    switch (a.kind) {
    case ValueKind::Float:
        return a.f32_bits == b.f32_bits;
    case ValueKind::Bool:
        return a.boolean == b.boolean;
    case ValueKind::String:
    case ValueKind::PString:
        return a.text == b.text && a.bytes == b.bytes;
    case ValueKind::WString:
        return a.wide == b.wide;
    case ValueKind::List: {
        if (a.list.size() != b.list.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.list.size(); ++i) {
            if (!sem_equal(a.list[i], b.list[i], field_name, a_ctx, b_ctx)) {
                return false;
            }
        }
        return true;
    }
    case ValueKind::Map: {
        if (a.map.size() != b.map.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.map.size(); ++i) {
            if (!sem_equal(a.map[i].first, b.map[i].first, field_name, a_ctx, b_ctx) ||
                !sem_equal(a.map[i].second, b.map[i].second, field_name, a_ctx, b_ctx)) {
                return false;
            }
        }
        return true;
    }
    case ValueKind::Struct:
    case ValueKind::Variant: {
        if (a.tag != b.tag || a.fields.size() != b.fields.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.fields.size(); ++i) {
            if (a.fields[i].first != b.fields[i].first ||
                !sem_equal(a.fields[i].second, b.fields[i].second, a.fields[i].first,
                           a_ctx, b_ctx)) {
                return false;
            }
        }
        return true;
    }
    default:
        return a.integer == b.integer;
    }
}

bool has_field(const Value &value, const std::string &name);
bool engine_graphic_shape(const Value &value);
void emit_engine_graphic(const std::string &path, const Value &value, const Value *base,
                         const std::string &scope, const Context &ctx, std::string &out,
                         int indent);

std::optional<std::string> compound_constructor(const Value &value,
                                                const std::string &scope,
                                                const Context &ctx);

void unlower_value(const std::string &path, const std::string &field_name,
                   const Value &value, const Value *base, const std::string &scope,
                   const Context &ctx, std::string &out, int indent, bool force);

void unlower_struct(const std::string &path, const Value &value,
                    const std::string &scope, const Context &ctx, std::string &out,
                    int indent, bool force) {
    bool emitted = false;
    for (const auto &[name, child] : value.fields) {
        if (ignored_field(name)) {
            continue;
        }
        unlower_value(path + "." + name, name, child, nullptr, scope, ctx, out, indent,
                      true);
        emitted = true;
    }
    if (!emitted && force) {
        emit_line(out, indent, path, "0");
    }
}

void unlower_list(const std::string &path, const std::string &field_name,
                  const Value &value, const Value *base, const std::string &scope,
                  const Context &ctx, std::string &out, int indent) {
    if (base != nullptr && base->kind == ValueKind::List && !base->list.empty()) {
        emit_clear(out, indent, path);
    }
    if (value.list.empty()) {
        return;
    }
    const bool struct_elements = value.list.front().kind == ValueKind::Struct ||
                                 value.list.front().kind == ValueKind::Variant;
    for (std::size_t i = 0; i < value.list.size(); ++i) {
        const std::string item_path = path + "[" + std::to_string(i) + "]";
        const Value &item = value.list[i];
        if (struct_elements) {
            if (auto constructor = compound_constructor(item, scope, ctx)) {
                out.append(static_cast<std::size_t>(indent) * 4, ' ');
                out += path;
                out += ".Add(";
                out += *constructor;
                out += ");\n";
            } else {
                unlower_struct(item_path, item, scope, ctx, out, indent, true);
            }
        } else {
            out.append(static_cast<std::size_t>(indent) * 4, ' ');
            out += path;
            out += ".Add(";
            out += format_expr(item, field_name, scope, ctx);
            out += ");\n";
        }
    }
}

void unlower_map(const std::string &path, const std::string &field_name,
                 const Value &value, const Value *base, const std::string &scope,
                 const Context &ctx, std::string &out, int indent) {
    if (base != nullptr && base->kind == ValueKind::Map && !base->map.empty()) {
        emit_clear(out, indent, path);
    }
    for (const auto &[key, mapped] : value.map) {
        std::string key_text;
        if (key.kind == ValueKind::Int || key.kind == ValueKind::Enum ||
            key.kind == ValueKind::Flags) {
            key_text = ctx.headers.resolve_map_key(scope, field_name,
                                                   static_cast<std::int32_t>(key.integer));
        } else {
            key_text = format_expr(key, field_name, scope, ctx);
        }
        const std::string item_path = path + "[" + key_text + "]";
        if (mapped.kind == ValueKind::Struct) {
            if (auto constructor = compound_constructor(mapped, scope, ctx)) {
                emit_line(out, indent, item_path, *constructor);
            } else {
                unlower_struct(item_path, mapped, scope, ctx, out, indent, true);
            }
        } else if (mapped.kind == ValueKind::Variant) {
            unlower_struct(item_path, mapped, scope, ctx, out, indent, true);
        } else {
            emit_line(out, indent, item_path, format_expr(mapped, field_name, scope, ctx));
        }
    }
}

void unlower_value(const std::string &path, const std::string &field_name,
                   const Value &value, const Value *base, const std::string &scope,
                   const Context &ctx, std::string &out, int indent, bool force) {
    switch (value.kind) {
    case ValueKind::Struct:
        if (engine_graphic_shape(value)) {
            emit_engine_graphic(path, value, base, scope, ctx, out, indent);
        } else if (auto constructor = compound_constructor(value, scope, ctx)) {
            emit_line(out, indent, path, *constructor);
        } else if (value.fields.size() == 1 && value.fields[0].first == "IntValue" &&
            value.fields[0].second.kind == ValueKind::Int) {
            const std::uint32_t packed =
                static_cast<std::uint32_t>(value.fields[0].second.integer);
            const int r = (packed >> 16) & 0xff;
            const int g = (packed >> 8) & 0xff;
            const int b = packed & 0xff;
            const int a = (packed >> 24) & 0xff;
            std::string expr = "CRGBColour(" + std::to_string(r) + ", " +
                               std::to_string(g) + ", " + std::to_string(b);
            if (a != 255) {
                expr += ", " + std::to_string(a);
            }
            expr += ")";
            emit_line(out, indent, path, expr);
        } else {
            unlower_struct(path, value, scope, ctx, out, indent, force);
        }
        break;
    case ValueKind::Variant:
        if (value.type_name == "CPhysicalPrimitiveInit") {
            std::string constructor;
            switch (value.tag) {
            case 1: {
                const Value *name = find_value_field(value, "BaseName");
                const Value *radius = find_value_field(value, "Radius");
                if ((name == nullptr || name->integer < 0) &&
                    (radius == nullptr || radius->f32_bits == 0xBF800000u)) {
                    constructor = "CPhysicalPrimitiveSphere()";
                } else {
                    constructor = "CPhysicalPrimitiveSphere(" +
                                  (name == nullptr ? std::string("\"\"")
                                                   : quote(value_string(*name, ctx))) +
                                  ", " +
                                  (radius == nullptr ? std::string("0.0")
                                                     : format_float(radius->f32_bits)) +
                                  ")";
                }
                break;
            }
            case 2: {
                const Value *name = find_value_field(value, "BaseName");
                const Value *radius = find_value_field(value, "Radius");
                const Value *height = find_value_field(value, "Height");
                if ((name == nullptr || name->integer < 0) &&
                    (radius == nullptr || radius->f32_bits == 0xBF800000u) &&
                    (height == nullptr || height->f32_bits == 0xBF800000u)) {
                    constructor = "CPhysicalPrimitiveCylinder()";
                } else {
                    constructor = "CPhysicalPrimitiveCylinder(" +
                                  (name == nullptr ? std::string("\"\"")
                                                   : quote(value_string(*name, ctx))) +
                                  ", " +
                                  (radius == nullptr ? std::string("0.0")
                                                     : format_float(radius->f32_bits)) +
                                  ", " +
                                  (height == nullptr ? std::string("0.0")
                                                     : format_float(height->f32_bits)) +
                                  ")";
                }
                break;
            }
            case 3:
                constructor = "CPhysicalPrimitiveMesh()";
                break;
            default:
                constructor = "CPhysicalPrimitiveNull()";
                break;
            }
            emit_line(out, indent, path, constructor);
        }
        break;
    case ValueKind::List:
        unlower_list(path, field_name, value, base, scope, ctx, out, indent);
        break;
    case ValueKind::Map:
        unlower_map(path, field_name, value, base, scope, ctx, out, indent);
        break;
    case ValueKind::Opaque:
        break;
    default:
        emit_line(out, indent, path, format_expr(value, field_name, scope, ctx));
        break;
    }
}


int32_t field_i32(const Value &value, const std::string &name, int32_t fallback = 0) {
    const Value *field = find_value_field(value, name);
    return field == nullptr ? fallback : static_cast<int32_t>(field->integer);
}

bool field_bool(const Value &value, const std::string &name, bool fallback = false) {
    const Value *field = find_value_field(value, name);
    return field == nullptr ? fallback : field->boolean;
}

std::string field_string(const Value &value, const std::string &name,
                         const Context &ctx) {
    const Value *field = find_value_field(value, name);
    return field == nullptr ? std::string{} : value_string(*field, ctx);
}

void emit_expression_set(const Value &set, const Context &ctx, std::string &out,
                         int indent) {
    const Value *expressions = find_value_field(set, "Expressions");
    if (expressions == nullptr) {
        return;
    }
    for (const Value &item : expressions->list) {
        if (item.kind != ValueKind::Struct) {
            continue;
        }
        const Value *key = find_value_field(item, "first");
        const Value *entry = find_value_field(item, "second");
        const int32_t type = key != nullptr ? static_cast<int32_t>(key->integer)
                                           : (entry == nullptr ? 0 : field_i32(*entry, "Type"));
        const int32_t anim = entry == nullptr ? 0 : field_i32(*entry, "AnimIndex");
        emit_method(out, indent, "Expressions", "Add",
                    {ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::ExpressionType), type),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Anim), anim)});
    }
}

void emit_wound_morphs(const Value &set, const Context &ctx, std::string &out,
                       int indent) {
    const Value *morphs = find_value_field(set, "Morphs");
    if (morphs == nullptr) {
        return;
    }
    for (const Value &item : morphs->list) {
        if (item.kind != ValueKind::Struct) {
            continue;
        }
        const Value *key = find_value_field(item, "first");
        const Value *entry = find_value_field(item, "second");
        if (entry == nullptr) {
            continue;
        }
        const int32_t location = key != nullptr ? static_cast<int32_t>(key->integer)
                                                : field_i32(*entry, "Location");
        std::vector<std::string> args;
        args.push_back(ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::BodyLocation),
                                                  location));
        for (const char *name : {"ReplacingTextureIndex", "ReplacingBumpMapIndex",
                                 "WoundTextureIndex", "WoundBumpMapIndex",
                                 "ScarTextureIndex", "ScarBumpMapIndex"}) {
            args.push_back(ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Texture),
                                                      field_i32(*entry, name)));
        }
        args.push_back(std::to_string(field_i32(*entry, "FadeToScarDuration")));
        args.push_back(std::to_string(field_i32(*entry, "TotalDuration")));
        const Value *permanent = find_value_field(*entry, "PermanentScarStrength");
        args.push_back(permanent == nullptr ? "0" : format_float_plain(permanent->f32_bits));
        emit_method(out, indent, "WoundMorphs", "AddWoundAndScar", args);
    }
}

void emit_random_appearance_morph(const Value &ram, const Context &ctx, std::string &out,
                                  int indent) {
    const auto resolve_mesh = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Mesh), value);
    };
    const auto resolve_texture = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Texture), value);
    };
    const auto resolve_body_part = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::BodyPart), value);
    };

    std::vector<int32_t> seen_meshes;
    for (const char *part_name : {"BodyParts0", "BodyParts1", "BodyParts2"}) {
        const Value *part = find_value_field(ram, part_name);
        if (part == nullptr) {
            continue;
        }
        const Value *meshes = find_value_field(*part, "Meshes");
        if (meshes == nullptr) {
            continue;
        }
        const int32_t body_part = part_name[9] - '0';
        for (const Value &mesh : meshes->list) {
            if (mesh.kind != ValueKind::Struct) {
                continue;
            }
            const int32_t mesh_id = field_i32(mesh, "MeshId");
            const int32_t texture_id = field_i32(mesh, "TextureId");
            emit_method(out, indent, "RandomAppearanceMorph", "AddBodyPart",
                        {resolve_body_part(body_part), std::to_string(texture_id),
                         resolve_mesh(mesh_id)});
            if (std::find(seen_meshes.begin(), seen_meshes.end(), mesh_id) !=
                seen_meshes.end()) {
                continue;
            }
            seen_meshes.push_back(mesh_id);

            const Value *morphs = find_value_field(mesh, "TextureMorphs");
            const Value *entries = morphs == nullptr ? nullptr : find_value_field(*morphs, "Entries");
            if (entries == nullptr || entries->list.empty()) {
                continue;
            }
            struct TextureEntry {
                int32_t key;
                int32_t original;
                int32_t replacement;
            };
            std::vector<TextureEntry> values;
            for (const Value &entry : entries->list) {
                if (entry.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *key = find_value_field(entry, "first");
                const Value *second = find_value_field(entry, "second");
                const Value *morph = second == nullptr ? nullptr : find_value_field(*second, "Morph");
                if (key == nullptr || morph == nullptr || morph->list.size() < 8) {
                    continue;
                }
                const auto read = [&](std::size_t at) {
                    return static_cast<int32_t>(morph->list[at].integer);
                };
                values.push_back(TextureEntry{static_cast<int32_t>(key->integer),
                                              read(0) | (read(1) << 8) | (read(2) << 16) |
                                                  (read(3) << 24),
                                              read(4) | (read(5) << 8) | (read(6) << 16) |
                                                  (read(7) << 24)});
            }
            std::size_t i = 0;
            while (i < values.size()) {
                const int32_t original = values[i].original;
                std::size_t j = i;
                while (j < values.size() && values[j].original == original) {
                    ++j;
                }
                const std::size_t group_size = j - i;
                int32_t n = 0;
                if (j < values.size()) {
                    if (values[j].key == 0) {
                        n = 0;
                    } else if (group_size > 1) {
                        n = values[i + 1].key + 1;
                    } else {
                        n = 1;
                    }
                } else if (group_size > 1 && values[i + 1].key > 0) {
                    n = values[i + 1].key + 1;
                } else if (field_i32(ram, "FinalTrailing") == 0) {
                    n = 0;
                } else {
                    n = field_i32(ram, "FinalTrailing");
                }
                std::vector<std::string> args = {
                    resolve_mesh(mesh_id), std::to_string(n), resolve_texture(original)};
                for (std::size_t k = i; k < j; ++k) {
                    args.push_back(resolve_texture(values[k].replacement));
                }
                emit_method(out, indent, "RandomAppearanceMorph", "AddTextureToMesh", args);
                i = j;
            }
        }
    }

    if (const Value *general = find_value_field(ram, "GeneralTextureMorphs")) {
        if (const Value *entries = find_value_field(*general, "Entries")) {
            int32_t original = 0;
            std::vector<std::string> args;
            for (const Value &entry : entries->list) {
                if (entry.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *second = find_value_field(entry, "second");
                const Value *morph = second == nullptr ? nullptr : find_value_field(*second, "Morph");
                if (morph == nullptr || morph->list.size() < 8) {
                    continue;
                }
                const auto read = [&](std::size_t at) {
                    return static_cast<int32_t>(morph->list[at].integer);
                };
                const int32_t this_original = read(0) | (read(1) << 8) | (read(2) << 16) |
                                              (read(3) << 24);
                const int32_t replacement = read(4) | (read(5) << 8) | (read(6) << 16) |
                                            (read(7) << 24);
                if (args.empty()) {
                    original = this_original;
                    args.push_back(resolve_texture(original));
                }
                args.push_back(resolve_texture(replacement));
            }
            if (!args.empty()) {
                emit_method(out, indent, "RandomAppearanceMorph", "AddTextureToAll", args);
            }
        }
    }

    if (const Value *skeletal = find_value_field(ram, "SkeletalMorphs")) {
        for (const Value &entry : skeletal->list) {
            if (entry.kind != ValueKind::Struct) {
                continue;
            }
            const int32_t key = field_i32(entry, "Key");
            const std::string name = resolve_string(
                static_cast<std::int64_t>(field_i32(entry, "BoneOrMorphIndex")), ctx);
            emit_method(out, indent, "RandomAppearanceMorph", "AddSkeletalMorph",
                        {std::to_string(key), quote(name),
                         field_bool(entry, "AllowVariation") ? "TRUE" : "FALSE"});
        }
    }
}





std::string float_value(const Value *value, double fallback = 0.0) {
    if (value == nullptr) {
        return std::to_string(fallback);
    }
    return format_float(value->f32_bits);
}



void emit_frequency_traits(const Value &frequency, const Context &ctx, std::string &out,
                           int indent) {
    const Value *traits = find_value_field(frequency, "Traits");
    if (traits == nullptr) {
        return;
    }
    for (std::size_t index = 0; index < traits->list.size() && index < 79; ++index) {
        const Value &trait = traits->list[index];
        if (trait.kind != ValueKind::Variant && trait.kind != ValueKind::Struct) {
            continue;
        }
        std::string constructor;
        if (trait.tag == 1) {
            const Value *min_wait = find_value_field(trait, "MinWait");
            const Value *wait_range = find_value_field(trait, "WaitRange");
            float min_value = 0.0f;
            float range_value = 0.0f;
            if (min_wait) std::memcpy(&min_value, &min_wait->f32_bits, sizeof(min_value));
            if (wait_range) std::memcpy(&range_value, &wait_range->f32_bits, sizeof(range_value));
            const float max_value = min_value + range_value;
            std::uint32_t max_bits = 0;
            std::memcpy(&max_bits, &max_value, sizeof(max_bits));
            constructor = "Linear(" +
                (min_wait == nullptr ? std::string("0.0") : format_float(min_wait->f32_bits)) +
                ", " + format_float(max_bits) + ")";
        } else if (trait.tag == 3) {
            const Value *max_count = find_value_field(trait, "MaxCount");
            const Value *min_gap = find_value_field(trait, "MinGapFrames");
            const Value *recharge = find_value_field(trait, "CountRechargePerFrame");
            float max_count_value = 0.0f;
            if (max_count) std::memcpy(&max_count_value, &max_count->f32_bits, sizeof(max_count_value));
            const std::uint32_t min_gap_frames =
                min_gap == nullptr ? 0u : static_cast<std::uint32_t>(min_gap->integer);
            float recharge_value = 0.0f;
            if (recharge) std::memcpy(&recharge_value, &recharge->f32_bits, sizeof(recharge_value));
            const float min_gap_secs = static_cast<float>(min_gap_frames) / 30.0f;
            const float recharge_secs =
                recharge_value > 0.0f
                    ? static_cast<float>(static_cast<double>(max_count_value) /
                                         (30.0 * static_cast<double>(recharge_value)))
                    : 0.0f;
            const auto put = [&](float v) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                return format_float(bits);
            };
            constructor = "ControlledCount(" + put(max_count_value) + ", " +
                          put(min_gap_secs) + ", " + put(recharge_secs) + ", " +
                          (field_bool(trait, "AllowIndividualRepeats") ? "TRUE" : "FALSE") +
                          ")";
        } else {
            continue;
        }
        emit_method(out, indent, "Frequency", "Set",
                    {ctx.headers.resolve_domain(
                         SymDomain::named(SymDomain::Kind::OpinionReactionType),
                         static_cast<std::int32_t>(index)),
                     constructor});
    }
}


std::string value_expr(const Value *value, const std::string &field_name,
                       const std::string &scope, const Context &ctx,
                       const char *fallback = "0") {
    if (value == nullptr) {
        return fallback;
    }
    return format_expr(*value, field_name, scope, ctx);
}

std::string particle_expr(const Value *value, const std::string &field_name,
                          const std::string &scope, const Context &ctx) {
    if (value != nullptr && value->kind == ValueKind::Int && value->integer > 0) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                          static_cast<std::int32_t>(value->integer));
    }
    return value_expr(value, field_name, scope, ctx);
}


bool engine_graphic_shape(const Value &value) {
    return has_field(value, "Type") && has_field(value, "BankIndex") &&
           has_field(value, "AnimStep") && has_field(value, "RenderSizeX") &&
           has_field(value, "AdditiveAlpha");
}

bool value_changed(const Value &value, const Value *base, const std::string &name,
                   const Context &ctx) {
    return base == nullptr || !sem_equal(value, *base, name, ctx, ctx);
}

void emit_engine_graphic(const std::string &path, const Value &value, const Value *base,
                         const std::string &scope, const Context &ctx, std::string &out,
                         int indent) {
    const Value *type_value = find_value_field(value, "Type");
    const int32_t type = type_value == nullptr ? 0 : static_cast<int32_t>(type_value->integer);
    for (const char *name : {"Type", "BankIndex", "AnimStep", "RenderSizeX", "AdditiveAlpha"}) {
        const Value *member = find_value_field(value, name);
        if (member == nullptr) {
            continue;
        }
        const Value *base_member =
            base == nullptr ? nullptr : find_value_field(*base, name);
        if (!value_changed(*member, base_member, name, ctx)) {
            continue;
        }
        std::string expr;
        if (std::string(name) == "Type") {
            expr = ctx.headers.resolve_domain(
                SymDomain::named(SymDomain::Kind::EngineGraphicType), type);
        } else if (std::string(name) == "BankIndex") {
            const int32_t bank = static_cast<int32_t>(member->integer);
            if (bank > 0) {
                if (type == 1 || type == 2) {
                    expr = ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Texture), bank);
                } else if (type == 3) {
                    expr = ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Particle), bank);
                } else if (type == 4 || type == 5) {
                    expr = ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Mesh), bank);
                } else {
                    expr = std::to_string(bank);
                }
            } else {
                expr = std::to_string(bank);
            }
        } else {
            expr = format_expr(*member, name, scope, ctx);
        }
        emit_line(out, indent, path + "." + name, expr);
    }
}

bool has_field(const Value &value, const std::string &name) {
    return find_value_field(value, name) != nullptr;
}

std::optional<std::string> compound_constructor(const Value &value,
                                                const std::string &scope,
                                                const Context &ctx) {
    const auto field = [&](const char *name) { return find_value_field(value, name); };
    if (value.fields.size() == 1 && value.fields[0].first == "IntValue" &&
        value.fields[0].second.kind == ValueKind::Int) {
        const std::uint32_t packed =
            static_cast<std::uint32_t>(value.fields[0].second.integer);
        std::string expr = "CRGBColour(" + std::to_string((packed >> 16) & 0xff) + ", " +
                           std::to_string((packed >> 8) & 0xff) + ", " +
                           std::to_string(packed & 0xff);
        const int alpha = (packed >> 24) & 0xff;
        if (alpha != 255) {
            expr += ", " + std::to_string(alpha);
        }
        expr += ")";
        return expr;
    }
    if (has_field(value, "NormalMusicID") && has_field(value, "DangerMusicID") &&
        has_field(value, "LoopCount")) {
        return "CMusicSetEntry(" +
               value_expr(field("NormalMusicID"), "NormalMusicID", scope, ctx) + ", " +
               value_expr(field("DangerMusicID"), "DangerMusicID", scope, ctx) + ", " +
               value_expr(field("LoopCount"), "LoopCount", scope, ctx) + ")";
    }
    if (has_field(value, "ID") && has_field(value, "Filename") &&
        has_field(value, "Gain") && has_field(value, "CacheToHD")) {
        return "CMusicEntry(" + value_expr(field("ID"), "ID", scope, ctx) + ", " +
               value_expr(field("Filename"), "Filename", scope, ctx) + ", " +
               value_expr(field("Gain"), "Gain", scope, ctx) + ", " +
               (field("CacheToHD") != nullptr && field("CacheToHD")->boolean ? "TRUE"
                                                                            : "FALSE") +
               ")";
    }
    if (has_field(value, "ID") && has_field(value, "Filename") &&
        has_field(value, "SymbolFilename") && has_field(value, "LipSyncBankHandle") &&
        has_field(value, "DVD")) {
        return "CSoundBankEntry(" + value_expr(field("ID"), "ID", scope, ctx) + ", " +
               value_expr(field("Filename"), "Filename", scope, ctx) + ", " +
               value_expr(field("SymbolFilename"), "SymbolFilename", scope, ctx) + ", " +
               value_expr(field("LipSyncBankHandle"), "LipSyncBankHandle", scope, ctx) +
               ", " +
               (field("DVD") != nullptr && field("DVD")->boolean ? "TRUE" : "FALSE") + ")";
    }
    if (value.fields.size() == 2 && has_field(value, "ID") &&
        has_field(value, "Filename")) {
        return "CAtmosBankEntry(" + value_expr(field("ID"), "ID", scope, ctx) + ", " +
               value_expr(field("Filename"), "Filename", scope, ctx) + ")";
    }
    if (value.fields.size() == 2 && has_field(value, "X") && has_field(value, "Y")) {
        return "CVector2D(" + value_expr(field("X"), "X", scope, ctx) + ", " +
               value_expr(field("Y"), "Y", scope, ctx) + ")";
    }
    if (value.fields.size() == 3 && has_field(value, "X") && has_field(value, "Y") &&
        has_field(value, "Z")) {
        return "CVector3D(" + value_expr(field("X"), "X", scope, ctx) + ", " +
               value_expr(field("Y"), "Y", scope, ctx) + ", " +
               value_expr(field("Z"), "Z", scope, ctx) + ")";
    }
    if (has_field(value, "Particle1Index") && has_field(value, "Particle2Index") &&
        has_field(value, "Particle1Value") && has_field(value, "Particle2Value")) {
        return "CBlendedParticleEffectSet(" +
               particle_expr(field("Particle1Index"), "Particle1Index", scope, ctx) + ", " +
               value_expr(field("Particle1Value"), "Particle1Value", scope, ctx) + ", " +
               particle_expr(field("Particle2Index"), "Particle2Index", scope, ctx) + ", " +
               value_expr(field("Particle2Value"), "Particle2Value", scope, ctx) + ")";
    }
    if (has_field(value, "PeriodSecs") && has_field(value, "ParticleEffect") &&
        has_field(value, "StartPoint") && has_field(value, "EndPoint")) {
        const Value *period = field("PeriodSecs");
        const Value *start = field("StartPoint");
        const Value *end = field("EndPoint");
        const std::size_t pairs = start == nullptr ? 0 : start->list.size();
        std::string expr = "CObjectAugmentationParticleSet(" +
                           value_expr(period, "PeriodSecs", scope, ctx) + ", " +
                           std::to_string(pairs);
        if (pairs != 0) {
            expr += ", " + particle_expr(field("ParticleEffect"), "ParticleEffect", scope, ctx);
            expr += ", " + particle_expr(field("ParticleEffectToBlendTo"),
                                         "ParticleEffectToBlendTo", scope, ctx);
            expr += ", " + particle_expr(field("AttackParticleEffect"),
                                         "AttackParticleEffect", scope, ctx);
            expr += ", " + particle_expr(field("AttackParticleEffectToBlendTo"),
                                         "AttackParticleEffectToBlendTo", scope, ctx);
            for (std::size_t i = 0; i < pairs; ++i) {
                expr += ", " + value_expr(i < start->list.size() ? &start->list[i] : nullptr,
                                          "StartPoint", scope, ctx, "\"\"");
                expr += ", " + value_expr(
                    end != nullptr && i < end->list.size() ? &end->list[i] : nullptr,
                    "EndPoint", scope, ctx, "\"\"");
            }
        }
        expr += ")";
        return expr;
    }
    if (has_field(value, "NumExplosions") && has_field(value, "RingRadius") &&
        has_field(value, "AngleOffset") && has_field(value, "SecondsBeforeSpawnNewRing")) {
        return "CExplosionRing(" +
               value_expr(field("RingRadius"), "RingRadius", scope, ctx) + ", " +
               value_expr(field("NumExplosions"), "NumExplosions", scope, ctx) + ", " +
               value_expr(field("AngleOffset"), "AngleOffset", scope, ctx) + ", " +
               value_expr(field("SecondsBeforeSpawnNewRing"), "SecondsBeforeSpawnNewRing",
                          scope, ctx) + ")";
    }
    if (has_field(value, "AttachmentObjectName") &&
        has_field(value, "GenerationCutoffDistance") && has_field(value, "DummyObject")) {
        return "CParticleAttachmentInfo(" +
               value_expr(field("AttachmentObjectName"), "AttachmentObjectName", scope, ctx) +
               ", " + particle_expr(field("ParticleIndex"), "ParticleIndex", scope, ctx) +
               ", " + value_expr(field("GenerationCutoffDistance"),
                                  "GenerationCutoffDistance", scope, ctx) +
               ", " + (field("DummyObject") != nullptr && field("DummyObject")->boolean
                           ? "TRUE" : "FALSE") + ")";
    }
    if (has_field(value, "HighValue") && has_field(value, "LowValue") &&
        value.fields.size() == 2) {
        return "CFloatRange(" + value_expr(field("HighValue"), "HighValue", scope, ctx) +
               ", " + value_expr(field("LowValue"), "LowValue", scope, ctx) + ")";
    }
    if (has_field(value, "ExperienceMultiplierNumSecondsForAttenuation") &&
        has_field(value, "ExperienceMultiplierParamDecayDeltaPerSecond") &&
        has_field(value, "ComboMultiplier")) {
        std::string expr = "CComboMultiplierData(" +
            value_expr(field("ExperienceMultiplierNumSecondsForAttenuation"),
                       "ExperienceMultiplierNumSecondsForAttenuation", scope, ctx) + ", " +
            value_expr(field("ExperienceMultiplierParamDecayDeltaPerSecond"),
                       "ExperienceMultiplierParamDecayDeltaPerSecond", scope, ctx);
        const Value *map = field("ComboMultiplier");
        if (map != nullptr) {
            for (const auto &[key, mapped] : map->map) {
                expr += ", Add(" +
                    ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::DamageAttribute),
                        static_cast<std::int32_t>(key.integer)) + ", " +
                    value_expr(&mapped, "ComboMultiplier", scope, ctx) + ")";
            }
        }
        expr += ")";
        return expr;
    }
    if (value.fields.size() == 1 && value.fields[0].first == "StatIncreases" &&
        value.fields[0].second.kind == ValueKind::Map) {
        std::string expr = "CHeroStatIncreaseData(";
        bool first = true;
        const auto &map = value.fields[0].second.map;
        for (const auto &[key, mapped] : map) {
            if (!first) expr += ", ";
            first = false;
            expr += "Add(" +
                ctx.headers.resolve_domain(
                    SymDomain::named(SymDomain::Kind::HeroExperienceStatCategory),
                    static_cast<std::int32_t>(key.integer)) + ", " +
                value_expr(&mapped, "StatIncreases", scope, ctx) + ")";
        }
        expr += ")";
        return expr;
    }
    if (has_field(value, "Attack") && has_field(value, "Knockdown") &&
        value.fields.size() == 2) {
        return "CWeaponTrailGraphicSet(" +
               value_expr(field("Attack"), "Attack", scope, ctx) + ", " +
               value_expr(field("Knockdown"), "Knockdown", scope, ctx) + ")";
    }
    if (value.fields.size() == 1 && value.fields[0].first == "Params" &&
        value.fields[0].second.kind == ValueKind::List) {
        std::string expr = "CReverbEnvironmentEntry(";
        const auto &params = value.fields[0].second.list;
        for (std::size_t i = 0; i < params.size(); ++i) {
            if (i) expr += ", ";
            expr += value_expr(&params[i], "Params", scope, ctx);
        }
        expr += ")";
        return expr;
    }
    if (has_field(value, "AttackCombo") && has_field(value, "Multiplier")) {
        std::string expr = "CAttackHistoryCombo(" +
                           value_expr(field("Multiplier"), "Multiplier", scope, ctx);
        const Value *combo = field("AttackCombo");
        if (combo != nullptr) {
            for (const Value &item : combo->list) {
                if (item.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *type = find_value_field(item, "first");
                const Value *attribute = find_value_field(item, "second");
                expr += ", Add(" +
                        ctx.headers.resolve_domain(
                            SymDomain::named(SymDomain::Kind::AttackHistoryType),
                            type == nullptr ? 0 : static_cast<std::int32_t>(type->integer)) +
                        ", " +
                        ctx.headers.resolve_domain(
                            SymDomain::named(SymDomain::Kind::DamageAttribute),
                            attribute == nullptr ? -1
                                                 : static_cast<std::int32_t>(attribute->integer)) +
                        ")";
            }
        }
        expr += ")";
        return expr;
    }
    return std::nullopt;
}

void emit_reaction_matches(const Value &matches, const Context &ctx, std::string &out,
                           int indent) {
    const Value *elements = find_value_field(matches, "Elements");
    if (elements == nullptr) {
        return;
    }
    for (const Value &entry : elements->list) {
        if (entry.kind != ValueKind::Variant && entry.kind != ValueKind::Struct) {
            continue;
        }
        const std::string reaction =
            ctx.headers.resolve_domain(
                SymDomain::named(SymDomain::Kind::OpinionReactionType),
                field_i32(entry, "ReactionType"));
        std::string constructor;
        switch (entry.tag) {
        case 0: {
            const Value *lower = find_value_field(entry, "LowerBoundOnAxis");
            const Value *interval = find_value_field(entry, "InvIntervalOnAxis");
            float lower_f = 0.0f;
            float interval_f = 0.0f;
            if (lower) std::memcpy(&lower_f, &lower->f32_bits, sizeof(lower_f));
            if (interval) std::memcpy(&interval_f, &interval->f32_bits, sizeof(interval_f));
            const float upper_f = interval_f == 0.0f ? lower_f : lower_f + 1.0f / interval_f;
            std::uint32_t upper_bits = 0;
            std::memcpy(&upper_bits, &upper_f, sizeof(upper_bits));
            std::vector<std::string> args = {
                std::to_string(field_i32(entry, "Axis")),
                lower == nullptr ? "0.0" : format_float(lower->f32_bits),
                format_float(upper_bits),
                float_value(find_value_field(entry, "MShiftZero")),
                float_value(find_value_field(entry, "MShiftWeight")),
                float_value(find_value_field(entry, "RShiftZero")),
                float_value(find_value_field(entry, "RShiftWeight"))};
            constructor = "Secondary(";
            for (std::size_t i = 0; i < args.size(); ++i) {
                if (i) constructor += ", ";
                constructor += args[i];
            }
            constructor += ")";
            break;
        }
        case 1: {
            std::vector<std::string> args = {
                std::to_string(field_i32(entry, "Axis")),
                float_value(find_value_field(entry, "CentreOnAxis")),
                float_value(find_value_field(entry, "RadiusOnAxis")),
                float_value(find_value_field(entry, "MShiftZero")),
                float_value(find_value_field(entry, "MShiftWeight")),
                float_value(find_value_field(entry, "RShiftZero")),
                float_value(find_value_field(entry, "RShiftWeight"))};
            constructor = "SecondaryCentred(";
            for (std::size_t i = 0; i < args.size(); ++i) {
                if (i) constructor += ", ";
                constructor += args[i];
            }
            constructor += ")";
            break;
        }
        case 2: {
            const Value *xc = find_value_field(entry, "XCentre");
            const Value *yc = find_value_field(entry, "YCentre");
            float xc_f = 0.0f, yc_f = 0.0f;
            if (xc) std::memcpy(&xc_f, &xc->f32_bits, sizeof(xc_f));
            if (yc) std::memcpy(&yc_f, &yc->f32_bits, sizeof(yc_f));
            const Value *nix = find_value_field(entry, "NegInvXRadius");
            const Value *pix = find_value_field(entry, "PosInvXRadius");
            const Value *niy = find_value_field(entry, "NegInvYRadius");
            const Value *piy = find_value_field(entry, "PosInvYRadius");
            float nix_f = 0.0f, pix_f = 0.0f, niy_f = 0.0f, piy_f = 0.0f;
            if (nix) std::memcpy(&nix_f, &nix->f32_bits, sizeof(nix_f));
            if (pix) std::memcpy(&pix_f, &pix->f32_bits, sizeof(pix_f));
            if (niy) std::memcpy(&niy_f, &niy->f32_bits, sizeof(niy_f));
            if (piy) std::memcpy(&piy_f, &piy->f32_bits, sizeof(piy_f));
            const float xmin = xc_f - (nix_f == 0.0f ? 0.0f : 1.0f / nix_f);
            const float xmax = xc_f + (pix_f == 0.0f ? 0.0f : 1.0f / pix_f);
            const float ymin = yc_f - (niy_f == 0.0f ? 0.0f : 1.0f / niy_f);
            const float ymax = yc_f + (piy_f == 0.0f ? 0.0f : 1.0f / piy_f);
            const auto put = [&](float v) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                return format_float(bits);
            };
            constructor = "Pyramid(" +
                std::to_string(field_i32(entry, "XAxisOpinionType")) + ", " +
                put(xmin) + ", " + put(xc_f) + ", " + put(xmax) + ", " +
                std::to_string(field_i32(entry, "YAxisOpinionType")) + ", " +
                put(ymin) + ", " + put(yc_f) + ", " + put(ymax) + ")";
            break;
        }
        case 3: {
            const Value *xc = find_value_field(entry, "XCentre");
            const Value *yc = find_value_field(entry, "YCentre");
            float xc_f = 0.0f, yc_f = 0.0f;
            if (xc) std::memcpy(&xc_f, &xc->f32_bits, sizeof(xc_f));
            if (yc) std::memcpy(&yc_f, &yc->f32_bits, sizeof(yc_f));
            const Value *nix = find_value_field(entry, "NegInvXRadius");
            const Value *pix = find_value_field(entry, "PosInvXRadius");
            const Value *niy = find_value_field(entry, "NegInvYRadius");
            const Value *piy = find_value_field(entry, "PosInvYRadius");
            float nix_f = 0.0f, pix_f = 0.0f, niy_f = 0.0f, piy_f = 0.0f;
            if (nix) std::memcpy(&nix_f, &nix->f32_bits, sizeof(nix_f));
            if (pix) std::memcpy(&pix_f, &pix->f32_bits, sizeof(pix_f));
            if (niy) std::memcpy(&niy_f, &niy->f32_bits, sizeof(niy_f));
            if (piy) std::memcpy(&piy_f, &piy->f32_bits, sizeof(piy_f));
            const float xmin = xc_f - (nix_f == 0.0f ? 0.0f : 1.0f / nix_f);
            const float xmax = xc_f + (pix_f == 0.0f ? 0.0f : 1.0f / pix_f);
            const float ymin = yc_f - (niy_f == 0.0f ? 0.0f : 1.0f / niy_f);
            const float ymax = yc_f + (piy_f == 0.0f ? 0.0f : 1.0f / piy_f);
            const auto put = [&](float v) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                return format_float(bits);
            };
            constructor = "MRBlob(" + put(ymin) + ", " + put(ymax) + ", " +
                          put(xmin) + ", " + put(xmax) + ", " +
                          float_value(find_value_field(entry, "ScarinessShift")) + ", " +
                          float_value(find_value_field(entry, "AgreeablenessShift")) + ", " +
                          float_value(find_value_field(entry, "AttractivenessShift")) + ")";
            break;
        }
        default:
            continue;
        }
        emit_method(out, indent, "Matches", "Add", {reaction, constructor});
    }
}

void emit_flammable(const Value &flammable, const Context &ctx, std::string &out,
                    int indent) {
    const Value *map = find_value_field(flammable, "ContainmentMap");
    if (map == nullptr) {
        const Value *set = find_value_field(flammable, "EffectCreationSet");
        map = set == nullptr ? nullptr : find_value_field(*set, "ContainmentMap");
    }
    if (map == nullptr) {
        return;
    }
    for (const auto &[key, value] : map->map) {
        std::string key_text;
        if (key.kind == ValueKind::DefString) {
            key_text = resolve_string(key.integer, ctx);
        } else if (key.kind == ValueKind::String) {
            key_text = key.text;
        }
        emit_method(out, indent, "EffectCreationSet", "Add",
                    {quote(key_text),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                                static_cast<int32_t>(value.integer))});
    }
}

void emit_opinion_personality(const Value &personality, const Value *base,
                              std::string &out, int indent) {
    const Value *traits = &personality;
    if (find_value_field(personality, "Data") == nullptr) {
        traits = find_value_field(personality, "PersonalityTraits");
        if (traits == nullptr) {
            return;
        }
    }
    const Value *data = find_value_field(*traits, "Data");
    if (data == nullptr || data->list.size() < 180) {
        return;
    }
    std::vector<int32_t> base_data(180, 0);
    if (base != nullptr) {
        if (const Value *base_traits = find_value_field(*base, "PersonalityTraits")) {
            if (const Value *base_values = find_value_field(*base_traits, "Data");
                base_values != nullptr) {
                for (std::size_t i = 0; i < 180 && i < base_values->list.size(); ++i) {
                    base_data[i] = static_cast<int32_t>(base_values->list[i].integer);
                }
            }
        }
    }
    for (int index = 0; index < 5; ++index) {
        const std::size_t start = static_cast<std::size_t>(index) * 36;
        bool changed = false;
        for (std::size_t i = 0; i < 36; ++i) {
            if (static_cast<int32_t>(data->list[start + i].integer) != base_data[start + i]) {
                changed = true;
                break;
            }
        }
        if (!changed) {
            continue;
        }
        std::array<float, 9> slots{};
        for (int slot = 0; slot < 9; ++slot) {
            const std::size_t offset = start + static_cast<std::size_t>(slot) * 4;
            const std::uint32_t bits =
                static_cast<std::uint32_t>(data->list[offset].integer) |
                (static_cast<std::uint32_t>(data->list[offset + 1].integer) << 8) |
                (static_cast<std::uint32_t>(data->list[offset + 2].integer) << 16) |
                (static_cast<std::uint32_t>(data->list[offset + 3].integer) << 24);
            std::memcpy(&slots[slot], &bits, sizeof(bits));
        }
        std::vector<std::string> args = {std::to_string(index)};
        for (int slot = 0; slot < 9; ++slot) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &slots[slot], sizeof(bits));
            args.push_back(format_float_plain(bits));
        }
        std::swap(args[7], args[8]);
        emit_method(out, indent, "PersonalityTraits", "Set", args);
    }
}

void emit_opinion_deed_effects(const Value &definition, const Context &ctx,
                               std::string &out, int indent) {
    const Value *effects = find_value_field(definition, "Effects");
    if (effects == nullptr) {
        return;
    }
    for (const Value &effect : effects->list) {
        if (effect.kind != ValueKind::Struct) {
            continue;
        }
        const int32_t opinion = field_i32(effect, "Opinion");
        const float run_in = [&] {
            const Value *v = find_value_field(effect, "OffsetPerFrameRunIn");
            return v == nullptr ? 0.0f : [&] {
                float f = 0.0f;
                std::memcpy(&f, &v->f32_bits, sizeof(f));
                return f;
            }();
        }();
        const int32_t frames_in = field_i32(effect, "FramesToRunIn");
        const float run_out = [&] {
            const Value *v = find_value_field(effect, "OffsetPerFrameRunOut");
            return v == nullptr ? 0.0f : [&] {
                float f = 0.0f;
                std::memcpy(&f, &v->f32_bits, sizeof(f));
                return f;
            }();
        }();
        const int32_t frames_out = field_i32(effect, "FramesToRunOut");
        const float peak = static_cast<float>(static_cast<double>(run_in) * frames_in);
        const float run_in_secs =
            (frames_in == 1 && run_in == peak) ? 0.0f : static_cast<float>(frames_in) / 15.0f;
        const float persist =
            static_cast<float>(static_cast<double>(peak) +
                               static_cast<double>(run_out) * frames_out);
        const float run_out_secs =
            (frames_out == 1 && run_out == (persist - peak))
                ? 0.0f
                : static_cast<float>(frames_out) / 15.0f;
        emit_method(out, indent, "Effects", "Add",
                    {ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Opinion),
                                                opinion),
                     [&] { float f = peak; std::uint32_t b; std::memcpy(&b, &f, 4); return format_float_plain(b); }(),
                     [&] { float f = run_in_secs; std::uint32_t b; std::memcpy(&b, &f, 4); return format_float_plain(b); }(),
                     [&] { float f = run_out_secs; std::uint32_t b; std::memcpy(&b, &f, 4); return format_float_plain(b); }(),
                     [&] { float f = persist; std::uint32_t b; std::memcpy(&b, &f, 4); return format_float_plain(b); }()});
    }
}

void emit_degradable(const Value &definition, const Context &ctx, std::string &out,
                     int indent) {
    const Value *degradations = find_value_field(definition, "Degradations");
    if (degradations == nullptr) {
        return;
    }
    for (const Value &degradation : degradations->list) {
        if (degradation.kind != ValueKind::Struct) {
            continue;
        }
        const Value *health = find_value_field(degradation, "HealthPercentage");
        emit_method(out, indent, "Degradations", "Add",
                    {health == nullptr ? "0" : format_float_plain(health->f32_bits),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Mesh),
                                                field_i32(degradation, "BankIndex")),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                                field_i32(degradation, "SmashParticleEmitter")),
                     field_bool(degradation, "BlocksNavigation") ? "TRUE" : "FALSE"});
    }
}

void emit_replaceable_meshes(const Value &definition, const Context &ctx,
                             std::string &out, int indent) {
    const Value *meshes = find_value_field(definition, "Meshes");
    const Value *entries = meshes == nullptr ? nullptr : find_value_field(*meshes, "Vector");
    if (entries == nullptr) {
        return;
    }
    for (const Value &entry : entries->list) {
        if (entry.kind != ValueKind::Struct) {
            continue;
        }
        emit_method(out, indent, "Meshes", "Add",
                    {ctx.headers.resolve_domain(
                         SymDomain::named(SymDomain::Kind::EngineGraphicType),
                         field_i32(entry, "GraphicType")),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Mesh),
                                                field_i32(entry, "BankIndex"))});
    }
}

void emit_entity_sound(const Value &sound_map, const Context &ctx, std::string &out,
                       int indent) {
    const auto resolve_sound = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Sound), value);
    };
    if (const Value *sounds = find_value_field(sound_map, "SoundMap")) {
        for (const Value &item : sounds->list) {
            if (item.kind != ValueKind::Struct) {
                continue;
            }
            const Value *key = find_value_field(item, "first");
            const Value *entry = find_value_field(item, "second");
            if (key == nullptr || entry == nullptr) {
                continue;
            }
            std::string key_text;
            if (const auto found = ctx.crc_to_str.find(static_cast<std::uint32_t>(key->integer));
                found != ctx.crc_to_str.end()) {
                key_text = found->second;
            } else {
                key_text = "snd_" + std::to_string(key->integer);
            }
            const int32_t first = field_i32(*entry, "FirstSound");
            const int32_t last = field_i32(*entry, "LastSound");
            if (first == last) {
                emit_method(out, indent, "SoundMap", "AddSingle",
                            {quote(key_text), resolve_sound(first)});
            } else {
                emit_method(out, indent, "SoundMap", "AddPair",
                            {quote(key_text), resolve_sound(first), resolve_sound(last)});
            }
        }
    }
    if (const Value *criteria = find_value_field(sound_map, "CriteriaMap")) {
        for (const Value &item : criteria->list) {
            if (item.kind != ValueKind::Struct) {
                continue;
            }
            const Value *key = find_value_field(item, "first");
            const Value *entry = find_value_field(item, "second");
            if (key == nullptr || entry == nullptr) {
                continue;
            }
            std::string key_text;
            if (const auto found = ctx.crc_to_str.find(static_cast<std::uint32_t>(key->integer));
                found != ctx.crc_to_str.end()) {
                key_text = found->second;
            } else {
                key_text = "snd_" + std::to_string(key->integer);
            }
            emit_method(out, indent, "SoundMap", "AddCriteria",
                        {quote(key_text), quote(value_string(*entry, ctx))});
        }
    }
}

void emit_special_effects(const Value &effects, const Context &ctx, std::string &out,
                          int indent) {
    const Value *map = find_value_field(effects, "ContainmentMap");
    if (map == nullptr) {
        return;
    }
    for (const auto &[key, value] : map->map) {
        std::string key_text;
        if (const auto found = ctx.crc_to_str.find(static_cast<std::uint32_t>(key.integer));
            found != ctx.crc_to_str.end()) {
            key_text = found->second;
        } else {
            key_text = "eff_" + std::to_string(key.integer);
        }
        emit_method(out, indent, "SpecialEffects", "Add",
                    {quote(key_text),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                                static_cast<int32_t>(value.integer))});
    }
}

void emit_components(const Value &components, const Context &ctx, std::string &out,
                     int indent) {
    const Value *entries = find_value_field(components, "Entries");
    if (entries == nullptr) {
        return;
    }
    for (const Value &entry : entries->list) {
        if (entry.kind != ValueKind::Struct) {
            continue;
        }
        const std::string name = field_string(entry, "Name", ctx);
        std::vector<std::string> args = {quote(name)};
        if (field_bool(entry, "Override")) {
            args.push_back("TRUE");
        }
        emit_method(out, indent, "Components", "Add", args);
    }
}

void emit_weapon_trails(const Value &trails, const Context &ctx, std::string &out,
                        int indent) {
    if (trails.kind != ValueKind::Map) {
        return;
    }
    const auto resolve = [&](SymDomain::Kind kind, int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(kind), value);
    };
    for (const auto &[key, mapped] : trails.map) {
        if (mapped.kind != ValueKind::Struct) {
            continue;
        }
        const int32_t augmentation = static_cast<int32_t>(key.integer);
        const int32_t attack = field_i32(mapped, "Attack");
        const int32_t knockdown = field_i32(mapped, "Knockdown");
        out.append(static_cast<std::size_t>(indent) * 4, ' ');
        out += "WeaponTrails[";
        out += resolve(SymDomain::Kind::ObjectAugmentation, augmentation);
        out += "] CWeaponTrailGraphicSet(";
        out += resolve(SymDomain::Kind::Texture, attack);
        out += ", ";
        out += resolve(SymDomain::Kind::Texture, knockdown);
        out += ");\n";
    }
}

void emit_appearance_graphics(const Value &graphics_field, const Context &ctx,
                              std::string &out, int indent) {
    const Value *graphics = find_value_field(graphics_field, "Graphics");
    if (graphics == nullptr) {
        return;
    }
    for (const Value &entry : graphics->list) {
        if (entry.kind != ValueKind::Struct) {
            continue;
        }
        const int32_t type = field_i32(entry, "Type");
        const int32_t bank = field_i32(entry, "BankIndex");
        const int32_t morph = field_i32(entry, "MorphType");
        const Value *size = find_value_field(entry, "RenderSizeX");
        const Value *start = find_value_field(entry, "StartMorphStrength");
        const Value *end = find_value_field(entry, "EndMorphStrength");
        SymDomain::Kind bank_domain = SymDomain::Kind::Mesh;
        if (type == 1 || type == 2) {
            bank_domain = SymDomain::Kind::Texture;
        } else if (type == 3) {
            bank_domain = SymDomain::Kind::Particle;
        }
        emit_method(out, indent, "Graphics", "Add",
                    {ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::EngineGraphicType),
                                                type),
                     ctx.headers.resolve_domain(SymDomain::named(bank_domain), bank),
                     size == nullptr ? "0.0" : format_float_plain(size->f32_bits),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::HeroMorph),
                                                morph),
                     start == nullptr ? "0.0" : format_float_plain(start->f32_bits),
                     end == nullptr ? "0.0" : format_float_plain(end->f32_bits)});
    }
}

void emit_hero_morph(const Value &hero, const Context &ctx, std::string &out,
                     int indent) {
    const auto resolve = [&](SymDomain::Kind kind, int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(kind), value);
    };
    if (const Value *skeletal = find_value_field(hero, "SkeletalMorphs")) {
        if (const Value *entries = find_value_field(*skeletal, "Morphs")) {
            for (const Value &item : entries->list) {
                if (item.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *key = find_value_field(item, "first");
                const Value *entry = find_value_field(item, "second");
                if (entry == nullptr) {
                    continue;
                }
                const int32_t morph = key == nullptr ? 0 : static_cast<int32_t>(key->integer);
                emit_method(out, indent, "SkeletalMorphs", "Add",
                            {resolve(SymDomain::Kind::HeroMorph, morph),
                             quote(field_string(*entry, "MorphName", ctx)),
                             std::to_string(field_i32(*entry, "Priority")),
                             field_bool(*entry, "SupressesLowerMorphs") ? "TRUE" : "FALSE"});
            }
        }
    }
    if (const Value *morphs = find_value_field(hero, "TextureMorphs")) {
        if (const Value *entries = find_value_field(*morphs, "Morphs")) {
            for (const Value &item : entries->list) {
                if (item.kind != ValueKind::Struct) {
                    continue;
                }
                const Value *key = find_value_field(item, "first");
                const Value *entry = find_value_field(item, "second");
                if (entry == nullptr) {
                    continue;
                }
                emit_method(out, indent, "TextureMorphs", "Add",
                            {std::to_string(field_i32(*entry, "TextureLayer")),
                             resolve(SymDomain::Kind::HeroMorph,
                                     key == nullptr ? 0 : static_cast<int32_t>(key->integer)),
                             resolve(SymDomain::Kind::Texture,
                                     field_i32(*entry, "ReplacingTextureIndex")),
                             resolve(SymDomain::Kind::Texture,
                                     field_i32(*entry, "BankIndex")),
                             resolve(SymDomain::Kind::Blend, field_i32(*entry, "Blend"))});
            }
        }
    }
    const auto particle = [&](const char *field_name, const char *call_name) {
        const Value *morphs = find_value_field(hero, field_name);
        if (morphs == nullptr) {
            return;
        }
        const Value *entries = find_value_field(*morphs, "Morphs");
        if (entries == nullptr) {
            return;
        }
        for (const Value &item : entries->list) {
            if (item.kind != ValueKind::Struct) {
                continue;
            }
            const Value *key = find_value_field(item, "first");
            const Value *entry = find_value_field(item, "second");
            if (entry == nullptr) {
                continue;
            }
            const int32_t morph = key == nullptr ? 0 : static_cast<int32_t>(key->integer);
            const int32_t location = field_i32(*entry, "Location");
            const std::string location_name = field_string(*entry, "LocationName", ctx);
            const int32_t location_index = field_i32(*entry, "LocationIndex");
            const int32_t particle_index = field_i32(*entry, "ParticleIndex");
            const int32_t second_particle = field_i32(*entry, "SecondParticleIndex");
            const Value *start = find_value_field(*entry, "StartAtMorphStrength");
            const Value *end = find_value_field(*entry, "EndAtMorphStrength");
            std::vector<std::string> args = {
                resolve(SymDomain::Kind::HeroMorph, morph),
                resolve(SymDomain::Kind::Particle, particle_index),
                start == nullptr ? "0.0" : format_float_plain(start->f32_bits),
                end == nullptr ? "0.0" : format_float_plain(end->f32_bits),
                resolve(SymDomain::Kind::MeshLocation, location), quote(location_name),
                std::to_string(location_index)};
            if (entry != nullptr && find_value_field(*entry, "Blending") != nullptr &&
                field_bool(*entry, "Blending")) {
                args.insert(args.begin() + 2,
                            resolve(SymDomain::Kind::Particle, second_particle));
                emit_method(out, indent, field_name, "AddBlend", args);
            } else {
                emit_method(out, indent, field_name, call_name, args);
            }
        }
    };
    particle("ParticleMorphs", "Add");
    particle("IdleParticleMorphs", "Add");
}

void unlower_body(const ParsedEntry &entry, const ParsedEntry *base,
                  const std::string &scope, const Context &ctx, std::string &out,
                  int indent) {
    std::set<std::string> seen_fields;
    const Value *components_value = nullptr;
    const Value *sound_map_value = nullptr;
    for (const auto &[name, value] : entry.fields) {
        if (ignored_field(name) || !seen_fields.insert(name).second) {
            continue;
        }
        if (name == "Components") {
            components_value = &value;
            continue;
        }
        if (name == "SoundMap") {
            sound_map_value = &value;
            continue;
        }
        const Value *base_value = nullptr;
        if (base != nullptr) {
            for (const auto &[base_name, candidate] : base->fields) {
                if (base_name == name) {
                    base_value = &candidate;
                    break;
                }
            }
        }
        if (base_value != nullptr && sem_equal(value, *base_value, name, ctx, ctx)) {
            continue;
        }
        if (name == "Animation" &&
            (scope == "CAnimatingObjectDef" || scope == "CAppearanceDef")) {
            if (value.kind == ValueKind::Struct) {
                emit_animation_set(value, base_value, ctx, out, indent);
            }
            continue;
        }
        if (scope == "CCreatureDef") {
            if (name == "Expressions") {
                emit_expression_set(value, ctx, out, indent);
                continue;
            }
            if (name == "WoundMorphs") {
                emit_wound_morphs(value, ctx, out, indent);
                continue;
            }
            if (name == "RandomAppearanceMorph") {
                emit_random_appearance_morph(value, ctx, out, indent);
                continue;
            }
        }
        if (scope == "CHeroMorphDef") {
            if (name == "SkeletalMorphs" || name == "TextureMorphs" ||
                name == "ParticleMorphs" || name == "IdleParticleMorphs") {
                continue;
            }
        }
        if (name == "Matches") {
            emit_reaction_matches(value, ctx, out, indent);
            continue;
        }
        if (name == "Frequency") {
            emit_frequency_traits(value, ctx, out, indent);
            continue;
        }
        if (scope == "CFlammableDef" && name == "EffectCreationSet") {
            emit_flammable(value, ctx, out, indent);
            continue;
        }
        if (scope == "OPINION_PERSONALITY" && name == "PersonalityTraits") {
            emit_opinion_personality(value, base_value, out, indent);
            continue;
        }
        if (scope == "OPINION_DEED_EFFECTS" && name == "Effects") {
            emit_opinion_deed_effects(value, ctx, out, indent);
            continue;
        }
        if (scope == "OPINION_SOURCE" &&
            (name == "BinaryReaction" || name == "BinaryOpinion")) {
            continue;
        }
        if (scope == "CDegradableDef" && name == "Degradations") {
            emit_degradable(value, ctx, out, indent);
            continue;
        }
        if (scope == "CReplaceableMeshDef" && name == "Meshes") {
            emit_replaceable_meshes(value, ctx, out, indent);
            continue;
        }
        if (name == "SpecialEffects") {
            emit_special_effects(value, ctx, out, indent);
            continue;
        }
        if (name == "WeaponTrails") {
            emit_weapon_trails(value, ctx, out, indent);
            continue;
        }
        if (scope == "CAppearanceModifierDef" && name == "Graphics") {
            emit_appearance_graphics(value, ctx, out, indent);
            continue;
        }
        unlower_value(name, name, value, base_value, scope, ctx, out, indent, false);
    }
    if (sound_map_value != nullptr) {
        emit_entity_sound(*sound_map_value, ctx, out, indent);
    }
    if (components_value != nullptr) {
        emit_components(*components_value, ctx, out, indent);
    }
    if (scope == "CHeroMorphDef") {
        Value combined;
        combined.kind = ValueKind::Struct;
        combined.fields = entry.fields;
        emit_hero_morph(combined, ctx, out, indent);
    }
}

void emit_definition(const BinaryEntry &binary_entry, const ParsedEntry &entry,
                     const Context &ctx, const std::map<std::string, std::string> &parents,
                     std::string &out) {
    out += entry.is_template ? "#definition_template " : "#definition ";
    out += entry.def_type;
    out.push_back(' ');
    out += binary_entry.file_name;
    if (const auto parent = parents.find(binary_entry.file_name);
        parent != parents.end()) {
        out += " specialises ";
        out += parent->second;
    }
    out.push_back('\n');
    const auto base_it = ctx.nulldefs.find(entry.def_type);
    const ParsedEntry *base = base_it == ctx.nulldefs.end() ? nullptr : &base_it->second;
    unlower_body(entry, base, entry.def_type, ctx, out, 1);

    if (binary_entry.sub_defs) {
        for (const SubDefRecord &record : *binary_entry.sub_defs) {
            if (record.def_index >= ctx.entries.size()) {
                continue;
            }
            const ParsedEntry &sub = ctx.entries[record.def_index];
            std::string tag;
            if (const auto it = ctx.crc_to_str.find(record.name_crc);
                it != ctx.crc_to_str.end()) {
                tag = it->second;
            } else if (!sub.def_type.empty()) {
                tag = sub.def_type;
            } else {
                continue;
            }
            out.append(static_cast<std::size_t>(1) * 4, ' ');
            out += "<" + tag + ">\n";
            const auto sub_base_it = ctx.nulldefs.find(tag);
            const ParsedEntry *sub_base =
                sub_base_it == ctx.nulldefs.end() ? nullptr : &sub_base_it->second;
            unlower_body(sub, sub_base, tag, ctx, out, 2);
            out.append(static_cast<std::size_t>(1) * 4, ' ');
            out += "<\\" + tag + ">\n";
        }
    }
    out += "#end_definition\n\n";
}

} // namespace

bool decompile_binary_to_text(const std::string &directory, const std::string &binary,
                              const std::string &defs_directory, std::string &out,
                              DecompileStats &stats, std::string &error) {
    Names names;
    if (!Names::load(directory + "/names.bin", names, error)) {
        return false;
    }
    DefBinary defs;
    if (!DefBinary::load(directory + "/" + binary + ".bin", names, defs, error)) {
        return false;
    }
    Context ctx;
    build_context(defs, names, HeaderResolver::build(defs_directory), defs_directory, ctx, stats);
    const auto parents = build_parent_map(defs_directory);
    out.clear();
    stats.entries = defs.entries.size();
    for (const BinaryEntry &entry : defs.entries) {
        if (entry.file_name.empty() || entry.file_name.rfind("NULLDEF_", 0) == 0) {
            continue;
        }
        emit_definition(entry, ctx.entries[entry.global_index], ctx, parents, out);
        ++stats.definitions;
    }
    return true;
}

bool diff_one_binary_to_text(const std::string &original_directory,
                             const std::string &modified_directory,
                             const std::string &defs_directory,
                             const std::string &binary, const Names &original_names,
                             const Names &modified_names, const HeaderResolver &headers,
                             const std::map<std::string, std::string> &parents,
                             std::string &out, DecompileStats &stats, std::string &error) {
    namespace fs = std::filesystem;
    const std::string original_path = original_directory + "/" + binary + ".bin";
    const std::string modified_path = modified_directory + "/" + binary + ".bin";
    if (!fs::exists(original_path) || !fs::exists(modified_path)) {
        return true;
    }
    DefBinary original_bin;
    DefBinary modified_bin;
    if (!DefBinary::load(original_path, original_names,
                         original_bin, error) ||
        !DefBinary::load(modified_path, modified_names, modified_bin, error)) {
        return false;
    }
    Context original_ctx;
    Context modified_ctx;
    build_context(original_bin, original_names, headers, defs_directory, original_ctx, stats);
    build_context(modified_bin, modified_names, headers, defs_directory, modified_ctx, stats);

    std::map<std::string, const BinaryEntry *> original_by_name;
    std::map<std::string, const ParsedEntry *> original_defs;
    for (std::size_t i = 0; i < original_bin.entries.size(); ++i) {
        const BinaryEntry &entry = original_bin.entries[i];
        if (entry.file_name.empty() || entry.file_name.rfind("NULLDEF_", 0) == 0) {
            continue;
        }
        original_by_name[entry.file_name] = &entry;
        original_defs[entry.file_name] = &original_ctx.entries[i];
    }

    for (std::size_t i = 0; i < modified_bin.entries.size(); ++i) {
        const BinaryEntry &entry = modified_bin.entries[i];
        if (entry.file_name.empty() || entry.file_name.rfind("NULLDEF_", 0) == 0) {
            continue;
        }
        const auto original_entry = original_by_name.find(entry.file_name);
        bool changed = original_entry == original_by_name.end();
        if (!changed) {
            const ParsedEntry &original_def = *original_defs[entry.file_name];
            const ParsedEntry &modified_def = modified_ctx.entries[i];
            Value modified_value;
            modified_value.kind = ValueKind::Struct;
            modified_value.fields = modified_def.fields;
            Value original_value;
            original_value.kind = ValueKind::Struct;
            original_value.fields = original_def.fields;
            changed = !sem_equal(modified_value, original_value, "", modified_ctx,
                                 original_ctx);

            if (!changed) {
                const auto &modified_subs = entry.sub_defs;
                const auto &original_subs = original_entry->second->sub_defs;
                if (modified_subs.has_value() != original_subs.has_value()) {
                    changed = true;
                } else if (modified_subs) {
                    if (modified_subs->size() != original_subs->size()) {
                        changed = true;
                    } else {
                        for (std::size_t sub = 0; sub < modified_subs->size(); ++sub) {
                            const SubDefRecord &m = (*modified_subs)[sub];
                            const SubDefRecord &o = (*original_subs)[sub];
                            if (m.name_crc != o.name_crc ||
                                m.def_index >= modified_ctx.entries.size() ||
                                o.def_index >= original_ctx.entries.size()) {
                                changed = true;
                                break;
                            }
                            Value mv;
                            mv.kind = ValueKind::Struct;
                            mv.fields = modified_ctx.entries[m.def_index].fields;
                            Value ov;
                            ov.kind = ValueKind::Struct;
                            ov.fields = original_ctx.entries[o.def_index].fields;
                            if (!sem_equal(mv, ov, "", modified_ctx, original_ctx)) {
                                changed = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!changed) {
            continue;
        }
        if (original_entry == original_by_name.end()) {
            ++stats.added;
        } else {
            ++stats.modified;
        }
        emit_definition(entry, modified_ctx.entries[i], modified_ctx, parents, out);
    }
    return true;
}

bool diff_binaries_to_text(const std::string &original_directory,
                           const std::string &modified_directory,
                           const std::string &defs_directory, std::string &out,
                           DecompileStats &stats, std::string &error) {
    Names original_names;
    Names modified_names;
    if (!Names::load(original_directory + "/names.bin", original_names, error) ||
        !Names::load(modified_directory + "/names.bin", modified_names, error)) {
        return false;
    }
    const HeaderResolver headers = HeaderResolver::build(defs_directory);
    const auto parents = build_parent_map(defs_directory);
    out.clear();
    for (const std::string binary : {"game", "frontend", "script"}) {
        if (!diff_one_binary_to_text(original_directory, modified_directory, defs_directory,
                                     binary, original_names, modified_names, headers,
                                     parents, out, stats, error)) {
            return false;
        }
    }
    return true;
}

namespace {

bool file_in_scope(const std::string &path, const std::string &binary) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    const bool script = normalized.find("/ScriptDefs/") != std::string::npos ||
                        normalized.rfind("ScriptDefs/", 0) == 0;
    const bool frontend = normalized.find("/FrontEndDefs/") != std::string::npos ||
                          normalized.rfind("FrontEndDefs/", 0) == 0;
    if (binary == "script") return script;
    if (binary == "frontend") return frontend;
    return !script && !frontend;
}

void patch_scope(const std::string &defs_directory, const std::string &binary,
                 std::map<std::string, std::string> &replacements, std::string &error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(defs_directory, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) {
            continue;
        }
        const fs::path path = it->path();
        if (!file_in_scope(normalized_path(path), binary)) {
            continue;
        }
        const std::string extension = path.extension().string();
        if (extension != ".def" && extension != ".tpl") {
            continue;
        }
        const std::string filename = path.filename().string();
        if (filename.rfind("z_mod_decompiled", 0) == 0 ||
            filename.find("_deprecated.") != std::string::npos) {
            continue;
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            continue;
        }
        std::string text{std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>()};
        ParseResult parsed = parse_source(text, FileId::anonymous());
        if (std::holds_alternative<ParseError>(parsed)) {
            continue;
        }
        const SourceAst &ast = std::get<SourceAst>(parsed);
        std::vector<std::pair<std::pair<std::size_t, std::size_t>, std::string>> edits;
        for (const auto &item : ast.items) {
            if (item.kind != Item::Kind::Definition) {
                continue;
            }
            const auto &definition = item.definition;
            const auto found = replacements.find(definition.value.name);
            if (found == replacements.end()) {
                continue;
            }
            std::string replacement = found->second;
            if (definition.value.is_template) {
                if (replacement.rfind("#definition ", 0) == 0) {
                    replacement.replace(0, 12, "#definition_template ");
                }
            } else if (replacement.rfind("#definition_template ", 0) == 0) {
                replacement.replace(0, 21, "#definition ");
            }
            edits.push_back({{definition.span.start, definition.span.end}, replacement});
            replacements.erase(found);
        }
        if (edits.empty()) {
            continue;
        }
        std::sort(edits.begin(), edits.end(),
                  [](const auto &a, const auto &b) { return a.first.first > b.first.first; });
        for (const auto &[span, replacement] : edits) {
            text.replace(span.first, span.second - span.first, replacement);
        }
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            error = "cannot write " + path.string();
            return;
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!out) {
            error = "cannot write " + path.string();
            return;
        }
    }
}

std::string added_path(const std::string &defs_directory, const std::string &binary) {
    if (binary == "script") {
        return defs_directory + "/ScriptDefs/z_mod_decompiled.def";
    }
    if (binary == "frontend") {
        return defs_directory + "/FrontEndDefs/z_mod_decompiled.def";
    }
    return defs_directory + "/z_mod_decompiled.def";
}

} // namespace

bool patch_binaries(const std::string &original_directory,
                    const std::string &modified_directory,
                    const std::string &defs_directory, DecompileStats &stats,
                    std::string &error) {
    Names original_names;
    Names modified_names;
    if (!Names::load(original_directory + "/names.bin", original_names, error) ||
        !Names::load(modified_directory + "/names.bin", modified_names, error)) {
        return false;
    }
    const HeaderResolver headers = HeaderResolver::build(defs_directory);
    const auto parents = build_parent_map(defs_directory);

    for (const std::string binary : {"game", "frontend", "script"}) {
        std::string diff_text;
        DecompileStats local;
        if (!diff_one_binary_to_text(original_directory, modified_directory, defs_directory,
                                     binary, original_names, modified_names, headers,
                                     parents, diff_text, local, error)) {
            return false;
        }
        stats.modified += local.modified;
        stats.added += local.added;
        stats.errors.insert(stats.errors.end(), local.errors.begin(), local.errors.end());

        std::map<std::string, std::string> replacements;
        if (!diff_text.empty()) {
            ParseResult parsed = parse_source(diff_text, FileId::anonymous());
            if (std::holds_alternative<ParseError>(parsed)) {
                error = "cannot parse generated decompiler text";
                return false;
            }
            const SourceAst &ast = std::get<SourceAst>(parsed);
            for (const auto &item : ast.items) {
                if (item.kind != Item::Kind::Definition) {
                    continue;
                }
                const auto &definition = item.definition;
                const std::size_t start = definition.span.start;
                const std::size_t end = definition.span.end;
                if (start >= end || end > diff_text.size()) {
                    continue;
                }
                replacements[definition.value.name] =
                    diff_text.substr(start, end - start);
            }
        }

        patch_scope(defs_directory, binary, replacements, error);
        if (!error.empty()) {
            return false;
        }

        const std::string path = added_path(defs_directory, binary);
        namespace fs = std::filesystem;
        std::error_code ec;
        if (replacements.empty()) {
            fs::remove(path, ec);
            continue;
        }
        if (const auto parent = fs::path(path).parent_path(); !parent.empty()) {
            fs::create_directories(parent, ec);
        }
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            error = "cannot write " + path;
            return false;
        }
        for (const auto &[name, block] : replacements) {
            (void)name;
            out << block;
            if (block.empty() || block.back() != '\n') {
                out << '\n';
            }
            out << '\n';
        }
    }
    return true;
}

} // namespace fable::defs::compiler
