#pragma once

/// @file userver/logging/log_helper_extras.hpp
/// @brief Logging for less popular types from std and boost

#include <tuple>

#include <userver/logging/log_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

template <typename... Types>
LogHelper& operator<<(LogHelper& lh, const std::tuple<Types...>& value) {
  lh << "(";
  if constexpr (sizeof...(Types) != 0) {
    lh << std::get<0>(value);
    if constexpr (sizeof...(Types) > 1) {
      std::apply([&lh](auto&&, auto&&... v) { ((lh << ", " << v), ...); },
                 value);
    }
  }
  lh << ")";
  return lh;
}

}  // namespace logging

USERVER_NAMESPACE_END
