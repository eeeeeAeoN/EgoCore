// Fable def model foundation types.
#ifndef FABLE_DEFS_BASE_HPP
#define FABLE_DEFS_BASE_HPP

#include <cstdint>

namespace fable {
namespace defs {

// A names.bin offset (`CDefString`). -1 is the null string.
struct CDefString {
    std::int32_t TablePos = -1;
    bool operator<(const CDefString &other) const { return TablePos < other.TablePos; }
    bool operator==(const CDefString &other) const { return TablePos == other.TablePos; }
};

// A global def entry index (`CDefIndex`). 0 resolves to a class's NULLDEF.
struct CDefIndex {
    std::int32_t Val;
    bool operator<(const CDefIndex &other) const { return Val < other.Val; }
    bool operator==(const CDefIndex &other) const { return Val == other.Val; }
};

} // namespace defs
} // namespace fable

#endif // FABLE_DEFS_BASE_HPP
