#include <userver/storages/rocks/snapshot.hpp>
#include <rocksdb/options.h>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

namespace detail {

template <IteratorDirection D>
Iterator<D> detail::SnapshotRangeImpl<SnapshotRangeLayout::kDefault>::NewIterator() const {
    rocksdb::ReadOptions options{};
    return {snapshot_impl_.NewIterator(options)};
}

template Iterator<IteratorDirection::kForward>
detail::SnapshotRangeImpl<SnapshotRangeLayout::kDefault>::NewIterator() const;
template Iterator<IteratorDirection::kBackward>
detail::SnapshotRangeImpl<SnapshotRangeLayout::kDefault>::NewIterator() const;

template <IteratorDirection D>
Iterator<D> detail::SnapshotRangeImpl<SnapshotRangeLayout::kColumnFamily>::NewIterator() const {
    rocksdb::ReadOptions options{};
    return {snapshot_impl_.NewIterator(options, column_family_)};
}

template Iterator<IteratorDirection::kForward>
detail::SnapshotRangeImpl<SnapshotRangeLayout::kColumnFamily>::NewIterator() const;
template Iterator<IteratorDirection::kBackward>
detail::SnapshotRangeImpl<SnapshotRangeLayout::kColumnFamily>::NewIterator() const;

}  // namespace detail

std::optional<std::string> Snapshot::Get(std::string_view key) const {
    rocksdb::ReadOptions options{};
    return snapshot_impl_.Get(options, key);
}

std::optional<std::string> Snapshot::Get(ColumnFamilyHandle column_family, std::string_view key) const {
    rocksdb::ReadOptions options{};
    return snapshot_impl_.Get(options, column_family, key);
}

template <IteratorDirection D>
Iterator<D> Snapshot::NewIterator() const {
    rocksdb::ReadOptions options{};
    return {snapshot_impl_.NewIterator(options)};
}

template Iterator<IteratorDirection::kForward>
Snapshot::NewIterator() const;
template Iterator<IteratorDirection::kBackward>
Snapshot::NewIterator() const;

template <IteratorDirection D>
Iterator<D> Snapshot::NewIterator(ColumnFamilyHandle column_family) const {
    rocksdb::ReadOptions options{};
    return {snapshot_impl_.NewIterator(options, column_family)};
}

template Iterator<IteratorDirection::kForward>
Snapshot::NewIterator(ColumnFamilyHandle column_family) const;
template Iterator<IteratorDirection::kBackward>
Snapshot::NewIterator(ColumnFamilyHandle column_family) const;

}  // namespace storages::rocks

USERVER_NAMESPACE_END
