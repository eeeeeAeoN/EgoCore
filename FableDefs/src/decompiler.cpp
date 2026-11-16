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
    const DefBinary *binary = nullptr;
    std::map<std::int32_t, std::string> idx_to_name;
    std::map<std::string, std::size_t> name_to_entry_idx;
    std::map<std::int32_t, std::string> off_to_str;
    std::map<std::uint32_t, std::string> crc_to_str;
    std::map<std::string, ParsedEntry> nulldefs;
    std::vector<ParsedEntry> entries;
    HeaderResolver headers;
    bool is_frontend = false;
    std::unordered_map<std::string, std::int32_t> ui_effective_type;
    mutable std::int32_t active_ui_type = -1;
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
    context.binary = &binary;
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
            context.name_to_entry_idx[entry.file_name] = i;
        }
        if (!entry.file_name.empty() && entry.file_name.rfind("NULLDEF_", 0) == 0) {
            context.nulldefs[entry.def_name] = context.entries[i];
        }
        context.crc_to_str[crc32(entry.def_name)] = entry.def_name;
        if (!entry.file_name.empty()) {
            context.crc_to_str[crc32(entry.file_name)] = entry.file_name;
        }
    }
    context.is_frontend = context.nulldefs.count("FRONT_END") > 0;
    // Retail frontend.bin leaves uninitialized non-zero bytes in NULLDEF_CONTROL_SCHEME
    // booleans (CControlsDef::Alloc does not zero them); CControlsDef::Transfer defaults
    // all 6 booleans to false, matching retail game.bin's clean NULLDEF_CONTROL_SCHEME.
    if (auto ctrl_it = context.nulldefs.find("CONTROL_SCHEME");
        ctrl_it != context.nulldefs.end()) {
        for (auto &[fname, fval] : ctrl_it->second.fields) {
            if (fval.kind == DecodedValueKind::Bool &&
                (fname == "ToggleZTarget" || fname == "ToggleSpells" ||
                 fname == "ToggleSneak" || fname == "ToggleExpressionMenu" ||
                 fname == "ToggleExpressionShift" ||
                 fname == "FlourishNeedsAttackButtonHeld")) {
                fval.boolean = false;
            }
        }
    }
    collect_text_crc(defs_directory, context);
    static const char *const kSoundMapEvents[] = {
        "AMBIENT_SOUND",
        "ARROW_FLY_BY",
        "CAMERA_LIST_PUSH_DOWN",
        "CAMERA_LIST_PUSH_ERROR",
        "CAMERA_LIST_PUSH_UP",
        "CREATURE_CONTINUOUS",
        "CROWD_BOO",
        "CROWD_CHEER",
        "END",
        "FIRE_DEFAULT",
        "INIT",
        "LOOP",
        "PICKUP_SOUND",
        "POWERUP1",
        "POWERUP2",
        "POWERUP3",
        "PROJECTILE_WEAPON_ZOOM_IN",
        "PUSH_SPECIALITY_CAMERA_MODE",
        "SET_AUTO_CONTROL",
        "TELEPORT_AWAY_WANK",
        "Z_TARGET_START",
    };
    for (const char *event_name : kSoundMapEvents) {
        add_crc_string(context.crc_to_str, event_name);
    }
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

bool has_parent_cycle(const std::map<std::string, std::string> &parents,
                      const std::string &child, const std::string &candidate_parent) {
    std::set<std::string> visited;
    visited.insert(child);
    std::string curr = candidate_parent;
    while (!curr.empty()) {
        if (!visited.insert(curr).second) {
            return true;
        }
        const auto it = parents.find(curr);
        if (it == parents.end()) {
            break;
        }
        curr = it->second;
    }
    return false;
}

std::map<std::string, std::string> build_binary_parent_map(const DefBinary &binary) {
    const std::size_t n = binary.entries.size();
    std::vector<bool> is_valid(n, false);
    for (std::size_t i = 0; i < n; ++i) {
        const BinaryEntry &entry = binary.entries[i];
        if (!entry.file_name.empty() && entry.file_name.rfind("NULLDEF_", 0) != 0) {
            is_valid[i] = true;
        }
    }

    std::vector<std::set<std::size_t>> ancestors(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (!is_valid[i] || !binary.entries[i].sub_defs) {
            continue;
        }
        const std::string &self_type = binary.entries[i].def_name;
        for (const SubDefRecord &rec : *binary.entries[i].sub_defs) {
            if (rec.owner_index < 0) {
                continue;
            }
            const std::size_t owner = static_cast<std::size_t>(rec.owner_index);
            if (owner != i && owner < n && is_valid[owner] &&
                binary.entries[owner].def_name == self_type) {
                ancestors[i].insert(owner);
            }
        }
    }

    std::vector<int> depth(n, -1);
    std::vector<int> state(n, 0);
    auto compute_depth = [&](auto &self, std::size_t u) -> int {
        if (state[u] == 2) {
            return depth[u];
        }
        if (state[u] == 1) {
            return 0;
        }
        state[u] = 1;
        int best = 0;
        for (std::size_t a : ancestors[u]) {
            best = std::max(best, 1 + self(self, a));
        }
        state[u] = 2;
        depth[u] = best;
        return best;
    };
    for (std::size_t i = 0; i < n; ++i) {
        if (is_valid[i]) {
            compute_depth(compute_depth, i);
        }
    }

    std::map<std::string, std::string> parents;
    for (std::size_t i = 0; i < n; ++i) {
        if (!is_valid[i] || ancestors[i].empty()) {
            continue;
        }
        std::size_t best_parent = n;
        int best_depth = -1;
        std::size_t best_anc_count = 0;
        bool best_tpl = false;
        for (std::size_t cand : ancestors[i]) {
            if (ancestors[cand].count(i) != 0 && cand > i) {
                continue;
            }
            const int d = depth[cand];
            const std::size_t ac = ancestors[cand].size();
            const bool tpl = binary.entries[cand].is_template;
            if (best_parent == n || d > best_depth ||
                (d == best_depth && (ac > best_anc_count ||
                                     (ac == best_anc_count && (tpl && !best_tpl))))) {
                best_parent = cand;
                best_depth = d;
                best_anc_count = ac;
                best_tpl = tpl;
            }
        }
        if (best_parent < n && best_parent != i &&
            !has_parent_cycle(parents, binary.entries[i].file_name,
                              binary.entries[best_parent].file_name)) {
            parents[binary.entries[i].file_name] = binary.entries[best_parent].file_name;
        }
    }
    return parents;
}

std::map<std::string, std::string> build_merged_parent_map(
    const DefBinary &binary, const std::string &defs_directory) {
    auto parents = build_binary_parent_map(binary);
    for (const auto &[child, parent] : build_parent_map(defs_directory)) {
        if (!has_parent_cycle(parents, child, parent)) {
            parents[child] = parent;
        }
    }
    return parents;
}

std::string sanitize_def_filename(const std::string &def_type) {
    std::string out;
    out.reserve(def_type.size());
    for (unsigned char c : def_type) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_') {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "misc";
    }
    return out + ".def";
}

std::string heuristic_relative_path(const std::string &binary, const std::string &def_type,
                                    bool is_template) {
    if (binary == "frontend") {
        if (is_template) {
            return "FrontEndDefs/frontend.tpl";
        }
        if (def_type == "UI" || def_type.rfind("UI_", 0) == 0) {
            return "FrontEndDefs/ui.def";
        }
        return "FrontEndDefs/frontend.def";
    }
    if (binary == "script") {
        if (is_template) {
            return "ScriptDefs/script.tpl";
        }
        if (def_type == "CCutsceneDef") {
            return "ScriptDefs/cutscenes.def";
        }
        if (def_type == "CRegionScriptDef") {
            return "ScriptDefs/region_scripts.def";
        }
        return "ScriptDefs/scripts.def";
    }

    if (is_template) {
        if (def_type == "BUILDING") {
            return "building.tpl";
        }
        if (def_type == "CREATURE") {
            return "creature.tpl";
        }
        if (def_type == "OBJECT") {
            return "objects.tpl";
        }
    }

    if (def_type == "OBJECT" || def_type == "OBJECT_FAMILY") {
        return "objects.def";
    }
    if (def_type == "BUILDING") {
        return "buildings.def";
    }
    if (def_type == "CREATURE" || def_type == "CREATURE_ABILITY" ||
        def_type == "CREATURE_GENERATION_FAMILY") {
        return "creatures.def";
    }
    if (def_type == "ARMOUR") {
        return "armour.def";
    }
    if (def_type == "THING" || def_type == "THING_GROUP" || def_type == "CARRY_SLOT") {
        return "things.def";
    }
    if (def_type == "REGION") {
        return "regions.def";
    }
    if (def_type == "VILLAGE" || def_type == "VILLAGER_INTERACTION" ||
        def_type == "SIM_BUILDING" || def_type == "SIM_VOICES" ||
        def_type == "HOLY_SITE") {
        return "villages.def";
    }
    if (def_type == "BRAIN") {
        return "brains.def";
    }
    if (def_type == "FACTION") {
        return "factions.def";
    }
    if (def_type == "EXPRESSION") {
        return "expressions.def";
    }
    if (def_type == "MARKER" || def_type == "SWITCH" ||
        def_type == "PHYSICAL_SWITCH" || def_type == "NOISE") {
        return "markers_and_switches.def";
    }
    if (def_type == "SHOT" || def_type == "LIGHTNING") {
        return "shots.def";
    }
    if (def_type == "MATERIAL") {
        return "materials.def";
    }
    if (def_type == "COMBAT_SEQUENCE" || def_type == "COMBAT_TYPE" ||
        def_type == "ATTACK_PATTERN" || def_type == "HIT_LOCATION" ||
        def_type == "MELEE_COMBAT_KNOCKDOWN_EFFECTS" ||
        def_type == "COMBAT_DIALOGUE_DEF") {
        return "combat.def";
    }
    if (def_type == "HERO_ABILITY" || def_type == "HERO_COMBAT" ||
        def_type == "HERO_MELEE_COMBAT_ABILITY" || def_type == "HERO_STATS" ||
        def_type == "CHeroPostcardGeneratorDef") {
        return "hero.def";
    }
    if (def_type.rfind("SPECIAL_ABILITIES_", 0) == 0) {
        return "special_abilities.def";
    }
    if (def_type == "INVENTORY_TYPE" || def_type == "INVENTORY_CATEGORY") {
        return "inventory.def";
    }
    if (def_type.rfind("OPINION_", 0) == 0) {
        return "opinions.def";
    }
    if (def_type.rfind("CAMERA_", 0) == 0) {
        return "cameras.def";
    }
    if (def_type == "ENGINE_THEME" || def_type == "ENGINE_THEME_GROUP" ||
        def_type == "ENVIRONMENT" || def_type == "ENVIRONMENT_THEME_DAY" ||
        def_type == "SKY" || def_type == "LOCAL_DETAIL_GENERATOR") {
        return "environment.def";
    }
    if (def_type == "ENGINE" || def_type == "ENGINE_VIDEO_OPTIONS" ||
        def_type == "CONFIG_OPTIONS_DEFAULTS_DEF" || def_type == "GLOBAL") {
        return "engine.def";
    }
    if (def_type == "PLAYER" || def_type == "PLAYER_GUI" ||
        def_type == "PLAYER_MOVEMENT" || def_type == "CONTROL_SCHEME" ||
        def_type == "FRONT_END" || def_type == "MESSAGE_EVENT") {
        return "player.def";
    }
    if (def_type == "SOUND_SETUP" || def_type == "SOUND_THEME") {
        return "sound.def";
    }
    if (def_type == "UI" || def_type.rfind("UI_", 0) == 0) {
        return "ui.def";
    }
    return sanitize_def_filename(def_type);
}

bool write_text_file(const std::filesystem::path &path, const std::string &content,
                     std::string &error) {
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            error = "cannot create directory " + path.parent_path().string() + ": " +
                    ec.message();
            return false;
        }
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
        error = "failed writing " + path.string();
        return false;
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
        if (c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string resolve_ref(std::int64_t value, const Context &ctx) {
    if (value == 0) {
        return "NULL";
    }
    const auto it = ctx.idx_to_name.find(static_cast<std::int32_t>(value));
    if (it != ctx.idx_to_name.end()) {
        return valid_identifier(it->second) ? it->second : quote(it->second);
    }
    return std::to_string(value);
}

std::string resolve_integer(std::int64_t value, const std::string &field_name,
                            const std::string &scope, const Context &ctx) {
    if ((scope == "UI" || scope == "CUIDef") &&
        (field_name == "MeshIndex" || field_name == "AnimationIndex")) {
        // CList (8) and CScrollingViewport (13) pass MeshIndex/AnimationIndex to CreateComponent.
        if (ctx.active_ui_type == 8 || ctx.active_ui_type == 13) {
            return resolve_ref(value, ctx);
        }
        // CMouseCursor (32) passes MeshIndex/AnimationIndex to SetGraphicIndex.
        if (ctx.active_ui_type == 32) {
            return ctx.headers.resolve_group("textures.h::EEngineGraphic",
                                             static_cast<std::int32_t>(value),
                                             ctx.is_frontend);
        }
    }
    std::string resolved = ctx.headers.resolve_field(
        scope, field_name, static_cast<std::int32_t>(value), ctx.is_frontend);
    if (resolved == std::to_string(value) && is_reference_field(field_name)) {
        return resolve_ref(value, ctx);
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
                                      std::chars_format::fixed);
    std::string text(buffer, result.ptr);
    if (const auto dot = text.find('.'); dot != std::string::npos) {
        while (text.size() > dot + 2 && text.back() == '0') {
            text.pop_back();
        }
    } else {
        text += ".0";
    }
    if (text == "-0.0") {
        text = "0.0";
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
                                      std::chars_format::fixed);
    std::string text(buffer, result.ptr);
    if (const auto dot = text.find('.'); dot != std::string::npos) {
        while (text.size() > dot + 1 && text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    if (text == "-0") {
        text = "0";
    }
    return text;
}

std::string format_expr(const DecodedValue &value, const std::string &field_name,
                        const std::string &scope, const Context &ctx) {
    switch (value.kind) {
    case DecodedValueKind::Int:
        return resolve_integer(value.integer, field_name, scope, ctx);
    case DecodedValueKind::Float:
        return format_float(value.f32_bits);
    case DecodedValueKind::Bool:
        return value.boolean ? "TRUE" : "FALSE";
    case DecodedValueKind::String:
        return quote(value.text);
    case DecodedValueKind::WString: {
        std::string text;
        for (char16_t ch : value.wide) {
            text.push_back(ch < 128 ? static_cast<char>(ch) : '?');
        }
        return quote(text);
    }
    case DecodedValueKind::PString: {
        std::string text(value.bytes.begin(), value.bytes.end());
        return quote(text);
    }
    case DecodedValueKind::DefString:
        return quote(resolve_string(value.integer, ctx));
    case DecodedValueKind::DefIndex:
        if (is_scalar_def_index(field_name)) {
            return ctx.headers.resolve_field(scope, field_name,
                                             static_cast<std::int32_t>(value.integer),
                                             ctx.is_frontend);
        }
        if (value.integer == 0) {
            return "NULL";
        }
        {
            const std::string name = index_name(value.integer, ctx);
            return valid_identifier(name) ? name : quote(name);
        }
    case DecodedValueKind::Enum:
    case DecodedValueKind::Flags:
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

const DecodedValue *find_value_field(const DecodedValue &value, const std::string &name) {
    for (const auto &[field_name, field_value] : value.fields) {
        if (field_name == name) {
            return &field_value;
        }
    }
    return nullptr;
}

std::int32_t value_i32(const DecodedValue &value) {
    return static_cast<std::int32_t>(value.integer);
}

std::string value_string(const DecodedValue &value, const Context &ctx) {
    if (value.kind == DecodedValueKind::DefString) {
        return resolve_string(value.integer, ctx);
    }
    return value.text;
}

bool sem_equal(const DecodedValue &a, const DecodedValue &b, const std::string &field_name,
               const Context &a_ctx, const Context &b_ctx,
               std::string *mismatch = nullptr);

std::string resolve_anim_key_name(const DecodedValue *key, const DecodedValue &entry, const Context &ctx) {
    std::string anim_name;
    if (const DecodedValue *name = find_value_field(entry, "AnimName")) {
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
    return anim_name;
}

void emit_animation_set(const DecodedValue &set, const DecodedValue *base, const Context &ctx,
                        std::string &out, int indent) {
    const DecodedValue *delay = find_value_field(set, "DefaultDelay");
    const DecodedValue *base_delay = base == nullptr ? nullptr : find_value_field(*base, "DefaultDelay");
    if (delay != nullptr && (base_delay == nullptr || delay->integer != base_delay->integer)) {
        emit_line(out, indent, "Animation.SetDefaultDelay", std::to_string(delay->integer));
    }
    const DecodedValue *group = find_value_field(set, "DefaultGroup");
    const DecodedValue *base_group = base == nullptr ? nullptr : find_value_field(*base, "DefaultGroup");
    if (group != nullptr && group->integer >= 0 &&
        (base_group == nullptr || group->integer != base_group->integer)) {
        emit_method(out, indent, "Animation", "SetDefaultGroupName",
                    {quote(resolve_string(group->integer, ctx))});
    }

    const DecodedValue *anims = find_value_field(set, "Anims");
    if (anims == nullptr || anims->kind != DecodedValueKind::List) {
        return;
    }
    const DecodedValue *base_anims = base == nullptr ? nullptr : find_value_field(*base, "Anims");
    const std::size_t child_count = anims->list.size();
    std::vector<int> child_matched_base(child_count, -1);
    std::vector<bool> base_used(
        base_anims != nullptr && base_anims->kind == DecodedValueKind::List ? base_anims->list.size() : 0,
        false);
    if (base_anims != nullptr && base_anims->kind == DecodedValueKind::List) {
        for (std::size_t ci = 0; ci < child_count; ++ci) {
            const DecodedValue &item = anims->list[ci];
            if (item.kind != DecodedValueKind::Struct) {
                continue;
            }
            for (std::size_t bi = 0; bi < base_anims->list.size(); ++bi) {
                if (!base_used[bi] &&
                    sem_equal(item, base_anims->list[bi], "Anims", ctx, ctx)) {
                    base_used[bi] = true;
                    child_matched_base[ci] = static_cast<int>(bi);
                    break;
                }
            }
        }
    }

    std::int32_t current_group = -1;
    if (base_anims != nullptr && base_anims->kind == DecodedValueKind::List) {
        std::set<std::pair<std::int32_t, std::string>> to_remove;
        for (std::size_t bi = 0; bi < base_anims->list.size(); ++bi) {
            if (base_used[bi]) {
                continue;
            }
            const DecodedValue &bitem = base_anims->list[bi];
            const DecodedValue *bentry = find_value_field(bitem, "second");
            if (bentry == nullptr || bentry->kind != DecodedValueKind::Struct) {
                continue;
            }
            const DecodedValue *bgroup = find_value_field(*bentry, "GroupName");
            const std::int32_t gpos = bgroup == nullptr ? -1 : value_i32(*bgroup);
            const std::string kname =
                resolve_anim_key_name(find_value_field(bitem, "first"), *bentry, ctx);
            to_remove.emplace(gpos, kname);
        }
        if (!to_remove.empty()) {
            // Retail CAnimationSet::Remove erases all entries sharing (key, group);
            // unmark any child entry that shared a removed (key, group) so it gets re-added.
            for (std::size_t ci = 0; ci < child_count; ++ci) {
                if (child_matched_base[ci] < 0) {
                    continue;
                }
                const DecodedValue &bitem =
                    base_anims->list[static_cast<std::size_t>(child_matched_base[ci])];
                const DecodedValue *bentry = find_value_field(bitem, "second");
                if (bentry == nullptr) {
                    continue;
                }
                const DecodedValue *bgroup = find_value_field(*bentry, "GroupName");
                const std::int32_t gpos = bgroup == nullptr ? -1 : value_i32(*bgroup);
                const std::string kname =
                    resolve_anim_key_name(find_value_field(bitem, "first"), *bentry, ctx);
                if (to_remove.count({gpos, kname})) {
                    child_matched_base[ci] = -1;
                }
            }
            for (const auto &[gpos, kname] : to_remove) {
                if (gpos != current_group) {
                    if (current_group >= 0) {
                        emit_method(out, indent, "Animation", "EndGroup", {});
                    }
                    current_group = gpos;
                    if (current_group >= 0) {
                        emit_method(out, indent, "Animation", "StartGroup",
                                    {quote(resolve_string(current_group, ctx))});
                    }
                }
                emit_method(out, indent, "Animation", "Remove", {quote(kname)});
            }
        }
    }

    for (std::size_t ci = 0; ci < child_count; ++ci) {
        const DecodedValue &item = anims->list[ci];
        if (item.kind != DecodedValueKind::Struct || child_matched_base[ci] >= 0) {
            continue;
        }
        const DecodedValue *key = find_value_field(item, "first");
        const DecodedValue *entry = find_value_field(item, "second");
        if (entry == nullptr || entry->kind != DecodedValueKind::Struct) {
            continue;
        }
        const DecodedValue *entry_group = find_value_field(*entry, "GroupName");
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

        const std::string anim_name = resolve_anim_key_name(key, *entry, ctx);
        const int32_t bank = find_value_field(*entry, "BankIndex") != nullptr
                                 ? value_i32(*find_value_field(*entry, "BankIndex"))
                                 : 0;
        std::vector<std::string> args = {
            quote(anim_name),
            ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Anim), bank)};

        if (const DecodedValue *components = find_value_field(*entry, "Components")) {
            for (const DecodedValue &component : components->list) {
                if (component.kind != DecodedValueKind::Variant && component.kind != DecodedValueKind::Struct) {
                    continue;
                }
                const DecodedValue *value = nullptr;
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
                    const DecodedValue *start = find_value_field(component, "StartHandedness");
                    const DecodedValue *end = find_value_field(component, "EndHandedness");
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
                    const DecodedValue *stage = find_value_field(component, "ComboStage");
                    const DecodedValue *id = find_value_field(component, "ComboId");
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
                    const DecodedValue *flourish = find_value_field(component, "MeleeFlourish");
                    const DecodedValue *knockdown = find_value_field(component, "MeleeKnockdown");
                    if (flourish && knockdown) {
                        args.push_back(std::string("CombatMisc(") +
                                       (flourish->boolean ? "TRUE" : "FALSE") + ", " +
                                       (knockdown->boolean ? "TRUE" : "FALSE") + ")");
                    }
                    break;
                }
                case 7: {
                    const DecodedValue *x = find_value_field(component, "TargetOffsetVectorX");
                    const DecodedValue *y = find_value_field(component, "TargetOffsetVectorY");
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
           name == "Skip" || name == "Dirty" || name == "dirty";
}

bool sem_equal(const DecodedValue &a, const DecodedValue &b, const std::string &field_name,
               const Context &a_ctx, const Context &b_ctx,
               std::string *mismatch) {
    const auto fail = [&](const std::string &why) {
        if (mismatch != nullptr && mismatch->empty()) {
            *mismatch = field_name + ": " + why;
        }
        return false;
    };
    const bool a_numeric = a.kind == DecodedValueKind::Int || a.kind == DecodedValueKind::Enum ||
                           a.kind == DecodedValueKind::Flags;
    const bool b_numeric = b.kind == DecodedValueKind::Int || b.kind == DecodedValueKind::Enum ||
                           b.kind == DecodedValueKind::Flags;
    if (a.kind == DecodedValueKind::DefIndex && b.kind == DecodedValueKind::DefIndex) {
        if (is_scalar_def_index(field_name)) {
            return a.integer == b.integer ? true
                                          : fail("scalar DefIndex " + std::to_string(a.integer) +
                                                 " vs " + std::to_string(b.integer));
        }
        const std::string an = index_name(a.integer, a_ctx);
        const std::string bn = index_name(b.integer, b_ctx);
        return an == bn ? true : fail("DefIndex " + an + " vs " + bn);
    }
    if (a.kind == DecodedValueKind::DefString && b.kind == DecodedValueKind::DefString) {
        const std::string as = resolve_string(a.integer, a_ctx);
        const std::string bs = resolve_string(b.integer, b_ctx);
        return as == bs ? true : fail("DefString " + as + " vs " + bs);
    }
    if (a_numeric && b_numeric) {
        if (field_name == "BoneOrMorphIndex") {
            const std::string as = resolve_string(a.integer, a_ctx);
            const std::string bs = resolve_string(b.integer, b_ctx);
            return as == bs ? true : fail("BoneOrMorphIndex " + as + " vs " + bs);
        }
        if (a.integer == b.integer) {
            return true;
        }
        std::string base_field = field_name;
        while (!base_field.empty()) {
            if (base_field.back() == ']') {
                const auto open = base_field.rfind('[');
                if (open == std::string::npos) {
                    break;
                }
                base_field.resize(open);
            } else if (base_field.size() >= 4 &&
                       base_field.compare(base_field.size() - 4, 4, ".val") == 0) {
                base_field.resize(base_field.size() - 4);
            } else {
                break;
            }
        }
        if (is_reference_field(base_field)) {
            const auto ia = a_ctx.idx_to_name.find(static_cast<std::int32_t>(a.integer));
            const auto ib = b_ctx.idx_to_name.find(static_cast<std::int32_t>(b.integer));
            if (ia != a_ctx.idx_to_name.end() && ib != b_ctx.idx_to_name.end()) {
                return ia->second == ib->second
                           ? true
                           : fail("RefInt " + ia->second + " vs " + ib->second);
            }
        }
        return fail("int " + std::to_string(a.integer) + " vs " + std::to_string(b.integer));
    }
    if (a.kind != b.kind) {
        return fail("kind " + std::to_string(static_cast<int>(a.kind)) + " vs " +
                    std::to_string(static_cast<int>(b.kind)));
    }
    switch (a.kind) {
    case DecodedValueKind::Float: {
        const bool same_bits = a.f32_bits == b.f32_bits ||
                               ((a.f32_bits & 0x7FFFFFFFu) == 0 && (b.f32_bits & 0x7FFFFFFFu) == 0);
        return same_bits
                   ? true
                   : fail("float " + format_float(a.f32_bits) + " (" +
                          std::to_string(a.f32_bits) + ") vs " +
                          format_float(b.f32_bits) + " (" +
                          std::to_string(b.f32_bits) + ")");
    }
    case DecodedValueKind::Bool:
        return a.boolean == b.boolean ? true : fail("bool");
    case DecodedValueKind::String:
    case DecodedValueKind::PString:
        return (a.text == b.text && a.bytes == b.bytes) ? true : fail("string " + a.text + " vs " + b.text);
    case DecodedValueKind::WString:
        return a.wide == b.wide ? true : fail("wstring");
    case DecodedValueKind::List: {
        if (a.list.size() != b.list.size()) {
            return fail("list size " + std::to_string(a.list.size()) + " vs " +
                        std::to_string(b.list.size()));
        }
        if (field_name == "Data" && a.list.size() == 180) {
            for (std::size_t off = 0; off < 180; off += 4) {
                const std::uint32_t abits =
                    static_cast<std::uint32_t>(a.list[off].integer) |
                    (static_cast<std::uint32_t>(a.list[off + 1].integer) << 8) |
                    (static_cast<std::uint32_t>(a.list[off + 2].integer) << 16) |
                    (static_cast<std::uint32_t>(a.list[off + 3].integer) << 24);
                const std::uint32_t bbits =
                    static_cast<std::uint32_t>(b.list[off].integer) |
                    (static_cast<std::uint32_t>(b.list[off + 1].integer) << 8) |
                    (static_cast<std::uint32_t>(b.list[off + 2].integer) << 16) |
                    (static_cast<std::uint32_t>(b.list[off + 3].integer) << 24);
                const bool same = abits == bbits ||
                                  ((abits & 0x7FFFFFFFu) == 0 && (bbits & 0x7FFFFFFFu) == 0);
                if (!same) {
                    return fail("Data[" + std::to_string(off) + "]: float " +
                                format_float(abits) + " vs " + format_float(bbits));
                }
            }
            return true;
        }
        if (field_name == "Anims") {
            std::vector<bool> used(b.list.size(), false);
            for (std::size_t i = 0; i < a.list.size(); ++i) {
                bool matched = false;
                for (std::size_t j = 0; j < b.list.size(); ++j) {
                    if (!used[j] && sem_equal(a.list[i], b.list[j], "Anims[]", a_ctx, b_ctx)) {
                        used[j] = true;
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    return sem_equal(a.list[i], b.list[i], field_name + "[" + std::to_string(i) + "]",
                                     a_ctx, b_ctx, mismatch);
                }
            }
            return true;
        }
        for (std::size_t i = 0; i < a.list.size(); ++i) {
            if (!sem_equal(a.list[i], b.list[i], field_name + "[" + std::to_string(i) + "]",
                           a_ctx, b_ctx, mismatch)) {
                return false;
            }
        }
        return true;
    }
    case DecodedValueKind::Map: {
        if (a.map.size() != b.map.size()) {
            return fail("map size " + std::to_string(a.map.size()) + " vs " +
                        std::to_string(b.map.size()));
        }
        for (std::size_t i = 0; i < a.map.size(); ++i) {
            if (!sem_equal(a.map[i].first, b.map[i].first, field_name + ".key", a_ctx, b_ctx, mismatch) ||
                !sem_equal(a.map[i].second, b.map[i].second, field_name + ".val", a_ctx, b_ctx, mismatch)) {
                return false;
            }
        }
        return true;
    }
    case DecodedValueKind::Struct:
    case DecodedValueKind::Variant: {
        if (a.tag != b.tag || a.fields.size() != b.fields.size()) {
            return fail("struct tag/size " + std::to_string(a.tag) + "(" +
                        std::to_string(a.fields.size()) + ") vs " + std::to_string(b.tag) + "(" +
                        std::to_string(b.fields.size()) + ")");
        }
        for (std::size_t i = 0; i < a.fields.size(); ++i) {
            if (a.fields[i].first != b.fields[i].first) {
                return fail(std::string("field name ") + a.fields[i].first + " vs " +
                            b.fields[i].first);
            }
            if (!sem_equal(a.fields[i].second, b.fields[i].second, a.fields[i].first,
                           a_ctx, b_ctx, mismatch)) {
                return false;
            }
        }
        return true;
    }
    default:
        return a.integer == b.integer ? true : fail("default int");
    }
}

bool has_field(const DecodedValue &value, const std::string &name);
bool engine_graphic_shape(const DecodedValue &value);
void emit_engine_graphic(const std::string &path, const DecodedValue &value, const DecodedValue *base,
                         const std::string &scope, const Context &ctx, std::string &out,
                         int indent);

std::optional<std::string> compound_constructor(const DecodedValue &value,
                                                const std::string &scope,
                                                const Context &ctx);

void unlower_value(const std::string &path, const std::string &field_name,
                   const DecodedValue &value, const DecodedValue *base, const std::string &scope,
                   const Context &ctx, std::string &out, int indent, bool force);

void unlower_struct(const std::string &path, const DecodedValue &value,
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
                  const DecodedValue &value, const DecodedValue *base, const std::string &scope,
                  const Context &ctx, std::string &out, int indent) {
    if (base != nullptr && base->kind == DecodedValueKind::List && !base->list.empty()) {
        emit_clear(out, indent, path);
    }
    if (value.list.empty()) {
        return;
    }
    const bool struct_elements = value.list.front().kind == DecodedValueKind::Struct ||
                                 value.list.front().kind == DecodedValueKind::Variant;
    for (std::size_t i = 0; i < value.list.size(); ++i) {
        const std::string item_path = path + "[" + std::to_string(i) + "]";
        const DecodedValue &item = value.list[i];
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
                 const DecodedValue &value, const DecodedValue *base, const std::string &scope,
                 const Context &ctx, std::string &out, int indent) {
    if (base != nullptr && base->kind == DecodedValueKind::Map && !base->map.empty()) {
        emit_clear(out, indent, path);
    }
    for (const auto &[key, mapped] : value.map) {
        std::string key_text;
        if (key.kind == DecodedValueKind::Int || key.kind == DecodedValueKind::Enum ||
            key.kind == DecodedValueKind::Flags) {
            key_text = ctx.headers.resolve_map_key(scope, field_name,
                                                   static_cast<std::int32_t>(key.integer));
        } else {
            key_text = format_expr(key, field_name, scope, ctx);
        }
        const std::string item_path = path + "[" + key_text + "]";
        if (mapped.kind == DecodedValueKind::Struct) {
            if (auto constructor = compound_constructor(mapped, scope, ctx)) {
                emit_line(out, indent, item_path, *constructor);
            } else {
                unlower_struct(item_path, mapped, scope, ctx, out, indent, true);
            }
        } else if (mapped.kind == DecodedValueKind::Variant) {
            unlower_struct(item_path, mapped, scope, ctx, out, indent, true);
        } else {
            emit_line(out, indent, item_path, format_expr(mapped, field_name, scope, ctx));
        }
    }
}

void unlower_value(const std::string &path, const std::string &field_name,
                   const DecodedValue &value, const DecodedValue *base, const std::string &scope,
                   const Context &ctx, std::string &out, int indent, bool force) {
    switch (value.kind) {
    case DecodedValueKind::Struct:
        if (engine_graphic_shape(value)) {
            emit_engine_graphic(path, value, base, scope, ctx, out, indent);
        } else if (auto constructor = compound_constructor(value, scope, ctx)) {
            emit_line(out, indent, path, *constructor);
        } else if (value.fields.size() == 1 && value.fields[0].first == "IntValue" &&
            value.fields[0].second.kind == DecodedValueKind::Int) {
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
    case DecodedValueKind::Variant:
        if (value.type_name == "CPhysicalPrimitiveInit") {
            std::string constructor;
            switch (value.tag) {
            case 1: {
                const DecodedValue *name = find_value_field(value, "BaseName");
                const DecodedValue *radius = find_value_field(value, "Radius");
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
                const DecodedValue *name = find_value_field(value, "BaseName");
                const DecodedValue *radius = find_value_field(value, "Radius");
                const DecodedValue *height = find_value_field(value, "Height");
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
    case DecodedValueKind::List:
        unlower_list(path, field_name, value, base, scope, ctx, out, indent);
        break;
    case DecodedValueKind::Map:
        unlower_map(path, field_name, value, base, scope, ctx, out, indent);
        break;
    case DecodedValueKind::Opaque:
        break;
    default:
        emit_line(out, indent, path, format_expr(value, field_name, scope, ctx));
        break;
    }
}


int32_t field_i32(const DecodedValue &value, const std::string &name, int32_t fallback = 0) {
    const DecodedValue *field = find_value_field(value, name);
    return field == nullptr ? fallback : static_cast<int32_t>(field->integer);
}

bool field_bool(const DecodedValue &value, const std::string &name, bool fallback = false) {
    const DecodedValue *field = find_value_field(value, name);
    return field == nullptr ? fallback : field->boolean;
}

std::string field_string(const DecodedValue &value, const std::string &name,
                         const Context &ctx) {
    const DecodedValue *field = find_value_field(value, name);
    return field == nullptr ? std::string{} : value_string(*field, ctx);
}

void emit_expression_set(const DecodedValue &set, const Context &ctx, std::string &out,
                         int indent) {
    const DecodedValue *expressions = find_value_field(set, "Expressions");
    if (expressions == nullptr) {
        return;
    }
    for (const DecodedValue &item : expressions->list) {
        if (item.kind != DecodedValueKind::Struct) {
            continue;
        }
        const DecodedValue *key = find_value_field(item, "first");
        const DecodedValue *entry = find_value_field(item, "second");
        const int32_t type = key != nullptr ? static_cast<int32_t>(key->integer)
                                           : (entry == nullptr ? 0 : field_i32(*entry, "Type"));
        const int32_t anim = entry == nullptr ? 0 : field_i32(*entry, "AnimIndex");
        emit_method(out, indent, "Expressions", "Add",
                    {ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::ExpressionType), type),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Anim), anim)});
    }
}

void emit_wound_morphs(const DecodedValue &set, const Context &ctx, std::string &out,
                       int indent) {
    const DecodedValue *morphs = find_value_field(set, "Morphs");
    if (morphs == nullptr) {
        return;
    }
    for (const DecodedValue &item : morphs->list) {
        if (item.kind != DecodedValueKind::Struct) {
            continue;
        }
        const DecodedValue *key = find_value_field(item, "first");
        const DecodedValue *entry = find_value_field(item, "second");
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
        const DecodedValue *permanent = find_value_field(*entry, "PermanentScarStrength");
        args.push_back(permanent == nullptr ? "0" : format_float_plain(permanent->f32_bits));
        emit_method(out, indent, "WoundMorphs", "AddWoundAndScar", args);
    }
}

void emit_random_appearance_morph(const DecodedValue &ram, const DecodedValue *base, const Context &ctx,
                                  std::string &out, int indent) {
    const auto resolve_mesh = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Mesh), value);
    };
    const auto resolve_texture = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Texture), value);
    };
    const auto resolve_body_part = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::BodyPart), value);
    };

    const auto find_base_mesh = [&](const char *part_name, int32_t mesh_id) -> const DecodedValue * {
        if (base == nullptr) {
            return nullptr;
        }
        const DecodedValue *bpart = find_value_field(*base, part_name);
        const DecodedValue *bmeshes = bpart == nullptr ? nullptr : find_value_field(*bpart, "Meshes");
        if (bmeshes == nullptr) {
            return nullptr;
        }
        for (const DecodedValue &bm : bmeshes->list) {
            if (bm.kind == DecodedValueKind::Struct && field_i32(bm, "MeshId") == mesh_id) {
                return &bm;
            }
        }
        return nullptr;
    };

    std::vector<int32_t> seen_meshes;
    for (const char *part_name : {"BodyParts0", "BodyParts1", "BodyParts2"}) {
        const DecodedValue *part = find_value_field(ram, part_name);
        if (part == nullptr) {
            continue;
        }
        const DecodedValue *meshes = find_value_field(*part, "Meshes");
        if (meshes == nullptr) {
            continue;
        }
        const DecodedValue *base_part = base == nullptr ? nullptr : find_value_field(*base, part_name);
        const DecodedValue *base_meshes =
            base_part == nullptr ? nullptr : find_value_field(*base_part, "Meshes");
        std::vector<bool> base_mesh_used(
            base_meshes != nullptr && base_meshes->kind == DecodedValueKind::List
                ? base_meshes->list.size()
                : 0,
            false);
        const int32_t body_part = part_name[9] - '0';
        for (const DecodedValue &mesh : meshes->list) {
            if (mesh.kind != DecodedValueKind::Struct) {
                continue;
            }
            const int32_t mesh_id = field_i32(mesh, "MeshId");
            const int32_t texture_id = field_i32(mesh, "TextureId");
            bool inherited_part = false;
            if (base_meshes != nullptr && base_meshes->kind == DecodedValueKind::List) {
                for (std::size_t bi = 0; bi < base_meshes->list.size(); ++bi) {
                    const DecodedValue &bm = base_meshes->list[bi];
                    if (!base_mesh_used[bi] && bm.kind == DecodedValueKind::Struct &&
                        field_i32(bm, "MeshId") == mesh_id &&
                        field_i32(bm, "TextureId") == texture_id) {
                        base_mesh_used[bi] = true;
                        inherited_part = true;
                        break;
                    }
                }
            }
            if (!inherited_part) {
                emit_method(out, indent, "RandomAppearanceMorph", "AddBodyPart",
                            {resolve_body_part(body_part), std::to_string(texture_id),
                             resolve_mesh(mesh_id)});
            }
            if (std::find(seen_meshes.begin(), seen_meshes.end(), mesh_id) !=
                seen_meshes.end()) {
                continue;
            }
            seen_meshes.push_back(mesh_id);

            const DecodedValue *morphs = find_value_field(mesh, "TextureMorphs");
            const DecodedValue *entries = morphs == nullptr ? nullptr : find_value_field(*morphs, "Entries");
            if (entries == nullptr || entries->list.empty()) {
                continue;
            }
            const DecodedValue *base_mesh = find_base_mesh(part_name, mesh_id);
            const DecodedValue *base_morphs =
                base_mesh == nullptr ? nullptr : find_value_field(*base_mesh, "TextureMorphs");
            const DecodedValue *base_entries =
                base_morphs == nullptr ? nullptr : find_value_field(*base_morphs, "Entries");
            if (base_morphs != nullptr &&
                sem_equal(*morphs, *base_morphs, "", ctx, ctx)) {
                continue;
            }
            struct TextureEntry {
                int32_t key;
                int32_t original;
                int32_t replacement;
            };
            std::vector<TextureEntry> values;
            std::vector<bool> base_entry_used(
                base_entries != nullptr && base_entries->kind == DecodedValueKind::List
                    ? base_entries->list.size()
                    : 0,
                false);
            for (const DecodedValue &entry : entries->list) {
                if (entry.kind != DecodedValueKind::Struct) {
                    continue;
                }
                bool inherited_entry = false;
                if (base_entries != nullptr && base_entries->kind == DecodedValueKind::List) {
                    for (std::size_t bi = 0; bi < base_entries->list.size(); ++bi) {
                        if (!base_entry_used[bi] &&
                            sem_equal(entry, base_entries->list[bi], "", ctx, ctx)) {
                            base_entry_used[bi] = true;
                            inherited_entry = true;
                            break;
                        }
                    }
                }
                if (inherited_entry) {
                    continue;
                }
                const DecodedValue *key = find_value_field(entry, "first");
                const DecodedValue *second = find_value_field(entry, "second");
                const DecodedValue *morph = second == nullptr ? nullptr : find_value_field(*second, "Morph");
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
                bool varying_keys = false;
                if (group_size > 2) {
                    for (std::size_t k = i + 2; k < j; ++k) {
                        if (values[k].key != values[i + 1].key) {
                            varying_keys = true;
                            break;
                        }
                    }
                }
                if (varying_keys) {
                    emit_method(out, indent, "RandomAppearanceMorph", "AddTextureToMesh",
                                {resolve_mesh(mesh_id), "1", resolve_texture(original),
                                 resolve_texture(values[i].replacement)});
                    for (std::size_t k = i + 1; k < j; ++k) {
                        const int32_t kn = values[k].key > 0 ? values[k].key + 1 : 0;
                        emit_method(out, indent, "RandomAppearanceMorph", "AddTextureToMesh",
                                    {resolve_mesh(mesh_id), std::to_string(kn),
                                     resolve_texture(original),
                                     resolve_texture(values[k].replacement)});
                    }
                    i = j;
                    continue;
                }
                int32_t n = 0;
                if (j < values.size()) {
                    if (values[j].key == 0) {
                        n = 0;
                    } else if (group_size > 1) {
                        n = values[i + 1].key == 0 ? 0 : values[i + 1].key + 1;
                    } else {
                        n = 1;
                    }
                } else if (group_size > 1) {
                    n = values[i + 1].key > 0 ? values[i + 1].key + 1 : 0;
                } else {
                    const int32_t mesh_trailing = field_i32(*morphs, "Trailing");
                    n = mesh_trailing;
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

    if (const DecodedValue *general = find_value_field(ram, "GeneralTextureMorphs")) {
        const DecodedValue *base_general =
            base == nullptr ? nullptr : find_value_field(*base, "GeneralTextureMorphs");
        const DecodedValue *entries =
            (base_general != nullptr && sem_equal(*general, *base_general, "", ctx, ctx))
                ? nullptr
                : find_value_field(*general, "Entries");
        if (entries != nullptr) {
            int32_t original = 0;
            std::vector<std::string> args;
            for (const DecodedValue &entry : entries->list) {
                if (entry.kind != DecodedValueKind::Struct) {
                    continue;
                }
                const DecodedValue *second = find_value_field(entry, "second");
                const DecodedValue *morph = second == nullptr ? nullptr : find_value_field(*second, "Morph");
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

    if (const DecodedValue *skeletal = find_value_field(ram, "SkeletalMorphs")) {
        const DecodedValue *base_skeletal =
            base == nullptr ? nullptr : find_value_field(*base, "SkeletalMorphs");
        std::vector<bool> base_skel_used(
            base_skeletal != nullptr && base_skeletal->kind == DecodedValueKind::List
                ? base_skeletal->list.size()
                : 0,
            false);
        for (const DecodedValue &entry : skeletal->list) {
            if (entry.kind != DecodedValueKind::Struct) {
                continue;
            }
            bool inherited = false;
            if (base_skeletal != nullptr && base_skeletal->kind == DecodedValueKind::List) {
                for (std::size_t bi = 0; bi < base_skeletal->list.size(); ++bi) {
                    if (!base_skel_used[bi] &&
                        sem_equal(entry, base_skeletal->list[bi], "", ctx, ctx)) {
                        base_skel_used[bi] = true;
                        inherited = true;
                        break;
                    }
                }
            }
            if (inherited) {
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





std::string float_value(const DecodedValue *value, double fallback = 0.0) {
    if (value == nullptr) {
        return std::to_string(fallback);
    }
    return format_float(value->f32_bits);
}



float step_ulp_f32(float v, int delta) {
    for (int k = 0; k < delta; ++k) {
        v = std::nextafterf(v, std::numeric_limits<float>::infinity());
    }
    for (int k = 0; k > delta; --k) {
        v = std::nextafterf(v, -std::numeric_limits<float>::infinity());
    }
    return v;
}

std::uint32_t f32_bits(float v) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

void emit_frequency_traits(const DecodedValue &frequency, const Context &ctx, std::string &out,
                           int indent) {
    constexpr double kConstantFps = 15.0;
    const DecodedValue *traits = find_value_field(frequency, "Traits");
    if (traits == nullptr) {
        return;
    }
    const auto put = [](float v) { return format_float(f32_bits(v)); };
    for (std::size_t index = 0; index < traits->list.size() && index < 79; ++index) {
        const DecodedValue &trait = traits->list[index];
        if (trait.kind != DecodedValueKind::Variant && trait.kind != DecodedValueKind::Struct) {
            continue;
        }
        std::string constructor;
        if (trait.tag == 1) {
            const DecodedValue *min_wait = find_value_field(trait, "MinWait");
            const DecodedValue *wait_range = find_value_field(trait, "WaitRange");
            float min_value = 0.0f;
            float range_value = 0.0f;
            if (min_wait) std::memcpy(&min_value, &min_wait->f32_bits, sizeof(min_value));
            if (wait_range) std::memcpy(&range_value, &wait_range->f32_bits, sizeof(range_value));
            float min_secs = static_cast<float>(static_cast<double>(min_value) / kConstantFps);
            std::size_t best_min_len = std::numeric_limits<std::size_t>::max();
            for (int d = -8; d <= 8; ++d) {
                const float cand = step_ulp_f32(min_secs, d);
                const float frames = static_cast<float>(kConstantFps * static_cast<double>(cand));
                if (f32_bits(frames) == f32_bits(min_value)) {
                    const std::size_t len = put(cand).size();
                    if (len < best_min_len) {
                        best_min_len = len;
                        min_secs = cand;
                    }
                }
            }
            const float min_frames =
                static_cast<float>(kConstantFps * static_cast<double>(min_secs));
            float max_secs = static_cast<float>(
                (static_cast<double>(min_value) + static_cast<double>(range_value)) / kConstantFps);
            std::size_t best_max_len = std::numeric_limits<std::size_t>::max();
            for (int d = -8; d <= 8; ++d) {
                const float cand = step_ulp_f32(max_secs, d);
                const float max_frames =
                    static_cast<float>(kConstantFps * static_cast<double>(cand));
                const float wr = max_frames - min_frames;
                if (f32_bits(wr) == f32_bits(range_value)) {
                    const std::size_t len = put(cand).size();
                    if (len < best_max_len) {
                        best_max_len = len;
                        max_secs = cand;
                    }
                }
            }
            constructor = "Linear(" + put(min_secs) + ", " + put(max_secs) + ")";
        } else if (trait.tag == 2) {
            const DecodedValue *low_freq = find_value_field(trait, "LowFreqPerMin");
            const DecodedValue *hl_interval = find_value_field(trait, "HighLowFreqInterval");
            const DecodedValue *ramp_up = find_value_field(trait, "RampUpTimeFrames");
            const DecodedValue *cutoff = find_value_field(trait, "CutoffIntervalFrames");
            float lf = 0.0f, hl = 0.0f;
            if (low_freq) std::memcpy(&lf, &low_freq->f32_bits, sizeof(lf));
            if (hl_interval) std::memcpy(&hl, &hl_interval->f32_bits, sizeof(hl));
            const std::uint32_t ru_frames = ramp_up ? static_cast<std::uint32_t>(ramp_up->integer) : 0u;
            const std::uint32_t co_frames = cutoff ? static_cast<std::uint32_t>(cutoff->integer) : 0u;
            const float ru_secs = static_cast<float>(static_cast<double>(ru_frames) / kConstantFps);
            const float sh_secs = static_cast<float>(
                static_cast<double>(co_frames >= ru_frames ? co_frames - ru_frames : 0u) /
                kConstantFps);
            constructor = "PeriodicFrequency(" + put(ru_secs) + ", " + put(sh_secs) + ", " +
                          put(lf) + ", " + put(lf + hl) + ")";
        } else if (trait.tag == 3) {
            const DecodedValue *max_count = find_value_field(trait, "MaxCount");
            const DecodedValue *min_gap = find_value_field(trait, "MinGapFrames");
            const DecodedValue *recharge = find_value_field(trait, "CountRechargePerFrame");
            float max_count_value = 0.0f;
            if (max_count) std::memcpy(&max_count_value, &max_count->f32_bits, sizeof(max_count_value));
            const std::uint32_t min_gap_frames =
                min_gap == nullptr ? 0u : static_cast<std::uint32_t>(min_gap->integer);
            float recharge_value = 0.0f;
            if (recharge) std::memcpy(&recharge_value, &recharge->f32_bits, sizeof(recharge_value));
            float min_gap_secs = static_cast<float>(static_cast<double>(min_gap_frames) / kConstantFps);
            std::size_t best_mg_len = std::numeric_limits<std::size_t>::max();
            for (int d = -8; d <= 8; ++d) {
                const float cand = step_ulp_f32(min_gap_secs, d);
                const auto frames = static_cast<std::uint32_t>(
                    static_cast<std::int64_t>(kConstantFps * static_cast<double>(cand)));
                if (frames == min_gap_frames) {
                    const std::size_t len = put(cand).size();
                    if (len < best_mg_len) {
                        best_mg_len = len;
                        min_gap_secs = cand;
                    }
                }
            }
            float recharge_secs = 0.0f;
            if (recharge_value > 0.0f && f32_bits(recharge_value) != f32_bits(max_count_value)) {
                recharge_secs = static_cast<float>(
                    static_cast<double>(max_count_value) /
                    (kConstantFps * static_cast<double>(recharge_value)));
                std::size_t best_rc_len = std::numeric_limits<std::size_t>::max();
                for (int d = -12; d <= 12; ++d) {
                    const float cand = step_ulp_f32(recharge_secs, d);
                    const float got = static_cast<float>(
                        static_cast<double>(max_count_value) /
                        (static_cast<double>(cand) * kConstantFps));
                    if (f32_bits(got) == f32_bits(recharge_value)) {
                        const std::size_t len = put(cand).size();
                        if (len < best_rc_len) {
                            best_rc_len = len;
                            recharge_secs = cand;
                        }
                    }
                }
            }
            const std::uint32_t mc_int = static_cast<std::uint32_t>(max_count_value);
            const std::string mc_str =
                static_cast<float>(mc_int) == max_count_value ? std::to_string(mc_int)
                                                              : put(max_count_value);
            constructor = "ControlledCount(" + mc_str + ", " +
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


std::string value_expr(const DecodedValue *value, const std::string &field_name,
                       const std::string &scope, const Context &ctx,
                       const char *fallback = "0") {
    if (value == nullptr) {
        return fallback;
    }
    return format_expr(*value, field_name, scope, ctx);
}

std::string particle_expr(const DecodedValue *value, const std::string &field_name,
                          const std::string &scope, const Context &ctx) {
    if (value != nullptr && value->kind == DecodedValueKind::Int && value->integer > 0) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                          static_cast<std::int32_t>(value->integer));
    }
    return value_expr(value, field_name, scope, ctx);
}


bool engine_graphic_shape(const DecodedValue &value) {
    return has_field(value, "Type") && has_field(value, "BankIndex") &&
           has_field(value, "AnimStep") && has_field(value, "RenderSizeX") &&
           has_field(value, "AdditiveAlpha");
}

bool value_changed(const DecodedValue &value, const DecodedValue *base, const std::string &name,
                   const Context &ctx) {
    return base == nullptr || !sem_equal(value, *base, name, ctx, ctx);
}

void emit_engine_graphic(const std::string &path, const DecodedValue &value, const DecodedValue *base,
                         const std::string &scope, const Context &ctx, std::string &out,
                         int indent) {
    const DecodedValue *type_value = find_value_field(value, "Type");
    const int32_t type = type_value == nullptr ? 0 : static_cast<int32_t>(type_value->integer);
    for (const char *name : {"Type", "BankIndex", "AnimStep", "RenderSizeX", "AdditiveAlpha"}) {
        const DecodedValue *member = find_value_field(value, name);
        if (member == nullptr) {
            continue;
        }
        const DecodedValue *base_member =
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
                        SymDomain::named(SymDomain::Kind::Texture), bank, ctx.is_frontend);
                } else if (type == 3) {
                    expr = ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Particle), bank, ctx.is_frontend);
                } else if (type == 4 || type == 5) {
                    expr = ctx.headers.resolve_domain(
                        SymDomain::named(SymDomain::Kind::Mesh), bank, ctx.is_frontend);
                } else {
                    expr = ctx.headers.resolve_field(scope, "BankIndex", bank, ctx.is_frontend);
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

bool has_field(const DecodedValue &value, const std::string &name) {
    return find_value_field(value, name) != nullptr;
}

std::optional<std::string> compound_constructor(const DecodedValue &value,
                                                const std::string &scope,
                                                const Context &ctx) {
    const auto field = [&](const char *name) { return find_value_field(value, name); };
    if (value.fields.size() == 1 && value.fields[0].first == "IntValue" &&
        value.fields[0].second.kind == DecodedValueKind::Int) {
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
        const DecodedValue *period = field("PeriodSecs");
        const DecodedValue *start = field("StartPoint");
        const DecodedValue *end = field("EndPoint");
        const std::size_t pairs = start == nullptr ? 0 : start->list.size();
        const auto non_zero_particle = [&](const char *name) {
            const DecodedValue *v = field(name);
            return v != nullptr && v->integer != 0;
        };
        const bool emit_particles = pairs != 0 || non_zero_particle("ParticleEffect") ||
                                    non_zero_particle("ParticleEffectToBlendTo") ||
                                    non_zero_particle("AttackParticleEffect") ||
                                    non_zero_particle("AttackParticleEffectToBlendTo");
        std::string expr = "CObjectAugmentationParticleSet(" +
                           value_expr(period, "PeriodSecs", scope, ctx) + ", " +
                           std::to_string(pairs);
        if (emit_particles) {
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
        const DecodedValue *map = field("ComboMultiplier");
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
        value.fields[0].second.kind == DecodedValueKind::Map) {
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
        value.fields[0].second.kind == DecodedValueKind::List) {
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
        const DecodedValue *combo = field("AttackCombo");
        if (combo != nullptr) {
            for (const DecodedValue &item : combo->list) {
                if (item.kind != DecodedValueKind::Struct) {
                    continue;
                }
                const DecodedValue *type = find_value_field(item, "first");
                const DecodedValue *attribute = find_value_field(item, "second");
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

void emit_reaction_matches(const DecodedValue &matches, const Context &ctx, std::string &out,
                           int indent) {
    const DecodedValue *elements = find_value_field(matches, "Elements");
    if (elements == nullptr) {
        return;
    }
    for (const DecodedValue &entry : elements->list) {
        if (entry.kind != DecodedValueKind::Variant && entry.kind != DecodedValueKind::Struct) {
            continue;
        }
        const std::string reaction =
            ctx.headers.resolve_domain(
                SymDomain::named(SymDomain::Kind::OpinionReactionType),
                field_i32(entry, "ReactionType"));
        std::string constructor;
        switch (entry.tag) {
        case 0: {
            const DecodedValue *lower = find_value_field(entry, "LowerBoundOnAxis");
            const DecodedValue *interval = find_value_field(entry, "InvIntervalOnAxis");
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
        case 2:
        case 3: {
            const auto read_f32 = [&](const char *field) {
                const DecodedValue *v = find_value_field(entry, field);
                float f = 0.0f;
                if (v) std::memcpy(&f, &v->f32_bits, sizeof(f));
                return f;
            };
            const float xc_f = read_f32("XCentre");
            const float yc_f = read_f32("YCentre");
            const float nix_f = read_f32("NegInvXRadius");
            const float pix_f = read_f32("PosInvXRadius");
            const float niy_f = read_f32("NegInvYRadius");
            const float piy_f = read_f32("PosInvYRadius");
            const auto put = [&](float v) {
                return format_float(f32_bits(v));
            };
            if (entry.tag == 2) {
                const auto refine_one = [&](float bound0, float target_inv, bool is_upper,
                                            float centre) {
                    if (target_inv == 0.0f) {
                        return bound0;
                    }
                    const std::uint32_t want = f32_bits(target_inv);
                    float best = bound0;
                    std::size_t best_len = std::numeric_limits<std::size_t>::max();
                    for (int d = -12; d <= 12; ++d) {
                        const float cand = step_ulp_f32(bound0, d);
                        const double denom = is_upper
                                                 ? static_cast<double>(cand) - static_cast<double>(centre)
                                                 : static_cast<double>(centre) - static_cast<double>(cand);
                        const float got = static_cast<float>(1.0 / denom);
                        if (f32_bits(got) == want) {
                            const std::size_t len = format_float(f32_bits(cand)).size();
                            if (len < best_len) {
                                best_len = len;
                                best = cand;
                            }
                        }
                    }
                    return best;
                };
                const float xmin = refine_one(xc_f - (nix_f == 0.0f ? 0.0f : 1.0f / nix_f), nix_f, false, xc_f);
                const float xmax = refine_one(xc_f + (pix_f == 0.0f ? 0.0f : 1.0f / pix_f), pix_f, true, xc_f);
                const float ymin = refine_one(yc_f - (niy_f == 0.0f ? 0.0f : 1.0f / niy_f), niy_f, false, yc_f);
                const float ymax = refine_one(yc_f + (piy_f == 0.0f ? 0.0f : 1.0f / piy_f), piy_f, true, yc_f);
                constructor = "Pyramid(" +
                    std::to_string(field_i32(entry, "XAxisOpinionType")) + ", " +
                    put(xmin) + ", " + put(xc_f) + ", " + put(xmax) + ", " +
                    std::to_string(field_i32(entry, "YAxisOpinionType")) + ", " +
                    put(ymin) + ", " + put(yc_f) + ", " + put(ymax) + ")";
            } else {
                const auto refine_bounds = [&](float c_f, float ni_f, float pi_f,
                                               float &out_min, float &out_max) {
                    float lo0 = c_f - (ni_f == 0.0f ? 0.0f : 1.0f / ni_f);
                    float hi0 = c_f + (pi_f == 0.0f ? 0.0f : 1.0f / pi_f);
                    out_min = lo0;
                    out_max = hi0;
                    if (ni_f == 0.0f || pi_f == 0.0f) {
                        return;
                    }
                    const std::uint32_t want_c = f32_bits(c_f);
                    const std::uint32_t want_ni = f32_bits(ni_f);
                    const std::uint32_t want_pi = f32_bits(pi_f);
                    std::size_t best_len = std::numeric_limits<std::size_t>::max();
                    for (int dl = -12; dl <= 12; ++dl) {
                        const float lo = step_ulp_f32(lo0, dl);
                        for (int dh = -12; dh <= 12; ++dh) {
                            const float hi = step_ulp_f32(hi0, dh);
                            const double centre =
                                static_cast<double>(lo) +
                                (static_cast<double>(hi) - static_cast<double>(lo)) * 0.5;
                            const float cf = static_cast<float>(centre);
                            const float nif = static_cast<float>(
                                1.0 / (static_cast<double>(cf) - static_cast<double>(lo)));
                            const float pif = static_cast<float>(
                                1.0 / (static_cast<double>(hi) - static_cast<double>(cf)));
                            if (f32_bits(cf) == want_c && f32_bits(nif) == want_ni &&
                                f32_bits(pif) == want_pi) {
                                const std::size_t len = format_float(f32_bits(lo)).size() +
                                                        format_float(f32_bits(hi)).size();
                                if (len < best_len) {
                                    best_len = len;
                                    out_min = lo;
                                    out_max = hi;
                                }
                            }
                        }
                    }
                };
                float xmin = 0.0f, xmax = 0.0f, ymin = 0.0f, ymax = 0.0f;
                refine_bounds(xc_f, nix_f, pix_f, xmin, xmax);
                refine_bounds(yc_f, niy_f, piy_f, ymin, ymax);
                constructor = "MRBlob(" + put(ymin) + ", " + put(ymax) + ", " +
                              put(xmin) + ", " + put(xmax) + ", " +
                              float_value(find_value_field(entry, "ScarinessShift")) + ", " +
                              float_value(find_value_field(entry, "AgreeablenessShift")) + ", " +
                              float_value(find_value_field(entry, "AttractivenessShift")) + ")";
            }
            break;
        }
        default:
            continue;
        }
        emit_method(out, indent, "Matches", "Add", {reaction, constructor});
    }
}

void emit_flammable(const DecodedValue &flammable, const Context &ctx, std::string &out,
                    int indent) {
    const DecodedValue *map = find_value_field(flammable, "ContainmentMap");
    if (map == nullptr) {
        const DecodedValue *set = find_value_field(flammable, "EffectCreationSet");
        map = set == nullptr ? nullptr : find_value_field(*set, "ContainmentMap");
    }
    if (map == nullptr) {
        return;
    }
    for (const auto &[key, value] : map->map) {
        std::string key_text;
        if (key.kind == DecodedValueKind::DefString) {
            key_text = resolve_string(key.integer, ctx);
        } else if (key.kind == DecodedValueKind::String) {
            key_text = key.text;
        }
        emit_method(out, indent, "EffectCreationSet", "Add",
                    {quote(key_text),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                                static_cast<int32_t>(value.integer))});
    }
}

void emit_opinion_personality(const DecodedValue &personality, const DecodedValue *base,
                              std::string &out, int indent) {
    const DecodedValue *traits = &personality;
    if (find_value_field(personality, "Data") == nullptr) {
        traits = find_value_field(personality, "PersonalityTraits");
        if (traits == nullptr) {
            return;
        }
    }
    const DecodedValue *data = find_value_field(*traits, "Data");
    if (data == nullptr || data->list.size() < 180) {
        return;
    }
    std::vector<int32_t> base_data(180, 0);
    if (base != nullptr) {
        const DecodedValue *base_traits =
            find_value_field(*base, "Data") != nullptr ? base : find_value_field(*base, "PersonalityTraits");
        if (base_traits != nullptr) {
            if (const DecodedValue *base_values = find_value_field(*base_traits, "Data");
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

void emit_opinion_deed_effects(const DecodedValue &definition, const Context &ctx,
                               std::string &out, int indent) {
    const DecodedValue *effects =
        definition.kind == DecodedValueKind::List ? &definition : find_value_field(definition, "Effects");
    if (effects == nullptr) {
        return;
    }
    for (const DecodedValue &effect : effects->list) {
        if (effect.kind != DecodedValueKind::Struct) {
            continue;
        }
        const int32_t opinion = field_i32(effect, "Opinion");
        const float run_in = [&] {
            const DecodedValue *v = find_value_field(effect, "OffsetPerFrameRunIn");
            return v == nullptr ? 0.0f : [&] {
                float f = 0.0f;
                std::memcpy(&f, &v->f32_bits, sizeof(f));
                return f;
            }();
        }();
        const int32_t frames_in = field_i32(effect, "FramesToRunIn");
        const float run_out = [&] {
            const DecodedValue *v = find_value_field(effect, "OffsetPerFrameRunOut");
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

void emit_degradable(const DecodedValue &definition, const Context &ctx, std::string &out,
                     int indent) {
    const DecodedValue *degradations = definition.kind == DecodedValueKind::List
                                    ? &definition
                                    : find_value_field(definition, "Degradations");
    if (degradations == nullptr) {
        return;
    }
    for (const DecodedValue &degradation : degradations->list) {
        if (degradation.kind != DecodedValueKind::Struct) {
            continue;
        }
        const DecodedValue *health = find_value_field(degradation, "HealthPercentage");
        emit_method(out, indent, "Degradations", "Add",
                    {health == nullptr ? "0" : format_float_plain(health->f32_bits),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Mesh),
                                                field_i32(degradation, "BankIndex")),
                     ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Particle),
                                                field_i32(degradation, "SmashParticleEmitter")),
                     field_bool(degradation, "BlocksNavigation") ? "TRUE" : "FALSE"});
    }
}

void emit_replaceable_meshes(const DecodedValue &definition, const DecodedValue *base,
                             const Context &ctx, std::string &out, int indent) {
    const DecodedValue *meshes = has_field(definition, "Vector")
                              ? &definition
                              : find_value_field(definition, "Meshes");
    const DecodedValue *entries = meshes == nullptr ? nullptr : find_value_field(*meshes, "Vector");
    if (entries == nullptr) {
        return;
    }
    const DecodedValue *base_entries = nullptr;
    if (base != nullptr) {
        const DecodedValue *base_meshes =
            has_field(*base, "Vector") ? base : find_value_field(*base, "Meshes");
        base_entries =
            base_meshes == nullptr ? nullptr : find_value_field(*base_meshes, "Vector");
    }
    std::size_t start_idx = 0;
    if (base_entries != nullptr && !base_entries->list.empty()) {
        bool prefix_match = entries->list.size() >= base_entries->list.size();
        if (prefix_match) {
            for (std::size_t i = 0; i < base_entries->list.size(); ++i) {
                if (!sem_equal(entries->list[i], base_entries->list[i], "Vector", ctx, ctx)) {
                    prefix_match = false;
                    break;
                }
            }
        }
        if (prefix_match) {
            start_idx = base_entries->list.size();
        } else {
            emit_method(out, indent, "Meshes", "clear", {});
        }
    }
    for (std::size_t i = start_idx; i < entries->list.size(); ++i) {
        const DecodedValue &entry = entries->list[i];
        if (entry.kind != DecodedValueKind::Struct) {
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

void emit_entity_sound(const DecodedValue &sound_map, const DecodedValue *base_sound_map,
                       const Context &ctx, std::string &out, int indent) {
    const auto resolve_sound = [&](int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(SymDomain::Kind::Sound), value);
    };
    const auto key_name = [&](const DecodedValue &key) {
        if (const auto found = ctx.crc_to_str.find(static_cast<std::uint32_t>(key.integer));
            found != ctx.crc_to_str.end()) {
            return found->second;
        }
        return "snd_" + std::to_string(key.integer);
    };
    const DecodedValue *base_sounds =
        base_sound_map == nullptr ? nullptr : find_value_field(*base_sound_map, "SoundMap");
    if (const DecodedValue *sounds = find_value_field(sound_map, "SoundMap")) {
        for (const DecodedValue &item : sounds->list) {
            if (item.kind != DecodedValueKind::Struct) {
                continue;
            }
            const DecodedValue *key = find_value_field(item, "first");
            const DecodedValue *entry = find_value_field(item, "second");
            if (key == nullptr || entry == nullptr) {
                continue;
            }
            const int32_t first = field_i32(*entry, "FirstSound");
            const int32_t last = field_i32(*entry, "LastSound");
            bool inherited = false;
            if (base_sounds != nullptr) {
                for (const DecodedValue &bitem : base_sounds->list) {
                    const DecodedValue *bkey = find_value_field(bitem, "first");
                    const DecodedValue *bentry = find_value_field(bitem, "second");
                    if (bkey != nullptr && bentry != nullptr &&
                        bkey->integer == key->integer &&
                        field_i32(*bentry, "FirstSound") == first &&
                        field_i32(*bentry, "LastSound") == last) {
                        inherited = true;
                        break;
                    }
                }
            }
            if (inherited) {
                continue;
            }
            const std::string key_text = key_name(*key);
            if (first == last) {
                emit_method(out, indent, "SoundMap", "AddSingle",
                            {quote(key_text), resolve_sound(first)});
            } else {
                emit_method(out, indent, "SoundMap", "AddPair",
                            {quote(key_text), resolve_sound(first), resolve_sound(last)});
            }
        }
    }
    const DecodedValue *base_criteria =
        base_sound_map == nullptr ? nullptr : find_value_field(*base_sound_map, "CriteriaMap");
    if (const DecodedValue *criteria = find_value_field(sound_map, "CriteriaMap")) {
        for (const DecodedValue &item : criteria->list) {
            if (item.kind != DecodedValueKind::Struct) {
                continue;
            }
            const DecodedValue *key = find_value_field(item, "first");
            const DecodedValue *entry = find_value_field(item, "second");
            if (key == nullptr || entry == nullptr) {
                continue;
            }
            const std::string val_str = value_string(*entry, ctx);
            bool inherited = false;
            if (base_criteria != nullptr) {
                for (const DecodedValue &bitem : base_criteria->list) {
                    const DecodedValue *bkey = find_value_field(bitem, "first");
                    const DecodedValue *bentry = find_value_field(bitem, "second");
                    if (bkey != nullptr && bentry != nullptr &&
                        bkey->integer == key->integer &&
                        value_string(*bentry, ctx) == val_str) {
                        inherited = true;
                        break;
                    }
                }
            }
            if (inherited) {
                continue;
            }
            emit_method(out, indent, "SoundMap", "AddCriteria",
                        {quote(key_name(*key)), quote(val_str)});
        }
    }
}

void emit_special_effects(const DecodedValue &effects, const Context &ctx, std::string &out,
                          int indent) {
    const DecodedValue *map = find_value_field(effects, "ContainmentMap");
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

void emit_components(const DecodedValue &components, const DecodedValue *base_components,
                     const Context &ctx, std::string &out, int indent) {
    const DecodedValue *entries = find_value_field(components, "Entries");
    if (entries == nullptr) {
        return;
    }
    struct CompItem {
        std::string name;
        bool override_flag = false;
    };
    auto collect = [&](const DecodedValue &comp) {
        std::vector<CompItem> list;
        const DecodedValue *e = find_value_field(comp, "Entries");
        if (e == nullptr) {
            return list;
        }
        for (const DecodedValue &item : e->list) {
            if (item.kind != DecodedValueKind::Struct) {
                continue;
            }
            list.push_back({field_string(item, "Name", ctx), field_bool(item, "Override")});
        }
        return list;
    };

    const std::vector<CompItem> current = collect(components);
    if (base_components == nullptr) {
        for (const CompItem &item : current) {
            std::vector<std::string> args = {quote(item.name)};
            if (item.override_flag) {
                args.push_back("TRUE");
            }
            emit_method(out, indent, "Components", "Add", args);
        }
        return;
    }

    const std::vector<CompItem> inherited = collect(*base_components);
    std::map<std::string, std::size_t> base_counts;
    std::map<std::string, std::size_t> cur_counts;
    for (const CompItem &x : inherited) {
        ++base_counts[x.name];
    }
    for (const CompItem &x : current) {
        ++cur_counts[x.name];
    }

    // CThingComponentSet::Remove erases all entries with a given name, so any name whose
    // multiplicity shrank or whose inherited items changed must be removed and re-added.
    std::set<std::string> removed_names;
    for (const CompItem &base_item : inherited) {
        if (removed_names.count(base_item.name)) {
            continue;
        }
        const std::size_t bc = base_counts[base_item.name];
        const std::size_t cc = cur_counts[base_item.name];
        if (cc < bc) {
            emit_method(out, indent, "Components", "Remove", {quote(base_item.name)});
            removed_names.insert(base_item.name);
        }
    }

    std::vector<bool> inherited_used(inherited.size(), false);
    for (const CompItem &item : current) {
        if (!removed_names.count(item.name)) {
            bool matched = false;
            for (std::size_t bi = 0; bi < inherited.size(); ++bi) {
                if (!inherited_used[bi] &&
                    inherited[bi].name == item.name &&
                    inherited[bi].override_flag == item.override_flag) {
                    inherited_used[bi] = true;
                    matched = true;
                    break;
                }
            }
            if (matched) {
                continue;
            }
        }
        std::vector<std::string> args = {quote(item.name)};
        if (item.override_flag) {
            args.push_back("TRUE");
        }
        emit_method(out, indent, "Components", "Add", args);
    }
}

void emit_weapon_trails(const DecodedValue &trails, const Context &ctx, std::string &out,
                        int indent) {
    if (trails.kind != DecodedValueKind::Map) {
        return;
    }
    const auto resolve = [&](SymDomain::Kind kind, int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(kind), value);
    };
    for (const auto &[key, mapped] : trails.map) {
        if (mapped.kind != DecodedValueKind::Struct) {
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

void emit_appearance_graphics(const DecodedValue &graphics_field, const Context &ctx,
                              std::string &out, int indent) {
    const DecodedValue *graphics = find_value_field(graphics_field, "Graphics");
    if (graphics == nullptr) {
        return;
    }
    for (const DecodedValue &entry : graphics->list) {
        if (entry.kind != DecodedValueKind::Struct) {
            continue;
        }
        const int32_t type = field_i32(entry, "Type");
        const int32_t bank = field_i32(entry, "BankIndex");
        const int32_t morph = field_i32(entry, "MorphType");
        const DecodedValue *size = find_value_field(entry, "RenderSizeX");
        const DecodedValue *start = find_value_field(entry, "StartMorphStrength");
        const DecodedValue *end = find_value_field(entry, "EndMorphStrength");
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

void emit_hero_morph(const DecodedValue &hero, const DecodedValue *base, const Context &ctx,
                     std::string &out, int indent) {
    const auto resolve = [&](SymDomain::Kind kind, int32_t value) {
        return ctx.headers.resolve_domain(SymDomain::named(kind), value);
    };
    const auto list_contains = [&](const DecodedValue *base_list, const DecodedValue &item) {
        if (base_list == nullptr || base_list->kind != DecodedValueKind::List) {
            return false;
        }
        for (const DecodedValue &b : base_list->list) {
            if (sem_equal(item, b, "", ctx, ctx)) {
                return true;
            }
        }
        return false;
    };
    const DecodedValue *base_skeletal =
        base == nullptr ? nullptr : find_value_field(*base, "SkeletalMorphs");
    const DecodedValue *base_skeletal_entries =
        base_skeletal == nullptr ? nullptr : find_value_field(*base_skeletal, "Morphs");
    if (const DecodedValue *skeletal = find_value_field(hero, "SkeletalMorphs")) {
        if (const DecodedValue *entries = find_value_field(*skeletal, "Morphs")) {
            for (const DecodedValue &item : entries->list) {
                if (item.kind != DecodedValueKind::Struct ||
                    list_contains(base_skeletal_entries, item)) {
                    continue;
                }
                const DecodedValue *key = find_value_field(item, "first");
                const DecodedValue *entry = find_value_field(item, "second");
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
    const DecodedValue *base_morphs =
        base == nullptr ? nullptr : find_value_field(*base, "TextureMorphs");
    const DecodedValue *base_morphs_entries =
        base_morphs == nullptr ? nullptr : find_value_field(*base_morphs, "Morphs");
    if (const DecodedValue *morphs = find_value_field(hero, "TextureMorphs")) {
        if (const DecodedValue *entries = find_value_field(*morphs, "Morphs")) {
            for (const DecodedValue &item : entries->list) {
                if (item.kind != DecodedValueKind::Struct ||
                    list_contains(base_morphs_entries, item)) {
                    continue;
                }
                const DecodedValue *key = find_value_field(item, "first");
                const DecodedValue *entry = find_value_field(item, "second");
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
        const DecodedValue *morphs = find_value_field(hero, field_name);
        if (morphs == nullptr) {
            return;
        }
        const DecodedValue *entries = find_value_field(*morphs, "Morphs");
        if (entries == nullptr) {
            return;
        }
        const DecodedValue *base_morphs = base == nullptr ? nullptr : find_value_field(*base, field_name);
        const DecodedValue *base_entries =
            base_morphs == nullptr ? nullptr : find_value_field(*base_morphs, "Morphs");
        for (const DecodedValue &item : entries->list) {
            if (item.kind != DecodedValueKind::Struct || list_contains(base_entries, item)) {
                continue;
            }
            const DecodedValue *key = find_value_field(item, "first");
            const DecodedValue *entry = find_value_field(item, "second");
            if (entry == nullptr) {
                continue;
            }
            const int32_t morph = key == nullptr ? 0 : static_cast<int32_t>(key->integer);
            const int32_t location = field_i32(*entry, "Location");
            const std::string location_name = field_string(*entry, "LocationName", ctx);
            const int32_t location_index = field_i32(*entry, "LocationIndex");
            const int32_t particle_index = field_i32(*entry, "ParticleIndex");
            const int32_t second_particle = field_i32(*entry, "SecondParticleIndex");
            const DecodedValue *start = find_value_field(*entry, "StartAtMorphStrength");
            const DecodedValue *end = find_value_field(*entry, "EndAtMorphStrength");
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
    const DecodedValue *components_value = nullptr;
    const DecodedValue *sound_map_value = nullptr;
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
        const DecodedValue *base_value = nullptr;
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
            if (value.kind == DecodedValueKind::Struct) {
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
                emit_random_appearance_morph(value, base_value, ctx, out, indent);
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
            emit_replaceable_meshes(value, base_value, ctx, out, indent);
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
        const DecodedValue *base_sound_map_value = nullptr;
        if (base != nullptr) {
            for (const auto &[base_name, candidate] : base->fields) {
                if (base_name == "SoundMap") {
                    base_sound_map_value = &candidate;
                    break;
                }
            }
        }
        emit_entity_sound(*sound_map_value, base_sound_map_value, ctx, out, indent);
    }
    if (components_value != nullptr) {
        const DecodedValue *base_components_value = nullptr;
        if (base != nullptr) {
            for (const auto &[base_name, candidate] : base->fields) {
                if (base_name == "Components") {
                    base_components_value = &candidate;
                    break;
                }
            }
        }
        emit_components(*components_value, base_components_value, ctx, out, indent);
    }
    if (scope == "CHeroMorphDef") {
        DecodedValue combined;
        combined.kind = DecodedValueKind::Struct;
        combined.fields = entry.fields;
        if (base != nullptr) {
            DecodedValue base_combined;
            base_combined.kind = DecodedValueKind::Struct;
            base_combined.fields = base->fields;
            emit_hero_morph(combined, &base_combined, ctx, out, indent);
        } else {
            emit_hero_morph(combined, nullptr, ctx, out, indent);
        }
    }
}

std::int32_t resolve_effective_ui_type(const std::string &def_name,
                                         const ParsedEntry &entry,
                                         const Context &ctx,
                                         const std::map<std::string, std::string> &parents) {
    std::set<std::string> visited;
    std::string current_name = def_name;
    const ParsedEntry *current = &entry;
    for (int depth = 0; depth < 32 && current != nullptr; ++depth) {
        for (const auto &[fname, fval] : current->fields) {
            if (fname == "Type" &&
                (fval.kind == DecodedValueKind::Int ||
                 fval.kind == DecodedValueKind::Enum ||
                 fval.kind == DecodedValueKind::Flags)) {
                return static_cast<std::int32_t>(fval.integer);
            }
        }
        if (!visited.insert(current_name).second) {
            break;
        }
        const auto pit = parents.find(current_name);
        if (pit == parents.end()) {
            break;
        }
        current_name = pit->second;
        const auto eit = ctx.name_to_entry_idx.find(current_name);
        if (eit == ctx.name_to_entry_idx.end() || eit->second >= ctx.entries.size()) {
            break;
        }
        current = &ctx.entries[eit->second];
    }
    return 4; // UI_TYPE_COMPOSITE default in CUIDef::Transfer.
}

struct ScopedActiveUiType {
    const Context &ctx;
    std::int32_t previous;
    ScopedActiveUiType(const Context &c, std::int32_t next)
        : ctx(c), previous(c.active_ui_type) {
        ctx.active_ui_type = next;
    }
    ~ScopedActiveUiType() { ctx.active_ui_type = previous; }
};

void emit_definition(const BinaryEntry &binary_entry, const ParsedEntry &entry,
                     const Context &ctx, const std::map<std::string, std::string> &parents,
                     std::string &out) {
    const bool is_ui = entry.def_type == "UI" || entry.def_type == "CUIDef";
    ScopedActiveUiType ui_scope(
        ctx, is_ui ? resolve_effective_ui_type(binary_entry.file_name, entry, ctx, parents) : -1);
    out += entry.is_template ? "#definition_template " : "#definition ";
    out += entry.def_type;
    out.push_back(' ');
    out += binary_entry.file_name;
    const ParsedEntry *base = nullptr;
    const BinaryEntry *parent_binary = nullptr;
    if (const auto parent = parents.find(binary_entry.file_name);
        parent != parents.end()) {
        if (const auto pit = ctx.name_to_entry_idx.find(parent->second);
            pit != ctx.name_to_entry_idx.end() && pit->second < ctx.entries.size()) {
            out += " specialises ";
            out += parent->second;
            base = &ctx.entries[pit->second];
            if (ctx.binary != nullptr && pit->second < ctx.binary->entries.size()) {
                parent_binary = &ctx.binary->entries[pit->second];
            }
        }
    }
    out.push_back('\n');
    if (base == nullptr) {
        const auto base_it = ctx.nulldefs.find(entry.def_type);
        base = base_it == ctx.nulldefs.end() ? nullptr : &base_it->second;
    }
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
            const ParsedEntry *sub_base = nullptr;
            bool inherited_sub = false;
            if (parent_binary != nullptr && parent_binary->sub_defs) {
                for (const SubDefRecord &parent_rec : *parent_binary->sub_defs) {
                    if (parent_rec.name_crc == record.name_crc &&
                        parent_rec.def_index < ctx.entries.size()) {
                        sub_base = &ctx.entries[parent_rec.def_index];
                        inherited_sub = true;
                        break;
                    }
                }
            }
            if (sub_base == nullptr) {
                const auto sub_base_it = ctx.nulldefs.find(tag);
                sub_base = sub_base_it == ctx.nulldefs.end() ? nullptr : &sub_base_it->second;
            }
            std::string sub_out;
            unlower_body(sub, sub_base, tag, ctx, sub_out, 2);
            if (inherited_sub && sub_out.empty()) {
                continue;
            }
            out.append(static_cast<std::size_t>(1) * 4, ' ');
            out += "<" + tag + ">\n";
            out += sub_out;
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
    if (binary == "frontend") {
        ctx.is_frontend = true;
    }
    const auto parents = build_merged_parent_map(defs, defs_directory);
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

bool decompile_binary_to_folder(const std::string &directory,
                                const std::string &binary,
                                const std::string &headers_directory,
                                const std::string &output_directory,
                                FolderDecompileStats &stats,
                                std::string &error) {
    namespace fs = std::filesystem;
    Names names;
    if (!Names::load(directory + "/names.bin", names, error)) {
        return false;
    }
    DefBinary defs;
    if (!DefBinary::load(directory + "/" + binary + ".bin", names, defs, error)) {
        return false;
    }

    DecompileStats local_stats;
    Context ctx;
    build_context(defs, names, HeaderResolver::build(headers_directory), headers_directory,
                  ctx, local_stats);
    if (binary == "frontend") {
        ctx.is_frontend = true;
    }
    stats.errors.insert(stats.errors.end(), local_stats.errors.begin(),
                        local_stats.errors.end());
    const auto parents = build_merged_parent_map(defs, headers_directory);

    stats.entries += defs.entries.size();

    // Group valid entries by target relative file path.
    std::map<std::string, std::vector<std::size_t>> grouped;
    std::map<std::string, std::size_t> name_to_index;
    for (std::size_t i = 0; i < defs.entries.size(); ++i) {
        const BinaryEntry &entry = defs.entries[i];
        if (entry.file_name.empty() || entry.file_name.rfind("NULLDEF_", 0) == 0) {
            continue;
        }
        name_to_index[entry.file_name] = i;
        const std::string rel =
            heuristic_relative_path(binary, ctx.entries[i].def_type, entry.is_template);
        grouped[rel].push_back(i);
    }

    const fs::path out_root(output_directory);
    for (auto &[rel, indices] : grouped) {
        // Put templates first, then order within the file so local parents appear first.
        std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
            const bool ta = defs.entries[a].is_template;
            const bool tb = defs.entries[b].is_template;
            if (ta != tb) {
                return ta > tb;
            }
            return defs.entries[a].file_name < defs.entries[b].file_name;
        });

        std::set<std::size_t> in_file(indices.begin(), indices.end());
        std::set<std::size_t> emitted;
        std::vector<std::size_t> ordered;
        ordered.reserve(indices.size());
        auto visit = [&](auto &self, std::size_t idx, std::set<std::size_t> &stack) -> void {
            if (emitted.count(idx) != 0 || !stack.insert(idx).second) {
                return;
            }
            const auto pit = parents.find(defs.entries[idx].file_name);
            if (pit != parents.end()) {
                const auto p_idx_it = name_to_index.find(pit->second);
                if (p_idx_it != name_to_index.end() && in_file.count(p_idx_it->second) != 0) {
                    self(self, p_idx_it->second, stack);
                }
            }
            stack.erase(idx);
            if (emitted.insert(idx).second) {
                ordered.push_back(idx);
            }
        };
        for (std::size_t idx : indices) {
            std::set<std::size_t> stack;
            visit(visit, idx, stack);
        }

        std::string content;
        for (std::size_t idx : ordered) {
            const BinaryEntry &entry = defs.entries[idx];
            emit_definition(entry, ctx.entries[idx], ctx, parents, content);
            ++stats.definitions;
            if (entry.is_template) {
                ++stats.templates;
            }
            if (parents.count(entry.file_name) != 0) {
                ++stats.specialises_recovered;
            }
        }

        const fs::path dest = out_root / rel;
        if (!write_text_file(dest, content, error)) {
            return false;
        }
        ++stats.files_written;
        stats.written_files.push_back(rel);
    }
    return true;
}

bool decompile_directory_to_folder(const std::string &directory,
                                   const std::string &headers_directory,
                                   const std::string &output_directory,
                                   FolderDecompileStats &stats,
                                   std::string &error) {
    namespace fs = std::filesystem;
    stats = FolderDecompileStats{};
    const fs::path dir_path(directory);
    if (!fs::exists(dir_path / "names.bin")) {
        error = "names.bin not found in " + directory;
        return false;
    }

    std::vector<std::string> found;
    for (const char *bin : {"game", "frontend", "script"}) {
        if (fs::exists(dir_path / (std::string(bin) + ".bin"))) {
            found.emplace_back(bin);
        }
    }
    if (found.empty()) {
        error = "no compiled definition binaries (game.bin/frontend.bin/script.bin) found in " +
                directory;
        return false;
    }
    for (const std::string &bin : found) {
        if (!decompile_binary_to_folder(directory, bin, headers_directory,
                                        output_directory, stats, error)) {
            return false;
        }
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
        std::string mismatch;
        if (!changed) {
            const ParsedEntry &original_def = *original_defs[entry.file_name];
            const ParsedEntry &modified_def = modified_ctx.entries[i];
            DecodedValue modified_value;
            modified_value.kind = DecodedValueKind::Struct;
            modified_value.fields = modified_def.fields;
            DecodedValue original_value;
            original_value.kind = DecodedValueKind::Struct;
            original_value.fields = original_def.fields;
            changed = !sem_equal(modified_value, original_value, "", modified_ctx,
                                 original_ctx, &mismatch);

            if (!changed) {
                const auto &modified_subs = entry.sub_defs;
                const auto &original_subs = original_entry->second->sub_defs;
                if (modified_subs.has_value() != original_subs.has_value()) {
                    changed = true;
                    mismatch = "sub_defs presence";
                } else if (modified_subs) {
                    if (modified_subs->size() != original_subs->size()) {
                        changed = true;
                        mismatch = "sub_defs count " + std::to_string(modified_subs->size()) +
                                   " vs " + std::to_string(original_subs->size());
                    } else {
                        for (std::size_t sub = 0; sub < modified_subs->size(); ++sub) {
                            const SubDefRecord &m = (*modified_subs)[sub];
                            const SubDefRecord &o = (*original_subs)[sub];
                            if (m.name_crc != o.name_crc ||
                                m.def_index >= modified_ctx.entries.size() ||
                                o.def_index >= original_ctx.entries.size()) {
                                changed = true;
                                mismatch = "sub_def[" + std::to_string(sub) + "] crc/index " +
                                           std::to_string(m.name_crc) + " vs " +
                                           std::to_string(o.name_crc);
                                break;
                            }
                            DecodedValue mv;
                            mv.kind = DecodedValueKind::Struct;
                            mv.fields = modified_ctx.entries[m.def_index].fields;
                            DecodedValue ov;
                            ov.kind = DecodedValueKind::Struct;
                            ov.fields = original_ctx.entries[o.def_index].fields;
                            if (!sem_equal(mv, ov, "", modified_ctx, original_ctx, &mismatch)) {
                                changed = true;
                                mismatch = "sub_def[" + std::to_string(sub) + "] " + mismatch;
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
