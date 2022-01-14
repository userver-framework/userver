#pragma once

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/snapshot_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config::impl {

struct StorageData final {
  rcu::Variable<SnapshotData> config;
  concurrent::AsyncEventChannel<const Snapshot&> channel{"taxi-config"};
};

}  // namespace taxi_config::impl

USERVER_NAMESPACE_END
