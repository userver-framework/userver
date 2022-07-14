#pragma once

#include <string>
#include <string_view>

#include <logging/dynamic_debug.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

struct PathAndLine {
  std::string path;
  int line;
};

inline PathAndLine SplitLocation(std::string_view location) {
  const auto separator_pos = location.find_last_of(':');

  if (separator_pos == std::string::npos) {
    return PathAndLine{std::string{location}, logging::kAnyLine};
  }

  auto path = std::string{location.substr(0, separator_pos)};
  const auto line = location.substr(separator_pos + 1);
  return PathAndLine{std::move(path),
                     utils::FromString<int>(std::string{line})};
}

}  // namespace logging

USERVER_NAMESPACE_END
