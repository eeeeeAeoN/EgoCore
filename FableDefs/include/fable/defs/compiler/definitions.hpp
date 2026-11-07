// Definition index and specialization chains.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/diagnostic.hpp"

namespace fable::defs::compiler {

/// Every definition in the corpus, by name. Later files overwrite earlier ones,
/// matching the builder's whole-body replacement.
class DefinitionIndex {
public:
    void add(const SourceAst &ast);
    const Definition *find(const std::string &name) const;
    const std::map<std::string, const Definition *> &definitions() const {
        return by_name_;
    }
    /// A provisional first-seen index for a named definition. The real build
    /// index (NULLDEFs first, manifest-filtered named entries) replaces this.
    std::optional<std::int32_t> index_of(const std::string &name) const;
    /// Replace the provisional first-seen indices with the manifest build index.
    void apply_indices(const std::map<std::string, std::uint32_t> &indices);

private:
    std::map<std::string, const Definition *> by_name_;
    std::map<std::string, std::int32_t> indices_;
    std::int32_t next_index_ = 1;
};

struct ChainError {
    Span span;
    std::string message;
};

/// Most-distant ancestor first, ending with the requested definition.
using Chain = std::vector<const Definition *>;

std::variant<Chain, ChainError> specialization_chain(const DefinitionIndex &index,
                                                     const std::string &name);

/// Concatenate a chain's bodies (parent first) into one definition.
Definition flatten_chain(const Chain &chain);

} // namespace fable::defs::compiler
