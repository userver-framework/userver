#pragma once
#include <userver/utils/impl/span.hpp>
#include <optional>
#include <string_view>
#include <vector>

namespace userver::websocket {

struct EncodeResult {
  std::string_view encoding;
  std::vector<std::byte> encoded_data;
};

EncodeResult encode(const utils::impl::Span<const std::byte>& in,
                    const std::string& accept_encoding) noexcept;
}  // namespace userver::websocket
