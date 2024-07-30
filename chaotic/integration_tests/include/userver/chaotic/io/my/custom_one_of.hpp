#pragma once

#include <string>
#include <variant>

#include <userver/chaotic/convert.hpp>

namespace my {

struct CustomOneOf {
  CustomOneOf() = default;

  CustomOneOf(const std::variant<int, std::string>& value) : a(value) {}

  std::variant<int, std::string> a;
};

inline bool operator==(const CustomOneOf& lhs, const CustomOneOf& rhs) {
  return lhs.a == rhs.a;
}

inline std::variant<int, std::string> Convert(
    const CustomOneOf& str,
    USERVER_NAMESPACE::chaotic::convert::To<std::variant<int, std::string>>) {
  return str.a;
}

}  // namespace my
