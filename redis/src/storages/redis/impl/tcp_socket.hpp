#pragma once

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace redis {

std::optional<std::chrono::microseconds> GetSocketPeerRtt(int fd);

}  // namespace redis

USERVER_NAMESPACE_END
