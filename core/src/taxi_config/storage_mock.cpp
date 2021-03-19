#include <taxi_config/storage_mock.hpp>

namespace taxi_config {

StorageMock::StorageMock(const DocsMap& docs_map)
    : storage_(std::make_unique<const impl::Storage>(Config::Parse(docs_map))) {
}

}  // namespace taxi_config
