#pragma once

/// @file userver/storages/rocks/detail/db_impl.hpp
/// @brief @copybrief storages::rocks::detail::DbImpl

#include <memory>
#include <string>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <userver/storages/rocks/column_family.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

namespace rocksdb {
class Options;
class WriteOptions;
class WriteBatch;
class ReadOptions;
class Iterator;
class Snapshot;
class DB;
class ColumnFamilyHandle;
}  // namespace rocksdb

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {

class DbImpl final {
public:
    DbImpl(const rocksdb::Options& options, const std::string& db, const std::vector<std::string>& column_families,
            engine::TaskProcessor& task_processor);
    ~DbImpl();

    [[nodiscard]] rocksdb::ColumnFamilyHandle* GetColumnFamily(const std::string& name) const;

    void Put(const rocksdb::WriteOptions& options, std::string_view key, std::string_view value);
    void Put(const rocksdb::WriteOptions& options, rocksdb::ColumnFamilyHandle* column_family, std::string_view key,
            std::string_view value);

    void Delete(const rocksdb::WriteOptions& options, std::string_view key);
    void Delete(const rocksdb::WriteOptions& options, rocksdb::ColumnFamilyHandle* column_family, std::string_view key);

    void Write(const rocksdb::WriteOptions& options, rocksdb::WriteBatch& write_batch);

    [[nodiscard]] std::optional<std::string> Get(const rocksdb::ReadOptions& options, std::string_view key) const;
    [[nodiscard]] std::optional<std::string> Get(const rocksdb::ReadOptions& options,
            rocksdb::ColumnFamilyHandle* column_family, std::string_view key) const;

    [[nodiscard]] std::unique_ptr<rocksdb::Iterator> NewIterator(const rocksdb::ReadOptions& options) const;
    [[nodiscard]] std::unique_ptr<rocksdb::Iterator> NewIterator(const rocksdb::ReadOptions& options,
            rocksdb::ColumnFamilyHandle* column_family) const;

    [[nodiscard]] const rocksdb::Snapshot* GetSnapshot() const;
    void ReleaseSnapshot(const rocksdb::Snapshot* snapshot) const;

    [[nodiscard]] engine::TaskProcessor& GetTaskProcessor();

private:
    std::unique_ptr<rocksdb::DB> db_;
    engine::TaskProcessor& task_processor_;
    std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*> column_family_handles_;
};

}  // namespace storages::rocks::detail

USERVER_NAMESPACE_END
