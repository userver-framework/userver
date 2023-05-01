#pragma once
#include <optional>
#include <userver/utils/impl/span.hpp>
#include <vector>

namespace userver::websocket {

std::optional<std::vector<std::byte>> deflate(
    const utils::impl::Span<const std::byte>& in) noexcept;
}
