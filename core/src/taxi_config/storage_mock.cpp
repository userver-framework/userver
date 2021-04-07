#include <taxi_config/storage_mock.hpp>

namespace taxi_config {

StorageMock::StorageMock(const DocsMap& docs_map)
    : storage_(std::make_unique<const impl::Storage>(Config{docs_map})),
      source_(*storage_) {}

StorageMock::StorageMock(
    std::initializer_list<Config::KeyValue> config_variables)
    : storage_(std::make_unique<const impl::Storage>(Config{config_variables})),
      source_(*storage_) {}

const Source& StorageMock::operator*() const { return source_; }

const Source* StorageMock::operator->() const { return &source_; }

}  // namespace taxi_config
