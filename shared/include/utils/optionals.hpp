#pragma once

/// @file utils/optionals.hpp
/// @brief Helper functions for std optionals

#include <optional>
#include <string>

#include <fmt/format.h>

namespace utils {

template <class T>
std::string ToString(const std::optional<T>& from) {
  return from ? fmt::format(" {}", *from) : "--";
}

}  // namespace utils
