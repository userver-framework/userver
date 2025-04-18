#pragma once

/// @file userver/utils/projected_set.hpp
/// @brief @copybrief utils::ProjectedUnorderedSet

#include <functional>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <userver/utils/impl/projecting_view.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl::projected_set {

template <typename Raw, auto Projection>
using ProjectionResult = std::decay_t<std::invoke_result_t<decltype(Projection), const Raw&>>;

template <typename Raw, auto Projection, typename ResultHash>
using DefaultedResultHash =
    std::conditional_t<std::is_void_v<ResultHash>, std::hash<ProjectionResult<Raw, Projection>>, ResultHash>;

template <typename Raw, auto Projection, typename ResultHash>
struct Hash : public DefaultedResultHash<Raw, Projection, ResultHash> {
    using is_transparent [[maybe_unused]] = void;
    using Base = DefaultedResultHash<Raw, Projection, ResultHash>;

    auto operator()(const Raw& value) const noexcept { return Base::operator()(std::invoke(Projection, value)); }

    using Base::operator();
};

template <typename Raw, auto Projection, typename ResultCompare>
struct Compare : public ResultCompare {
    using is_transparent [[maybe_unused]] = void;

    auto operator()(const Raw& lhs, const Raw& rhs) const noexcept {
        return ResultCompare::operator()(std::invoke(Projection, lhs), std::invoke(Projection, rhs));
    }

    template <typename T>
    auto operator()(const Raw& lhs, const T& rhs) const noexcept {
        return ResultCompare::operator()(std::invoke(Projection, lhs), rhs);
    }

    template <typename T>
    auto operator()(const T& lhs, const Raw& rhs) const noexcept {
        return ResultCompare::operator()(lhs, std::invoke(Projection, rhs));
    }

    template <typename T, typename U>
    auto operator()(const T& lhs, const U& rhs) const noexcept {
        return ResultCompare::operator()(lhs, rhs);
    }
};

template <typename Set, typename Value>
void DoInsert(Set& set, Value&& value) {
    const auto [iter, success] = set.insert(std::forward<Value>(value));
    if (!success) {
        using SetValue = std::decay_t<decltype(*iter)>;
        // 'const_cast' is safe here, because the new key compares equal to the
        // old one and should have the same ordering (or hash) as the old one.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const_cast<SetValue&>(*iter) = std::forward<Value>(value);
    }
}

template <typename T>
using HasHasher = typename T::hasher;

}  // namespace impl::projected_set

/// @ingroup userver_universal
/// @brief A `std::unordered_set` that compares its elements (of type @a Value)
/// based on their @a Projection. It allows to create, essentially, an
/// equivalent of `std::unordered_map` where keys are stored inside values.
///
/// Usage example:
/// @snippet utils/projected_set_test.cpp  user
/// @snippet utils/projected_set_test.cpp  usage
///
/// @see @ref utils::ProjectedInsertOrAssign
/// @see @ref utils::ProjectedFind
template <
    typename Value,
    auto Projection,
    typename Hash = void,
    typename Equal = std::equal_to<>,
    typename Allocator = std::allocator<Value>>
using ProjectedUnorderedSet = utils::impl::TransparentSet<
    Value,
    impl::projected_set::Hash<Value, Projection, Hash>,
    impl::projected_set::Compare<Value, Projection, Equal>,
    Allocator>;

/// @ingroup userver_universal
/// @brief Same as @ref utils::ProjectedUnorderedSet, but for `std::set`.
template <typename Value, auto Projection, typename Compare = std::less<>, typename Allocator = std::allocator<Value>>
using ProjectedSet = std::set<Value, impl::projected_set::Compare<Value, Projection, Compare>, Allocator>;

/// @brief An equivalent of `std::unordered_map::insert_or_assign` for
/// utils::ProjectedUnorderedSet and utils::ProjectedSet.
template <typename Container, typename Value>
void ProjectedInsertOrAssign(Container& set, Value&& value) {
    impl::projected_set::DoInsert(set, std::forward<Value>(value));
}

/// @brief An equivalent of `std::unordered_map::find` for @ref utils::ProjectedUnorderedSet
/// and @ref utils::ProjectedSet.
///
/// Only required for pre-C++20 compilers, can just use `set.find(key)` otherwise.
///
/// @note Always returns const iterator, even for a non-const `set` parameter.
template <typename Container, typename Key>
auto ProjectedFind(Container& set, const Key& key) {
    if constexpr (meta::kIsDetected<impl::projected_set::HasHasher, std::decay_t<Container>>) {
        return utils::impl::FindTransparent(set, key);
    } else {
        return set.find(key);
    }
}

namespace impl {

/// @name Mutating elements inside utils::ProjectedUnorderedSet.
/// @{

/// @brief Used to work around the fact that mutation is prohibited inside utils::ProjectedUnorderedSet.
///
/// @warning Use with utmost caution! Mutating the part of the values that serves as key invokes UB.
template <typename Value>
class MutableWrapper {
public:
    template <typename... Args>
    /*implicit*/ MutableWrapper(Args&&... args) : value_(std::forward<Args>(args)...) {}

    Value& operator*() const { return value_; }
    Value* operator->() const { return std::addressof(value_); }

private:
    mutable Value value_;
};

template <typename Container, typename Key>
auto ProjectedFindOrNullptrUnsafe(Container& set, const Key& key) {
    auto iter = utils::ProjectedFind(set, key);
    if constexpr (std::is_const_v<Container>) {
        return iter == set.end() ? nullptr : std::addressof(std::as_const(**iter));
    } else {
        return iter == set.end() ? nullptr : std::addressof(**iter);
    }
}
/// @}

}  // namespace impl

namespace impl::projected_set {

// Comparing Projected*Set results in only Projections being compared, which
// breaks value semantics. Unfortunately, if we define them as aliases of
// std::*set, we can't have operator== compare full values. The least bad
// decision in this case is to prohibit the comparison.
template <
    typename V1,
    const auto& P1,
    typename H1,
    typename E1,
    typename A1,
    typename V2,
    const auto& P2,
    typename H2,
    typename E2,
    typename A2>
void operator==(
    const ProjectedUnorderedSet<V1, P1, H1, E1, A1>& lhs,
    const ProjectedUnorderedSet<V2, P2, H2, E2, A2>& rhs
) = delete;

template <
    typename V1,
    const auto& P1,
    typename H1,
    typename E1,
    typename A1,
    typename V2,
    const auto& P2,
    typename H2,
    typename E2,
    typename A2>
void operator!=(
    const ProjectedUnorderedSet<V1, P1, H1, E1, A1>& lhs,
    const ProjectedUnorderedSet<V2, P2, H2, E2, A2>& rhs
) = delete;

template <typename V1, const auto& P1, typename C1, typename A1, typename V2, const auto& P2, typename C2, typename A2>
void operator==(const ProjectedSet<V1, P1, C1, A1>& lhs, const ProjectedSet<V2, P2, C2, A2>& rhs) = delete;

template <typename V1, const auto& P1, typename C1, typename A1, typename V2, const auto& P2, typename C2, typename A2>
void operator!=(const ProjectedSet<V1, P1, C1, A1>& lhs, const ProjectedSet<V2, P2, C2, A2>& rhs) = delete;

}  // namespace impl::projected_set

}  // namespace utils

USERVER_NAMESPACE_END
