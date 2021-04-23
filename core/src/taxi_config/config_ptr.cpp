#include <taxi_config/config_ptr.hpp>

#include <components/component_context.hpp>
#include <taxi_config/storage/component.hpp>

namespace taxi_config {

namespace impl {

Storage::Storage(Config config)
    : config(std::move(config)), channel("taxi-config") {}

impl::Storage& FindStorage(const components::ComponentContext& context) {
  auto& component = context.FindComponent<components::TaxiConfig>();
  component.Get();  // wait for the first update to finish
  return *component.cache_;
}

}  // namespace impl

const Config& SnapshotPtr::operator*() const& { return *container_; }

const Config* SnapshotPtr::operator->() const& { return &*container_; }

Source::Source(const components::ComponentContext& context)
    : storage_(&impl::FindStorage(context)) {}

Source::Source(impl::Storage& storage) : storage_(&storage) {}

SnapshotPtr Source::GetSnapshot() const { return SnapshotPtr{*storage_}; }

utils::AsyncEventChannel<const SnapshotPtr&>& Source::GetEventChannel() {
  return storage_->channel;
}

}  // namespace taxi_config
