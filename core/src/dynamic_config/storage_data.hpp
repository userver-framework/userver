#pragma once

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/snapshot_impl.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

struct StorageData final {
  rcu::Variable<SnapshotData> config;
  concurrent::AsyncEventChannel<const Snapshot&> channel{"taxi-config"};
};

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
