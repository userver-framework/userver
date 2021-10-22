#include <userver/taxi_config/snapshot.hpp>

#include <userver/taxi_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

Snapshot::Snapshot(const impl::StorageData& storage)
    : container_(storage.config.Read()) {}

}  // namespace taxi_config

USERVER_NAMESPACE_END
