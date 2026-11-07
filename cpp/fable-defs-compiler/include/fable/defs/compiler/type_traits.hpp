// Shared container/pointer traits for the compiler lowering and wire layers.
#pragma once

#include <list>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace fable::defs::compiler::detail {

template <class T>
struct IsVector : std::false_type {};
template <class T, class A>
struct IsVector<std::vector<T, A>> : std::true_type {
    using value_type = T;
};

template <class T>
struct IsList : std::false_type {};
template <class T, class A>
struct IsList<std::list<T, A>> : std::true_type {
    using value_type = T;
};

template <class T>
struct IsSet : std::false_type {};
template <class T, class C, class A>
struct IsSet<std::set<T, C, A>> : std::true_type {
    using value_type = T;
};

template <class T>
struct IsMap : std::false_type {};
template <class K, class V, class C, class A>
struct IsMap<std::map<K, V, C, A>> : std::true_type {
    using key_type = K;
    using mapped_type = V;
};

template <class T>
struct IsMultiMap : std::false_type {};
template <class K, class V, class C, class A>
struct IsMultiMap<std::multimap<K, V, C, A>> : std::true_type {
    using key_type = K;
    using mapped_type = V;
};

template <class T>
struct IsPair : std::false_type {};
template <class A, class B>
struct IsPair<std::pair<A, B>> : std::true_type {};

template <class T>
struct IsSharedPtr : std::false_type {};
template <class T>
struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

} // namespace fable::defs::compiler::detail
