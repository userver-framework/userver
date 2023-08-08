#pragma once

/// @file userver/utils/get_if.hpp
/// @brief @copybrief utils::GetIf

#include <functional>
#include <memory>
#include <utility>

#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
using IsPointerLike =
    decltype(std::declval<T&>() ? std::addressof(*std::declval<T&&>())
                                : nullptr);

template <typename T>
inline constexpr bool kIsPointerLike = meta::kIsDetected<IsPointerLike, T>;

#define UPARENS ()

#define UEXPAND(...) UEXPAND4(UEXPAND4(UEXPAND4(UEXPAND4(__VA_ARGS__))))
#define UEXPAND4(...) UEXPAND3(UEXPAND3(UEXPAND3(UEXPAND3(__VA_ARGS__))))
#define UEXPAND3(...) UEXPAND2(UEXPAND2(UEXPAND2(UEXPAND2(__VA_ARGS__))))
#define UEXPAND2(...) UEXPAND1(UEXPAND1(UEXPAND1(UEXPAND1(__VA_ARGS__))))
#define UEXPAND1(...) __VA_ARGS__

#define UFOR_EACH(macro, ...)                                    \
  __VA_OPT__(UEXPAND(UFOR_EACH_HELPER(macro, __VA_ARGS__)))
#define UFOR_EACH_HELPER(macro, arg, ...)                         \
  macro(arg)                                                     \
  __VA_OPT__(UFOR_EACH_AGAIN UPARENS (macro, __VA_ARGS__))
#define UFOR_EACH_AGAIN() UFOR_EACH_HELPER

#define UMEMBER_ACCESS(member) , \
[]<typename T>(T* clazz) constexpr -> auto* \
{ \
    return utils::GetIf(clazz, &T::member); \
}

template<typename Prev, typename Curr, typename... Next>
constexpr auto* OptDeref(Prev* prev, Curr curr, Next... next)
{
    auto* clazz = curr(prev);
    if constexpr (sizeof...(next) == 0) {
        return clazz;
    } else {
        return OptDeref(clazz, next...);
    }
}

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
/// @snippet universal/src/utils/get_if_test.cpp Sample Usage
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

/// @brief Dereferences a chain of indirections and compositions,
/// returns nullptr if one of the chain elements is not set
///
/// @snippet universal/src/utils/get_if_test.cpp Sample Usage

#define UOPT_DEREF(first, ...) \
    utils::impl::OptDeref(utils::GetIf(first) UFOR_EACH(UMEMBER_ACCESS, __VA_ARGS__))

}  // namespace utils

USERVER_NAMESPACE_END
