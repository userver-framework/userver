#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

inline constexpr std::chrono::seconds kDefaultMaxDisconnectTime{35};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
