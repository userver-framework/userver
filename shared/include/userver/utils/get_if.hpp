#pragma once

/// @file userver/utils/get_if.hpp
/// @brief @copybrief utils::GetIf

#include <functional>
#include <memory>
#include <utility>

#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
using IsPointerLike =
    decltype(std::declval<T&>() ? std::addressof(*std::declval<T&&>())
                                : nullptr);

template <typename T>
inline constexpr bool kIsPointerLike = meta::kIsDetected<IsPointerLike, T>;

}  // namespace impl

template <typename Leaf>
constexpr auto* GetIf(Leaf&& leaf) {
  if constexpr (impl::kIsPointerLike<Leaf>) {
    return leaf ? utils::GetIf(*std::forward<Leaf>(leaf)) : nullptr;
  } else {
    return std::addressof(std::forward<Leaf>(leaf));
  }
}

/// @brief Dereferences a chain of indirections and compositions,
/// returns nullptr if one of the chain elements is not set
///
/// @snippet shared/src/utils/get_if_test.cpp Sample Usage
template <typename Root, typename Head, typename... Tail>
constexpr auto* GetIf(Root&& root, Head&& head, Tail&&... tail) {
  if constexpr (impl::kIsPointerLike<Root>) {
    return root ? utils::GetIf(*std::forward<Root>(root),
                               std::forward<Head>(head),
                               std::forward<Tail>(tail)...)
                : nullptr;
  } else {
    return utils::GetIf(
        std::invoke(std::forward<Head>(head), std::forward<Root>(root)),
        std::forward<Tail>(tail)...);
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
