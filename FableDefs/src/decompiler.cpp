#include "fable/defs/compiler/decompiler.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <utility>

#include "fable/defs/compiler/binary_reader.hpp"
#include "fable/defs/compiler/def_reader.hpp"
#include "fable/defs/compiler/wire.hpp"

namespace fable::defs::compiler {
namespace {

struct Context {
    std::map<std::int32_t, std::string> idx_to_name;
    std::map<std::int32_t, std::string> off_to_str;
    std::map<std::uint32_t, std::string> crc_to_str;
    std::map<std::string, ParsedEntry> nulldefs;
    std::vector<ParsedEntry> entries;
};

bool build_context(const DefBinary &binary, const Names &names, Context &context,
                   DecompileStats &stats) {
    context = Context{};
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
    return true;
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

std::string format_float(std::uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    return out.str();
}

std::string format_expr(const Value &value, const std::string &field_name,
                        const Context &ctx) {
    switch (value.kind) {
    case ValueKind::Int:
        return std::to_string(value.integer);
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
            return std::to_string(value.integer);
        }
        return index_name(value.integer, ctx);
    case ValueKind::Enum:
    case ValueKind::Flags:
        return std::to_string(value.integer);
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

void unlower_value(const std::string &path, const std::string &field_name,
                   const Value &value, const Value *base, const Context &ctx,
                   std::string &out, int indent, bool force);

void unlower_struct(const std::string &path, const Value &value, const Context &ctx,
                    std::string &out, int indent, bool force) {
    bool emitted = false;
    for (const auto &[name, child] : value.fields) {
        if (ignored_field(name)) {
            continue;
        }
        unlower_value(path + "." + name, name, child, nullptr, ctx, out, indent, true);
        emitted = true;
    }
    if (!emitted && force) {
        emit_line(out, indent, path, "0");
    }
}

void unlower_list(const std::string &path, const std::string &field_name,
                  const Value &value, const Value *base, const Context &ctx,
                  std::string &out, int indent) {
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
            unlower_struct(item_path, item, ctx, out, indent, true);
        } else {
            out.append(static_cast<std::size_t>(indent) * 4, ' ');
            out += path;
            out += ".Add(";
            out += format_expr(item, field_name, ctx);
            out += ");\n";
        }
    }
}

void unlower_map(const std::string &path, const std::string &field_name,
                 const Value &value, const Value *base, const Context &ctx,
                 std::string &out, int indent) {
    if (base != nullptr && base->kind == ValueKind::Map && !base->map.empty()) {
        emit_clear(out, indent, path);
    }
    for (const auto &[key, mapped] : value.map) {
        const std::string key_text = format_expr(key, field_name, ctx);
        const std::string item_path = path + "[" + key_text + "]";
        if (mapped.kind == ValueKind::Struct || mapped.kind == ValueKind::Variant) {
            unlower_struct(item_path, mapped, ctx, out, indent, true);
        } else {
            emit_line(out, indent, item_path, format_expr(mapped, field_name, ctx));
        }
    }
}

void unlower_value(const std::string &path, const std::string &field_name,
                   const Value &value, const Value *base, const Context &ctx,
                   std::string &out, int indent, bool force) {
    switch (value.kind) {
    case ValueKind::Struct:
        unlower_struct(path, value, ctx, out, indent, force);
        break;
    case ValueKind::Variant:
        // Method-call reconstruction for tagged values is added with the
        // dedicated decompiler passes; skip it in the generic pass.
        break;
    case ValueKind::List:
        unlower_list(path, field_name, value, base, ctx, out, indent);
        break;
    case ValueKind::Map:
        unlower_map(path, field_name, value, base, ctx, out, indent);
        break;
    case ValueKind::Opaque:
        break;
    default:
        emit_line(out, indent, path, format_expr(value, field_name, ctx));
        break;
    }
}

void unlower_body(const ParsedEntry &entry, const ParsedEntry *base,
                  const Context &ctx, std::string &out, int indent) {
    for (const auto &[name, value] : entry.fields) {
        if (ignored_field(name)) {
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
        unlower_value(name, name, value, base_value, ctx, out, indent, false);
    }
}

void emit_definition(const BinaryEntry &binary_entry, const ParsedEntry &entry,
                     const Context &ctx, std::string &out) {
    out += entry.is_template ? "#definition_template " : "#definition ";
    out += entry.def_type;
    out.push_back(' ');
    out += binary_entry.file_name;
    out.push_back('\n');
    const auto base_it = ctx.nulldefs.find(entry.def_type);
    const ParsedEntry *base = base_it == ctx.nulldefs.end() ? nullptr : &base_it->second;
    unlower_body(entry, base, ctx, out, 1);

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
            out.push_back('\n');
            out.append(static_cast<std::size_t>(1) * 4, ' ');
            out += "<" + tag + ">\n";
            const auto sub_base_it = ctx.nulldefs.find(tag);
            const ParsedEntry *sub_base =
                sub_base_it == ctx.nulldefs.end() ? nullptr : &sub_base_it->second;
            unlower_body(sub, sub_base, ctx, out, 2);
            out.append(static_cast<std::size_t>(1) * 4, ' ');
            out += "<\\" + tag + ">\n";
        }
    }
    out += "#end_definition\n\n";
}

} // namespace

bool decompile_binary_to_text(const std::string &directory, const std::string &binary,
                              std::string &out, DecompileStats &stats,
                              std::string &error) {
    Names names;
    if (!Names::load(directory + "/names.bin", names, error)) {
        return false;
    }
    DefBinary defs;
    if (!DefBinary::load(directory + "/" + binary + ".bin", names, defs, error)) {
        return false;
    }
    Context ctx;
    build_context(defs, names, ctx, stats);
    out.clear();
    stats.entries = defs.entries.size();
    for (const BinaryEntry &entry : defs.entries) {
        if (entry.file_name.empty() || entry.file_name.rfind("NULLDEF_", 0) == 0) {
            continue;
        }
        emit_definition(entry, ctx.entries[entry.global_index], ctx, out);
        ++stats.definitions;
    }
    return true;
}

bool diff_binaries_to_text(const std::string &original_directory,
                           const std::string &modified_directory, std::string &out,
                           DecompileStats &stats, std::string &error) {
    Names original_names;
    Names modified_names;
    if (!Names::load(original_directory + "/names.bin", original_names, error) ||
        !Names::load(modified_directory + "/names.bin", modified_names, error)) {
        return false;
    }

    out.clear();
    for (const std::string binary : {"game", "frontend", "script"}) {
        DefBinary original_bin;
        DefBinary modified_bin;
        if (!DefBinary::load(original_directory + "/" + binary + ".bin",
                             original_names, original_bin, error) ||
            !DefBinary::load(modified_directory + "/" + binary + ".bin",
                             modified_names, modified_bin, error)) {
            return false;
        }
        Context original_ctx;
        Context modified_ctx;
        build_context(original_bin, original_names, original_ctx, stats);
        build_context(modified_bin, modified_names, modified_ctx, stats);

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
            }
            if (!changed) {
                continue;
            }
            if (original_entry == original_by_name.end()) {
                ++stats.added;
            } else {
                ++stats.modified;
            }
            emit_definition(entry, modified_ctx.entries[i], modified_ctx, out);
        }
    }
    return true;
}

} // namespace fable::defs::compiler
