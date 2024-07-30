#pragma once

#include <stdexcept>
#include <string>

#include <userver/utils/regex.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <const std::string_view& Regex>
struct Pattern final {
  static const utils::regex kRegex;

  static void Validate(const std::string& value) {
    if (!utils::regex_search(value, kRegex))
      throw std::runtime_error("doesn't match regex");
  }
};

template <const std::string_view& Regex>
inline const utils::regex Pattern<Regex>::kRegex{std::string{Regex}};

}  // namespace chaotic

USERVER_NAMESPACE_END
