#pragma once

#include <chrono>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace meta {

template <class T>
struct is_bool : std::false_type {};
template <>
struct is_bool<bool> : std::true_type {};

template <template <typename...> class Template, typename T>
struct is_instantiation_of : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

template <template <typename...> class Template, typename... Args>
inline constexpr bool is_instantiation_of_v =
    is_instantiation_of<Template, Args...>::value;

template <class T>
struct is_duration : is_instantiation_of<std::chrono::duration, T> {};

template <class T>
struct is_vector : is_instantiation_of<std::vector, T> {};

template <class T>
struct is_array {
  template <typename X, size_t V>
  static constexpr auto check_is_array(const std::array<X, V>&)
      -> std::true_type;
  static constexpr auto check_is_array(...) -> std::false_type;

  static constexpr bool value =
      decltype(check_is_array(std::declval<T>()))::value;
};

template <class T>
struct is_set {
  static constexpr bool value = is_instantiation_of_v<std::set, T> ||
                                is_instantiation_of_v<std::unordered_set, T>;
};

template <class T>
struct is_map {
  static constexpr bool value = is_instantiation_of_v<std::unordered_map, T> ||
                                is_instantiation_of_v<std::map, T>;
};

template <class T>
struct is_optional : is_instantiation_of<std::optional, T> {};

/// Returns `true` if the type is a fundamental character type.
/// `signed char` and `unsigned char` are not character types.
template <typename T>
struct is_character
    : std::integral_constant<bool, std::is_same_v<T, char> ||
                                       std::is_same_v<T, wchar_t> ||
                                       std::is_same_v<T, char16_t> ||
                                       std::is_same_v<T, char32_t>> {};

template <typename T>
inline constexpr bool is_character_v = is_character<T>::value;

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`).
/// `signed char` and `unsigned char` are integer types.
template <typename T>
struct is_integer : std::integral_constant<bool, std::is_integral_v<T> &&
                                                     !is_character_v<T> &&
                                                     !std::is_same_v<T, bool>> {
};

/// Returns `true` if the type is a true integer type (not `*char*` or `bool`)
/// `signed char` and `unsigned char` are integer types
template <typename T>
inline constexpr bool is_integer_v = is_integer<T>::value;

}  // namespace meta
