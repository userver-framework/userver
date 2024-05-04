#pragma once

#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
using PrintableValue = std::conditional_t<(sizeof(T) > 1), T, int>;

}

template <typename U, typename T>
U numeric_cast(T input) {
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
