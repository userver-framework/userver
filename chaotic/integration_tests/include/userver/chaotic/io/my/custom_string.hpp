#pragma once

#include <string>

#include <userver/chaotic/convert.hpp>

namespace my {

// Definition of a custom user structure
struct CustomString final {
  CustomString(std::string&& s) : s(s) {}

  std::string s;
};

inline bool operator==(const CustomString& lhs, const CustomString& rhs) {
  return lhs.s == rhs.s;
}

// Convert must be located:
// 1) either in T's namespace (my) for user types,
// 2) or in chaotic::convert namespace for third-party libraries types

// The CustomString -> std::string Convert() is used during serialization
// (CustomString -> json::Value)
inline std::string Convert(
    const CustomString& str,
    USERVER_NAMESPACE::chaotic::convert::To<std::string>) {
  return str.s;
}

// The std::string -> CustomString Convert() is used during parsing
// (json::Value -> CustomString)
inline CustomString Convert(
    std::string&& str, USERVER_NAMESPACE::chaotic::convert::To<CustomString>) {
  return CustomString(std::move(str));
}

}  // namespace my
