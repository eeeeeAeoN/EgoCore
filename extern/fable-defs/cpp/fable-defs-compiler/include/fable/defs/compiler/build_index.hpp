// Manifest-driven global index allocation for the named def region.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "fable/defs/compiler/ast.hpp"
#include "fable/defs/compiler/manifest.hpp"

namespace fable::defs::compiler {

class BuildIndex {
public:
    static BuildIndex build(BinaryKind kind, const std::vector<SourceAst> &asts);
    static BuildIndex build(BinaryKind kind, const std::vector<const SourceAst *> &asts);

    std::optional<std::uint32_t> index_of(std::string_view name) const;
    std::uint32_t nulldef_count() const { return nulldef_count_; }
    const std::map<std::string, std::uint32_t> &indices() const { return indices_; }
    const std::vector<std::string> &named_order() const { return named_order_; }
    std::size_t named_count() const { return indices_.size(); }
    BinaryKind kind() const { return kind_; }

private:
    BinaryKind kind_ = BinaryKind::Game;
    std::uint32_t nulldef_count_ = 0;
    std::map<std::string, std::uint32_t> indices_;
    std::vector<std::string> named_order_;
};

} // namespace fable::defs::compiler
