#pragma once

#include <optional>
#include <string>

#include <userver/chaotic/convert.hpp>

namespace my {

struct CustomAllOf {
  CustomAllOf() = default;

  template <typename T>
  CustomAllOf(T&& value)
      : a(std::move(value.field1)), b(std::move(value.field2)) {}

  CustomAllOf(std::string&& a, std::string&& b)
      : a(std::move(a)), b(std::move(b)) {}

  std::optional<std::string> a;
  std::optional<std::string> b;
};

inline bool operator==(const CustomAllOf& lhs, const CustomAllOf& rhs) {
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

template <typename T>
T Convert(const CustomAllOf& value,
          USERVER_NAMESPACE::chaotic::convert::To<T>) {
  T t;
  t.field1 = value.a;
  t.field2 = value.b;
  return t;
}

}  // namespace my
