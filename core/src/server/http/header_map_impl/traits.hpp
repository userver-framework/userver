#pragma once

#include <limits>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

struct Traits final {
  using Key = std::string;
  using Value = std::string;

  using Size = std::uint16_t;
  using HashValue = std::uint16_t;

  static constexpr std::size_t kMaxSize = 1 << 15;
  static_assert(kMaxSize < std::numeric_limits<Size>::max());
};

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
