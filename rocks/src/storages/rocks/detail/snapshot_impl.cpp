#include <userver/storages/rocks/detail/snapshot_impl.hpp>
#include <userver/storages/rocks/detail/db_impl.hpp>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

SnapshotImpl::SnapshotImpl(const std::shared_ptr<DbImpl>& db, const rocksdb::Snapshot* snapshot)
    : db_impl_{db}, snapshot_(snapshot, [db_impl_ = db.get()](const auto* snapshot) {
        // Be careful with the order of destruction. This is done to avoid a single atomic increment =)
        db_impl_->ReleaseSnapshot(snapshot);
    }) {}

std::optional<std::string> SnapshotImpl::Get(rocksdb::ReadOptions& options, std::string_view key) const {
    options.snapshot = snapshot_.get();
    return db_impl_->Get(options, key);
}

std::optional<std::string> SnapshotImpl::Get(rocksdb::ReadOptions& options, rocksdb::ColumnFamilyHandle* column_family,
        std::string_view key) const {
    options.snapshot = snapshot_.get();
    return db_impl_->Get(options, column_family, key);
}

IteratorImpl SnapshotImpl::NewIterator(rocksdb::ReadOptions& options) const {
    options.snapshot = snapshot_.get();
    return detail::IteratorImpl{db_impl_, snapshot_, db_impl_->NewIterator(options)};
}

IteratorImpl SnapshotImpl::NewIterator(rocksdb::ReadOptions& options,
        rocksdb::ColumnFamilyHandle* column_family) const {
    options.snapshot = snapshot_.get();
    return detail::IteratorImpl{db_impl_, snapshot_, db_impl_->NewIterator(options, column_family)};
}

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
