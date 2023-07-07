#pragma once

/// @file userver/utils/optionals.hpp
/// @brief Helper functions for std optionals

#include <optional>
#include <string>

#include <fmt/compile.h>
#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Converts std::optional to a string, empty value represented as "--"
template <class T>
std::string ToString(const std::optional<T>& from) {
  return from ? fmt::format(FMT_COMPILE(" {}"), *from) : "--";
}

}  // namespace utils

USERVER_NAMESPACE_END
