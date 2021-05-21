#include <taxi_config/config_ptr.hpp>

namespace taxi_config {

namespace impl {

Storage::Storage(Config config)
    : config(std::move(config)), channel("taxi-config") {}

}  // namespace impl

const Config& SnapshotPtr::operator*() const& { return *container_; }

const Config* SnapshotPtr::operator->() const& { return &*container_; }

Source::Source(impl::Storage& storage) : storage_(&storage) {}

SnapshotPtr Source::GetSnapshot() const { return SnapshotPtr{*storage_}; }

utils::AsyncEventChannel<const SnapshotPtr&>& Source::GetEventChannel() {
  return storage_->channel;
}

}  // namespace taxi_config
