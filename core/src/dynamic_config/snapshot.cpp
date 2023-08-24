#include <userver/dynamic_config/snapshot.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/rcu/rcu.hpp>

#include <dynamic_config/storage_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

struct Snapshot::Impl final {
  explicit Impl(const impl::StorageData& storage) : data_ptr(storage.Read()) {}

  rcu::ReadablePtr<impl::SnapshotData> data_ptr;
};

Snapshot::Snapshot(const Snapshot&) = default;

Snapshot::Snapshot(Snapshot&&) noexcept = default;

Snapshot& Snapshot::operator=(const Snapshot&) = default;

Snapshot& Snapshot::operator=(Snapshot&&) noexcept = default;

Snapshot::~Snapshot() = default;

Snapshot::Snapshot(const impl::StorageData& storage) : impl_(storage) {}

const impl::SnapshotData& Snapshot::GetData() const { return *impl_->data_ptr; }

}  // namespace dynamic_config

USERVER_NAMESPACE_END
