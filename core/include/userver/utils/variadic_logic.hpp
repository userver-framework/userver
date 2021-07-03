#pragma once

#include <type_traits>

namespace utils {

// TODO use std::conjunciton/disjunction when they are available in the library

/// Implementation of std::conjunciton (logical AND)
template <typename... T>
struct And;

/// @cond
template <typename T, typename U, typename... V>
struct And<T, U, V...> {
 private:
  using PrevResult = typename And<T, U>::type;
  static constexpr auto prev_value = PrevResult::value;

 public:
  using type =
      std::conditional_t<prev_value, typename And<PrevResult, V...>::type,
                         PrevResult>;
};
template <typename T, T A, T B>
struct And<std::integral_constant<T, A>, std::integral_constant<T, B>>
    : std::integral_constant<T, B && A> {};
template <typename T, T A>
struct And<std::integral_constant<T, A>> : std::integral_constant<T, A> {};
/// @endcond

/// Implementation of std::disjunction (logical OR)
template <typename... T>
struct Or;

/// @cond
template <typename T, typename U, typename... V>
struct Or<T, U, V...> {
 private:
  using PrevResult = typename And<T, U>::type;
  static constexpr auto prev_value = PrevResult::value;

 public:
  using type = std::conditional_t<prev_value, PrevResult,
                                  typename Or<PrevResult, V...>::type>;
};
template <typename T, T A, T B>
struct Or<std::integral_constant<T, A>, std::integral_constant<T, B>>
    : std::integral_constant<T, B || A> {};
template <typename T, T A>
struct Or<std::integral_constant<T, A>> : std::integral_constant<T, A> {};
/// @endcond
}  // namespace utils
