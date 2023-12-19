#pragma once

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

extern const dynamic_config::Key<std::chrono::milliseconds> kDefaultMaxTime;

extern const dynamic_config::Key<bool> kDeadlinePropagationEnabled;

extern const dynamic_config::Key<bool> kCongestionControlEnabled;

}  // namespace storages::mongo

USERVER_NAMESPACE_END
