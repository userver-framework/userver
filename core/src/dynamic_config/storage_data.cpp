#include <dynamic_config/storage_data.hpp>

#include <mutex>
#include <optional>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

StorageData::StorageData(SnapshotData config)
    : config_(std::move(config)),
      snapshot_channel_("dynamic-config-snapshot",
                        [&](auto& func) {
                          const auto snapshot = GetSnapshot();
                          if (!snapshot.GetData().IsEmpty()) func(snapshot);
                        }),
      diff_channel_("dynamic-config-diff", [&](auto& func) {
        auto snapshot = GetSnapshot();
        if (snapshot.GetData().IsEmpty()) return;
        const Diff diff{std::nullopt, std::move(snapshot)};
        func(diff);
      }) {}

StorageData::StorageData() : StorageData(SnapshotData{}) {}

rcu::ReadablePtr<SnapshotData> StorageData::Read() const {
  return config_.Read();
}

void StorageData::Update(SnapshotData config,
                         AfterAssignHook after_assign_hook) {
  std::lock_guard lock(update_mutex_);

  std::optional<Snapshot> previous_config;
  {
    Snapshot current_config(*this);
    if (!current_config.GetData().IsEmpty())
      previous_config = std::move(current_config);
  }

  config_.Assign(std::move(config));
  after_assign_hook();

  const Diff diff{std::move(previous_config), GetSnapshot()};
  diff_channel_.SendEvent(diff);
  snapshot_channel_.SendEvent(GetSnapshot());
}

StorageData::SnapshotChannel& StorageData::GetChannel() {
  return snapshot_channel_;
}

concurrent::AsyncEventSubscriberScope StorageData::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    SnapshotChannel::Function&& func) {
  auto updater = [&, func_copy = func] { func_copy(GetSnapshot()); };
  return snapshot_channel_.DoUpdateAndListen(id, name, std::move(func),
                                             std::move(updater));
}

concurrent::AsyncEventSubscriberScope StorageData::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    DiffChannel::Function&& func) {
  // Must be locked to avoid the following case:
  //
  // 1. assign the new config (let's call it 'new_config') to our `config_` in
  // `StorageData::Update`.
  // 2. the execution context is switched.
  // 3. call `func_copy(diff{std::nullopt, GetSnapshot()})`, i.e. with
  // `std::nullopt` and 'new_config'.
  // 4. the execution context is switched back to `StorageData::Update`.
  // 5. call `diff_channel_.SendEvent(diff{previous_config, GetSnapshot()})`,
  // i.e. with 'new_config' as second param.
  //
  // As a result, the subscriber receives two `Diff` object with 'new_config' as
  // a `current` field.
  std::lock_guard lock(update_mutex_);

  auto updater = [&, func_copy = func] {
    const Diff diff{std::nullopt, GetSnapshot()};
    func_copy(diff);
  };
  return diff_channel_.DoUpdateAndListen(id, name, std::move(func),
                                         std::move(updater));
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
