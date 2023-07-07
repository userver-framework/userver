#pragma once

/// @file userver/utils/underlying_value.hpp
/// @brief @copybrief utils::UnderlyingValue

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Function that extracts integral value from enum or StrongTypedef
template <class T>
constexpr auto UnderlyingValue(T v) noexcept {
  static_assert(std::is_enum<T>::value,
                "UnderlyingValue works only with enums or StrongTypedef");

  return static_cast<std::underlying_type_t<T>>(v);
}

}  // namespace utils

USERVER_NAMESPACE_END
