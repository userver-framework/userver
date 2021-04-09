#include <taxi_config/config_ptr.hpp>

#include <components/component_context.hpp>
#include <taxi_config/storage/component.hpp>

namespace taxi_config {

namespace impl {

const impl::Storage& FindStorage(const components::ComponentContext& context) {
  const auto& component = context.FindComponent<components::TaxiConfig>();
  component.Get();  // wait for the first update to finish
  return *component.cache_;
}

}  // namespace impl

const Config& SnapshotPtr::operator*() const& { return *container_; }

const Config* SnapshotPtr::operator->() const& { return &*container_; }

Source::Source(const components::ComponentContext& context)
    : storage_(&impl::FindStorage(context)) {}

Source::Source(const impl::Storage& storage) : storage_(&storage) {}

SnapshotPtr Source::GetSnapshot() const { return SnapshotPtr{*storage_}; }

}  // namespace taxi_config
