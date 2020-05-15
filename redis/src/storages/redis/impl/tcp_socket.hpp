#pragma once

#include <chrono>
#include <optional>

namespace redis {

std::optional<std::chrono::microseconds> GetSocketPeerRtt(int fd);

}  // namespace redis
