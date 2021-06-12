#include <taxi_config/storage_mock.hpp>

namespace taxi_config {

StorageMock::StorageMock() : StorageMock({}, {}) {}

StorageMock::StorageMock(std::initializer_list<KeyValue> config_variables)
    : storage_(std::make_unique<impl::Storage>(Config{config_variables})) {}

StorageMock::StorageMock(const DocsMap& defaults,
                         const std::vector<KeyValue>& overrides)
    : storage_(std::make_unique<impl::Storage>(Config{defaults, overrides})) {}

StorageMock::StorageMock(const DocsMap& docs_map) : StorageMock(docs_map, {}) {}

taxi_config::Source StorageMock::GetSource() const& {
  UASSERT(storage_);
  return Source{*storage_};
}

void StorageMock::Patch(const std::vector<KeyValue>& overrides) {
  UASSERT(storage_);
  const auto old_config = storage_->config.Read();
  storage_->config.Assign(Config{*old_config, overrides});
  storage_->channel.SendEvent(GetSource().GetSnapshot());
}

Source StorageMock::operator*() const { return GetSource(); }

impl::SourceHolder StorageMock::operator->() const { return {GetSource()}; }

}  // namespace taxi_config
