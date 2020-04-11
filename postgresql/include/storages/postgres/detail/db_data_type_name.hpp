#pragma once

#include <utility>

#include <utils/algo.hpp>
#include <utils/string_view.hpp>

namespace storages::postgres::utils {

/// @brief A simple constexpr parser splitting a string by dot.
/// Doesn't do any checks (yet).
constexpr std::pair<::utils::string_view, ::utils::string_view> ParseDBName(
    const char* db_name) {
  std::size_t pos{0};
  for (; db_name[pos] != 0; ++pos) {
    if (db_name[pos] == '.') {
      break;
    }
  }
  if (db_name[pos] == 0) {
    // No dot found
    return {::utils::string_view{}, ::utils::string_view{db_name, pos}};
  }
  auto name_start = db_name + pos + 1;
  return {::utils::string_view{db_name, pos},
          ::utils::string_view{name_start, ::utils::StrLen(name_start)}};
}

}  // namespace storages::postgres::utils
