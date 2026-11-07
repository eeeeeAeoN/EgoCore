#include "fable/defs/compiler/definitions.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

namespace fable::defs::compiler {

void DefinitionIndex::add(const SourceAst &ast) {
    for (const auto &item : ast.items) {
        if (item.kind != Item::Kind::Definition) {
            continue;
        }
        const std::string &name = item.definition.value.name;
        indices_.try_emplace(name, next_index_++);
        by_name_[name] = &item.definition.value;
    }
}

const Definition *DefinitionIndex::find(const std::string &name) const {
    auto it = by_name_.find(name);
    return it == by_name_.end() ? nullptr : it->second;
}

void DefinitionIndex::apply_indices(
    const std::map<std::string, std::uint32_t> &indices) {
    for (const auto &[name, value] : indices) {
        indices_[name] = static_cast<std::int32_t>(value);
    }
}

std::optional<std::int32_t> DefinitionIndex::index_of(const std::string &name) const {
    auto it = indices_.find(name);
    if (it == indices_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::variant<Chain, ChainError> specialization_chain(const DefinitionIndex &index,
                                                     const std::string &name) {
    Chain chain;
    std::set<std::string> seen;
    const Definition *current = index.find(name);
    if (current == nullptr) {
        return ChainError{Span{}, "unknown definition `" + name + "`"};
    }
    while (current != nullptr) {
        if (!seen.insert(current->name).second) {
            return ChainError{current->span,
                              "specialization cycle at `" + current->name + "`"};
        }
        chain.push_back(current);
        if (!current->specializes) {
            break;
        }
        const Definition *parent = index.find(*current->specializes);
        if (parent == nullptr) {
            return ChainError{current->specializes_span,
                              "unknown specialised parent `" + *current->specializes +
                                  "`"};
        }
        current = parent;
    }
    std::reverse(chain.begin(), chain.end());
    return chain;
}

Definition flatten_chain(const Chain &chain) {
    Definition out;
    if (chain.empty()) {
        return out;
    }
    const Definition *derived = chain.back();
    out.is_template = derived->is_template;
    out.def_type = derived->def_type;
    out.name = derived->name;
    out.specializes = derived->specializes;
    out.specializes_span = derived->specializes_span;
    out.span = derived->span;
    for (const Definition *definition : chain) {
        out.body.insert(out.body.end(), definition->body.begin(),
                        definition->body.end());
    }
    return out;
}

} // namespace fable::defs::compiler
