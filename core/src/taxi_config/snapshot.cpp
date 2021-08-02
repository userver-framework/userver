#include <userver/taxi_config/snapshot.hpp>

#include <userver/taxi_config/storage_mock.hpp>

namespace taxi_config {

Snapshot::Snapshot(const impl::StorageData& storage)
    : container_(storage.config.ReadShared()) {}

const Snapshot& Snapshot::operator*() const& { return *this; }

const Snapshot* Snapshot::operator->() const& { return this; }

}  // namespace taxi_config
