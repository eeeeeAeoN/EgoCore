#include "fable/defs/compiler/header_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string_view>
#include <vector>

#include "fable/defs/compiler/parser.hpp"

namespace fable::defs::compiler {
namespace {

namespace fs = std::filesystem;

bool ends_with(std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
}

bool is_valid_identifier(std::string_view text) {
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

bool is_sentinel_symbol(const std::string &name) {
    return name.rfind("MAX_", 0) == 0 || name.rfind("NUM_", 0) == 0 ||
           name.rfind("FIRST_", 0) == 0 || name.rfind("LAST_", 0) == 0 ||
           ends_with(name, "_MAX") || ends_with(name, "_LAST") ||
           ends_with(name, "_FIRST") || ends_with(name, "_COUNT") ||
           ends_with(name, "_END") || ends_with(name, "_TOTAL") ||
           name.rfind("DUMMY_", 0) == 0;
}

std::string normalize(const fs::path &path) {
    std::string text = path.string();
    std::replace(text.begin(), text.end(), '\\', '/');
    return text;
}

bool read_text(const fs::path &path, std::string &text) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

void collect_files(const fs::path &root, const fs::path &current,
                   const std::string &extension, std::vector<fs::path> &out) {
    std::error_code error;
    for (fs::recursive_directory_iterator it(current, error), end;
         !error && it != end; it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }
        const fs::path path = it->path();
        if (path.extension().string() == extension) {
            out.push_back(path);
        }
    }
    (void)root;
}

void collect_expr_symbols(const Expr &expr, std::vector<std::string> &out,
                          bool &is_bitor) {
    if (expr.kind == Expr::Kind::Symbol) {
        if (expr.text != "NULL" && expr.text != "TRUE" && expr.text != "FALSE" &&
            expr.text != "BTRUE" && expr.text != "BFALSE") {
            out.push_back(expr.text);
        }
    } else if (expr.kind == Expr::Kind::BitOr) {
        is_bitor = true;
        for (const Expr &term : expr.terms) {
            collect_expr_symbols(term, out, is_bitor);
        }
    }
}

using Votes = std::map<std::pair<std::string, std::string>,
                       std::map<std::size_t, std::size_t>>;

void record_symbol_votes(const std::string &scope, const std::string &key,
                         const Expr &expr,
                         const std::map<std::string, std::vector<std::size_t>> &groups,
                         Votes &votes, std::set<std::pair<std::string, std::string>> &bitor_seen) {
    std::vector<std::string> symbols;
    bool is_bitor = false;
    collect_expr_symbols(expr, symbols, is_bitor);
    if (symbols.empty()) {
        return;
    }
    if (is_bitor) {
        bitor_seen.insert({scope, key});
        bitor_seen.insert({"*", key});
    }
    for (const std::string &symbol : symbols) {
        const auto found = groups.find(symbol);
        if (found == groups.end()) {
            continue;
        }
        for (std::size_t group : found->second) {
            ++votes[{scope, key}][group];
            ++votes[{"*", key}][group];
        }
    }
}

void train_from_statement(
    const std::string &scope, const Statement &statement,
    const std::map<std::string, std::vector<std::size_t>> &groups, Votes &votes,
    std::set<std::pair<std::string, std::string>> &bitor_seen) {
    if (statement.kind == Statement::Kind::Field) {
        std::string last_field;
        for (const PathSegment &segment : statement.field.path.segments) {
            if (segment.kind == PathSegment::Kind::Field) {
                last_field = segment.field;
            } else if (!last_field.empty()) {
                record_symbol_votes(scope, last_field + "[key]", segment.index, groups,
                                    votes, bitor_seen);
            }
        }
        if (!last_field.empty()) {
            record_symbol_votes(scope, last_field, statement.field.expr, groups, votes,
                                bitor_seen);
        }
    } else if (statement.kind == Statement::Kind::MethodCall) {
        if (statement.method_call.object.segments.empty()) {
            return;
        }
        const PathSegment &object = statement.method_call.object.segments.back();
        if (object.kind == PathSegment::Kind::Field &&
            statement.method_call.method == "Add" &&
            statement.method_call.arguments.size() == 1) {
            record_symbol_votes(scope, object.field,
                                statement.method_call.arguments.front(), groups, votes,
                                bitor_seen);
        }
    } else if (statement.kind == Statement::Kind::TaggedBlock && statement.tagged_block) {
        for (const Statement &child : statement.tagged_block->body) {
            train_from_statement(statement.tagged_block->tag, child, groups, votes,
                                 bitor_seen);
        }
    }
}

} // namespace

void EnumGroupTable::insert(std::int32_t value, std::string name) {
    const auto it = val_to_sym.find(value);
    if (it == val_to_sym.end()) {
        val_to_sym[value] = name;
    } else if (is_sentinel_symbol(it->second) && !is_sentinel_symbol(name)) {
        it->second = name;
    }

    if (value > 0 && (value & (value - 1)) == 0 && !is_sentinel_symbol(name)) {
        bool found = false;
        for (auto &[bit, symbol] : bit_flags) {
            if (bit == value) {
                if (is_sentinel_symbol(symbol)) {
                    symbol = name;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            bit_flags.emplace_back(value, std::move(name));
        }
    }
}

std::optional<std::string> EnumGroupTable::resolve(std::int32_t value,
                                                    bool as_bitflags) const {
    if (as_bitflags) {
        if (value == 0) {
            const auto zero = val_to_sym.find(0);
            return zero == val_to_sym.end() ? std::optional<std::string>("0")
                                            : std::optional<std::string>(zero->second);
        }
        if (value > 0 && !bit_flags.empty()) {
            std::int32_t remaining = value;
            std::vector<std::string> terms;
            for (const auto &[bit, symbol] : bit_flags) {
                if (bit > 0 && (remaining & bit) == bit) {
                    remaining &= ~bit;
                    terms.push_back(symbol);
                }
            }
            if (remaining == 0 && !terms.empty()) {
                std::string out;
                for (std::size_t i = 0; i < terms.size(); ++i) {
                    if (i != 0) {
                        out += " | ";
                    }
                    out += terms[i];
                }
                return out;
            }
        }
    }
    const auto it = val_to_sym.find(value);
    return it == val_to_sym.end() ? std::nullopt
                                  : std::optional<std::string>(it->second);
}

HeaderResolver HeaderResolver::build(const std::string &target_defs_directory) {
    HeaderResolver resolver;
    if (target_defs_directory.empty()) {
        return resolver;
    }
    const fs::path root(target_defs_directory);

    std::string selected_set;
    for (const char *candidate : {"RetailHeaders", "DevHeaders"}) {
        if (fs::is_directory(root / candidate)) {
            selected_set = candidate;
            break;
        }
    }

    std::vector<fs::path> header_files;
    collect_files(root, root, ".h", header_files);
    std::sort(header_files.begin(), header_files.end());

    std::vector<std::pair<std::string, SourceAst>> parsed_headers;
    SymbolTable symbols;
    const std::string root_prefix = normalize(root);
    for (const fs::path &path : header_files) {
        const std::string full = normalize(path);
        std::string relative = full;
        if (relative.rfind(root_prefix, 0) == 0) {
            relative = relative.substr(root_prefix.size());
        }
        if (relative.empty()) {
            relative = "/";
        } else if (relative.front() != '/') {
            relative.insert(relative.begin(), '/');
        }
        const std::string lower = [&] {
            std::string out = relative;
            std::transform(out.begin(), out.end(), out.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        }();
        if (lower.find("/xbox/") != std::string::npos ||
            lower.find("scriptdialoguesnds2") != std::string::npos) {
            continue;
        }
        if (!selected_set.empty()) {
            for (const char *other : {"RetailHeaders", "DevHeaders"}) {
                if (selected_set != other &&
                    lower.find(std::string("/") + other + "/") != std::string::npos) {
                    goto skip_header;
                }
            }
        }

        {
            std::string text;
            if (!read_text(path, text)) {
                continue;
            }
            ParseResult parsed = parse_source(text, FileId::anonymous());
            if (auto *error = std::get_if<ParseError>(&parsed)) {
                (void)error;
                continue;
            }
            SourceAst ast = std::move(std::get<SourceAst>(parsed));
            std::vector<Redefinition> redefined;
            (void)symbols.evaluate_items(ast.items, redefined);
            parsed_headers.emplace_back(relative.substr(1), std::move(ast));
        }
    skip_header:;
    }

    // Engine fallbacks that no text header declares.
    for (const auto &[name, value] : std::vector<std::pair<std::string, std::int64_t>>{
             {"WATER_BUMP_PC", 0}, {"BLEND_NULL", 0}, {"BLEND_ADDITIVE", 1},
             {"BLEND_ALPHA", 2}, {"BLEND_SOLID", 3}, {"BLEND_MULTIPLY", 4}}) {
        if (!symbols.lookup(name)) {
            symbols.insert(name, value);
        }
    }

    {
        EnumGroupTable blend;
        for (const auto &[name, value] : std::vector<std::pair<std::string, std::int32_t>>{
                 {"BLEND_NULL", 0}, {"BLEND_ADDITIVE", 1}, {"BLEND_ALPHA", 2},
                 {"BLEND_SOLID", 3}, {"BLEND_MULTIPLY", 4}}) {
            blend.insert(value, name);
        }
        resolver.domains_[SymDomain::named(SymDomain::Kind::Blend)] = std::move(blend);
    }

    std::map<std::string, std::vector<std::size_t>> symbol_groups;
    for (const auto &[relative, ast] : parsed_headers) {
        resolver.collect_enum_items(relative, ast.items, symbols);
    }
    // Rebuild the symbol-group index from the enum tables.
    for (std::size_t gid = 0; gid < resolver.enum_groups_.size(); ++gid) {
        for (const auto &[value, name] : resolver.enum_groups_[gid].val_to_sym) {
            (void)value;
            symbol_groups[name].push_back(gid);
        }
    }

    std::vector<SourceAst> parsed_defs;
    std::vector<fs::path> def_files;
    collect_files(root, root, ".def", def_files);
    collect_files(root, root, ".tpl", def_files);
    std::sort(def_files.begin(), def_files.end());
    parsed_defs.reserve(def_files.size());
    for (const fs::path &path : def_files) {
        const std::string filename = path.filename().string();
        if (filename.rfind("z_mod_decompiled", 0) == 0 ||
            filename.find("_deprecated.") != std::string::npos) {
            continue;
        }
        std::string text;
        if (!read_text(path, text)) {
            continue;
        }
        ParseResult parsed = parse_source(text, FileId::anonymous());
        if (std::holds_alternative<ParseError>(parsed)) {
            continue;
        }
        SourceAst ast = std::move(std::get<SourceAst>(parsed));
        std::vector<Redefinition> redefined;
        (void)symbols.evaluate_items(ast.items, redefined);
        parsed_defs.push_back(std::move(ast));
    }

    // Enums may also be declared in .def files.
    for (const SourceAst &ast : parsed_defs) {
        resolver.collect_enum_items("defs", ast.items, symbols);
    }
    symbol_groups.clear();
    for (std::size_t gid = 0; gid < resolver.enum_groups_.size(); ++gid) {
        for (const auto &[value, name] : resolver.enum_groups_[gid].val_to_sym) {
            (void)value;
            symbol_groups[name].push_back(gid);
        }
    }

    Votes votes;
    std::set<std::pair<std::string, std::string>> bitor_seen;
    for (const SourceAst &ast : parsed_defs) {
        for (const auto &item : ast.items) {
            if (item.kind != Item::Kind::Definition) {
                continue;
            }
            const Definition &definition = item.definition.value;
            for (const Statement &statement : definition.body) {
                train_from_statement(definition.def_type, statement, symbol_groups, votes,
                                     bitor_seen);
            }
        }
    }
    for (const auto &[key, counts] : votes) {
        const auto best = std::max_element(
            counts.begin(), counts.end(),
            [](const auto &a, const auto &b) { return a.second < b.second; });
        if (best == counts.end()) {
            continue;
        }
        const bool is_bitflags =
            bitor_seen.count(key) != 0 || bitor_seen.count({"*", key.second}) != 0;
        resolver.field_domains_[key] = {
            SymDomain::enum_group(best->first), is_bitflags};
    }
    return resolver;
}

void HeaderResolver::collect_enum_items(const std::string &relative_path,
                                        const std::vector<Item> &items,
                                        const SymbolTable &symbols) {
    for (const Item &item : items) {
        if (item.kind == Item::Kind::Enum) {
            record_enum(relative_path, item.enum_decl, symbols);
        } else if (item.kind == Item::Kind::Namespace && item.namespace_) {
            collect_enum_items(relative_path, item.namespace_->items, symbols);
        } else if (item.kind == Item::Kind::Conditional && item.conditional) {
            const IfDef &conditional = *item.conditional;
            const bool taken = (conditional.condition == "_WINDOWS") != conditional.inverted;
            if (taken) {
                collect_enum_items(relative_path, conditional.if_branch, symbols);
            } else if (conditional.else_branch) {
                collect_enum_items(relative_path, *conditional.else_branch, symbols);
            }
        }
    }
}

void HeaderResolver::record_enum(const std::string &relative_path, const EnumDecl &decl,
                                 const SymbolTable &symbols) {
    const std::string enum_name = decl.name ? decl.name->value : std::string{};
    std::string lower = relative_path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::optional<SymDomain> named_domain;
    const auto named = [&](SymDomain::Kind kind) {
        named_domain = SymDomain::named(kind);
    };
    if ((enum_name == "EAnimType2" && ends_with(lower, "meshdata.h"))) {
        named(SymDomain::Kind::Anim);
    } else if (enum_name == "EMeshType2" && ends_with(lower, "meshdata.h")) {
        named(SymDomain::Kind::Mesh);
    } else if ((enum_name == "EEngineGraphic" || enum_name == "EGuiGraphicBank") &&
               (ends_with(lower, "textures.h") || ends_with(lower, "gui_bank.h"))) {
        named(SymDomain::Kind::Texture);
    } else if ((enum_name == "EGameText" || enum_name == "ETextGroup") &&
               ends_with(lower, "text.h")) {
        named(SymDomain::Kind::Text);
    } else if ((enum_name == "EParticleEmitter" || enum_name == "EFrontEndParticleEmitter" ||
                enum_name == "EEngineGeneratedEffect") &&
               (ends_with(lower, "particles.h") || ends_with(lower, "particles_frontend.h") ||
                ends_with(lower, "generated_effect.h"))) {
        named(SymDomain::Kind::Particle);
    } else if (ends_with(lower, "gamesnds.h") || ends_with(lower, "dialoguesnds.h") ||
               ends_with(lower, "dialoguesnds2.h") ||
               ends_with(lower, "scriptdialoguesnds.h")) {
        named(SymDomain::Kind::Sound);
    } else if (lower.find("lipsync") != std::string::npos) {
        named(SymDomain::Kind::LipSync);
    } else if (enum_name == "EFont" && ends_with(lower, "fonts.h")) {
        named(SymDomain::Kind::Font);
    } else if (enum_name == "EEngineMeshType" && ends_with(lower, "engmesh.h")) {
        named(SymDomain::Kind::EngMesh);
    } else if (enum_name == "EAnimationFlags") {
        named(SymDomain::Kind::AnimFlags);
    } else if (enum_name == "ECombatAnimationHandedness") {
        named(SymDomain::Kind::Handedness);
    } else if (enum_name == "EExpressionType") {
        named(SymDomain::Kind::ExpressionType);
    } else if (enum_name == "EBodyLocation") {
        named(SymDomain::Kind::BodyLocation);
    } else if (enum_name == "EBodyPartType") {
        named(SymDomain::Kind::BodyPart);
    } else if (enum_name == "EHeroMorphType") {
        named(SymDomain::Kind::HeroMorph);
    } else if (enum_name == "EMeshLocationPoint") {
        named(SymDomain::Kind::MeshLocation);
    } else if (enum_name == "EObjectAugmentationType") {
        named(SymDomain::Kind::ObjectAugmentation);
    } else if (enum_name == "EEngineGraphicType") {
        named(SymDomain::Kind::EngineGraphicType);
    } else if (enum_name == "EOpinion") {
        named(SymDomain::Kind::Opinion);
    } else if (enum_name == "EOpinionReactionType") {
        named(SymDomain::Kind::OpinionReactionType);
    } else if (enum_name == "EOpinionAttitudeType") {
        named(SymDomain::Kind::OpinionAttitudeType);
    } else if (enum_name == "EOpinionTargetingConditionType") {
        named(SymDomain::Kind::OpinionTargetingConditionType);
    } else if (enum_name == "EOpinionDeedType") {
        named(SymDomain::Kind::OpinionDeedType);
    } else if (enum_name == "EAttackHistoryType") {
        named(SymDomain::Kind::AttackHistoryType);
    } else if (enum_name == "EDamageAttribute") {
        named(SymDomain::Kind::DamageAttribute);
    }

    EnumGroupTable group;
    const std::size_t gid = enum_groups_.size();
    for (const EnumVariant &variant : decl.variants) {
        const std::string &name = variant.name.value;
        if (!is_valid_identifier(name)) {
            continue;
        }
        const auto value = symbols.lookup(name);
        if (!value) {
            continue;
        }
        const std::int32_t v = static_cast<std::int32_t>(*value);
        group.insert(v, name);
        if (named_domain) {
            domains_[*named_domain].insert(v, name);
        }
    }
    (void)gid;
    enum_groups_.push_back(std::move(group));
}

std::string HeaderResolver::resolve_domain(SymDomain domain,
                                           std::int32_t value) const {
    const bool bitflags = domain.kind == SymDomain::Kind::AnimFlags ||
                          domain.kind == SymDomain::Kind::ObjectAugmentation;
    if (domain.kind == SymDomain::Kind::EnumGroup) {
        if (domain.group < enum_groups_.size()) {
            if (auto resolved = enum_groups_[domain.group].resolve(value, false)) {
                return *resolved;
            }
        }
        return std::to_string(value);
    }
    const auto it = domains_.find(domain);
    if (it != domains_.end()) {
        if (auto resolved = it->second.resolve(value, bitflags)) {
            return *resolved;
        }
    }
    return std::to_string(value);
}

std::string HeaderResolver::resolve_field(const std::string &scope,
                                          const std::string &field_name,
                                          std::int32_t value) const {
    auto found = field_domains_.find({scope, field_name});
    if (found == field_domains_.end()) {
        found = field_domains_.find({"*", field_name});
    }
    if (found != field_domains_.end()) {
        const auto &[domain, is_bitflags] = found->second;
        if (domain.kind == SymDomain::Kind::EnumGroup) {
            if (domain.group < enum_groups_.size()) {
                if (auto resolved = enum_groups_[domain.group].resolve(value, is_bitflags)) {
                    return *resolved;
                }
            }
        } else {
            const auto table = domains_.find(domain);
            if (table != domains_.end()) {
                if (auto resolved = table->second.resolve(value, is_bitflags)) {
                    return *resolved;
                }
            }
        }
    }

    if (field_name == "PhonemeAnim") {
        return resolve_domain(SymDomain::named(SymDomain::Kind::Anim), value);
    }
    if (value > 0) {
        if (field_name == "BodyParticleEffect" || field_name == "DeadBodyDecayEffect" ||
            field_name == "StunnedParticle" || field_name == "PointerParticleEffect" ||
            ends_with(field_name, "ParticleEffect") || ends_with(field_name, "Particle") ||
            ends_with(field_name, "ParticleEmitter")) {
            return resolve_domain(SymDomain::named(SymDomain::Kind::Particle), value);
        }
        if (field_name == "ThankingPhrase" || field_name == "IgnoringPhrase" ||
            field_name == "BeerRequestPhrase" || field_name == "InitialHawkingPhrase" ||
            field_name == "ReplyToRequestPhrase" ||
            field_name == "CustomerFollowPhrase" || field_name == "ItemName" ||
            field_name == "ItemDescription" || field_name == "ItemDetails" ||
            field_name == "TitleText" || field_name == "DescriptionText" ||
            field_name == "EmptySlotName" || ends_with(field_name, "Phrase") ||
            ends_with(field_name, "TextID") || ends_with(field_name, "TextTag")) {
            return resolve_domain(SymDomain::named(SymDomain::Kind::Text), value);
        }
        if (field_name == "PrimaryMesh" || field_name == "SecondaryMesh" ||
            field_name == "MeshIndex" || field_name == "MeshID") {
            return resolve_domain(SymDomain::named(SymDomain::Kind::Mesh), value);
        }
        if (field_name == "EmoteIconForAvailableWitness") {
            return resolve_domain(SymDomain::named(SymDomain::Kind::Texture), value);
        }
    }
    return std::to_string(value);
}

bool HeaderResolver::has_map_key_domain(const std::string &scope,
                                        const std::string &field_name) const {
    const std::string key = field_name + "[key]";
    return field_domains_.count({scope, key}) != 0 ||
           field_domains_.count({"*", key}) != 0;
}

std::string HeaderResolver::resolve_map_key(const std::string &scope,
                                            const std::string &field_name,
                                            std::int32_t value) const {
    return resolve_field(scope, field_name + "[key]", value);
}

} // namespace fable::defs::compiler
