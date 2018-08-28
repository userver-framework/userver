#pragma once

#include <chrono>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace meta {

template <class T>
struct is_bool : std::false_type {};
template <>
struct is_bool<bool> : std::true_type {};

template <template <typename...> class Template, typename T>
struct is_instantiation_of : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template, Template<Args...> > : std::true_type {};

template <class T>
struct is_duration : is_instantiation_of<std::chrono::duration, T> {};

template <class T>
struct is_vector : is_instantiation_of<std::vector, T> {};

template <class T>
struct is_map {
  static constexpr bool value =
      is_instantiation_of<std::unordered_map, T>::value ||
      is_instantiation_of<std::map, T>::value;
};

}  // namespace meta
