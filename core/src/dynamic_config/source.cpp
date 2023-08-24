#include <userver/dynamic_config/source.hpp>

#include <optional>

#include <dynamic_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

Source::Source(impl::StorageData& storage) : storage_(&storage) {}

Snapshot Source::GetSnapshot() const { return Snapshot{*storage_}; }

Source::SnapshotEventSource& Source::GetEventChannel() {
  return storage_->GetChannel();
}

concurrent::AsyncEventSubscriberScope Source::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    SnapshotEventSource::Function&& func) {
  return storage_->DoUpdateAndListen(id, name, std::move(func));
}

concurrent::AsyncEventSubscriberScope Source::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    DiffEventSource::Function&& func) {
  return storage_->DoUpdateAndListen(id, name, std::move(func));
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
