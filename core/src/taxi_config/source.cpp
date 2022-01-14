#include <userver/taxi_config/source.hpp>

#include <taxi_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

Source::Source(impl::StorageData& storage) : storage_(&storage) {}

Snapshot Source::GetSnapshot() const { return Snapshot{*storage_}; }

Source::EventSource& Source::GetEventChannel() { return storage_->channel; }

concurrent::AsyncEventSubscriberScope Source::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  return storage_->channel.DoUpdateAndListen(
      id, name, std::move(func), [&, func] { func(GetSnapshot()); });
}

}  // namespace taxi_config

USERVER_NAMESPACE_END
