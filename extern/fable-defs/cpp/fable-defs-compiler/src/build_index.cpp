#include "fable/defs/compiler/build_index.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace fable::defs::compiler {

BuildIndex BuildIndex::build(BinaryKind kind, const std::vector<SourceAst> &asts) {
    std::vector<const SourceAst *> pointers;
    pointers.reserve(asts.size());
    for (const auto &ast : asts) {
        pointers.push_back(&ast);
    }
    return build(kind, pointers);
}

BuildIndex BuildIndex::build(BinaryKind kind,
                             const std::vector<const SourceAst *> &asts) {
    BuildIndex out;
    out.kind_ = kind;
    out.nulldef_count_ = static_cast<std::uint32_t>(null_entry_count(kind));

    for (const SourceAst *ast : asts) {
        for (const auto *definition : ast->definitions()) {
            const Definition &def = definition->value;
            if (out.indices_.find(def.name) != out.indices_.end()) {
                continue;
            }
            if (!is_allowed_type(kind, def.def_type)) {
                continue;
            }
            const std::uint32_t index =
                out.nulldef_count_ + static_cast<std::uint32_t>(out.indices_.size());
            out.indices_.emplace(def.name, index);
            out.named_order_.push_back(def.name);
        }
    }
    return out;
}

std::optional<std::uint32_t> BuildIndex::index_of(std::string_view name) const {
    auto it = indices_.find(std::string(name));
    if (it == indices_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace fable::defs::compiler
