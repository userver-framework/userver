#pragma once

#include <limits>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace http::headers::header_map {

struct Traits final {
  using Size = std::uint16_t;
  using HashValue = std::uint16_t;
  using HeaderIndex = std::int8_t;

  static constexpr std::size_t kMaxSize = 1 << 15;
  static_assert(kMaxSize < std::numeric_limits<Size>::max());
};

}  // namespace http::headers::header_map

USERVER_NAMESPACE_END
