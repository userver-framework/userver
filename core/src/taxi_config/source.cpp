#include <userver/taxi_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

Source::Source(impl::StorageData& storage) : storage_(&storage) {}

Snapshot Source::GetSnapshot() const { return Snapshot{*storage_}; }

concurrent::AsyncEventChannel<const Snapshot&>& Source::GetEventChannel() {
  return storage_->channel;
}

}  // namespace taxi_config

USERVER_NAMESPACE_END
