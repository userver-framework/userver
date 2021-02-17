#include <taxi_config/storage_mock.hpp>

namespace taxi_config {

StorageMock::StorageMock(const DocsMap& docs_map)
    : storage_(std::make_unique<const impl::Storage>(
          std::make_shared<const Config>(Config::Parse(docs_map)))) {}

std::shared_ptr<const Config> StorageMock::GetShared() const {
  return storage_->ReadCopy();
}

}  // namespace taxi_config
