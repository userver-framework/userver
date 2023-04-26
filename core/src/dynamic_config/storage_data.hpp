#pragma once

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

class StorageData final {
 public:
  using SnapshotChannel = concurrent::AsyncEventChannel<const Snapshot&>;
  using DiffChannel = concurrent::AsyncEventChannel<const Diff&>;
  using AfterAssignHook = std::function<void()>;

  StorageData();
  explicit StorageData(SnapshotData config);

  rcu::ReadablePtr<SnapshotData> Read() const;

  void Update(SnapshotData config, AfterAssignHook after_assign_hook);

  SnapshotChannel& GetChannel();

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      SnapshotChannel::Function&& func);

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      DiffChannel::Function&& func);

 private:
  Snapshot GetSnapshot() { return Snapshot{*this}; }

  rcu::Variable<SnapshotData> config_;
  SnapshotChannel snapshot_channel_;
  DiffChannel diff_channel_;

  engine::Mutex update_mutex_;
};

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
