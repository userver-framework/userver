#pragma once

/// @file userver/utils/projected_set.hpp
/// @brief @copybrief utils::ProjectedUnorderedSet

#include <functional>
#include <set>
#include <type_traits>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl::projected_set {

template <typename Raw, auto Projection>
using ProjectionResult =
    std::decay_t<std::invoke_result_t<decltype(Projection), const Raw&>>;

template <typename Raw, auto Projection, typename ResultHash>
using DefaultedResultHash =
    std::conditional_t<std::is_void_v<ResultHash>,
                       std::hash<ProjectionResult<Raw, Projection>>,
                       ResultHash>;

template <typename Raw, auto Projection, typename ResultHash>
struct Hash : public DefaultedResultHash<Raw, Projection, ResultHash> {
  using is_transparent [[maybe_unused]] = void;
  using Base = DefaultedResultHash<Raw, Projection, ResultHash>;

  auto operator()(const Raw& value) const noexcept {
    return Base::operator()(std::invoke(Projection, value));
  }

  using Base::operator();
};

template <typename Raw, auto Projection, typename ResultCompare>
struct Compare : public ResultCompare {
  using is_transparent [[maybe_unused]] = void;

  auto operator()(const Raw& lhs, const Raw& rhs) const noexcept {
    return ResultCompare::operator()(std::invoke(Projection, lhs),
                                     std::invoke(Projection, rhs));
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

}  // namespace impl::projected_set

/// @ingroup userver_containers
/// @brief A `std::unordered_set` that compares its elements (of type @a Value)
/// based on their @a Projection. It allows to create, essentially, an
/// equivalent of `std::unordered_map` where keys are stored inside values.
///
/// Usage example:
/// @snippet utils/projected_set_test.cpp  user
/// @snippet utils/projected_set_test.cpp  usage
template <typename Value, auto Projection, typename Hash = void,
          typename Equal = std::equal_to<>,
          typename Allocator = std::allocator<Value>>
using ProjectedUnorderedSet = std::unordered_set<
    Value, impl::projected_set::Hash<Value, Projection, Hash>,
    impl::projected_set::Compare<Value, Projection, Equal>, Allocator>;

/// @ingroup userver_containers
/// @brief Same as ProjectedUnorderedSet, but for `std::set`.
template <typename Value, auto Projection, typename Compare = std::less<>,
          typename Allocator = std::allocator<Value>>
using ProjectedSet =
    std::set<Value, impl::projected_set::Compare<Value, Projection, Compare>,
             Allocator>;

/// @brief An equivalent of `std::unordered_map::insert_or_assign` for
/// utils::ProjectedUnorderedSet and utils::ProjectedSet.
template <typename Container, typename Value>
void ProjectedInsertOrAssign(Container& set, Value&& value) {
  impl::projected_set::DoInsert(set, std::forward<Value>(value));
}

namespace impl::projected_set {

// Comparing Projected*Set results in only Projections being compared, which
// breaks value semantics. Unfortunately, if we define them as aliases of
// std::*set, we can't have operator== compare full values. The least bad
// decision in this case is to prohibit the comparison.
template <typename V1, const auto& P1, typename H1, typename E1, typename A1,
          typename V2, const auto& P2, typename H2, typename E2, typename A2>
void operator==(const ProjectedUnorderedSet<V1, P1, H1, E1, A1>& lhs,
                const ProjectedUnorderedSet<V2, P2, H2, E2, A2>& rhs) = delete;

template <typename V1, const auto& P1, typename H1, typename E1, typename A1,
          typename V2, const auto& P2, typename H2, typename E2, typename A2>
void operator!=(const ProjectedUnorderedSet<V1, P1, H1, E1, A1>& lhs,
                const ProjectedUnorderedSet<V2, P2, H2, E2, A2>& rhs) = delete;

template <typename V1, const auto& P1, typename C1, typename A1, typename V2,
          const auto& P2, typename C2, typename A2>
void operator==(const ProjectedSet<V1, P1, C1, A1>& lhs,
                const ProjectedSet<V2, P2, C2, A2>& rhs) = delete;

template <typename V1, const auto& P1, typename C1, typename A1, typename V2,
          const auto& P2, typename C2, typename A2>
void operator!=(const ProjectedSet<V1, P1, C1, A1>& lhs,
                const ProjectedSet<V2, P2, C2, A2>& rhs) = delete;

}  // namespace impl::projected_set

}  // namespace utils

USERVER_NAMESPACE_END
