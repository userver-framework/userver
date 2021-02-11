#include <taxi_config/test_helpers.hpp>

namespace taxi_config {

Storage::Storage(const DocsMap& docs_map)
    : storage_(std::make_shared<const Config>(Config::Parse(docs_map))) {}

std::shared_ptr<const Config> Storage::GetShared() const {
  return storage_.ReadCopy();
}

}  // namespace taxi_config
