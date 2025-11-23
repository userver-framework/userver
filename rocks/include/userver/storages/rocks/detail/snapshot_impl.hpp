#pragma once

/// @file userver/storages/rocks/detail/snapshot_impl.hpp
/// @brief @copybrief storages::rocks::detail::SnapshotImpl

#include <memory>
#include <string>
#include <optional>
#include <string_view>
#include <userver/storages/rocks/column_family.hpp>
#include <userver/storages/rocks/detail/iterator_impl.hpp>

namespace rocksdb {
class Snapshot;
class ReadOptions;
class ColumnFamilyHandle;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

class DbImpl;

class SnapshotImpl final {
public:
    SnapshotImpl(const std::shared_ptr<DbImpl>& db, const rocksdb::Snapshot* snapshot);

    [[nodiscard]] std::optional<std::string> Get(rocksdb::ReadOptions& options, std::string_view key) const;
    [[nodiscard]] std::optional<std::string> Get(rocksdb::ReadOptions& options,
            rocksdb::ColumnFamilyHandle* column_family, std::string_view key) const;

    [[nodiscard]] IteratorImpl NewIterator(rocksdb::ReadOptions& options) const;
    [[nodiscard]] IteratorImpl NewIterator(rocksdb::ReadOptions& options,
            rocksdb::ColumnFamilyHandle* column_family) const;

private:
    std::shared_ptr<detail::DbImpl> db_impl_;
    std::shared_ptr<const rocksdb::Snapshot> snapshot_;
};

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
