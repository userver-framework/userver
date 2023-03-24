#pragma once

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

struct StorageData final {
  rcu::Variable<SnapshotData> config;
  concurrent::AsyncEventChannel<const Snapshot&> channel{
      "dynamic-config", [this](auto& function) {
        const auto snapshot = Snapshot{*this};
        if (!snapshot.GetData().IsEmpty()) function(snapshot);
      }};
};

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
