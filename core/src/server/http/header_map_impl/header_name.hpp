#pragma once

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

void ToLowerCaseInPlace(std::string& header_name);

std::string ToLowerCase(const std::string& header_name);

// TODO : think about this
constexpr bool IsLowerCase(std::string_view header_name) {
  bool fail = false;
  // This is vectorized, with early return it's not.
  for (const auto c : header_name) {
    fail |= c >= 'A' && c <= 'Z';
  }

  return !fail;
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
