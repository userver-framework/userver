#include <userver/dynamic_config/storage_mock.hpp>

#include <utility>

#include <dynamic_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

StorageMock::StorageMock() : StorageMock(std::initializer_list<KeyValue>{}) {}

StorageMock::StorageMock(std::initializer_list<KeyValue> config_variables)
    : storage_(new impl::StorageData{impl::SnapshotData{config_variables}}) {}

StorageMock::StorageMock(const std::vector<KeyValue>& config_variables)
    : storage_(new impl::StorageData{impl::SnapshotData{config_variables}}) {}

StorageMock::StorageMock(const DocsMap& defaults,
                         const std::vector<KeyValue>& overrides)
    : storage_(new impl::StorageData{impl::SnapshotData{defaults, overrides}}) {
}

StorageMock::StorageMock(StorageMock&&) noexcept = default;

StorageMock& StorageMock::operator=(StorageMock&&) noexcept = default;

StorageMock::~StorageMock() = default;

Source StorageMock::GetSource() const& {
  UASSERT(storage_);
  return Source{*storage_};
}

Snapshot StorageMock::GetSnapshot() const& { return GetSource().GetSnapshot(); }

void StorageMock::Extend(const std::vector<KeyValue>& overrides) {
  UASSERT(storage_);
  const auto old_config = storage_->config.Read();
  storage_->config.Assign(impl::SnapshotData{*old_config, overrides});
  storage_->channel.SendEvent(GetSnapshot());
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
