#pragma once

/// @file utils/underlying_value.hpp
/// @brief @copybrief utils::UnderlyingValue

#include <type_traits>

namespace utils {

/// Function that extracts integral value from enum or StrongTypedef
template <class T>
constexpr auto UnderlyingValue(T v) {
  static_assert(std::is_enum<T>::value,
                "UnderlyingValue works only with enums or StrongTypedef");

  return static_cast<std::underlying_type_t<T>>(v);
}

}  // namespace utils
