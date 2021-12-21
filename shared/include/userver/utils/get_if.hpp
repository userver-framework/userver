#pragma once

/// @file utils/get_if.hpp
/// @brief @copybrief utils::GetIf

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename>
struct DecomposePointerToMember;

template <typename M, typename C>
struct DecomposePointerToMember<M C::*> {
  using MemberType = M;
  using ClassType = C;
};

}  // namespace impl

template <typename Leaf, typename Root>
constexpr decltype(auto) GetIf(Root& root) {
  if constexpr (!std::is_same_v<std::remove_cv_t<Root>, Leaf>) {
    return root ? USERVER_NAMESPACE::utils::GetIf<Leaf>(*root) : nullptr;
  } else {
    return &root;
  }
}

/// @brief Dereferences a chain of indirections and compositions,
/// returns nullptr if one of the chain elements is not set
/// @snippet utils/get_if_test.cpp Sample GetIf Usage
template <typename Leaf, auto head, auto... tail, typename Root>
constexpr decltype(auto) GetIf(Root& root) {
  using HeadClass =
      typename impl::DecomposePointerToMember<decltype(head)>::ClassType;
  if constexpr (!std::is_same_v<std::remove_cv_t<Root>, HeadClass>) {
    return root ? USERVER_NAMESPACE::utils::GetIf<Leaf, head, tail...>(*root)
                : nullptr;
  } else if constexpr (sizeof...(tail) > 0) {
    return USERVER_NAMESPACE::utils::GetIf<Leaf, tail...>(root.*head);
  } else {
    return USERVER_NAMESPACE::utils::GetIf<Leaf>(root.*head);
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
