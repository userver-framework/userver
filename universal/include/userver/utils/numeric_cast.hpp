#pragma once

/// @file userver/utils/numeric_cast.hpp
/// @brief @copybrief utils::numeric_cast

#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
using PrintableValue = std::conditional_t<(sizeof(T) > 1), T, int>;

}

/// Detects loss of range when a numeric type is converted, and throws an
/// exception if the range cannot be preserved
///
/// ## Example usage:
///
/// @snippet utils/numeric_cast_test.cpp  Sample utils::numeric_cast usage
template <typename U, typename T>
constexpr U numeric_cast(T input) {
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_integral_v<U>);

  U result = input;
  if (static_cast<T>(result) != input || ((result < 0) != (input < 0))) {
    throw std::runtime_error(fmt::format(
        "Failed to convert {} {} into {} due to integer overflow",
        compiler::GetTypeName<T>(), static_cast<impl::PrintableValue<T>>(input),
        compiler::GetTypeName<U>()));
  }
  return result;
}

}  // namespace utils

USERVER_NAMESPACE_END
