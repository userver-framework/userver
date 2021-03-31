#include <taxi_config/storage_mock.hpp>

namespace taxi_config {

StorageMock::StorageMock(const DocsMap& docs_map)
    : storage_(std::make_unique<const impl::Storage>(Config{docs_map})) {}

StorageMock::StorageMock(
    std::initializer_list<Config::KeyValue> config_variables)
    : storage_(
          std::make_unique<const impl::Storage>(Config{config_variables})) {}

}  // namespace taxi_config
