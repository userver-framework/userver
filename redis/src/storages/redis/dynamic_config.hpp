#pragma once

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

extern const dynamic_config::Key<int> kDeadlinePropagationVersion;

inline constexpr int kDeadlinePropagationExperimentVersion = 1;

extern const dynamic_config::Key<bool> kRedisAutoTopologyEnabled;

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
