#pragma once

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

std::optional<std::chrono::microseconds> GetSocketPeerRtt(int fd);

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
