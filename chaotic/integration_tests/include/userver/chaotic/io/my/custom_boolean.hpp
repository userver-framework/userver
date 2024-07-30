#pragma once

#include <userver/chaotic/convert.hpp>

namespace my {

struct CustomBoolean final {
  explicit CustomBoolean(bool b) : b(b) {}

  bool b;
};

inline bool operator==(const CustomBoolean& lhs, const CustomBoolean& rhs) {
  return lhs.b == rhs.b;
}

inline bool Convert(const CustomBoolean& b,
                    USERVER_NAMESPACE::chaotic::convert::To<bool>) {
  return b.b;
}

}  // namespace my
