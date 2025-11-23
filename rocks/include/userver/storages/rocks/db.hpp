#pragma once

/// @file userver/storages/rocks/db.hpp
/// @brief @copybrief storages::rocks::Db

#include <memory>
#include <string>
#include <optional>
#include <string_view>
#include <userver/storages/rocks/snapshot.hpp>
#include <userver/storages/rocks/column_family.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks::detail {
class DbImpl;
}  // namespace storages::rocks::detail

namespace storages::rocks {

class WriteBatch;

struct DbOptions final {
    std::optional<std::string_view> compression;
    std::optional<int> compression_level;
    std::optional<std::string_view> bottommost_compression;
    std::optional<int> bottommost_compression_level;
    std::optional<bool> use_direct_reads;
    std::optional<bool> use_direct_io_for_flush_and_compaction;
};

class Db final {
public:
    Db(const std::string& db_path, int max_background_jobs, const std::vector<std::string>& column_families,
            const DbOptions& db_options, engine::TaskProcessor& task_processor);

    [[nodiscard]] ColumnFamilyHandle GetColumnFamily(const std::string& name) const;

    void Put(std::string_view key, std::string_view value);
    void Put(ColumnFamilyHandle column_family, std::string_view key, std::string_view value);

    void Delete(std::string_view key);
    void Delete(ColumnFamilyHandle column_family, std::string_view key);

    void Write(WriteBatch& write_batch);

    [[nodiscard]] std::optional<std::string> Get(std::string_view key) const;
    [[nodiscard]] std::optional<std::string> Get(ColumnFamilyHandle column_family, std::string_view key) const;

    [[nodiscard]] Snapshot GetSnapshot() const;

private:
    std::shared_ptr<detail::DbImpl> db_impl_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
