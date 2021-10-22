#pragma once

#include <string_view>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utils {

/// @brief A simple constexpr parser splitting a string by dot.
/// Doesn't do any checks (yet).
constexpr std::pair<std::string_view, std::string_view> ParseDBName(
    std::string_view db_name) {
  const std::size_t pos = db_name.find('.');
  if (pos == std::string_view::npos) return {{}, db_name};
  return {db_name.substr(0, pos), db_name.substr(pos + 1)};
}

}  // namespace storages::postgres::utils

USERVER_NAMESPACE_END
