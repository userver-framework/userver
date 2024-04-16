#pragma once

/// @file userver/utils/optionals.hpp
/// @brief Helper functions for std optionals
/// @ingroup userver_universal

#include <optional>
#include <string>
#include <utility>

#include <fmt/compile.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Converts std::optional to a string, empty value represented as "--"
template <class T>
std::string ToString(const std::optional<T>& from) {
  return from ? fmt::format(FMT_COMPILE(" {}"), *from) : "--";
}

/// A polyfill for C++23 monadic operations for `std::optional`.
template <typename T, typename Func>
auto OptionalTransform(T&& opt, Func func)
    -> std::optional<decltype(std::move(func)(*std::forward<T>(opt)))> {
  if (opt) return std::move(func)(*std::forward<T>(opt));
  return std::nullopt;
}

}  // namespace utils

USERVER_NAMESPACE_END
