// Header/enum name resolution for the def decompiler.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/symbols.hpp"

namespace fable::defs::compiler {

struct SymDomain {
    enum class Kind {
        Anim,
        Mesh,
        Texture,
        Text,
        Particle,
        Sound,
        LipSync,
        Font,
        EngMesh,
        AnimFlags,
        Handedness,
        ExpressionType,
        BodyLocation,
        BodyPart,
        HeroMorph,
        MeshLocation,
        ObjectAugmentation,
        EngineGraphicType,
        Opinion,
        OpinionReactionType,
        OpinionAttitudeType,
        OpinionTargetingConditionType,
        OpinionDeedType,
        AttackHistoryType,
        DamageAttribute,
        Blend,
        EnumGroup,
    };

    Kind kind = Kind::Anim;
    std::size_t group = 0;

    static SymDomain named(Kind kind) { return SymDomain{kind, 0}; }
    static SymDomain enum_group(std::size_t group) {
        return SymDomain{Kind::EnumGroup, group};
    }

    friend bool operator<(const SymDomain &a, const SymDomain &b) {
        return a.kind == b.kind ? a.group < b.group
                                : static_cast<int>(a.kind) < static_cast<int>(b.kind);
    }
    friend bool operator==(const SymDomain &a, const SymDomain &b) {
        return a.kind == b.kind && a.group == b.group;
    }
};

struct EnumGroupTable {
    std::map<std::int32_t, std::string> val_to_sym;
    std::vector<std::pair<std::int32_t, std::string>> bit_flags;

    void insert(std::int32_t value, std::string name);
    std::optional<std::string> resolve(std::int32_t value, bool as_bitflags) const;
};

class HeaderResolver {
public:
    static HeaderResolver build(const std::string &target_defs_directory);

    std::string resolve_domain(SymDomain domain, std::int32_t value) const;
    std::string resolve_field(const std::string &scope, const std::string &field_name,
                              std::int32_t value) const;
    bool has_map_key_domain(const std::string &scope,
                            const std::string &field_name) const;
    std::string resolve_map_key(const std::string &scope,
                                const std::string &field_name,
                                std::int32_t value) const;

private:
    void collect_enum_items(const std::string &relative_path,
                            const std::vector<Item> &items, const SymbolTable &symbols);
    void record_enum(const std::string &relative_path, const EnumDecl &decl,
                     const SymbolTable &symbols);

    std::map<SymDomain, EnumGroupTable> domains_;
    std::vector<EnumGroupTable> enum_groups_;
    std::map<std::pair<std::string, std::string>, std::pair<SymDomain, bool>>
        field_domains_;
};

} // namespace fable::defs::compiler
