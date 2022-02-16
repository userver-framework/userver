#include <dynamic_config/storage_data.hpp>
#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

Source::Source(impl::StorageData& storage) : storage_(&storage) {}

Snapshot Source::GetSnapshot() const { return Snapshot{*storage_}; }

Source::EventSource& Source::GetEventChannel() { return storage_->channel; }

concurrent::AsyncEventSubscriberScope Source::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  auto func_copy = func;
  return storage_->channel.DoUpdateAndListen(id, name, std::move(func),
                                             [&] { func_copy(GetSnapshot()); });
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
