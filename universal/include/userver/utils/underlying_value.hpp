#pragma once

/// @file userver/utils/underlying_value.hpp
/// @brief @copybrief utils::UnderlyingValue
/// @ingroup userver_universal

#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Function that extracts integral value from enum or StrongTypedef
template <class T, std::enable_if_t<std::is_enum_v<T>, int> /*Enable*/ = 0>
constexpr auto UnderlyingValue(T v) noexcept {
  return static_cast<std::underlying_type_t<T>>(v);
}

}  // namespace utils

USERVER_NAMESPACE_END
