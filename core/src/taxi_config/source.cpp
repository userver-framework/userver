#include <taxi_config/source.hpp>

namespace taxi_config {

Source::Source(impl::StorageData& storage) : storage_(&storage) {}

Snapshot Source::GetSnapshot() const { return Snapshot{*storage_}; }

concurrent::AsyncEventChannel<const Snapshot&>& Source::GetEventChannel() {
  return storage_->channel;
}

}  // namespace taxi_config
