#pragma once

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

extern const dynamic_config::Key<dynamic_config::ValueDict<PoolSettings>> kPoolSettings;

extern const dynamic_config::Key<std::chrono::milliseconds> kDefaultMaxTime;

extern const dynamic_config::Key<bool> kDeadlinePropagationEnabled;

extern const dynamic_config::Key<bool> kCongestionControlEnabled;

extern const dynamic_config::Key<dynamic_config::ValueDict<bool>> kCongestionControlDatabasesSettings;

}  // namespace storages::mongo

USERVER_NAMESPACE_END
